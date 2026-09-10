// Simulation-only stubs for the Verilator motherboard builds. These
// children of Amstrad_motherboard are SystemVerilog and the motherboard's
// `.do(` T80pa pin forces --language 1364-2001 for every Verilator pass
// that includes it, so they cannot participate as themselves:
//
//   - ga40010 : SystemVerilog netlist wrapper (+ casgen/syncgen/video
//               siblings); its timing contract into the Plus path is
//               separately pinned live by the GADIFF lockstep bench.
//   - YM2149  : SystemVerilog (rtl/YM2149.sv).
//   - hid     : SystemVerilog (rtl/hid.sv).
//
// Each stub keeps the FULL port list of its real counterpart (copied from
// the source listed above), so pin wiring, widths, directions, and
// implicit-net bugs at those boundaries are still checked. Behaviour is
// deliberately absent: outputs tied to constants, never to emulate the
// part. The T80pa stub is NOT here — it lives in t80pa_lint_stub.v (lint)
// and t80pa_bench_cpu.v (mobo-bench), which pick one implementation each.
// Never synthesise this file; never add it to files.qip. When a port list
// changes upstream, this file must change in the same commit — the pass
// fails loudly if the two disagree about a *connected* pin.

// Port list copied from rtl/GA40010/ga40010.sv (production composition: the
// `ifdef VERILATOR clk_16 input is excluded, matching -UVERILATOR).
`ifndef B7_DARK_SILICON_MUTATION
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
	assign ROMEN_N   = 1'b1; // no ROM mapped — irrelevant in plus_mode benches
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
`endif

// The production DMA/PPI motherboard fixture supplies the real YM2149 and
// HID so it can validate PHI-aligned keyboard read data under DMA ownership.
`ifndef P10_DMA_MOBO_REAL_IO
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
	assign DO        = 8'hFF; // wired-AND neutral on the CPU data bus
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
`endif
