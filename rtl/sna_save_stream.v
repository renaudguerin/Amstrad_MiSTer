//============================================================================
//  SNA snapshot DDR3 save stream (B18 slice 4b).
//
//  Transport decision:
//  The core writes the snapshot into a MiSTer save-state-layout DDR3 slot at
//  0x3E000000 and a host script pulls it over SSH via /dev/mem; the C64-style
//  ioctl_upload_req route was considered and is not preferable, because MiSTer
//  Main polls UIO 0x3C only for C64/C128 and arcade NVRAM with bespoke per-core
//  handlers, so it would need new Amstrad-specific Main code, and Main's
//  automatic save-state copy to SD needs an FS load that would take over SD
//  slot 0, which is drive A here (see docs/b18-sna-save.md "Why not SD directly").
//
//  Publication order:
//  1. word 0 = 64'hFFFF_FFFF_FFFF_FFFF (invalid generation)
//  2. payload words from word 2: 256 header bytes (32 words) + RAM pages
//     8 per 64-bit word, little-endian
//  3. word 1 = {32'd0, file_bytes / 4} (length in 32-bit words: 0x8040 or 0x4040)
//  4. word 0 = {32'd0, generation} (generation counter 1.., never all ones)
//
//  Reference: docs/b18-sna-save.md, "Transport", "Memory capture".
//============================================================================

