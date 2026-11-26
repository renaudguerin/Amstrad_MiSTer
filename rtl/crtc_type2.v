//============================================================================
//  Motorola MC6845 (Amstrad CPC CRTC type 2)
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
//  Amstrad CPC CRTC Compendium v1.11 (French), Serge Querne (Longshot / Logon System),
//  https://shaker.logonsystem.eu -- licensed CC BY-NC-ND 4.0. Its attribution
//  directive requires this notice in the source of CRTC emulation modules and
//  in the credits of any distributed product built from them. Individual rules
//  cite their ACCC section at the point of implementation.
//============================================================================

// Progressive timing, IVM addressing and natural MID-VSYNC. Interlace
// extra lines, cursor output and the sub-character R0/R2/R3 and VSYNC
// aperture need separate qualification.
module crtc_type2 (
	input            CLOCK,
	input            CLKEN,
	input            nRESET,
	input            ENABLE,
	input            nCS,
	input            R_nW,
	input            RS,
	input      [7:0] DI,
	output     [7:0] DO,
	output reg       HSYNC,
	output reg       VSYNC,
	output           DE,
	output    [13:0] MA,
	output     [4:0] RA
);

reg [4:0] selected;
reg [7:0] h_total, h_displayed, h_sync_pos;
reg [3:0] h_sync_width;
reg [6:0] v_total, v_displayed, v_sync_pos;
reg [4:0] v_adjust, max_raster;
reg [1:0] interlace_mode;
wire ivm_mode = &interlace_mode;
reg [13:0] start_addr;
reg [5:0] cursor_addr_high;
reg [7:0] cursor_addr_low;

// FR §21.2.2 p.245 maps R14/15 and §28.1.9 p.293 omits them.
// Provisionally follow the readback measured on AmSpirit Lite 1.18.3.
// FR §21.3.2 p.247: no status driver; FF is neutral on the wired-AND bus.
wire read_data = ENABLE && !nCS && R_nW && RS;
assign DO = !read_data ? 8'hff :
            selected == 5'd14 ? {2'b00, cursor_addr_high} :
            selected == 5'd15 ? cursor_addr_low : 8'h00;

wire write_selected = ENABLE && !nCS && !R_nW && RS;
always @(posedge CLOCK) begin
	if (!nRESET) begin
		selected <= 0;
		h_total <= 0;
		h_displayed <= 0;
		h_sync_pos <= 0;
		h_sync_width <= 0;
		v_total <= 0;
		v_displayed <= 0;
		v_sync_pos <= 0;
		v_adjust <= 0;
		max_raster <= 0;
		interlace_mode <= 0;
		start_addr <= 0;
		cursor_addr_high <= 0;
		cursor_addr_low <= 0;
	end else if (ENABLE && !nCS && !R_nW) begin
		if (!RS) selected <= DI[4:0];
		else case (selected)
			5'd0: h_total <= DI;
			5'd1: h_displayed <= DI;
			5'd2: h_sync_pos <= DI;
			5'd3: h_sync_width <= DI[3:0];
			5'd4: v_total <= DI[6:0];
			5'd5: v_adjust <= DI[4:0];
			5'd6: v_displayed <= DI[6:0];
			5'd7: v_sync_pos <= DI[6:0];
			5'd8: interlace_mode <= DI[1:0];
			5'd9: max_raster <= DI[4:0];
			5'd12: start_addr[13:8] <= DI[5:0];
			5'd13: start_addr[7:0] <= DI;
			5'd14: cursor_addr_high <= DI[5:0];
			5'd15: cursor_addr_low <= DI;
			default: ;
		endcase
	end
end

reg [7:0] hcc;
reg [6:0] row;
reg [4:0] raster;
reg [3:0] ivm_raster;
reg frame_odd, next_frame_odd;
reg [4:0] adjustment;
reg in_adjustment;
reg [13:0] line_addr, next_line_addr;
reg [13:0] video_addr;
reg last_line, previous_last_line, last_line_enabled;
reg at_line_opening, opening_raster_match;
reg [3:0] hsc, vsc;
reg hsync_released;
reg vsync_counting, vsync_matched;
reg vsync_mid_pending;
reg horizontal_display, vertical_display;

wire line_end = hcc == h_total;
wire [7:0] next_hcc = line_end ? 8'd0 : hcc + 8'd1;
wire raster_end = raster == max_raster;
// FR §11.3.1 p.87: C5 can wrap during an active adjustment when
// software moves R5 below C5+1. Zero is a counter value in that state.
wire [4:0] next_adjustment = adjustment + 5'd1;
wire frame_end = in_adjustment ? next_adjustment == v_adjust :
                 last_line && v_adjust == 0;
