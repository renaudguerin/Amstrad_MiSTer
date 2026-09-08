// Amstrad Plus SNA v3 "CPC+" chunk parser and loader (Phase P8).
//
// Interprets the optional "CPC+" chunk in SNA v3 snapshot files,
// restoring ASIC sprite RAM, sprite attributes, 12-bit palette entries,
// raster/interrupt control registers, sound DMA registers, and ASIC lock state.
//
// Reference: docs/references/Snapshot (.SNA) file format.md

module plus_sna_parser
(
	input             clk,
	input             reset,

	input             sna_download,
	input             cpc_plus_chunk_start, // asserted when "CPC+" chunk header is decoded
	input             cpc_plus_byte_wr,    // strobe on each incoming chunk payload byte
	input      [7:0]  cpc_plus_byte_data,  // incoming payload byte
	output            ioctl_wait,          // throttle HPS when write FIFO fills
	output            busy,                // high while download active or FIFO draining

	// Interface to asic_regs
	output reg        asic_sna_wr,
	output reg [13:0] asic_sna_addr,
	output reg  [7:0] asic_sna_data,

	// Interface to plus_mmu / asic_unlock
	output reg        asic_sna_active,     // goes high if a valid CPC+ chunk is found
	output reg  [7:0] asic_sna_rmr2,
	output reg        asic_sna_unlock,

	// B8-5 slice A: CPC+ DMA internal-state shadows (chunk 0x8E0-0x8F4,
	// three seven-byte records) and unlock sequence state (0x8F7).
	// SNA format map: per channel LE16 loop count (lower 12 bits), LE16
	// loop address, LE16 pause count (lower 12 bits), prescaler8.
	output [11:0] asic_sna_loop_cnt0,
	output [11:0] asic_sna_loop_cnt1,
	output [11:0] asic_sna_loop_cnt2,
	output [15:0] asic_sna_loop_addr0,
	output [15:0] asic_sna_loop_addr1,
	output [15:0] asic_sna_loop_addr2,
	output [11:0] asic_sna_pause_cnt0,
	output [11:0] asic_sna_pause_cnt1,
	output [11:0] asic_sna_pause_cnt2,
	output  [7:0] asic_sna_pause_presc0,
	output  [7:0] asic_sna_pause_presc1,
	output  [7:0] asic_sna_pause_presc2,
	output  [4:0] asic_sna_seq_state
);

	reg [11:0] sna_loop_cnt [0:2];
	reg [15:0] sna_loop_addr [0:2];
	reg [11:0] sna_pause_cnt [0:2];
	reg  [7:0] sna_pause_presc [0:2];
	reg  [4:0] sna_seq_state_r;
	reg  [7:0] pair_lo;

	reg [15:0] chunk_byte_cnt;
	reg        sna_download_d;

	// 8-entry FIFO for unpacking writes to asic_regs
	reg [21:0] fifo_mem [0:7];
	reg [3:0]  fifo_wr_ptr;
	reg [3:0]  fifo_rd_ptr;

	wire [2:0] fifo_wr_idx0 = fifo_wr_ptr[2:0];
	wire [2:0] fifo_wr_idx1 = fifo_wr_ptr[2:0] + 3'd1;
	wire [2:0] fifo_rd_idx  = fifo_rd_ptr[2:0];

	wire [3:0] fifo_count = fifo_wr_ptr - fifo_rd_ptr;
	wire       fifo_empty = (fifo_wr_ptr == fifo_rd_ptr);

	assign asic_sna_loop_cnt0    = sna_loop_cnt[0];
	assign asic_sna_loop_cnt1    = sna_loop_cnt[1];
	assign asic_sna_loop_cnt2    = sna_loop_cnt[2];
	assign asic_sna_loop_addr0   = sna_loop_addr[0];
	assign asic_sna_loop_addr1   = sna_loop_addr[1];
	assign asic_sna_loop_addr2   = sna_loop_addr[2];
	assign asic_sna_pause_cnt0   = sna_pause_cnt[0];
	assign asic_sna_pause_cnt1   = sna_pause_cnt[1];
	assign asic_sna_pause_cnt2   = sna_pause_cnt[2];
	assign asic_sna_pause_presc0 = sna_pause_presc[0];
	assign asic_sna_pause_presc1 = sna_pause_presc[1];
	assign asic_sna_pause_presc2 = sna_pause_presc[2];
	assign asic_sna_seq_state    = sna_seq_state_r;

	// Channel and sub-offset of a 0x8E0-0x8F4 internal-register byte.
	// The record boundaries (0x8E0/0x8E7/0x8EE) select the channel; the
	// low 3 bits relative to the record base select the sub-offset 0..6
	// with 3-bit wrap, keeping the 7-record framing without division.
	wire [1:0] sna_int_ch = (chunk_byte_cnt >= 16'h08EE) ? 2'd2 :
	                        (chunk_byte_cnt >= 16'h08E7) ? 2'd1 : 2'd0;
	wire [2:0] sna_int_base_l3 = (sna_int_ch == 2'd2) ? 3'd6 :
	                             (sna_int_ch == 2'd1) ? 3'd7 : 3'd0;
	wire [2:0] sna_int_sub = chunk_byte_cnt[2:0] - sna_int_base_l3;
	// A sprite payload byte expands to two writes.  The producer can already
	// have two accepted bytes in flight when it observes ioctl_wait, so assert
	// with five physical slots left: both bytes fit even without relying on a
	// simultaneous dequeue, and one slot remains to keep the ring unambiguous.
	assign ioctl_wait     = (fifo_count >= 4'd3);
	// The top-level registers each accepted payload byte and its strobe.  The
	// last registered strobe can therefore reach this module one clock after
	// sna_download falls; keep the lifecycle busy and accept that tail byte.
	assign busy           = sna_download || cpc_plus_byte_wr ||
	                        !fifo_empty || asic_sna_wr;

	always @(posedge clk) begin
		if (reset) begin
			chunk_byte_cnt  <= 16'd0;
			sna_download_d  <= 1'b0;
			asic_sna_wr     <= 1'b0;
			asic_sna_addr   <= 14'd0;
			asic_sna_data   <= 8'd0;
			asic_sna_active <= 1'b0;
			asic_sna_rmr2   <= 8'd0;
			asic_sna_unlock <= 1'b0;
			sna_loop_cnt[0]  <= 12'd0;
			sna_loop_cnt[1]  <= 12'd0;
			sna_loop_cnt[2]  <= 12'd0;
			sna_loop_addr[0] <= 16'd0;
			sna_loop_addr[1] <= 16'd0;
			sna_loop_addr[2] <= 16'd0;
			sna_pause_cnt[0] <= 12'd0;
			sna_pause_cnt[1] <= 12'd0;
			sna_pause_cnt[2] <= 12'd0;
			sna_pause_presc[0] <= 8'd0;
			sna_pause_presc[1] <= 8'd0;
			sna_pause_presc[2] <= 8'd0;
			sna_seq_state_r  <= 5'd0;
			pair_lo          <= 8'd0;
			fifo_wr_ptr     <= 4'd0;
			fifo_rd_ptr     <= 4'd0;
		end
		else begin
			sna_download_d <= sna_download;

			// A later classic snapshot must not inherit the prior CPC+ shadow
			// state when it contains no CPC+ chunk of its own.  A new download
			// also aborts any residual write tail from the previous snapshot;
			// the top-level pulses plus_asic_reset on this same edge, so neither
			// the queued entries nor an already-presented asic_sna_wr can leak
			// into the new ASIC image.
			if (sna_download && !sna_download_d) begin
				chunk_byte_cnt  <= 16'd0;
				asic_sna_wr     <= 1'b0;
				asic_sna_active <= 1'b0;
				asic_sna_rmr2   <= 8'd0;
				asic_sna_unlock <= 1'b0;
				sna_loop_cnt[0]  <= 12'd0;
				sna_loop_cnt[1]  <= 12'd0;
				sna_loop_cnt[2]  <= 12'd0;
				sna_loop_addr[0] <= 16'd0;
				sna_loop_addr[1] <= 16'd0;
				sna_loop_addr[2] <= 16'd0;
				sna_pause_cnt[0] <= 12'd0;
				sna_pause_cnt[1] <= 12'd0;
				sna_pause_cnt[2] <= 12'd0;
				sna_pause_presc[0] <= 8'd0;
				sna_pause_presc[1] <= 8'd0;
				sna_pause_presc[2] <= 8'd0;
				sna_seq_state_r  <= 5'd0;
				pair_lo          <= 8'd0;
				fifo_wr_ptr     <= 4'd0;
				fifo_rd_ptr     <= 4'd0;
			end

			if (cpc_plus_chunk_start) begin
				chunk_byte_cnt  <= 16'd0;
				asic_sna_active <= 1'b1;
				fifo_wr_ptr     <= 4'd0;
				fifo_rd_ptr     <= 4'd0;
			end

			// Enqueue registered payload strobes even on the first clock after
			// sna_download falls; that is the production pipeline's tail.
			if (cpc_plus_byte_wr) begin
				chunk_byte_cnt <= chunk_byte_cnt + 16'd1;

				if (chunk_byte_cnt < 16'h0800) begin
					// Sprite Bitmaps (0x000-0x7FF): packed 2 pixels per byte
					// Push high nibble (pixel 0) and low nibble (pixel 1)
					fifo_mem[fifo_wr_idx0] <= {{2'b00, chunk_byte_cnt[10:0], 1'b0}, {4'b0, cpc_plus_byte_data[7:4]}};
					fifo_mem[fifo_wr_idx1] <= {{2'b00, chunk_byte_cnt[10:0], 1'b1}, {4'b0, cpc_plus_byte_data[3:0]}};
					fifo_wr_ptr <= fifo_wr_ptr + 4'd2;
				end
				else if (chunk_byte_cnt >= 16'h0800 && chunk_byte_cnt < 16'h0880) begin
					// Sprite Attributes (0x800-0x87F): 16 sprites × 8 bytes at &6000-&607F
					fifo_mem[fifo_wr_idx0] <= {14'h2000 | {7'd0, chunk_byte_cnt[6:0]}, cpc_plus_byte_data};
					fifo_wr_ptr <= fifo_wr_ptr + 4'd1;
				end
				else if (chunk_byte_cnt >= 16'h0880 && chunk_byte_cnt < 16'h08C0) begin
					// Palette (0x880-0x8BF): 32 entries × 2 bytes at &6400-&643F
					fifo_mem[fifo_wr_idx0] <= {14'h2400 | {8'd0, chunk_byte_cnt[5:0]}, cpc_plus_byte_data};
					fifo_wr_ptr <= fifo_wr_ptr + 4'd1;
				end
				else if (chunk_byte_cnt >= 16'h08C0 && chunk_byte_cnt <= 16'h08C5) begin
					// Control registers &6800-&6805 (PRI, SPLT, SSA hi, SSA lo, SSCR, IVR)
					fifo_mem[fifo_wr_idx0] <= {14'h2800 + {10'd0, (chunk_byte_cnt[3:0] - 4'h0)}, cpc_plus_byte_data};
					fifo_wr_ptr <= fifo_wr_ptr + 4'd1;
				end
				else if (chunk_byte_cnt >= 16'h08D0 && chunk_byte_cnt <= 16'h08DB) begin
					// Sound DMA channel attributes (0x8D0-0x8DB) at &6C00-&6C0B
					fifo_mem[fifo_wr_idx0] <= {14'h2C00 + {10'd0, (chunk_byte_cnt[3:0] - 4'h0)}, cpc_plus_byte_data};
					fifo_wr_ptr <= fifo_wr_ptr + 4'd1;
				end
				else if (chunk_byte_cnt == 16'h08DF) begin
					// DCSR at &6C0F
					fifo_mem[fifo_wr_idx0] <= {14'h2C0F, cpc_plus_byte_data};
					fifo_wr_ptr <= fifo_wr_ptr + 4'd1;
				end
				else if (chunk_byte_cnt == 16'h08F5) begin
					// Gate array A0 register value (RMR2)
					asic_sna_rmr2 <= cpc_plus_byte_data;
				end
				else if (chunk_byte_cnt == 16'h08F6) begin
					// Gate array A0 lock (0=locked, 1=unlocked)
					asic_sna_unlock <= cpc_plus_byte_data[0];
				end
				else if (chunk_byte_cnt == 16'h08F7) begin
					// ASIC unlock sequence state: the next expected byte
					// (SNA format note 10; 0..16 fit 5 bits).
					sna_seq_state_r <= cpc_plus_byte_data[4:0];
				end
				else if (chunk_byte_cnt >= 16'h08E0 &&
				         chunk_byte_cnt <= 16'h08F4) begin
					// DMA channel internal registers: each record is
					// LE16 loop count (12 meaningful bits), LE16 loop
					// address, LE16 pause count (12 bits), prescaler8.
					// The pause field is a remaining prescaled tick count
					// and the prescaler field its remaining phase
					// (audit: Arnold V PAUSE + runtime, B8-5 doc).
					case (sna_int_sub)
					3'd0: pair_lo <= cpc_plus_byte_data;
					3'd1: sna_loop_cnt[sna_int_ch] <=
						{cpc_plus_byte_data[3:0], pair_lo};
					3'd2: pair_lo <= cpc_plus_byte_data;
					3'd3: sna_loop_addr[sna_int_ch] <=
						{cpc_plus_byte_data, pair_lo};
					3'd4: pair_lo <= cpc_plus_byte_data;
					3'd5: sna_pause_cnt[sna_int_ch] <=
						{cpc_plus_byte_data[3:0], pair_lo};
					3'd6: sna_pause_presc[sna_int_ch] <=
						cpc_plus_byte_data;
					default: ;
					endcase
				end
			end

			// Dequeue writes to asic_regs.  A new snapshot edge has priority:
			// fifo_empty still reflects the old pointers during this clock, so
			// allowing the normal branch would re-present one stale write even
			// though the restart block above cleared the queue.
			if (sna_download && !sna_download_d) begin
				asic_sna_wr <= 1'b0;
			end
			else if (!fifo_empty) begin
				asic_sna_wr   <= 1'b1;
				asic_sna_addr <= fifo_mem[fifo_rd_idx][21:8];
				asic_sna_data <= fifo_mem[fifo_rd_idx][7:0];
				fifo_rd_ptr   <= fifo_rd_ptr + 4'd1;
			end
			else begin
				asic_sna_wr <= 1'b0;
			end
		end
	end

endmodule
