// Amstrad Plus cartridge MMU (P0): cartridge window paging and the CPU-side
// read bridge to plus_cartridge_memory.
//
// Behaviour implemented from docs/plus/references/asic-reference.md:
//
//   High window (&C000-&FFFF), selected by the ROM-select port (any I/O
//   write with A13=0, mirroring the classic MMU's alias decode):
//     128-255 -> physical cartridge page value[4:0]
//     GX4000, any value < 128 -> page 1 (no disc-select hardware)
//     otherwise value == 7 -> page 3 (AMSDOS)
//     value == 0 -> /EXP low selects page 1, else page 3 (reset default)
//     remaining 1..126 -> page 1 (BASIC)                        (§11, §12)
//
//   Low window position/page from RMR2 (Gate Array port pattern 101xxxxx).
//   While the ASIC is locked the write is left to the Gate Array's MRER
//   handling, exactly as the reference describes. Position code 11 means
//   "low ROM at &0000 with ASIC page at &4000-&7FFF"; the ASIC-page-enable
//   flag is captured here but consuming it (backing the ASIC page) is P2
//   scope.                                                    (§1, §2, §11)
//
//   Both cartridge windows are gated by the same Gate Array ROM-enable term
//   the classic MMU applies to its ROM windows, so MRER ROM-disable lets
//   RAM show through, as upstream.
//
// The exp_n input is a defined dynamic input, not a modelled constant: high
// means no expansion device is connected (the pulled-up bare machine). P0
// wires it high at the top level; future expansion emulation drives it low
// while claiming the port. It is sampled live whenever the ROM-select value
// resolves through the value-0 rule.
//
// Read bridging: a bus read hitting a cartridge window claims the held
// request/acknowledge CPU port of the memory service, holds the Z80 in WAIT
// until the data returns, and owns the CPU data mux until the bus cycle
// ends. cart_dout is captured one edge after cart_ready because the service
// registers data and completion on the same edge. A watchdog releases the
// stall with open-bus FF if no answer ever arrives, so a wedged backend
// cannot hang the machine. The Z80 deasserts MREQ/RD between bus cycles,
// which is what terminates the ownership window.
//
// Sources are the reference sections cited above; this module implements no
// CRTC behaviour, so no Compendium attribution applies.

module plus_mmu
(
	input             clk,
	input             reset,

	input             plus_mode,
	input             gx4000,

	// Z80 bus visibility (io_wr = IORQ & WR, mem_rd = MREQ & RD)
	input             io_wr,
	input             mem_rd,
	input      [15:0] A,
	input      [7:0]  D,

	// Gate Array ROM enable, active high (the top-level `romen`)
	input             rom_en,

	// expansion-port /EXP state: 1 = nothing connected (pulled up)
	input             exp_n,

	// cartridge memory service CPU port (held request/acknowledge)
	output reg        cart_valid,
	output reg  [4:0] cart_page,
	output reg [13:0] cart_offset,
	input             cart_ready,
	input      [7:0]  cart_data,

	// result side
	output reg        cart_own,    // this read cycle belongs to the cartridge
	output reg        cart_stall,  // hold the Z80 in WAIT, active high
	output reg  [7:0] cart_dout,   // stable while cart_own

	// stored ASIC-page enable (RMR2 D4D3 = 11); consumed from P2 onward
	output reg        asic_page_on
);

localparam [1:0] CART_IDLE    = 2'd0;
localparam [1:0] CART_WAIT    = 2'd1;
localparam [1:0] CART_CAPTURE = 2'd2;
localparam [1:0] CART_DONE    = 2'd3;

// Generous against the worst SDRAM slot starvation (a few slots, tens of
// clk_sys cycles); small enough to recover the CPU almost immediately.
localparam [15:0] STALL_TIMEOUT = 16'd1024;

reg        unlocked;
reg [1:0]  rmr2_pos;    // low-window position as A[15:14] compare code
reg [2:0]  rmr2_page;
reg [7:0]  romsel;

reg [1:0]  cart_state;
reg [15:0] stall_count;

// Unlock-sequence detector: the published sequence rides writes to the CRTC
// register-select port, which this core decodes as I/O writes with nCS=A14=0,
// R_nW=A9=0 and RS=A8=0 (the &BCxx range and its aliases).
wire unlock_write = io_wr & ~A[14] & ~A[9] & ~A[8];

