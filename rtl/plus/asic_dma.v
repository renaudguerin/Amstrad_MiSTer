// Amstrad Plus / GX4000 3-Channel DMA Sound Engine (Phase P7)
//
// Author: Gemini (Claude / Codex independent cross-provider review)
// Target: MiSTer FPGA (Quartus 17.0.2 target, DE10-Nano)
//
// Sources & Reference:
// - docs/plus/references/asic-reference.md §9 (DMA Sound Engine)
// - docs/references/_Arnold V_ Specification - Issue 1.5 §2.6 (Automatic feeding of sound generator)
// - docs/references/_Arnold V_ Specification - Issue 1.5 §2.7 (Interrupt Service)
//
// Attribution:
// Portions implementing documented Amstrad Plus ASIC behaviour follow
// the Amstrad Plus / Arnold V specifications.

module asic_dma (
	input  wire        clk,            // Master 64 MHz clock
	input  wire        reset,          // Synchronous active-high reset
	input  wire        cclk_en_p,      // 4 MHz CRTC clock enable (rising phase / 1us)
	/* verilator lint_off UNUSEDSIGNAL */
	input  wire        cclk_en_n,      // 4 MHz CRTC clock enable (falling phase)
	/* verilator lint_on UNUSEDSIGNAL */

	// Timing synchronization
	input  wire        hsync,          // CRTC HSYNC (active high)

	// Channel register inputs from asic_regs
	input  wire [7:0]  sar0_lo,
	input  wire [7:0]  sar0_hi,
	input  wire [7:0]  ppr0,
	input  wire        sar0_wr,        // CPU wrote to SAR0 (reloads current address)

	input  wire [7:0]  sar1_lo,
	input  wire [7:0]  sar1_hi,
	input  wire [7:0]  ppr1,
	input  wire        sar1_wr,        // CPU wrote to SAR1

	input  wire [7:0]  sar2_lo,
	input  wire [7:0]  sar2_hi,
	input  wire [7:0]  ppr2,
	input  wire        sar2_wr,        // CPU wrote to SAR2

	input  wire [2:0]  dcsr_ena,       // Channel enable bits from DCSR[2:0]

	// Feedback to asic_regs / DCSR
	output reg  [2:0]  dcsr_ena_clr,   // Pulse to clear channel enable on STOP
	output reg  [2:0]  dma_int_set,    // Pulse to set DCSR interrupt flag on INT

	// Current SAR output for status/debug
	output wire [15:0] sar0_addr,
	output wire [15:0] sar1_addr,
	output wire [15:0] sar2_addr,

	// 16-bit RAM fetch interface
	output reg         ram_req,
	output reg  [15:0] ram_addr,
	input  wire [15:0] ram_data,       // CPU interface / arbitration
	input  wire [7:0]  cpu_psg_addr,
	output reg         dma_load_owner,

	// PSG / AY-3-8912 interface
	output reg         psg_bdir,
	output reg         psg_bc1,
	output reg  [7:0]  psg_dout,
	output reg         psg_active
);

	// Internal channel state
	reg [15:0] sar_cur [0:2];
	reg [11:0] pause_cnt [0:2];
	reg [7:0]  prescaler_cnt [0:2];
	reg [11:0] loop_cnt [0:2];
	reg [15:0] loop_addr [0:2];

	assign sar0_addr = sar_cur[0];
	assign sar1_addr = sar_cur[1];
	assign sar2_addr = sar_cur[2];

	// HSYNC edge detection
	reg hsync_d;
	wire hsync_rising = hsync && !hsync_d;

	// Active channels for current scanline
	reg [2:0] active_ch;
	reg [15:0] instr [0:2];

	// State machine definition (5-bit state encoding)
	localparam [4:0]
		ST_IDLE      = 5'd0,
		ST_DEAD      = 5'd1,
		ST_FETCH0    = 5'd2,
		ST_FETCH1    = 5'd3,
		ST_FETCH2    = 5'd4,
		ST_EXEC0_A   = 5'd5,
		ST_EXEC0_B   = 5'd6,
		ST_EXEC0_C   = 5'd7,
		ST_EXEC0_D   = 5'd8,
		ST_EXEC0_E   = 5'd9,
		ST_EXEC0_F   = 5'd10,
		ST_EXEC0_G   = 5'd11,
		ST_EXEC0_H   = 5'd12,
		ST_EXEC1_A   = 5'd13,
		ST_EXEC1_B   = 5'd14,
		ST_EXEC1_C   = 5'd15,
		ST_EXEC1_D   = 5'd16,
		ST_EXEC1_E   = 5'd17,
		ST_EXEC1_F   = 5'd18,
		ST_EXEC1_G   = 5'd19,
		ST_EXEC1_H   = 5'd20,
		ST_EXEC2_A   = 5'd21,
		ST_EXEC2_B   = 5'd22,
		ST_EXEC2_C   = 5'd23,
		ST_EXEC2_D   = 5'd24,
		ST_EXEC2_E   = 5'd25,
		ST_EXEC2_F   = 5'd26,
		ST_EXEC2_G   = 5'd27,
		ST_EXEC2_H   = 5'd28,
		ST_DONE      = 5'd29;

	reg [4:0] state;

	integer c;

	always @(posedge clk) begin
		if (reset) begin
			hsync_d        <= 1'b0;
			state          <= ST_IDLE;
			active_ch      <= 3'd0;
			dcsr_ena_clr   <= 3'd0;
			dma_int_set    <= 3'd0;
			ram_req        <= 1'b0;
			ram_addr       <= 16'd0;
			dma_load_owner <= 1'b0;
			psg_bdir       <= 1'b0;
			psg_bc1        <= 1'b0;
			psg_dout       <= 8'd0;
			psg_active     <= 1'b0;

			for (c = 0; c < 3; c = c + 1) begin
				sar_cur[c]       <= 16'd0;
				pause_cnt[c]     <= 12'd0;
				prescaler_cnt[c] <= 8'd0;
				loop_cnt[c]      <= 12'd0;
				loop_addr[c]     <= 16'd0;
				instr[c]         <= 16'd0;
			end
		end
		else begin
			hsync_d <= hsync;
			dcsr_ena_clr <= 3'd0;
			dma_int_set  <= 3'd0;

			// Handle CPU writes to SAR registers
			if (sar0_wr) sar_cur[0] <= {sar0_hi, sar0_lo};
			if (sar1_wr) sar_cur[1] <= {sar1_hi, sar1_lo};
			if (sar2_wr) sar_cur[2] <= {sar2_hi, sar2_lo};

			// HSYNC leading edge starts DMA line cycle
			if (hsync_rising) begin
				// Advance Pause counters for all enabled channels
				for (c = 0; c < 3; c = c + 1) begin
					if (dcsr_ena[c] && (pause_cnt[c] > 12'd0)) begin
						if (prescaler_cnt[c] == 8'd0) begin
							prescaler_cnt[c] <= (c == 0) ? ppr0 : (c == 1) ? ppr1 : ppr2;
							pause_cnt[c]     <= pause_cnt[c] - 12'd1;
						end
						else begin
							prescaler_cnt[c] <= prescaler_cnt[c] - 8'd1;
						end
					end
				end

				// Snapshot active channels (enabled and not pausing)
				active_ch[0] <= dcsr_ena[0] && (pause_cnt[0] == 12'd0);
				active_ch[1] <= dcsr_ena[1] && (pause_cnt[1] == 12'd0);
				active_ch[2] <= dcsr_ena[2] && (pause_cnt[2] == 12'd0);

				state <= ST_DEAD;
			end
			else if (cclk_en_p) begin
				case (state)
				ST_IDLE: begin
					dma_load_owner <= 1'b0;
					psg_active     <= 1'b0;
					psg_bdir       <= 1'b0;
					psg_bc1        <= 1'b0;
				end

				// 1 dead cycle (1us) after HSYNC leading edge
				ST_DEAD: begin
					ram_req <= 1'b0;
					if (|active_ch) begin
						if (active_ch[0]) begin
							// The Arnold V timing rule provides one fetch cycle for
							// each active channel, in channel order.  Select the
							// first active channel here so an inactive lower-numbered
							// channel does not consume a fetch slot.
							state    <= ST_FETCH0;
							ram_req  <= 1'b1;
							ram_addr <= {sar_cur[0][15:1], 1'b0};
						end
						else if (active_ch[1]) begin
							state    <= ST_FETCH1;
							ram_req  <= 1'b1;
							ram_addr <= {sar_cur[1][15:1], 1'b0};
						end
						else begin
							state    <= ST_FETCH2;
							ram_req  <= 1'b1;
							ram_addr <= {sar_cur[2][15:1], 1'b0};
						end
					end
					else begin
						state <= ST_IDLE;
					end
				end

				// Channel 0 fetch
				ST_FETCH0: begin
					if (active_ch[0]) begin
						instr[0]   <= ram_data;
						sar_cur[0] <= sar_cur[0] + 16'd2;
					end
					if (active_ch[1]) begin
						ram_req  <= 1'b1;
						ram_addr <= {sar_cur[1][15:1], 1'b0};
						state    <= ST_FETCH1;
					end
					else if (active_ch[2]) begin
						ram_req  <= 1'b1;
						ram_addr <= {sar_cur[2][15:1], 1'b0};
						state    <= ST_FETCH2;
					end
					else begin
						ram_req  <= 1'b0;
						state    <= ST_EXEC0_A;
					end
				end

				// Channel 1 fetch
				ST_FETCH1: begin
					if (active_ch[1]) begin
						instr[1]   <= ram_data;
						sar_cur[1] <= sar_cur[1] + 16'd2;
					end
					if (active_ch[2]) begin
						ram_req  <= 1'b1;
						ram_addr <= {sar_cur[2][15:1], 1'b0};
						state    <= ST_FETCH2;
					end
					else begin
						ram_req  <= 1'b0;
						state    <= ST_EXEC0_A;
					end
				end

				// Channel 2 fetch
				ST_FETCH2: begin
					if (active_ch[2]) begin
						instr[2]   <= ram_data;
						sar_cur[2] <= sar_cur[2] + 16'd2;
					end
					ram_req <= 1'b0;
					state   <= ST_EXEC0_A;
				end

				// Channel 0 execute
				ST_EXEC0_A: begin
					if (!active_ch[0]) begin
						state <= ST_EXEC1_A;
					end
					else begin
						case (instr[0][15:12])
						4'h0: begin // LOAD R, DD (8-cycle execution)
							// Substep 0: Acquire ownership, set target PSG register address
							dma_load_owner <= 1'b1;
							psg_active     <= 1'b1;
							psg_bdir       <= 1'b1;
							psg_bc1        <= 1'b1;
							psg_dout       <= {4'h0, instr[0][11:8]};
							state          <= ST_EXEC0_B;
						end
						4'h1: begin // PAUSE N
							if (instr[0][11:0] != 12'd0) begin
								pause_cnt[0]     <= instr[0][11:0];
								prescaler_cnt[0] <= ppr0;
							end
							state <= ST_EXEC1_A;
						end
						4'h2: begin // REPEAT N
							if (instr[0][11:0] != 12'd0) begin
								loop_cnt[0]  <= instr[0][11:0];
								loop_addr[0] <= sar_cur[0];
							end
							state <= ST_EXEC1_A;
						end
						4'h3: begin // PAUSE then REPEAT (undocumented)
							pause_cnt[0]     <= instr[0][11:0];
							prescaler_cnt[0] <= ppr0;
							loop_cnt[0]      <= instr[0][11:0];
							loop_addr[0]     <= sar_cur[0];
							state <= ST_EXEC1_A;
						end
						4'h4: begin // Control group: NOP / LOOP / INT / STOP
							if (instr[0][5]) begin // STOP
								dcsr_ena_clr[0] <= 1'b1;
							end
							if (instr[0][4]) begin // INT
								dma_int_set[0]  <= 1'b1;
							end
							if (instr[0][0] && !instr[0][5]) begin // LOOP (ignored if STOP)
								if (loop_cnt[0] > 12'd0) begin
									loop_cnt[0] <= loop_cnt[0] - 12'd1;
									sar_cur[0]  <= loop_addr[0];
								end
							end
							state <= ST_EXEC1_A;
						end
						default: begin
							state <= ST_EXEC1_A;
						end
						endcase
					end
				end

				ST_EXEC0_B: begin
					// Substep 1: Inactive separation
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC0_C;
				end

				ST_EXEC0_C: begin
					// Substep 2: Write data to PSG
					psg_bdir <= 1'b1;
					psg_bc1  <= 1'b0;
					psg_dout <= instr[0][7:0];
					state    <= ST_EXEC0_D;
				end

				ST_EXEC0_D: begin
					// Substep 3: Inactive separation
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC0_E;
				end

				ST_EXEC0_E: begin
					// Substep 4: Restore CPU-selected PSG register address
					psg_bdir <= 1'b1;
					psg_bc1  <= 1'b1;
					psg_dout <= cpu_psg_addr;
					state    <= ST_EXEC0_F;
				end

				ST_EXEC0_F: begin
					// Substep 5: Inactive separation
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC0_G;
				end

				ST_EXEC0_G: begin
					// Substep 6: Hold / collision margin
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC0_H;
				end

				ST_EXEC0_H: begin
					// Substep 7: Release ownership
					dma_load_owner <= 1'b0;
					psg_active     <= 1'b0;
					psg_bdir       <= 1'b0;
					psg_bc1        <= 1'b0;
					state          <= ST_EXEC1_A;
				end

				// Channel 1 execute
				ST_EXEC1_A: begin
					if (!active_ch[1]) begin
						state <= ST_EXEC2_A;
					end
					else begin
						case (instr[1][15:12])
						4'h0: begin // LOAD R, DD
							dma_load_owner <= 1'b1;
							psg_active     <= 1'b1;
							psg_bdir       <= 1'b1;
							psg_bc1        <= 1'b1;
							psg_dout       <= {4'h0, instr[1][11:8]};
							state          <= ST_EXEC1_B;
						end
						4'h1: begin // PAUSE N
							if (instr[1][11:0] != 12'd0) begin
								pause_cnt[1]     <= instr[1][11:0];
								prescaler_cnt[1] <= ppr1;
							end
							state <= ST_EXEC2_A;
						end
						4'h2: begin // REPEAT N
							if (instr[1][11:0] != 12'd0) begin
								loop_cnt[1]  <= instr[1][11:0];
								loop_addr[1] <= sar_cur[1];
							end
							state <= ST_EXEC2_A;
						end
						4'h3: begin // PAUSE then REPEAT
							pause_cnt[1]     <= instr[1][11:0];
							prescaler_cnt[1] <= ppr1;
							loop_cnt[1]      <= instr[1][11:0];
							loop_addr[1]     <= sar_cur[1];
							state <= ST_EXEC2_A;
						end
						4'h4: begin // Control group
							if (instr[1][5]) begin // STOP
								dcsr_ena_clr[1] <= 1'b1;
							end
							if (instr[1][4]) begin // INT
								dma_int_set[1]  <= 1'b1;
							end
							if (instr[1][0] && !instr[1][5]) begin // LOOP
								if (loop_cnt[1] > 12'd0) begin
									loop_cnt[1] <= loop_cnt[1] - 12'd1;
									sar_cur[1]  <= loop_addr[1];
								end
							end
							state <= ST_EXEC2_A;
						end
						default: begin
							state <= ST_EXEC2_A;
						end
						endcase
					end
				end

				ST_EXEC1_B: begin
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC1_C;
				end

				ST_EXEC1_C: begin
					psg_bdir <= 1'b1;
					psg_bc1  <= 1'b0;
					psg_dout <= instr[1][7:0];
					state    <= ST_EXEC1_D;
				end

				ST_EXEC1_D: begin
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC1_E;
				end

				ST_EXEC1_E: begin
					psg_bdir <= 1'b1;
					psg_bc1  <= 1'b1;
					psg_dout <= cpu_psg_addr;
					state    <= ST_EXEC1_F;
				end

				ST_EXEC1_F: begin
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC1_G;
				end

				ST_EXEC1_G: begin
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC1_H;
				end

				ST_EXEC1_H: begin
					dma_load_owner <= 1'b0;
					psg_active     <= 1'b0;
					psg_bdir       <= 1'b0;
					psg_bc1        <= 1'b0;
					state          <= ST_EXEC2_A;
				end

				// Channel 2 execute
				ST_EXEC2_A: begin
					if (!active_ch[2]) begin
						state <= ST_DONE;
					end
					else begin
						case (instr[2][15:12])
						4'h0: begin // LOAD R, DD
							dma_load_owner <= 1'b1;
							psg_active     <= 1'b1;
							psg_bdir       <= 1'b1;
							psg_bc1        <= 1'b1;
							psg_dout       <= {4'h0, instr[2][11:8]};
							state          <= ST_EXEC2_B;
						end
						4'h1: begin // PAUSE N
							if (instr[2][11:0] != 12'd0) begin
								pause_cnt[2]     <= instr[2][11:0];
								prescaler_cnt[2] <= ppr2;
							end
							state <= ST_DONE;
						end
						4'h2: begin // REPEAT N
							if (instr[2][11:0] != 12'd0) begin
								loop_cnt[2]  <= instr[2][11:0];
								loop_addr[2] <= sar_cur[2];
							end
							state <= ST_DONE;
						end
						4'h3: begin // PAUSE then REPEAT
							pause_cnt[2]     <= instr[2][11:0];
							prescaler_cnt[2] <= ppr2;
							loop_cnt[2]      <= instr[2][11:0];
							loop_addr[2]     <= sar_cur[2];
							state <= ST_DONE;
						end
						4'h4: begin // Control group
							if (instr[2][5]) begin // STOP
								dcsr_ena_clr[2] <= 1'b1;
							end
							if (instr[2][4]) begin // INT
								dma_int_set[2]  <= 1'b1;
							end
							if (instr[2][0] && !instr[2][5]) begin // LOOP
								if (loop_cnt[2] > 12'd0) begin
									loop_cnt[2] <= loop_cnt[2] - 12'd1;
									sar_cur[2]  <= loop_addr[2];
								end
							end
							state <= ST_DONE;
						end
						default: begin
							state <= ST_DONE;
						end
						endcase
					end
				end

				ST_EXEC2_B: begin
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC2_C;
				end

				ST_EXEC2_C: begin
					psg_bdir <= 1'b1;
					psg_bc1  <= 1'b0;
					psg_dout <= instr[2][7:0];
					state    <= ST_EXEC2_D;
				end

				ST_EXEC2_D: begin
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC2_E;
				end

				ST_EXEC2_E: begin
					psg_bdir <= 1'b1;
					psg_bc1  <= 1'b1;
					psg_dout <= cpu_psg_addr;
					state    <= ST_EXEC2_F;
				end

				ST_EXEC2_F: begin
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC2_G;
				end

				ST_EXEC2_G: begin
					psg_bdir <= 1'b0;
					psg_bc1  <= 1'b0;
					state    <= ST_EXEC2_H;
				end

				ST_EXEC2_H: begin
					dma_load_owner <= 1'b0;
					psg_active     <= 1'b0;
					psg_bdir       <= 1'b0;
					psg_bc1        <= 1'b0;
					state          <= ST_DONE;
				end

				ST_DONE: begin
					dma_load_owner <= 1'b0;
					psg_active     <= 1'b0;
					psg_bdir       <= 1'b0;
					psg_bc1        <= 1'b0;
					state          <= ST_IDLE;
				end

				default: state <= ST_IDLE;
				endcase
			end
		end
	end

endmodule
