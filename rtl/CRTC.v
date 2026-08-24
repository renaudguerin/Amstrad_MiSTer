//============================================================================
//  Classic Amstrad CPC CRTC (HD6845S / UM6845R, selected by CRTC_TYPE)
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
//
//  Port-compatible wrapper around the two per-type counter engines
//  (crtc_type0_engine / crtc_type1_engine). The motherboard wiring and every
//  external pin are unchanged. The wrapper is named for the component, not
//  one variant: the socket takes a type-0 HD6845S/UM6845 or a type-1
//  UM6845R depending on model (the file was rtl/UM6845R.v before the
//  per-type split; the old name survives only in history).
//
//  Why one set of shared flops rather than two stateful engines: CRTC_TYPE is
//  a live input (snapshot model selection), and the required vectors pin the
//  round-trip contract that a type switch continues counting from the very
//  state the previous type left behind (t02j/t06d/t09f/t16l). A physical
//  CRTC's counters exist once regardless of which variant is emulated, so the
//  wrapper owns the register file, the shared counters, and the sequencing,
//  while each engine module owns everything type-specific: its rule
//  expressions, its private latches (arbitration cluster, partial-VSYNC
//  holdoff, status bit 5), and its shares of the sync/display outputs.

module CRTC
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

// Type 1 carries the IVM line parity in C9 itself (ACCC v1.10 section
// 19.8.2: no separate C9.VMA concept), so RA is the raw counter.  Type 0
// forms the split C9.VMA of section 19.8.1 on lines that started with IVM
// active; the old field-OR approximation is gone.
assign RA = CRTC_TYPE ? line : (e0_ivm_disp ? e0_line_vma : line);

assign DE = de[CRTC_TYPE ? e1_de_index : e0_de_index];

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
wire      status_bit5;

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

reg  [7:0] hcc;
wire       hcc_last  = hcc == R0_h_total;
// ACCC v1.10 section 13.7.1.2 p.124: a type-1 R0-widening write landing on
// the C0==R0 comparator edge of the frame's last line defers that line end
// (the type-1 engine raises rfd_r0_extend for that one edge), so the line
// runs on into the widened remainder and ends at the new total instead.
// hcc_end is the effective line-end strobe every line-event consumer uses;
// raw hcc_last stays for terms whose documented semantics are tied to the
// comparator edge itself.
wire       hcc_end   = hcc_last & ~(CRTC_TYPE & e1_rfd_r0_extend);
wire [7:0] hcc_next  = hcc_end ? 8'h00 : hcc + 1'd1;

reg  [4:0] line;
reg  [6:0] row;
reg  [4:0] c5;
reg        crtc1_adj_from_row0;
reg        line_last_r;
reg        row_last_r;
reg        frame_adj_r;
reg        field;

// Interlace parity state (ACCC v1.10 ch.19, F10).  Shared flops because a
// live CRTC_TYPE switch must continue from the same state, exactly like the
// shared counters above; each type's engine contributes its own update rules.
// ParityFrame: type 1 toggles every frame at C4=C9=C0=0 regardless of R8
// (section 19.5.3 p.208); type 0 snapshots ParityR6 at the frame origin
// (section 19.5.2 p.205).  ParityC9: bit 0 of the line value used for video
// address construction in IVM.  ParityR6: type-0-only companion latch,
// ParityFrame xor 1 captured when C4 reaches R6, independent of R8; frozen
// when R6>R4 (section 19.5.2 p.205).  F10 fixture stage: the flops exist so
// the deterministic vectors can address them; their update rules land with
// the per-type F10 behavior commits and until then they hold reset values.
reg        parity_frame;
reg        parity_c9;
reg        parity_r6;

// ------------------------------------------------------------------
// Per-type engines
// ------------------------------------------------------------------
wire       e0_r0_frozen, e0_line_new, e0_row_frame_last, e0_row_new, e0_frame_adj;
wire       e0_c0_line_last, e0_c0_row_last, e0_in_adj_route, e0_frozen_row_advance;
wire       e0_hcc2_adj_keep, e0_reload, e0_row_addr_save, e0_field_count_tick;
wire       e0_hsync_off, e0_vsync_line_fire, e0_r7_write_fire, e0_vsync_holdoff;
wire       e0_vde_toggle, e0_r6_vder_write, e0_r6_vder_value;
wire       e0_pf_write, e0_pf_value, e0_pc9_write, e0_pc9_value;
wire       e0_pr6_write, e0_pr6_value, e0_ivm_disp;
wire [4:0] e0_line_vma;
wire [4:0] e0_line_next, e0_c5_next;
wire [6:0] e0_row_next;
wire [1:0] e0_de_index;
wire       e0_spurious_border_off;
wire [3:0] e0_vsc_load;