module sna_save_stream
(
	input                 clk,
	input                 reset,
	input                 start,          // one-clock pulse: sna_save_capture.captured
	input                 ram128,         // 1: dump 128K (8 pages), 0: 64K (4 pages)
	input          [1:0]  bank,           // SDRAM bank of the running model
	input       [2047:0]  header,         // sna_save_capture.header, byte o at [o*8 +: 8]
	input                 clkref,         // ce_ref, the SDRAM slot reference (Amstrad.sv:125-133)

	// SDRAM held-request port (rtl/sdram.v cart_*), read-only use
	output reg            cart_req,
	output         [1:0]  cart_bank,
	output reg    [22:0]  cart_addr,
	input          [7:0]  cart_dout,
	input                 cart_ack,

	// DDR3 Avalon-style master (same shape as rtl/ssm_marker.v ddram_*)
	input                 ddr_grant,      // from the mux: this master owns DDRAM_*
	output reg            ddr_request,    // to the mux: wants ownership
	output reg    [28:0]  ddram_addr,     // word address (byte address >> 3)
	output reg    [63:0]  ddram_din,
	output         [7:0]  ddram_be,       // 8'hFF
	output         [7:0]  ddram_burstcnt, // 8'd1
	output reg            ddram_we,
	input                 ddram_busy,

	output reg            done,           // one-clock pulse: publication finished (drives release_req)
	output                active
);

	localparam [28:0] BASE_WORD = 29'h07C0_0000; // 32'h3E00_0000 >> 3

	assign ddram_be       = 8'hFF;
	assign ddram_burstcnt = 8'd1;

	localparam [3:0] S_IDLE              = 4'd0;
	localparam [3:0] S_WAIT_GRANT        = 4'd1;
	localparam [3:0] S_WRITE_INVALID_GEN = 4'd2;
	localparam [3:0] S_WAIT_INVALID_GEN  = 4'd3;
	localparam [3:0] S_WRITE_HDR_WORD    = 4'd4;
	localparam [3:0] S_WAIT_HDR_WORD     = 4'd5;
	localparam [3:0] S_RAM_REQ           = 4'd6;
	localparam [3:0] S_RAM_WAIT_ACK      = 4'd7;
	localparam [3:0] S_WRITE_RAM_WORD    = 4'd8;
	localparam [3:0] S_WAIT_RAM_WORD     = 4'd9;
	localparam [3:0] S_WRITE_LEN         = 4'd10;
	localparam [3:0] S_WAIT_LEN          = 4'd11;
	localparam [3:0] S_WRITE_GEN         = 4'd12;
	localparam [3:0] S_WAIT_GEN          = 4'd13;
	localparam [3:0] S_QUIESCE           = 4'd14;

	reg [3:0] state;
	assign active = (state != S_IDLE);

	// Latched parameters for the current save operation. The header is not
	// copied: sna_save_capture holds it stable until this stream's `done`
	// releases the freeze.
	reg          saved_ram128;
	reg   [1:0]  saved_bank;

	assign cart_bank = saved_bank;

	// Progress counters
	reg  [4:0] hdr_word_idx;       // 0..31 (32 words = 256 header bytes)
	reg  [2:0] ram_page;           // 0..(saved_ram128 ? 7 : 3)
	reg [13:0] ram_offset;         // 0..0x3FF8 step 8
	reg  [2:0] byte_idx;           // 0..7 within current 64-bit RAM word
	reg [14:0] payload_word_idx;   // word offset from BASE_WORD + 2 (32..0x401F or 32..0x201F)
	reg [63:0] word_buf;           // collects 8 bytes for DDR write

	// 32-bit generation counter (starts at 1, increments per completed save,
	// never all ones). It survives core reset on purpose: the host detects a
	// new save by a changed generation, and resets are routine (OSD reset,
	// model change), so restarting at 1 could repeat a value already seen.
	reg [31:0] generation;

	// Fairness yield tracker:
	// After each cart_ack, cart_req must stay low through the clock that sees
	// clkref rising edge and the one after it (q == STATE_IDLE arbitration).
	localparam [1:0] Y_IDLE     = 2'd0;
	localparam [1:0] Y_WAIT_REF = 2'd1;
	localparam [1:0] Y_WAIT_ARB = 2'd2;

	reg [1:0] yield_state;
	reg clkref_d;
	wire clkref_rising = clkref && !clkref_d;

	always @(posedge clk) begin
		clkref_d <= clkref;

		if (reset) begin
			cart_req    <= 1'b0;
			yield_state <= Y_IDLE;
			done        <= 1'b0;
			if (ddram_we && ddram_busy) begin
				// In-flight DDR write cannot be dropped prematurely (violates Avalon-MM waitrequest)
				state <= S_QUIESCE;
			end else begin
				state       <= S_IDLE;
				ddr_request <= 1'b0;
				ddram_we    <= 1'b0;
			end
		end else if (state == S_QUIESCE) begin
			// Holding stalled write during reset quiesce
			if (!ddram_busy) begin
				ddram_we    <= 1'b0;
				ddr_request <= 1'b0;
				state       <= S_IDLE;
			end
		end else begin
			done <= 1'b0;

			// Advance fairness yield tracker
			case (yield_state)
				Y_IDLE: begin
					// Ready for cart_req
				end
				Y_WAIT_REF: begin
					if (clkref_rising) begin
						yield_state <= Y_WAIT_ARB;
					end
				end
				Y_WAIT_ARB: begin
					// During this cycle, sdram.v evaluates q == STATE_IDLE with cart_req == 0.
					// At the next posedge, arbitration is finished.
					yield_state <= Y_IDLE;
				end
				default: yield_state <= Y_IDLE;
			endcase

			case (state)
				S_IDLE: begin
					ddram_we <= 1'b0;
					cart_req <= 1'b0;
					if (start) begin
						saved_ram128 <= ram128;
						saved_bank   <= bank;
						ddr_request  <= 1'b1;
						state        <= S_WAIT_GRANT;
					end
				end

				S_WAIT_GRANT: begin
					if (ddr_grant) begin
						// Step 1: word 0 = 64'hFFFF_FFFF_FFFF_FFFF (invalid generation)
						ddram_addr <= BASE_WORD + 29'd0;
						ddram_din  <= 64'hFFFF_FFFF_FFFF_FFFF;
						ddram_we   <= 1'b1;
						state      <= S_WAIT_INVALID_GEN;
					end
				end

				S_WAIT_INVALID_GEN: begin
					if (!ddram_busy) begin
						ddram_we     <= 1'b0;
						hdr_word_idx <= 5'd0;
						state        <= S_WRITE_HDR_WORD;
					end
				end

				S_WRITE_HDR_WORD: begin
					ddram_addr <= BASE_WORD + 29'd2 + {24'd0, hdr_word_idx};
					ddram_din  <= header[hdr_word_idx*64 +: 64];
					ddram_we   <= 1'b1;
					state      <= S_WAIT_HDR_WORD;
				end

				S_WAIT_HDR_WORD: begin
					if (!ddram_busy) begin
						ddram_we <= 1'b0;
						if (hdr_word_idx == 5'd31) begin
							// Header publication complete, begin SDRAM RAM dump
							ram_page         <= 3'd0;
							ram_offset       <= 14'd0;
							byte_idx         <= 3'd0;
							payload_word_idx <= 15'd32;
							state            <= S_RAM_REQ;
						end else begin
							hdr_word_idx <= hdr_word_idx + 5'd1;
							state        <= S_WRITE_HDR_WORD;
						end
					end
				end

				S_RAM_REQ: begin
					// Raise request only when fairness yield condition is met
					if (yield_state == Y_IDLE) begin
						cart_addr <= {9'd8 + {6'd0, ram_page}, ram_offset[13:3], byte_idx};
						cart_req  <= 1'b1;
						state     <= S_RAM_WAIT_ACK;
					end
				end

				S_RAM_WAIT_ACK: begin
					if (cart_ack) begin
						word_buf[byte_idx*8 +: 8] <= cart_dout;
						cart_req                  <= 1'b0;
						yield_state               <= Y_WAIT_REF;
						if (byte_idx == 3'd7) begin
							state <= S_WRITE_RAM_WORD;
						end else begin
							byte_idx <= byte_idx + 3'd1;
							state    <= S_RAM_REQ;
						end
					end
				end

				S_WRITE_RAM_WORD: begin
					ddram_addr <= BASE_WORD + 29'd2 + {14'd0, payload_word_idx};
					ddram_din  <= word_buf;
					ddram_we   <= 1'b1;
					state      <= S_WAIT_RAM_WORD;
				end

				S_WAIT_RAM_WORD: begin
					if (!ddram_busy) begin
						ddram_we         <= 1'b0;
						payload_word_idx <= payload_word_idx + 15'd1;
						byte_idx         <= 3'd0;
						if (ram_offset == 14'h3FF8) begin
							ram_offset <= 14'd0;
							if (ram_page == (saved_ram128 ? 3'd7 : 3'd3)) begin
								// All RAM pages published
								state <= S_WRITE_LEN;
							end else begin
								ram_page <= ram_page + 3'd1;
								state    <= S_RAM_REQ;
							end
						end else begin
							ram_offset <= ram_offset + 14'd8;
							state      <= S_RAM_REQ;
						end
					end
				end

				S_WRITE_LEN: begin
					// Step 3: word 1 = {32'd0, file_bytes / 4} (0x8040 for 128K, 0x4040 for 64K)
					ddram_addr <= BASE_WORD + 29'd1;
					ddram_din  <= saved_ram128 ? {32'd0, 32'h0000_8040} : {32'd0, 32'h0000_4040};
					ddram_we   <= 1'b1;
					state      <= S_WAIT_LEN;
				end

				S_WAIT_LEN: begin
					if (!ddram_busy) begin
						ddram_we <= 1'b0;
						state    <= S_WRITE_GEN;
					end
				end

				S_WRITE_GEN: begin
					// Step 4: word 0 = {32'd0, generation}
					// The generation is consumed when its write is issued, not
					// when it is accepted: a reset now still drains this write
					// (S_QUIESCE), so advancing later could publish it twice.
					ddram_addr <= BASE_WORD + 29'd0;
					ddram_din  <= {32'd0, generation};
					ddram_we   <= 1'b1;
					generation <= (generation == 32'hFFFF_FFFE) ? 32'd1 : generation + 32'd1;
					state      <= S_WAIT_GEN;
				end

				S_WAIT_GEN: begin
					if (!ddram_busy) begin
						ddram_we    <= 1'b0;
						ddr_request <= 1'b0;
						done        <= 1'b1;
						state       <= S_IDLE;
					end
				end

				default: state <= S_IDLE;
			endcase
		end
	end

	initial begin
		state            = S_IDLE;
		cart_req         = 1'b0;
		cart_addr        = 23'd0;
		ddr_request      = 1'b0;
		ddram_addr       = 29'd0;
		ddram_din        = 64'd0;
		ddram_we         = 1'b0;
		done             = 1'b0;
		generation       = 32'd1;
		saved_ram128     = 1'b0;
		saved_bank       = 2'd0;
		hdr_word_idx     = 5'd0;
		ram_page         = 3'd0;
		ram_offset       = 14'd0;
		byte_idx         = 3'd0;
		payload_word_idx = 15'd0;
		word_buf         = 64'd0;
		yield_state      = Y_IDLE;
		clkref_d         = 1'b0;
	end

endmodule
