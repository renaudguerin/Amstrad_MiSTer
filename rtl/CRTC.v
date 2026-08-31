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
				// R12/R13: readable on type 0 only, return 0 on type 1 (ACCC §21.2.2 vs §28.1.9; F18).
				// R14/R15: read/write cursor registers on both types (ACCC §21.2.2; F18).
				// R16/R17: light-pen registers; without LPSTB attached they return 0 (default).
				// R31: undefined dummy register, returns 0xFF on type 1, 0x00 on type 0 (§21.2.2, §28.1.9).
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

// Interlace parity state (ACCC v1.11 FR ch.19, F10; bilingual BL-038/IA-2).
// Shared flops preserve state across live CRTC_TYPE switches, as do the shared
// counters above; each type's engine contributes its own update rules.
// ParityFrame: type 1 toggles every frame at C4=C9=C0=0 regardless of R8
// (French section 19.5.3 p.209); type 0 snapshots ParityR6 at the frame origin
// (French section 19.5.2 pp.206-208).  French section 19.5.3 also requires
// ParityC9=ParityFrame at the type-1 frame origin; BL-038/IA-2 owns the directed
// even-R9 discriminator and any resulting behavior correction.  ParityC9 is bit
// 0 of the line value used for video address construction in IVM.  ParityR6 is
// the type-0-only companion latch: ParityFrame xor 1 captured when C4 reaches
// R6, independent of R8; frozen
// when R6>R4 (French section 19.5.2 pp.206-208).  The F10 per-type engines drive
// the update strobes and values for these shared flops below.
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
wire       e0_vsync_delay_suppress, e0_vsync_delay_half;
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
	e0_vsync_delay_suppress, e0_vsync_delay_half,
	e0_vsc_load, e0_r7_write_fire, e0_vsync_holdoff, e0_vde_toggle,
	e0_r6_vder_write, e0_r6_vder_value,
	e0_pf_write, e0_pf_value, e0_pc9_write, e0_pc9_value,
	e0_pr6_write, e0_pr6_value, e0_ivm_disp, e0_line_vma
);