crtc_type0_engine crtc_type0_engine
(
	CLOCK, CLKEN, nRESET, CRTC_TYPE, SNA_LOAD,
	ENABLE, nCS, R_nW, RS, DI, addr,
	R0_h_total, R1_h_displayed, R3_h_sync_width, R3_v_sync_width,
	R4_v_total, R5_v_total_adj, R6_v_displayed, R7_v_sync_pos,
	R8_skew, R8_interlace, R9_v_max_line,
	hcc, hcc_next, hcc_last, line, row, in_adj, field,
	line_last_r, row_last_r, frame_adj_r,
	VSYNC_r, vsync_allow, hsc,
	parity_frame, parity_c9, parity_r6,
	e0_r0_frozen, e0_line_new, e0_line_next, e0_c5_next,
	e0_row_frame_last, e0_row_next, e0_row_new, e0_frame_adj,
	e0_c0_line_last, e0_c0_row_last,
	e0_in_adj_route, e0_frozen_row_advance, e0_hcc2_adj_keep,
	e0_reload, e0_row_addr_save,
	e0_field_count_tick, e0_hsync_off, e0_de_index, e0_spurious_border_off,
	e0_vsync_line_fire,
	e0_vsc_load, e0_r7_write_fire, e0_vsync_holdoff, e0_vde_toggle,
	e0_r6_vder_write, e0_r6_vder_value,
	e0_pf_write, e0_pf_value, e0_pc9_write, e0_pc9_value,
	e0_pr6_write, e0_pr6_value, e0_ivm_disp, e0_line_vma
);

wire       e1_line_last, e1_line_new, e1_row_last, e1_row_frame_last, e1_row_new, e1_frame_adj;
wire       e1_adj_from_row0;
wire       e1_reload, e1_row_addr_save, e1_field_count_tick, e1_hsync_off;
wire       e1_vsync_line_fire, e1_r7_write_fire, e1_r6_vde_write, e1_r6_vde_value;
wire       e1_r6_vder_write, e1_r6_vder_value, e1_status_bit5;
wire       e1_rfd_r0_extend;
wire       e1_pf_write, e1_pf_value, e1_pc9_write, e1_pc9_value;
wire       e1_line_poke, e1_line_poke_bit;
wire [4:0] e1_line_next, e1_c5_next;
wire [6:0] e1_row_next;
wire [1:0] e1_de_index;
wire [3:0] e1_vsc_load;

crtc_type1_engine crtc_type1_engine
(
	CLOCK, CLKEN, nCLKEN, nRESET, CRTC_TYPE, SNA_LOAD,
	ENABLE, nCS, R_nW, RS, DI, addr,
	R0_h_total, R1_h_displayed, R3_h_sync_width, R4_v_total,
	R5_v_total_adj, R6_v_displayed, R7_v_sync_pos, R9_v_max_line,
	hcc, hcc_next, hcc_last, hcc_end, line, row, c5, in_adj, crtc1_adj_from_row0,
	VSYNC_r, vsync_allow, hsc, vde_r,
	parity_frame, parity_c9,
	e1_line_last, e1_line_new, e1_line_next, e1_c5_next,
	e1_row_last, e1_row_frame_last, e1_row_next, e1_row_new, e1_frame_adj,
	e1_adj_from_row0,
	e1_reload, e1_row_addr_save,
	e1_field_count_tick, e1_hsync_off, e1_de_index, e1_vsync_line_fire,
	e1_vsc_load, e1_r7_write_fire,
	e1_r6_vde_write, e1_r6_vde_value, e1_r6_vder_write, e1_r6_vder_value,
	e1_rfd_r0_extend,
	e1_pf_write, e1_pf_value, e1_pc9_write, e1_pc9_value,
	e1_line_poke, e1_line_poke_bit,
	e1_status_bit5
);

assign status_bit5 = e1_status_bit5;

