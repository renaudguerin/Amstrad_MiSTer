//============================================================================
//  ACCURACY REFERENCE
//
//  Technical information sourced from the "Amstrad CPC CRTC Compendium" by
//  Longshot (CC BY-NC-ND).
//
//  The counter, sync, and video-pointer behaviour of this module follows The
//  Amstrad CPC CRTC Compendium v1.10, Serge Querne (Longshot / Logon System),
//  https://shaker.logonsystem.eu -- licensed CC BY-NC-ND 4.0. Its attribution
//  directive requires this notice in the source of CRTC emulation modules and
//  in the credits of any distributed product built from them. Individual rules
//  cite their ACCC section at the point of implementation.
//============================================================================
//
//  Type-1 CRTC engine (UM6845R-type behaviour selected by CRTC_TYPE=1).
//
//  This module holds every type-1-specific rule of this core: the C5
//  vertical-adjustment counter behaviour, the C4/C9 rollover rules, the
//  row-0/row-1 VMA reloads, the fixed 16-line VSYNC, the zero-width HSYNC
//  cut, the R6 bus-write display handling, and the type-1 status register
//  (bit 5), whose flops are private state cleared while another type is
//  selected. All shared counters stay in the CRTC wrapper so a live
//  CRTC_TYPE change continues counting from the same state, exactly as one
//  physical chip would.
//
//  CRTC_TYPE is an input here in its "this type is selected" sense: outputs
//  that the original shared machine gated on it stay gated on it.

module crtc_type1_engine
(
	input            CLOCK,
	input            CLKEN,
	input            nCLKEN,
	input            nRESET,
	input            CRTC_TYPE,
	input            SNA_LOAD,

	input            ENABLE,
	input            nCS,
	input            R_nW,
	input            RS,
	input      [7:0] DI,
	input      [4:0] addr,

	input      [7:0] R0_h_total,
	input      [7:0] R1_h_displayed,
	input      [3:0] R3_h_sync_width,
	input      [6:0] R4_v_total,
	input      [4:0] R5_v_total_adj,
	input      [6:0] R6_v_displayed,
	input      [6:0] R7_v_sync_pos,
	input      [1:0] R8_interlace,
	input      [4:0] R9_v_max_line,

	input      [7:0] hcc,
	input      [7:0] hcc_next,
	input            hcc_last,
	input            hcc_end,
	input      [4:0] line,
	input      [6:0] row,
	input      [4:0] c5,
	input            in_adj,
	input            crtc1_adj_from_row0,

	// Wrapper-owned VSYNC/display state read by type-1 rules.
	input            VSYNC_r,
	input            vsync_allow,
	input      [3:0] hsc,
	input            vde_r,

	// Wrapper-owned interlace parity state (ACCC v1.10 ch.19; shared flops
	// so a live CRTC_TYPE switch continues from the same state).  The engine
	// reads them and contributes the type-1 update decisions.
	input            parity_frame,
	input            parity_c9,

	// Counter next-state contributions.
	output           line_last,
	output           line_new,
	output     [4:0] line_next,
	output reg [4:0] c5_next,
	output           row_last,
	output           row_frame_last,
	output     [6:0] row_next,
	output           row_new,
	output           frame_adj,
	output           adj_from_row0,

	// Video pointer reload / save decisions.
	output           reload,
	// The section 20.3.2 row-0 arm of reload alone (frame origin and every
	// non-final row-0 line boundary).  The wrapper loads this term from the
	// post-edge register file so a same-edge R12/R13 write is caught.
	output           row0_reload,
	output           row_addr_save,

	// Output-stage terms.
	output           field_count_tick,
	output           hsync_off,
	output     [1:0] de_index,
	output           vsync_line_fire,
	output     [3:0] vsc_load,
	output           r7_write_fire,
	output           r6_vde_write,
	output           r6_vde_value,
	output           r6_vder_write,
	output           r6_vder_value,

	// Same-edge R0-widening line-extension request for the section
	// 13.7.1.2 trigger route.  The wrapper owns the shared C0 counter, so
	// this term defers its line-end strobe there; the engine consumes the
	// deferred strobe back as hcc_end.
	output           rfd_r0_extend,

	// Interlace parity updates (F10, ACCC v1.10 sections 19.5.3 and
	// 19.8.2).  pf_write/pc9_write strobes the wrapper's shared flops to
	// the given value on the current edge; line_poke writes C9's bit 0
	// mid-line at the documented 3rd/4th-us stage edges.
	output           pf_write,
	output           pf_value,
	output           pc9_write,
	output           pc9_value,
	output           line_poke,
	output           line_poke_bit,

	// Type-1 status register view (readback multiplexed by the wrapper).
	output           status_bit5
);