wire [6:0] next_row = frame_end ? 7'd0 : row + {6'd0, raster_end};
wire [4:0] next_raster = frame_end || raster_end ? 5'd0 : raster + 5'd1;
// FR §19.8.3 pp.227–228: C9.IVM runs in every R8 mode, resetting
// at C9=R9/2 and at the row end. C4 still uses the ordinary comparator.
wire [3:0] ivm_limit = max_raster[4:1];
wire ivm_raster_reset = frame_end || raster_end || raster == {1'b0, ivm_limit};
wire [3:0] next_ivm_raster = ivm_raster_reset ? 4'd0 : ivm_raster + 4'd1;
// The address transfer compares C9.IVM without parity. With even R9
// the row ends before that counter reaches its limit on the second half.
wire address_raster_end = ivm_mode ? ivm_raster == ivm_limit : raster_end;
wire next_address_raster_end = ivm_mode ? next_ivm_raster == ivm_limit :
                              next_raster == max_raster;
wire display_end = next_hcc == h_displayed;
// FR §17.4.3 p.186: R1=0 compares before last-line evaluation.
// On entry to a frame, only the AND half of the pointer reload runs.
wire [13:0] line_addr_on_wrap =
	frame_end && h_displayed == 0 ? next_line_addr & start_addr :
	frame_end || address_raster_end ? next_line_addr : line_addr;
// FR §17.1 p.178: VMA runs through every character, even if a late
// R0 write forces C0 to overflow without starting a new line.
wire [13:0] next_video_addr = video_addr + 14'd1;

// FR §14.6 p.142: counting from zero wraps back to zero after 16
// characters. The running comparator also sees later R3 writes.
wire hsync_start = next_hcc == h_sync_pos;
// FR §15.3.1–2 p.150: an R2 match on the end edge keeps the
// same pulse active while the width counter rolls over.
wire hsync_end = HSYNC && hsc + 4'd1 == h_sync_width && !hsync_start;
wire hsync_next = hsync_end ? 1'b0 : HSYNC || hsync_start;

// FR §12.4.1 pp.97–99: both vertical matches arm the current last line.
// The last HSYNC character supplies the previous-match memory.
wire [6:0] compare_r4 = write_selected && selected == 5'd4 ? DI[6:0] : v_total;
wire [4:0] compare_r9 = write_selected && selected == 5'd9 ? DI[4:0] : max_raster;
wire last_line_match = row == compare_r4 && raster == compare_r9;
wire timing_write = write_selected && (selected == 5'd4 || selected == 5'd9);

// FR §16.4.3 p.171: a match during HSYNC consumes the same 16-line
// interval as a visible VSYNC. Remember the region until C4 leaves R7.
wire [6:0] compare_r7 = write_selected && selected == 5'd7 ? DI[6:0] : v_sync_pos;
wire vsync_line_tick = CLKEN && line_end;
wire vsync_match = (vsync_line_tick ? next_row : row) == compare_r7;
wire vsync_admit = vsync_match && !vsync_matched &&
                   (vsync_line_tick || (write_selected && selected == 5'd7));
// FR §19.7.2 p.219: the new frame's parity takes effect before R7=0.
// In either interlace mode the even field uses C0=R0/2.
wire vsync_mid_on_line = vsync_line_tick && interlace_mode[0] &&
                        !(frame_end ? next_frame_odd : frame_odd);
wire vsync_mid_tick = CLKEN && next_hcc == {1'b0, h_total[7:1]};
wire vsync_arm_mid = vsync_admit && vsync_mid_on_line && !vsync_mid_tick;
// MODEL LIMIT (B26): reserve the midpoint on natural row entry and
// cancel the pending event if R8 or R7 removes its match. Midpoint bus
// write ordering needs original-CPC probes. Live R7 retains §16.4.3.
wire vsync_mid_event = vsync_mid_pending && vsync_mid_tick && !vsync_line_tick &&
                       vsync_match && interlace_mode[0];
wire vsync_event = (vsync_admit && !vsync_arm_mid) || vsync_mid_event;
// FR §16.1 p.161: the fixed sixteen counts occur at C0=0,
// including after a midpoint onset and when the pin is hidden.
wire vsync_end = vsync_line_tick && vsc == 4'd15;
// FR §15.6 p.157: the natural row-entry compare sees the outgoing
// character. HSYNC starting at C0=0 can still blank DE on that line.
wire vsync_hidden = vsync_line_tick ? HSYNC :
                    CLKEN ? hsync_next || hsync_end : HSYNC || hsync_released;

assign MA = video_addr;
assign RA = ivm_mode ? {ivm_raster, frame_odd} : raster;
assign DE = horizontal_display && vertical_display;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		hcc <= 0;
		row <= 0;
		raster <= 0;
		ivm_raster <= 0;
		frame_odd <= 0;
		next_frame_odd <= 0;
		adjustment <= 0;
		in_adjustment <= 0;
		line_addr <= 0;
		next_line_addr <= 0;
		video_addr <= 0;
		last_line <= 0;
		previous_last_line <= 0;
		last_line_enabled <= 0;
		at_line_opening <= 0;
		opening_raster_match <= 0;
		hsc <= 0;
		vsc <= 0;
		hsync_released <= 0;
		HSYNC <= 0;
		VSYNC <= 0;
		vsync_counting <= 0;
		vsync_matched <= 0;
		vsync_mid_pending <= 0;
		horizontal_display <= 0;
		vertical_display <= 0;
	end else begin
		if (!vsync_match || vsync_admit) vsync_matched <= vsync_match;
		if (vsync_line_tick || vsync_mid_tick || !vsync_match || !interlace_mode[0])
			vsync_mid_pending <= vsync_arm_mid;
		if (vsync_event && (!vsync_counting || vsync_end)) begin
			vsync_counting <= 1;
			VSYNC <= !vsync_hidden;
			vsc <= 0;
		end else if (vsync_counting && vsync_line_tick) begin
			vsc <= vsc + 4'd1;
			if (vsync_end) begin
				vsync_counting <= 0;
				VSYNC <= 0;
			end
		end

		if (timing_write) begin
			// FR §12.4.1 p.97. During the first character,
			// a write uses the new R4 and the opening R9.
			if (at_line_opening) begin
				if (selected == 5'd4)
					last_line <= !HSYNC && !previous_last_line &&
					             row == compare_r4 && opening_raster_match;
			end else if (last_line_enabled && !previous_last_line &&
			             !HSYNC && last_line_match) begin
				last_line <= 1;
			end
		end

		if (CLKEN) begin
			hcc <= next_hcc;
			at_line_opening <= line_end;
			video_addr <= line_end ? line_addr_on_wrap : next_video_addr;
			HSYNC <= hsync_next;
			hsc <= HSYNC ? hsc + 4'd1 : 4'd0;
			hsync_released <= hsync_end;
			if (hsync_end && !last_line_match) last_line_enabled <= 1;
			if (hsync_end) previous_last_line <= last_line_match;

			// FR §17.4.3 pp.185–186: R12/R13 enter the pending pointer
			// at R1 during the last line. Handle R1=0 on the line seam.
			if (display_end && !line_end) begin
				horizontal_display <= 0;
				if (frame_end)
					next_line_addr <= start_addr;
				else if (address_raster_end)
					next_line_addr <= next_video_addr;
			end
			if (line_end) begin
				opening_raster_match <= next_raster == max_raster;
				row <= next_row;
				raster <= next_raster;
				ivm_raster <= next_ivm_raster;
				// FR §19.5.4 p.213: reaching R6 determines the next
				// frame's parity independently of the current R8 mode.
				if (frame_end) frame_odd <= next_frame_odd;
				if (next_row == v_displayed && (frame_end || next_row != row))
					next_frame_odd <= frame_end ? !next_frame_odd : !frame_odd;
				horizontal_display <= h_displayed != 0 && !hsync_next;
				if (frame_end || next_row == v_displayed)
					vertical_display <= frame_end && v_displayed != 0;
				line_addr <= line_addr_on_wrap;
				// Ordinary R1=0 uses the incoming raster and VMA.
				// The terminal transfer above uses the outgoing last-line state.
				if (h_displayed == 0 && (frame_end || next_address_raster_end))
					next_line_addr <= line_addr_on_wrap;
				if (frame_end) in_adjustment <= 0;
				else if (in_adjustment) adjustment <= next_adjustment;
				else if (last_line) begin
					in_adjustment <= 1;
					adjustment <= 0;
				end
				last_line <= next_row == compare_r4 && next_raster == max_raster &&
				             !(hsync_end ? last_line_match : previous_last_line) &&
				             !hsync_next;
				last_line_enabled <= (next_row != 0 || next_raster != 0) &&
				                     !hsync_next;
			end
		end
	end
end

endmodule
