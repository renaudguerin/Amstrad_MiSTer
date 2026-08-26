/*

	Converted to verilog optimized and simplified
	(C) 2018 Sorgelig


--    {@{@{@{@{@{@
--  {@{@{@{@{@{@{@{@  This code is covered by CoreAmstrad synthesis r004
--  {@    {@{@    {@  A core of Amstrad CPC 6128 running on MiST-board platform
--  {@{@{@{@{@{@{@{@
--  {@  {@{@{@{@  {@  CoreAmstrad is implementation of FPGAmstrad on MiST-board
--  {@{@        {@{@   Contact : renaudhelias@gmail.com
--  {@{@{@{@{@{@{@{@   @see http://code.google.com/p/mist-board/
--    {@{@{@{@{@{@     @see FPGAmstrad at CPCWiki
--
*/

module Amstrad_motherboard
(
	input         reset,

	input         clk,
	input         ce_16,

	// Reserved capability inputs for staged Amstrad Plus integration.
	// They intentionally do not affect the classic CPC implementation yet.
	input         plus_mode,
	input         plus_ram_128k,
	input         plus_has_fdc,
	input         plus_has_tape,

	// Active-high extra WAIT for Plus cartridge-window memory reads. The
	// cartridge fetch path drives it; it is constant 0 in classic mode, so
	// the wait expression is unchanged there.
	input         plus_mem_wait,

	// Plus ASIC register page enable (RMR2 position 11, unlock-gated,
	// captured by plus_mmu). While high, memory accesses at &4000-&7FFF
	// are answered by the on-chip asic_regs page and MUST be suppressed
	// against main memory by the caller (no write-through, reference §2);
	// the caller also owns the CPU data mux for the answered reads.
	input         plus_aspage_on,
	output [7:0]  plus_asic_dout, // wired-AND-neutral page read data
	output        plus_asic_rd,   // page answering a read this cycle
	output [7:0]  plus_vec_byte,  // INT-acknowledge vector (P3, reference §7)
	output        plus_vec_valid, // high during the acknowledge cycle

	input   [6:0] joy1,
	input   [6:0] joy2,
	input         right_shift_mod,
	input         keypad_mod,
	input  [10:0] ps2_key,
	input  [24:0] ps2_mouse,
	output        joy1_sel,
	output        joy2_sel,
	output        key_nmi,
	output        key_reset,
	output  [9:0] Fn,

	input   [3:0] ppi_jumpers,
	input         crtc_type,
	input         sync_filter,
	input         no_wait,

	input         sna_load,
	input [211:0] sna_cpu_dir,
	input   [4:0] sna_crtc_addr,
	input [143:0] sna_crtc_regs,
	input   [4:0] sna_ga_inksel,
	input [135:0] sna_ga_palette,
	input   [7:0] sna_ga_config,
	input   [7:0] sna_ram_config,
	input   [7:0] sna_rom_select,
	input   [7:0] sna_ppi_a,
	input   [7:0] sna_ppi_b,
	input   [7:0] sna_ppi_c,
	input   [7:0] sna_ppi_control,
	input   [3:0] sna_psg_addr,
	input [127:0] sna_psg_regs,

	input         tape_in,
	output        tape_out,
	output        tape_motor,

	output  [7:0] audio_l,
	output  [7:0] audio_r,

	output  [1:0] mode,

	// 4-bit-per-channel video (P2 widening). In Plus mode these carry the
	// ASIC palette's native 4-bit levels. In classic mode the low two bits
	// hold the netlist's raw {level, OE_N} pair unchanged — the consumption
	// point (color_mix) keeps its exact GA-DAC behaviour; the upper bits
	// are zero there.
	output  [3:0] red,
	output  [3:0] green,
	output  [3:0] blue,
	output        hblank,
	output        vblank,
	output        hsync,
	output        vsync,
	output        field,

	input  [15:0] vram_din,
	output reg [14:0] vram_addr,

	input [255:0] rom_map,
	input         ram64k,
	output [22:0] mem_addr,
	output        mem_rd,
	output        mem_wr,
	output        romen,
	// expansion port
	output        phi_n,
	output        phi_en_n,
	output        phi_en_p,
	output [15:0] cpu_addr,
	output  [7:0] cpu_dout,
	input   [7:0] cpu_din,
	output        iorq,
	output        mreq,
	output        rd,
	output        wr,
	output        m1,
	output reg [7:0] io_bus_byte,
	output        ga_ready,
	input         irq,
	input         nmi,
	output        cursor
);

