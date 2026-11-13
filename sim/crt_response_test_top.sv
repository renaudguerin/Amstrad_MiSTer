// SPDX-License-Identifier: GPL-2.0-or-later
// Simulation fixture: real enabled sprite leaf + test screen/sprite mux.
// This deliberately does not claim production asic_video compositor coverage.
module crt_response_test_top (
    input logic clk, ce_16, reset, context_clear, owner,
    input logic ga_hs, ga_vs, source_missing,
    input logic [11:0] rgb12,
    input logic raw_vblank, hs, vs, hblank, vblank, field, frame_field,
    output wire [11:0] rgb12_out,
    output wire raw_vblank_out, available,
    output wire hs_out, vs_out, hblank_out, vblank_out, field_out, frame_field_out,
    output wire [11:0] fixture_rgb,
    output wire fixture_sprite
);
    logic [9:0] dot=0;
    logic [9:0] line=0;
    wire request;
    wire [10:0] address_unused;
    logic ack=0;
    wire spr_en;
    wire [11:0] spr_rgb;
    always_ff @(posedge clk) begin
        ack<=request;
        if(reset) begin dot<=0; line<=0; ack<=0; end
        else if(ce_16) begin
            dot<=dot+10'd1;
            if(dot==1023) line<=line+10'd1;
        end
    end
    asic_sprites sprites (
        .CLOCK(clk),.PIXEN(ce_16),.CLKEN(ce_16 && dot[3:0]==15),
        .HWRAP(ce_16 && dot==1023),.nRESET(!reset),.LINE(line[9:3]),.ROW(line[4:0]),
        .SPR_X(160'd16),.SPR_Y(144'd8),.SPR_MAG(64'd5),.SPR_PAL(180'h124),
        .ACC_EN(1'b0),.ACC_IDX(4'd0),.spr_wr_en(1'b0),.spr_wr_addr(12'd0),.spr_wr_data(4'd0),
        .FQ_REQ(request),.FQ_ADDR(address_unused),.FQ_ACK(ack),.FQ_DATA(8'h11),
        .SPR_EN(spr_en),.SPR_RGB(spr_rgb),.SPR_IDX(),.SPR_WIN()
    );
    assign fixture_sprite=owner && spr_en;
    assign fixture_rgb=fixture_sprite ? spr_rgb : rgb12;
    crt_response receiver (
        .clk(clk),.ce_16(ce_16),.reset(reset),.context_clear(context_clear),.owner(owner),
        .ga_hs(ga_hs),.ga_vs(ga_vs),.source_missing(source_missing),.rgb12(fixture_rgb),
        .raw_vblank(raw_vblank),.hs(hs),.vs(vs),.hblank(hblank),.vblank(vblank),
        .field(field),.frame_field(frame_field),.rgb12_out(rgb12_out),
        .raw_vblank_out(raw_vblank_out),.available(available),.hs_out(hs_out),.vs_out(vs_out),
        .hblank_out(hblank_out),.vblank_out(vblank_out),.field_out(field_out),.frame_field_out(frame_field_out)
    );
endmodule