asic_unlock unlock_detector
(
	.clk(clk),
	.RESET_N(~reset),
	.write_strobe(unlock_write),
	.write_data(D),
	.unlocked(unlocked)
);

// Window decode
wire rom_active = plus_mode & rom_en;
wire low_hit    = rom_active & (A[15:14] == rmr2_pos);
wire high_hit   = rom_active & (A[15:14] == 2'b11);
// A cartridge-owned bus cycle: a read inside one of the two windows.
wire window_hit = mem_rd & (low_hit | high_hit);

// High-window physical page (see header table). Sampled live.
reg [4:0] high_page;
always @(*) begin
	if (romsel[7])           high_page = romsel[4:0];
	else if (gx4000)         high_page = 5'd1;
	else if (romsel == 8'd7) high_page = 5'd3;
	else if (romsel == 8'd0) high_page = exp_n ? 5'd3 : 5'd1;
	else                     high_page = 5'd1;
end

// RMR2 and ROM-select capture
always @(posedge clk) begin
	if (reset) begin
		rmr2_pos     <= 2'b00;
		rmr2_page    <= 3'd0;
		asic_page_on <= 1'b0;
		romsel       <= 8'h00;
	end
	else begin
		if (io_wr && !A[15] && A[14] && (D[7:5] == 3'b101) && unlocked) begin
			// RMR2: D4D3 position (11 => &0000 + ASIC page on), D2-D0 page
			rmr2_pos     <= (D[4:3] == 2'b11) ? 2'b00 : D[4:3];
			rmr2_page    <= D[2:0];
			asic_page_on <= (D[4:3] == 2'b11);
		end

		if (io_wr && !A[13]) begin
			romsel <= D;
		end
	end
end

// Cartridge read bridge
always @(posedge clk) begin
	if (reset) begin
		cart_state  <= CART_IDLE;
		cart_valid  <= 1'b0;
		cart_page   <= 5'd0;
		cart_offset <= 14'd0;
		cart_own    <= 1'b0;
		cart_stall  <= 1'b0;
		cart_dout   <= 8'hFF;
		stall_count <= 16'd0;
	end
	else begin
		case (cart_state)
			CART_IDLE: begin
				cart_own    <= 1'b0;
				cart_stall  <= 1'b0;
				stall_count <= 16'd0;
				if (window_hit) begin
					cart_valid  <= 1'b1;
					cart_page   <= low_hit ? {2'b00, rmr2_page} : high_page;
					cart_offset <= A[13:0];
					cart_own    <= 1'b1;
					cart_stall  <= 1'b1;
					cart_state  <= CART_WAIT;
				end
			end

			CART_WAIT: begin
				stall_count <= stall_count + 16'd1;
				if (cart_ready) begin
					cart_valid <= 1'b0;
					cart_state <= CART_CAPTURE;
				end
				else if (!window_hit || (stall_count == STALL_TIMEOUT)) begin
					// The bus cycle vanished underneath us, or the backend
					// never answered: release with open-bus data instead of
					// hanging the machine.
					cart_valid <= 1'b0;
					cart_stall <= 1'b0;
					cart_dout  <= 8'hFF;
					cart_state <= window_hit ? CART_DONE : CART_IDLE;
				end
			end

			CART_CAPTURE: begin
				// The service registers data and completion on the same
				// edge, so the data is sampled on this following edge.
				cart_dout  <= cart_data;
				cart_stall <= 1'b0;
				cart_state <= CART_DONE;
			end

			CART_DONE: begin
				if (!window_hit) begin
					cart_own   <= 1'b0;
					cart_state <= CART_IDLE;
				end
			end
		endcase
	end
end

// Quartus maps these synthesizable initial values to FPGA power-up state.
// `unlocked` belongs to the asic_unlock instance and starts locked.
initial begin
	rmr2_pos     = 2'b00;
	rmr2_page    = 3'd0;
	asic_page_on = 1'b0;
	romsel       = 8'h00;
	cart_state   = CART_IDLE;
	cart_valid   = 1'b0;
	cart_page    = 5'd0;
	cart_offset  = 14'd0;
	cart_own     = 1'b0;
	cart_stall   = 1'b0;
	cart_dout    = 8'hFF;
	stall_count  = 16'd0;
end

endmodule