// ------------------------------------------------------------------
// Type-multiplexed contributions (the only place the two engines meet)
// ------------------------------------------------------------------
wire       r0_frozen      = CRTC_TYPE ? 1'b0          : e0_r0_frozen;
wire       line_new       = CRTC_TYPE ? e1_line_new   : e0_line_new;
wire [4:0] line_next      = CRTC_TYPE ? e1_line_next  : e0_line_next;
wire [4:0] c5_next        = CRTC_TYPE ? e1_c5_next    : e0_c5_next;
wire       row_frame_last = CRTC_TYPE ? e1_row_frame_last : e0_row_frame_last;
wire [6:0] row_next       = CRTC_TYPE ? e1_row_next   : e0_row_next;
wire       row_new        = CRTC_TYPE ? e1_row_new    : e0_row_new;
wire       frame_adj      = CRTC_TYPE ? e1_frame_adj  : e0_frame_adj;
wire       frame_new      = row_new & row_frame_last;

wire       crtc0_reload     = e0_reload;
wire       crtc1_reload     = e1_reload;
wire       row_addr_save    = CRTC_TYPE ? e1_row_addr_save : e0_row_addr_save;

// counters
always @(posedge CLOCK) begin
	if(~nRESET) begin
		hcc    <= 0;
		line   <= 0;
		row    <= 0;
		c5     <= 0;
		in_adj <= 0;
		field  <= 0;
		crtc1_adj_from_row0 <= 0;
		parity_frame <= 0;
		parity_c9    <= 0;
		parity_r6    <= 0;
	end
	else if(CLKEN) begin
		hcc <= hcc_next;
		if(line_new) line <= line_next;
		// F10 type-1 (ACCC v1.10 section 19.5.3 pp.208-209): the R8-toggle
		// stage edges write C9's bit 0 mid-line -- stage A plants the new
		// parity, stage B plants the settled value.  A same-edge row end
		// keeps the documented restart value (line_new wins; that
		// coincidence is itself unpinned in the source).
		else if(CRTC_TYPE && e1_line_poke)
			line <= {line[4:1], e1_line_poke_bit};
		c5 <= c5_next;
		if(hcc == 0 && !r0_frozen) begin
			line_last_r <= CRTC_TYPE ? e1_line_last : e0_c0_line_last;
			row_last_r <= CRTC_TYPE ? e1_row_last : e0_c0_row_last;
			frame_adj_r <= (CRTC_TYPE ? (e1_line_last & e1_row_last) :
									 (e0_c0_line_last & e0_c0_row_last)) & ~in_adj;
		end
		// CRTC0 always schedule the adjustment run at HCC=0,
		// then at HCC=2 it decides that it really has to run
		if(hcc == 2) frame_adj_r <= frame_adj_r & e0_hcc2_adj_keep;
		if(e0_in_adj_route)
			in_adj <= 1;
		// ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.6: on the first
		// repeated C0=0, C9 freezes but a matching C9/R9 consumes the
		// already-armed C4 increment exactly once. A simultaneous C4/R4
		// match enters the short-R0 default adjustment route.
		if(e0_frozen_row_advance) begin
			row <= row + 1'd1;
			if(row == R4_v_total) in_adj <= 1;
		end

		if(row_new) begin
			row <= row_next;
			if(frame_adj) begin
				in_adj <= 1;
				if(e1_adj_from_row0) crtc1_adj_from_row0 <= 1;
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

		// F10 parity updates: each engine decides for its type (type 1:
		// section 19.5.3; type 0: section 19.5.2); the shared flops live
		// here so a live CRTC_TYPE switch continues from the same state.
		if(CRTC_TYPE) begin
			if(e1_pf_write)  parity_frame <= e1_pf_value;
			if(e1_pc9_write) parity_c9    <= e1_pc9_value;
		end
		else begin
			if(e0_pf_write)  parity_frame <= e0_pf_value;
			if(e0_pc9_write) parity_c9    <= e0_pc9_value;
			if(e0_pr6_write) parity_r6    <= e0_pr6_value;
		end
	end
end

