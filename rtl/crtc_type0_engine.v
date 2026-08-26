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
//  Type-0 CRTC engine (HD6845S / UM6845 as fitted to the CPC classic models).
//
//  This module holds every type-0-specific rule of this core (type 0 is the
//  HD6845S/UM6845, not the UM6845R the old wrapper filename referred to):
//  R0=0 freeze, the last-line / vertical-adjustment arbitration cluster
//  (with its private latches), the C0=0 seam comparators, the partial-VSYNC
//  holdoff latch, and the type-0 shares of the sync and display-enable
//  outputs. It owns only state that is provably type-0-private (cleared or
//  held harmless while another type is selected); all shared counters stay
//  in the CRTC wrapper so a live CRTC_TYPE change continues counting
//  from the same state, exactly as one physical chip would.
//
//  CRTC_TYPE is an input here in its "another type is selected" sense: the
//  bus-write detects and lifecycle clears are gated on it exactly as they
//  were when this logic lived in the shared state machine.

module crtc_type0_engine
(
	input            CLOCK,
	input            CLKEN,
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
	input      [3:0] R3_v_sync_width,
	input      [6:0] R4_v_total,
	input      [4:0] R5_v_total_adj,
	input      [6:0] R6_v_displayed,
	input      [6:0] R7_v_sync_pos,
	input      [1:0] R8_skew,
	input      [1:0] R8_interlace,
	input      [4:0] R9_v_max_line,

	input      [7:0] hcc,
	input      [7:0] hcc_next,
	input            hcc_last,
	input      [4:0] line,
	input      [6:0] row,
	input            in_adj,
	input            field,

	// Wrapper-owned shared counter state read by type-0 rules.
	input            line_last_r,
	input            row_last_r,
	input            frame_adj_r,

	// Wrapper-owned VSYNC state read by type-0 rules.
	input            VSYNC_r,
	input            vsync_allow,
	input      [3:0] hsc,

	// Wrapper-owned interlace parity state (ACCC v1.10 ch.19; shared flops
	// so a live CRTC_TYPE switch continues from the same state).
	input            parity_frame,
	input            parity_c9,
	input            parity_r6,

	// Counter next-state contributions.
	output           r0_frozen,
	output           line_new,
	output     [4:0] line_next,
	output     [4:0] c5_next,
	output           row_frame_last,
	output     [6:0] row_next,
	output           row_new,
	output           frame_adj,

	// C0=0 seam captures for the wrapper's hcc==0 latch load.
	output           c0_line_last,
	output           c0_row_last,

	// Arbitration route into vertical adjustment on the current CLKEN.
	output           in_adj_route,
	// Deferred single C4 increment while frozen at R0=0 (ACCC v1.10 13.2.6).
	output           frozen_row_advance,
	// |effective R5| term for the wrapper's hcc==2 adjustment-schedule update.
	output           hcc2_adj_keep,

	// Video pointer reload / save decisions.
	output           reload,
	output           row_addr_save,

	// Output-stage terms.
	output           field_count_tick,
	output           hsync_off,
	output     [1:0] de_index,
	// Type-0-only substituted border-start trigger for R1>R0 (ACCC v1.10
	// section 17.6.2); the wrapper injects it ahead of the SKEW-DISPTMG
	// delay line. See the assign below for the rule.
	output           spurious_border_off,
	output           vsync_line_fire,
	// F15 VSYNC delay correction (section 19.5.2): suppress holds the
	// natural field=1 fire off during the first line of C4=R7 on a
	// ParityFrame-odd frame; half fires the delayed pulse one line later.
	output           vsync_delay_suppress,
	output           vsync_delay_half,
	output     [3:0] vsc_load,
	output           r7_write_fire,
	output           vsync_holdoff,
	output           vde_toggle,
	output           r6_vder_write,
	output           r6_vder_value,

	// F10 interlace parity updates and C9.VMA view (ACCC v1.10 section
	// 19.8.1 pp.219-220).  pf/pc9/parity_r6 write strobes drive the
	// wrapper's shared flops; ivm_disp/line_vma feed the wrapper's RA mux.
	output           pf_write,
	output           pf_value,
	output           pc9_write,
	output           pc9_value,
	output           pr6_write,
	output           pr6_value,
	output           ivm_disp,
	output     [4:0] line_vma
);

/* verilator lint_off WIDTH */

