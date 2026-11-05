// Thin classic observation wrapper around production B18 snapshot top.
// Exposes read-only actual CRTC/GA/CPU/VRAM signals and raw classic RGB.
// (B18 existing video_rgb is PLUS even when PLUS_MODE=0).
//
// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND 4.0), French v1.11 §§9.1, 9.3.4 pp47, 56-67.
module classic_mode_boundary_top (
    input clk,
    input reset_btn,
    input pause,
    input crtc_type,
    input sna_download,
    input load_wr,
    input [24:0] load_addr,
    input [7:0] load_data,
    input [7:0] mem_din,
    input [15:0] video_word,
    output [22:0] mem_addr,
    output [7:0] mem_dout,
    output mem_rd,
    output mem_wr,
    output load_mem_wr,
    output [22:0] load_mem_addr,
    output sna_load,
    output sna_hold,
    output owner_reset,
    output [15:0] cpu_addr,
    output cpu_insn_start,
    output cpu_iorq,
    output cpu_mreq,
    output cpu_rd,
    output cpu_wr,
    output [14:0] video_addr,

    // Read-only classic GA, CRTC, bus and pixel observations
    output [5:0] classic_rgb,
    output classic_raw_blank,
    output [7:0] c0,
    output [6:0] c4,
    output [4:0] c9,
    output [3:0] hsc,
    output [7:0] seq,
    output raw_hs,
    output [6:0] dbg_char_phase,
    output dbg_write,
    output dbg_write_d,
    output dbg_jit_write,
    output [4:0] crtc_reg,
    output [7:0] r2_value,
    output [7:0] r3_value,
    output ready,
    output ce16,
    output ga_load,
    output [7:0] video_buf,
    output [7:0] shift_reg,
    output ga_ctrl_accept,
    output [7:0] ga_data,
    output [1:0] effective_mode,
    output [2:0] effective_decode,
    output [1:0] requested_mode,
    output mode_sync,
    output [79:0] observed_inks
);
    b18_snapshot_top #(.PLUS_MODE(1'b0), .VIDEO_RAM(1'b1)) machine (
        .clk(clk),
        .reset_btn(reset_btn),
        .pause(pause),
        .model(2'd0),
        .crtc_type(crtc_type),
        .save_req(1'b0),
        .sna_download(sna_download),
        .load_wr(load_wr),
        .load_addr(load_addr),
        .load_data(load_data),
        .mem_din(mem_din),
        .cart_dout(8'hff),
        .cart_ack(1'b0),
        .ddram_busy(1'b0),
        .mem_addr(mem_addr),
        .mem_bank(),
        .mem_rd(mem_rd),
        .mem_wr(mem_wr),
        .mem_dout(mem_dout),
        .load_mem_wr(load_mem_wr),
        .load_mem_addr(load_mem_addr),
        .load_mem_bank(),
        .cart_req(),
        .cart_addr(),
        .cart_bank(),
        .ddram_addr(),
        .ddram_din(),
        .ddram_we(),
        .captured(),
        .save_hold(),
        .save_done(),
        .sna_load(sna_load),
        .sna_hold(sna_hold),
        .owner_reset(owner_reset),
        .cpu_reg(),
        .cpu_addr(cpu_addr),
        .cpu_insn_start(cpu_insn_start),
        .cpu_halt_n(),
        .hw_header(),
        .saved_header(),
        .active_model(),
        .cpu_iorq(cpu_iorq),
        .cpu_mreq(cpu_mreq),
        .cpu_rd(cpu_rd),
        .cpu_wr(cpu_wr),
        .ppi_a_latch(),
        .ppi_b_latch(),
        .plus_hcc(),
        .plus_r0(),
        .plus_row(),
        .plus_raster(),
        .plus_cclk(),
        .video_word(video_word),
        .video_addr(video_addr),
        .video_rgb(),
        .video_pen(),
        .video_hs(),
        .video_vs(),
        .video_de()
    );

    // Expose classic raw GA RGB {red, green, blue}
    assign classic_rgb = {machine.motherboard.ga_red, machine.motherboard.ga_green, machine.motherboard.ga_blue};
    assign classic_raw_blank = machine.motherboard.GateArray.HCNTLT28 | machine.motherboard.crtc_hs;

    // CRTC counter and timing status
    assign c0 = machine.motherboard.crtc.hcc;
    assign c4 = machine.motherboard.crtc.row;
    assign c9 = machine.motherboard.crtc.line;
    assign hsc = machine.motherboard.crtc.hsc;
    assign raw_hs = machine.motherboard.crtc_hs;
    assign dbg_char_phase = machine.motherboard.crtc.hsync_char_phase;
    assign dbg_write = machine.motherboard.crtc.register_write;
    assign dbg_write_d = machine.motherboard.crtc.register_write_d;
    assign dbg_jit_write = machine.motherboard.crtc.r2_jit_write;
    assign crtc_reg = machine.motherboard.crtc.addr;
    assign r2_value = machine.motherboard.crtc.R2_h_sync_pos;
    assign r3_value = {machine.motherboard.crtc.R3_v_sync_width, machine.motherboard.crtc.R3_h_sync_width};

    // GA status and serializer observation
    assign seq = machine.motherboard.GateArray.S;
    assign ready = machine.motherboard.ready;
    assign ce16 = machine.ce_16;
    assign ga_load = machine.motherboard.GateArray.video_sync.load;
    assign video_buf = machine.motherboard.GateArray.VIDEO_BUF;
    assign shift_reg = machine.motherboard.GateArray.video_sync.shift_reg;
    assign ga_ctrl_accept = machine.motherboard.GateArray.ctrl_en;
    assign ga_data = machine.motherboard.GateArray.D;

    // Mode and palette status
    assign effective_decode = {machine.motherboard.GateArray.video_sync.mode_is_2,
                               machine.motherboard.GateArray.video_sync.mode_is_1,
                               machine.motherboard.GateArray.video_sync.mode_is_0};
    assign effective_mode = effective_decode[2] ? 2'd2 :
                            effective_decode[1] ? 2'd1 :
                            effective_decode[0] ? 2'd0 : 2'd3;
    assign requested_mode = {machine.motherboard.GateArray.mode1, machine.motherboard.GateArray.mode0};
    assign mode_sync = machine.motherboard.GateArray.mode_sync_en;
    assign observed_inks = machine.snap_ga_inkr;
endmodule
