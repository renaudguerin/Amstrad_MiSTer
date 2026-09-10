// Top wrapper module for Phase P8 unit and the production parser / asic_regs /
// MMU SNA restore seam. Amstrad.sv itself remains a Quartus-only integration.
module plus_p8_test_top (
	input clk,
	input reset,
	output [7:0] ack_vec_byte,
	output       ack_vec_valid,

	// i8255 ports
	input        ppi_cs,
	input        ppi_we,
	input        ppi_oe,
	input  [1:0] ppi_addr,
	input  [7:0] ppi_idata,
	output [7:0] ppi_odata,
	input  [7:0] ppi_ipa,
	output [7:0] ppi_opa,
	input  [7:0] ppi_ipb,
	output [7:0] ppi_opb,
	input  [7:0] ppi_ipc,
	output [7:0] ppi_opc,
	input        ppi_plus_mode,

	// plus_sna_parser ports
	input        sna_download,
	input        cpc_plus_chunk_start,
	input        cpc_plus_byte_wr,
	input  [7:0] cpc_plus_byte_data,
	output       sna_ioctl_wait,
	output       sna_busy,
	output       asic_sna_wr,
	output [13:0] asic_sna_addr,
	output [7:0] asic_sna_data,
	output       asic_sna_active,
	output [7:0] asic_sna_rmr2,
	output       asic_sna_unlock,

	// plus_model_select ports
	input  [1:0] model_plus_model,
	output       model_plus_mode,
	output       model_ram_128k,
	output       model_has_fdc,
	output       model_has_tape,

	// FDC decode test signals
	input  [15:0] fdc_test_addr,
	input         fdc_test_status17,
	input         fdc_test_plus_mode,
	input         fdc_test_has_fdc,
	output        fdc_motor_sel,
	output        u765_sel,

	// Integrated SNA lifecycle and ASIC-register reset-seam signals
	input         seam_machine_reset,
	input         seam_plus_asic_reset,
	input         seam_sna_load,
	input         seam_plus_mode,

	// B8-5: shared production apply controller + the production header
	// decoder driven from a real byte stream. seam_use_ctl=0 keeps the manual
	// seam above (old tests); =1 drives the owners from the shared controller
	// exactly as production does (new tests).
	input         seam_use_ctl,
	input         hdr_wr,          // one header byte of the current download
	input   [7:0] hdr_addr,        // its SNA file offset
	input   [7:0] hdr_data,
	output  [7:0] hdr_version,
	output  [7:0] hdr_ga_config,   // decoded 0x40, for assertions
	output  [7:0] hdr_romsel,      // decoded 0x55, for assertions
	output        hdr_hsync,       // decoded B0 bit 1
	input         mmu_test_io_wr,  // unlock-sequence write strobe stimulus
	input  [7:0] mmu_test_D,       // unlock-sequence write data stimulus
	output        ctl_finish_pending,
	output  [2:0] ctl_apply_cnt,
	output        ctl_sna_load,
	output        ctl_sna_hold,
	output        ctl_owner_hold,
	output        ctl_owner_reset, // registered, production-equivalent owner reset
	output [11:0] sna_loop_cnt0,
	output [11:0] sna_loop_cnt1,
	output [11:0] sna_loop_cnt2,
	output [15:0] sna_loop_addr0,
	output [15:0] sna_loop_addr1,
	output [15:0] sna_loop_addr2,
	output [11:0] sna_pause_cnt0,
	output [11:0] sna_pause_cnt1,
	output [11:0] sna_pause_cnt2,
	output  [7:0] sna_pause_presc0,
	output  [7:0] sna_pause_presc1,
	output  [7:0] sna_pause_presc2,
	output  [4:0] sna_seq_state,

	// Legacy Gate Array I/O bus stimulus (B8-5 slice B): real &7Fxx writes, so
	// the accepted-palette-event path into asic_regs is exercised rather than
	// poked. Held inactive by default.
	input         ga_iorq_n,
	input         ga_m1_n,
	input   [1:0] ga_A,
	input   [7:0] ga_D,
	output        ga_leg_pal_wr,
	output  [4:0] ga_leg_pal_addr,
	output  [4:0] ga_leg_pal_data,
	output  [3:0] video_rgb_r,
	output  [3:0] video_rgb_g,
	output  [3:0] video_rgb_b,

	// CPU-side read port into asic_regs for test verification
	input         aregs_cs,
	input         aregs_mem_rd,
	input         aregs_mem_wr,
	input  [13:0] aregs_addr,
	input   [7:0] aregs_din,
	output  [7:0] aregs_dout,

	// Direct probed views from asic_regs. pal_probe_sel=1 hands the single
	// video-side read port to the test probe; =0 leaves it wired to asic_video
	// as production does, so palette restores can be judged from consumed RGB.
	input         pal_probe_sel,
	input   [4:0] aregs_pal_raddr,
	output [11:0] aregs_pal_rdata,
	output  [7:0] aregs_pri,
	output  [7:0] aregs_splt,
	output  [7:0] aregs_sscr,
	output  [7:0] aregs_ivr,
	output  [7:0] aregs_ssa_hi,
	output  [7:0] aregs_ssa_lo,
	output  [7:0] aregs_dcsr,
	output  [7:0] aregs_sar0_lo,
	output  [7:0] aregs_sar0_hi,
	output  [7:0] aregs_ppr0,
	output  [7:0] aregs_sar1_lo,
	output  [7:0] aregs_sar1_hi,
	output  [7:0] aregs_ppr1,
	output  [7:0] aregs_sar2_lo,
	output  [7:0] aregs_sar2_hi,
	output  [7:0] aregs_ppr2,
	output [159:0] aregs_spr_x_view,
	output [143:0] aregs_spr_y_view,
	output  [63:0] aregs_spr_mag_view,
	output [179:0] aregs_spr_pal_view,

	// Probed views from plus_mmu & asic_unlock
	output        mmu_asic_page_on,
	output        mmu_asic_unlocked,

	// Production-selected owner modules: asic_dma, asic_ga_timing, asic_video
	// DMA owner ports
	input         dma_test_hsync,
	output [15:0] dma_sar0_addr,
	output [15:0] dma_sar1_addr,
	output [15:0] dma_sar2_addr,
	output        dma_ram_req,
	output [15:0] dma_ram_addr,
	input  [15:0] dma_ram_data,
	output        dma_load_owner_o,
	output        dma_psg_active_o,
	// B8-5 slice A: actual GA consume cadence. ST_FETCH0/1/2 latch
	// ram_data at the pre-edge cclk_en_p && ram_req with no ACK, and
	// ram_req can stay high across channels (even at the same address),
	// so the C++ collector samples ram_addr on this enable, not on
	// ram_req rising edges.
	output        dma_cclk_en_p,

	// GA timing owner ports
	output  [1:0] ga_mode_out,
	output  [4:0] ga_border_out,
	output [79:0] ga_inkr_out,
	output        ga_int_n_out,
	output        ga_vsync_o,     // shaped monitor VSYNC (hcnt in 4..7)
	// Production INT_n composition (Amstrad_motherboard: the GA line and the
	// ASIC DMA request are ORed), so the aggregate level can be judged.
	output        int_n_merged,

	// Video owner ports
	input         video_crtc_cs,
	input         video_crtc_rd,
	input         video_crtc_rs,
	input   [7:0] video_crtc_din,
	output  [7:0] video_crtc_dout,
	output        video_hs,
	output        video_vs,
	output        video_de,
	output [13:0] video_ma,
	output  [4:0] video_ra,
	output  [7:0] video_hcc,      // C0
	output  [6:0] video_line_out, // C4
	output  [4:0] video_row_out,  // C9
	output        video_adj_out,  // vertical total adjust active

	// MMU test access ports
	input  [15:0] mmu_test_A,
	input         mmu_test_mem_rd,
	output        mmu_cart_valid,
	output  [4:0] mmu_cart_page,
	output [13:0] mmu_cart_offset,
	output        mmu_cart_own,
	output        mmu_cart_stall
);

	wire ga_last_raster;
	wire cpu_ack = ~ga_m1_n & ~ga_iorq_n;

	plus_fdc_decode fdc_decode
	(
		.addr(fdc_test_addr),
		.plus_mode(fdc_test_plus_mode),
		.plus_has_fdc(fdc_test_has_fdc),
		.fdc_disabled(fdc_test_status17),
		.motor_sel(fdc_motor_sel),
		.u765_sel(u765_sel)
	);

	i8255 ppi
	(
		.reset(reset),
		.clk_sys(clk),
		.cs(ppi_cs),
		.we(ppi_we),
		.oe(ppi_oe),
		.addr(ppi_addr),
		.idata(ppi_idata),
		.odata(ppi_odata),
		.ipa(ppi_ipa),
		.opa(ppi_opa),
		.ipb(ppi_ipb),
		.opb(ppi_opb),
		.ipc(ppi_ipc),
		.opc(ppi_opc),
		.plus_mode(ppi_plus_mode),
		.sna_load(1'b0),
		.sna_opa(8'd0),
		.sna_opb(8'd0),
		.sna_opc(8'd0),
		.sna_control(8'd0)
	);

	plus_sna_apply sna_ctl
	(
		.clk(clk),
		.sna_download(sna_download),
		.romdl_wait(1'b0),
		.boot_wr(1'b0),
		.sna_rle_count(8'd0),
		.plus_sna_busy(sna_busy),
		.finish_pending(ctl_finish_pending),
		.apply_cnt(ctl_apply_cnt),
		.sna_load(ctl_sna_load),
		.sna_hold(ctl_sna_hold),
		.owner_reset_hold(ctl_owner_hold)
	);

	// Production registers the top reset; the owners therefore release one
	// cycle before the apply pulse and DIRSet is never held through reset.
	reg ctl_reset_q;
	initial ctl_reset_q = 1'b0;
	always @(posedge clk) ctl_reset_q <= ctl_owner_hold;
	assign ctl_owner_reset = ctl_reset_q;

	// Owner reset/load source: manual seam (old tests) or shared
	// controller (new B8-5 tests), selected per test.
	wire dma_rst_src = seam_use_ctl ? (reset | ctl_owner_reset | ~seam_plus_mode) :
	                                  (seam_machine_reset | ~seam_plus_mode);
	wire mmu_rst_src = seam_use_ctl ? (reset | ctl_owner_reset) : seam_machine_reset;
	wire vid_rst_src = seam_use_ctl ? (reset | ctl_owner_reset) : seam_machine_reset;
	wire sna_load_src = seam_use_ctl ? ctl_sna_load : seam_sna_load;

	// Production header decoder, driven by the test's real byte stream.
	wire  [4:0] hdr_inksel;
	wire [135:0] hdr_palette;
	wire  [4:0] hdr_crtc_addr;
	wire [143:0] hdr_crtc_regs;
	wire  [7:0] hdr_crtc_hcc;
	wire  [6:0] hdr_crtc_line;
	wire  [4:0] hdr_crtc_raster;
	wire  [4:0] hdr_crtc_vta;
	wire  [3:0] hdr_crtc_hsw;
	wire  [3:0] hdr_crtc_vsw;
	wire        hdr_crtc_vs;
	wire        hdr_crtc_adj;
	wire  [1:0] hdr_ga_vsdelay;
	wire  [5:0] hdr_ga_intcnt;
	wire        hdr_int_pending;

	plus_sna_header sna_header
	(
		.clk(clk),
		.sna_download(sna_download),
		.wr(hdr_wr),
		.addr(hdr_addr),
		.data(hdr_data),

		.version(hdr_version),
		.v3(),

		.ga_inksel(hdr_inksel),
		.ga_palette(hdr_palette),
		.ga_config(hdr_ga_config),
		.crtc_addr(hdr_crtc_addr),
		.crtc_regs(hdr_crtc_regs),
		.rom_select(hdr_romsel),

		.crtc_hcc(hdr_crtc_hcc),
		.crtc_line(hdr_crtc_line),
		.crtc_raster(hdr_crtc_raster),
		.crtc_vta(hdr_crtc_vta),
		.crtc_hsw(hdr_crtc_hsw),
		.crtc_vsw(hdr_crtc_vsw),
		.crtc_vs(hdr_crtc_vs),
		.crtc_hs(hdr_hsync),
		.crtc_adj(hdr_crtc_adj),
		.ga_vsdelay(hdr_ga_vsdelay),
		.ga_intcnt(hdr_ga_intcnt),
		.int_pending(hdr_int_pending)
	);

	// Same B4 attribution rule as Amstrad_motherboard: a pending flag the
	// restored DCSR already explains belongs to the DMA path, not the GA.
	wire aregs_dma_int_req;
	wire hdr_ga_int_pending = hdr_int_pending & ~aregs_dma_int_req;

	plus_sna_parser sna_parser
	(
		.clk(clk),
		.reset(reset),
		.sna_download(sna_download),
		.cpc_plus_chunk_start(cpc_plus_chunk_start),
		.cpc_plus_byte_wr(cpc_plus_byte_wr),
		.cpc_plus_byte_data(cpc_plus_byte_data),
		.ioctl_wait(sna_ioctl_wait),
		.busy(sna_busy),
		.asic_sna_wr(asic_sna_wr),
		.asic_sna_addr(asic_sna_addr),
		.asic_sna_data(asic_sna_data),
		.asic_sna_active(asic_sna_active),
		.asic_sna_rmr2(asic_sna_rmr2),
		.asic_sna_unlock(asic_sna_unlock),
		.asic_sna_loop_cnt0(sna_loop_cnt0),
		.asic_sna_loop_cnt1(sna_loop_cnt1),
		.asic_sna_loop_cnt2(sna_loop_cnt2),
		.asic_sna_loop_addr0(sna_loop_addr0),
		.asic_sna_loop_addr1(sna_loop_addr1),
		.asic_sna_loop_addr2(sna_loop_addr2),
		.asic_sna_pause_cnt0(sna_pause_cnt0),
		.asic_sna_pause_cnt1(sna_pause_cnt1),
		.asic_sna_pause_cnt2(sna_pause_cnt2),
		.asic_sna_pause_presc0(sna_pause_presc0),
		.asic_sna_pause_presc1(sna_pause_presc1),
		.asic_sna_pause_presc2(sna_pause_presc2),
		.asic_sna_seq_state(sna_seq_state)
	);

	plus_model_select model_select
	(
		.plus_model(model_plus_model),
		.plus_mode(model_plus_mode),
		.ram_128k(model_ram_128k),
		.has_fdc(model_has_fdc),
		.has_tape(model_has_tape)
	);

	asic_regs aregs
	(
		.clk(clk),
		.reset(seam_plus_asic_reset),

		.asic_cs(aregs_cs),
		.mem_wr(aregs_mem_wr),
		.mem_rd(aregs_mem_rd),
		.A(aregs_addr),
		.D_in(aregs_din),
		.D_out(aregs_dout),

		// B8-3 accepted legacy palette event, straight from the GA as
		// production wires it (B8-5 slice B: proves a snapshot apply emits no
		// event, and that the first CPU write afterwards still lands).
		.leg_pal_wr(ga_leg_pal_wr),
		.leg_pal_addr(ga_leg_pal_addr),
		.leg_pal_data(ga_leg_pal_data),

		.leg_border(5'd16),
		.leg_inkr(80'd0),

		.pal_raddr(pal_probe_sel ? aregs_pal_raddr : video_pal_addr),
		.pal_rdata(aregs_pal_rdata),

		.pri(aregs_pri),
		.splt(aregs_splt),
		.sscr(aregs_sscr),
		.ivr(aregs_ivr),
		.ssa_hi(aregs_ssa_hi),
		.ssa_lo(aregs_ssa_lo),
		.dcsr(aregs_dcsr),

		.intack_raster(ga_last_raster),
		.intack(cpu_ack),
		.int_pending(~ga_int_n_out),
		.vec_byte(ack_vec_byte),
		.vec_valid(ack_vec_valid),

		.dma_int_set(dma_int_set),

		.sprq_req(1'b0),
		.sprq_addr(11'd0),
		.sprq_data(),
		.sprq_ack(),

		.spr_acc_en(),
		.spr_acc_idx(),
		.spr_wr_en(),
		.spr_wr_addr(),
		.spr_wr_data(),

		.spr_x_view(aregs_spr_x_view),
		.spr_y_view(aregs_spr_y_view),
		.spr_mag_view(aregs_spr_mag_view),
		.spr_pal_view(aregs_spr_pal_view),

		.sar0_lo(aregs_sar0_lo), .sar0_hi(aregs_sar0_hi), .ppr0(aregs_ppr0), .sar0_wr(dma_sar0_wr),
		.sar1_lo(aregs_sar1_lo), .sar1_hi(aregs_sar1_hi), .ppr1(aregs_ppr1), .sar1_wr(dma_sar1_wr),
		.sar2_lo(aregs_sar2_lo), .sar2_hi(aregs_sar2_hi), .ppr2(aregs_ppr2), .sar2_wr(dma_sar2_wr),
		.dcsr_ena_out(dma_dcsr_ena),
		.dcsr_ena_clr(dma_dcsr_ena_clr),
		.dma_int_req(aregs_dma_int_req),

		.sna_wr(asic_sna_wr),
		.sna_addr(asic_sna_addr),
		.sna_data(asic_sna_data),

		// B8-5 palette provenance: a plain SNA (no CPC+ chunk) translates the
		// header's hardware colours; a CPC+ snapshot keeps its 12-bit entries.
		.sna_pal_load(sna_load_src),
		.sna_pal_plain(~asic_sna_active),
		.sna_pal_hdr(hdr_palette)
	);

	wire        dma_sar0_wr;
	wire        dma_sar1_wr;
	wire        dma_sar2_wr;
	wire [2:0]  dma_dcsr_ena;
	wire [2:0]  dma_dcsr_ena_clr;
	wire [2:0]  dma_int_set;
	wire        plus_cclk_en_p;
	wire        plus_cclk_en_n;
	assign dma_cclk_en_p = plus_cclk_en_p;

	// DMA HSYNC source: production wires the selected video HSYNC straight
	// into the DMA (as the old tests do via the OR below). The new B8-5
	// tests drive the settled header/source boundary deliberately through
	// dma_test_hsync alone — the free-running reset-default video would
	// otherwise inject uncounted edges — and the final slice connects the
	// real selected video.
	wire dma_hsync_src = seam_use_ctl ? dma_test_hsync :
	                                   (video_hs | dma_test_hsync);

	asic_dma dma_sound
	(
		.clk(clk),
		.reset(dma_rst_src),
		.cclk_en_p(plus_cclk_en_p),
		.cclk_en_n(plus_cclk_en_n),
		.hsync(dma_hsync_src),

		.sna_load(sna_load_src),
		.sna_loop_cnt0(sna_loop_cnt0),
		.sna_loop_cnt1(sna_loop_cnt1),
		.sna_loop_cnt2(sna_loop_cnt2),
		.sna_loop_addr0(sna_loop_addr0),
		.sna_loop_addr1(sna_loop_addr1),
		.sna_loop_addr2(sna_loop_addr2),
		.sna_pause_cnt0(sna_pause_cnt0),
		.sna_pause_cnt1(sna_pause_cnt1),
		.sna_pause_cnt2(sna_pause_cnt2),
		.sna_pause_presc0(sna_pause_presc0),
		.sna_pause_presc1(sna_pause_presc1),
		.sna_pause_presc2(sna_pause_presc2),
		.sna_hsync(hdr_hsync),

		.sar0_lo(aregs_sar0_lo),
		.sar0_hi(aregs_sar0_hi),
		.ppr0(aregs_ppr0),
		.sar0_wr(dma_sar0_wr),

		.sar1_lo(aregs_sar1_lo),
		.sar1_hi(aregs_sar1_hi),
		.ppr1(aregs_ppr1),
		.sar1_wr(dma_sar1_wr),

		.sar2_lo(aregs_sar2_lo),
		.sar2_hi(aregs_sar2_hi),
		.ppr2(aregs_ppr2),
		.sar2_wr(dma_sar2_wr),

		.dcsr_ena(dma_dcsr_ena),
		.dcsr_ena_clr(dma_dcsr_ena_clr),
		.dma_int_set(dma_int_set),

		.sar0_addr(dma_sar0_addr),
		.sar1_addr(dma_sar1_addr),
		.sar2_addr(dma_sar2_addr),

		.ram_req(dma_ram_req),
		.ram_addr(dma_ram_addr),
		.ram_data(dma_ram_data),
		.cpu_psg_addr(8'd0),
		.cpu_ppi_access(1'b0),
		.cpu_psg_write(1'b0),
		.dma_load_owner(dma_load_owner_o),
		.dma_load_busy(),

		.psg_bdir(),
		.psg_bc1(),
		.psg_dout(),
		.psg_active(dma_psg_active_o)
	);

	wire video_adj;
	wire [6:0] video_line;
	wire [4:0] video_row;
	wire [4:0] video_pal_addr;

	assign video_line_out = video_line;
	assign video_row_out  = video_row;
	assign video_adj_out  = video_adj;
	assign int_n_merged   = ga_int_n_out & ~aregs_dma_int_req;

	asic_ga_timing asic_ga
	(
		.clk(clk),
		.cen_16(1'b1),
		.fast(1'b0),
		.RESET_N(~vid_rst_src),
		.plus_unlocked(mmu_asic_unlocked),
		.A(ga_A),
		.D(ga_D),
		.MREQ_N(1'b1),
		.M1_N(ga_m1_n),
		.RD_N(1'b1),
		.IORQ_N(ga_iorq_n),
		.HSYNC_I(video_hs),
		.VSYNC_I(video_vs),
		.pri(aregs_pri),
		.crtc_line({video_line[5:0], video_row[2:0]}),
		.crtc_adj(video_adj),
		.intack(cpu_ack),
		.int_last_raster(ga_last_raster),
		.CCLK(),
		.CCLK_EN_P(plus_cclk_en_p),
		.CCLK_EN_N(plus_cclk_en_n),
		.PHI_N(),
		.PHI_EN_N(),
		.PHI_EN_P(),
		.RAS_N(),
		.CASAD_N(),
		.CAS_N(),
		.READY(),
		.CPU_N(),
		.MWE_N(),
		.E244_N(),
		.ROMEN_N(),
		.RAMRD_N(),
		.ROM(),
		.HSYNC_O(),
		.VSYNC_O(ga_vsync_o),
		.SYNC_N(),
		.INT_N(ga_int_n_out),
		.VBLANK(),
		.MODE_SYNC_EN(),
		.MODE(),
		.BORDER_O(ga_border_out),
		.INKR_O(ga_inkr_out),
		.GAMODE_O(ga_mode_out),

		.LEGACY_PAL_WR(ga_leg_pal_wr),
		.LEGACY_PAL_ADDR(ga_leg_pal_addr),
		.LEGACY_PAL_DATA(ga_leg_pal_data),

		.SNA_LOAD(sna_load_src),
		.SNA_INKSEL(hdr_inksel),
		.SNA_PALETTE(hdr_palette),
		.SNA_CONFIG(hdr_ga_config),
		.SNA_VSDELAY(hdr_ga_vsdelay),
		.SNA_INTCNT(hdr_ga_intcnt),
		.SNA_INT(hdr_ga_int_pending),
		.SNA_VS(hdr_crtc_vs),
		.SNA_HS(hdr_hsync)
	);

	asic_video asic_vid
	(
		.CLOCK(clk),
		.CLKEN(plus_cclk_en_n),
		.nRESET(~vid_rst_src),

		.ENABLE(video_crtc_cs),
		.nCS(~video_crtc_cs),
		.R_nW(video_crtc_rd),
		.RS(video_crtc_rs),
		.DI(video_crtc_din),
		.DO(video_crtc_dout),

		.HSYNC(video_hs),
		.VSYNC(video_vs),
		.FIELD(),
		.DE(video_de),
		.MA(video_ma),
		.RA(video_ra),

		.HCC(video_hcc),
		.LINE(video_line),
		.ROW(video_row),
		.ADJ(video_adj),

		.SPLT(aregs_splt),
		.SSA({aregs_ssa_hi[5:0], aregs_ssa_lo[7:0]}),
		.SSCR(aregs_sscr),

		.PIXEN(1'b1),
		.VIDEOD(16'd0),
		.GAMODE(ga_mode_out),
		.BORDER_I(ga_border_out),
		.INKR_I(ga_inkr_out),
		.RGB_R(video_rgb_r),
		.RGB_G(video_rgb_g),
		.RGB_B(video_rgb_b),
		.PEN(),

		.HWRAP(),
		.SPR_EN(1'b0),
		.SPR_RGB(12'd0),

		// Production palette consumption: the restored 12-bit entries reach
		// the screen through this port, so palette assertions can be made on
		// consumed RGB rather than on a shadow register.
		.PAL_EN(1'b1),
		.PAL_ADDR(video_pal_addr),
		.PAL_RGB(aregs_pal_rdata),

		.SNA_LOAD(sna_load_src),
		.SNA_ADDR(hdr_crtc_addr),
		.SNA_REGS(hdr_crtc_regs),
		.SNA_HCC(hdr_crtc_hcc),
		.SNA_LINE(hdr_crtc_line),
		.SNA_RASTER(hdr_crtc_raster),
		.SNA_VTA(hdr_crtc_vta),
		.SNA_HSW(hdr_crtc_hsw),
		.SNA_VSW(hdr_crtc_vsw),
		.SNA_VS(hdr_crtc_vs),
		.SNA_HS(hdr_hsync),
		.SNA_ADJ(hdr_crtc_adj),
		.SNA_MODE(hdr_ga_config[1:0])
	);

	plus_mmu mmu
	(
		.clk(clk),
		.reset(mmu_rst_src),
		.plus_mode(seam_plus_mode),
		.gx4000(1'b0),
		.io_rd(1'b0),
		.io_wr(seam_use_ctl ? mmu_test_io_wr : 1'b0),
		.mem_rd(mmu_test_mem_rd),
		.A(mmu_test_A),
		.D(seam_use_ctl ? mmu_test_D : 8'd0),
		.rom_en(1'b1),
		.exp_n(1'b1),
		.cart_valid(mmu_cart_valid),
		.cart_page(mmu_cart_page),
		.cart_offset(mmu_cart_offset),
		.cart_ready(1'b0),
		.cart_data(8'd0),
		.cart_busy(1'b0),
		.cart_own(mmu_cart_own),
		.cart_stall(mmu_cart_stall),
		.cart_dout(),
		.asic_page_on(mmu_asic_page_on),
		.asic_unlocked(mmu_asic_unlocked),
		.sna_load(sna_load_src),
		.sna_rmr2(asic_sna_rmr2),
		.sna_unlock(asic_sna_unlock),
		.sna_ga_config(hdr_ga_config),
		.sna_romsel(hdr_romsel),
		.sna_seq_state(sna_seq_state)
	);

endmodule
