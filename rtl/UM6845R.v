//============================================================================
//  UM6845R for Amstrad CPC
//  Copyright (C) 2018 Sorgelig
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//============================================================================

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

module UM6845R
(
	input            CLOCK,
	input            CLKEN,
	input            nCLKEN,
	input            nRESET,
	input            CRTC_TYPE,

	input            ENABLE,
	input            nCS,
	input            R_nW,
	input            RS,
	input      [7:0] DI,
	output reg [7:0] DO,

	input            SNA_LOAD,
	input      [4:0] SNA_ADDR,
	input    [143:0] SNA_REGS,
	
	output reg       VSYNC,
	output reg       HSYNC,
	output           DE,
	output           FIELD,
	output           CURSOR,

	output    [13:0] MA,
	output     [4:0] RA
);

/* verilator lint_off WIDTH */

assign FIELD = ~field & interlace[0];

assign MA = row_addr_r;
assign RA = line | (field & interlace[0]);

assign DE = de[R8_skew & ~{2{CRTC_TYPE}}];

reg [7:0] R0_h_total;
reg [7:0] R1_h_displayed;
reg [7:0] R2_h_sync_pos;
reg [3:0] R3_v_sync_width;
reg [3:0] R3_h_sync_width;
reg [6:0] R4_v_total;
reg [4:0] R5_v_total_adj;
reg [6:0] R6_v_displayed;
reg [6:0] R7_v_sync_pos;
reg [1:0] R8_skew;
reg [1:0] R8_interlace;
reg [4:0] R9_v_max_line;
reg [1:0] R10_cursor_mode;
reg [4:0] R10_cursor_start;
reg [4:0] R11_cursor_end;
reg [5:0] R12_start_addr_h;
reg [7:0] R13_start_addr_l;
reg [5:0] R14_cursor_h;
reg [7:0] R15_cursor_l;
reg       r6_border_condition;
reg       status_bit5;

reg [4:0] addr;
always @(*) begin
	DO = 8'hFF;
	if (ENABLE & ~nCS) begin
		if (RS) begin
			case (addr)
				// R10/R11 are not readable on CRTC types 0/1, only on 3/4 (ACCC v1.10 §21.2);
				// they remain writable and still drive the CURSOR output.
				12: DO = CRTC_TYPE ? 8'h00 : R12_start_addr_h;
				13: DO = CRTC_TYPE ? 8'h00 : R13_start_addr_l;
				14: DO = R14_cursor_h;
				15: DO = R15_cursor_l;
				31: DO = CRTC_TYPE ? 8'hFF : 8'h00;
			 default: DO = 0;
			endcase
		end
		else if(CRTC_TYPE) begin
			DO = {2'b00, status_bit5, 5'b00000}; // status for CRTC1
		end
	end
end

always @(posedge CLOCK) begin
	if (SNA_LOAD) begin
		addr <= SNA_ADDR;
		R0_h_total       <= SNA_REGS[  0 +: 8];
		R1_h_displayed   <= SNA_REGS[  8 +: 8];
		R2_h_sync_pos    <= SNA_REGS[ 16 +: 8];
		{R3_v_sync_width,R3_h_sync_width} <= SNA_REGS[24 +: 8];
		R4_v_total       <= SNA_REGS[ 32 +: 7];
		R5_v_total_adj   <= SNA_REGS[ 40 +: 5];
		R6_v_displayed   <= SNA_REGS[ 48 +: 7];
		R7_v_sync_pos    <= SNA_REGS[ 56 +: 7];
		{R8_skew, R8_interlace} <= {SNA_REGS[69:68], SNA_REGS[65:64]};
		R9_v_max_line    <= SNA_REGS[ 72 +: 5];
		{R10_cursor_mode,R10_cursor_start} <= SNA_REGS[86:80] & 7'h7f;
		R11_cursor_end   <= SNA_REGS[ 88 +: 5];
		R12_start_addr_h <= SNA_REGS[ 96 +: 6];
		R13_start_addr_l <= SNA_REGS[104 +: 8];
		R14_cursor_h     <= SNA_REGS[112 +: 6];
		R15_cursor_l     <= SNA_REGS[120 +: 8];
	end
	else if (ENABLE & ~nCS & ~R_nW) begin
		if (~RS) addr <= DI[4:0];
		else begin
			case (addr)
				00: R0_h_total <= DI;
				01: R1_h_displayed <= DI;
				02: R2_h_sync_pos <= DI;
				03: {R3_v_sync_width,R3_h_sync_width} <= DI;
				04: R4_v_total <= DI[6:0];
				05: R5_v_total_adj <= DI[4:0];
				06: R6_v_displayed <= DI[6:0];
				07: R7_v_sync_pos <= DI[6:0];
				08: {R8_skew, R8_interlace} <= {DI[5:4],DI[1:0]};
				09: R9_v_max_line <= DI[4:0];
				10: {R10_cursor_mode,R10_cursor_start} <= DI[6:0];
				11: R11_cursor_end <= DI[4:0];
				12: R12_start_addr_h <= DI[5:0];
				13: R13_start_addr_l <= DI[7:0];
				14: R14_cursor_h <= DI[5:0];
				15: R15_cursor_l <= DI[7:0];
			endcase
		end
	end
