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
//  Type-1 CRTC engine (UM6845R behaviour selected by CRTC_TYPE=1).
//
//  This module holds every type-1-specific rule of the UM6845R core: the C5
//  vertical-adjustment counter behaviour, the C4/C9 rollover rules, the
//  row-0/row-1 VMA reloads, the fixed 16-line VSYNC, the zero-width HSYNC
//  cut, the R6 bus-write display handling, and the type-1 status register
//  (bit 5), whose flops are private state cleared while another type is
//  selected. All shared counters stay in the UM6845R wrapper so a live
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
	output           r6_vde_write,
	output           r6_vde_value,
	output           r6_vder_write,
	output           r6_vder_value,

	// Type-1 status register view (readback multiplexed by the wrapper).
	output           status_bit5
);

/* verilator lint_off WIDTH */

wire [4:0] interlace = &R8_interlace[1:0];

wire [4:0] crtc1_line_max = R9_v_max_line & ~interlace;

// ACCC v1.10 section 10.3: C9 uses equality, never magnitude.  A zero limit
// reached from C9>0 must let C9 run to 31 and wrap, so no unconditional
// "limit is zero" match may short-circuit the comparison.
wire       line_last_w = (line == crtc1_line_max);
assign     line_last = line_last_w;
assign     line_new = hcc_last;

// ACCC v1.10 section 11.3.2: Type 1 adjustment ends when C5+1 reaches R5
// evaluated by equality at the line boundary. R5=0 never satisfies this comparison.
wire       crtc1_adj_end = CRTC_TYPE & in_adj & ({1'b0, c5} + 6'd1 == {1'b0, R5_v_total_adj}) & (|R5_v_total_adj);

assign line_next = ((line_last_w | crtc1_adj_end) ?
						 5'd0 : line + 1'd1 + interlace) & ~interlace;

// ACCC v1.10 section 11.1 specifies that CRTC 1 has a separate C5 counter
// for vertical adjustment, while C9 continues cycling 0..R9 and C4
// increments at each C9==R9 wrap.
always @(*) begin
	if(line_new) begin
		if(in_adj) begin
			if(crtc1_adj_end) c5_next = 5'd0;
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
wire       frame_adj_CRTC1 = row_last_w && ~in_adj && |R5_v_total_adj;
assign     frame_adj = frame_adj_CRTC1;
wire       crtc1_row_frame_last = in_adj ? crtc1_adj_end : (row_last_w & ~frame_adj_CRTC1);
assign     row_frame_last = crtc1_row_frame_last;
assign     row_next = row_frame_last ? 7'd0 : row + 1'd1;
assign     row_new = line_new & (line_last_w | crtc1_adj_end);

wire       frame_new_w = row_new & row_frame_last;

// Technical information sourced from ACCC v1.10 §11.2.4:
// If C4 was 0 immediately before adjustment began, VMA loads from R12/R13
// while C4==1 in adjustment.
wire crtc1_adj_entry_from_row0 = CRTC_TYPE & !in_adj & row_last_w & line_last_w & (|R5_v_total_adj) & (row == 0);
wire crtc1_adj_row1_reload = CRTC_TYPE & (crtc1_adj_entry_from_row0 | (in_adj & crtc1_adj_from_row0 & (row == 1) & ~line_last_w)) & !hcc_next;
wire crtc1_row0_reload = CRTC_TYPE & (frame_new_w | (~line_last_w & !row & !hcc_next));
assign reload = crtc1_row0_reload | crtc1_adj_row1_reload;

assign row_addr_save = hcc == R1_h_displayed && line_last_w;

assign field_count_tick = (hcc_next == {1'b0, R0_h_total[7:1]});

assign vsync_line_fire = (((CRTC_TYPE && in_adj) ? (row + 1'd1) : row_next) == R7_v_sync_pos && line_last_w);
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

		if(CLKEN && hcc_last) begin
			// A row transition can assert or clear the condition on this same
			// C0=R0 edge, so sample the resulting state rather than the old flop.
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