wire       e1_line_last, e1_line_new, e1_row_last, e1_row_frame_last, e1_row_new, e1_frame_adj;
wire       e1_adj_from_row0;
wire       e1_reload, e1_row0_reload, e1_row_addr_save, e1_field_count_tick, e1_hsync_off;
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
	R5_v_total_adj, R6_v_displayed, R7_v_sync_pos, R8_interlace, R9_v_max_line,
	hcc, hcc_next, hcc_last, hcc_end, line, row, c5, in_adj, crtc1_adj_from_row0,
	VSYNC_r, vsync_allow, hsc, vde_r,
	parity_frame, parity_c9,
	e1_line_last, e1_line_new, e1_line_next, e1_c5_next,
	e1_row_last, e1_row_frame_last, e1_row_next, e1_row_new, e1_frame_adj,
	e1_adj_from_row0,
	e1_reload, e1_row0_reload, e1_row_addr_save,
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
// ACCC v1.10 section 20.3.2 p.242: the type-1 row-0 reload samples the
// register file as of AFTER the current edge -- the second CRTC-1
// chronogram draws an R12 write landing on the reload boundary edge itself
// caught (OFFSET=#30xx from C0=0) where the paired CRTC-0 chronogram
// (section 20.3.1) leaves the old offset.  Mirror the register block's
// snapshot/write priority so a same-edge write or SNA load participates.
wire       reg_data_write = ENABLE & ~nCS & ~R_nW & RS;
wire [5:0] r12_effective  = SNA_LOAD ? SNA_REGS[96 +: 6] :
                             (reg_data_write & (addr == 5'd12)) ? DI[5:0]
                                                                : R12_start_addr_h;
wire [7:0] r13_effective  = SNA_LOAD ? SNA_REGS[104 +: 8] :
                             (reg_data_write & (addr == 5'd13)) ? DI
                                                                : R13_start_addr_l;
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
		if(e1_row0_reload) begin
			row_addr_r <= {r12_effective, r13_effective};
		end
	end
end

// horizontal output
reg        hde;
reg  [3:0] hsc;
reg  [6:0] hsync_char_phase;
reg  [6:0] hsync_start_phase;
reg  [6:0] hsync_off_count;
reg        hsync_off_pending;
reg        hsync_phaseful;
reg        type1_hsync_start_pending;
reg  [1:0] type1_hsync_start_count;
reg        type0_hsync_restart_pending;
reg  [3:0] type0_hsync_restart_count;
reg        r2_jit_pending;
reg        register_write_d;

wire hsync_on = hcc == R2_h_sync_pos && R3_h_sync_width != 0;
wire hsync_off = CRTC_TYPE ? e1_hsync_off : e0_hsync_off;
wire register_write = ENABLE & RS & ~nCS & ~R_nW;
// ACCC v1.11 section 14.6.1 p.141: an OUT(C),r8 update which makes
// R2 equal to the current C0 is the R2.JIT event.  The bus write is a level
// for several master clocks in the integrated machine, so recognize only its
// first edge and let the updated comparator fire on the following edge.
wire r2_jit_write = register_write & ~register_write_d &
					(addr == 5'd02) & (hcc == DI) &
					(R2_h_sync_pos != DI) & (R3_h_sync_width != 0);
// ACCC v1.11 sections 15.3.2-15.3.3 pp.150-151: on type 0, changing
// R3l at the old terminal count while C0 is again equal to a relocated R2
// drops the current raw HSYNC but starts another one without resetting C3l.
// A zero low nibble retains the existing R3=0 suppression contract; changing
// only R3v is not an R3l modification.
wire type0_r3_terminal_write = register_write & ~register_write_d &
					!CRTC_TYPE & (addr == 5'd03) & HSYNC &
					(hcc == R2_h_sync_pos) & (hsc == R3_h_sync_width) &
					(DI[3:0] != 0) & (DI[3:0] != R3_h_sync_width);

always @(posedge CLOCK) begin

	if(~nRESET) begin
		hsc                       <= 0;
		hde                       <= 0;
		HSYNC                     <= 0;
		hsync_char_phase          <= 0;
		hsync_start_phase         <= 0;
		hsync_off_count           <= 0;
		hsync_off_pending         <= 0;
		hsync_phaseful            <= 0;
		type1_hsync_start_pending <= 0;
		type1_hsync_start_count   <= 0;
		type0_hsync_restart_pending <= 0;
		type0_hsync_restart_count   <= 0;
		r2_jit_pending            <= 0;
		register_write_d          <= 0;
	end
	else begin
		register_write_d <= register_write;
		if (CLKEN) hsync_char_phase <= 0;
		else       hsync_char_phase <= hsync_char_phase + 1'd1;

		if (r2_jit_write) r2_jit_pending <= 1;
		if (!CRTC_TYPE) type1_hsync_start_pending <= 0;
		// The p.151 earliest restart is approximately 3.5 Pixel-M2 after the
		// old pulse ends.  Reuse F20's production phase scale (four master
		// CLOCK ticks per Pixel-M2): count thirteen intervening ticks, then
		// raise the pin on the fourteenth.  Snapshot load and a live switch to
		// type 1 discard this type-0-only pending event.
		if (SNA_LOAD || CRTC_TYPE) begin
			type0_hsync_restart_pending <= 0;
			type0_hsync_restart_count   <= 0;
		end else if (type0_r3_terminal_write) begin
			type0_hsync_restart_pending <= 1;
			type0_hsync_restart_count   <= 4'd13;
		end

		// Through the GA's 16 MHz video sampler CRTC1's normal start is the
		// documented sixth rather than fifth pixel-M2. Four 64 MHz master edges
		// express that one-pixel phase in production; the integrated GA fixture
		// pins this production ratio. A JIT comparator hit starts immediately at
		// the write phase. A JIT pulse keeps only the ordinary type-specific
		// trailing-edge phase, not the later write phase: ACCC sections
		// 9.3.4.1/9.3.4.3 pp.53-57 state that R2.JIT removes the left part of
		// blanking without delaying display reactivation, shortening the physical
		// pulse by 4/3 pixel-M2 on type 0/1 respectively.
		if (hsync_off_pending) begin
			if (hsync_off_count == 0) begin
				HSYNC             <= 0;
				hsync_off_pending <= 0;
				hsync_phaseful    <= 0;
			end else begin
				hsync_off_count <= hsync_off_count - 1'd1;
			end
		end else if (hsync_off) begin
			if (HSYNC && hsync_phaseful && hsync_start_phase != 0) begin
				hsync_off_pending <= 1;
				hsync_off_count   <= hsync_start_phase - 1'd1;
			end else begin
				HSYNC          <= 0;
				hsync_phaseful <= 0;
			end
		end else if (!HSYNC) begin
			if (!SNA_LOAD && !CRTC_TYPE && type0_hsync_restart_pending) begin
				if (type0_hsync_restart_count == 0) begin
					HSYNC                       <= 1;
					hsync_phaseful              <= 0;
					hsync_start_phase           <= 0;
					type0_hsync_restart_pending <= 0;
				end else begin
					type0_hsync_restart_count <=
						type0_hsync_restart_count - 1'd1;
				end
			end else if (r2_jit_pending && hsync_on) begin
				HSYNC                     <= 1;
				hsync_phaseful            <= CRTC_TYPE;
				hsync_start_phase         <= CRTC_TYPE ? 7'd4 : 7'd0;
				r2_jit_pending            <= 0;
				type1_hsync_start_pending <= 0;
			end else if (CRTC_TYPE) begin
				if (type1_hsync_start_pending) begin
					if (type1_hsync_start_count == 0) begin
						HSYNC                     <= 1;
						hsync_phaseful            <= 1;
						hsync_start_phase         <= hsync_char_phase;
						type1_hsync_start_pending <= 0;
					end else begin
						type1_hsync_start_count <= type1_hsync_start_count - 1'd1;
					end
				end else if (hsync_on) begin
					type1_hsync_start_pending <= 1;
					type1_hsync_start_count   <= 3;
				end
			end else if (hsync_on) begin
				HSYNC          <= 1;
				hsync_phaseful <= 0;
			end
		end

		// A comparator opportunity that has passed cannot leak into a later
		// line or a live CRTC type switch.
		if (CLKEN && hcc != R2_h_sync_pos) r2_jit_pending <= 0;

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
// ACCC v1.11 French section 18.3.2 p.191: the C4=R6=C9=0 first-line
// alternation ends permanently when the live C0=R1 border condition is
// reached with R6 still zero. Keep that condition separate from vde so the
// half-character toggle cannot reassert DISPLAY ENABLE after the R1 edge.
reg type0_r6_zero_origin_border;
reg VSYNC_r;
reg vsync_allow;
// Section 19.5.3 p.208: during type-1 IVM the VSYNC start line is pinned by
// the row-structure rule on both frame parities (the table's boxes sit at
// the first line of C4=R7 whatever the frame parity), and the prose
// schedules the MID-VSYNC on the ParityFrame-even frame: that frame's pulse
// starts at the half-line tick (the p.207 type-0 Note words the same rule
// as "the VSYNC occurs in the middle of the line on C0 = R0/2"), the
// odd-parity frame's at the line seam. e1_vsync_line_fire is
// hcc-independent -- true across the whole last line of C4=R7-1 -- so the
// even-parity fire decision is latched at the seam and consumed at the
// half-line tick of the pulse's first line; consuming the level term
// mid-line would start the pulse a line early. The gate reads the raw R8
// mode rather than the engine's latched IVM state: around an R8 toggle
// write the two disagree for one to two characters, an unpinned window
// recorded in the F10 notes (review N-1, 2026-08-25).
wire       vsync_type1_ivm = CRTC_TYPE && interlace[0];
wire       vsync_ivm_mid   = vsync_type1_ivm && !parity_frame;
reg        vsync_ivm_arm;
wire vsync_count_tick = CLKEN && (
	vsync_ivm_mid   ? e1_field_count_tick :
	vsync_type1_ivm ? line_new :
	field           ? (CRTC_TYPE ? e1_field_count_tick : e0_field_count_tick) :
	line_new);
wire vsync_holdoff = e0_vsync_holdoff;
wire vsync_fire = vsync_allow & (
	vsync_ivm_mid   ? vsync_ivm_arm :
	vsync_type1_ivm ? e1_vsync_line_fire :
	field           ? (((row == R7_v_sync_pos && !line) &&
						!e0_vsync_delay_suppress) ||
				   e0_vsync_delay_half) :
	(CRTC_TYPE ? e1_vsync_line_fire : e0_vsync_line_fire));

// Seam-latched MID-VSYNC fire decision: set when the line now starting is
// the first line of C4=R7, consumed by the half-line fire on the
// ParityFrame-even frame (see above).
always @(posedge CLOCK) begin
	if(~nRESET) vsync_ivm_arm <= 0;
	else if(CLKEN) begin
		if(line_new) vsync_ivm_arm <= vsync_type1_ivm && e1_vsync_line_fire;
		else if(vsync_count_tick && vsync_ivm_arm) vsync_ivm_arm <= 0;
	end
end
wire [3:0] vsc_load = CRTC_TYPE ? e1_vsc_load : e0_vsc_load;
wire r7_write_hit = ENABLE & RS & ~nCS & ~R_nW & addr == 5'd07;
wire r7_write_fire = CRTC_TYPE ? e1_r7_write_fire : e0_r7_write_fire;
wire type0_r6_zero_origin_r1_hit = !CRTC_TYPE && !row_new &&
									(row == 0) && (line == 0) &&
									(R6_v_displayed == 0) &&
									(hcc_next == R1_h_displayed);

always @(posedge CLOCK) begin
	if(~nRESET | SNA_LOAD | CRTC_TYPE)
		type0_r6_zero_origin_border <= 0;
	else if(CLKEN) begin
		if(row_new)
			type0_r6_zero_origin_border <= 0;
		else if(type0_r6_zero_origin_r1_hit)
			type0_r6_zero_origin_border <= 1;
	end
end

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
		if (e0_vde_toggle && !type0_r6_zero_origin_border) begin
			vde <= ~vde;
			vde_r <= ~vde_r;
		end

		if(row_new) begin
			if((frame_new & row !=0) | row_next != row) vsync_allow <= 1;
			if(frame_new)                  begin vde <= 1; vde_r <= 1; end
			// ACCC v1.11 French section 18.3.2 p.191: on a type-0
			// frame-origin line with C4=R6=C9=0, DISPLAY ENABLE starts
			// high and falls at nCLKEN.  Do not let the ordinary R6
			// border assignment below overwrite frame_new's start value;
			// all non-origin matches, including type 1, keep the normal
			// priority.
			if(row_next == R6_v_displayed && !(frame_new && !CRTC_TYPE)) begin
				vde <= 0; vde_r <= 0;
			end
		end
		if(type0_r6_zero_origin_r1_hit) begin
			vde <= 0; vde_r <= 0;
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
		if (e0_vde_toggle && !type0_r6_zero_origin_border) begin
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

// DISPTMG delay line. ACCC v1.11 section 17.6.2 p.186 and section 19.2.4
// p.195: when type 0 cannot reach C0=R1 because R1>R0, its substituted
// border event occupies only the second half of C0=R0. nCLKEN marks that
// half-character phase and CLKEN ends it at the next C0 transition.
//
// The exact pulse remains ahead of the character-granular SKEW-DISPTMG
// stages. This is deliberate: p.195 shows delay 1/2 rounding the deferred
// event onto the full C0=0/C0=1 character respectively, while mode 2'b11
// suppresses DISPTMG entirely. Type 1 has no substituted event.
reg de_second_half;
always @(posedge CLOCK) begin
	if(~nRESET)       de_second_half <= 0;
	else if(CLKEN)    de_second_half <= 0;
	else if(nCLKEN)   de_second_half <= 1;
end

wire de_unskewed = hde & vde & vde_r &
					~(e0_spurious_border_off & de_second_half);
wire [3:0] de = {1'b0, dde[1:0], de_unskewed};
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
