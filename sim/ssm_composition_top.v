// SSM marker + recorder + arbiter composition harness (Finding 1 regression).
//
// Combines production ssm_marker, ssm_sample_recorder and ssm_ddr_arb to
// prove startup ordering, absence of deadlock when both are initially idle,
// and arbitration across stalled slave conditions and concurrent traffic.

module ssm_composition_top
(
	input         clk,
	input         reset,
	input         enable,

	// Marker opcode fetch driver
	input         m1_fetch,
	input   [7:0] bus_data,
	input         ce_pix,
	input         hsync,
	input         vsync,
	input         field,

	// Recorder sample driver
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

	// DDR stall
	input   [3:0] ddr_stall,

	// Status outputs
	output        marker_hit,
	output [15:0] marker_last_code,
	output [31:0] marker_event_count,
	output        recorder_ready,
	output [63:0] recorder_sample_count,
	output [31:0] recorder_captures_published,

	// Live Avalon observation
	output        ddr_we_o,
	output        ddr_busy_o,
	output [28:0] ddr_addr_o,
	output [63:0] ddr_din_o,
	output reg [31:0] ddr_write_count
);

localparam [31:0] RING_BASE = 32'h3000_0000;
localparam [31:0] CAP_BASE  = 32'h3100_0000;

wire [28:0] m_addr, arb_addr;
wire [63:0] m_din, arb_din;
wire  [7:0] m_be, arb_be;
wire  [7:0] m_burstcnt, arb_burstcnt;
wire        m_we, arb_we;
wire        m_busy;

reg [3:0] stall = 0;
always @(posedge clk) stall <= (stall == 0) ? ddr_stall : (stall - 1'b1);
wire ddram_busy = (stall != 0);

assign ddr_we_o   = arb_we;
assign ddr_busy_o = ddram_busy;
assign ddr_addr_o = arb_addr;
assign ddr_din_o  = arb_din;

always @(posedge clk) begin
	if (reset) ddr_write_count <= 32'd0;
	else if (arb_we & ~ddram_busy) ddr_write_count <= ddr_write_count + 1'd1;
end

wire        cap_stb;
wire [63:0] cap_rec_a, cap_rec_b, cap_cut;

ssm_marker #(
	.DDR_BASE(RING_BASE),
	.SLOT_BITS(4)
) marker (
	.clk(clk),
	.reset(reset),
	.enable(enable),

	.m1_fetch(m1_fetch),
	.bus_data(bus_data),

	.ce_pix(ce_pix),
	.hsync(hsync),
	.vsync(vsync),
	.field(field),
	.sample_count(recorder_sample_count),

	.last_code(marker_last_code),
	.event_count(marker_event_count),
	.dropped_count(),
	.event_stb(marker_hit),
	.event_capture(cap_stb),
	.event_code(),
	.event_tick(),
	.event_cut(cap_cut),
	.event_rec_a(cap_rec_a),
	.event_rec_b(cap_rec_b),

	.ddram_addr(m_addr),
	.ddram_din(m_din),
	.ddram_be(m_be),
	.ddram_burstcnt(m_burstcnt),
	.ddram_we(m_we),
	.ddram_busy(m_busy)
);

ssm_recorder_subsystem #(
	.CAP_BASE(CAP_BASE),
	.PAYLOAD_OFF(32'h0000_1000),
	.WINDOWS(4),
	.WIN_IDX_BITS(2),
	.SAMPLE_BITS(5),
	.PIN_PREV(2),
	.HOLD_TICKS(32'd2000),
	.FIFO_BITS(3),
	.CAPTURE_SLOTS(4),
	.CAP_IDX_BITS(2)
) sub (
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

	.c0_addr(m_addr),
	.c0_din(m_din),
	.c0_be(m_be),
	.c0_burstcnt(m_burstcnt),
	.c0_we(m_we),
	.c0_busy(m_busy),

	.sample_count(recorder_sample_count),
	.ready(recorder_ready),
	.image_loss_count(),
	.forced_expiry_count(),
	.capture_alloc(),
	.capture_dropped(),
	.captures_published(recorder_captures_published),
	.epoch(),

	.ddram_addr(arb_addr),
	.ddram_din(arb_din),
	.ddram_be(arb_be),
	.ddram_burstcnt(arb_burstcnt),
	.ddram_we(arb_we),
	.ddram_busy(ddram_busy)
);

endmodule
