// Production motherboard and GHDL-translated T80. External memory is supplied
// by C++; no SDRAM controller, cartridge mux, firmware, or physical device model.
module ssm_provider_top #(parameter CRTC_KIND=0) (
 input clk, reset, irq, nmi, extra_wait, plus_mode,
 input [7:0] cpu_din,
 output [15:0] cpu_addr,
 output [7:0] cpu_dout,
 output [22:0] mem_addr,
 output mem_rd, mem_wr, romen, cpu_halt_n, iorq, mreq, rd, m1,
 output ssm_m1_fetch,
 output [7:0] ssm_bus_data,
 output [20:0] ssm_sync_observation,
 output wait_n,
 output [15:0] last_code,
 output [31:0] event_count,
 output event_stb
`ifdef SSM_S_PASSIVE_TRACE
 , input s_trace_enable, s_trace_busy
 , output [127:0] s_trace_machine
 , output [15:0] s_trace_rgb
 , output [28:0] s_trace_ddr_addr
 , output [63:0] s_trace_ddr_data
 , output s_trace_ddr_we
`endif
);
 reg [1:0] div = 0;
 reg ce_16 = 0;
 always @(posedge clk) begin
   div <= div + 1'b1;
   ce_16 <= div == 0;
 end
`ifdef SSM_S_PASSIVE_TRACE
 wire [6:0] trace_hidden, trace_row;
 wire [4:0] trace_line, trace_c5, trace_sel;
 wire [7:0] trace_hcc, trace_ppi;
 wire [127:0] trace_regs;
 wire trace_adj, trace_vs;
 assign s_trace_rgb[15]=ce_16;
 assign s_trace_machine={33'd0,trace_ppi,trace_regs[79:72],trace_regs[71:64],
 trace_regs[63:56],trace_regs[47:40],trace_regs[39:32],trace_regs[7:0],
 trace_sel,trace_vs,trace_hidden,trace_adj,trace_c5,trace_line,trace_row,trace_hcc};
`endif
 Amstrad_motherboard mb (
 .reset(reset),
 .clk(clk),
 .ce_16(ce_16),
 .plus_mode(plus_mode),
 .plus_unlocked('0),
 .plus_ram_128k('0),
 .plus_has_fdc('0),
 .plus_has_tape('0),
 .plus_mem_wait(extra_wait),
 .plus_aspage_on('0),
 .plus_asic_dout(),
 .plus_asic_rd(),
 .plus_vec_byte(),
 .plus_vec_valid(),
 .joy1('0),
 .joy2('0),
 .right_shift_mod('0),
 .keypad_mod('0),
 .ps2_key('0),
 .ps2_mouse('0),
 .joy1_sel(),
 .joy2_sel(),
 .key_nmi(),
 .key_reset(),
 .Fn(),
 .ppi_jumpers(4'b1010),
 .crtc_type(CRTC_KIND[0]),
 .sync_filter('0),
 .no_wait('0),
 .sna_load('0),
 .sna_cpu_dir('0),
 .sna_crtc_addr('0),
 .sna_crtc_regs('0),
 .sna_ga_inksel('0),
 .sna_ga_palette('0),
 .sna_ga_config('0),
 .sna_ram_config('0),
 .sna_rom_select('0),
 .sna_ppi_a('0),
 .sna_ppi_b('0),
 .sna_ppi_c('0),
 .sna_ppi_control('0),
 .sna_psg_addr('0),
 .sna_psg_regs('0),
 .sna_hold('0),
 .save_hold('0),
 .sna_hsync('0),
 .sna_dma_loop_cnt0('0),
 .sna_dma_loop_cnt1('0),
 .sna_dma_loop_cnt2('0),
 .sna_dma_loop_addr0('0),
 .sna_dma_loop_addr1('0),
 .sna_dma_loop_addr2('0),
 .sna_dma_pause_cnt0('0),
 .sna_dma_pause_cnt1('0),
 .sna_dma_pause_cnt2('0),
 .sna_dma_pause_presc0('0),
 .sna_dma_pause_presc1('0),
 .sna_dma_pause_presc2('0),
 .sna_crtc_hcc('0),
 .sna_crtc_line('0),
 .sna_crtc_raster('0),
 .sna_crtc_vta('0),
 .sna_crtc_hsw('0),
 .sna_crtc_vsw('0),
 .sna_crtc_vs('0),
 .sna_crtc_adj('0),
 .sna_ga_vsdelay('0),
 .sna_ga_intcnt('0),
 .sna_int_pending('0),
 .sna_plus_chunk('0),
 .plus_sna_wr('0),
 .plus_sna_addr('0),
 .plus_sna_data('0),
 .plus_asic_reset('0),
 .tape_in(1'b1),
 .tape_out(),
 .tape_motor(),
 .audio_l(),
 .audio_r(),
 .mode(),
 `ifdef SSM_S_PASSIVE_TRACE
 .red(s_trace_rgb[11:8]), .green(s_trace_rgb[7:4]), .blue(s_trace_rgb[3:0]),
`else
 .red(), .green(), .blue(),
`endif
 .ssm_m1_fetch(ssm_m1_fetch),
 .ssm_bus_data(ssm_bus_data),
 .ssm_sync_observation(ssm_sync_observation),
 .hblank(),
 .vblank(),
`ifdef SSM_S_PASSIVE_TRACE
 .hsync(s_trace_rgb[14]), .vsync(s_trace_rgb[13]), .field(s_trace_rgb[12]),
`else
 .hsync(), .vsync(), .field(),
`endif
 .raw_crt(),
 .pixel_vblank(),
 .vram_din('0),
 .vram_addr(),
 .rom_map('0),
 .ram64k('0),
 .mem_addr(mem_addr),
 .mem_rd(mem_rd),
 .mem_wr(mem_wr),
 .romen(romen),
 .phi_n(),
 .phi_en_n(),
 .phi_en_p(),
 .cpu_addr(cpu_addr),
 .cpu_dout(cpu_dout),
 .cpu_din(cpu_din),
 .iorq(iorq),
 .asic_iorq(),
 .mreq(mreq),
 .rd(rd),
 .wr(),
 .m1(m1),
 .io_bus_byte(),
 .ga_ready(),
 .irq(irq),
 .nmi(nmi),
 .cursor(),
 .snap_ga_inksel(),
 .snap_ga_border(),
 .snap_ga_inkr(),
 .snap_ga_hromen(),
 .snap_ga_lromen(),
 .snap_ga_mode(),
 .snap_ga_intcnt(),
 .snap_ga_hcnt(),
 .snap_ga_int_n(),
`ifdef SSM_S_PASSIVE_TRACE
 .s_trace_crtc_state(trace_hidden),
 .snap_crtc_addr(trace_sel), .snap_crtc_regs(trace_regs),
 .snap_crtc_hcc(trace_hcc), .snap_crtc_row(trace_row), .snap_crtc_line(trace_line),
 .snap_crtc_c5(trace_c5), .snap_crtc_in_adj(trace_adj),
`else
 .snap_crtc_addr(),
 .snap_crtc_regs(),
 .snap_crtc_hcc(),
 .snap_crtc_row(),
 .snap_crtc_line(),
 .snap_crtc_c5(),
 .snap_crtc_in_adj(),
`endif
 .snap_crtc_hsc(),
 .snap_crtc_hsync(),
`ifdef SSM_S_PASSIVE_TRACE
 .snap_crtc_vsync_r(trace_vs),
`else
 .snap_crtc_vsync_r(),
`endif
 .snap_crtc_vsw_elapsed(),
 .snap_mmu_rammap(),
 .snap_mmu_rampage(),
 .snap_mmu_rom_select_shadow(),
 .snap_ppi_porta_in(),
`ifdef SSM_S_PASSIVE_TRACE
 .snap_ppi_portb_in(trace_ppi),
`else
 .snap_ppi_portb_in(),
`endif
 .snap_ppi_opc_r(),
 .snap_ppi_mode(),
 .snap_psg_addr(),
 .snap_psg_regs(),
 .snap_printer_data(),
 .cpu_reg(),
 .cpu_insn_start(),
 .cpu_halt_n(cpu_halt_n)
 );
 assign wait_n = mb.CPU.WAIT_n;
 ssm_marker marker (
 .clk(clk), .reset(reset), .enable(1'b1),
 .m1_fetch(ssm_m1_fetch), .bus_data(ssm_bus_data),
 .sync_observation(ssm_sync_observation),
 .ce_pix(ce_16), .hsync(1'b0), .vsync(1'b0), .field(1'b0),
 .last_code(last_code), .event_count(event_count), .event_stb(event_stb),
 .dropped_count(), .ddram_be(), .ddram_burstcnt(),
`ifdef SSM_S_PASSIVE_TRACE
 .s_trace_enable(s_trace_enable), .s_trace_addr(cpu_addr),
 .s_trace_qualified(!plus_mode && CRTC_KIND==1 && !romen && mem_addr=={7'd2,cpu_addr}),
 .s_trace_snapshot(s_trace_machine),
 .ddram_addr(s_trace_ddr_addr), .ddram_din(s_trace_ddr_data),
 .ddram_we(s_trace_ddr_we), .ddram_busy(s_trace_busy)
`else
 .ddram_addr(), .ddram_din(), .ddram_we(), .ddram_busy(1'b0)
`endif
 );
endmodule
