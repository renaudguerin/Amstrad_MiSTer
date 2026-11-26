// Copyright (C) 2018-2019 Sorgelig
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Core-owned output chain, extracted from Amstrad.sv for B6 boundary tests.
// This is the production colour/interlace/mixer/crop path. The motherboard
// owns raw_crt and pixel_vblank; retained menu settings enter independently.
// No framework module is copied or changed
// (hardware observations).
module amstrad_video_output (
    input CLK_VIDEO, ce_16, raw_crt, native_cadence, pixel_vblank, plus_mode,
    input [1:0] mode, scale, ar, integer_scale,
    input [2:0] mix,
    input [3:0] r4, g4, b4,
    input hs, vs, hbl, vbl,
    input pixel_rate_select, forced_scandoubler, field_in, vcrop_en, narrow_border,
    input [11:0] HDMI_WIDTH, HDMI_HEIGHT,
    input HDMI_FREEZE, progress_pix,
    inout [21:0] gamma_bus,
    output CE_PIXEL,
    output [7:0] VGA_R, VGA_G, VGA_B,
    output VGA_HS, VGA_VS, VGA_DE,
    output [1:0] VGA_SL,
    output [12:0] VIDEO_ARX, VIDEO_ARY,
    output reg en270p,

    // B4 phase 2 observation tap: the converted RGB24 and its aligned
    // sync/blanking tuple, before gamma, scandoubling, HQ2x and crop. These
    // are outputs only; nothing downstream of this module reads them, so the
    // displayed picture is identical whether or not a recorder is compiled in.
    //
    // obs_native_cadence reports whether the *selected* settings give one
    // sample per native dot. Raw CRT happens to force the same cadence, but
    // it also changes acquisition geometry, so it is not a way to satisfy the
    // capture profile: an unsupported selection is flagged here rather than
    // normalised away.
    output obs_ce_pix,
    output [7:0] obs_r, obs_g, obs_b,
    output obs_hs, obs_vs, obs_hbl, obs_vbl,
    output obs_field,
    output obs_native_cadence,
    output [7:0] obs_applied_config
);
wire ce_pix;
wire [7:0] B, G, R;
wire HSync, VSync, HBlank, VBlank;

// B6: the applied Raw CRT selection and the raw vertical-blank pixel tag,
// both owned by the motherboard's single applied-mode register.

// Raw CRT forces the core's own post-processing off: native ce_16 (no
// adaptive pixel rate, no HQ2x), no scandoubler, no scanline effect and no
// vertical crop.  The retained OSD settings themselves are untouched and
// come back when a safe mode is selected again.
// Native dot cadence is independent of Raw CRT acquisition/crop bypass.
// V1 retains the existing downstream scanline/scandoubler/crop policy.
wire       hq2x = (scale == 1) & ~(raw_crt | native_cadence);

amstrad_video_color video_color (
    .CLK_VIDEO(CLK_VIDEO), .ce_16(ce_16),
    .hq2x(hq2x), .pixel_rate_select(pixel_rate_select & ~(raw_crt | native_cadence)),
    .pixel_vblank(pixel_vblank), .plus_mode(plus_mode),
    .mode(mode), .mix(mix), .r4(r4), .g4(g4), .b4(b4),
    .hs(hs), .vs(vs), .hbl(hbl), .vbl(vbl), .ce_pix(ce_pix),
    .R_out(R), .G_out(G), .B_out(B),
    .HSync(HSync), .VSync(VSync), .HBlank(HBlank), .VBlank(VBlank)
);

// Observation tap: retained for verification and regression testing.
// Independently asserted by sim/b6_video_output_test.cpp for B8-6 coverage.
// In particular, obs_native_cadence formalizes the unscaled native (768x273)
// pixel cadence predicate that unscaled screenshot capture validity relies upon.
assign obs_ce_pix = ce_pix;
assign obs_r      = R;
assign obs_g      = G;
assign obs_b      = B;
assign obs_hs     = HSync;
assign obs_vs     = VSync;
assign obs_hbl    = HBlank;
assign obs_vbl    = VBlank;

reg obs_field_r = 1'b0;
always @(posedge CLK_VIDEO) begin
    if (ce_pix) obs_field_r <= field_in;
end
assign obs_field  = obs_field_r;
assign obs_native_cadence = ~raw_crt & (native_cadence | ~(pixel_rate_select | (scale == 2'd1)));
assign obs_applied_config = {plus_mode, mix, hq2x, pixel_rate_select, raw_crt, obs_native_cadence};

// The normal filtered raster is 768 dots by about 273 lines. Keep a
// 672-by-216 window, with a margin around the nominal 640-by-200 picture.
// Count native dots so the same window applies at every adaptive pixel rate.
// The tap above retains the original blanking and geometry for capture.
reg [9:0] border_dot = 0;
reg [8:0] border_line = 0;
reg old_hblank = 1'b1;
reg old_vblank = 1'b1;
reg narrow_frame = 1'b0;
reg narrow_hblank = 1'b1;
always @(posedge CLK_VIDEO) begin
    if (ce_16) begin
        old_hblank <= hbl;
        old_vblank <= vbl;
        if (hbl) border_dot <= 0;
        else if (!(&border_dot)) border_dot <= border_dot + 1'd1;

        if (vbl) border_line <= 0;
        else if (hbl && !old_hblank && !(&border_line))
            border_line <= border_line + 1'd1;

        if (vbl && !old_vblank) narrow_frame <= narrow_border;
    end

    // Register the mask beside the colour sample. Adaptive and HQ2x CE
    // arrive one clock after the native dot counter has already advanced.
    if (ce_pix) begin
        narrow_hblank <= (border_dot < (ce_16 ? 10'd48 : 10'd49)) ||
                         (border_dot >= (ce_16 ? 10'd720 : 10'd721));
    end
end

// Raw CRT and CRT response need the full raster. Main passes zero HDMI
// dimensions in Direct Video, which must also keep its original blanking.
wire narrow_active = narrow_frame && !raw_crt && !native_cadence && (HDMI_HEIGHT != 0);
wire mixer_hblank = HBlank | (narrow_active && narrow_hblank);
wire mixer_vblank = VBlank | (narrow_active && (border_line < 9'd28 || border_line >= 9'd244));

// Raw CRT forces the core's own post-processing off: native ce_16 (no
// adaptive pixel rate, no HQ2x), no scandoubler, no scanline effect and no
// vertical crop.  The retained OSD settings themselves are untouched and
// come back when a safe mode is selected again.

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
	.HBlank(mixer_hblank),
	.VBlank(mixer_vblank),
	.VGA_DE(vga_de),
	.freeze_sync(),
	.scandoubler(scandoubler_en)
);

always @(posedge CLK_VIDEO) begin
	en270p <= (HDMI_WIDTH == 1920) && (HDMI_HEIGHT == 1080) && !forced_scandoubler && !scale && !narrow_active;
end

wire vga_de;
wire vga_de_cropped;
video_freak video_freak
(
	.*,
	.VGA_DE(vga_de_cropped),
	.VGA_DE_IN(vga_de),

	// 4:3 scaled by the horizontal and vertical fractions of the original
	// capture window, preserving the pixel aspect ratio after cropping.
	.ARX((!ar) ? (narrow_active ? 12'd637 : 12'd4) : (ar - 1'd1)),
	.ARY((!ar) ? (narrow_active ? 12'd432 : 12'd3) : 12'd0),
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