wire register_write = ENABLE & ~nCS & ~R_nW & RS;

// ------------------------------------------------------------------
// F10: type-0 IVM counting (ACCC v1.10 section 19.8.1 pp.219-220; the
// worked tables pp.221-224, render-verified 2026-08-24, all R9=6).
//
// C9 keeps counting by 1; the address-visible line value is the split
// C9.VMA = ((C9 x 2) + ParityC9) mod 32 ("the more significant bit is
// lost", p.219).  The line-limit test has two independent IVM bits:
//
//   value doubled  -- lines that started with IVM on (ivm_disp, latched
//     at each C0=0 seam from the live R8 register: the doubled value and
//     doubled test start "on the next C0=0, after the C9/R9 test of the
//     line", p.219 -- so the switch line itself tests raw C9).
//   target parity  -- "R9 or ParityFrame" on the switch line (p.219
//     pseudocode), "R9 or ParityC9" on steady IVM lines (p.220), plain R9
//     from the exit line on (parity dropped, p.220).
//
// The exit line keeps the doubled value against plain R9 (p.220 prose;
// pp.223-224 tables), which is exactly value-doubled=1 with target
// parity=0 -- the same form the tables show.
//
// ParityC9 is seeded from ParityFrame when IVM turns on at a seam (the
// tables' doubled display carries the frame parity from the first doubled
// line on).  With even R9 it never changes afterwards: the p.219 row-end
// ParityC9 update is gated on R9 odd -- the printed token `If R9.0=0` was
// adjudicated 2026-08-25 as a typo for `R9.0=1` (author question Q19 main
// token, resolved by default reading; see accc-author-questions.md item 19
// and finding F15).  With odd R9 the update is live (F15, implemented
// 2026-08-26): ParityC9 := C4.0(new) xor ParityFrame at every IVM row end
// and the frame parity at each origin, and the limit target becomes
// R9 + (ParityC9 xor R9.0) -- the p.206 worked example's 5/4 line
// alternation.  The section 19.5.2 VSYNC delay-by-1-line correction for
// odd-C4 R7 on ParityFrame-odd frames is implemented with it (see the
// vsync delay block below).  Q19(b) post-exit behavior remains out of
// scope: the exit line and post-write lines keep the implemented plain-R9
// resume (unpinned divergence recorded in accc-author-questions.md).
reg        ivm_disp_r;    // this line started with IVM active
reg        tog_line;      // an R8 toggle write landed during this line
reg        tog_enter_line;
reg        ivm_exit_frozen;   // F16: retaining frozen C9.VMA comparator after IVM exit
reg  [4:0] exit_frozen_vma;

