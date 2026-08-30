// P1 motherboard-integration validation bench (architecture §4 P1 exit,
// "intra-character pixel phase" item; mirrors the t05h assumption check).
//
// Reproduces the production Plus video slice exactly as wired in
// rtl/Amstrad_motherboard.v: asic_ga_timing strobes drive a verbatim copy of
// the motherboard's VRAM fetch block and twice-per-character word assembler,
// and asic_video consumes the resulting VIDEOD word. A fake VRAM returns
// words that encode their own address, so the displayed PEN stream can be
// decoded end to end: if the e0/03 latch windows were swapped relative to
// the CAS pair halves, or asic_video's dot phase were off by one character,
// the reconstructed byte sequence misaligns and the test fails.
//
// The two always blocks marked [production copy] must stay textually in
// sync with Amstrad_motherboard.v.
//
// Simulation-only: never added to files.qip.

`default_nettype none

module p1_video_test_top (
	input wire        clk,
	input wire        cen_16,     // PIXEN cadence (one dot)
	input wire        fast,
	input wire        RESET_N,

	// ASIC video register bus (asic_video samples writes every clock).
	input wire        vid_enable,
	input wire        vid_nCS,
	input wire        vid_R_nW,
	input wire        vid_RS,
	input wire [7:0]  vid_di,

	// Legacy GA register port (asic_ga_timing fast-path decode).
	input wire [15:0] ga_addr,
	input wire [7:0]  ga_data,
	input wire        ga_iorq_n,
	input wire        ga_m1_n,

	output wire [13:0] dbg_ma,
	output wire        dbg_de,
	output wire        dbg_cclk_en_p,
	output wire [7:0]  dbg_vram_d,
	output wire        dbg_ras_n,
	output wire        dbg_cas_n,
	output wire        dbg_cpu_n,
	output wire [4:0]  dbg_pen,
	output wire [13:0] dbg_c_ma,
	output wire        dbg_c_de,
	output wire [7:0]  dbg_c_vbuf,
	output wire [7:0]  dbg_c_s,
	output wire        dbg_hde,
	output wire        dbg_vde,
	output wire [7:0]  dbg_hcc,
	output wire [4:0]  dbg_r1,
	output wire [7:0]  dbg_r0,
	output wire [4:0]  dbg_addr_sel,
	output wire        dbg_cclk_en_n,
	output wire        dbg_hsync,
	output wire        dbg_vsync,
	output wire [15:0] dbg_vidword,
	output wire        dbg_vbs,
	output wire        dbg_ep,
	output wire [14:0] dbg_vaddr,
	output wire [4:0]  dbg_ra,
	output wire [3:0]  dbg_pixcnt,
	output wire [7:0]  dbg_veven,
	output wire [7:0]  dbg_vodd
);

	// ---- asic_ga_timing ----
	wire cclk_en_p, cclk_en_n;
	wire ras_n, cas_n, cpu_n;
	wire crtc_hs, crtc_vs, crtc_de;
	wire [13:0] ma;
	wire [4:0]  ra;
	wire [4:0]  border;
	wire [79:0] inkr;
	wire [1:0]  gamode;

	asic_ga_timing ga (
		.clk(clk),
		.cen_16(cen_16),
		.fast(fast),
		.RESET_N(RESET_N),
		.plus_unlocked(1'b0),
		.A(ga_addr[15:14]),
		.D(ga_data),
		.MREQ_N(1'b1),
		.M1_N(ga_m1_n),
		.RD_N(1'b1),
		.IORQ_N(ga_iorq_n),
		.HSYNC_I(crtc_hs),
		.VSYNC_I(crtc_vs),
		.pri(8'd0),
		.crtc_line(9'd0),
		.crtc_adj(1'b0),
		.int_last_raster(),
		.CCLK(),
		.CCLK_EN_P(cclk_en_p),
		.CCLK_EN_N(cclk_en_n),
		.PHI_N(),
		.PHI_EN_N(),
		.PHI_EN_P(),
		.RAS_N(ras_n),
		.CASAD_N(),
		.CAS_N(cas_n),
		.READY(),
		.CPU_N(cpu_n),
		.MWE_N(),
		.E244_N(),
		.ROMEN_N(),
		.RAMRD_N(),
		.ROM(),
		.HSYNC_O(),
		.VSYNC_O(),
		.SYNC_N(),
		.INT_N(),
		.VBLANK(),
		.MODE_SYNC_EN(),
		.MODE(),
		.BORDER_O(border),
		.INKR_O(inkr),
		.GAMODE_O(gamode)
	);

	// ---- asic_video ----
	wire [3:0] rgb_r, rgb_g, rgb_b;
	wire [4:0] pen;
	reg [15:0] vidword;

	asic_video vid (
		.CLOCK(clk),
		.CLKEN(cclk_en_n),
		.nRESET(RESET_N),
		.ENABLE(vid_enable),
		.nCS(vid_nCS),
		.R_nW(vid_R_nW),
		.RS(vid_RS),
		.DI(vid_di),
		.DO(),
		.HSYNC(crtc_hs),
		.VSYNC(crtc_vs),
		.DE(crtc_de),
		.MA(ma),
		.RA(ra),
		.HCC(),
		.LINE(),
		.ROW(),
		.ADJ(),
		.SPLT(8'd0),
		.SSA(14'd0),
		.SSCR(8'd0),
		.PIXEN(cen_16),
		.VIDEOD(vidword),
		.GAMODE(gamode),
		.BORDER_I(border),
		.INKR_I(inkr),
		.RGB_R(rgb_r),
		.RGB_G(rgb_g),
		.RGB_B(rgb_b),
		.PEN(pen),
		.HWRAP(),
		.SPR_EN(1'b0),
		.SPR_RGB(12'd0),
		// No asic_regs in this bench: keep the internal legacy-colour ROM.
		.PAL_EN(1'b0),
		.PAL_ADDR(),
		.PAL_RGB(12'd0)
	);
	assign dbg_vidword = vidword;
	assign dbg_vbs = vram_bs;
	assign dbg_ep = cclk_en_p;
	assign dbg_vaddr = vram_addr_r;
	assign dbg_ra = ra;
	assign dbg_pixcnt = vid.pix_cnt;
	assign dbg_veven = vid.vid_even;
	assign dbg_vodd = vid.vid_odd;

	// ---- [production copy] VRAM fetch block (Amstrad_motherboard.v) ----
	wire [14:0] crtc_vram_addr = {ma[13:12], ra[2:0], ma[9:0]};
	// Production registers the presented address (the sdram.v request
	// address); the fake backend returns the word for that registered
	// address combinationally, modelling zero-latency return inside the
	// CAS window.
	reg [14:0] vram_addr_r;

	reg vram_bs;
	reg [7:0] vram_d;
	reg [7:0] vram_din_shift;
	wire sync_filter = 1'b0; // bench uses the plain path

	always @(posedge clk) begin
		reg cas_n_old;
		cas_n_old <= cas_n;
		if (!cpu_n) begin
			vram_bs <= 0;
			vram_addr_r <= crtc_vram_addr;
		end
		else begin
			if (!ras_n & !cas_n_old & cas_n) vram_bs <= 1;
			if (!ras_n & !cas_n)
				vram_d <= vram_bs ? vram_din_plus[15:8] : vram_din_plus[7:0];
		end
	end

	function [7:0] pat;
		input [14:0] a;
		begin
			pat = {1'b0, a[6:0]} ^ {a[9:8], 6'b010110} ^ {2'b01, a[14:10], 1'b1};
		end
	endfunction
	wire [14:0] even_a = {vram_addr_r[14:1], 1'b0};
	wire [15:0] vram_din_plus = {pat(even_a | 15'd1), pat(even_a)};
	wire [14:0] c_even_a = {c_vram_addr_r[14:1], 1'b0};
	wire [15:0] vram_din_classic = {pat(c_even_a | 15'd1), pat(c_even_a)};

	// ---- [production copy] word assembler ----
	always @(posedge clk) begin
		if (!RESET_N) vidword <= 16'd0;
		else begin
			if (cclk_en_p) vidword[7:0]  <= vram_d;
			if (cclk_en_n) vidword[15:8] <= vram_d;
		end
	end

	// ---- Classic reference slice (oracle): CRTC type 0 + ga40010 fed the
	// same register program, bus and fake VRAM, wired exactly like
	// rtl/Amstrad_motherboard.v. Its twice-per-character VIDEO_BUF latches
	// are exported for slot-by-slot comparison with the Plus PEN stream.
	wire c_crtc_hs, c_crtc_vs, c_crtc_de, c_field, c_cursor;
	wire [13:0] c_ma;
	wire [4:0]  c_ra;
	wire [7:0]  c_crtc_dout;
	wire        c_int_n_unused;

	CRTC c_crtc (
		.CLOCK(clk), .CLKEN(cclk_en_n), .nCLKEN(cclk_en_p),
		.nRESET(RESET_N), .CRTC_TYPE(1'b0),
		.ENABLE(vid_enable), .nCS(vid_nCS), .R_nW(vid_R_nW), .RS(vid_RS),
		.DI(vid_di), .DO(c_crtc_dout),
		.SNA_LOAD(1'b0), .SNA_ADDR(5'b0), .SNA_REGS(144'b0),
		.VSYNC(c_crtc_vs), .HSYNC(c_crtc_hs), .DE(c_crtc_de),
		.FIELD(c_field), .CURSOR(c_cursor),
		.MA(c_ma), .RA(c_ra)
	);

	wire [14:0] c_vram_addr = {c_ma[13:12], c_ra[2:0], c_ma[9:0]};
	reg c_bs;
	reg [7:0] c_vram_d;
	reg [14:0] c_vram_addr_r;
	always @(posedge clk) begin
		reg cas_old;
		cas_old <= cas_n;
		if (!cpu_n) begin
			c_bs <= 0;
			c_vram_addr_r <= c_vram_addr;
		end
		else begin
			if (!ras_n & !cas_old & cas_n) c_bs <= 1;
			if (!ras_n & !cas_n)
				c_vram_d <= c_bs ? vram_din_classic[15:8] : vram_din_classic[7:0];
		end
	end

	ga40010 c_ga (
		.clk(clk), .cen_16(cen_16), .fast(fast), .RESET_N(RESET_N),
		.A(ga_addr[15:14]), .D(c_ga_din),
		.MREQ_N(1'b1), .M1_N(ga_m1_n), .RD_N(1'b1), .IORQ_N(ga_iorq_n),
		.HSYNC_I(c_crtc_hs), .VSYNC_I(c_crtc_vs), .DISPEN(c_crtc_de),
		.CCLK(), .CCLK_EN_P(), .CCLK_EN_N(), .PHI_N(), .PHI_EN_N(),
		.PHI_EN_P(), .RAS_N(), .CAS_N(), .READY(), .CASAD_N(), .CPU_N(),
		.MWE_N(), .E244_N(), .ROMEN_N(), .RAMRD_N(),
		.HSYNC_O(), .VSYNC_O(), .SYNC_N(), .INT_N(c_int_n_unused),
		.VBLANK(), .MODE(),
		.BLUE_OE_N(), .BLUE(), .GREEN_OE_N(), .GREEN(),
		.RED_OE_N(), .RED(),
		.SNA_LOAD(1'b0), .SNA_INKSEL(5'b0), .SNA_PALETTE(136'b0),
		.SNA_CONFIG(8'b0)
	);
	// ga_din mux analogue: e244 path unused here, feed vram byte directly.
	wire [7:0] c_ga_din = c_vram_d;
	// VIDEO_BUF latch phases exposed for the comparison bench.
	// Simulation-only hierarchical peeks at the reference's buffer stage.
	wire [7:0] c_video_buf   = c_ga.VIDEO_BUF;
	wire [7:0] c_dispen_buf  = c_ga.DISPEN_BUF;
	wire [7:0] c_S           = c_ga.S;

	assign dbg_c_ma    = c_ma;
	assign dbg_c_de    = c_crtc_de;
	assign dbg_c_vbuf  = c_video_buf;
	assign dbg_c_s     = c_S;

	assign dbg_ma    = ma;
	assign dbg_de    = crtc_de;
	assign dbg_pen   = pen;
	wire dbg_hde_w   = vid.hde;
	wire dbg_vde_w   = vid.vde;
	wire [7:0] dbg_hcc_w = vid.hcc;
	wire [7:0] dbg_r1_w  = vid.R1_h_displayed;
	wire [7:0] dbg_r0_w  = vid.R0_h_total;
	wire [4:0] dbg_addr_w = vid.addr;
	assign dbg_hde = dbg_hde_w;
	assign dbg_vde = dbg_vde_w;
	assign dbg_hcc = dbg_hcc_w;
	assign dbg_r1  = dbg_r1_w;
	assign dbg_r0  = dbg_r0_w;
	assign dbg_addr_sel = dbg_addr_w;
	assign dbg_cclk_en_p = cclk_en_p;
	assign dbg_vram_d = vram_d;
	assign dbg_ras_n  = ras_n;
	assign dbg_cas_n  = cas_n;
	assign dbg_cpu_n  = cpu_n;
	assign dbg_cclk_en_n = cclk_en_n;
	assign dbg_hsync = crtc_hs;
	assign dbg_vsync = crtc_vs;

endmodule

`default_nettype wire
