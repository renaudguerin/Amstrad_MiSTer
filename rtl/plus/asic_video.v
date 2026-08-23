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
//   - Status registers R10/R11 contents and the mod-8 read map: P5 owns all
//     readback semantics (ACCC §21.2.3, §21.3.4); DO is open-bus neutral.
//   - Light pen R16/R17: no light-pen input exists on CPC hardware.
//   - The R4=0-at-C0=0-with-Rom-select I/O race (ACCC §12.5 p.101) is a Z80
//     bus-level ASIC race owned by the register-interface layer, not the
//     counter engine.
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
	output     [7:0] DO,

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
	output reg [4:0]    PEN        // observability for later raster consumers:
	                               // {showing_border, decoded ink nibble}
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
reg [5:0] R12_start_addr_h;
reg [7:0] R13_start_addr_l;
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
		R12_start_addr_h <= 6'd0;
		R13_start_addr_l <= 8'd0;
	end
	else if (ENABLE & ~nCS & ~R_nW) begin
		if (~RS) begin
			addr <= DI[4:0];
		end
		else begin
			case (addr)
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
			5'd12: R12_start_addr_h <= DI[5:0];
			5'd13: R13_start_addr_l <= DI;
			default: ;  // 10/11 status, 14-17 land with later phases
			endcase
		end
	end
end

// P5 owns every readback semantic including the mod-8 select map (§21.2.3);
// until then the data pin stays at the unselected level.
assign DO = 8'hFF;

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
// Two-stage {R12,R13} -> VMA' -> VMA behaviour, as on type 0 (ACCC
// §20.3.4 p.243): at every C0=0 while C4=0 BOTH pointers reload from
// R12/R13 — note there is no C9 term in the type-3/4 condition, unlike
// type 0's C4=C9=C0=0 (§20.3.1). At any other line start VMA loads from
// VMA'. The row-end capture VMA' <- VMA fires on the live comparison
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

wire row_latch_event = CLKEN && !in_adj &&
                       (hcc == R1_h_displayed) &&
                       (raster == R9_v_max_line);

always @(posedge CLOCK) begin
	if (!nRESET) begin
		vma       <= 14'd0;
		vma_latch <= 14'd0;
	end
	else if (CLKEN) begin
		if (row_latch_event) vma_latch <= vma;

		if (hcc_last) begin
			// §20.3.4 frame-start reload has highest priority. Otherwise
			// a simultaneous C0=R1=R0 row-end capture supplies the next
			// row base, so do not overwrite VMA with the stale latch value
			// on that same edge (ACCC §17.1 p.176 / §17.6.1 p.185).
			if (!adj_n && (charline_n == 7'd0)) begin
				vma       <= {R12_start_addr_h, R13_start_addr_l};
				vma_latch <= {R12_start_addr_h, R13_start_addr_l};
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
			vde <= (charline_n != R6_v_displayed);
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
assign RA   = raster;

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
// Unverified P1 model assumption (pinned by t05x vectors, mirrors t04i):
// the first pixel of a character's even byte is presented on dot 0 and
// RGB is registered once per dot (one-dot presentation latency). The real
// GA has fixed pipeline latencies relative to its load/DISPEN cadence —
// the Plus shows INKR effects at ~1/4 character ([KT]/Grimware INKR
// timings) and the 40010 starts mode-2 rasterisation one pixel early —
// both deferred to the motherboard-integration milestone (architecture
// §5 Risk 1), where the timing contract is decided with fitter data.
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
	else if (PIXEN) begin
		if (CLKEN) begin
			vid_even <= VIDEOD[7:0];
			de_hold  <= DE;
		end
		else if (pix_cnt == 4'd8) begin
			vid_odd <= VIDEOD[15:8];
		end
	end
end

// Grimware §RMR: "VM ... will take effect after the next HSync" (and the
// byte=>pixels decoder needs an HSYNC >= 2 us to update; type-3 HSYNC is
// always >= 2 us because an R3l of 0 still yields a 16-character pulse,
// ACCC §14.5 p.141). The netlist re-times mode identically
// (MODE_SYNC = ~HSYNC_O, ga40010.sv).
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

wire [4:0] hw_sel  = de_hold ? INKR_I[pen_nib*5 +: 5] : BORDER_I;
wire [11:0] rgb_mux = legacy_colour(hw_sel);
wire        blank   = HSYNC;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		RGB_R <= 4'h0;
		RGB_G <= 4'h0;
		RGB_B <= 4'h0;
		PEN   <= 5'd0;
	end
	else if (PIXEN) begin
		PEN   <= {~de_hold, pen_nib};
		RGB_R <= blank ? 4'h0 : rgb_mux[11:8];
		RGB_G <= blank ? 4'h0 : rgb_mux[7:4];
		RGB_B <= blank ? 4'h0 : rgb_mux[3:0];
	end
end

endmodule
