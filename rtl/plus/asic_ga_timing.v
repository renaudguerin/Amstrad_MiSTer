//============================================================================
//  Amstrad Plus ASIC (AMS40489) — locked-ASIC Gate Array support path:
//  bus sequencer, CPU/memory strobes, sync filtering and interrupt
//  generation, and the legacy Gate Array register file.
//
//  Architecture role (docs/plus/architecture.md §2/§5 Risk 1, decision
//  recorded 2026-08-24): in Plus mode the Z80/SDRAM timing contract that the
//  classic path receives from the ga40010 netlist is reproduced HERE,
//  behaviourally, so no netlist-derived Gate Array personality runs in the
//  Plus path. Rasterisation itself lives in rtl/plus/asic_video.v; this
//  module provides everything else ga40010 provided: the 16 MHz sequencer,
//  CCLK/PHI/READY/RAS/CAS generation, the monitor-side sync shaper, the
//  52-line interrupt counter, and the legacy GA registers (INKR/BORDER/RMR)
//  that drive asic_video's GAMODE/BORDER_I/INKR_I inputs.
//
//  The state machines follow the published Gate Array timing description in
//  "40010 simplified" V03 by Gerald (as does rtl/GA40010/ga40010.sv); this
//  is an independent synchronous expression written for the Plus stream,
//  licensed like its source. Cycle-exact equivalence with the synthesised
//  ga40010 path (ga40010.sv + casgen_sync + syncgen_sync + rslatch) is
//  pinned by the lockstep differential bench
//  sim/plus/asic_ga_timing_diff_test.cpp, which drives both modules with
//  identical randomised bus/sync traffic and compares every shared output
//  on every clock edge. Register payload decoding (border/INKR/mode/ROM
//  mapping) is additionally covered by directed vectors, because ga40010
//  does not export those registers for comparison.
//
//  Deliberate differences from ga40010 (each documented, none behavioural
//  for the shared contract):
//   - No snapshot (SNA) preload ports: snapshots are unsupported in Plus
//     mode (architecture §5.6). Power-up values are therefore defined
//     constants instead of uninitialised storage: border resets to 5'b10000
//     and RMR to 0 exactly as ga40010 resets them, and INKR entries plus
//     ink select carry an explicit RTL reset to 0 (ga40010 leaves those to
//     FPGA power-up init); the INKR/ink-select power-up value is a named
//     unverified model assumption (real ASIC power-up contents undocumented).
//   - No video-buffer/RGB section: asic_video owns rasterisation.
//   - No DISPEN input: only the omitted video buffer consumed it.
//   - No ``ifdef VERILATOR`` shadow domain: the *_sync variants this module
//     mirrors are already fully synchronous.
//============================================================================

module asic_ga_timing
(
	input        clk,
	input        cen_16,    // 16 MHz dot-clock enable (64 MHz / 4)
	input        fast,      // CPU won't WAIT speed hack (OSD no_wait)

	input        RESET_N,

	// Z80 bus (chip-side view, same signals ga40010 samples). D[5] is
	// genuinely undecoded, exactly as in the reference.
	/* verilator lint_off UNUSEDSIGNAL */
	input [15:14] A,
	input [7:0]  D,
	/* verilator lint_on UNUSEDSIGNAL */
	input        MREQ_N,
	input        M1_N,
	input        RD_N,
	input        IORQ_N,

	// CRTC type-3 raster inputs (asic_video outputs)
	input        HSYNC_I,
	input        VSYNC_I,

	// ---- Bus/clock contract (mirrors ga40010 outputs consumed by the
	// motherboard: T80pa enables/wait, VRAM fetch block, crt_filter CE) ----
	output       CCLK,
	output       CCLK_EN_P,
	output       CCLK_EN_N,
	output reg   PHI_N,
	output       PHI_EN_N,
	output       PHI_EN_P,
	output reg   RAS_N,
	output       CAS_N,
	output reg   CASAD_N,
	output       READY,
	output       CPU_N,
	output       MWE_N,
	output       E244_N,
	output       ROMEN_N,
	output       RAMRD_N,
	output       ROM,
	// Video-buffer latch windows: CCLK_EN_P (ring state e0) and CCLK_EN_N
	// (state 03) are exactly the twice-per-character VIDEO_BUF phases of the
	// reference; the motherboard samples the VRAM byte on each to assemble
	// the 16-bit VIDEOD word for asic_video.

	// ---- Sync shaping / interrupts (mirrors ga40010 sync outputs) ----
	output       HSYNC_O,
	output       VSYNC_O,
	output       SYNC_N,
	output reg   INT_N,
	output       VBLANK,    // ga40010 HCNTLT28 analogue
	output       MODE_SYNC_EN, // screen-mode resync strobe (asic_video
	                          // currently re-latches GAMODE on raw HSYNC;
	                          // this is the netlist's equivalent phase
	                          // reference for the P1 pixel-phase check)
	output [1:0] MODE,

	// ---- Legacy GA register file, driving asic_video ----
	output [4:0]  BORDER_O,   // border hardware colour number
	output [79:0] INKR_O,     // 16 hardware colour numbers, entry k at
	                          // [k*5 +: 5] (asic_video INKR_I packing)
	output [1:0]  GAMODE_O    // RMR VM bits, unsynchronised (asic_video
	                          // re-latches on HSYNC itself)
);

	wire reset = ~RESET_N;

