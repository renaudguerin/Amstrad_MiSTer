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
    input [1:0] filter_request,
    input save_req, cart_ack,
    input [7:0] cart_dout,
    output cart_req, captured, save_hold,
    output [22:0] cart_addr,
    output [2047:0] saved_header,
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
    output [5:0] public_rgb,
    output [7:0] r0_value, r1_value, crtc_data, byte_out,
    output crtc_data_write, raw_vs, raw_de, outgoing_de, force_blank,
    output ce4, fetch, bs, shift, filtered_hs, filtered_vs, hblank, vblank,
    output selected_hs, selected_hb, selected_vb, filter_nohs, filter_commit,
    output border_accept, ink_accept, end_gen,
    output [4:0] palette_target, palette_data,
    output [8:0] filter_size,
    output [1:0] applied_mode,
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
        .filter_request(filter_request),
        .model(2'd0),
        .crtc_type(crtc_type),
        .save_req(save_req),
        .sna_download(sna_download),
        .load_wr(load_wr),
        .load_addr(load_addr),
        .load_data(load_data),
        .mem_din(mem_din),
        .cart_dout(cart_dout),
        .cart_ack(cart_ack),
        .ddram_busy(1'b0),
        .mem_addr(mem_addr),
        .mem_bank(),
        .mem_rd(mem_rd),
        .mem_wr(mem_wr),
        .mem_dout(mem_dout),
        .load_mem_wr(load_mem_wr),
        .load_mem_addr(load_mem_addr),
        .load_mem_bank(),
        .cart_req(cart_req),
        .cart_addr(cart_addr),
        .cart_bank(),
        .ddram_addr(),
        .ddram_din(),
        .ddram_we(),
        .captured(captured),
        .save_hold(save_hold),
        .save_done(),
        .sna_load(sna_load),
        .sna_hold(sna_hold),
        .owner_reset(owner_reset),
        .cpu_reg(),
        .cpu_addr(cpu_addr),
        .cpu_insn_start(cpu_insn_start),
        .cpu_halt_n(),
        .hw_header(),
        .saved_header(saved_header),
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

    // Public motherboard RGB is separate from the old GA-source oracle.
    assign public_rgb = {machine.motherboard.red[1:0], machine.motherboard.green[1:0], machine.motherboard.blue[1:0]};
    assign r0_value = machine.motherboard.crtc.R0_h_total;
    assign r1_value = machine.motherboard.crtc.R1_h_displayed;
    assign crtc_data = machine.motherboard.D;
    assign crtc_data_write = machine.motherboard.crtc.reg_data_write;
    assign byte_out = machine.motherboard.vram_d;
    assign raw_vs = machine.motherboard.crtc_vs;
    assign raw_de = machine.motherboard.crtc_de;
    assign outgoing_de = machine.motherboard.GateArray.video_sync.u1005;
    assign force_blank = machine.motherboard.GateArray.video_sync.FORCE_BLANK;
    assign ce4 = machine.motherboard.phi_en_n;
    assign fetch = machine.motherboard.cpu_n && !machine.motherboard.ras_n && !machine.motherboard.cas_n;
    assign bs = machine.motherboard.vram_bs;
    assign shift = machine.motherboard.crtc_shift;
    assign filtered_hs = machine.motherboard.hsync_filtered;
    assign filtered_vs = machine.motherboard.vsync_filtered;
    assign hblank = machine.motherboard.hblank_filtered;
    assign vblank = machine.motherboard.vblank_filtered;
    assign selected_hs = machine.motherboard.hsync;
    assign selected_hb = machine.motherboard.hblank;
    assign selected_vb = machine.motherboard.vblank;
    assign filter_nohs = machine.motherboard.crt_filter.no_hsync;
    assign filter_size = machine.motherboard.crt_filter.hSyncSize;
    assign applied_mode = machine.motherboard.sync_filter_applied;
    assign filter_commit = machine.motherboard.sync_filter_commit;
    assign border_accept = machine.motherboard.GateArray.border_en;
    assign ink_accept = machine.motherboard.GateArray.inkr_en;
    assign palette_target = machine.motherboard.GateArray.inksel;
    assign palette_data = machine.motherboard.GateArray.D[4:0];
    assign end_gen = machine.motherboard.crtc_end_gen;

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
