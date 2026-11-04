// GA-to-decoder mode acceptance: actual production modules, constant VRAM data.
// RMR requests use the real GA port; no mode or qualification equation is copied.
// Technical information sourced from the Amstrad CPC CRTC Compendium by Longshot
// (CC BY-NC-ND 4.0). FR v1.11 §§9.3.1 p52,9.3.3 p53: all chips need >=2us HSYNC.
module asic_mode_latch_test_top(input clk, RESET_N, sna_load,
 input [143:0] regs, input [7:0] ga_data, input ga_iorq_n, input snapshot_hs,
 output dot, output [4:0] pen_o, output [7:0] hcc, output hs, de, mode_event, shaped_hs,
 output [1:0] requested_mode);
 reg[1:0] div=0; wire cen_16=(div==0);
 always @(posedge clk) div<=div+1'b1;
 wire fast=0; wire[15:0] ga_addr=16'h7f00; wire ga_m1_n=1;
 wire vid_enable=0,vid_nCS=1,vid_R_nW=0,vid_RS=0;wire[7:0] vid_di=0;
 wire cclk_en_p,cclk_en_n,ras_n,cas_n,cpu_n,crtc_hs,crtc_vs,crtc_de;
 wire[13:0]ma;wire[4:0]ra,border,pen;wire[79:0]inkr;wire[1:0]gamode;
 wire[3:0]rgb_r,rgb_g,rgb_b;
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
		.crtc_adj(vid.in_adj),
		.intack(1'b0),
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
		.HSYNC_O(shaped_hs),
		.VSYNC_O(),
		.SYNC_N(),
		.INT_N(),
		.VBLANK(),
		.MODE_SYNC_EN(),.ASIC_MODE_SYNC_EN(mode_event),
		.MODE(),.LEGACY_PAL_WR(),.LEGACY_PAL_ADDR(),.LEGACY_PAL_DATA(),
		.BORDER_O(border),
		.INKR_O(inkr),
		.GAMODE_O(gamode),
        .SNA_LOAD(sna_load), .SNA_HS(snapshot_hs), .SNA_VS(1'b0),
        .SNA_INKSEL(5'd0), .SNA_PALETTE(136'd0), .SNA_CONFIG(8'd0),
        .SNA_INTCNT(6'd0), .SNA_INT(1'b0), .SNA_VSDELAY(2'd0)
	);

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
		.FIELD(),.HSYNC(crtc_hs),
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
		.VIDEOD(16'hffff),
		.GAMODE(gamode),.MODE_UPDATE_EN(mode_event),
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
		.PAL_RGB(12'd0),
        .SNA_LOAD(sna_load), .SNA_REGS(regs), .SNA_ADDR(5'd0),
        .SNA_HCC(8'd1), .SNA_LINE(7'd0),
        .SNA_RASTER(5'd0), .SNA_VTA(5'd0),
        .SNA_HSW(4'd0), .SNA_VSW(4'd0), .SNA_HS(snapshot_hs),
        .SNA_VS(1'b0), .SNA_ADJ(1'b0), .SNA_MODE(2'd0)
	);

 assign dot=cen_16;assign pen_o=pen;assign hs=crtc_hs;assign de=crtc_de;
 assign hcc=vid.hcc;assign requested_mode=gamode;
endmodule
