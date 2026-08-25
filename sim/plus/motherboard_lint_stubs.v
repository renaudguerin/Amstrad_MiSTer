// Simulation-only stubs for the Verilator motherboard lint/elaboration pass
// (`make -C sim/plus motherboard-lint`). They exist because Verilator cannot
// read these children of Amstrad_motherboard under the pass's
// `--language 1364-2001` setting (the T80pa instantiation's `.do(` port name
// is a SystemVerilog keyword, which also rules out running the pass in
// default-SV mode):
//
//   - T80pa   : VHDL (rtl/T80/T80pa.vhd); Verilator has no VHDL front end.
//   - ga40010 : SystemVerilog netlist wrapper + its casgen/syncgen/video
//               siblings; their boundary into the motherboard is separately
//               exercised live by the GADIFF lockstep bench and the p1_video
//               bench, which compile the real files.
//   - YM2149  : SystemVerilog (rtl/YM2149.sv).
//   - hid     : SystemVerilog (rtl/hid.sv).
//
// Each stub keeps the FULL port list of its real counterpart (copied from the
// source listed above), so pin wiring, widths, directions, and implicit-net
// bugs at those boundaries are still checked by the pass. Behaviour is
// deliberately absent: outputs are tied to constants, never to emulate the
// part. Never synthesise this file; never add it to files.qip. When a port
// list changes upstream, this file must change in the same commit — the
// pass fails loudly if the two disagree about a *connected* pin, which is
// the intended tripwire.