/* verilator lint_off WIDTH */

// ------------------------------------------------------------------
// F10: IVM mode flag and the two-stage R8-toggle update (ACCC v1.10
// sections 19.5.3 p.208-209 and 19.8.2 p.225; the 16 SHAKER 22C/3 panels
// pp.210-211).
//
// The former interlace approximation -- C9 stepping by 2 with bit 0 masked
// and the line limit halved via `R9 & ~interlace` -- is replaced by the ivm
// flag and the section 19.8.2 counting below.  The wrapper keeps using R8's
// bit 0 for the FIELD output convention.
//
// ivm is the effective IVM flag: it follows the R8 toggle from the 3rd-us
// stage edge onward, so counting and display switch one character after an
// entering write and the leaving write keeps the old mode until its own
// stage A.  tog_stage runs the documented two-stage update: armed by the
// bus write (register-file timing), stage A (3rd us) at the next CLKEN
// edge, stage B (4th us) at the one after.  A second toggle write while a
// stage pair is in flight is ignored (the source documents one OUT at a
// time; a back-to-back pair is unpinned).
reg [1:0] tog_stage;   // 0 idle, 1 stage A pending, 2 stage B pending
reg       tog_enter;
reg       ivm;

wire r8_write_hit = CRTC_TYPE & ENABLE & RS & ~nCS & ~R_nW & (addr == 5'd08);
wire r8_toggle_write = r8_write_hit & ((DI[1:0] == 2'b11) != ivm);

wire stage_a_edge = CLKEN && tog_stage == 2'd1;
wire stage_b_edge = CLKEN && tog_stage == 2'd2;

// Stage A (3rd us, p.209): the parity of the new C9 is the current C9.0
// xor, when R9 is even, C4.0 -- and C9.0 takes that value immediately, in
// BOTH directions.  p.209 states the 3rd-us rule once ("when R8 changes
// from 3 to 0 or vice versa") and the leaving 4th-us rule is only
// ParityFrame = ParityC9; the four X=1 panels (4(D), 2(B), 12(L), 1(A))
// draw C9.0 changing in the leaving write's 3rd-us column, one character
// before the off cell.  (Review finding B-1, 2026-08-25.)
wire stage_a_x = row[0] & ~R9_v_max_line[0];
wire stage_a_pc9 = line[0] ^ stage_a_x;

// Stage B (4th us, p.209).  Entering IVM: an even ParityFrame re-points
// ParityC9 at C4.0-and-not-R9.0, then ParityFrame := ParityFrame and
// (ParityC9 xor X); for an odd ParityFrame the formula reduces to the old
// C9.0 -- the documented "changes to even, except when ParityFrame and
// ParityC9 were odd" clause.  Leaving IVM: ParityFrame := ParityC9 and
// C9.0 := ParityC9 (the "deactivation modifies C9" clause).
wire stage_b_x = row[0] & ~R9_v_max_line[0];
wire stage_b_pc9_value = parity_frame ? parity_c9 : stage_b_x;
wire stage_b_pf_value = parity_frame & (stage_b_pc9_value ^ stage_b_x);

always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD) begin
		tog_stage <= 2'd0;
		tog_enter <= 0;
		ivm <= 0;
	end
	else if(CRTC_TYPE) begin
		if(r8_toggle_write && tog_stage == 2'd0) begin
			tog_stage <= 2'd1;
			tog_enter <= (DI[1:0] == 2'b11);
		end
		if(CLKEN) begin
			if(stage_a_edge) begin
				tog_stage <= 2'd2;
				ivm <= tog_enter;
			end
			else if(stage_b_edge) begin
				tog_stage <= 2'd0;
			end
			else if(tog_stage == 2'd0) begin
				// No toggle in flight: track the register.  This is what
				// engages IVM when R8=3 is already programmed at reset or
				// arrives through a snapshot load -- there is no toggle
				// write to run the stage machine in that case.
				ivm <= (R8_interlace == 2'b11);
			end
		end
	end
	else begin
		// The type input is live: do not preserve hidden IVM toggle state
		// across a round-trip through type 0.
		tog_stage <= 2'd0;
		tog_enter <= 0;
		ivm <= 0;
	end
end

// Parity update decisions for the wrapper's shared flops are defined below,
// after frame_new_w / row_new exist.

// Section 19.8.2 counting while IVM is active (p.225): pre-increment C9
// when R9 is even, compare with bit 0 masked, and on a match restart C9
// from the toggled ParityC9; otherwise advance by two regardless of R9
// parity.  Type 1 has no separate C9.VMA -- C9 itself carries the parity.
wire [5:0] c9_pre = {1'b0, line} + (R9_v_max_line[0] ? 6'd0 : 6'd1);
wire ivm_row_end = ((c9_pre[4:0] & 5'b11110) == (R9_v_max_line & 5'b11110));
wire [4:0] c9_ivm_step = R9_v_max_line[0] ? (c9_pre[4:0] + 5'd2)
                                          : (c9_pre[4:0] + 5'd1);
// The concat forces 1-bit context for the invert: a bare `~parity_c9`
// arm would be widened to 5 bits first and ~{4'b0, pc9} = 5'b11111.
wire [4:0] pc9_toggled = {4'b0000, R9_v_max_line[0] ? parity_c9
                                                    : ~parity_c9};

wire [4:0] crtc1_line_max = R9_v_max_line;

// The register file and this engine sample the same CLOCK edge.  For the
// RFD-forming R5 0->nonzero write, use the old stored R5 to recognize the
// transition and the live DI value in this edge's rollover decisions.
wire r5_write_hit = CRTC_TYPE & ENABLE & RS & ~nCS & ~R_nW & (addr == 5'd05);
wire rfd_arm = CLKEN & hcc_last & r5_write_hit &
               (R5_v_total_adj == 0) & (|DI[4:0]);
wire [4:0] crtc1_rollover_r5 = rfd_arm ? DI[4:0] : R5_v_total_adj;

// ACCC v1.10 section 11.3.2: Type 1 adjustment ends when C5+1 reaches R5
// evaluated by equality at the line boundary. R5=0 never satisfies this comparison.
// F14 (ACCC v1.10 section 19.6.2 p.216; Q10 resolution in
// accc-author-questions.md item 10): on a ParityFrame-even frame with an
// interlace mode active (R8=1 or 3) and R9+1 a multiple of R5, C4 is
// "incremented once again": the adjustment end instead runs one more line
// -- "C9 counts up to R9 and when it goes back to 0, C4 is incremented
// without taking R4 into account" (section 11.2.4 p.84) -- so the
// additional line carries C9=0 at C4 one past the last adjustment row,
// and the frame origin follows it.  The R5 end test itself is unchanged;
// the intercept defers the origin by exactly one line.  With R5=0 the
// multiple condition is vacuous: a type-1 frame without adjustment lines
// never gains the line (which is what keeps the t21-t24 IVM walks, all
// R5=0, undisturbed).
wire [5:0] type1_r9p1 = {1'b0, R9_v_max_line} + 6'd1;
wire       type1_add_cond = (|R5_v_total_adj) &&
							((type1_r9p1 % {1'b0, R5_v_total_adj}) == 6'd0);
wire       type1_add_armed = R8_interlace[0] && !parity_frame && type1_add_cond;
wire       crtc1_adj_end = CRTC_TYPE & in_adj & ({1'b0, c5} + 6'd1 == {1'b0, crtc1_rollover_r5}) & (|crtc1_rollover_r5);
// The intercept: at the gated adjustment end the origin is deferred one
// line.  The additional line itself ends the frame unconditionally.
reg        type1_add_line_active;
wire       type1_add_intercept = line_new && in_adj && crtc1_adj_end &&
								 type1_add_armed && !type1_add_line_active;
wire       crtc1_adj_end_eff = crtc1_adj_end ||
							   (type1_add_line_active && line_new);

// ACCC v1.10 section 10.3: C9 uses equality, never magnitude.  A zero limit
// reached from C9>0 must let C9 run to 31 and wrap, so no unconditional
// "limit is zero" match may short-circuit the comparison.
wire       line_last_w = (line == crtc1_line_max);

// Two distinct IVM-aware row-end shapes, per section 19.8.2 read against
// the section 11.1 adjustment rules:
//
//   line_limit_match -- the frame-structure test (frame-adj entry latch,
//   adjustment entry): outside adjustment IVM counting replaces the plain
//   C9==R9 comparison.  Adjustment-during-IVM interaction is unpinned in
//   the source, so inside adjustment this follows the adjustment end
//   (named residual in the F10 notes).
//
//   line_row_event -- the per-line row-event test that increments C4:
//   during type-1 adjustment C4 increments at every C9==R9 wrap (section
//   11.1), so the plain wrap test stays live there and the IVM test only
//   applies outside adjustment.
//
//   line_row_structure_last -- the "final line of the row" test for the
//   VMA reload/save/vsync consumers: the plain C9==R9 wrap while
//   adjustment cycles C9, the IVM-aware test otherwise.
wire       line_limit_match = in_adj ? crtc1_adj_end_eff :
                                ivm  ? ivm_row_end : line_last_w;
wire       line_row_event = in_adj ? (line_last_w | crtc1_adj_end_eff) :
                              ivm  ? ivm_row_end : line_last_w;
wire       line_row_structure_last = in_adj ? line_last_w : line_limit_match;
assign     line_last = line_limit_match;
assign     line_new = hcc_end;

// ACCC v1.10 section 13.7.1.2 p.124 (digest-01 section 8.6): a second RFD
// trigger route exists on CRTC 1.  Widening R0 with an OUT(C),reg8 write
// landing exactly at C0==R0 on the last line of the frame (C9==R9, C4==R4,
// R5==0, outside adjustment beforehand) does not end that line: per the
// section 13.6.2 chronogram gist, p.122 ("just-in-time write considered
// this rollover") the widened total is used by this rollover's own
// decision, so
// the comparator match is overridden and the line runs on into the widened
// remainder.  If the last-line condition is then cancelled by R9/R4
// rewrites -- evaluated from the register state held at the line's actual
// end, matching the documented "(C9 != R9 by line end)" / "(C4 != R4 by
// line end)" variant definitions -- RFD arms exactly there and behaves
// like the R5-route state above.  The evidence at hand only covers the
// last-line recipe, so the continuation is gated to it; whether an
// arbitrary mid-frame widening write at C0==R0 also extends its line is
// deliberately left unmodeled pending sourced chronograms.
//
// Every term below requires CRTC_TYPE, so type 0 and ordinary type-1 R0
// writes keep their existing registered-comparator behaviour.
wire r0_write_hit = CRTC_TYPE & ENABLE & RS & ~nCS & ~R_nW & (addr == 5'd00);
wire rfd_r0_widen_at_last_line = CLKEN & hcc_last & r0_write_hit &
                                 ({1'b0, DI} > {1'b0, R0_h_total}) &
                                 (line == crtc1_line_max) &
                                 (row == R4_v_total) &
                                 (R5_v_total_adj == 0) & ~in_adj;
assign rfd_r0_extend = rfd_r0_widen_at_last_line;

wire rfd_r0_cancelled = (line != crtc1_line_max) | (row != R4_v_total);
// Raw hcc_last is safe here by an exact-negation invariant: arming requires
// rfd_r0_cancelled at this edge, and that is the precise complement of the
// line/row conjunction inside rfd_r0_widen_at_last_line, so that term is
// necessarily false on an arm edge.  A re-extend and an arm can therefore
// never share an edge, and hcc_end == hcc_last on every possible arm edge.
// (Opening the window earlier does require rfd_r0_widen_at_last_line true
// at some prior edge; that is what set rfd_r0_pending.)
wire rfd_r0_arm = rfd_r0_pending & CLKEN & hcc_last & rfd_r0_cancelled;

assign line_next = in_adj ? ((line_last_w | crtc1_adj_end_eff) ? 5'd0 : line + 5'd1)
                 : ivm    ? (ivm_row_end ? pc9_toggled : c9_ivm_step)
                 :          (line_last_w ? 5'd0 : line + 5'd1);

// ACCC v1.10 section 11.1 specifies that CRTC 1 has a separate C5 counter
// for vertical adjustment, while C9 continues cycling 0..R9 and C4
// increments at each C9==R9 wrap.
always @(*) begin
	if(line_new) begin
		if(in_adj) begin
			if(crtc1_adj_end_eff) c5_next = 5'd0;
			else c5_next = c5 + 1'd1;
		end
		else c5_next = 5'd0;
	end
	else c5_next = c5;
end

// ACCC v1.10 section 12: outside vertical adjustment C4 is equality-compared
// too.
wire       row_last_w = (row == R4_v_total);
assign     row_last = row_last_w;
wire       frame_adj_CRTC1 = row_last_w && ~in_adj && |crtc1_rollover_r5;
assign     frame_adj = frame_adj_CRTC1;
// Named residual N2 (ACCC v1.11 §11.3.2 p.85; question 20): during stuck
// adjustment with R5=0, C5 loops and C4 advances past R4 at each C9==R9
// wrap. In our model, C4 free-runs and wraps to 0 by 7-bit overflow (at
// C4=128), remaining in adjustment until a reachable R5>0 triggers exit via
// crtc1_adj_end_eff.
wire       crtc1_row_frame_last = in_adj ? (crtc1_adj_end_eff & ~type1_add_intercept) :
										 (row_last_w & ~frame_adj_CRTC1);
assign     row_frame_last = crtc1_row_frame_last;
assign     row_next = row_frame_last ? 7'd0 : row + 1'd1;
assign     row_new = line_new & line_row_event;

wire       frame_new_w = row_new & row_frame_last;

// F14 additional-line state: set on the intercept edge (the gated
// adjustment end), cleared by the true origin that ends the additional
// line.  Intercept priority matters on the shared edge; CLKEN-gated for
// the same level-true-across-the-character reason as the type-0 flop.
always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD | !CRTC_TYPE) type1_add_line_active <= 0;
	else if(CLKEN && type1_add_intercept) type1_add_line_active <= 1;
	else if(CLKEN && frame_new_w) type1_add_line_active <= 0;
end

// Parity update decisions for the wrapper's shared flops.  ParityFrame toggles
// at every C4=C9=C0=0 frame boundary regardless of R8 (French v1.11 section
// 19.5.3 p.209).  ParityC9 toggles at each C4-increment row end when R9 is even
// per the section 19.8.2 p.225 match branch, which toggles the flop and restarts
// C9 from it as one step, including the C4=0 frame-boundary arm (review finding
// B-2).  At the frame origin, however, French section 19.5.3 explicitly assigns
// ParityC9=ParityFrame; BL-038/IA-2 owns the even-R9 discriminator and any
// resulting correction at this edit site.  A
// stage edge coinciding with either wins: the OUT stages carry the
// documented value semantics; that coincidence itself is unpinned.
wire c4_increment_toggle = row_new && !R9_v_max_line[0];
assign pc9_write = stage_a_edge || (stage_b_edge && tog_enter) ||
                   c4_increment_toggle;
assign pc9_value = stage_a_edge ? stage_a_pc9 :
                   stage_b_edge ? stage_b_pc9_value : ~parity_c9;
assign pf_write = stage_b_edge || frame_new_w;
assign pf_value = stage_b_edge ? (tog_enter ? stage_b_pf_value : parity_c9)
                                : ~parity_frame;
assign line_poke = stage_a_edge || stage_b_edge;
assign line_poke_bit = stage_a_edge ? stage_a_pc9 :
                       tog_enter    ? stage_b_pc9_value : parity_c9;

// ACCC v1.10 sections 11.6-11.6.3, pages 87-90: "Rupture For
// Dummies" is armed only by a type-1 R5 write from zero to nonzero that
// is visible on the C0=R0 rollover edge.  The register file and this engine
// sample the same CLOCK edge, so use the old stored R5 value together with
// the live write data.  Feeding rfd_arm into the combinational reload/save
// terms below makes the newly armed state participate in that same rollover
// rather than one line late.  crtc1_rollover_r5 above also makes the new
// value visible to adjustment entry/end on this edge (section 11.4 p.86).

reg rfd_vma_flag;
reg rfd_parity_flag;
reg rfd_frame_parity;
reg rfd_r0_pending;

// Finding F17 (ACCC v1.10 §11.6.1 p.88): an RFD triggered on C9=R9 via the
// general R5-write route disables the state allowing VMA to be updated with
// R12/R13 (vma_flag), while parity management (parity_flag) remains armed.
// In contrast, the section 13.7.1.2 p.124 R0-widening R4-variant route
// (rfd_r0_arm) explicitly specifies that when R4 is modified during the
// extension while C9==R9 still holds, "R12/R13 is considered" (arms the
// VMA-source state).
wire rfd_vma_disarm_hit = rfd_arm & (line == crtc1_line_max);
wire rfd_vma_arm = (rfd_arm & (line != crtc1_line_max)) | rfd_r0_arm;
wire rfd_parity_active = rfd_parity_flag | rfd_arm | rfd_r0_arm;
wire rfd_vma_active = (rfd_vma_flag & ~rfd_vma_disarm_hit) | rfd_vma_arm;

// Section 11.6 p.87: when R1>R0, C0=R1 is unreachable, so the bare
// C9=R9 match deactivates the VMA-source state without a VMA' save.
// This term is level-triggered, so the flag clears at the first CLOCK
// edge inside the last line rather than at the match edge itself;
// that early clear is unobservable through MA because frame_new_w
// forces an R12/R13 reload at that same row boundary anyway.
wire rfd_r1_gt_r0_disarm = rfd_vma_flag &
                           (R1_h_displayed > R0_h_total) & line_last_w;

// Technical information sourced from ACCC v1.10 §11.2.4 p.84:
// If C4 was 0 immediately before adjustment began, VMA loads from R12/R13
// while C4==1 in adjustment.  A positive R4 rewrite on the exact C0=R0
// entry edge suppresses that reload; an R9 rewrite on the same edge does not.
wire r4_positive_write_at_adj_entry = CRTC_TYPE & CLKEN & hcc_last &
									 ENABLE & RS & ~nCS & ~R_nW &
									 (addr == 5'd04) & (|DI[6:0]);
wire crtc1_adj_entry_from_row0 = CRTC_TYPE & !in_adj & row_last_w &
									 line_limit_match & (|crtc1_rollover_r5) & (row == 0) &
									 ~r4_positive_write_at_adj_entry;
assign adj_from_row0 = crtc1_adj_entry_from_row0;
wire crtc1_adj_row1_reload = CRTC_TYPE & (crtc1_adj_entry_from_row0 | (in_adj & crtc1_adj_from_row0 & (row == 1) & ~line_row_structure_last)) & !hcc_next;
wire crtc1_row0_reload = CRTC_TYPE & (frame_new_w | (~line_row_structure_last & !row & !hcc_next));
wire crtc1_rfd_reload = CRTC_TYPE & rfd_vma_active & !hcc_next;
assign reload = crtc1_row0_reload | crtc1_adj_row1_reload | crtc1_rfd_reload;
assign row0_reload = crtc1_row0_reload;

// The VMA' save shares the line-limit test, parity included while IVM is
// active (ACCC v1.10 section 19.8.1 p.220 Note: the C9=R9 / C9.VMA='R9 or
// ParityC9' test also governs the VMA' assignment).
wire row_addr_save_base = hcc == R1_h_displayed && line_row_structure_last;
assign row_addr_save = row_addr_save_base &
                       (~rfd_parity_active | rfd_frame_parity);

always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD) begin
		rfd_vma_flag <= 0;
		rfd_parity_flag <= 0;
		rfd_frame_parity <= 0;
		rfd_r0_pending <= 0;
	end
	else if(CRTC_TYPE) begin
		// ACCC v1.10 section 11.6.1, pages 88-89: parity changes only
		// at a genuine C4=C9=C0=0 frame boundary when R9 is odd.
		if(CLKEN && frame_new_w && R9_v_max_line[0])
			rfd_frame_parity <= ~rfd_frame_parity;

		// Clear only when the parity-gated save really fires, through
		// the R1>R0 bare-C9 route, or when an RFD is triggered on C9=R9
		// (ACCC v1.10 §11.6.1 p.88; F17). A same-edge trigger on C9!=R9
		// arms the source flag.
		if((CLKEN && row_addr_save) | rfd_r1_gt_r0_disarm |
		   (rfd_arm & (line == crtc1_line_max)))
			rfd_vma_flag <= 0;
		else if(rfd_vma_arm) begin
			rfd_vma_flag <= 1;
		end
		if(rfd_arm | rfd_r0_arm) begin
			rfd_parity_flag <= 1;
		end

		// Section 13.7.1.2 trigger window: opened only by the qualifying
		// widening write, closed by the next line end (the extended line's
		// actual end) whether or not that end arms.  CLKEN gates both so a
		// mid-character bus phase can neither open nor close the window.
		if(CLKEN) begin
			if(rfd_r0_widen_at_last_line)
				rfd_r0_pending <= 1;
			else if(hcc_end)
				rfd_r0_pending <= 0;
		end
		// RFD#10, the optional "1-B" chip variant from section 11.6.2
		// p.89, is deliberately not modeled: this baseline implements the
		// ordinary CRTC-1 behavior for every nonzero R5 value.
	end
	else begin
		// CRTC_TYPE is live; do not preserve hidden type-1 RFD state
		// across a round-trip through type 0.
		rfd_vma_flag <= 0;
		rfd_parity_flag <= 0;
		rfd_frame_parity <= 0;
		rfd_r0_pending <= 0;
	end
end

// The half-line comparison reads the stored R0, which still holds the old
// total on the extend edge while hcc_next already carries R0_old+1; the
// extended line's genuine midpoint tick fires later in the remainder as
// usual.
assign field_count_tick = (hcc_next == {1'b0, R0_h_total[7:1]});

// ACCC v1.10 sections 16.1/16.4.2: while adjustment continues, C4 reaches
// row+1 at a C9 wrap and may fire VSYNC there.  On the adjustment-ending
// line C4 instead returns directly to row_next=0; comparing final-row+1
// would invent a C4 value the chip never reaches (review action A1).
// The VSYNC fires crossing into C4=R7.  line_row_structure_last (not the
// plain C9==R9 test) is the row-end test here: during IVM the wrap line's
// C9 differs from R9 on parity-short rows, and the section 19.5.3 p.208
// table starts the pulse at the first line of C4=R7 on every frame.
assign vsync_line_fire = (((CRTC_TYPE && in_adj && !crtc1_adj_end) ?
								 (row + 1'd1) : row_next) == R7_v_sync_pos && line_row_structure_last);
assign vsc_load = 4'd0 - 1'd1;
assign r7_write_fire = !VSYNC_r && vsync_allow;

assign hsync_off = (hsc == R3_h_sync_width) || (R3_h_sync_width == 0);
assign de_index = 2'b00;

// nCLKEN R6-write handling.  Program order in the original shared block:
// vde_r clears first, then three prioritised vde writes (copy delayed
// state, set on leaving the displayed row, clear) resolve last-wins to a
// single final value here.
wire r6w_a = (row != DI[6:0]) && (DI[6:0] != 0);
wire r6w_b = (row == R6_v_displayed) && (DI[6:0] != row);
wire r6w_c = (row == DI[6:0]) || (DI[6:0] == 0);
assign r6_vder_write = (row == DI[6:0]);
assign r6_vder_value = 1'b0;
assign r6_vde_write = r6w_a | r6w_b | r6w_c;
assign r6_vde_value = r6w_c ? 1'b0 : (r6w_b ? 1'b1 : vde_r);

// Type 1 status bit 5 is a line-sampled view of the sticky C4=R6 border
// condition.  Keep that condition separate from vde: writing R6=0 while
// C4>0 also forces vde low, but is not a C4=R6 match and must not set status.
reg        r6_border_condition;
reg        status_bit5_r;
always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD) begin
		r6_border_condition <= 0;
		status_bit5_r <= 0;
	end
	else if(CRTC_TYPE) begin
		if(CLKEN && row_new) begin
			// C4=C9=C0=0 is explicitly outside the R6-border condition,
			// including the otherwise-equal R6=0 case at frame origin.
			if(frame_new_w) r6_border_condition <= 0;
			else if(row_next == R6_v_displayed) r6_border_condition <= 1;
		end

		if(nCLKEN & ENABLE & RS & ~nCS & ~R_nW & addr == 5'd06 &
		   row == DI[6:0]) begin
			r6_border_condition <= 1;
		end

		if(CLKEN && hcc_end) begin
			// A row transition can assert or clear the condition on this
			// same line-end edge, so sample the resulting state rather than
			// the old flop.
			if(row_new && frame_new_w) status_bit5_r <= 0;
			else if(row_new && row_next == R6_v_displayed) status_bit5_r <= 1;
			else status_bit5_r <= r6_border_condition;
		end
	end
	else begin
		// The type input is live.  Do not preserve hidden type-1 status when
		// the model runs as type 0 and is later switched back to type 1.
		r6_border_condition <= 0;
		status_bit5_r <= 0;
	end
end
assign status_bit5 = status_bit5_r;

endmodule
