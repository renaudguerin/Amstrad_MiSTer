//============================================================================
//  Amstrad Plus ASIC (AMS40489 "Arnold V") integrated video — CRTC type 3
//  foundation: register file and horizontal character counter.
//
//  This module is the Plus-stream counterpart of the classic rtl/CRTC.v
//  wrapper. Per docs/plus/architecture.md the ASIC's CRTC-3 is a new
//  behavioural implementation, never a third personality inside the classic
//  core. P1 scope (roadmap §5, architecture §4): CRTC3 counters, MA/RA, DE,
//  both sync widths, VSYNC gate, +1µs display alignment; exact readback
//  quirks stay in P5.
//
//  Technical information sourced from the "Amstrad CPC CRTC Compendium" by
//  Longshot (CC BY-NC-ND).
//
//  The counter behaviour of this module follows The Amstrad CPC CRTC
//  Compendium v1.10, Serge Querne (Longshot / Logon System),
//  https://shaker.logonsystem.eu -- licensed CC BY-NC-ND 4.0. Its attribution
//  directive requires this notice in the source of CRTC emulation modules and
//  in the credits of any distributed product built from them. Individual
//  rules cite their ACCC section at the point of implementation.
//
//  Deliberate P1 exclusions (each must land with its own vectors later):
//   - Interlace (R8 bits 1:0 stored but not acted on): the type-3 IVM
//     counting/parity machinery is its own rule set (ACCC §19.5.5, §19.8.4)
//     and no P1 exit criterion needs it.
//   - Light pen R16/R17: no light-pen strobe source is emulated. The
//     registers are stored and readable since P5 (mod-8 map slots 0/1) but
//     hold their reset value (named assumption at the readback section).
//   - The R4=0-at-C0=0-with-Rom-select I/O race (ACCC §12.5 p.101) is a Z80
//     bus-level ASIC race owned by the register-interface layer, not the
//     counter engine.
//
//  P5 scope landed here: the mod-8 read map with stored R14/R15, the
//  R10/R11 status group, and the full readback contract (section at the
//  bottom of this file). The IN-performs-write trap is a property of the
//  upstream bus wiring (the write strobe deliberately does not distinguish
//  read from write cycles); the motherboard pins it.
//============================================================================

module asic_video
(
	input            CLOCK,
	input            CLKEN,      // 1 µs character-clock enable
	input            nRESET,

	// CRTC register bus (type-3 port decode sits upstream of this module)
	input            ENABLE,
	input            nCS,
	input            R_nW,
	input            RS,
	input      [7:0] DI,
	output reg [7:0] DO,

	// Raster timing outputs. DE follows the SKEW-DISPTMG programming
	// (ACCC §19.2, types 0/3/4); MA is the current video pointer VMA and
	// RA the scanline address C9 (which carries the adjustment index on
	// types 3/4, ACCC §11.2.6).
	output reg       HSYNC,
	output reg       VSYNC,
	output           DE,
	output    [13:0] MA,
	output     [4:0] RA,

	// Horizontal counter tap for later raster consumers (PRI/sprites/DMA)
	output     [7:0] HCC,

	// Vertical counter taps: C4 char-row counter, C9 scanline-in-row
	// counter (which doubles as the vertical-adjustment line index on
	// types 3/4, ACCC §11.2.6), and the adjustment-active flag.
	output     [6:0] LINE,
	output     [4:0] ROW,
	output           ADJ,

	// Screen split & soft scroll control registers (P6, from asic_regs)
	input      [7:0] SPLT,
	input     [13:0] SSA,
	input      [7:0] SSCR,

	// ---- Locked-ASIC pixel path (P1 remainder, architecture §4/§7) ----
	//
	// Behavioral locked-ASIC Gate-Array emulation: decodes the video-memory
	// word addressed by MA into pen numbers at the documented mode-dependent
	// rate and translates pens plus border into 4-bit-per-channel RGB
	// through the fixed legacy-colour table. Sources, cited at each point of
	// use below:
	//  - [KT] Kevin Thacker, "Extra CPC Plus Hardware Information",
	//    cpctech.cpc-live.com/docs/cpcplus.html (via web.archive.org capture
	//    20230923001014), Palette section: measured 32-entry hardware-colour
	//    -> R,G,B table (mid level = 6). Cross-checked entry by entry against
	//    this repository's own ga40010 netlist DAC equations during P1
	//    extraction; all 32 agree.
	//  - Byte/pixel structure per video mode: Grimware Gate Array page
	//    (grimware.org/doku.php/documentations/devices/gatearray, §RMR,
	//    "Byte/Pixel structure" table; already cited by rtl/color_mix.sv).
	//    The checked-in netlist corroborates it: the mode-0 leftmost pixel
	//    nibble {b1,b5,b3,b7} is exactly the netlist cidx tap order
	//    {shift_reg[1],[5],[3],[7]} sampled on a freshly loaded register
	//    (rtl/GA40010/video.sv).
	//  - Two video bytes per CRTC character: ga40010 latches VIDEO_BUF twice
	//    per character (vidbuf_clk_en, rtl/GA40010/ga40010.sv) and every CPC
	//    display line spans 80 bytes at R1=40 characters — MA is a word
	//    address. This port models that word directly.
	input               PIXEN,     // 16 MHz dot enable; 16 dots per character
	input      [15:0]   VIDEOD,    // VRAM word for the current MA address:
	                               // {odd byte (second half), even byte}
	input      [1:0]    GAMODE,    // legacy GA screen mode (RMR VM bits)
	input      [4:0]    BORDER_I,  // border hardware colour number
	input      [79:0]   INKR_I,    // 16 x 5-bit ink hardware colours; entry
	                               // k occupies INKR_I[k*5 +: 5]
	output reg [3:0]    RGB_R,     // 4-bit levels per channel ([KT]: 0/6/15)
	output reg [3:0]    RGB_G,
	output reg [3:0]    RGB_B,
	output reg [4:0]    PEN,       // observability for later raster consumers:
	                               // {showing_border, decoded ink nibble}

	// Sprite-plane interface (P4). HWRAP strobes the line seam (CLKEN
	// edge where the character counter sits on R0) for the sprite
	// engine's horizontal scale; SPR_* is the composited sprite pixel,
	// shown between the screen and the border (asic-reference §5
	// priority: border > sprites > screen), under HSYNC force-blank.
	output           HWRAP,
	input            SPR_EN,
	input     [11:0] SPR_RGB,

	// ---- ASIC 12-bit palette port (asic-reference §6) ----
	//
	// On a Plus the pen/border lookup is the 32-entry 12-bit palette held
	// in asic_regs, not the fixed 27-colour Gate-Array ROM: PAL_ADDR names
	// the entry this dot needs (pen 0-15 inside DE, entry 16 for the
	// border) and PAL_RGB returns it as {G,R,B} nibbles. asic_regs
	// registers that read, so PAL_RGB lags PAL_ADDR by one CLOCK — which
	// still lands a dot early relative to the RGB register here, because
	// PIXEN is one clock in four.
	//
	// PAL_EN low keeps the internal [KT] legacy-colour ROM below. That is
	// the standalone-bench path (sim/plus/asic_video_test.cpp and
	// p1_video_test_top.v elaborate this module with no asic_regs beside
	// it); the production motherboard ties it high. Legacy PENR/INKR
	// writes still reach the screen with PAL_EN high because asic_regs
	// shadows them into palette entries 0-16.
	input            PAL_EN,
	output     [4:0] PAL_ADDR,
	input     [11:0] PAL_RGB
);