end

wire [4:0] interlace = &R8_interlace[1:0];

reg        in_adj;

// Technical information sourced from the "Amstrad CPC CRTC Compendium" by
// Longshot (CC BY-NC-ND). ACCC v1.10 section 11.2.2 specifies that an R4
// write on a type-0 last line from C0=2 through C0=R0 switches the line-end
// comparison from C9/R9 to C9/R5 before the next line is calculated.
reg        type0_r4_adjust_switch;
reg        type0_r9_live_compare;
reg        type0_r9_at_r0_pending;
reg        type0_c0_1_adjust;
reg        type0_r0_zero_entry_consumed;
reg        type0_zero_adj_entry;
reg        type0_r5_adjust_override;
reg  [4:0] type0_r5_adjust_target;
wire       register_write = ENABLE & ~nCS & ~R_nW & RS;
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

reg  [7:0] hcc;
wire       hcc_last  = hcc == R0_h_total;
wire [7:0] hcc_next  = hcc_last ? 8'h00 : hcc + 1'd1;
// Type 0 still compares C0 with R0 when both are zero, but that repeated
// equality pins C0 rather than completing a stream of one-character lines.
wire       r0_frozen = !CRTC_TYPE && !R0_h_total && !hcc;

reg  [4:0] line;
reg  [6:0] row;
reg  [4:0] c5;
reg        crtc1_adj_from_row0;

