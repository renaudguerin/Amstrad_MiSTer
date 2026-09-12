// SSM recorder subsystem (composition of ssm_sample_recorder and ssm_ddr_arb).
//
// Encapsulates the video sample recorder and the DDR write arbiter into a
// single module shared by the production top (Amstrad.sv) and the simulation
// harness (ssm_composition_top.v).

module ssm_recorder_subsystem #(
	parameter [31:0] CAP_BASE       = 32'h3100_0000,
	parameter [31:0] PAYLOAD_OFF    = 32'h0010_0000,
	parameter integer WINDOWS       = 8,
	parameter integer WIN_IDX_BITS  = 3,
	parameter integer SAMPLE_BITS   = 19,
	parameter integer PIN_PREV      = 2,
	parameter [31:0] HOLD_TICKS     = 32'h1000_0000,
	parameter integer FIFO_BITS     = 6,
	parameter integer CAPTURE_SLOTS = 16,
	parameter integer CAP_IDX_BITS  = 4
) (
	input         clk,
	input         reset,
	input         enable,

	// Sample video inputs
	input         smp_ce,
	input   [7:0] smp_r,
	input   [7:0] smp_g,
	input   [7:0] smp_b,
	input         smp_hbl,
	input         smp_vbl,
	input         smp_hs,
	input         smp_vs,
	input         smp_field,
	input   [7:0] applied_config,

	// Capture trigger from marker/event engine
	input         cap_stb,
	input  [63:0] cap_rec_a,
	input  [63:0] cap_rec_b,
	input  [63:0] cap_cut,

	// Event ring DDR client 0 interface
	input  [28:0] c0_addr,
	input  [63:0] c0_din,
	input   [7:0] c0_be,
	input   [7:0] c0_burstcnt,
	input         c0_we,
	output        c0_busy,

	// Status outputs
	output [63:0] sample_count,
	output        ready,
	output [31:0] image_loss_count,
	output [31:0] forced_expiry_count,
	output [31:0] capture_alloc,
	output [31:0] capture_dropped,
	output [31:0] captures_published,
	output [31:0] epoch,

	// Shared DDR output interface
	output [28:0] ddram_addr,
	output [63:0] ddram_din,
	output  [7:0] ddram_be,
	output  [7:0] ddram_burstcnt,
	output        ddram_we,
	input         ddram_busy
);

wire [28:0] rec_addr;
wire [63:0] rec_din;
wire  [7:0] rec_be, rec_burstcnt;
wire        rec_we, rec_busy;

ssm_sample_recorder #(
	.CAP_BASE(CAP_BASE),
	.PAYLOAD_OFF(PAYLOAD_OFF),
	.WINDOWS(WINDOWS),
	.WIN_IDX_BITS(WIN_IDX_BITS),
	.SAMPLE_BITS(SAMPLE_BITS),
	.PIN_PREV(PIN_PREV),
	.HOLD_TICKS(HOLD_TICKS),
	.FIFO_BITS(FIFO_BITS),
	.CAPTURE_SLOTS(CAPTURE_SLOTS),
	.CAP_IDX_BITS(CAP_IDX_BITS)
) recorder (
	.clk(clk),
	.reset(reset),
	.enable(enable),

	.smp_ce(smp_ce),
	.smp_r(smp_r), .smp_g(smp_g), .smp_b(smp_b),
	.smp_hbl(smp_hbl), .smp_vbl(smp_vbl),
	.smp_hs(smp_hs), .smp_vs(smp_vs), .smp_field(smp_field),
	.applied_config(applied_config),

	.cap_stb(cap_stb),
	.cap_rec_a(cap_rec_a),
	.cap_rec_b(cap_rec_b),
	.cap_cut(cap_cut),

	.sample_count(sample_count),
	.ready(ready),
	.image_loss_count(image_loss_count),
	.forced_expiry_count(forced_expiry_count),
	.capture_alloc(capture_alloc),
	.capture_dropped(capture_dropped),
	.captures_published(captures_published),
	.epoch(epoch),

	.ddram_addr(rec_addr),
	.ddram_din(rec_din),
	.ddram_be(rec_be),
	.ddram_burstcnt(rec_burstcnt),
	.ddram_we(rec_we),
	.ddram_busy(rec_busy)
);

ssm_ddr_arb arb (
	.clk(clk),
	.c0_addr(c0_addr), .c0_din(c0_din), .c0_be(c0_be),
	.c0_burstcnt(c0_burstcnt), .c0_we(c0_we), .c0_busy(c0_busy),
	.c1_addr(rec_addr), .c1_din(rec_din), .c1_be(rec_be),
	.c1_burstcnt(rec_burstcnt), .c1_we(rec_we), .c1_busy(rec_busy),
	.ddram_addr(ddram_addr), .ddram_din(ddram_din), .ddram_be(ddram_be),
	.ddram_burstcnt(ddram_burstcnt), .ddram_we(ddram_we), .ddram_busy(ddram_busy)
);

endmodule
