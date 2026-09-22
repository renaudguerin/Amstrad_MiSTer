// Combined classic SNA save/reload fixture. All owners and byte decoders are
// production RTL; C++ supplies byte-addressed RAM and the DDR write sink.
// FDC is intentionally absent: motor and physical tracks are fixed at zero.
// Video fetch pixels are zero; real GA/CRTC timing still runs during save.
// Only standard uncompressed classic snapshots are accepted (no chunks).
module b18_snapshot_top (
    input clk, reset_btn,
    input [1:0] model,
    input crtc_type, save_req, sna_download, load_wr,
    input [24:0] load_addr,
    input [7:0] load_data, mem_din, cart_dout,
    input cart_ack, ddram_busy,
    output [22:0] mem_addr,
    output [1:0] mem_bank,
    output mem_rd, mem_wr,
    output [7:0] mem_dout,
    output load_mem_wr,
    output [22:0] load_mem_addr,
    output [1:0] load_mem_bank,
    output cart_req,
    output [22:0] cart_addr,
    output [1:0] cart_bank,
    output [28:0] ddram_addr,
    output [63:0] ddram_din,
    output ddram_we, captured, save_hold, save_done, sna_load, sna_hold,
    output reg owner_reset = 1'b0,
    output [211:0] cpu_reg,
    output [15:0] cpu_addr,
    output cpu_insn_start, cpu_halt_n,
    output [1079:0] hw_header,
    output [2047:0] saved_header,
    output reg [1:0] active_model = 2'd0,
    output cpu_iorq, cpu_mreq, cpu_rd, cpu_wr,
    output [7:0] ppi_a_latch, ppi_b_latch
);
    // Production 64 MHz divider; neither phase is reset by a snapshot.
    reg [2:0] div = 3'd0;
    reg ce_16 = 1'b0, ce_ref = 1'b0;
    always @(posedge clk) begin
        div <= div + 1'd1;
        ce_ref <= !div;
        ce_16 <= !div[1:0];
    end

    wire owner_reset_hold;
    wire [1:0] sna_hdr_model;
    always @(posedge clk) begin
        owner_reset <= reset_btn | owner_reset_hold;
        if (sna_load) active_model <= sna_hdr_model;
        else if (owner_reset) active_model <= model;
    end
    assign mem_bank = sna_load ? sna_hdr_model : active_model;
    wire mb_mem_rd, mb_mem_wr;
    assign mem_rd = !owner_reset && mb_mem_rd;
    assign mem_wr = !owner_reset && mb_mem_wr;
    wire [7:0] cpu_din = mem_rd ? mem_din : 8'hff;
    assign ppi_a_latch = motherboard.PPI.opa_r;
    assign ppi_b_latch = motherboard.PPI.opb_r;

    // Standard uncompressed classic SNA data region, matching Amstrad.sv.
    // The C++ RAM retires each accepted byte on this edge, so no physical
    // SDRAM queue/drain remains outstanding beyond load_mem_wr.
    wire [15:0] sna_mem_size;
    wire [15:0] std_mem_size = sna_mem_size > 16'd128 ? 16'd128 : sna_mem_size;
    wire [24:0] payload_addr = load_addr - 25'h100;
    wire [24:0] chunk_start = 25'h100 + {std_mem_size[14:0], 10'd0};
    assign load_mem_wr = sna_download && load_wr && load_addr >= 25'h100 &&
                         load_addr < chunk_start && {9'd0, payload_addr[16:10]} < std_mem_size;
    assign load_mem_addr = {9'd8 + {6'd0, payload_addr[16:14]}, payload_addr[13:0]};
    assign load_mem_bank = sna_hdr_model;
    wire header_wr = sna_download && load_wr && load_addr < 25'h100;
    wire [211:0] sna_cpu_dir;
    wire [4:0] sna_crtc_addr;
    wire [143:0] sna_crtc_regs;
    wire [4:0] sna_ga_inksel;
    wire [135:0] sna_ga_palette;
    wire [7:0] sna_ga_config;
    wire [7:0] sna_ram_config;
    wire [7:0] sna_rom_select;
    wire [7:0] sna_ppi_a;
    wire [7:0] sna_ppi_b;
    wire [7:0] sna_ppi_c;
    wire [7:0] sna_ppi_control;
    wire [3:0] sna_psg_addr;
    wire [127:0] sna_psg_regs;
    wire  sna_hsync;
    wire [7:0] sna_crtc_hcc;
    wire [6:0] sna_crtc_line;
    wire [4:0] sna_crtc_raster;
    wire [4:0] sna_crtc_vta;
    wire [3:0] sna_crtc_hsw;
    wire [3:0] sna_crtc_vsw;
    wire  sna_crtc_vs;
    wire  sna_crtc_adj;
    wire [1:0] sna_ga_vsdelay;
    wire [5:0] sna_ga_intcnt;
    wire  sna_int_pending;
    wire [4:0] snap_ga_inksel;
    wire [4:0] snap_ga_border;
    wire [79:0] snap_ga_inkr;
    wire  snap_ga_hromen;
    wire  snap_ga_lromen;
    wire [1:0] snap_ga_mode;
    wire [5:0] snap_ga_intcnt;
    wire [4:0] snap_ga_hcnt;
    wire  snap_ga_int_n;
    wire [4:0] snap_crtc_addr;
    wire [127:0] snap_crtc_regs;
    wire [7:0] snap_crtc_hcc;
    wire [6:0] snap_crtc_row;
    wire [4:0] snap_crtc_line;
    wire [4:0] snap_crtc_c5;
    wire  snap_crtc_in_adj;
    wire [3:0] snap_crtc_hsc;
    wire  snap_crtc_hsync;
    wire  snap_crtc_vsync_r;
    wire [3:0] snap_crtc_vsw_elapsed;
    wire [2:0] snap_mmu_rammap;
    wire [4:0] snap_mmu_rampage;
    wire [7:0] snap_mmu_rom_select_shadow;
    wire [7:0] snap_ppi_porta_in;
    wire [7:0] snap_ppi_portb_in;
    wire [7:0] snap_ppi_opc_r;
    wire [7:0] snap_ppi_mode;
    wire [7:0] snap_psg_addr;
    wire [127:0] snap_psg_regs;
    wire [7:0] snap_printer_data;