wire r8_write_hit_t0 = !CRTC_TYPE & ENABLE & RS & ~nCS & ~R_nW & (addr == 5'd08);
wire r8_toggle_write_t0 = r8_write_hit_t0 && ((DI[1:0] == 2'b11) != ivm_disp_r);

// The seam edge (hcc==0) latches the new line's mode from the live R8
// register and consumes the previous line's toggle status.  Both updates
// are nonblocking, so the seam capture below reads the pre-edge values:
// a toggle write from the previous line still qualifies this line's
// target, and a write landing exactly on the seam edge qualifies only
// from the next line ("after the C9/R9 test of the line").
// Named residual (review N-7, 2026-08-25): under the R0=0 freeze C0 is
// pinned at 0, so this seam predicate fires every CLKEN and a toggle
// write's line-scoped status is consumed immediately.  A switch line inside
// an R0=0 freeze therefore loses its one-line target adjustment; the
// combination (IVM toggle during a frozen C0) is unpinned in the source.
wire type0_seam = CLKEN && (hcc == 0);
wire type0_leaving_ivm_line = (tog_line && !tog_enter_line) ||
                              (r8_toggle_write_t0 && (DI[1:0] != 2'b11));

always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD | CRTC_TYPE) begin
		ivm_disp_r <= 0;
		tog_line <= 0;
		tog_enter_line <= 0;
		ivm_exit_frozen <= 0;
		exit_frozen_vma <= 0;
	end
	else begin
		if(type0_seam) begin
			ivm_disp_r <= (R8_interlace == 2'b11);
			tog_line <= 0;
		end
		if(CLKEN && line_new) begin
			if(type0_leaving_ivm_line) begin
				ivm_exit_frozen <= !type0_rollover_line_last;
			end
			else if(type0_rollover_line_last) begin
				ivm_exit_frozen <= 0;
			end
		end
		if(r8_toggle_write_t0) begin
			tog_line <= 1;
			tog_enter_line <= (DI[1:0] == 2'b11);
			if(DI[1:0] != 2'b11) begin
				exit_frozen_vma <= line_vma;
			end
			else begin
				ivm_exit_frozen <= 0;
			end
		end
		// Named residual (review N-6, 2026-08-25): OUT R8,3 followed by
		// OUT R8,0 inside one line leaves tog_enter_line set, so that
		// line's limit target keeps the entering R9-or-ParityFrame form
		// even though R8 ended at 0.  The source pins neither this nor the
		// type-1 back-to-back case; the last write winning would be an
		// equally defensible model.  Unpinned -- do not fixture.
	end
end

// ParityC9 seeding: IVM turning on at this seam plants the frame parity
// (the tables' doubled display parity from the first doubled line on).
wire type0_ivm_turn_on = type0_seam && !ivm_disp_r && (R8_interlace == 2'b11);

// F15: with R9 odd the p.219 row-end update re-derives the parity at every
// IVM row end, ParityC9 := C4.0(new) xor ParityFrame (the pseudocode's
// post-increment C4.0), and at a true frame origin the new frame's parity
// (ParityR6 snapshot) -- the p.206 table's frame-start rows.  Even R9
// leaves ParityC9 at its seeded value, exactly as before.
wire type0_pc9_odd_update = R9_v_max_line[0] && ivm_disp_r &&
                            (pf_write || (row_new && !in_adj));
assign pc9_write = type0_ivm_turn_on || type0_pc9_odd_update;
assign pc9_value = type0_ivm_turn_on ? parity_frame :
                   pf_write         ? parity_r6 :
                                      (row_next[0] ^ parity_frame);
assign ivm_disp  = ivm_disp_r;
assign line_vma  = {line[3:0], parity_c9};

reg        type0_r4_adjust_switch;
reg        type0_r9_live_compare;
reg        type0_r9_at_r0_pending;
reg        type0_c0_1_adjust;
reg        type0_r0_zero_entry_consumed;
reg        type0_zero_adj_entry;
reg        type0_r5_adjust_override;
reg  [4:0] type0_r5_adjust_target;

// Technical information sourced from the "Amstrad CPC CRTC Compendium" by
// Longshot (CC BY-NC-ND). ACCC v1.10 section 11.2.2 specifies that an R4
// write on a type-0 last line from C0=2 through C0=R0 switches the line-end
// comparison from C9/R9 to C9/R5 before the next line is calculated.
wire       type0_r4_window_write;
wire       type0_r4_switch_write;
wire       type0_r4_switch_clear_write;
wire       type0_r9_compare_write;
wire       type0_r4_switch_active = (type0_r4_adjust_switch | type0_r4_switch_write) &
									~type0_r4_switch_clear_write;
wire       type0_r9_compare_active = type0_r9_live_compare | type0_r9_compare_write;
wire       type0_c0_1_break_write = !CRTC_TYPE && register_write && hcc == 1 &&
									frame_adj_r && !in_adj &&
									((addr == 5'd04 && DI[6:0] != row) ||
									 (addr == 5'd09 && DI[4:0] != line));
wire       type0_c0_1_adjust_active = type0_c0_1_adjust | type0_c0_1_break_write;

// Type 0 still compares C0 with R0 when both are zero, but that repeated
// equality pins C0 rather than completing a stream of one-character lines.
wire       r0_frozen_w = !R0_h_total && !hcc;
assign     r0_frozen = !CRTC_TYPE && r0_frozen_w;

// F10: the old `& ~interlace` halving of the limit is gone; the IVM limit
// comparison below replaces it (section 19.8.1).
wire [4:0] crtc0_line_max = (in_adj ? (R5_v_total_adj - 1'd1) : R9_v_max_line);

// The live IVM-aware line-limit comparison (evaluated at hcc_last by the
// rollover's live path, and by the VSYNC row-end consumer).
//
// F15 (ACCC v1.10 section 19.5.2 pp.205-206, the rendered R9=7 worked
// example; the p.219 row-end gate adjudicated as `If R9.0=1` in author
// question Q19): the target is a 6-bit sum.  Switch line: raw C9 against
// R9 + ParityFrame -- the p.219 prose and its overflow sentence ("If C9=R9
// and the parity is odd, then the test C9=R9+1 is false") pin the addition
// form.  Steady IVM lines: the row ends at the first C9.VMA at or past R9,
// i.e. target R9 + (ParityC9 xor R9.0) -- odd-parity rows end exactly at
// R9, even-parity rows at R9+1, producing the documented 5/4 line
// alternation.  For even R9 the addend reduces to ParityC9 and R9+P equals
// the previous R9-or-P form bit-for-bit, so the t22 vectors are unchanged.
// Exit/toggle-leave and IVM-off lines: plain R9.  The 6-bit compare keeps
// an overflowed target (R9=31 with an even addend) unmatchable, preserving
// the old degenerate no-row-end behavior.
wire [5:0] type0_limit_addend = tog_line ? (tog_enter_line ? {5'b00000, parity_frame}
                                                           : 6'd0) :
                             ivm_disp_r ? {4'b0000, parity_c9 ^ R9_v_max_line[0]} : 6'd0;
wire [5:0] type0_limit_target6 = {1'b0, R9_v_max_line} + type0_limit_addend;
wire [4:0] type0_limit_value = ivm_disp_r ? line_vma :
                               ivm_exit_frozen ? exit_frozen_vma : line;
wire       type0_ivm_limit = ({1'b0, type0_limit_value} == type0_limit_target6);

// ACCC v1.10 section 10.3: C9 uses equality, never magnitude.  A zero limit
// reached from C9>0 must let C9 run to 31 and wrap, so no unconditional
// "limit is zero" match may short-circuit the comparison.  Adjustment keeps
// its plain C9-vs-R5 reuse; outside adjustment the IVM comparison applies.
wire       line_last = in_adj ? (line == crtc0_line_max) : type0_ivm_limit;
wire [4:0] type0_r5_adjust_target_effective = type0_r5_window_write ?
											(DI[4:0] - 1'd1) : type0_r5_adjust_target;
wire       type0_r5_write = !CRTC_TYPE && register_write && addr == 5'd05;
wire [4:0] type0_effective_r5 = type0_r5_write ? DI[4:0] : R5_v_total_adj;
wire       type0_r5_window_write = type0_r5_write && hcc <= 2 && in_adj;
wire       type0_r5_override_active = (type0_r5_adjust_override | type0_r5_window_write) & in_adj;
wire       type0_zero_adj_entry_active = type0_zero_adj_entry &
										 ~(type0_r5_write && (hcc <= 2) && (|DI[4:0]));
wire [4:0] type0_adjust_line_max =
					(type0_r5_override_active ? type0_r5_adjust_target_effective :
					 (type0_effective_r5 - 1'd1));
wire       type0_r9_at_r0_write;
wire       type0_r9_at_r0_active = type0_r9_at_r0_pending | type0_r9_at_r0_write;
// Section 10.3.1.1: once `Last Line` is false, the rollover uses the live
// C9/R9 comparison rather than the value latched at C0=0.  That is what makes
// the section 12.2.1 first-line RLAL sequence (R9=0 written after the C0<2
// window) increment C4 on the very next line.
// F10 (section 19.8.1): the live comparison is the IVM-aware limit test;
// with IVM off it reduces to the plain C9==R9 equality.  This is the path
// mid-line R8 toggles act through: the exit line's doubled-value-vs-plain-R9
// form and the switch line's raw-value-vs-R9-or-ParityFrame form are both
// carried by type0_ivm_limit's per-line bits.
wire       type0_live_line_last = type0_ivm_limit;
wire       type0_last_line_armed = line_last_r & row_last_r;
wire       type0_rollover_line_last = type0_c0_1_adjust_active ? 1'b0 :
									 type0_r9_at_r0_active ?
									 (line == type0_adjust_line_max) :
								 type0_r4_switch_active ?
									 (line == type0_adjust_line_max) :
								 type0_r9_compare_active ? type0_live_line_last :
								 type0_r5_override_active ?
									 ((line == type0_adjust_line_max) | type0_zero_adj_entry_active) :
								 (in_adj | type0_last_line_armed) ? line_last_r :
									 type0_live_line_last;
wire       type0_rollover_row_last = type0_r9_at_r0_active ? line_last_r :
									 type0_rollover_line_last;

assign line_new = hcc_last && !r0_frozen_w;
// F14: on the intercept edge the "line" that starts is the additional one;
// its C9 continues the adjustment count to R5 (section 11.2 p.84).
assign line_next = type0_add_intercept ? R5_v_total_adj :
                   type0_rollover_line_last ? 5'd0 : line + 5'd1;
assign c5_next = 5'd0;

// ACCC v1.10 section 12: outside vertical adjustment C4 is equality-compared
// too.  Type 0's R4=0 frame end comes from the `Last Line` / adjustment
// arbitration above, not from a magnitude special case.
wire       row_last_w = (row == R4_v_total);
wire       crtc0_row_frame_last = (row_last_r | in_adj) & ~type0_adjustment_selected;
// F14 (ACCC v1.10 section 19.6.1 p.216; Q10 resolution in
// accc-author-questions.md item 10): with an interlace mode active (R8=1 or
// 3) and ParityR6 odd, one additional line is appended after the R5
// adjustment lines -- directly after the last character row when R5=0 --
// before the frame origin.  ParityR6 is captured when C4 reaches R6 and
// freezes when R6>R4 (section 19.5.2 p.205), so the gate persists: a line
// every frame if frozen odd, never if frozen even (section 19.6.1 p.216).
// C4 is incremented only once for the whole additional-lines period and
// equals R4+1 there (section 19.6.1 p.216): the adjustment-entry increment
// to R4+1 (section 11.2.2 p.81) already covers the R5 lines, so the
// additional line holds C4=R4+1 and continues the adjustment count at
// C9=R5 -- "the counting is done as if this line had been added to R5"
// (section 11.2 p.84).  The frame origin (C4=C9=C0=0, ParityFrame snapshot,
// VMA reload) moves to the end of that line; its duration counts in the
// following odd frame (section 19.3 p.199).
wire       type0_add_armed = R8_interlace[0] && parity_r6;
reg        type0_add_line_active;
wire       type0_frame_end_raw = row_new & crtc0_row_frame_last;
wire       type0_add_intercept = type0_frame_end_raw && !type0_add_line_active &&
								 type0_add_armed;
assign     frame_adj = type0_adjustment_selected | type0_add_intercept;
assign     row_frame_last = crtc0_row_frame_last & ~type0_add_intercept;
assign     row_next = type0_add_intercept ? (in_adj ? row : row + 7'd1) :
									 row_frame_last ? 7'd0 : row + 1'd1;
assign     row_new = line_new & type0_rollover_row_last;
wire       frame_new_w = row_new & row_frame_last;

// Type-0 parity rules (section 19.5.2 p.205): ParityFrame snapshots
// ParityR6 at the frame origin (C4=C9=C0=0); ParityR6 captures
// ParityFrame xor 1 when C4 reaches R6 -- independent of R8, frozen when
// R6>R4 (the event then never fires).  The frame origin itself is excluded
// from the R6 capture (same convention as the R6-border-condition
// exclusion at C4=C9=C0=0).  The p.219 pseudocode's alternative frame-end
// toggle (ParityFrame ^= ParityR6 when C4==R4) is equivalent to this
// snapshot at the origin and is not duplicated here.
assign pf_write  = frame_new_w;
assign pf_value  = parity_r6;
assign pr6_write = row_new && !frame_new_w && !in_adj &&
                   (row_next == R6_v_displayed);
assign pr6_value = parity_frame ^ 1'd1;

wire       type0_r4_at_c0_write = register_write && addr == 5'd04 && hcc == 0;
wire       type0_r9_at_c0_write = register_write && addr == 5'd09 && hcc == 0;
wire       type0_r5_at_c0_write = type0_r5_write && hcc == 0;
wire [6:0] type0_c0_r4 = type0_r4_at_c0_write ? DI[6:0] : R4_v_total;
wire [4:0] type0_c0_r9 = type0_r9_at_c0_write ? DI[4:0] : R9_v_max_line;
wire [4:0] type0_c0_r5 = type0_r5_at_c0_write ? DI[4:0] : R5_v_total_adj;
wire [4:0] type0_c0_adjust_line_max = (type0_c0_r5 - 1'd1);
wire       type0_c0_zero_adj_entry = type0_zero_adj_entry & ~(type0_r5_at_c0_write & (|DI[4:0]));
// The C0=0 seam evaluates `Last Line` against the effective (possibly
// same-edge written) R4/R9.  Both are plain equalities: a zero limit is an
// ordinary value that only matches a counter already at zero.  F10: the
// seam forms the new line's IVM comparison from the live R8 register (the
// mode that line will run) and the still-pending toggle status; with IVM
// off it reduces to the plain C9==R9 equality.  F15: the same 6-bit
// R9-plus-addend target form as the live comparison above, with the seam's
// own toggle/IVM bits.
wire       type0_seam_ivm = (R8_interlace == 2'b11);
wire [4:0] type0_seam_value = type0_seam_ivm ? line_vma :
                              ivm_exit_frozen ? exit_frozen_vma : line;
wire [5:0] type0_seam_addend = tog_line ? (tog_enter_line ? {5'b00000, parity_frame}
                                                          : 6'd0) :
                             type0_seam_ivm ? {4'b0000, parity_c9 ^ R9_v_max_line[0]} : 6'd0;
wire [5:0] type0_seam_target6 = {1'b0, type0_c0_r9} + type0_seam_addend;
wire       type0_c0_row_last = (row == type0_c0_r4);
// F14: on the additional line (C9=R5) the seam must latch the row-end so
// the line ends at the frame origin; the plain adjustment limit (R5-1)
// cannot match there.
wire       type0_c0_line_last = in_adj ? (((type0_add_line_active && (line == type0_c0_r5)) |
											(line == type0_c0_adjust_line_max)) | type0_c0_zero_adj_entry) :
										(({1'b0, type0_seam_value} == type0_seam_target6));
assign     c0_line_last = type0_c0_line_last;
assign     c0_row_last = type0_c0_row_last;

wire       type0_adjustment_selected = type0_c0_1_adjust_active |
									 ((hcc == 2) ?
									  frame_adj_r & |type0_effective_r5 : frame_adj_r);
assign type0_r4_window_write = !CRTC_TYPE && register_write && addr == 5'd04 &&
								  hcc >= 2 && hcc <= R0_h_total &&
								  type0_adjustment_selected && !in_adj;
assign type0_r4_switch_write = type0_r4_window_write && DI[6:0] != row;
assign type0_r4_switch_clear_write = type0_r4_window_write && DI[6:0] == row;
assign type0_r9_compare_write = !CRTC_TYPE && register_write && addr == 5'd09 &&
								  hcc >= 2 && hcc < R0_h_total &&
								  type0_adjustment_selected && !in_adj;
assign type0_r9_at_r0_write = !CRTC_TYPE && register_write && addr == 5'd09 &&
								 hcc_last && hcc >= 2 && type0_adjustment_selected && !in_adj;

assign in_adj_route = !CRTC_TYPE && line_new && (type0_r4_switch_active | type0_r9_compare_active |
												type0_c0_1_adjust_active);

// ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.6: on the first repeated C0=0,
// C9 freezes but a matching C9/R9 consumes the already-armed C4 increment
// exactly once. A simultaneous C4/R4 match enters the short-R0 default
// adjustment route.
assign frozen_row_advance = !CRTC_TYPE && r0_frozen && !in_adj &&
	!type0_r0_zero_entry_consumed && line == R9_v_max_line;

assign hcc2_adj_keep = |type0_effective_r5;

assign reload = ~CRTC_TYPE & frame_new_w;
assign row_addr_save = hcc == R1_h_displayed && type0_live_line_last;

// F14 additional-line state: set on the intercept edge (the would-be frame
// origin), cleared by the true origin that ends the additional line.  The
// intercept priority in the if-chain matters: on the intercept edge itself
// frame_new_w is masked to 0, but the raw end condition is what armed it.
// CLKEN-gated: both terms are levels true across the whole hcc_last
// character, so an ungated flop would toggle on every clock edge and feed
// the oscillation back through the parity block.
always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD | CRTC_TYPE) type0_add_line_active <= 0;
	else if(CLKEN && type0_add_intercept) type0_add_line_active <= 1;
	else if(CLKEN && frame_new_w) type0_add_line_active <= 0;
end

// Register writes are clocked at the 16 MHz bus rate, not only on CLKEN.
// Retain the selected comparator for the rest of the current character line.
always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD | CRTC_TYPE) begin
		type0_r4_adjust_switch <= 0;
		type0_r9_live_compare <= 0;
		type0_r9_at_r0_pending <= 0;
		type0_c0_1_adjust <= 0;
		type0_r0_zero_entry_consumed <= 0;
		type0_zero_adj_entry <= 0;
		type0_r5_adjust_override <= 0;
		type0_r5_adjust_target <= 0;
	end
	else begin
		if(type0_r4_window_write) type0_r4_adjust_switch <= DI[6:0] != row;
		else if(CLKEN && line_new) type0_r4_adjust_switch <= 0;
		if(type0_r9_compare_write) type0_r9_live_compare <= 1;
		else if(CLKEN && line_new) type0_r9_live_compare <= 0;
		if(type0_r9_at_r0_write) type0_r9_at_r0_pending <= 1;
		else if(CLKEN && line_new) type0_r9_at_r0_pending <= 0;
		// A line boundary consumes the current-line override.  An accepted
		// write on that same edge still affects the combinational rollover
		// through type0_r5_adjust_target_effective, but must not leak into the
		// next line's state.
		if(CLKEN && line_new) begin
			type0_r5_adjust_override <= 0;
			type0_r5_adjust_target <= 0;
		end
		else if(type0_r5_window_write) begin
			type0_r5_adjust_override <= 1;
			type0_r5_adjust_target <= DI[4:0] - 1'd1;
		end
		// A C0=1 write can also be the R0=1 rollover. Its combinational
		// effect is consumed on that edge and must not leak into the next line.
		if(CLKEN && line_new) type0_c0_1_adjust <= 0;
		else if(type0_c0_1_break_write) type0_c0_1_adjust <= 1;
		if(!r0_frozen_w) type0_r0_zero_entry_consumed <= 0;
		else if(CLKEN) type0_r0_zero_entry_consumed <= 1;
		if(type0_r5_write && (hcc <= 2) && (|DI[4:0])) type0_zero_adj_entry <= 0;

		if(CLKEN) begin
			if(line_new && (type0_r4_switch_active | type0_r9_compare_active |
							type0_c0_1_adjust_active))
				type0_zero_adj_entry <= type0_c0_1_adjust_active & !(|type0_effective_r5);
			else if(r0_frozen_w && !in_adj && !type0_r0_zero_entry_consumed &&
					line == R9_v_max_line && row == R4_v_total)
				type0_zero_adj_entry <= !(|R5_v_total_adj);
			else if(row_new) begin
				if(frame_adj)
					type0_zero_adj_entry <= !(|type0_effective_r5);
				else if(frame_new_w)
					type0_zero_adj_entry <= 0;
			end
		end
	end
end

// Partial first line of a VSYNC started by an R7=C4 write after C0=1: the
// count tick that ends the write character must not consume the first C3h
// unit (ACCC v1.10 sections 14.2 and 16.4.1). Cleared while another type is
// selected or a snapshot loads; never carried across either boundary.
reg        type0_vsync_wait_line_start;

wire vsync_count_tick_t0 = CLKEN &&
	(field ? (!r0_frozen_w && (hcc_next == {1'b0, R0_h_total[7:1]})) : line_new);

assign field_count_tick = !r0_frozen_w && (hcc_next == {1'b0, R0_h_total[7:1]});

wire r7_write_hit = ENABLE & RS & ~nCS & ~R_nW & addr == 5'd07;
wire r7_write_fire_t0 = !VSYNC_r && vsync_allow && (hcc > 1);
assign r7_write_fire = r7_write_fire_t0;
assign vsync_holdoff = !CRTC_TYPE && type0_vsync_wait_line_start;

always @(posedge CLOCK) begin
	if(~nRESET) begin
		type0_vsync_wait_line_start <= 0;
	end
	else begin
		if(vsync_count_tick_t0) begin
			if(!CRTC_TYPE && type0_vsync_wait_line_start)
				type0_vsync_wait_line_start <= 0;
		end
		if(r7_write_hit) begin
			if(row != DI[6:0]) begin
				// A false comparison re-arms only; the holdoff latch is
				// untouched outside the equal-comparison branch.
			end
			else if(r7_write_fire_t0)
				type0_vsync_wait_line_start <= !vsync_count_tick_t0;
			else if(!VSYNC_r) begin
				type0_vsync_wait_line_start <= 0;
			end
		end
		if(CRTC_TYPE || SNA_LOAD) type0_vsync_wait_line_start <= 0;
	end
end

// ACCC v1.10 section 21.3: with R6=0 the display toggles border/display each
// character at frame origin instead of latching a stable state.
assign vde_toggle = !CRTC_TYPE && row == 0 && line == 0 && R6_v_displayed == 0;

// F15 (ACCC v1.10 section 19.5.2 pp.205-206): with R9 odd (the balancing
// scheme active) and R7 on an odd C4, the ParityFrame-odd frame delays the
// VSYNC by one line -- it fires at the second line of C4=R7, where
// C9.VMA=2, so the pulse lands at the same physical line offset on both
// frames.  Even R7 needs no correction (the frames already agree); the
// delay is keyed on ParityFrame, not on the legacy field flop.
// Two staged line-granular flops, advanced at each line end:
//   d1 -- set where the natural fire was due and the arm holds; suppresses
//         the natural fires for the next line (the first line of C4=R7)
//         and fires the seam path at its end, so the pulse starts at the
//         second line's seam;
//   d2 -- high for the line after that; the field=1 half-line path
//         consumes it at the second line's half-line tick (its natural
//         fire point, one line late).
wire       vsync_fire_seam = ((row_next) == R7_v_sync_pos && line_last);
wire       type0_vsync_delay_arm = R9_v_max_line[0] && ivm_disp_r &&
                                   R7_v_sync_pos[0] && parity_frame && !in_adj;
reg        type0_vsync_delay_d1;
reg        type0_vsync_delay_d2;
always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD | CRTC_TYPE) begin
		type0_vsync_delay_d1 <= 0;
		type0_vsync_delay_d2 <= 0;
	end
	else if(CLKEN && hcc_last) begin
		type0_vsync_delay_d1 <= vsync_fire_seam && type0_vsync_delay_arm;
		type0_vsync_delay_d2 <= type0_vsync_delay_d1;
	end
end
assign vsync_line_fire = (vsync_fire_seam && !type0_vsync_delay_arm) ||
                         type0_vsync_delay_d1;
// Both delay outputs are qualified with the type selection: the clearing
// edge of an armed d1/d2 is the first CLOCK edge after a live switch to
// type 1, and the wrapper samples the pre-edge value on that same edge --
// without the qualifier a type-1 field count tick landing exactly there
// would see the stale suppress and lose its natural VSYNC fire (review
// blocking 1, 2026-08-26).
assign vsync_delay_suppress = !CRTC_TYPE && type0_vsync_delay_d1;
assign vsync_delay_half = !CRTC_TYPE && type0_vsync_delay_d2;
assign vsc_load = R3_v_sync_width - 1'd1;

assign hsync_off = (hsc == R3_h_sync_width);
assign de_index = R8_skew;

// Technical information sourced from the "Amstrad CPC CRTC Compendium" by
// Longshot (CC BY-NC-ND). ACCC v1.10 section 17.6.2 (p.186): when R1>R0 the
// C0=R1 DISPTMG-off comparison can never fire (C0 wraps at R0 first), so a
// type-0 CRTC substitutes C0=R0 as the border-start trigger -- the source
// describes a 0.5 us spurious interline border byte. Stage 1 deliberately
// keeps this core's character-granular DE contract: the accepted model holds
// DISPTMG off for the full 1 us character containing that trigger, rather than
// claiming the exact half-character pin timing. F13 remains hardware-blocked
// for that distinction. Section 19.2.4 (p.195): a programmed SKEW-DISPTMG
// delay is counted from the substituted trigger, so the wrapper must inject
// this ahead of the delay line; mode 2'b11 (non-output) suppresses it entirely.
// The term is combinational by intent: section 17.3 has the C0=R1
// comparison evaluate live, so the substitution tracks live R1/R0 writes
// too. Gated on !CRTC_TYPE because type 1 emits no border byte at all in
// this configuration (ACCC p.186-187; section 28.1.6 discriminator).
//
// Recorded residual: with R0=0 the frozen C0 pins hcc==R0 permanently, so
// this term holds DISPTMG off for every character; the book's alternating
// display/border-byte description of that extreme (p.186) needs a toggle
// mechanism and remains outside this Stage 1 approximation.
assign spurious_border_off = !CRTC_TYPE &&
							 (R1_h_displayed > R0_h_total) &&
							 (hcc == R0_h_total);

// nCLKEN R6-write handling: type 0 clears the delayed display-enable latch
// unless the write lands on the frame-origin toggle point.
assign r6_vder_write = (row == DI[6:0]) && !(row == 0 && line == 0);
assign r6_vder_value = 1'b0;

endmodule
