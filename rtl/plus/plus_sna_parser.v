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

	// Interface to asic_regs
	output reg        asic_sna_wr,
	output reg [13:0] asic_sna_addr,
	output reg  [7:0] asic_sna_data,

	// Interface to plus_mmu / asic_unlock
	output reg        asic_sna_active,     // goes high if a valid CPC+ chunk is found
	output reg  [7:0] asic_sna_rmr2,
	output reg        asic_sna_unlock
);

	reg [15:0] chunk_byte_cnt;
	reg [7:0]  pending_pixel_low;
	reg        pending_pixel_wr;
	reg [11:0] pending_pixel_addr;

	always @(posedge clk) begin
		if (reset || !sna_download) begin
			chunk_byte_cnt   <= 16'd0;
			asic_sna_wr      <= 1'b0;
			asic_sna_addr    <= 14'd0;
			asic_sna_data    <= 8'd0;
			asic_sna_active  <= 1'b0;
			asic_sna_rmr2    <= 8'd0;
			asic_sna_unlock  <= 1'b0;
			pending_pixel_wr <= 1'b0;
		end
		else begin
			asic_sna_wr <= 1'b0;

			// Handle the second pixel nibble write from the previous byte
			if (pending_pixel_wr) begin
				pending_pixel_wr <= 1'b0;
				asic_sna_wr      <= 1'b1;
				asic_sna_addr    <= {2'b00, pending_pixel_addr};
				asic_sna_data    <= pending_pixel_low;
			end

			if (cpc_plus_chunk_start) begin
				chunk_byte_cnt  <= 16'd0;
				asic_sna_active <= 1'b1;
			end

			if (cpc_plus_byte_wr) begin
				chunk_byte_cnt <= chunk_byte_cnt + 16'd1;

				if (chunk_byte_cnt < 16'h0800) begin
					// Sprite Bitmaps (0x000-0x7FF): packed 2 pixels per byte
					// First pixel (bits 7:4) written immediately
					asic_sna_wr      <= 1'b1;
					asic_sna_addr    <= {2'b00, chunk_byte_cnt[10:0], 1'b0};
					asic_sna_data    <= {4'b0, cpc_plus_byte_data[7:4]};
					// Second pixel (bits 3:0) queued for the next cycle
					pending_pixel_low  <= {4'b0, cpc_plus_byte_data[3:0]};
					pending_pixel_addr <= {chunk_byte_cnt[10:0], 1'b1};
					pending_pixel_wr   <= 1'b1;
				end
				else if (chunk_byte_cnt >= 16'h0800 && chunk_byte_cnt < 16'h0880) begin
					// Sprite Attributes (0x800-0x87F): 16 sprites × 8 bytes at &6000-&607F
					asic_sna_wr   <= 1'b1;
					asic_sna_addr <= 14'h2000 | {7'd0, chunk_byte_cnt[6:0]};
					asic_sna_data <= cpc_plus_byte_data;
				end
				else if (chunk_byte_cnt >= 16'h0880 && chunk_byte_cnt < 16'h08C0) begin
					// Palette (0x880-0x8BF): 32 entries × 2 bytes at &6400-&643F
					asic_sna_wr   <= 1'b1;
					asic_sna_addr <= 14'h2400 | {8'd0, chunk_byte_cnt[5:0]};
					asic_sna_data <= cpc_plus_byte_data;
				end
				else if (chunk_byte_cnt >= 16'h08C0 && chunk_byte_cnt <= 16'h08C5) begin
					// Control registers &6800-&6805 (PRI, SPLT, SSA hi, SSA lo, SSCR, IVR)
					asic_sna_wr   <= 1'b1;
					asic_sna_addr <= 14'h2800 + {10'd0, (chunk_byte_cnt[3:0] - 4'h0)};
					asic_sna_data <= cpc_plus_byte_data;
				end
				else if (chunk_byte_cnt >= 16'h08D0 && chunk_byte_cnt <= 16'h08DB) begin
					// Sound DMA channel attributes (0x8D0-0x8DB) at &6C00-&6C0B
					asic_sna_wr   <= 1'b1;
					asic_sna_addr <= 14'h2C00 + {10'd0, (chunk_byte_cnt[3:0] - 4'h0)};
					asic_sna_data <= cpc_plus_byte_data;
				end
				else if (chunk_byte_cnt == 16'h08DF) begin
					// DCSR at &6C0F
					asic_sna_wr   <= 1'b1;
					asic_sna_addr <= 14'h2C0F;
					asic_sna_data <= cpc_plus_byte_data;
				end
				else if (chunk_byte_cnt == 16'h08F5) begin
					// Gate array A0 register value (RMR2)
					asic_sna_rmr2 <= cpc_plus_byte_data;
				end
				else if (chunk_byte_cnt == 16'h08F6) begin
					// Gate array A0 lock (0=locked, 1=unlocked)
					asic_sna_unlock <= cpc_plus_byte_data[0];
				end
			end
		end
	end

endmodule
