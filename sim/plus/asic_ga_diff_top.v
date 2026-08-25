// Lockstep differential bench top: drives the Plus behavioural Gate Array
// support path (asic_ga_timing) and the classic netlist-derived ga40010 with
// IDENTICAL inputs and exposes both modules' shared contract outputs side by
// side. The C++ bench compares them every clock edge; cycle-exact equality is
// the acceptance criterion for docs/plus/architecture.md §5 Risk 1 option (a).
//
// Simulation-only: never added to files.qip.

`default_nettype none

module asic_ga_diff_top (
	input wire        clk,
	input wire        cen_16,
	input wire        fast,
	input wire        RESET_N,
	input wire [15:0] A,
	input wire [7:0]  D,
	input wire        MREQ_N,
	input wire        M1_N,
	input wire        RD_N,
	input wire        IORQ_N,
	input wire        HSYNC_I,
	input wire        VSYNC_I,
	input wire        DISPEN,

	// ---- ga40010 (classic reference) ----
	output wire       ga_CCLK,
	output wire       ga_CCLK_EN_P,
	output wire       ga_CCLK_EN_N,
	output wire       ga_PHI_N,
	output wire       ga_PHI_EN_N,
	output wire       ga_PHI_EN_P,
	output wire       ga_RAS_N,
	output wire       ga_CASAD_N,
	output wire       ga_CAS_N,
	output wire       ga_READY,
	output wire       ga_CPU_N,
	output wire       ga_MWE_N,
	output wire       ga_E244_N,
	output wire       ga_ROMEN_N,
	output wire       ga_RAMRD_N,
	output wire       ga_ROM,
	output wire       ga_HSYNC_O,
	output wire       ga_VSYNC_O,
	output wire       ga_SYNC_N,
	output wire       ga_INT_N,
	output wire       ga_VBLANK,
	output wire [1:0] ga_MODE,

	// ---- asic_ga_timing (Plus behavioural replica) ----
	output wire       as_CCLK,
	output wire       as_CCLK_EN_P,
	output wire       as_CCLK_EN_N,
	output wire       as_PHI_N,
	output wire       as_PHI_EN_N,
	output wire       as_PHI_EN_P,
	output wire       as_RAS_N,
	output wire       as_CASAD_N,
	output wire       as_CAS_N,
	output wire       as_READY,
	output wire       as_CPU_N,
	output wire       as_MWE_N,
	output wire       as_E244_N,
	output wire       as_ROMEN_N,
	output wire       as_RAMRD_N,
	output wire       as_ROM,
	output wire       as_HSYNC_O,
	output wire       as_VSYNC_O,
	output wire       as_SYNC_N,
	output wire       as_INT_N,
	output wire       as_VBLANK,
	output wire [1:0] as_MODE,

	// Replica-only observability (register payloads cannot be compared
	// against ga40010, which does not export them; directed vectors own
	// these).
	output wire [4:0]  as_BORDER_O,
	output wire [79:0] as_INKR_O,
	output wire [1:0]  as_GAMODE_O,
	output wire        as_MODE_SYNC_EN
);

	ga40010 u_ref (
		.clk(clk),
		.cen_16(cen_16),
		.fast(fast),
`ifdef VERILATOR
		// Only exists when the reference's shadow domain is compiled in;
		// this bench deliberately undefines VERILATOR (see Makefile).
		.clk_16(clk),
`endif
		.RESET_N(RESET_N),
		.A(A[15:14]),
		.D(D),
		.MREQ_N(MREQ_N),
		.M1_N(M1_N),
		.RD_N(RD_N),
		.IORQ_N(IORQ_N),
		.HSYNC_I(HSYNC_I),
		.VSYNC_I(VSYNC_I),
		.DISPEN(DISPEN),
		.CCLK(ga_CCLK),
		.CCLK_EN_P(ga_CCLK_EN_P),
		.CCLK_EN_N(ga_CCLK_EN_N),
		.PHI_N(ga_PHI_N),
		.PHI_EN_N(ga_PHI_EN_N),
		.PHI_EN_P(ga_PHI_EN_P),
		.RAS_N(ga_RAS_N),
		.CAS_N(ga_CAS_N),
		.READY(ga_READY),
		.CASAD_N(ga_CASAD_N),
		.CPU_N(ga_CPU_N),
		.MWE_N(ga_MWE_N),
		.E244_N(ga_E244_N),
		.ROMEN_N(ga_ROMEN_N),
		.RAMRD_N(ga_RAMRD_N),
		.ROM(ga_ROM),
		.MODE(ga_MODE),
		.HSYNC_O(ga_HSYNC_O),
		.VSYNC_O(ga_VSYNC_O),
		.SYNC_N(ga_SYNC_N),
		.INT_N(ga_INT_N),
		.VBLANK(ga_VBLANK),
		.SNA_LOAD(1'b0),
		.SNA_INKSEL(5'b0),
		.SNA_PALETTE(136'b0),
		.SNA_CONFIG(8'b0)
	);
	// Video DAC outputs of the reference are classic-path behaviour owned by
	// asic_video in the Plus architecture; left unconnected here.

	asic_ga_timing u_replica (
		.clk(clk),
		.cen_16(cen_16),
		.fast(fast),
		.RESET_N(RESET_N),
		.A(A[15:14]),
		.D(D),
		.MREQ_N(MREQ_N),
		.M1_N(M1_N),
		.RD_N(RD_N),
		.IORQ_N(IORQ_N),
		.HSYNC_I(HSYNC_I),
		.VSYNC_I(VSYNC_I),
		.pri(8'd0),
		.crtc_line(9'd0),
		.crtc_adj(1'b0),
		.int_last_raster(),
		.CCLK(as_CCLK),
		.CCLK_EN_P(as_CCLK_EN_P),
		.CCLK_EN_N(as_CCLK_EN_N),
		.PHI_N(as_PHI_N),
		.PHI_EN_N(as_PHI_EN_N),
		.PHI_EN_P(as_PHI_EN_P),
		.RAS_N(as_RAS_N),
		.CASAD_N(as_CASAD_N),
		.CAS_N(as_CAS_N),
		.READY(as_READY),
		.CPU_N(as_CPU_N),
		.MWE_N(as_MWE_N),
		.E244_N(as_E244_N),
		.ROMEN_N(as_ROMEN_N),
		.RAMRD_N(as_RAMRD_N),
		.ROM(as_ROM),
		.HSYNC_O(as_HSYNC_O),
		.VSYNC_O(as_VSYNC_O),
		.SYNC_N(as_SYNC_N),
		.INT_N(as_INT_N),
		.VBLANK(as_VBLANK),
		.MODE_SYNC_EN(as_MODE_SYNC_EN),
		.MODE(as_MODE),
		.BORDER_O(as_BORDER_O),
		.INKR_O(as_INKR_O),
		.GAMODE_O(as_GAMODE_O)
	);

endmodule

`default_nettype wire
