// SSM marker detector harness (backlog B4, phase 1).
//
// Two drivers share one production `ssm_marker` instance, selected by
// `use_cpu`:
//
//   0  A synthetic opcode-fetch driver. The test holds `m1_fetch` for an
//      arbitrary number of clocks to imitate a wait-stated fetch, which is
//      the property the production strobe has to survive: the motherboard
//      condition `~M1_n & ~MREQ_n & ~RD_n` is a level spanning the whole M1
//      read, so counting it as a level would see one fetch several times.
//
//   1  A real Z80 (TV80 behind the T80pa wrapper) executing bytes from a
//      256-byte RAM, through the production GA40010 divider, clock enables
//      and WAIT. This is what proves an undefined-ED instruction really does
//      present both of its bytes as opcode fetches, and that an interrupt
//      taken between two pairs breaks the marker.
//
// The DDR3 slave is a small memory plus a stall generator, so the ring
// contents and the Avalon waitrequest handshake are both observable. It is
// indexed from the production default base, so the default parameter is
// exercised rather than replaced with a convenient one.

module ssm_marker_top
(
	input         clk,
	input         reset,
	input         enable,

	// Synthetic fetch driver (use_cpu = 0)
	input         use_cpu,
	input         m1_fetch_in,
	input   [7:0] bus_data_in,

	// Executing CPU driver (use_cpu = 1)
	input         cpu_reset,
	input         int_n,
	input         prog_we,
	input   [7:0] prog_addr,
	input   [7:0] prog_data,

	// DDR3 slave behaviour
	input   [3:0] ddr_stall,

	output [15:0] last_code,
	output [31:0] event_count,
	output  [7:0] dropped_count,
	output        event_stb,

	output reg [31:0] ddr_write_count,
	input      [8:0] peek_word,
	output    [63:0] peek_data,

	// CPU observation
	output [15:0] cpu_addr,
	output        cpu_m1_n,
	output        cpu_iorq_n,
	output        cpu_halt_n,
	output reg [31:0] fetch_strobes
);

//----------------------------------------------------------------------------
// Executing CPU: production GA enables and WAIT, 256-byte program RAM.
//----------------------------------------------------------------------------

reg [2:0] div = 0;
reg       ce_16 = 0;
always @(posedge clk) begin
	div   <= div + 1'b1;
	ce_16 <= (div[1:0] == 0);
end

reg [7:0] ram [0:255];
integer i;
initial for (i = 0; i < 256; i = i + 1) ram[i] = 8'h00;

wire [15:0] A;
wire  [7:0] DO;
wire        M1_n, MREQ_n, IORQ_n, RD_n, WR_n, RFSH_n, HALT_n, BUSAK_n;

wire mem_rd = ~(RD_n | MREQ_n);
wire mem_wr = ~(WR_n | MREQ_n);
wire [7:0] DI = mem_rd ? ram[A[7:0]] : 8'hFF;

always @(posedge clk) begin
	if (prog_we)    ram[prog_addr] <= prog_data;
	else if (mem_wr) ram[A[7:0]]   <= DO;
end

wire phi_en_p, phi_en_n, cclk_en_p, cclk_en_n, ready_o;
wire cpu_wait_n = ready_o | (IORQ_n & MREQ_n);

