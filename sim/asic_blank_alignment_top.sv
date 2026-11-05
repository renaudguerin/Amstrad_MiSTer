// Simulation-only composition: real B18 motherboard/CPU boundary plus a
// public-pin leaf for SNA_LOAD-without-reset lifecycle cases. The real SNA
// download owner resets the motherboard before apply, so it cannot exercise
// stale pixel history crossing a reset-free SNA_LOAD by itself.
module asic_blank_alignment_top (
    input clk, reset_btn, sna_download, load_wr,
    input [24:0] load_addr,
    input [7:0] load_data, mem_din,
    input [15:0] video_word,
    output [22:0] mem_addr, load_mem_addr,
    output [7:0] mem_dout,
    output mem_rd, mem_wr, load_mem_wr, sna_load, sna_hold, owner_reset,
    output [15:0] cpu_addr,
    output cpu_insn_start, cpu_halt_n,
    output [7:0] hcc,
    output [6:0] row,
    output [4:0] raster,
    output [14:0] video_addr,
    output [11:0] rgb,
    output [4:0] pen,
    output hs, vs, de,
    input life_reset_n, life_pix, life_ce, life_load, life_hs,
    input [7:0] life_hcc,
    input [3:0] life_hsw,
    output life_raw_hs,
    output [11:0] life_rgb
);
    b18_snapshot_top #(.PLUS_MODE(1'b1), .VIDEO_RAM(1'b1)) machine (
        .clk(clk), .reset_btn(reset_btn), .pause(1'b0), .filter_request(2'd0), .model(2'd0), .crtc_type(1'b1),
        .save_req(1'b0), .sna_download(sna_download), .load_wr(load_wr),
        .load_addr(load_addr), .load_data(load_data), .mem_din(mem_din),
        .cart_dout(8'hff), .cart_ack(1'b0), .ddram_busy(1'b0),
        .mem_addr(mem_addr), .mem_dout(mem_dout), .mem_rd(mem_rd), .mem_wr(mem_wr),
        .load_mem_wr(load_mem_wr), .load_mem_addr(load_mem_addr),
        .sna_load(sna_load), .sna_hold(sna_hold), .owner_reset(owner_reset),
        .cpu_addr(cpu_addr), .cpu_insn_start(cpu_insn_start), .cpu_halt_n(cpu_halt_n),
        .plus_hcc(hcc), .plus_row(row), .plus_raster(raster),
        .video_word(video_word), .video_addr(video_addr), .video_rgb(rgb),
        .video_pen(pen), .video_hs(hs), .video_vs(vs), .video_de(de)
    );

    // White data AND border: black uniquely identifies the force-blank
    // output, independent of DE/serializer startup in the lifecycle fixture.
    wire [3:0] red, green, blue;
    asic_video restore_leaf (
        .CLOCK(clk), .CLKEN(life_ce), .nRESET(life_reset_n),
        .ENABLE(1'b0), .nCS(1'b1), .R_nW(1'b1), .RS(1'b0), .DI(8'd0),
        .HSYNC(life_raw_hs), .SPLT(8'd0), .SSA(14'd0), .SSCR(8'd0),
        .PIXEN(life_pix), .VIDEOD(16'd0), .MODE_UPDATE_EN(1'b0), .GAMODE(2'd1),
        .BORDER_I(5'd11), .INKR_I({16{5'd11}}),
        .RGB_R(red), .RGB_G(green), .RGB_B(blue),
        .SPR_EN(1'b0), .SPR_RGB(12'd0), .PAL_EN(1'b0), .PAL_RGB(12'd0),
        .SNA_LOAD(life_load), .SNA_ADDR(5'd0),
        .SNA_REGS(144'h000000000030000007001e1900260216283f),
        .SNA_HCC(life_hcc), .SNA_LINE(7'd0), .SNA_RASTER(5'd0),
        .SNA_VTA(5'd0), .SNA_HSW(life_hsw), .SNA_VSW(4'd0),
        .SNA_VS(1'b0), .SNA_HS(life_hs), .SNA_ADJ(1'b0), .SNA_MODE(2'd1)
    );
    assign life_rgb = {red, green, blue};
endmodule