wire crtc_shift;


wire io_rd = ~(RD_n | IORQ_n);
wire io_wr = ~(WR_n | IORQ_n);

assign mem_rd = ~(RD_n | MREQ_n);
assign mem_wr = ~(WR_n | MREQ_n);

assign cpu_dout = D;
assign cpu_addr = A;
assign m1 = ~M1_n;
assign iorq = ~IORQ_n;
assign mreq = ~MREQ_n;
assign rd = ~RD_n;
assign wr = ~WR_n;
assign romen = ~romen_n;
assign ga_ready = ready;
wire [15:0] A;
wire  [7:0] D;
wire RD_n;
wire WR_n;
wire MREQ_n;
wire IORQ_n;
wire RFSH_n;
wire ga_int_n;
wire INT_n = plus_mode ? plus_int_n : ga_int_n;
wire M1_n;
wire [7:0] cpu_data_bus = crtc_dout_sel & ppi_dout & cpu_din;
wire [7:0] plus_io_data = io_rd ? io_bus_byte : D;

// Write-only Plus ports see the byte left by the final opcode fetch of the
// current instruction, not T80's undriven/stale DO value ([KT] Ports). This
// is the ASIC's open-bus source for IN-performs-write traps.
always @(posedge clk) begin
	if (reset) io_bus_byte <= 8'hFF;
	else if (~M1_n & ~MREQ_n & ~RD_n) io_bus_byte <= cpu_data_bus;
end

T80pa CPU
(
	.reset_n(~reset),
	
	.clk(clk),
	.cen_p(phi_en_p),
	.cen_n(phi_en_n),

	.a(A),
	.do(D),
	.di(cpu_data_bus),

	.rd_n(RD_n),
	.wr_n(WR_n),
	.iorq_n(IORQ_n),
	.mreq_n(MREQ_n),
	.m1_n(M1_n),
	.rfsh_n(RFSH_n),

	.busrq_n(1),
	.int_n(INT_n & ~irq),
	.nmi_n(~nmi),
	// plus_mem_wait stretches cartridge-window read cycles beyond the
	// no_wait fast-timing option: correctness outranks the speed hack.
	.wait_n((ready | (IORQ_n & MREQ_n) | no_wait) & ~plus_mem_wait), // workaround a bug in T80pa: should wait only in memory or io cycles
	.DIRSet(sna_load),
	.DIR(sna_cpu_dir)
);

wire crtc_hs, crtc_vs, crtc_de;
wire [13:0] MA;
wire  [4:0] RA;
wire  [7:0] crtc_dout;
wire  [7:0] plus_crtc_dout;
// Only the selected machine's CRTC may participate in the wired-AND CPU
// data bus. The explicit mux also keeps plus_mode=0 invariant.
wire  [7:0] crtc_dout_sel = plus_mode ? plus_crtc_dout : crtc_dout;

