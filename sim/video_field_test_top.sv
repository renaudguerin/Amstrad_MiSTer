// Real CRTC -> production core output boundary -> ASCAL trace source.
// Pixel colours are immaterial to the scaler's public framebuffer header.
module video_field_test_top (
    input clk, reset_n, crtc_type, clken, nclken, ce_16,
    input enable, rs, border,
    input [7:0] data,
    output [7:0] c0, c4, c9,
    output raw_field, raw_vs,
    output ce, hs, vs, field_out, de
);
    wire hsync, display;
    CRTC crtc (
        .CLOCK(clk), .CLKEN(clken), .nCLKEN(nclken), .nRESET(reset_n),
        .CRTC_TYPE(crtc_type), .ENABLE(enable), .nCS(~enable),
        .R_nW(1'b0), .RS(rs), .DI(data), .DO(),
        .SNA_LOAD(1'b0), .SNA_ADDR(5'd0), .SNA_REGS(144'd0),
        .VSYNC(raw_vs), .HSYNC(hsync), .HSYNC_RESTART(), .DE(display),
        .FIELD(raw_field), .FRAME_FIELD(field_out),
        .CURSOR(), .MA(), .RA(), .SNAP_HCC(c0), .SNAP_ROW(c4), .SNAP_LINE(c9)
    );
    wire [21:0] gamma_bus = 22'd0;
    amstrad_video_output output_chain (
        .CLK_VIDEO(clk), .ce_16(ce_16), .raw_crt(1'b0),
        .pixel_vblank(1'b0), .plus_mode(1'b0), .mode(2'd2),
        .scale(2'd0), .ar(2'd0), .integer_scale(2'd0), .mix(3'd0),
        .r4(4'hf), .g4(4'hf), .b4(4'hf),
        // The adversarial border case admits pixels before the CRTC origin,
        // as production monitor blanking does; CRTC DE excludes that border.
        .hs(hsync), .vs(raw_vs), .hbl(border ? hsync : ~display),
        .vbl(border ? raw_vs : 1'b0),
        .pixel_rate_select(1'b0), .forced_scandoubler(1'b0),
        .field_in(raw_field), .vcrop_en(1'b0),
        .HDMI_WIDTH(12'd1920), .HDMI_HEIGHT(12'd1080),
        .HDMI_FREEZE(1'b0), .progress_pix(1'b0), .gamma_bus(gamma_bus),
        .CE_PIXEL(ce), .VGA_R(), .VGA_G(), .VGA_B(),
        .VGA_HS(hs), .VGA_VS(vs), .VGA_DE(de),
        .VGA_SL(), .VIDEO_ARX(), .VIDEO_ARY(), .en270p()
    );
endmodule
