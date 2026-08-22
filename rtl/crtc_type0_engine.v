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
	output           vsync_line_fire,
	output     [3:0] vsc_load,
	output           r7_write_fire,
	output           vsync_holdoff,
	output           vde_toggle,
	output           r6_vder_write,
	output           r6_vder_value
);

/* verilator lint_off WIDTH */

wire register_write = ENABLE & ~nCS & ~R_nW & RS;

wire [4:0] interlace = &R8_interlace[1:0];

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

wire [4:0] crtc0_line_max = (in_adj ? (R5_v_total_adj - 1'd1) : R9_v_max_line) & ~interlace;

// ACCC v1.10 section 10.3: C9 uses equality, never magnitude.  A zero limit
// reached from C9>0 must let C9 run to 31 and wrap, so no unconditional
// "limit is zero" match may short-circuit the comparison.
wire       line_last = (line == crtc0_line_max);
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
					 (type0_effective_r5 - 1'd1)) & ~interlace;
wire       type0_r9_at_r0_write;
wire       type0_r9_at_r0_active = type0_r9_at_r0_pending | type0_r9_at_r0_write;
// Section 10.3.1.1: once `Last Line` is false, the rollover uses the live
// C9/R9 comparison rather than the value latched at C0=0.  That is what makes
// the section 12.2.1 first-line RLAL sequence (R9=0 written after the C0<2
// window) increment C4 on the very next line.
wire       type0_live_line_last = (line == (R9_v_max_line & ~interlace));
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
assign line_next = (type0_rollover_line_last ?
						 5'd0 : line + 1'd1 + interlace) & ~interlace;
assign c5_next = 5'd0;

// ACCC v1.10 section 12: outside vertical adjustment C4 is equality-compared
// too.  Type 0's R4=0 frame end comes from the `Last Line` / adjustment
// arbitration above, not from a magnitude special case.
wire       row_last_w = (row == R4_v_total);
wire       crtc0_row_frame_last = (row_last_r | in_adj) & ~type0_adjustment_selected;
assign     frame_adj = type0_adjustment_selected;
assign     row_frame_last = crtc0_row_frame_last;
assign     row_next = row_frame_last ? 7'd0 : row + 1'd1;
assign     row_new = line_new & type0_rollover_row_last;
wire       frame_new_w = row_new & row_frame_last;

wire       type0_r4_at_c0_write = register_write && addr == 5'd04 && hcc == 0;
wire       type0_r9_at_c0_write = register_write && addr == 5'd09 && hcc == 0;
wire       type0_r5_at_c0_write = type0_r5_write && hcc == 0;
wire [6:0] type0_c0_r4 = type0_r4_at_c0_write ? DI[6:0] : R4_v_total;
wire [4:0] type0_c0_r9 = type0_r9_at_c0_write ? DI[4:0] : R9_v_max_line;
wire [4:0] type0_c0_r5 = type0_r5_at_c0_write ? DI[4:0] : R5_v_total_adj;
wire [4:0] type0_c0_adjust_line_max = (type0_c0_r5 - 1'd1) & ~interlace;
wire       type0_c0_zero_adj_entry = type0_zero_adj_entry & ~(type0_r5_at_c0_write & (|DI[4:0]));
// The C0=0 seam evaluates `Last Line` against the effective (possibly
// same-edge written) R4/R9.  Both are plain equalities: a zero limit is an
// ordinary value that only matches a counter already at zero.
wire       type0_c0_row_last = (row == type0_c0_r4);
wire       type0_c0_line_last = in_adj ? ((line == type0_c0_adjust_line_max) | type0_c0_zero_adj_entry) :
									 (line == type0_c0_r9);
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

assign vsync_line_fire = ((row_next) == R7_v_sync_pos && line_last);
assign vsc_load = R3_v_sync_width - 1'd1;

assign hsync_off = (hsc == R3_h_sync_width);
assign de_index = R8_skew;

// nCLKEN R6-write handling: type 0 clears the delayed display-enable latch
// unless the write lands on the frame-origin toggle point.
assign r6_vder_write = (row == DI[6:0]) && !(row == 0 && line == 0);
assign r6_vder_value = 1'b0;

endmodule