// address
reg  [13:0] row_addr;   // saved pointer
reg  [13:0] row_addr_r; // current pointer
always @(posedge CLOCK) begin
	if(CLKEN) begin
		if(row_addr_save) row_addr <= row_addr_r; // save current pointer

		if(line_new & !row_addr_save) row_addr_r <= row_addr; // restore the pointer, take care of simultaneous saving and restoring
		if(!hcc_end)                  row_addr_r <= row_addr_r + 1'd1;

		if(crtc0_reload) begin
			row_addr <= {R12_start_addr_h, R13_start_addr_l};
			row_addr_r <= {R12_start_addr_h, R13_start_addr_l};
		end
		if(crtc1_reload) begin
			row_addr_r <= {R12_start_addr_h, R13_start_addr_l};
		end
	end
end

// horizontal output
reg        hde;
reg  [3:0] hsc;

wire hsync_on = hcc == R2_h_sync_pos && R3_h_sync_width != 0;
wire hsync_off = CRTC_TYPE ? e1_hsync_off : e0_hsync_off;

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
			// hcc_next carries the post-mux continuation value at a
			// section 13.7.1.2 suppressed-wrap edge, so this comparison
			// sees the extended line's genuine roll-into-R1 display end
			// (ACCC v1.10 section 6.1.3 p.33) with no special case.
			if(hcc_next == R1_h_displayed) hde <= 0;

			if(HSYNC) hsc <= hsc + 1'd1;
			else hsc <= 0;
		end
	end
end

// vertical output
reg vde, vde_r;
reg VSYNC_r;
reg vsync_allow;
wire vsync_count_tick = CLKEN &&
	(field ? (CRTC_TYPE ? e1_field_count_tick : e0_field_count_tick) : line_new);
wire vsync_holdoff = e0_vsync_holdoff;
wire vsync_fire = vsync_allow &
	(field ? (row == R7_v_sync_pos && !line) :
			 (CRTC_TYPE ? e1_vsync_line_fire : e0_vsync_line_fire));
wire [3:0] vsc_load = CRTC_TYPE ? e1_vsc_load : e0_vsc_load;
wire r7_write_hit = ENABLE & RS & ~nCS & ~R_nW & addr == 5'd07;
wire r7_write_fire = CRTC_TYPE ? e1_r7_write_fire : e0_r7_write_fire;

always @(posedge CLOCK) VSYNC <= VSYNC_r; // delay the same as HSYNC to not confuse the GA
always @(posedge CLOCK) begin
	reg  [3:0] vsc;

	if(~nRESET) begin
		vsc    <= 0;
		vde    <= 0;
		vde_r  <= 0;
		VSYNC_r<= 0;
		vsync_allow <= 1;
	end
	else if (CLKEN) begin
		if (e0_vde_toggle) begin
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
			// first following type-specific count tick; the engine owns the
			// holdoff latch and clears it on this tick itself.
			if(vsync_holdoff) ;
			else if(vsc) vsc <= vsc - 1'd1;
			else if (vsync_fire) begin
				VSYNC_r <= 1;
				// Don't allow a new VSYNC until C4=R7 has become false and true again.
				vsync_allow <= 0;
				vsc <= vsc_load;
			end
			else VSYNC_r <= 0;
		end
	end
	else if (nCLKEN) begin
		if (e0_vde_toggle) begin
			vde <= ~vde;
			vde_r <= ~vde_r;
		end
	end

	if (r7_write_hit) begin
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
			if(r7_write_fire) begin
				VSYNC_r <= 1;
				vsc <= vsc_load;
			end
		end
	end

	if (nCLKEN & ENABLE & RS & ~nCS & ~R_nW & addr == 5'd06) begin
		if (CRTC_TYPE) begin
			if (e1_r6_vder_write) vde_r <= e1_r6_vder_value;
			if (e1_r6_vde_write)  vde   <= e1_r6_vde_value;
		end else begin
			if (e0_r6_vder_write) vde_r <= e0_r6_vder_value;
		end
	end
end

// DISPTMG delay line. The type-0 spurious-border term (ACCC v1.10 section
// 17.6.2 p.186, substituted border start for R1>R0) is injected here,
// ahead of the SKEW-DISPTMG stages, so a programmed delay displaces it
// like a natural border edge and mode 2'b11 suppresses it (section
// 19.2.4). The term is already gated on !CRTC_TYPE in the engine: type 1
// has no border-start substitution at all (ACCC p.186-187).
wire [3:0] de = {1'b0, dde[1:0], hde & vde & vde_r & ~e0_spurious_border_off};
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