// Technical information sourced from the "Amstrad CPC CRTC Compendium" by
// Longshot (CC BY-NC-ND). ACCC v1.10 section 11.1 specifies that CRTC 1 has a
// separate C5 counter for vertical adjustment, while C9 continues cycling 0..R9
// and C4 increments at each C9==R9 wrap. CRTC 0 reuses C9 against R5.
wire [4:0] crtc1_line_max = R9_v_max_line & ~interlace;
wire [4:0] crtc0_line_max = (in_adj ? (R5_v_total_adj - 1'd1) : R9_v_max_line) & ~interlace;
wire [4:0] line_max       = CRTC_TYPE ? crtc1_line_max : crtc0_line_max;
reg        line_last_r;
reg        row_last_r;
// ACCC v1.10 section 10.3: C9 uses equality, never magnitude.  A zero limit
// reached from C9>0 must let C9 run to 31 and wrap, so no unconditional
// "limit is zero" match may short-circuit the comparison.
wire       line_last = (line == line_max);
wire [4:0] type0_r5_adjust_target_effective = type0_r5_window_write ?
											(DI[4:0] - 1'd1) : type0_r5_adjust_target;
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

// ACCC v1.10 section 11.3.2: Type 1 adjustment ends when C5+1 reaches R5
// evaluated by equality at the line boundary. R5=0 never satisfies this comparison.
wire       crtc1_adj_end = CRTC_TYPE & in_adj & ({1'b0, c5} + 6'd1 == {1'b0, R5_v_total_adj}) & (|R5_v_total_adj);

wire [4:0] line_next = ((CRTC_TYPE ? (line_last | crtc1_adj_end) : type0_rollover_line_last) ?
						 5'd0 : line + 1'd1 + interlace) & ~interlace;
wire       line_new  = hcc_last && !r0_frozen;

// ACCC v1.10 section 12: outside vertical adjustment C4 is equality-compared
// too.  Type 0's R4=0 frame end comes from the `Last Line` / adjustment
// arbitration below, not from a magnitude special case.
wire       row_last  = (row == R4_v_total);
wire       crtc1_row_frame_last = in_adj ? crtc1_adj_end : (row_last & ~frame_adj_CRTC1);
wire       crtc0_row_frame_last = (row_last_r | in_adj) & ~frame_adj_CRTC0;
wire       row_frame_last = CRTC_TYPE ? crtc1_row_frame_last : crtc0_row_frame_last;
wire [6:0] row_next  = row_frame_last ? 7'd0 : row + 1'd1;
wire       crtc1_row_new = line_new & (line_last | crtc1_adj_end);
wire       crtc0_row_new = line_new & type0_rollover_row_last;
wire       row_new   = CRTC_TYPE ? crtc1_row_new : crtc0_row_new;

reg        frame_adj_r;
wire       type0_r5_write = !CRTC_TYPE && register_write && addr == 5'd05;
wire [4:0] type0_effective_r5 = type0_r5_write ? DI[4:0] : R5_v_total_adj;
wire       type0_r5_window_write = type0_r5_write && hcc <= 2 && in_adj;
wire       type0_r5_override_active = (type0_r5_adjust_override | type0_r5_window_write) & in_adj;
wire       type0_zero_adj_entry_active = type0_zero_adj_entry &
										 ~(type0_r5_write && (hcc <= 2) && (|DI[4:0]));
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
wire       frame_adj_CRTC0 = type0_adjustment_selected;
wire       frame_adj_CRTC1 = row_last && ~in_adj && |R5_v_total_adj;
wire       frame_adj = CRTC_TYPE ? frame_adj_CRTC1 : frame_adj_CRTC0;
wire       frame_new = row_new & row_frame_last;
wire       type0_r4_at_c0_write = !CRTC_TYPE && register_write && addr == 5'd04 && hcc == 0;
wire       type0_r9_at_c0_write = !CRTC_TYPE && register_write && addr == 5'd09 && hcc == 0;
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
		if(!r0_frozen) type0_r0_zero_entry_consumed <= 0;
		else if(CLKEN) type0_r0_zero_entry_consumed <= 1;
		if(type0_r5_write && (hcc <= 2) && (|DI[4:0])) type0_zero_adj_entry <= 0;

		if(CLKEN) begin
			if(line_new && (type0_r4_switch_active | type0_r9_compare_active |
							type0_c0_1_adjust_active))
				type0_zero_adj_entry <= type0_c0_1_adjust_active & !(|type0_effective_r5);
			else if(r0_frozen && !in_adj && !type0_r0_zero_entry_consumed &&
					line == R9_v_max_line && row == R4_v_total)
				type0_zero_adj_entry <= !(|R5_v_total_adj);
			else if(row_new) begin
				if(frame_adj)
					type0_zero_adj_entry <= !(|type0_effective_r5);
				else if(frame_new)
					type0_zero_adj_entry <= 0;
			end
		end
	end
end

// counters
reg  field;
always @(posedge CLOCK) begin
	if(~nRESET) begin
		hcc    <= 0;
		line   <= 0;
		row    <= 0;
		c5     <= 0;
		in_adj <= 0;
		field  <= 0;
		crtc1_adj_from_row0 <= 0;
	end
	else if(CLKEN) begin
		hcc <= hcc_next;
		if(line_new) line <= line_next;
		if(CRTC_TYPE) begin
			if(line_new) begin
				if(in_adj) begin
					if(crtc1_adj_end) c5 <= 0;
					else c5 <= c5 + 1'd1;
				end
				else c5 <= 0;
			end
		end else begin
			c5 <= 0;
		end
		if(hcc == 0 && !r0_frozen) begin
			line_last_r <= CRTC_TYPE ? line_last : type0_c0_line_last;
			row_last_r <= CRTC_TYPE ? row_last : type0_c0_row_last;
			frame_adj_r <= (CRTC_TYPE ? (line_last & row_last) :
									 (type0_c0_line_last & type0_c0_row_last)) & ~in_adj;
		end
		// CRTC0 always schedule the adjustment run at HCC=0,
		// then at HCC=2 it decides that it really has to run
		if(hcc == 2) frame_adj_r <= frame_adj_r & |type0_effective_r5;
		if(line_new && !CRTC_TYPE && (type0_r4_switch_active | type0_r9_compare_active |
									 type0_c0_1_adjust_active))
			in_adj <= 1;
		// ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.6: on the first
		// repeated C0=0, C9 freezes but a matching C9/R9 consumes the
		// already-armed C4 increment exactly once. A simultaneous C4/R4
		// match enters the short-R0 default adjustment route.
		if(!CRTC_TYPE && r0_frozen && !in_adj &&
			!type0_r0_zero_entry_consumed &&
			line == R9_v_max_line) begin
			row <= row + 1'd1;
			if(row == R4_v_total) in_adj <= 1;
		end

		if(row_new) begin
			row <= row_next;
			if(frame_adj) begin
				in_adj <= 1;
				if(CRTC_TYPE && row == 0) crtc1_adj_from_row0 <= 1;
			end
			else if(frame_new) begin
				in_adj <= 0;
				row <= 0;
				field <= ~field & R8_interlace[0];
				crtc1_adj_from_row0 <= 0;
			end
			else if(CRTC_TYPE && in_adj && row_next != 1) begin
				crtc1_adj_from_row0 <= 0;
			end
		end
	end
end

// Technical information sourced from ACCC v1.10 §11.2.4:
// If C4 was 0 immediately before adjustment began, VMA loads from R12/R13
// while C4==1 in adjustment.
wire crtc1_adj_entry_from_row0 = CRTC_TYPE & !in_adj & row_last & line_last & (|R5_v_total_adj) & (row == 0);
wire crtc1_adj_row1_reload = CRTC_TYPE & (crtc1_adj_entry_from_row0 | (in_adj & crtc1_adj_from_row0 & (row == 1) & ~line_last)) & !hcc_next;
wire crtc1_row0_reload = CRTC_TYPE & (frame_new | (~line_last & !row & !hcc_next));
wire CRTC1_reload = crtc1_row0_reload | crtc1_adj_row1_reload;
wire CRTC0_reload = ~CRTC_TYPE & frame_new;
wire row_addr_save = hcc == R1_h_displayed && (CRTC_TYPE ? line_last : type0_live_line_last);

// address
reg  [13:0] row_addr;   // saved pointer
reg  [13:0] row_addr_r; // current pointer
always @(posedge CLOCK) begin
	if(CLKEN) begin
		if(row_addr_save) row_addr <= row_addr_r; // save current pointer

		if(line_new & !row_addr_save) row_addr_r <= row_addr; // restore the pointer, take care of simultaneous saving and restoring
		if(!hcc_last)                 row_addr_r <= row_addr_r + 1'd1;

		if(CRTC0_reload) begin
			row_addr <= {R12_start_addr_h, R13_start_addr_l};
			row_addr_r <= {R12_start_addr_h, R13_start_addr_l};
		end
		if(CRTC1_reload) begin
			row_addr_r <= {R12_start_addr_h, R13_start_addr_l};
		end
	end
end

// horizontal output
reg        hde;
reg  [3:0] hsc;

wire hsync_on = hcc == R2_h_sync_pos && R3_h_sync_width != 0;
wire hsync_off = (hsc == R3_h_sync_width) || (CRTC_TYPE && R3_h_sync_width == 0);

always @(posedge CLOCK) begin

	if(~nRESET) begin
		hsc    <= 0;
		hde    <= 0;
		HSYNC  <= 0;
	end
	else begin
		// should be a half char delay (other edge of the clock?)
		if (hsync_off)     HSYNC <= 0;
		else if (hsync_on) HSYNC <= 1;

		if (ENABLE & RS & ~nCS & ~R_nW & addr == 5'd01 & hcc == DI) hde <= 0;

		if (CLKEN) begin
			if(line_new)                   hde <= 1;
			if(hcc_next == R1_h_displayed) hde <= 0;

			if(HSYNC) hsc <= hsc + 1'd1;
			else hsc <= 0;
		end
	end
end

// vertical output
reg vde, vde_r;
reg VSYNC_r;
wire vsync_count_tick = CLKEN &&
	(field ? (!r0_frozen && (hcc_next == {1'b0, R0_h_total[7:1]})) : line_new);
always @(posedge CLOCK) VSYNC <= VSYNC_r; // delay the same as HSYNC to not confuse the GA
always @(posedge CLOCK) begin
	reg  [3:0] vsc;
	reg        vsync_allow;
	reg        type0_vsync_wait_line_start;

	if(~nRESET) begin
		vsc    <= 0;
		vde    <= 0;
		vde_r  <= 0;
		VSYNC_r<= 0;
		vsync_allow <= 1;
		type0_vsync_wait_line_start <= 0;
	end
	else if (CLKEN) begin
		if (!CRTC_TYPE && row == 0 && line == 0 && R6_v_displayed == 0) begin
			vde <= ~vde;
			vde_r <= ~vde_r;
		end

		if(row_new) begin
			if((frame_new & row !=0) | row_next != row) vsync_allow <= 1;
			if(frame_new)                  begin vde <= 1; vde_r <= 1; end
			if(row_next == R6_v_displayed) begin vde <= 0; vde_r <= 0; end
		end
		if(vsync_count_tick) begin
			// A type 0 VSYNC started by an R7=C4 write after C0=1
			// does not count its partial first line.  Preserve C3h at the
			// first following type-specific count tick.
			if(!CRTC_TYPE && type0_vsync_wait_line_start)
				type0_vsync_wait_line_start <= 0;
			else if(vsc) vsc <= vsc - 1'd1;
			else if (vsync_allow & (field ? (row == R7_v_sync_pos && !line) : (((CRTC_TYPE && in_adj) ? (row + 1'd1) : row_next) == R7_v_sync_pos && line_last))) begin
				VSYNC_r <= 1;
				// Don't allow a new VSYNC until C4=R7 has become false and true again.
				vsync_allow <= 0;
				vsc <= (CRTC_TYPE ? 4'd0 : R3_v_sync_width) - 1'd1;
			end
			else VSYNC_r <= 0;
		end
	end
	else if (nCLKEN) begin
		if (!CRTC_TYPE && row == 0 && line == 0 && R6_v_displayed == 0) begin
			vde <= ~vde;
			vde_r <= ~vde_r;
		end
	end

	if (ENABLE & RS & ~nCS & ~R_nW & addr == 5'd07) begin
		if(row != DI[6:0]) begin
			// A false comparison re-arms the next genuine C4=R7 match.  It
			// does not alter a VSYNC already in progress.
			vsync_allow <= 1;
		end
		else begin
			// An equal comparison is consumed even when type 0 blocks the
			// pulse at C0=0/1.  Qualifying with the old allow state also makes
			// a bus write held for several clocks one comparison, not many.
			vsync_allow <= 0;
			if(!VSYNC_r && vsync_allow && (CRTC_TYPE || hcc > 1)) begin
				VSYNC_r <= 1;
				vsc <= (CRTC_TYPE ? 4'd0 : R3_v_sync_width) - 1'd1;
				type0_vsync_wait_line_start <= !CRTC_TYPE && !vsync_count_tick;
			end else if(!VSYNC_r) begin
				type0_vsync_wait_line_start <= 0;
			end
		end
	end

	// The type input is live and snapshots do not restore this derived latch.
	// Never carry a pending type 0 partial-line state through either boundary.
	if(CRTC_TYPE || SNA_LOAD) type0_vsync_wait_line_start <= 0;
	if (nCLKEN & ENABLE & RS & ~nCS & ~R_nW & addr == 5'd06) begin
		if (CRTC_TYPE) begin
			if (row == DI[6:0]) vde_r <= 0;
			if (row != DI[6:0] && DI[6:0] != 0) vde <= vde_r;
			if (row == R6_v_displayed && DI[6:0] != row) vde <= 1;
			if (row == DI[6:0] || DI[6:0] == 0) vde <= 0;
		end else begin
			if (row == DI[6:0] && !(row == 0 && line == 0)) vde_r <= 0;
		end
	end
end

// Type 1 status bit 5 is a line-sampled view of the sticky C4=R6 border
// condition.  Keep that condition separate from vde: writing R6=0 while
// C4>0 also forces vde low, but is not a C4=R6 match and must not set status.
always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD) begin
		r6_border_condition <= 0;
		status_bit5 <= 0;
	end
	else if(CRTC_TYPE) begin
		if(CLKEN && row_new) begin
			// C4=C9=C0=0 is explicitly outside the R6-border condition,
			// including the otherwise-equal R6=0 case at frame origin.
			if(frame_new) r6_border_condition <= 0;
			else if(row_next == R6_v_displayed) r6_border_condition <= 1;
		end

		if(nCLKEN & ENABLE & RS & ~nCS & ~R_nW & addr == 5'd06 &
		   row == DI[6:0]) begin
			r6_border_condition <= 1;
		end

		if(CLKEN && hcc_last) begin
			// A row transition can assert or clear the condition on this same
			// C0=R0 edge, so sample the resulting state rather than the old flop.
			if(row_new && frame_new) status_bit5 <= 0;
			else if(row_new && row_next == R6_v_displayed) status_bit5 <= 1;
			else status_bit5 <= r6_border_condition;
		end
	end
	else begin
		// The type input is live.  Do not preserve hidden type-1 status when
		// the model runs as type 0 and is later switched back to type 1.
		r6_border_condition <= 0;
		status_bit5 <= 0;
	end
end

wire [3:0] de = {1'b0, dde[1:0], hde & vde & vde_r};
reg  [1:0] dde;
always @(posedge CLOCK) if (CLKEN) dde <= {dde[0],de[0]};

// Cursor control
reg cursor_line;
assign CURSOR = hde & vde & MA == {R14_cursor_h, R15_cursor_l} & cursor_line;

always @(posedge CLOCK) begin

	if(~nRESET) begin
		cursor_line <= 0;
	end
	else if (CLKEN) begin
		if (line == R10_cursor_start)
			cursor_line <= 1;
		else if (line == R11_cursor_end)
			cursor_line <= 0;
		end
	end

endmodule
