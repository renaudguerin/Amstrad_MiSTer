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

// Progressive timing. Interlace, cursor output and the sub-character
// R0/R2/R3 and VSYNC aperture require a separately qualified board path.
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
reg [4:0] adjustment;
reg in_adjustment;
reg [13:0] line_addr, next_line_addr;
reg [13:0] video_addr;
reg last_line, previous_last_line, last_line_enabled;
reg [3:0] hsc, vsc;
reg hsync_released;
reg vsync_counting, vsync_matched;
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
wire display_end = next_hcc == h_displayed;
// FR §17.4.3 p.186: R1=0 compares before last-line evaluation.
// On entry to a frame, only the AND half of the pointer reload runs.
wire [13:0] line_addr_on_wrap =
	frame_end && h_displayed == 0 ? next_line_addr & start_addr :
	frame_end || raster_end ? next_line_addr : line_addr;
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
wire vsync_match = (CLKEN && line_end ? next_row : row) == compare_r7;
wire vsync_event = vsync_match && !vsync_matched &&
                   ((CLKEN && line_end) || (write_selected && selected == 5'd7));
wire vsync_end = CLKEN && line_end && vsc == 4'd15;
// FR §15.6 p.157: the natural row-entry compare sees the outgoing
// character. HSYNC starting at C0=0 can still blank DE on that line.
wire vsync_hidden = CLKEN && line_end ? HSYNC :
                    CLKEN ? hsync_next || hsync_end : HSYNC || hsync_released;

assign MA = video_addr;
assign RA = raster;
assign DE = horizontal_display && vertical_display;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		hcc <= 0;
		row <= 0;
		raster <= 0;
		adjustment <= 0;
		in_adjustment <= 0;
		line_addr <= 0;
		next_line_addr <= 0;
		video_addr <= 0;
		last_line <= 0;
		previous_last_line <= 0;
		last_line_enabled <= 0;
		hsc <= 0;
		vsc <= 0;
		hsync_released <= 0;
		HSYNC <= 0;
		VSYNC <= 0;
		vsync_counting <= 0;
		vsync_matched <= 0;
		horizontal_display <= 0;
		vertical_display <= 0;
	end else begin
		if (!vsync_match || vsync_event) vsync_matched <= vsync_match;
		if (vsync_event && (!vsync_counting || vsync_end)) begin
			vsync_counting <= 1;
			VSYNC <= !vsync_hidden;
			vsc <= 0;
		end else if (vsync_counting && CLKEN && line_end) begin
			vsc <= vsc + 4'd1;
			if (vsync_end) begin
				vsync_counting <= 0;
				VSYNC <= 0;
			end
		end

		if (timing_write && last_line_enabled && !HSYNC && last_line_match)
			last_line <= 1;

		if (CLKEN) begin
			hcc <= next_hcc;
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
				else if (raster_end)
					next_line_addr <= next_video_addr;
			end
			if (line_end) begin
				row <= next_row;
				raster <= next_raster;
				horizontal_display <= h_displayed != 0 && !hsync_next;
				if (frame_end || next_row == v_displayed)
					vertical_display <= frame_end && v_displayed != 0;
				line_addr <= line_addr_on_wrap;
				// Ordinary R1=0 uses the incoming raster and VMA.
				// The terminal transfer above uses the outgoing last-line state.
				if (h_displayed == 0 && (frame_end || next_raster == max_raster))
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