T80pa cpu (
	.reset_n(~(reset | cpu_reset)),
	.clk(clk),
	.cen_p(phi_en_p),
	.cen_n(phi_en_n),
	.wait_n(cpu_wait_n),
	.int_n(int_n),
	.nmi_n(1'b1),
	.busrq_n(1'b1),
	.m1_n(M1_n),
	.mreq_n(MREQ_n),
	.iorq_n(IORQ_n),
	.rd_n(RD_n),
	.wr_n(WR_n),
	.rfsh_n(RFSH_n),
	.halt_n(HALT_n),
	.busak_n(BUSAK_n),
	.OUT0(1'b0),
	.a(A),
	.di(DI),
	.do(DO),
	.R800_mode(1'b0),
	.REG(),
	.DIRSet(1'b0),
	.DIR(212'd0)
);

ga40010 ga (
	.clk(clk),
	.cen_16(ce_16),
	.fast(1'b0),
	.RESET_N(~reset),
	.A(A[15:14]),
	.D(DO),
	.MREQ_N(MREQ_n),
	.M1_N(M1_n),
	.RD_N(RD_n),
	.IORQ_N(IORQ_n),
	.HSYNC_I(1'b0),
	.VSYNC_I(1'b0),
	.DISPEN(1'b0),
	.CCLK_EN_P(cclk_en_p),
	.CCLK_EN_N(cclk_en_n),
	.PHI_EN_P(phi_en_p),
	.PHI_EN_N(phi_en_n),
	.PHI_N(),
	.CCLK(),
	.RAS_N(),
	.CAS_N(),
	.CASAD_N(),
	.READY(ready_o),
	.CPU_N(),
	.MWE_N(),
	.E244_N(),
	.ROMEN_N(),
	.RAMRD_N(),
	.ROM(),
	.MODE(),
	.HSYNC_O(),
	.VSYNC_O(),
	.SYNC_N(),
	.INT_N(),
	.VBLANK(),
	.BLUE_OE_N(),
	.BLUE(),
	.GREEN_OE_N(),
	.GREEN(),
	.RED_OE_N(),
	.RED(),
	.SNA_LOAD(1'b0),
	.SNA_INKSEL(5'd0),
	.SNA_PALETTE(136'd0),
	.SNA_CONFIG(8'd0)
);
assign cpu_addr   = A;
assign cpu_m1_n   = M1_n;
assign cpu_iorq_n = IORQ_n;
assign cpu_halt_n = HALT_n;

//----------------------------------------------------------------------------
// Tap. The CPU-side expression is the same one rtl/Amstrad_motherboard.v
// exports; the synthetic side substitutes a level the test controls.
//----------------------------------------------------------------------------

wire cpu_m1_fetch = ~M1_n & ~MREQ_n & ~RD_n;
wire m1_fetch     = use_cpu ? cpu_m1_fetch : m1_fetch_in;
wire [7:0] bus_data = use_cpu ? DI : bus_data_in;

//----------------------------------------------------------------------------
// DDR3 slave model: stall generator plus a window of the ring.
//----------------------------------------------------------------------------

localparam [28:0] BASE_WORD = 32'h3000_0000 >> 3;

wire [28:0] ddram_addr;
wire [63:0] ddram_din;
wire  [7:0] ddram_be, ddram_burstcnt;
wire        ddram_we;

reg [3:0] stall = 0;
always @(posedge clk) stall <= (stall == 0) ? ddr_stall : (stall - 1'b1);
wire ddram_busy = (stall != 0);

reg [63:0] ddr_mem [0:511];
reg  [8:0] peek_reg = 0;
initial for (i = 0; i < 512; i = i + 1) ddr_mem[i] = 64'd0;

assign peek_data = ddr_mem[peek_reg];

always @(posedge clk) begin
	peek_reg <= peek_word;
	if (reset) ddr_write_count <= 32'd0;
	else if (ddram_we & ~ddram_busy) begin
		ddr_mem[ddram_addr - BASE_WORD] <= ddram_din;
		ddr_write_count <= ddr_write_count + 1'd1;
	end
end

//----------------------------------------------------------------------------

reg m1_fetch_d = 0;
always @(posedge clk) begin
	m1_fetch_d <= m1_fetch;
	if (reset) fetch_strobes <= 32'd0;
	else if (m1_fetch_d & ~m1_fetch) fetch_strobes <= fetch_strobes + 1'd1;
end

ssm_marker dut (
	.clk(clk),
	.reset(reset),
	.enable(enable),

	.m1_fetch(m1_fetch),
	.bus_data(bus_data),

	.ce_pix(ce_16),
	.hsync(1'b0),
	.vsync(1'b0),
	.field(1'b0),

	.last_code(last_code),
	.event_count(event_count),
	.dropped_count(dropped_count),
	.event_stb(event_stb),

	.ddram_addr(ddram_addr),
	.ddram_din(ddram_din),
	.ddram_be(ddram_be),
	.ddram_burstcnt(ddram_burstcnt),
	.ddram_we(ddram_we),
	.ddram_busy(ddram_busy)
);

endmodule