//----------------------------------------------------------------------
// Sequencer S: eight-phase ring advanced by cen_16. U204 restarts the
// ring from bit 1 on an interrupt-acknowledge-flavoured bus state and is
// simultaneously part of the normal counting term (S[6] & ~S[7]).
//----------------------------------------------------------------------

	reg [7:0] S;
	wire U204 = (reset & ~M1_N & ~IORQ_N & ~RD_N) | (S[6] & ~S[7]);

	always @(posedge clk) begin
		if (cen_16) begin
			S[0] <= ~S[7];
			S[1] <= S[0] | U204;
			S[2] <= S[1] | U204;
			S[3] <= S[2] | U204;
			S[4] <= S[3] | U204;
			S[5] <= S[4] | U204;
			S[6] <= S[5] | U204;
			S[7] <= S[6];
		end
	end

	// PHI/RAS decode, registered on the dot clock.
	always @(posedge clk) begin
		if (cen_16) begin
			PHI_N   <= (S[1] ^ S[3]) | (S[5] ^ S[7]);
			RAS_N   <= (S[6] | ~S[2]) & S[0];
			CASAD_N <= RAS_N;
		end
	end

	assign PHI_EN_N = cen_16 & (S == 8'hc0 || S == 8'h03 || S == 8'h3f || S == 8'hfc);
	assign PHI_EN_P = cen_16 & (S == 8'h00 || S == 8'h0f || S == 8'hff || S == 8'hf0);
	assign CCLK     = ~(S[2] | S[5]);
	assign CCLK_EN_P = cen_16 & (S == 8'he0);
	assign CCLK_EN_N = cen_16 & (S == 8'h03);
	assign CPU_N    = ~(S[1] & ~S[7]);
	assign MWE_N    = ~(RD_N & S[0] & S[5]);
	assign E244_N   = ~(~IORQ_N & S[2] & S[3]);

	// READY: set-dominant RS latch in the main clock domain (set wins when
	// both terms are active on the same edge), holding between events.
	wire ready_set = (S[3] & ~S[6]);
	wire ready_rst = CASAD_N;
	reg  ready_hold;
	reg  ready_q;
	always @(*) begin
		if (ready_set)      ready_q = 1'b1;
		else if (ready_rst) ready_q = 1'b0;
		else                ready_q = ready_hold;
	end
	always @(posedge clk) ready_hold <= ready_q;
	assign READY = ready_q;

//----------------------------------------------------------------------
// CAS generation. U708 masks refresh: between the end of the M1 cycle and
// the release of MREQ, an active MREQ does not generate an active CAS
// (the DRAM row is refreshed by the CRTC address bus in that window).
// U712 stretches CAS across the CPU cycle; S_d1/S_d2 blank CAS during
// the video transfer phases.
//----------------------------------------------------------------------

	reg U708; // 0 = MREQ masked
	reg M1_N_d, MREQ_N_d;
	always @(posedge clk) begin
		M1_N_d   <= M1_N;
		MREQ_N_d <= MREQ_N;
	end

	always @(posedge clk or negedge RESET_N) begin
		if (!RESET_N)                 U708 <= 1'b1;
		else if (~M1_N_d & M1_N)      U708 <= 1'b0;
		else if (~MREQ_N_d & MREQ_N)  U708 <= 1'b1;
	end

	reg S_d1, S_d2; // u706 / u709
	always @(posedge clk) begin
		if (cen_16) begin
			S_d1 <= (~S[4] & S[5]) | (~S[3] & S[1]) | (S[1] & S[7]);
			S_d2 <= S_d1;
		end
	end

	wire U710    = ~U708 | MREQ_N | ~S[4] | S[5];
	wire u712_s  = S_d1;
	wire u712_r  = ~(S[2] & U710);
	reg  u712_hold;
	reg  u712_q;
	// Reset-dominant latch (reset wins on simultaneity).
	always @(*) begin
		if (u712_r)         u712_q = 1'b0;
		else if (u712_s)    u712_q = 1'b1;
		else                u712_q = u712_hold;
	end
	always @(posedge clk) u712_hold <= u712_q;

	assign CAS_N = u712_q | S_d1 | S_d2;

//----------------------------------------------------------------------
// Legacy GA register file: port &7Fxx decode latching on the sequencer's
// register window (or immediately in fast/no-wait mode). IRQ reset is
// RMR bit 4. Power-up/reset values match the classic FPGA power-up state
// (ga40010 relies on init for these; here they are explicit RTL resets):
// INKR entries and ink select reset to 0 — the real-ASIC power-up contents
// are undocumented (named assumption, see header).
//----------------------------------------------------------------------

	reg [4:0] inksel;
	reg [4:0] border;
	reg [79:0] inkr; // entry k at [k*5 +: 5]
	reg       hromen, lromen, mode1, mode0;

	wire reg_latch = (S[0] & S[7]) | (fast & ~E244_N);
	wire reg_sel   = reg_latch & ~IORQ_N & ~A[15] & A[14] & M1_N;
	wire ink_en    = reg_sel & ~D[7] & ~D[6];
	wire border_en = reg_sel & ~D[7] &  D[6] &  inksel[4];
	wire ctrl_en   = reg_sel &  D[7] & ~D[6];
	wire inkr_en   = reg_sel & ~D[7] &  D[6] & ~inksel[4];

	wire irq_reset = ctrl_en & D[4];

	// Explicit RTL resets for inksel/inkr: without these the power-up
	// values would be whatever the simulator/FPGA init gives them. Zero
	// matches the classic FPGA power-up state; the real ASIC power-up
	// contents are undocumented (named assumption, header comment above).
	// The !reset guards keep the reset dominant while leaving every write
	// a top-level if — the else-wrapped form trips an internal error in
	// older Verilator (5.020 V3Gate ICE).
	always @(posedge clk) begin
		if (reset) border <= 5'b10000;
		else if (border_en) border <= D[4:0];

		if (reset) {hromen, lromen, mode1, mode0} <= 4'd0;
		else if (ctrl_en) {hromen, lromen, mode1, mode0} <= D[3:0];

		if (reset) begin
			inksel <= 5'd0;
			inkr   <= 80'd0;
		end
		if (!reset && ink_en)  inksel <= D[4:0];
		if (!reset && inkr_en) inkr[inksel[3:0]*5 +: 5] <= D[4:0];
	end

	assign BORDER_O  = border;
	assign INKR_O    = inkr;
	assign GAMODE_O  = {mode1, mode0};
	assign MODE      = {mode1, mode0};

	// Lower/upper ROM mapping exactly as the classic Gate Array decodes it;
	// in Plus mode plus_mmu overlays cartridge windows on top of this.
	wire rom_map_rom = (~lromen & ~A[15] & ~A[14]) | (~hromen & A[15] & A[14]);
	assign ROM     = rom_map_rom;
	assign ROMEN_N = ~rom_map_rom | MREQ_N | RD_N;
	assign RAMRD_N =  rom_map_rom | MREQ_N | RD_N;

//----------------------------------------------------------------------
// Monitor-side sync shaping and the 52-line interrupt counter.
//
// hcnt walks a fixed 26-line sequence on each falling edge of the CRTC
// HSYNC and parks at 5'h1E if no VSYNC arrives (lost-sync state). VSYNC_O
// (monitor C-VSYNC) is emitted while hcnt sits in 4..7; VBLANK
// (HCNTLT28) while it is NOT in 28..31 — the ASIC's vertical-blank
// window. hdelay reshapes HSYNC into the monitor HSYNC_O with its own
// microsequence. intcnt counts shaped lines and fires INT_N low on the
// 52nd, cleared by RMR bit 4 or the interrupt-acknowledge cycle; the
// VSYNC-edge term (intcntclr_4) re-arms it at frame start.
//
// All of this free-runs on clk; only the vsync edge detectors are
// sampled once per character (CCLK_EN_N).
//----------------------------------------------------------------------

	reg [5:0] intcnt_reg, intcnt_next;
	reg [4:0] hcnt_reg, hcnt_next;
	reg [3:0] hdelay_reg;

	reg  vsync_d;    // u803
	wire hsync_n = ~HSYNC_I; // u801
	reg  hsync_n_d;
	reg  vsync_o_d;  // u812
	reg  irqack_rst;

	// Edge detectors.
	always @(posedge clk) begin
		hsync_n_d <= hsync_n;
		if (CCLK_EN_N) begin
			vsync_d   <= VSYNC_I;
			vsync_o_d <= VSYNC_O_int;
		end
	end

	wire hcnt_cnt = ~hsync_n_d & hsync_n;

	// Combinational hcnt with reset overrides, registered form follows.
	wire [4:0] hcnt_comb_base = hcnt_cnt ? hcnt_next : hcnt_reg;
	wire hcnt_res0 = ~RESET_N | (hcnt_cnt & hcnt_next[2] & hcnt_next[3] & hcnt_next[4]);
	wire hcnt_res1 = VSYNC_I & ~vsync_d;

	reg [4:0] hcnt_comb;
	always @(*) begin
		hcnt_comb = hcnt_comb_base;
		if (hcnt_res0) hcnt_comb[0]   = 1'b0;
		if (hcnt_res1) hcnt_comb[4:1] = 4'd0;
	end

	wire VSYNC_O_int = hcnt_comb[2] & ~hcnt_comb[3] & ~hcnt_comb[4];
	wire VBLANK_comb = ~(hcnt_comb[2] & hcnt_comb[3] & hcnt_comb[4]);

	always @(posedge clk) begin
		hcnt_reg <= hcnt_comb;
		case (hcnt_comb)
		5'h00: hcnt_next <= 5'h01;
		5'h01: hcnt_next <= 5'h06;
		5'h06: hcnt_next <= 5'h07;
		5'h07: hcnt_next <= 5'h04;
		5'h04: hcnt_next <= 5'h05;
		5'h05: hcnt_next <= 5'h0A;
		5'h0A: hcnt_next <= 5'h0B;
		5'h0B: hcnt_next <= 5'h08;
		5'h08: hcnt_next <= 5'h09;
		5'h09: hcnt_next <= 5'h0E;
		5'h0E: hcnt_next <= 5'h0F;
		5'h0F: hcnt_next <= 5'h0C;
		5'h0C: hcnt_next <= 5'h0D;
		5'h0D: hcnt_next <= 5'h12;
		5'h12: hcnt_next <= 5'h13;
		5'h13: hcnt_next <= 5'h10;
		5'h10: hcnt_next <= 5'h11;
		5'h11: hcnt_next <= 5'h16;
		5'h16: hcnt_next <= 5'h17;
		5'h17: hcnt_next <= 5'h14;
		5'h14: hcnt_next <= 5'h15;
		5'h15: hcnt_next <= 5'h1A;
		5'h1A: hcnt_next <= 5'h1B;
		5'h1B: hcnt_next <= 5'h18;
		5'h18: hcnt_next <= 5'h19;
		5'h19: hcnt_next <= 5'h1E;
		default: ;
		endcase
	end

	// Monitor HSYNC microsequence.
	wire hdelay_res0 = hsync_n | hdelay_reg[3]; // u804
	wire hdelay_res1 = hsync_n;                 // u822

	reg hdelay_res0_d, hdelay2d;
	always @(posedge clk) begin
		hdelay_res0_d <= hdelay_res0;
		hdelay2d      <= hdelay_comb[2];
	end

	reg [3:0] hdelay_comb;
	reg mode_sync_en_w;
	always @(*) begin
		hdelay_comb  = hdelay_reg;
		mode_sync_en_w = ~hdelay_res0_d & hdelay_res0 & hdelay2d;
		if (hdelay_res0) hdelay_comb[2:0] = 3'd0;
		if (hdelay_res1) hdelay_comb[3]   = 1'b0;
	end
	assign MODE_SYNC_EN = mode_sync_en_w;

	always @(posedge clk) begin
		if (hdelay_res0 | hdelay_res1) hdelay_reg <= hdelay_comb;
		else if (CCLK_EN_N) begin
			case (hdelay_comb)
			4'h0: hdelay_reg <= 4'h1;
			4'h1: hdelay_reg <= 4'h6;
			4'h6: hdelay_reg <= 4'h7;
			4'h7: hdelay_reg <= 4'h4;
			4'h4: hdelay_reg <= 4'h5;
			4'h5: hdelay_reg <= 4'h8;
			default: ;
			endcase
		end
	end

	assign HSYNC_O = hdelay_comb[2];
	assign SYNC_N  = ~(VSYNC_O_int ^ HSYNC_O);

	// Interrupt counter: cleared at 52 shaped lines, on the shaped VSYNC
	// edge, by RMR bit 4, or by the acknowledge cycle.
	wire intcnt52 = hcnt_cnt & intcnt_next[2] & intcnt_next[4] & intcnt_next[5];
	wire intcntclr_52_s = intcnt52;
	wire intcntclr_52_r = ~hsync_n;
	reg  intcntclr_52_hold;
	reg  intcntclr_52;
	// Reset-dominant latch: while the CRTC HSYNC is asserted the clear
	// flag is forced low; a 52nd-line event between HSYNC pulses sets it.
	always @(*) begin
		if (intcntclr_52_r)      intcntclr_52 = 1'b0;
		else if (intcntclr_52_s) intcntclr_52 = 1'b1;
		else                     intcntclr_52 = intcntclr_52_hold;
	end
	always @(posedge clk) intcntclr_52_hold <= intcntclr_52;

	wire intcntclr_4 = VSYNC_O_int & ~vsync_o_d; // u817
	wire intcnt_res0 = intcntclr_52 | intcntclr_4 | irq_reset; // u831
	wire intcnt_res1 = intcnt_res0 | irqack_rst;               // u833

	reg [5:0] intcnt_comb;
	always @(*) begin
		intcnt_comb = intcnt_reg;
		if (intcnt_res0) intcnt_comb[4:0] = 5'd0;
		else if (hcnt_cnt) intcnt_comb = intcnt_next;
		if (intcnt_res1) intcnt_comb[5] = 1'b0;
	end

	always @(posedge clk) begin
		intcnt_reg  <= intcnt_comb;
		intcnt_next <= intcnt_comb + 6'd1;
	end

	// Interrupt acknowledge: any INT-sampled I/O or M1 cycle sets
	// irqack_rst until M1 releases (set-dominant latch).
	wire irqack_s = ~(INT_N | IORQ_N | M1_N);
	wire irqack_r = M1_N;
	reg  irqack_hold;
	always @(*) begin
		if (irqack_s)      irqack_rst = 1'b1;
		else if (irqack_r) irqack_rst = 1'b0;
		else               irqack_rst = irqack_hold;
	end
	always @(posedge clk) irqack_hold <= irqack_rst;

	wire int_reset = irq_reset | irqack_rst;

	always @(posedge clk) begin
		reg cnt5;
		cnt5 <= intcnt_comb[5];
		if (int_reset) INT_N <= 1'b1;
		else if (~intcnt_comb[5] & cnt5) INT_N <= 1'b0;
	end

	assign VSYNC_O = VSYNC_O_int;
	assign VBLANK  = VBLANK_comb;

endmodule
