// Copyright (C) 2018-2019 Sorgelig
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Core-owned output chain, extracted from Amstrad.sv for B6 boundary tests.
// This is the production colour/interlace/mixer/crop path. The motherboard
// owns raw_crt and pixel_vblank; retained menu settings enter independently.
// No framework module is copied or changed (docs/b6-video-boundary.md).
module amstrad_video_output (
    input CLK_VIDEO, ce_16, raw_crt, pixel_vblank, plus_mode,
    input [1:0] mode, scale, ar, integer_scale,
    input [2:0] mix,
    input [3:0] r4, g4, b4,
    input hs, vs, hbl, vbl,
    input pixel_rate_select, forced_scandoubler, field_in, vcrop_en,
    input [11:0] HDMI_WIDTH, HDMI_HEIGHT,
    input HDMI_FREEZE, progress_pix,
    inout [21:0] gamma_bus,
    output CE_PIXEL,
    output [7:0] VGA_R, VGA_G, VGA_B,
    output VGA_HS, VGA_VS, VGA_DE,
    output [1:0] VGA_SL,
    output [12:0] VIDEO_ARX, VIDEO_ARY,
    output reg en270p
);
wire ce_pix;
wire [7:0] B, G, R;
wire HSync, VSync, HBlank, VBlank;

// B6: the applied Raw CRT selection and the raw vertical-blank pixel tag,
// both owned by the motherboard's single applied-mode register.

amstrad_video_color video_color (
    .CLK_VIDEO(CLK_VIDEO), .ce_16(ce_16),
    .hq2x(hq2x), .pixel_rate_select(pixel_rate_select & ~raw_crt),
    .pixel_vblank(pixel_vblank), .plus_mode(plus_mode),
    .mode(mode), .mix(mix), .r4(r4), .g4(g4), .b4(b4),
    .hs(hs), .vs(vs), .hbl(hbl), .vbl(vbl), .ce_pix(ce_pix),
    .R_out(R), .G_out(G), .B_out(B),
    .HSync(HSync), .VSync(VSync), .HBlank(HBlank), .VBlank(VBlank)
);

// Raw CRT forces the core's own post-processing off: native ce_16 (no
// adaptive pixel rate, no HQ2x), no scandoubler, no scanline effect and no
// vertical crop.  The retained OSD settings themselves are untouched and
// come back when a safe mode is selected again.
wire       hq2x = (scale == 1) & ~raw_crt;

assign VGA_SL = (scale[1] & ~raw_crt) ? scale : 2'b00;

wire [2:0] interlace;
wire       scandoubler_hist;
// Forced scandoubler is disabled at this boundary too: otherwise a framework
// setting could silently resample a selected CRT mode.
wire       scandoubler_en = scandoubler_hist & ~raw_crt;
video_interlace interlace_hist
(
	.clk(CLK_VIDEO),
	.vsync_in(vs),
	.field_in(field_in),
	.scale(scale),
	.forced_scandoubler(forced_scandoubler & ~raw_crt),
	.interlace(interlace),
	.scandoubler(scandoubler_hist)
);

video_mixer #(.LINE_LENGTH(800), .GAMMA(1)) video_mixer
(
	.*,
	.R(R | {8{progress_pix}}),
	.G(G | {8{progress_pix}}),
	.B(B | {8{progress_pix}}),
	.VGA_DE(vga_de),
	.freeze_sync(),
	.scandoubler(scandoubler_en)
);

always @(posedge CLK_VIDEO) begin
	en270p <= ((HDMI_WIDTH == 1920) && (HDMI_HEIGHT == 1080) && !forced_scandoubler && !scale);
end

wire vga_de;
wire vga_de_cropped;
video_freak video_freak
(
	.*,
	.VGA_DE(vga_de_cropped),
	.VGA_DE_IN(vga_de),

	.ARX((!ar) ? 12'd4 : (ar - 1'd1)),
	.ARY((!ar) ? 12'd3 : 12'd0),
	.CROP_SIZE((en270p & vcrop_en & ~raw_crt) ? 10'd270 : 10'd0),
	.CROP_OFF(0),
	.SCALE(integer_scale)
);

// CROP_SIZE alone is not enough to bypass the crop in Raw CRT: video_freak
// only re-evaluates its crop window on a VSYNC rise, so a raw stream without
// usable VSYNC would hold the previous vde and keep cropping lines that the
// selected mode must pass through.  Take the mixer's DE directly instead.
// Aspect-ratio and integer-scale outputs stay framework-owned either way.
assign VGA_DE = raw_crt ? vga_de : vga_de_cropped;

endmodule
