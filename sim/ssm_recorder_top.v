// Experimental SSM sample recorder harness (backlog B4, phase 2).
//
// The production `ssm_sample_recorder` with small parameter overrides: four
// windows of 32 samples, an eight-entry queue and a short hold time. The
// shapes under test are the ordering rules, the loss bookkeeping and the pool
// behaviour, none of which depend on the production sizes; the sizes only
// decide how long a vector has to run.
//
// Every accepted DDR3 write is exported beat by beat, so the C++ side can
// check the *order* of publication and not just the settled bytes. That is the
// whole point: "invalidate before reuse" and "seal after the last payload
// beat" are statements about order.

module ssm_recorder_top
(
	input         clk,
	input         reset,
	input         enable,

	// Native sample stream
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

	// Marker, with the cut the detector latched at the HH fetch boundary
	input         cap_stb,
	input  [63:0] cap_rec_a,
	input  [63:0] cap_rec_b,
	input  [63:0] cap_cut,

	// DDR3 slave behaviour
	input   [3:0] ddr_stall,

	output        ready,
	output [63:0] sample_count,
	output [31:0] image_loss_count,
	output [31:0] forced_expiry_count,
	output [31:0] capture_alloc,
	output [31:0] capture_dropped,
	output [31:0] captures_published,
	output [31:0] epoch,

	// Live Avalon view
	output        ddr_we_o,
	output        ddr_busy_o,
	output [28:0] ddr_addr_o,
	output [63:0] ddr_din_o,
	output reg [31:0] ddr_write_count
);

localparam [31:0] CAP_BASE    = 32'h3100_0000;
localparam [31:0] PAYLOAD_OFF = 32'h0000_1000;   // 512 words in

wire [28:0] ddram_addr;
wire [63:0] ddram_din;
wire  [7:0] ddram_be, ddram_burstcnt;
wire        ddram_we;

reg [3:0] stall = 0;
always @(posedge clk) stall <= (stall == 0) ? ddr_stall : (stall - 1'b1);
wire ddram_busy = (stall != 0);

assign ddr_we_o   = ddram_we;
assign ddr_busy_o = ddram_busy;
assign ddr_addr_o = ddram_addr;
assign ddr_din_o  = ddram_din;

always @(posedge clk) begin
	if (reset) ddr_write_count <= 32'd0;
	else if (ddram_we & ~ddram_busy) ddr_write_count <= ddr_write_count + 1'd1;
end

ssm_sample_recorder #(
	.CAP_BASE(CAP_BASE),
	.PAYLOAD_OFF(PAYLOAD_OFF),
	.WINDOWS(4),
	.WIN_IDX_BITS(2),
	.SAMPLE_BITS(5),          // 32 samples, 16 packed words per window
	.PIN_PREV(2),
	.HOLD_TICKS(32'd2000),
	.FIFO_BITS(3),            // 8 entries, 6 usable by payload
	.CAPTURE_SLOTS(4),
	.CAP_IDX_BITS(2)
) dut (
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

	.ddram_addr(ddram_addr),
	.ddram_din(ddram_din),
	.ddram_be(ddram_be),
	.ddram_burstcnt(ddram_burstcnt),
	.ddram_we(ddram_we),
	.ddram_busy(ddram_busy)
);

endmodule
