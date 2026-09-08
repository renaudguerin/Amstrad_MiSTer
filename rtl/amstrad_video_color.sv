// Copyright (C) 2018-2019 Sorgelig
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Top-level CPC/Plus colour conversion and pixel-enable boundary.
// Extracted from Amstrad.sv; classic DAC conversion remains in color_mix.
module amstrad_video_color (
    input CLK_VIDEO, ce_16, hq2x, pixel_rate_select, plus_mode,
    input [1:0] mode,
    input [2:0] mix,
    input [3:0] r4, g4, b4,
    input hs, vs, hbl, vbl,
    output ce_pix,
    output [7:0] R_out, G_out, B_out,
    output HSync, VSync, HBlank, VBlank
);
reg ce_pix_fs;
always @(posedge CLK_VIDEO) begin
	reg [1:0] mode_fs;
	reg [1:0] mode_next;
	reg [1:0] cycle;
	reg       old_vsync;

	ce_pix_fs <= 0;

	if (ce_16) begin
		cycle <= cycle + 1'd1;

		case(mode_fs)
			2:   ce_pix_fs <= 1;
			1:   ce_pix_fs <= !cycle[0];
			0,3: ce_pix_fs <= !cycle[1:0];
		endcase

		old_vsync <= vs;
		if(~old_vsync & vs) begin
			mode_fs <= mode_next; //HQ2x friendly vmode
			mode_next <= 0;
			cycle <= 0;
		end

		// choose highest pixel rate during the whole active time
		if (~hbl && ~vbl && ~&mode && mode > mode_next) mode_next <= mode;
	end
end

assign ce_pix = (hq2x | pixel_rate_select) ? ce_pix_fs : ce_16;

// P2 RGB widening: the motherboard emits 4-bit channels. In classic mode
// the low two bits are the raw netlist {level, OE_N} pair, so color_mix
// keeps its exact GA-DAC behaviour bit-for-bit; in Plus mode the nibble is
// the native ASIC palette level and bypasses the GA table after expansion.

color_mix color_mix
(
	.clk_vid(CLK_VIDEO),
	.ce_pix(ce_pix),
	.mix(mix),

	.HSync_in(hs),
	.VSync_in(vs),
	.HBlank_in(hbl),
	.VBlank_in(vbl),
	.B_in(b4[1:0]),
	.G_in(g4[1:0]),
	.R_in(r4[1:0]),

	.HSync_out(HSync),
	.VSync_out(VSync),
	.HBlank_out(HBlank),
	.VBlank_out(VBlank),
	.B_out(B),
	.G_out(G),
	.R_out(R)
);

wire [7:0] B, G, R;

// Plus-native 4-bit expansion to the 8-bit video path (nibble * 17).
wire [7:0] R_plus_raw = {r4[3:0], r4[3:0]};
wire [7:0] G_plus_raw = {g4[3:0], g4[3:0]};
wire [7:0] B_plus_raw = {b4[3:0], b4[3:0]};

// Plus-native luma weighting (G:R:B = 9:3:1 / 13) matching CRT monitor options
wire [15:0] px_plus = R_plus_raw * 16'd59 + G_plus_raw * 16'd177 + B_plus_raw * 16'd20;
wire [7:0]  y_plus  = px_plus[15:8];

reg [7:0] R_plus, G_plus, B_plus;
always @(*) begin
	case (mix)
		3'd0, 3'd1: {R_plus, G_plus, B_plus} = {R_plus_raw, G_plus_raw, B_plus_raw}; // color
		3'd2:       {R_plus, G_plus, B_plus} = {8'd0, y_plus, 8'd0};                 // green
		3'd3:       {R_plus, G_plus, B_plus} = {y_plus, y_plus - px_plus[15:10], 8'd0}; // amber
		3'd4:       {R_plus, G_plus, B_plus} = {8'd0, y_plus, y_plus};               // cyan
		3'd5:       {R_plus, G_plus, B_plus} = {y_plus, y_plus, y_plus};             // gray
		default:    {R_plus, G_plus, B_plus} = {R_plus_raw, G_plus_raw, B_plus_raw};
	endcase
end

assign R_out = plus_mode ? R_plus : R;
assign G_out = plus_mode ? G_plus : G;
assign B_out = plus_mode ? B_plus : B;
endmodule