plus_sna_header sna_header
(
	.clk(clk),
	.sna_download(sna_download),
	.wr(header_wr),
	.addr(load_addr[7:0]),
	.data(load_data),

	.version(),
	.v3(),

	.ga_inksel(sna_ga_inksel),
	.ga_palette(sna_ga_palette),
	.ga_config(sna_ga_config),
	.crtc_addr(sna_crtc_addr),
	.crtc_regs(sna_crtc_regs),
	.rom_select(sna_rom_select),

	.crtc_hcc(sna_crtc_hcc),
	.crtc_line(sna_crtc_line),
	.crtc_raster(sna_crtc_raster),
	.crtc_vta(sna_crtc_vta),
	.crtc_hsw(sna_crtc_hsw),
	.crtc_vsw(sna_crtc_vsw),
	.crtc_vs(sna_crtc_vs),
	.crtc_hs(sna_hsync),
	.crtc_adj(sna_crtc_adj),
	.ga_vsdelay(sna_ga_vsdelay),
	.ga_intcnt(sna_ga_intcnt),
	.int_pending(sna_int_pending)
);

sna_cpu_header sna_cpu_header
(
	.clk(clk),
	.sna_download(sna_download),
	.wr(header_wr),
	.addr(load_addr[7:0]),
	.data(load_data),
	.menu_model(model),

	.cpu_dir(sna_cpu_dir),
	.ram_config(sna_ram_config),
	.ppi_a(sna_ppi_a),
	.ppi_b(sna_ppi_b),
	.ppi_c(sna_ppi_c),
	.ppi_control(sna_ppi_control),
	.psg_addr(sna_psg_addr),
	.psg_regs(sna_psg_regs),
	.mem_size(sna_mem_size),
	.model(sna_hdr_model)
);

    plus_sna_apply apply_controller (
        .clk(clk), .sna_download(sna_download), .romdl_wait(1'b0),
        .boot_wr(load_mem_wr), .sna_rle_count(8'd0), .plus_sna_busy(1'b0),
        .finish_pending(), .apply_cnt(), .sna_load(sna_load),
        .sna_hold(sna_hold), .owner_reset_hold(owner_reset_hold)
    );

    Amstrad_motherboard motherboard (
        .reset(owner_reset),
        .clk(clk),
        .ce_16(ce_16),
        .plus_mode('0),
        .plus_unlocked('0),
        .plus_ram_128k('0),
        .plus_has_fdc('0),
        .plus_has_tape('0),
        .plus_mem_wait('0),
        .plus_aspage_on('0),
        .plus_asic_dout(),
        .plus_asic_rd(),
        .plus_vec_byte(),
        .plus_vec_valid(),
        .joy1(7'b0000001),
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
        .crtc_type(crtc_type),
        .sync_filter('0),
        .no_wait('0),
        .sna_load(sna_load),
        .sna_cpu_dir(sna_cpu_dir),
        .sna_crtc_addr(sna_crtc_addr),
        .sna_crtc_regs(sna_crtc_regs),
        .sna_ga_inksel(sna_ga_inksel),
        .sna_ga_palette(sna_ga_palette),
        .sna_ga_config(sna_ga_config),
        .sna_ram_config(sna_ram_config),
        .sna_rom_select(sna_rom_select),
        .sna_ppi_a(sna_ppi_a),
        .sna_ppi_b(sna_ppi_b),
        .sna_ppi_c(sna_ppi_c),
        .sna_ppi_control(sna_ppi_control),
        .sna_psg_addr(sna_psg_addr),
        .sna_psg_regs(sna_psg_regs),
        .sna_hold(sna_hold),
        .save_hold(save_hold),
        .sna_hsync(sna_hsync),
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
        .sna_crtc_hcc(sna_crtc_hcc),
        .sna_crtc_line(sna_crtc_line),
        .sna_crtc_raster(sna_crtc_raster),
        .sna_crtc_vta(sna_crtc_vta),
        .sna_crtc_hsw(sna_crtc_hsw),
        .sna_crtc_vsw(sna_crtc_vsw),
        .sna_crtc_vs(sna_crtc_vs),
        .sna_crtc_adj(sna_crtc_adj),
        .sna_ga_vsdelay(sna_ga_vsdelay),
        .sna_ga_intcnt(sna_ga_intcnt),
        .sna_int_pending(sna_int_pending),
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
        .red(),
        .green(),
        .blue(),
        .ssm_m1_fetch(),
        .ssm_bus_data(),
        .hblank(),
        .vblank(),
        .hsync(),
        .vsync(),
        .field(),
        .raw_crt(),
        .pixel_vblank(),
        .vram_din('0),
        .vram_addr(),
        .rom_map(256'd0),
        .ram64k(active_model != 2'd0),
        .mem_addr(mem_addr),
        .mem_rd(mb_mem_rd),
        .mem_wr(mb_mem_wr),
        .romen(),
        .phi_n(),
        .phi_en_n(),
        .phi_en_p(),
        .cpu_addr(cpu_addr),
        .cpu_dout(mem_dout),
        .cpu_din(cpu_din),
        .iorq(cpu_iorq),
        .mreq(cpu_mreq),
        .rd(cpu_rd),
        .wr(cpu_wr),
        .m1(),
        .io_bus_byte(),
        .ga_ready(),
        .irq(1'b0),
        .nmi(1'b0),
        .cursor(),
        .snap_ga_inksel(snap_ga_inksel),
        .snap_ga_border(snap_ga_border),
        .snap_ga_inkr(snap_ga_inkr),
        .snap_ga_hromen(snap_ga_hromen),
        .snap_ga_lromen(snap_ga_lromen),
        .snap_ga_mode(snap_ga_mode),
        .snap_ga_intcnt(snap_ga_intcnt),
        .snap_ga_hcnt(snap_ga_hcnt),
        .snap_ga_int_n(snap_ga_int_n),
        .snap_crtc_addr(snap_crtc_addr),
        .snap_crtc_regs(snap_crtc_regs),
        .snap_crtc_hcc(snap_crtc_hcc),
        .snap_crtc_row(snap_crtc_row),
        .snap_crtc_line(snap_crtc_line),
        .snap_crtc_c5(snap_crtc_c5),
        .snap_crtc_in_adj(snap_crtc_in_adj),
        .snap_crtc_hsc(snap_crtc_hsc),
        .snap_crtc_hsync(snap_crtc_hsync),
        .snap_crtc_vsync_r(snap_crtc_vsync_r),
        .snap_crtc_vsw_elapsed(snap_crtc_vsw_elapsed),
        .snap_mmu_rammap(snap_mmu_rammap),
        .snap_mmu_rampage(snap_mmu_rampage),
        .snap_mmu_rom_select_shadow(snap_mmu_rom_select_shadow),
        .snap_ppi_porta_in(snap_ppi_porta_in),
        .snap_ppi_portb_in(snap_ppi_portb_in),
        .snap_ppi_opc_r(snap_ppi_opc_r),
        .snap_ppi_mode(snap_ppi_mode),
        .snap_psg_addr(snap_psg_addr),
        .snap_psg_regs(snap_psg_regs),
        .snap_printer_data(snap_printer_data),
        .cpu_reg(cpu_reg),
        .cpu_insn_start(cpu_insn_start),
        .cpu_halt_n(cpu_halt_n)
    );

sna_hw_header save_hw_header
(
	.ga_inksel(snap_ga_inksel),
	.ga_border(snap_ga_border),
	.ga_inkr(snap_ga_inkr),
	.ga_hromen(snap_ga_hromen),
	.ga_lromen(snap_ga_lromen),
	.ga_mode(snap_ga_mode),
	.ga_intcnt(snap_ga_intcnt),
	.ga_hcnt(snap_ga_hcnt),
	.ga_int_n(snap_ga_int_n),

	.mmu_rammap(snap_mmu_rammap),
	.mmu_rom_select_shadow(snap_mmu_rom_select_shadow),

	.crtc_addr(snap_crtc_addr),
	.crtc_regs(snap_crtc_regs),
	.crtc_hcc(snap_crtc_hcc),
	.crtc_row(snap_crtc_row),
	.crtc_line(snap_crtc_line),
	.crtc_c5(snap_crtc_c5),
	.crtc_in_adj(snap_crtc_in_adj),
	.crtc_hsc(snap_crtc_hsc),
	.crtc_hsync(snap_crtc_hsync),
	.crtc_vsync_r(snap_crtc_vsync_r),
	.crtc_vsw_elapsed(snap_crtc_vsw_elapsed),

	.ppi_porta_in(snap_ppi_porta_in),
	.ppi_portb_in(snap_ppi_portb_in),
	.ppi_opc_r(snap_ppi_opc_r),
	.ppi_mode(snap_ppi_mode),

	.psg_addr(snap_psg_addr),
	.psg_regs(snap_psg_regs),

	.printer_data(snap_printer_data),
	.model(active_model),
	.crtc_type(crtc_type),
	.fdc_motor(1'b0),
	.fdc_pcn_a(8'd0),
	.fdc_pcn_b(8'd0),

	.hdr(hw_header)
);

    wire [2047:0] save_header;
    assign saved_header = save_header;
    wire save_abort = owner_reset | sna_download;
    sna_save_capture capture (
        .clk(clk), .reset(save_abort), .save_req(save_req),
        .admit(!owner_reset && !sna_download && !sna_hold && !sna_load),
        .rampage_ok(snap_mmu_rampage == 5'd3), .release_req(save_done),
        .insn_start(cpu_insn_start), .halt_n(cpu_halt_n),
        .cpu_reg(cpu_reg), .hw_hdr(hw_header), .hold(save_hold),
        .captured(captured), .refused(), .cancelled(), .busy(), .header(save_header)
    );
    // This fixture is the sole DDR and cart master; their arbitration and
    // physical SDRAM timing have independent production-boundary benches.
    wire ddr_request;
    sna_save_stream stream (
        .clk(clk), .reset(save_abort), .start(captured),
        .ram128(active_model == 2'd0), .bank(mem_bank), .header(save_header),
        .clkref(ce_ref), .cart_req(cart_req), .cart_bank(cart_bank),
        .cart_addr(cart_addr), .cart_dout(cart_dout), .cart_ack(cart_ack),
        .ddr_grant(ddr_request), .ddr_request(ddr_request),
        .ddram_addr(ddram_addr), .ddram_din(ddram_din), .ddram_be(),
        .ddram_burstcnt(), .ddram_we(ddram_we), .ddram_busy(ddram_busy),
        .done(save_done), .active()
    );
endmodule