/* verilator lint_off WIDTH */

//----------------------------------------------------------------------
// Register file
//
// Writes decode the full five-bit index (ACCC §28.1.9 notes reads are
// mod-8 on types 3/4 while writes reach the selected storage register;
// software stores into R14/R15 and reads them back through slots 6/7).
// Select indices 10/11 (ASIC status, read-only), 16/17 (light pen) have
// no storage here (§21.2.3 table, p.246).
//----------------------------------------------------------------------

reg  [4:0] addr;

/* verilator lint_off UNUSEDSIGNAL */
reg [7:0] R0_h_total;
reg [7:0] R1_h_displayed;
// R1/R2/R3 gain consumers with the display and sync phases; this scaffold
// pragma block shrinks as each phase lands.
/* verilator lint_off UNUSEDSIGNAL */
reg [7:0] R2_h_sync_pos;
reg [3:0] R3_h_sync_width;
reg [3:0] R3_v_sync_width;
reg [6:0] R6_v_displayed;
reg [6:0] R7_v_sync_pos;
reg [1:0] R8_skew;         // bits 5:4, SKEW-DISPTMG (types 0/3/4, §19.1)
/* verilator lint_on UNUSEDSIGNAL */
// R8 bits 1:0 select interlace modes and stay stored-but-inert for the
// whole P1 foundation: the type-3 IVM counting/parity machinery (ACCC
// §19.5.5, §19.8.4) is deliberately out of P1 scope (see header). The
// register keeps its storage so a later phase adds behaviour without a
// bus-contract change.
/* verilator lint_off UNUSEDSIGNAL */
reg [1:0] R8_interlace;
/* verilator lint_on UNUSEDSIGNAL */
reg [6:0] R4_v_total;
reg [4:0] R5_v_total_adj;
reg [4:0] R9_v_max_line;
// Full 8-bit storage (P5): bits 7:6 are the §20.5 (p.244) extended
// start-address bits ("These 2 bits represent bits 14 and 15 of the video
// pointer"). The 14-bit VMA reload below consumes R12[5:0] only; the
// >16K carry mechanism those bits take part in is not modelled, but the
// bits are stored and readable (§21.2.3 slot 4 returns the full register).
reg [7:0] R12_start_addr_h;
reg [7:0] R13_start_addr_l;
// Cursor address (§21.2.3 slots 6/7): stored and readable on types 3/4 —
// "it is perfectly possible to store values in these registers and then
// read them back" (§21.2.3 note). R14 is 6-bit (bits 7:6 read 0); R15
// full 8-bit. The cursor itself is not managed on the CPC (no CUDISP
// consumer); only the storage exists. ACCC v1.10 supersedes [KT]'s
// earlier "always return 0" observation for these slots.
reg [5:0] R14_cursor_h;
reg [7:0] R15_cursor_l;
// Light pen (§21.2.3 slots 0/1): readable pointer registers with no
// CPC-side strobe source. They hold their reset value forever here
// (named assumption; see the readback section).
reg [5:0] R16_pen_h;
reg [7:0] R17_pen_l;
/* verilator lint_on UNUSEDSIGNAL */

