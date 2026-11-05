// Passive ASIC public-RGB observation composition. Production motherboard/ASIC/T80
// physics are unchanged. Technical information sourced from the "Amstrad CPC
// CRTC Compendium" by Longshot (CC BY-NC-ND 4.0), FR v1.11 §14.4 pp134-135.
module asic_r3_position_top (
 input clk, reset_btn, sna_download, load_wr,
 input [1:0] filter_request,
 input [24:0] load_addr,
 input [7:0] load_data, mem_din,
 input [15:0] video_word,
 output [22:0] mem_addr, load_mem_addr,
 output [7:0] mem_dout,
 output mem_rd, mem_wr, load_mem_wr, sna_load, sna_hold, owner_reset,
 output [15:0] cpu_addr,
 output cpu_insn_start, cpu_halt_n,
 output [14:0] video_addr,
 output [11:0] public_rgb, source_rgb,
 output [7:0] c0, r0, r1, r2, r3, bus_data, video_byte,
 output [6:0] c4,
 output [4:0] c9, crtc_addr, pen, palette_addr,
 output raw_hs, raw_vs, raw_de, eff_de, blank, selected_hs, selected_hb, selected_vb,
 output filtered_hs, filtered_vs, filtered_hb, filtered_vb, ce16, ce4, crtc_write,
 output filter_start, filter_end, filter_extra, filter_nohs, filter_shift,
 output [8:0] filter_size,
 output [1:0] filter_applied, mode_requested, mode_effective,
 output [11:0] palette_rgb,
 output sprite_en, ready,
 output [15:0] filter_observation
);
 b18_snapshot_top #(.PLUS_MODE(1'b1), .VIDEO_RAM(1'b1)) machine (
 .clk(clk), .reset_btn(reset_btn), .pause(1'b0), .filter_request(filter_request), .model(2'd0), .crtc_type(1'b1),
 .save_req(1'b0), .sna_download(sna_download), .load_wr(load_wr),
 .load_addr(load_addr), .load_data(load_data), .mem_din(mem_din),
 .cart_dout(8'hff), .cart_ack(1'b0), .ddram_busy(1'b0),
 .mem_addr(mem_addr), .mem_dout(mem_dout), .mem_rd(mem_rd), .mem_wr(mem_wr),
 .load_mem_wr(load_mem_wr), .load_mem_addr(load_mem_addr),
 .sna_load(sna_load), .sna_hold(sna_hold), .owner_reset(owner_reset),
 .cpu_addr(cpu_addr), .cpu_insn_start(cpu_insn_start), .cpu_halt_n(cpu_halt_n),
 .video_word(video_word), .video_addr(video_addr)
 );
 assign public_rgb = {machine.motherboard.red,machine.motherboard.green,machine.motherboard.blue};
 assign source_rgb = {machine.motherboard.plus_rgb_r,machine.motherboard.plus_rgb_g,machine.motherboard.plus_rgb_b};
 assign c0 = machine.motherboard.asic_vid.hcc;
 assign c4 = machine.motherboard.asic_vid.charline;
 assign c9 = machine.motherboard.asic_vid.raster;
 assign r0 = machine.motherboard.asic_vid.R0_h_total;
 assign r1 = machine.motherboard.asic_vid.R1_h_displayed;
 assign r2 = machine.motherboard.asic_vid.R2_h_sync_pos;
 assign r3 = {machine.motherboard.asic_vid.R3_v_sync_width,machine.motherboard.asic_vid.R3_h_sync_width};
 assign crtc_addr = machine.motherboard.asic_vid.addr;
 assign crtc_write = machine.motherboard.asic_vid.ENABLE & ~machine.motherboard.asic_vid.nCS & ~machine.motherboard.asic_vid.R_nW & machine.motherboard.asic_vid.RS;
 assign bus_data = machine.motherboard.asic_vid.DI;
 assign raw_hs = machine.motherboard.plus_crtc_hs;
 assign raw_vs = machine.motherboard.plus_crtc_vs;
 assign raw_de = machine.motherboard.plus_crtc_de;
 assign eff_de = machine.motherboard.asic_vid.eff_de;
 assign blank = machine.motherboard.asic_vid.blank;
 assign pen = machine.motherboard.asic_vid.PEN;
 assign video_byte = machine.motherboard.vram_d;
 assign palette_addr = machine.motherboard.plus_pal_raddr;
 assign palette_rgb = machine.motherboard.plus_pal_rdata;
 assign sprite_en = machine.motherboard.plus_spr_en;
 assign ready = machine.motherboard.ready;
 assign mode_requested = machine.motherboard.plus_gamode;
 assign mode_effective = machine.motherboard.asic_vid.mode_q;
 assign selected_hs = machine.motherboard.hsync;
 assign selected_hb = machine.motherboard.hblank;
 assign selected_vb = machine.motherboard.vblank;
 assign filtered_hs = machine.motherboard.hsync_filtered;
 assign filtered_vs = machine.motherboard.vsync_filtered;
 assign filtered_hb = machine.motherboard.hblank_filtered;
 assign filtered_vb = machine.motherboard.vblank_filtered;
 assign ce16 = machine.ce_16;
 assign ce4 = machine.motherboard.phi_en_n;
 assign filter_start = machine.motherboard.crtc_start_gen;
 assign filter_end = machine.motherboard.crtc_end_gen;
 assign filter_extra = machine.motherboard.crtc_extra_byte;
 assign filter_nohs = machine.motherboard.crt_filter.no_hsync;
 assign filter_size = machine.motherboard.crt_filter.hSyncSize;
 assign filter_shift = machine.motherboard.crtc_shift;
 assign filter_applied = machine.motherboard.sync_filter_applied;
 assign filter_observation = machine.motherboard.filter_sync_observation;
endmodule