CRTC crtc
(
	.CLOCK(clk),
	.CLKEN(cclk_en_n),
	.nCLKEN(cclk_en_p),
	.nRESET(~reset),
	.CRTC_TYPE(crtc_type),

	.ENABLE(io_rd | io_wr),
	.nCS(A[14]),
	.R_nW(A[9]),
	.RS(A[8]),
	.DI(~RD_n ? 8'hFF : D),
	.DO(crtc_dout),

	.SNA_LOAD(sna_load),
	.SNA_ADDR(sna_crtc_addr),
	.SNA_REGS(sna_crtc_regs),

	.VSYNC(crtc_vs),
	.HSYNC(crtc_hs),
	.DE(crtc_de),
	.FIELD(field),
	.CURSOR(cursor),

	.MA(MA),
	.RA(RA)
);

// -----------------------------------------------------------------------
// Plus ASIC video/timing path (plus_mode == 1). Both subsystems always
// instantiate (the house pattern here is capability muxes, no generate);
// in classic mode their outputs are muxed away and the classic CRTC +
// ga40010 path is untouched bit-for-bit, which the classic suites and the
// soak hash pin. See docs/plus/architecture.md §2/§5 Risk 1.
// -----------------------------------------------------------------------
wire plus_crtc_hs, plus_crtc_vs, plus_crtc_de;
wire [13:0] plus_ma;
wire [4:0]  plus_ra;
wire [6:0]  plus_vc;   // CRTC3 char line counter (VC; [5:0] used by PRI)
wire [4:0]  plus_rc;   // C9 raster count within the char line
wire        plus_adj;  // vertical adjustment active
wire        plus_ga_int_n;
wire        plus_dma_int_req;
wire        plus_int_n = plus_ga_int_n & ~plus_dma_int_req;
wire        plus_ready, plus_ras_n, plus_cas_n, plus_cpu_n;
wire        plus_cclk_en_p, plus_cclk_en_n;
wire        plus_phi_en_p, plus_phi_en_n, plus_phi_n;
wire        plus_hsync_o, plus_vsync_o, plus_vblank;
wire [4:0]  asic_border;
wire [79:0] asic_inkr;
wire [1:0]  plus_gamode;
wire [3:0]  plus_rgb_r, plus_rgb_g, plus_rgb_b;
reg  [15:0] plus_vidword;
// P4 sprite engine plumbing (asic_video seam + composited plane, and the
// asic_regs video services it consumes).
wire         plus_hwrap;
wire         plus_spr_en;
wire  [11:0] plus_spr_rgb;
wire         plus_spr_fq_req;
wire  [10:0] plus_spr_fq_addr;
wire   [7:0] plus_spr_fq_data;
wire         plus_spr_fq_ack;
wire         plus_spr_acc_en;
wire   [3:0] plus_spr_acc_idx;
wire [159:0] plus_spr_x;
wire [143:0] plus_spr_y;
wire  [63:0] plus_spr_mag;
wire [179:0] plus_spr_pal;

// Selected raster sources: classic path when Plus model = Off.
wire [13:0] ma_sel = plus_mode ? plus_ma : MA;
wire [4:0]  ra_sel = plus_mode ? plus_ra : RA;
wire        hs_sel = plus_mode ? plus_crtc_hs : crtc_hs;
wire        vs_sel = plus_mode ? plus_crtc_vs : crtc_vs;
wire        de_sel = plus_mode ? plus_crtc_de : crtc_de;

asic_ga_timing asic_ga
(
	.clk(clk),
	.cen_16(ce_16),
	.fast(no_wait),
	.RESET_N(~reset),

	.A(A[15:14]),
	.D(plus_io_data),
	.MREQ_N(MREQ_n),
	.M1_N(M1_n),
	.RD_N(RD_n),
	.IORQ_N(IORQ_n),

	.HSYNC_I(plus_crtc_hs),
	.VSYNC_I(plus_crtc_vs),

	// P3 programmable raster interrupt: {VC5..VC0, RC2..RC0} per the
	// reference comparison; the shaped-monitor trailing edge fires.
	.pri(asic_pri),
	.crtc_line({plus_vc[5:0], plus_rc[2:0]}),
	.crtc_adj(plus_adj),
	.intack(plus_mode & ~M1_n & iorq),
	.int_last_raster(asic_int_last_raster),

	.CCLK(),
	.CCLK_EN_P(plus_cclk_en_p),
	.CCLK_EN_N(plus_cclk_en_n),
	.PHI_N(plus_phi_n),
	.PHI_EN_N(plus_phi_en_n),
	.PHI_EN_P(plus_phi_en_p),
	.RAS_N(plus_ras_n),
	.CASAD_N(),
	.CAS_N(plus_cas_n),
	.READY(plus_ready),
	.CPU_N(plus_cpu_n),
	.MWE_N(),
	.E244_N(),
	// The ASIC mirrors the GA ROM-enable decode exactly (both watch the
	// same bus), but romen for Amstrad_MMU keeps coming from ga40010 in
	// both modes; plus_mmu overlays cartridge windows on top of it.
	.ROMEN_N(),
	.RAMRD_N(),
	.ROM(),

	.HSYNC_O(plus_hsync_o),
	.VSYNC_O(plus_vsync_o),
	.SYNC_N(),
	.INT_N(plus_ga_int_n),
	.VBLANK(plus_vblank),
	.MODE_SYNC_EN(),

	// MODE and GAMODE_O alias the same RMR bits inside the module; the
	// motherboard consumes GAMODE_O, so MODE stays unconnected here.
	.MODE(),
	.BORDER_O(asic_border),
	.INKR_O(asic_inkr),
	.GAMODE_O(plus_gamode)
);

// Locked-ASIC CRTC type 3 + pixel pipeline. Register accesses share the
// classic CRTC sparse decode. On Plus hardware an IN on either write port
// performs the corresponding write with the live bus byte ([KT] Ports;
// asic-reference sections 4/13), so DI remains D during read cycles too.
asic_video asic_vid
(
	.CLOCK(clk),
	.CLKEN(plus_cclk_en_n),
	.nRESET(~reset),

	.ENABLE(io_rd | io_wr),
	.nCS(A[14]),
	.R_nW(A[9]),
	.RS(A[8]),
	.DI(plus_io_data),
	.DO(plus_crtc_dout),

	.HSYNC(plus_crtc_hs),
	.VSYNC(plus_crtc_vs),
	.DE(plus_crtc_de),
	.MA(plus_ma),
	.RA(plus_ra),

	.HCC(),
	.LINE(plus_vc),
	.ROW(plus_rc),
	.ADJ(plus_adj),

	.SPLT(asic_splt),
	.SSA({asic_ssa_hi[5:0], asic_ssa_lo[7:0]}),
	.SSCR(asic_sscr),

	.PIXEN(ce_16),
	.VIDEOD(plus_vidword),
	.GAMODE(plus_gamode),
	.BORDER_I(asic_border),
	.INKR_I(asic_inkr),
	.RGB_R(plus_rgb_r),
	.RGB_G(plus_rgb_g),
	.RGB_B(plus_rgb_b),
	.PEN(),

	.HWRAP(plus_hwrap),
	.SPR_EN(plus_spr_en),
	.SPR_RGB(plus_spr_rgb)
);

// ASIC register page (P2). The legacy GA shadow feeding its translation
// comes straight from asic_ga_timing, so PENR/INKR writes land in the
// 12-bit palette exactly as on hardware (reference §6 secondary port).
wire [7:0] asic_regs_dout;
wire       asic_regs_rd;
wire [7:0] asic_pri;
wire [7:0] asic_splt;
wire [7:0] asic_sscr;
wire [7:0] asic_ssa_hi;
wire [7:0] asic_ssa_lo;
wire       asic_int_last_raster;
// The page answers only under Plus mode: plus_mmu captures RMR2 without a
// mode gate, so a classic program emitting the unlock sequence could
// otherwise hijack the &4000-&7FFF data bus (review finding 5).
wire asic_page_active = plus_mode & plus_aspage_on;

// P7 3-channel DMA sound engine signals
wire [7:0] dma_sar0_lo, dma_sar0_hi, dma_ppr0;
wire       dma_sar0_wr;
wire [7:0] dma_sar1_lo, dma_sar1_hi, dma_ppr1;
wire       dma_sar1_wr;
wire [7:0] dma_sar2_lo, dma_sar2_hi, dma_ppr2;
wire       dma_sar2_wr;
wire [2:0] dma_dcsr_ena;
wire [2:0] dma_dcsr_ena_clr;
wire [2:0] dma_int_set;
wire [15:0] dma_ram_addr;
wire        dma_ram_req;
wire        psg_dma_bdir;
wire        psg_dma_bc1;
wire [7:0]  psg_dma_dout;
wire        psg_dma_active;

asic_regs asic_page
(
	.clk(clk),
	.reset(reset),

	.asic_cs(asic_page_active & (A[15:14] == 2'b01)),
	.mem_wr(mem_wr),
	.mem_rd(mem_rd),
	.A(A[13:0]),
	.D_in(D),
	.D_out(asic_regs_dout),

	.leg_border(asic_border),
	.leg_inkr(asic_inkr),

	.pal_raddr(5'd0),          // video-side palette port lands with the
	.pal_rdata(),              // P2 RGB widening commit

	.pri(asic_pri), .splt(asic_splt), .sscr(asic_sscr), .ivr(),
	.ssa_hi(asic_ssa_hi), .ssa_lo(asic_ssa_lo), .dcsr(),
	.intack_raster(asic_int_last_raster),
	// Acknowledge cycle (M1 low with IORQ asserted), gated to Plus mode:
	// classic machines deliver the stale wired-AND bus byte on ack, and
	// the review found the ungated form hijacking classic cpu_din.
	.intack(plus_mode & ~M1_n & iorq),
	.int_pending(~plus_ga_int_n),
	.dma_int_set(dma_int_set),
	.vec_byte(plus_vec_byte),
	.vec_valid(plus_vec_valid),

	.sprq_req(plus_spr_fq_req),
	.sprq_addr(plus_spr_fq_addr),
	.sprq_data(plus_spr_fq_data),
	.sprq_ack(plus_spr_fq_ack),
	.spr_acc_en(plus_spr_acc_en),
	.spr_acc_idx(plus_spr_acc_idx),
	.spr_x_view(plus_spr_x),
	.spr_y_view(plus_spr_y),
	.spr_mag_view(plus_spr_mag),
	.spr_pal_view(plus_spr_pal),

	.sar0_lo(dma_sar0_lo), .sar0_hi(dma_sar0_hi), .ppr0(dma_ppr0), .sar0_wr(dma_sar0_wr),
	.sar1_lo(dma_sar1_lo), .sar1_hi(dma_sar1_hi), .ppr1(dma_ppr1), .sar1_wr(dma_sar1_wr),
	.sar2_lo(dma_sar2_lo), .sar2_hi(dma_sar2_hi), .ppr2(dma_ppr2), .sar2_wr(dma_sar2_wr),
	.dcsr_ena_out(dma_dcsr_ena),
	.dcsr_ena_clr(dma_dcsr_ena_clr),
	.dma_int_req(plus_dma_int_req)
);
assign plus_asic_dout = asic_regs_dout;
assign plus_asic_rd   = asic_page_active & (A[15:14] == 2'b01) & mem_rd;

// P7 3-channel DMA sound engine
asic_dma dma_sound
(
	.clk(clk),
	.reset(reset || !plus_mode),
	.cclk_en_p(plus_cclk_en_p),
	.cclk_en_n(plus_cclk_en_n),
	.hsync(plus_crtc_hs),

	.sar0_lo(dma_sar0_lo),
	.sar0_hi(dma_sar0_hi),
	.ppr0(dma_ppr0),
	.sar0_wr(dma_sar0_wr),

	.sar1_lo(dma_sar1_lo),
	.sar1_hi(dma_sar1_hi),
	.ppr1(dma_ppr1),
	.sar1_wr(dma_sar1_wr),

	.sar2_lo(dma_sar2_lo),
	.sar2_hi(dma_sar2_hi),
	.ppr2(dma_ppr2),
	.sar2_wr(dma_sar2_wr),

	.dcsr_ena(dma_dcsr_ena),
	.dcsr_ena_clr(dma_dcsr_ena_clr),
	.dma_int_set(dma_int_set),

	.sar0_addr(),
	.sar1_addr(),
	.sar2_addr(),

	.ram_req(dma_ram_req),
	.ram_addr(dma_ram_addr),
	.ram_data(vram_din),

	.psg_bdir(psg_dma_bdir),
	.psg_bc1(psg_dma_bc1),
	.psg_dout(psg_dma_dout),
	.psg_active(psg_dma_active)
);

// P4 hardware sprite engine: compares against the CRTC3 taps ([KT]
// formulas), stages row bytes through asic_page's video port, and
// composites between screen and border inside asic_video.
asic_sprites plus_sprites
(
	.CLOCK(clk),
	.PIXEN(ce_16),
	.CLKEN(plus_cclk_en_n),
	.HWRAP(plus_hwrap),
	.nRESET(~reset),

	.LINE(plus_vc),
	.ROW(plus_rc),

	.SPR_X(plus_spr_x),
	.SPR_Y(plus_spr_y),
	.SPR_MAG(plus_spr_mag),
	.SPR_PAL(plus_spr_pal),

	.ACC_EN(plus_spr_acc_en),
	.ACC_IDX(plus_spr_acc_idx),

	.FQ_REQ(plus_spr_fq_req),
	.FQ_ADDR(plus_spr_fq_addr),
	.FQ_DATA(plus_spr_fq_data),
	.FQ_ACK(plus_spr_fq_ack),

	.SPR_EN(plus_spr_en),
	.SPR_RGB(plus_spr_rgb),
	.SPR_IDX(),
	.SPR_WIN()
);

// The caller uses plus_asic_rd to mux the CPU data bus and to suppress
// main-memory read AND write cycles for the whole &4000-&7FFF window
// while the page is enabled (no read/write-through, reference §2).

// Twice-per-character word assembly on the reference VIDEO_BUF phases:
// state e0 latches the even byte, state 03 the odd byte (ring order
// e0 -> ... -> 03 within one character). Validated end-to-end against
// the p1_video integration bench (test p1a).
always @(posedge clk) begin
	if (reset) plus_vidword <= 16'd0;
	else begin
		if (plus_cclk_en_p) plus_vidword[7:0]  <= vram_d;
		if (plus_cclk_en_n) plus_vidword[15:8] <= vram_d;
	end
end

wire [14:0] crtc_vram_addr = {ma_sel[13:12], ra_sel[2:0], ma_sel[9:0]};

reg vram_bs;
reg [7:0] vram_d;
reg [7:0] vram_din_shift;
always @(posedge clk) begin
	// simulate two 8-bit fetches in the vram cycle
	reg cas_n_old;
	cas_n_old <= cas_n;
	if (!cpu_n) vram_bs <= 0;
	else begin
		if (plus_mode && dma_ram_req)
			vram_addr <= {dma_ram_addr[15:14], dma_ram_addr[13:1]};
		else
			vram_addr <= crtc_vram_addr;
		if (!ras_n & !cas_n_old & cas_n) vram_bs <= 1;
		if (!ras_n & !cas_n)
			if (sync_filter & crtc_shift) begin
				if (vram_bs) vram_din_shift <= de_sel ? vram_din[15:8] : 8'd0;
				vram_d <= vram_bs ? vram_din[7:0] : vram_din_shift;
			end else
				vram_d <= vram_bs ? vram_din[15:8] : vram_din[7:0];
	end
end

wire cclk_en_n, cclk_en_p;
wire ga_cclk_en_n, ga_cclk_en_p;
assign cclk_en_p = plus_mode ? plus_cclk_en_p : ga_cclk_en_p;
assign cclk_en_n = plus_mode ? plus_cclk_en_n : ga_cclk_en_n;

wire e244_n;
wire ga_cpu_n, ga_ras_n, ga_cas_n;
wire cpu_n = plus_mode ? plus_cpu_n : ga_cpu_n;
wire ras_n = plus_mode ? plus_ras_n : ga_ras_n;
wire cas_n = plus_mode ? plus_cas_n : ga_cas_n;
wire [7:0] ga_din = e244_n ? vram_d : D;
wire ga_ready_o;
wire ready = plus_mode ? plus_ready : ga_ready_o;
wire romen_n;

wire ga_hsync_o, hsync_filtered;
wire ga_vsync_o, vsync_filtered;

wire hblank_filtered;
wire ga_vblank_o, vblank_filtered;

wire hsync_ga = plus_mode ? plus_hsync_o : ga_hsync_o;
wire vsync_ga = plus_mode ? plus_vsync_o : ga_vsync_o;
wire vblank_ga = plus_mode ? plus_vblank : ga_vblank_o;

assign hsync = sync_filter ? hsync_filtered : hsync_ga;
assign vsync = sync_filter ? vsync_filtered : vsync_ga;
assign hblank = sync_filter ? hblank_filtered : hs_sel;
assign vblank = sync_filter ? vblank_filtered : vblank_ga;

crt_filter crt_filter
(
	.CLK(clk),
	.CE_4(phi_en_n),
	.HSYNC_I(hs_sel),
	.VSYNC_I(vs_sel),
	.HSYNC_O(hsync_filtered),
	.VSYNC_O(vsync_filtered),
	.HBLANK(hblank_filtered),
	.VBLANK(vblank_filtered),
	.SHIFT(crtc_shift)
);

// Screen mode and RGB: classic netlist pair passes through in the low
// bits (consumption-point conversion), locked-ASIC 4-bit levels are
// native (P2 widening; the temporary lvl4_to_ga adapter is gone).
wire [1:0] ga_mode;
wire [1:0] ga_red, ga_green, ga_blue;
assign mode  = plus_mode ? plus_gamode : ga_mode;
assign red   = plus_mode ? plus_rgb_r : {2'b00, ga_red};
assign green = plus_mode ? plus_rgb_g : {2'b00, ga_green};
assign blue  = plus_mode ? plus_rgb_b : {2'b00, ga_blue};

// CPU/expansion phase enables follow the selected machine. The ASIC path
// replicates ga40010's timing contract cycle-exactly today (asic_ga_timing
// lockstep bench), so this mux is behaviour-neutral now; it makes asic_ga
// the Plus-mode owner so any deliberate Plus timing delta lands everywhere
// at once (CPU, crt_filter CE, expansion header).
wire ga_phi_n, ga_phi_en_n, ga_phi_en_p;
assign phi_n    = plus_mode ? plus_phi_n    : ga_phi_n;
assign phi_en_n = plus_mode ? plus_phi_en_n : ga_phi_en_n;
assign phi_en_p = plus_mode ? plus_phi_en_p : ga_phi_en_p;

ga40010 GateArray (
	.clk(clk),
	.cen_16(ce_16),
	.fast(no_wait),
	.RESET_N(~reset),
	.A(A[15:14]),
	.D(ga_din),
	.MREQ_N(MREQ_n),
	.M1_N(M1_n),
	.RD_N(RD_n),
	.IORQ_N(IORQ_n),
	.HSYNC_I(crtc_hs),
	.VSYNC_I(crtc_vs),
	.DISPEN(crtc_de),
	.CCLK(),
	.CCLK_EN_P(ga_cclk_en_p),
	.CCLK_EN_N(ga_cclk_en_n),
	.PHI_N(ga_phi_n),
	.PHI_EN_N(ga_phi_en_n),
	.PHI_EN_P(ga_phi_en_p),
	.RAS_N(ga_ras_n),
	.CAS_N(ga_cas_n),
	.READY(ga_ready_o),
	.CASAD_N(),
	.CPU_N(ga_cpu_n),
	.MWE_N(),
	.E244_N(e244_n),
	.ROMEN_N(romen_n),
	.RAMRD_N(),
	.HSYNC_O(ga_hsync_o),
	.VSYNC_O(ga_vsync_o),
	.VBLANK(ga_vblank_o),
	.MODE(ga_mode),
	.SYNC_N(),
	.INT_N(ga_int_n),
	.BLUE_OE_N(ga_blue[0]),
	.BLUE(ga_blue[1]),
	.GREEN_OE_N(ga_green[0]),
	.GREEN(ga_green[1]),
	.RED_OE_N(ga_red[0]),
	.RED(ga_red[1]),
	.SNA_LOAD(sna_load),
	.SNA_INKSEL(sna_ga_inksel),
	.SNA_PALETTE(sna_ga_palette),
	.SNA_CONFIG(sna_ga_config)
);

Amstrad_MMU MMU
(
	.CLK(clk),
	.reset(reset),
	.ram64k(ram64k),
	.romen_n(romen_n),
	.rom_map(rom_map),
	.A(A),
	.D(D),
	.io_WR(io_wr),
	.sna_load(sna_load),
	.sna_ram_config(sna_ram_config),
	.sna_rom_select(sna_rom_select),
	.ram_A(mem_addr)
);

wire [7:0] ppi_dout;
wire [7:0] portC;
wire [7:0] portAout;
wire [7:0] portAin;

i8255 PPI
(
	.reset(reset),
	.clk_sys(clk),

	.addr(A[9:8]),
	.idata(D),
	.odata(ppi_dout),
	.cs(~A[11]),
	.we(io_wr),
	.oe(io_rd),

	.ipa(portAin), 
	.opa(portAout),
	.ipb({tape_in, 2'b11, ppi_jumpers, vs_sel}),
	.opb(),
	.ipc(8'hFF), 
	.opc(portC),

	.sna_load(sna_load),
	.sna_opa(sna_ppi_a),
	.sna_opb(sna_ppi_b),
	.sna_opc(sna_ppi_c),
	.sna_control(sna_ppi_control)
);

assign tape_motor = portC[4];
assign tape_out   = portC[5];

assign audio_l = {1'b0, ch_a[7:1]} + {2'b00, ch_b[7:2]};
assign audio_r = {1'b0, ch_c[7:1]} + {2'b00, ch_b[7:2]};

wire psg_bc_mux   = (plus_mode && psg_dma_active) ? psg_dma_bc1  : portC[6];
wire psg_bdir_mux = (plus_mode && psg_dma_active) ? psg_dma_bdir : portC[7];
wire [7:0] psg_di_mux = (plus_mode && psg_dma_active) ? psg_dma_dout : portAout;

wire [7:0] ch_a, ch_b, ch_c;
YM2149 PSG
(
	.RESET(reset),

	.CLK(clk),
	.CE(cclk_en_p),
	.SEL(0),
	.MODE(0),

	.BC(psg_bc_mux),
	.BDIR(psg_bdir_mux),
	.DI(psg_di_mux),
	.DO(portAin),

	.CHANNEL_A(ch_a),
	.CHANNEL_B(ch_b),
	.CHANNEL_C(ch_c),

	.IOA_in(kbd_out),
	.IOB_in(8'hFF),

	.SNA_LOAD(sna_load),
	.SNA_ADDR(sna_psg_addr),
	.SNA_REGS(sna_psg_regs)
);

wire [7:0] kbd_out;
hid HID
(
	.reset(reset),
	.clk(clk),
	
	.right_shift_mod(right_shift_mod),
	.keypad_mod(keypad_mod),

	.ps2_key(ps2_key),
	.ps2_mouse(ps2_mouse),

	.joystick1(joy1),
	.joystick2(joy2),

	.Y(portC[3:0]),
	.X(kbd_out),
	.key_nmi(key_nmi),
	.key_reset(key_reset),
	.Fn(Fn)
);

assign joy1_sel = (portC[3:0] == 9);
assign joy2_sel = (portC[3:0] == 6);

endmodule
