// SPDX-License-Identifier: GPL-2.0-or-later
// Transport the legacy pixel and all of its metadata together. The alternative
// Full phase tap already accounts for this two-native-dot presentation delay.
module amstrad_video_presentation (
    input clk,
    input [11:0] rgb_in,
    input [11:0] full_phase_rgb,
    input full_phase_valid,
    input plus_in,
    input [1:0] mode_in,
    input raw_crt_in, native_cadence_in, pixel_vblank_in,
    input field_in, frame_field_in,
    input hs_in, vs_in, hblank_in, vblank_in, progress_in,
    output [11:0] rgb_out,
    output plus_out,
    output [1:0] mode_out,
    output raw_crt_out, native_cadence_out, pixel_vblank_out,
    output field_out, frame_field_out,
    output hs_out, vs_out, hblank_out, vblank_out, progress_out
);
localparam PRESENTATION_CLOCKS = 8; // 64 MHz, four clocks per native dot
localparam [24:0] STARTUP_BLANK = {10'd0, 2'b11, 13'd0};
reg [24:0] history [0:PRESENTATION_CLOCKS-1];
integer slot;
initial begin
    for (slot = 0; slot < PRESENTATION_CLOCKS; slot = slot + 1)
        history[slot] = STARTUP_BLANK;
end

always @(posedge clk) begin : presentation_delay
    integer i;
    history[0] <= {plus_in, mode_in, raw_crt_in, native_cadence_in, pixel_vblank_in,
                   field_in, frame_field_in, hs_in, vs_in, hblank_in, vblank_in,
                   progress_in, rgb_in};
    for (i = 1; i < PRESENTATION_CLOCKS; i = i + 1)
        history[i] <= history[i-1];
end

wire [24:0] pixel = history[PRESENTATION_CLOCKS-1];
assign {plus_out, mode_out, raw_crt_out, native_cadence_out, pixel_vblank_out,
        field_out, frame_field_out, hs_out, vs_out, hblank_out, vblank_out,
        progress_out} = pixel[24:12];
assign rgb_out = full_phase_valid && !hblank_out && !vblank_out ? full_phase_rgb : pixel[11:0];
endmodule