always @(posedge CLOCK) begin
	if (!nRESET) begin
		addr             <= 5'd0;
		R0_h_total       <= 8'd0;
		R1_h_displayed   <= 8'd0;
		R2_h_sync_pos    <= 8'd0;
		{R3_v_sync_width, R3_h_sync_width} <= 8'd0;
		R4_v_total       <= 7'd0;
		R5_v_total_adj   <= 5'd0;
		R6_v_displayed   <= 7'd0;
		R7_v_sync_pos    <= 7'd0;
		{R8_skew, R8_interlace} <= 4'd0;
		R9_v_max_line    <= 5'd0;
		R12_start_addr_h <= 8'd0;
		R13_start_addr_l <= 8'd0;
		R14_cursor_h     <= 6'd0;
		R15_cursor_l     <= 8'd0;
		R16_pen_h        <= 6'd0;
		R17_pen_l        <= 8'd0;
	end
	else if (ENABLE & ~nCS & ~R_nW) begin
		if (~RS) begin
			addr <= DI[4:0];
		end
		else begin
			case (addr)
			// Writes keep the full five-bit decode (§21.2.3 truncates
			// READS to three bits only): aliased selects 4/20 must
			// program R4/R12 distinctly, exactly as software assumes.
			5'd00: R0_h_total     <= DI;
			5'd01: R1_h_displayed <= DI;
			5'd02: R2_h_sync_pos  <= DI;
			5'd03: {R3_v_sync_width, R3_h_sync_width} <= DI;
			5'd04: R4_v_total     <= DI[6:0];
			5'd05: R5_v_total_adj <= DI[4:0];
			5'd06: R6_v_displayed <= DI[6:0];
			5'd07: R7_v_sync_pos  <= DI[6:0];
			5'd08: {R8_skew, R8_interlace} <= {DI[5:4], DI[1:0]};
			5'd09: R9_v_max_line  <= DI[4:0];
			5'd12: R12_start_addr_h <= DI;
			5'd13: R13_start_addr_l <= DI;
			5'd14: R14_cursor_h   <= DI[5:0];
			5'd15: R15_cursor_l   <= DI;
			default: ;  // 10/11 status group is read-only (§21.2.3);
			            // 16/17 light pen has no strobe source (header)
			endcase
		end
	end
end

//----------------------------------------------------------------------
// Horizontal character counter C0 ("HCC")
//
// C0 counts 0..R0 inclusive and wraps via equality against the live R0
// value (ACCC §13.1, p.102). Type 3 accepts any R0 without disturbing
// other counters — there is no type-0-style freeze machinery (§13.5,
// p.121). The exact post-write behavior when R0 is lowered below the
// current C0 is not given as a direct CRTC3 chronogram in the source; the
// equality-based eight-bit overflow below is therefore an explicitly
// unverified P1 model assumption, protected by t01e pending direct CRTC3
// rule/chronogram, Logon, or hardware evidence. ACCC §28.1.1 is not support
// for this behavior: it describes C4/C9 identification overflow, not C0.
//----------------------------------------------------------------------

reg [7:0] hcc;

wire        hcc_last = (hcc == R0_h_total);
wire [7:0]  hcc_next = hcc_last ? 8'h00 : hcc + 8'd1;

always @(posedge CLOCK) begin
	if (!nRESET) hcc <= 8'h00;
	else if (CLKEN) hcc <= hcc_next;
end

//----------------------------------------------------------------------
// Vertical counters: C9 (raster line within the character row) and C4
// (character row within the frame), plus the vertical-adjustment state.
//
// All decisions sample the live register values at the line-end edge
// (C0==R0); a bus write landing mid-line is therefore considered at that
// line's end, and a write landing exactly on the decision edge itself is
// seen by the *next* decision (the documented same-edge hazard window;
// vectors stay inside the documented mid-line windows).
//
// C9/R9 completion uses ">=" not equality: on types 3/4 an R9 lowered
// below the current C9 forces next-C9=0 with normal row accounting,
// "it is impossible to overflow C9" (ACCC §10.3.4 p.77). Equality alone
// is the natural case; the greater-than term is the forced reset.
//
// Entering adjustment from the last character row does NOT increment C4:
// it stays equal to R4 for every adjustment line (ACCC §11.2.6 p.84),
// and R6==R4 consequently covers those lines too (§18.2.4 note).
// Adjustment ends when the next adjustment-line index C9+1 has reached
// R5 — or would pass it after a mid-adjustment R5 shrink; both end the
// management on the same line ("Whether with R5 or R9, it is impossible
// to overflow C9", ACCC §11.3.3 p.86).
//
// An R4 lowered below the current C4 makes the frame-end equality
// unreachable: C4 free-runs through its seven-bit range (ACCC §12.5
// p.101, "overflow of the C4 counter"), in contrast to C9 above.
//----------------------------------------------------------------------

reg  [6:0] charline;
reg  [4:0] raster;
reg        in_adj;

