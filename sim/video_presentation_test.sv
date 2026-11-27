// Exercise the actual converter at a fixed presentation delay across an
// adaptive rate change and a Raw CRT handoff. The inputs are deliberately
// independent so crossed metadata connections cannot agree by accident.
module video_presentation_test;
    logic clk = 0, ce16 = 0;
    logic plus = 0, raw = 0, tag = 0, field = 0, frame_field = 0;
    logic hs = 0, vs = 0, hb = 1, vb = 1, progress_pixel = 0;
    logic [1:0] mode = 0;
    logic [11:0] rgb = 0;
    logic override_valid = 0;
    wire [1:0] out_mode;
    wire [11:0] out_rgb;
    wire out_plus, out_raw, out_native, out_tag, out_field, out_frame;
    wire out_hs, out_vs, out_hb, out_vb, out_progress;
    amstrad_video_presentation presentation (
        .clk(clk), .rgb_in(rgb), .full_phase_rgb(12'hd9a), .full_phase_valid(override_valid),
        .plus_in(plus), .mode_in(mode), .raw_crt_in(raw), .native_cadence_in(1'b0),
        .pixel_vblank_in(tag), .field_in(field), .frame_field_in(frame_field),
        .hs_in(hs), .vs_in(vs), .hblank_in(hb), .vblank_in(vb), .progress_in(progress_pixel),
        .rgb_out(out_rgb), .plus_out(out_plus), .mode_out(out_mode), .raw_crt_out(out_raw),
        .native_cadence_out(out_native), .pixel_vblank_out(out_tag), .field_out(out_field),
        .frame_field_out(out_frame), .hs_out(out_hs), .vs_out(out_vs),
        .hblank_out(out_hb), .vblank_out(out_vb), .progress_out(out_progress)
    );

    typedef struct packed {
        logic obs_ce, obs_hs, obs_vs, obs_hb, obs_vb, obs_field;
        logic [23:0] obs_rgb;
        logic vga_ce, vga_hs, vga_vs, vga_de;
        logic [23:0] vga_rgb;
    } video_t;
    wire video_t path_output [0:1];
    for (genvar lane = 0; lane < 2; lane++) begin : output_path
        wire [21:0] gamma_bus;
        wire [11:0] lane_rgb = (lane == 1) ? out_rgb : rgb;
        assign gamma_bus[20:0] = '0;
        amstrad_video_output video (
            .CLK_VIDEO(clk), .ce_16(ce16), .raw_crt((lane == 1) ? out_raw : raw),
            .native_cadence((lane == 1) ? out_native : 1'b0), .pixel_vblank((lane == 1) ? out_tag : tag),
            .plus_mode((lane == 1) ? out_plus : plus), .mode((lane == 1) ? out_mode : mode),
            .scale(2'd0), .ar(2'd0), .integer_scale(2'd0), .mix(3'd0),
            .r4(lane_rgb[11:8]), .g4(lane_rgb[7:4]), .b4(lane_rgb[3:0]),
            .hs((lane == 1) ? out_hs : hs), .vs((lane == 1) ? out_vs : vs),
            .hbl((lane == 1) ? out_hb : hb), .vbl((lane == 1) ? out_vb : vb), .field_in((lane == 1) ? out_field : field),
            .pixel_rate_select(1'b1), .forced_scandoubler(1'b0), .vcrop_en(1'b0), .narrow_border(1'b0),
            .HDMI_WIDTH(12'd0), .HDMI_HEIGHT(12'd0), .HDMI_FREEZE(1'b0),
            .progress_pix((lane == 1) ? out_progress : progress_pixel), .gamma_bus(gamma_bus),
            .CE_PIXEL(path_output[lane].vga_ce), .VGA_HS(path_output[lane].vga_hs),
            .VGA_VS(path_output[lane].vga_vs), .VGA_DE(path_output[lane].vga_de),
            .VGA_R(path_output[lane].vga_rgb[23:16]), .VGA_G(path_output[lane].vga_rgb[15:8]),
            .VGA_B(path_output[lane].vga_rgb[7:0]), .VGA_SL(), .VIDEO_ARX(), .VIDEO_ARY(), .en270p(),
            .obs_ce_pix(path_output[lane].obs_ce), .obs_hs(path_output[lane].obs_hs),
            .obs_vs(path_output[lane].obs_vs), .obs_hbl(path_output[lane].obs_hb),
            .obs_vbl(path_output[lane].obs_vb), .obs_field(path_output[lane].obs_field),
            .obs_r(path_output[lane].obs_rgb[23:16]), .obs_g(path_output[lane].obs_rgb[15:8]),
            .obs_b(path_output[lane].obs_rgb[7:0]), .obs_native_cadence(), .obs_applied_config()
        );
    end

    video_t history [0:7];
    logic delayed_f1 [0:7];
    integer count;
    initial begin
        for (int frame = 0; frame < 8; frame++) begin
            for (int y = 0; y < 24; y++) begin
                count = 0;
                for (int x = 0; x < 1024; x++) begin
                    ce16 = x[1:0] == 0;
                    mode = frame >= 4 ? 2'd2 : frame >= 2 ? 2'd1 : 2'd0;
                    raw = frame == 6 || (frame == 5 && y >= 21);
                    plus = (frame >= 3) != (frame >= 7);
                    field = frame[0] ^ (y >= 21);
                    frame_field = frame[1] ^ (y >= 21);
                    hs = x >= 848 && x < 912;
                    hb = x >= 768;
                    vb = y >= 20;
                    vs = y == 21 || y == 22;
                    tag = x >= 303 && x < 391 && y[0];
                    progress_pixel = x >= 505 && x < 557 && y[1];
                    rgb = {x[5:2], y[3:0], x[7:4] ^ y[3:0]};
                    #1;
                    if (frame >= 1 && out_frame !== delayed_f1[7])
                        $fatal(1, "presentation F1 slipped at frame %0d, line %0d, clock %0d", frame,y,x);
                    // Observe both paths on the sample each downstream register sees.
                    if (frame >= 1 && path_output[1] !== history[7])
                        $fatal(1, "presentation packet slipped at frame %0d, line %0d, clock %0d: %h != %h",
                               frame,y,x,path_output[1],history[7]);
                    for (int i = 7; i > 0; i--) begin
                        history[i] = history[i-1];
                        delayed_f1[i] = delayed_f1[i-1];
                    end
                    history[0] = path_output[0];
                    delayed_f1[0] = frame_field;
                    if (path_output[0].obs_ce) count++;
                    clk = 1;
                    #1;
                    clk = 0;
                end
                // The converter must really consume three different rates.
                if (y == 8 && ((frame == 1 && count != 64) || (frame == 3 && count != 128) ||
                              ((frame == 5 || frame == 6) && count != 256)))
                    $fatal(1, "unexpected pixel cadence %0d in frame %0d", count, frame);
            end
        end
        override_valid = 1;
        if (out_rgb == 12'hd9a)
            $fatal(1, "phase tap overrode a blank packet");
        hb = 0;
        vb = 0;
        rgb = 12'h135;
        repeat (8) begin
            #1;
            clk = 1;
            #1;
            clk = 0;
        end
        if (out_rgb != 12'hd9a)
            $fatal(1, "qualified phase tap missed an active packet");
        $display("PASS fixed presentation through adaptive sampling, FIELD and Raw CRT");
        $finish;
    end
endmodule