module T80pa (
	input  wire         reset_n,
	input  wire         clk,
	input  wire         cen_p,
	input  wire         cen_n,
	output wire [15:0]  a,
	output wire [7:0]   do,
	input  wire [7:0]   di,
	output wire         rd_n,
	output wire         wr_n,
	output wire         iorq_n,
	output wire         mreq_n,
	output wire         m1_n,
	output wire         rfsh_n,
	input  wire         busrq_n,
	input  wire         int_n,
	input  wire         nmi_n,
	input  wire         wait_n,
	input  wire         DIRSet,
	input  wire [211:0] DIR
);
	assign a      = 16'h0000;
	assign do     = 8'h00;
	assign rd_n   = 1'b1;
	assign wr_n   = 1'b1;
	assign iorq_n = 1'b1;
	assign mreq_n = 1'b1;
	assign m1_n   = 1'b1;
	assign rfsh_n = 1'b1;
	wire unused = &{1'b0, reset_n, clk, cen_p, cen_n, di, busrq_n,
			int_n, nmi_n, wait_n, DIRSet, DIR, 1'b0};
endmodule

// Port list copied from rtl/GA40010/ga40010.sv (production composition: the
// `ifdef VERILATOR clk_16 input is excluded, matching -UVERILATOR).
module ga40010 (
	input  wire        clk,
	input  wire        cen_16,
	input  wire        fast,
	input  wire        RESET_N,
	input  wire [15:14] A,
	input  wire [7:0]  D,
	input  wire        MREQ_N,
	input  wire        M1_N,
	input  wire        RD_N,
	input  wire        IORQ_N,
	input  wire        HSYNC_I,
	input  wire        VSYNC_I,
	input  wire        DISPEN,
	output wire        CCLK,
	output wire        CCLK_EN_P,
	output wire        CCLK_EN_N,
	output reg         PHI_N,
	output reg         PHI_EN_N,
	output reg         PHI_EN_P,
	output reg         RAS_N,
	output wire        CAS_N,
	output wire        READY,
	output reg         CASAD_N,
	output wire        CPU_N,
	output wire        MWE_N,
	output wire        E244_N,
	output wire        ROMEN_N,
	output wire        RAMRD_N,
	output wire        ROM,
	output wire [1:0]  MODE,
	output wire        HSYNC_O,
	output wire        VSYNC_O,
	output wire        SYNC_N,
	output reg         INT_N,
	output wire        VBLANK,
	output wire        BLUE_OE_N,
	output wire        BLUE,
	output wire        GREEN_OE_N,
	output wire        GREEN,
	output wire        RED_OE_N,
	output wire        RED,
	input  wire        SNA_LOAD,
	input  wire [4:0]  SNA_INKSEL,
	input  wire [135:0] SNA_PALETTE,
	input  wire [7:0]  SNA_CONFIG
);
	initial begin
		PHI_N = 1'b1; PHI_EN_N = 1'b0; PHI_EN_P = 1'b0;
		RAS_N = 1'b1; CASAD_N = 1'b1; INT_N = 1'b1;
	end
	assign CCLK      = 1'b0;
	assign CCLK_EN_P = 1'b0;
	assign CCLK_EN_N = 1'b0;
	assign CAS_N     = 1'b1;
	assign READY     = 1'b1;
	assign CPU_N     = 1'b1;
	assign MWE_N     = 1'b1;
	assign E244_N    = 1'b1;
	assign ROMEN_N   = 1'b1;
	assign RAMRD_N   = 1'b1;
	assign ROM       = 1'b0;
	assign MODE      = 2'b00;
	assign HSYNC_O   = 1'b0;
	assign VSYNC_O   = 1'b0;
	assign SYNC_N    = 1'b1;
	assign VBLANK    = 1'b0;
	assign BLUE_OE_N = 1'b1;
	assign BLUE      = 1'b0;
	assign GREEN_OE_N = 1'b1;
	assign GREEN     = 1'b0;
	assign RED_OE_N  = 1'b1;
	assign RED       = 1'b0;
	wire unused = &{1'b0, clk, cen_16, fast, RESET_N, A, D, MREQ_N, M1_N,
			RD_N, IORQ_N, HSYNC_I, VSYNC_I, DISPEN, SNA_LOAD,
			SNA_INKSEL, SNA_PALETTE, SNA_CONFIG, 1'b0};
endmodule

// Port list copied from rtl/YM2149.sv.
module YM2149 (
	input  wire        CLK,
	input  wire        CE,
	input  wire        RESET,
	input  wire        BDIR,
	input  wire        BC,
	input  wire [7:0]  DI,
	output wire [7:0]  DO,
	output wire [7:0]  CHANNEL_A,
	output wire [7:0]  CHANNEL_B,
	output wire [7:0]  CHANNEL_C,
	input  wire        SEL,
	input  wire        MODE,
	output wire [5:0]  ACTIVE,
	input  wire [7:0]  IOA_in,
	output wire [7:0]  IOA_out,
	input  wire [7:0]  IOB_in,
	output wire [7:0]  IOB_out,
	input  wire        SNA_LOAD,
	input  wire [3:0]  SNA_ADDR,
	input  wire [127:0] SNA_REGS
);
	assign DO        = 8'hFF;
	assign CHANNEL_A = 8'h00;
	assign CHANNEL_B = 8'h00;
	assign CHANNEL_C = 8'h00;
	assign ACTIVE    = 6'h00;
	assign IOA_out   = 8'hFF;
	assign IOB_out   = 8'hFF;
	wire unused = &{1'b0, CLK, CE, RESET, BDIR, BC, DI, SEL, MODE,
			IOA_in, IOB_in, SNA_LOAD, SNA_ADDR, SNA_REGS, 1'b0};
endmodule

// Port list copied from rtl/hid.sv.
module hid (
	input  wire        reset,
	input  wire        clk,
	input  wire [10:0] ps2_key,
	input  wire [24:0] ps2_mouse,
	input  wire        right_shift_mod,
	input  wire        keypad_mod,
	input  wire [6:0]  joystick1,
	input  wire [6:0]  joystick2,
	input  wire [3:0]  Y,
	output wire [7:0]  X,
	output reg         key_nmi,
	output reg         key_reset,
	output reg  [9:0]  Fn
);
	initial begin
		key_nmi = 1'b0;
		key_reset = 1'b0;
		Fn = 10'h0;
	end
	assign X = 8'hFF;
	wire unused = &{1'b0, reset, clk, ps2_key, ps2_mouse,
			right_shift_mod, keypad_mod, joystick1, joystick2, Y, 1'b0};
endmodule