wire       c9_done       = in_adj ? 1'b0 : (raster >= R9_v_max_line);
wire       last_charline = (charline == R4_v_total);
wire       enter_adj     = c9_done & last_charline &
                           (R5_v_total_adj != 5'd0);
wire       frame_wrap    = c9_done & last_charline &
                           (R5_v_total_adj == 5'd0);
wire       adj_end_n     = ((raster + 5'd1) >= R5_v_total_adj);

// Next-state values at the line-end edge, shared with the video-pointer
// and display-enable logic so every consumer sees one consistent seam.
// Adjustment end restarts the frame: C4=C9=0 (ACCC §11.3 general).
wire [4:0] raster_n      = in_adj ? (adj_end_n ? 5'd0 : raster + 5'd1) :
                           c9_done ? 5'd0 : raster + 5'd1;
wire [6:0] charline_n    = (in_adj & adj_end_n) ? 7'd0 :
                           in_adj ? charline :
                           enter_adj ? charline :
                           frame_wrap ? 7'd0 :
                           c9_done ? charline + 7'd1 : charline;
wire       adj_n         = in_adj ? !adj_end_n : enter_adj;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		charline <= 7'd0;
		raster   <= 5'd0;
		in_adj   <= 1'b0;
	end
	else if (CLKEN) begin
		if (hcc_last) begin
			in_adj   <= adj_n;
			raster   <= raster_n;
			charline <= charline_n;
		end
	end
end

//----------------------------------------------------------------------
// Video pointer: VMA (current) and VMA' (row-start latch).
//
// Two-stage {R12,R13} -> VMA' -> VMA behaviour, as on type 0. The ACCC
// v1.11 §20.3.4 p.243 opening-sentence/current-pointer-model reading (the
// later prose drops C9; hardware confirmation remains open) is used here:
// at the frame origin C4=C9=C0=0 BOTH pointers reload from R12/R13. At any
// other line start VMA loads from VMA'. The row-end capture VMA' <- VMA fires
// on the live comparison
// C0==R1 && C9==R9 and is suppressed during vertical adjustment, which
// instead re-solidifies the captured row base each adjustment line
// ("without updating the video pointer", ACCC §11.2.6 p.84).
//
// With R1>R0 the capture can never fire, so every row re-displays the
// frozen VMA' base — character-line repetition (ACCC §17.2 p.179) with
// no spurious border substitution on types 3/4 (§17.6.2/§19.2.4).
//----------------------------------------------------------------------

reg [13:0] vma;
reg [13:0] vma_latch;

// Selected §20.3.4 frame-origin reading; see the source-conflict note above.
wire pointer_frame_origin = !adj_n && (charline_n == 7'd0) &&
                            (raster_n == 5'd0);

// P6: Soft scroll vertical scanline offset (SSCR[6:4], asic-reference §8)
wire [4:0] ra_eff = {raster[4:3], (raster[2:0] + SSCR[6:4]) & 3'd7};

// Row-end VMA latch update: when the displayed scanline ra_eff reaches R9,
// capture VMA into vma_latch at HCC == R1 so MA advances before the wrapped
// RA 0 line under SSCR vertical scroll (Arnold V §2.5, asic-reference §8).
wire row_latch_event = CLKEN && !in_adj &&
                       (hcc == R1_h_displayed) &&
                       (ra_eff == R9_v_max_line);

// P6: Screen split comparison ({SPLT7..0} == {VC4..0, RC2..0}, asic-reference §8).
// When matched and SPLT != 0, capture SSA into vma_latch at HCC == R1.
wire split_match = (SPLT != 8'd0) && ({charline[4:0], raster[2:0]} == SPLT);
wire split_latch_event = CLKEN && !in_adj && split_match && (hcc == R1_h_displayed);

always @(posedge CLOCK) begin
	if (!nRESET) begin
		vma       <= 14'd0;
		vma_latch <= 14'd0;
	end
	else if (CLKEN) begin
		if (split_latch_event)
			vma_latch <= SSA;
		else if (row_latch_event)
			vma_latch <= vma;

		if (hcc_last) begin
			// §20.3.4 frame-start reload has highest priority. Otherwise
			// a simultaneous C0=R1=R0 row-end capture supplies the next
			// row base, so do not overwrite VMA with the stale latch value
			// on that same edge (ACCC §17.1 p.176 / §17.6.1 p.185).
			if (pointer_frame_origin) begin
				vma       <= {R12_start_addr_h[5:0], R13_start_addr_l};
				vma_latch <= {R12_start_addr_h[5:0], R13_start_addr_l};
			end
			else if (split_latch_event) begin
				vma <= SSA;
			end
			else if (!row_latch_event) begin
				vma <= vma_latch;
			end
		end
		else if (!hcc_last) begin
			// VMA increments on every character cell processed,
			// displayed or border (ACCC §17.1 p.176).
			vma <= vma + 14'd1;
		end
	end
end

//----------------------------------------------------------------------
// Display enable.
//
// Horizontal: DISPTMG on at C0=0, off from C0=R1 (ACCC §17.1 p.175).
// The registered clear makes an R1==R0 line show exactly one border
// character at C0=R0 (the interline blip, §17.6.1), while R1>R0 keeps
// the whole line displayed because the equality never fires (types 3/4
// substitute nothing, §17.6.2). R1=0 clears every line start: no
// characters are ever displayed (§17.1/§18.3.1).
//
// Vertical: the R6 test runs ONLY at the beginning of a line on types
// 3/4 — mid-line R6 updates are not considered until the next line
// start ("The update of R6 during the line is therefore not considered",
// ACCC §18.2.4 p.189). There is no R6==0 special case (§18.3.4). Because
// C4 stays ==R4 through adjustment, R6==R4 borders those lines too
// (§18.2.4 note).
//
// SKEW-DISPTMG (R8 bits 5:4; types 0/3/4, §19.2): 00 immediate, 01/10
// delay both border edges by one/two characters, 11 BORDER ON (output
// suppressed). Only the visible edges shift; pointer bookkeeping is
// unaffected (§19.2.3 p.194).
//----------------------------------------------------------------------

reg       hde;
reg       vde;
reg [1:0] dde;

wire      disp_raw = hde & vde;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		hde <= 1'b0;
		vde <= 1'b0;
		dde <= 2'b00;
	end
	else if (CLKEN) begin
		if (hcc_last) begin
			hde <= (R1_h_displayed != 8'd0);
			vde <= (charline_n < R6_v_displayed);
		end
		else if ((hcc + 8'd1) == R1_h_displayed) begin
			hde <= 1'b0;
		end
		dde <= {dde[0], disp_raw};
	end
end

//----------------------------------------------------------------------
// HSYNC (ACCC §14/§15).
//
// HSYNC starts when C0 reaches R2 and lasts R3l characters. Types 3/4
// generate a 16-character HSYNC when R3l=0 ("it is impossible not to
// generate HSYNC", §14.5 p.141). The width counter is a free-running
// nibble compared against the LIVE R3l: a mid-HSYNC rewrite below the
// already-counted value wraps the full nibble before the new equality
// can hit, so an interrupted HSYNC never ends early (§14.4 general rule;
// compendium-02 §4). There is no R3.JIT on types 3/4 (§14.4).
//
// The start comparison keys on the ENTERING edge (hcc_next == R2) so the
// registered HSYNC is visible during character R2 itself, matching the
// §15.2.2 chronograms where C3l=0 sits under C0=R2 — the same
// registered-output convention the DISPTMG logic uses for C0=R1.
//
// Type-1..4 re-entrancy bug (§15.3.1 p.148): when C0 reaches R2 on the
// same edge that C3 reaches R3l, the active pulse does not end. C3 keeps
// counting through its wrapped nibble until the next equality. This is a
// live collision, not a static register relation: §15.3.5 p.151 shows a
// CRTC3 R2 rewrite from 11 to 21 creating the collision at the natural
// end of an already-active pulse. The R0=0, R2=0, R3l=1 extreme makes
// every end edge collide and therefore produces infinite HSYNC
// (§15.3.2). ACCC §14.5 establishes that R3l=0 produces a 16-character
// pulse, but does not say whether the §15.3 collision applies when that
// pulse's natural end meets a live start. This model assumes it does not,
// leaving R3l=0 bounded; that is an unverified P1 model assumption, pinned
// by t04i pending a direct rule, Logon observation, or hardware capture.
//----------------------------------------------------------------------

reg       in_hsync;
reg [3:0] hsc;
reg       in_vsync;
reg [3:0] vsc;

wire       hsync_start               = (hcc_next == R2_h_sync_pos);
wire [3:0] hsc_next                  = hsc + 4'd1;
wire       hsync_end_hit             = (hsc_next == R3_h_sync_width);
// See the unverified R3l=0 boundary assumption above: collision extension
// is enabled only for explicit nonzero widths.
wire       hsync_end_start_collision = (R3_h_sync_width != 4'd0) &&
                                        hsync_start;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		HSYNC    <= 1'b0;
		in_hsync <= 1'b0;
		hsc      <= 4'd0;
	end
	else if (CLKEN) begin
		if (in_hsync) begin
			hsc <= hsc_next;
			if (hsync_end_hit && !hsync_end_start_collision) begin
				in_hsync <= 1'b0;
				HSYNC    <= 1'b0;
			end
			// else: bug path — keep the pulse asserted and let hsc roll on
		end
		else if (hsync_start) begin
			in_hsync <= 1'b1;
			HSYNC    <= 1'b1;
			hsc      <= 4'd0;
		end
	end
end

//----------------------------------------------------------------------
// VSYNC (ACCC §16.4.4 p.170): starts only at line starts where
// C4==R7 AND C9==0 AND C0==0 hold simultaneously; rewriting R7 to the
// current C4 while C0>0 does not trigger. There is no re-entrancy
// protection — a condition that persists across a finished pulse
// restarts it immediately. Width is R3h lines with 0 meaning 16 (§14.2),
// counted per line with the same live-equality nibble semantics; dynamic
// R3h rewrites follow the CRTCs-0/3/4 rule (compendium-02 §2). Interlace
// MID-VSYNC delays are out of P1 scope (header note); the ASIC needs >=3
// active lines to emit monitor C-VSYNC, which belongs to the integrated
// pipeline phase.
//----------------------------------------------------------------------

wire        vsync_fire      = hcc_last &&
                              (charline_n == R7_v_sync_pos) &&
                              (raster_n == 5'd0);
wire [3:0]  vsc_next        = vsc + 4'd1;
wire        vsync_width_hit = (vsc_next == R3_v_sync_width);

// The width counts LINE boundaries while the pulse is active (R3h is a
// line count), and a condition that persists across the expiring edge
// renews the pulse on that same edge — there is no re-entrancy
// protection, so with R7=R4=R9=0 the pin never visibly drops (§16.4.4).
always @(posedge CLOCK) begin
	if (!nRESET) begin
		VSYNC    <= 1'b0;
		in_vsync <= 1'b0;
		vsc      <= 4'd0;
	end
	else if (CLKEN) begin
		if (in_vsync) begin
			if (hcc_last) begin
				vsc <= vsc_next;
				if (vsync_width_hit) begin
					if (vsync_fire) begin
						vsc <= 4'd0;   // renewed without a gap
					end
					else begin
						in_vsync <= 1'b0;
						VSYNC    <= 1'b0;
					end
				end
			end
		end
		else if (vsync_fire) begin
			in_vsync <= 1'b1;
			VSYNC    <= 1'b1;
			vsc      <= 4'd0;
		end
	end
end

assign DE = (R8_skew == 2'b00) ? disp_raw :
            (R8_skew == 2'b01) ? dde[0] :
            (R8_skew == 2'b10) ? dde[1] : 1'b0;

assign HCC  = hcc;
assign LINE = charline;
assign ROW  = raster;
assign ADJ  = in_adj;
assign MA   = vma;
// P6: Soft scroll vertical scanline offset (SSCR[6:4], asic-reference §8)
assign RA   = ra_eff;

//----------------------------------------------------------------------
// P5: register readback, status groups (ACCC §21.2.3 p.246, §21.3.4
// pp.248-249).
//
// Reads decode ONLY the three least significant bits of the selected
// register number through the fixed map
//   {R16, R17, STATUS1, STATUS2, R12, R13, R14, R15}
// ("Reading R4 therefore also means reading register 12", §21.2.3;
// §21.3.4's note that "bit 3 of the register number is forced to 1"
// describes the same truncation for slots 2/3). Writes keep the full
// five-bit decode. The two documented read ports (&BE00/&BF00) differ
// only in RS (A8); type 3 ignores RS while reading, so both return the
// selected register (§21.2.3). DO is HIGH-NEUTRAL (wired-AND) whenever
// this module does not answer, the same open-bus convention as asic_regs.
//
// [KT] (cpctech cpcplus.html, CRTC section) first published this map
// with slots 6/7 returning 0; ACCC v1.10 §21.2.3 supersedes that with
// stored, writable R14/R15. R12 reads return all eight stored bits:
// bits 7:6 are the §20.5 (p.244) extended start-address bits, kept for
// readback while the VMA reload consumes R12[5:0] only.
//
// STATUS 1 (slot 2) and STATUS 2 (slot 3) are live combinational levels
// of the counter/pointer state — several bits hold for one character
// only (§21.3.4 "requires great precision"). The HSYNC/VSYNC boundary
// boundaries follow the literal §21.3.4 comparators. In particular STATUS1
// bit 4 is low at C0=R2+R3l; unlike HSYNC pulse generation, that comparator
// does not substitute 16 for R3l=0. For bit 5, [KT]'s final-VSYNC-line rule
// supplies the R3h=0 sixteenth-line case that ACCC leaves unspecified.
//
// Named assumptions (each would need its own vector against new
// evidence):
//  - R16/R17 hold their reset value: no light-pen strobe source is emulated.
//  - Status 2 bit 3 resets to 0 and toggles at every 16th frame origin
//    (§21.3.4.2 "Timer 16 CRTC frames"; §28.1.10 p.293 notes this bit
//    differs between CRTC 3 and CRTC 4 — type 3 only here).
//  - Writes to selects 16+ are ignored (ACCC documents read truncation
//    only; aliased-select write behaviour is not evidenced).
//----------------------------------------------------------------------

// STATUS 1 (§21.3.4.1 p.248): horizontal-event group.
wire s1_bit0 = hcc_last;                                    // 1 at C0=R0
wire s1_bit1 = ~(hcc == {1'b0, R0_h_total[7:1]});           // 0 at C0=R0/2
wire s1_bit2 = ~((R0_h_total >= R1_h_displayed) &&
                 (hcc == (R1_h_displayed - 8'd1)));         // 0 at C0=R1-1
wire s1_bit3 = ~(hcc == R2_h_sync_pos);                     // 0 at C0=R2
wire [8:0] status1_hsync_end = {1'b0, R2_h_sync_pos} +
                               {5'd0, R3_h_sync_width};
wire s1_bit4 = ~(hcc == status1_hsync_end);                 // 0 at C0=R2+R3l
wire s1_bit5 = ~(in_vsync &&
                 (vsc == (R3_v_sync_width - 4'd1)));        // 0 on last VSYNC line
// C0=0..R0-1 with VMA LSB 0xFF, or C0=R0 with VMA' LSB 0x00: the next
// character resets the video-pointer low byte (§21.3.4.1 prose).
wire s1_bit7 = ~(((hcc != R0_h_total) && (vma[7:0] == 8'hFF)) ||
                 ((hcc == R0_h_total) && (vma_latch[7:0] == 8'h00)));

// STATUS 2 (§21.3.4.2 p.249): vertical-event group. ACCC specifies literal
// counter comparisons, so these remain live during vertical adjustment too;
// whether hardware suppresses an adjustment-time duplicate is not evidenced.
wire s2_bit0 = ~((charline == R4_v_total) &&
                 (raster == R9_v_max_line) && hcc_last);
wire s2_bit1 = ~((charline == (R6_v_displayed - 7'd1)) &&
                 (raster == R9_v_max_line) && hcc_last);
wire s2_bit2 = ~((charline == (R7_v_sync_pos - 7'd1)) &&
                 (raster == R9_v_max_line) && hcc_last);
wire s2_bit3 = frame16_toggle;
wire s2_bit5 = ~(raster == R9_v_max_line);
wire s2_bit7 = ((raster == R9_v_max_line) && hcc_last) ||
               ((raster == 5'd0) && (hcc != R0_h_total));

// Frame-origin strobe: the same edge the video pointers reload from
// R12/R13 (C4=C9=C0, §20.3.4 p.243).
reg  [3:0] frame16_cnt;
reg        frame16_toggle;
wire frame_origin = CLKEN && hcc_last && pointer_frame_origin;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		frame16_cnt    <= 4'd0;
		frame16_toggle <= 1'b0;
	end
	else if (frame_origin) begin
		if (frame16_cnt == 4'd15) begin
			frame16_cnt    <= 4'd0;
			frame16_toggle <= ~frame16_toggle;
		end
		else begin
			frame16_cnt <= frame16_cnt + 4'd1;
		end
	end
end

always @(*) begin
	DO = 8'hFF;
	if (ENABLE & ~nCS & R_nW) begin
		case (addr[2:0])
			3'd0: DO = {2'b00, R16_pen_h};
			3'd1: DO = R17_pen_l;
			3'd2: DO = {s1_bit7, 1'b1, s1_bit5, s1_bit4,
			            s1_bit3, s1_bit2, s1_bit1, s1_bit0};
			3'd3: DO = {s2_bit7, 1'b0, s2_bit5, 1'b1,
			            s2_bit3, s2_bit2, s2_bit1, s2_bit0};
			3'd4: DO = R12_start_addr_h;
			3'd5: DO = R13_start_addr_l;
			3'd6: DO = {2'b00, R14_cursor_h};
			3'd7: DO = R15_cursor_l;
		endcase
	end
end

//----------------------------------------------------------------------
// Locked-ASIC pixel path (P1 remainder).
//
// Contract: one video word per character arrives on VIDEOD; the even byte
// is displayed during dots 0-7 and the odd byte during dots 8-15. Within
// each byte half the documented nibble/bit groups are emitted leftmost
// first at the mode-dependent rate:
//   mode 0 (%00): 2 px/half, 4 dots each, pen = {b1,b5,b3,b7} then
//                 {b0,b4,b2,b6}          (Grimware "Byte/Pixel structure";
//                 == netlist cidx taps {r1,r5,r3,r7} / post-shift state)
//   mode 1 (%01): 4 px/half, 2 dots each, pens {b3,b7} {b2,b6} {b1,b5}
//                 {b0,b4}                (== netlist taps {r3,r7} family)
//   mode 2 (%10): 8 px/half, 1 dot each,  bit b7 first
//                 (== netlist tap r7)
//   mode 3 (%11): 2 px/half, 4 dots each, pens {b3,b7} then {b2,b6}
//                 (Grimware documents this layout for the undocumented
//                 VM=3; the netlist decodes it through the mode-1 mux)
// Pens select INKR_I entries (mode 2 uses only inks 0/1); outside DE the
// border colour is substituted; RGB is forced to black while HSYNC is
// active — the netlist FORCE_BLANK analogue (video.sv FORCE_BLANK term).
// The vertical-blank region blanking of ga40010 (HCNTLT28) needs the
// monitor-side timing that lands with motherboard integration.
//
// P1/Pre-P6 pixel pipeline contract (validated by p1_video and motherboard benches):
// the first pixel of a character's even byte is presented on dot 0 and
// RGB is registered once per dot (one-dot presentation latency). The
// registered output and de_hold capture reproduce the documented Plus GA
// emulation pipeline latency (INKR effects at ~1/4-1/2 character per [KT]
// and Grimware timings).
//----------------------------------------------------------------------

reg  [3:0] pix_cnt;    // dot index within the character (0..15)
reg  [7:0] vid_even;   // byte displayed during dots 0-7
reg  [7:0] vid_odd;    // byte displayed during dots 8-15
reg        de_hold;    // DE captured at the character edge (post-skew)
reg  [1:0] mode_q;     // GAMODE latched on HSYNC assertion
reg        hsync_d;

always @(posedge CLOCK) begin
	if (!nRESET) pix_cnt <= 4'd0;
	else if (PIXEN) pix_cnt <= CLKEN ? 4'd0 : (pix_cnt + 4'd1);
end

// Byte latches mirror ga40010's twice-per-character VIDEO_BUF capture.
always @(posedge CLOCK) begin
	if (!nRESET) begin
		vid_even <= 8'd0;
		vid_odd  <= 8'd0;
		de_hold  <= 1'b0;
	end
	// Each half must be latched on the edge BEFORE the first dot that
	// displays it: the even byte at the CLKEN edge that resets pix_cnt to
	// 0, the odd byte at the edge that ends dot 7. Latching the odd byte
	// at pix_cnt==8 instead leaves dot 8 showing the previous character's
	// odd byte, which is invisible whenever VIDEOD is held constant (t05h).
	else if (PIXEN) begin
		if (CLKEN) begin
			vid_even <= VIDEOD[7:0];
			de_hold  <= DE;
		end
		else if (pix_cnt == 4'd7) begin
			vid_odd <= VIDEOD[15:8];
		end
	end
end

// Grimware §RMR: "VM ... will take effect after the next HSync". The
// netlist re-times mode identically (MODE_SYNC = ~HSYNC_O, ga40010.sv).
//
// This model latches on the HSYNC rising edge, so it does not depend on
// the pulse having any particular width. That deliberately does NOT model
// the real GA's documented requirement for an HSYNC of at least 2 us
// before the byte=>pixel decoder updates. A shorter type-3 pulse is
// reachable here — R3l=1 statically, or an R3l rewrite during the pulse:
// ACCC §14.5 p.141 bounds an R3l=0 HSYNC to 16 characters only "unless it
// is interrupted by modifying R3 during HSYNC" — and on hardware such a
// pulse would leave the screen mode unchanged. Deferred with the rest of
// the GA pixel-phase contract to motherboard integration (architecture
// §5 Risk 1).
always @(posedge CLOCK) begin
	if (!nRESET) begin
		mode_q  <= 2'b00;
		hsync_d <= 1'b0;
	end
	else if (PIXEN) begin
		hsync_d <= HSYNC;
		if (HSYNC && !hsync_d) mode_q <= GAMODE;
	end
end

wire [7:0] vbyte = pix_cnt[3] ? vid_odd : vid_even;
wire [2:0] s     = pix_cnt[2:0];

reg [3:0] pen_nib;
always @(*) begin
	case (mode_q)
		// Mode 0: leftmost pixel takes the odd bits, second the even bits
		// (Grimware bit row A0 B0 A2 B2 A1 B1 A3 B3 => A={A3,A2,A1,A0}).
		2'b00: pen_nib = s[2] ? {vbyte[0], vbyte[4], vbyte[2], vbyte[6]}
		                      : {vbyte[1], vbyte[5], vbyte[3], vbyte[7]};
		// Mode 1: pairs {b3,b7} {b2,b6} {b1,b5} {b0,b4}, two dots per pen;
		// pixel p takes high bit b(3-p) and low bit b(7-p).
		2'b01: pen_nib = {vbyte[{1'b0, ~s[2], ~s[1]}],
		                  vbyte[{1'b1, ~s[2], ~s[1]}]};
		// Mode 2: sequential bits, MSB first, one dot per pen.
		2'b10: pen_nib = {3'b000, vbyte[~s]};
		// Mode 3 (Grimware): {b3,b7} then {b2,b6}, four dots per pen.
		2'b11: pen_nib = s[2] ? {2'b00, vbyte[2], vbyte[6]}
		                      : {2'b00, vbyte[3], vbyte[7]};
	endcase
end

// [KT] measured legacy-colour table (Palette section): hardware colour
// number -> {R,G,B} nibbles, mid level = 6. Verified entry-by-entry
// against the ga40010 netlist DAC equations during P1 extraction.
function [11:0] legacy_colour(input [4:0] hw);
	begin
		case (hw)
			5'd00: legacy_colour = {4'd6,  4'd6,  4'd6 };
			5'd01: legacy_colour = {4'd6,  4'd6,  4'd6 };
			5'd02: legacy_colour = {4'd0,  4'd15, 4'd6 };
			5'd03: legacy_colour = {4'd15, 4'd15, 4'd6 };
			5'd04: legacy_colour = {4'd0,  4'd0,  4'd6 };
			5'd05: legacy_colour = {4'd15, 4'd0,  4'd6 };
			5'd06: legacy_colour = {4'd0,  4'd6,  4'd6 };
			5'd07: legacy_colour = {4'd15, 4'd6,  4'd6 };
			5'd08: legacy_colour = {4'd15, 4'd0,  4'd6 };
			5'd09: legacy_colour = {4'd15, 4'd15, 4'd6 };
			5'd10: legacy_colour = {4'd15, 4'd15, 4'd0 };
			5'd11: legacy_colour = {4'd15, 4'd15, 4'd15};
			5'd12: legacy_colour = {4'd15, 4'd0,  4'd0 };
			5'd13: legacy_colour = {4'd15, 4'd0,  4'd15};
			5'd14: legacy_colour = {4'd15, 4'd6,  4'd0 };
			5'd15: legacy_colour = {4'd15, 4'd6,  4'd15};
			5'd16: legacy_colour = {4'd0,  4'd0,  4'd6 };
			5'd17: legacy_colour = {4'd0,  4'd15, 4'd6 };
			5'd18: legacy_colour = {4'd0,  4'd15, 4'd0 };
			5'd19: legacy_colour = {4'd0,  4'd15, 4'd15};
			5'd20: legacy_colour = {4'd0,  4'd0,  4'd0 };
			5'd21: legacy_colour = {4'd0,  4'd0,  4'd15};
			5'd22: legacy_colour = {4'd0,  4'd6,  4'd0 };
			5'd23: legacy_colour = {4'd0,  4'd6,  4'd15};
			5'd24: legacy_colour = {4'd6,  4'd0,  4'd6 };
			5'd25: legacy_colour = {4'd6,  4'd15, 4'd6 };
			5'd26: legacy_colour = {4'd6,  4'd15, 4'd0 };
			5'd27: legacy_colour = {4'd6,  4'd15, 4'd15};
			5'd28: legacy_colour = {4'd6,  4'd0,  4'd0 };
			5'd29: legacy_colour = {4'd6,  4'd0,  4'd15};
			5'd30: legacy_colour = {4'd6,  4'd6,  4'd0 };
			5'd31: legacy_colour = {4'd6,  4'd6,  4'd15};
			default: legacy_colour = 12'h000;
		endcase
	end
endfunction

// P6: Soft scroll horizontal delay line (SSCR[3:0]) and border mask (SSCR[7]).
reg [3:0] pen_delay [0:14];
integer p_idx;
always @(posedge CLOCK) begin
	if (!nRESET) begin
		for (p_idx = 0; p_idx < 15; p_idx = p_idx + 1)
			pen_delay[p_idx] <= 4'd0;
	end
	else if (PIXEN) begin
		pen_delay[0] <= pen_nib;
		for (p_idx = 1; p_idx < 15; p_idx = p_idx + 1)
			pen_delay[p_idx] <= pen_delay[p_idx - 1];
	end
end

wire [3:0] pen_delayed = (SSCR[3:0] == 4'd0) ? pen_nib : pen_delay[SSCR[3:0] - 4'd1];

// Tracks the first character of active display on each line for SSCR[7] border masking.
reg de_first_char;
always @(posedge CLOCK) begin
	if (!nRESET) begin
		de_first_char <= 1'b0;
	end
	else if (PIXEN && CLKEN) begin
		de_first_char <= (!de_hold && DE) || (hcc_last && DE);
	end
end

wire eff_de   = de_hold & ~(SSCR[7] & de_first_char);
wire [4:0] hw_sel  = eff_de ? INKR_I[pen_delayed*5 +: 5] : BORDER_I;

// Palette entry for this dot: the decoded pen inside active display, entry
// 16 (border) outside it (asic-reference §6).
assign PAL_ADDR = eff_de ? {1'b0, pen_delayed} : 5'd16;

// asic_regs stores the word as {G,R,B}; this pipeline carries {R,G,B}.
wire [11:0] asic_rgb = {PAL_RGB[7:4], PAL_RGB[11:8], PAL_RGB[3:0]};

wire [11:0] rgb_mux = PAL_EN ? asic_rgb : legacy_colour(hw_sel);
wire        blank   = HSYNC;

// Line seam strobe for the sprite engine: the character-counter wrap edge.
assign HWRAP = CLKEN & hcc_last;

// Final precedence (asic-reference §5): HSYNC force-blank beats
// everything; the border (outside DE) beats sprites; a sprite pixel beats
// the decoded screen ink inside DE. SPR_RGB carries {R,G,B} nibbles.
wire show_spr = de_hold & SPR_EN;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		RGB_R <= 4'h0;
		RGB_G <= 4'h0;
		RGB_B <= 4'h0;
		PEN   <= 5'd0;
	end
	else if (PIXEN) begin
		PEN   <= {~eff_de, pen_delayed};
		RGB_R <= blank    ? 4'h0 :
		         show_spr ? SPR_RGB[11:8] : rgb_mux[11:8];
		RGB_G <= blank    ? 4'h0 :
		         show_spr ? SPR_RGB[7:4]  : rgb_mux[7:4];
		RGB_B <= blank    ? 4'h0 :
		         show_spr ? SPR_RGB[3:0]  : rgb_mux[3:0];
	end
end

endmodule
