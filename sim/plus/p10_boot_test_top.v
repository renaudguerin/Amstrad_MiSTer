`timescale 1ns/1ps

// ===========================================================================
// P10a Real-T80 Production-Path CPR Boot Harness Top
//
// Connects the production CPR parser, cartridge memory service, real SDRAM,
// plus_mmu, plus_model_select, and Amstrad_motherboard (with real T80pa CPU)
// under production reset sequencing and I/O decode.
// ===========================================================================

module p10_boot_test_top #(
	// Preserve the established P10/B7 Off route by default. A frame-capture
	// target can override this with Full without masking the B7 GA mutation.
	parameter [1:0] SYNC_FILTER = 2'd2
)
(
	input             clk,
	input             clkref,
	input             init,
	input             reset_btn,

	input      [1:0]  plus_model_i, // 0: Off, 1: GX4000, 2: 6128+, 3: 464+

	// CPR / ioctl download stream
	input             cpr_download,
	input             ioctl_wr,
	input      [24:0] ioctl_addr,
	input      [7:0]  ioctl_dout,
	output            ioctl_wait,

	// Physical SDRAM bus to testbench memory model
	input      [15:0] memory_dq,
	input             memory_dq_oe,
	output     [15:0] observed_dq,
	output     [12:0] sdram_a,
	output      [1:0] sdram_ba,
	output            sdram_dqml,
	output            sdram_dqmh,
	output            sdram_nwe,
	output            sdram_nras,
	output            sdram_ncas,

	// Testbench IRQ stimulus
	input             force_irq,
	// Select the production shared CPU/u765 divider for timing diagnostics.
	// The legacy P10a fixture remains selectable to preserve its pinned trace.
	input             production_clocking,

	// Real u765 / MiSTer SD-block interface. The host drives only the media
	// transport; CPU port decode and command execution stay in production RTL.
	input      [1:0]  fdc_img_mounted,
	input             fdc_img_wp,
	input      [31:0] fdc_img_size,
	output     [31:0] fdc_sd_lba,
	output      [1:0] fdc_sd_rd,
	output      [1:0] fdc_sd_wr,
	input             fdc_sd_ack,
	input       [8:0] fdc_sd_buff_addr,
	input       [7:0] fdc_sd_buff_dout,
	output      [7:0] fdc_sd_buff_din,
	input             fdc_sd_buff_wr,

	// Trace and probe outputs for C++ verification
	output reg [15:0] dbg_pc,
	output     [15:0] dbg_addr,
	output      [7:0] dbg_dout,
	output      [7:0] dbg_din,
	output            dbg_m1_n,
	output            dbg_mreq_n,
	output            dbg_iorq_n,
	output            dbg_rd_n,
	output            dbg_wr_n,
	output            dbg_wait_n,

	output            dbg_cart_own,
	output      [4:0] dbg_cart_page,
	output            dbg_cart_stall,
	output      [7:0] dbg_cart_dout,

	output      [7:0] dbg_rmr2,
	output            dbg_asic_page_on,
	output            dbg_unlock_done,
	output      [3:0] dbg_unlock_seq,

	output            dbg_int_n,
	output            dbg_int_ack,
	output      [7:0] dbg_vec_byte,
	output            dbg_vec_valid,

	output reg        dbg_crtc_wr,
	output reg  [4:0] dbg_crtc_reg,
	output reg  [7:0] dbg_crtc_val,

	output reg        dbg_asic_wr,
	output reg [13:0] dbg_asic_addr,
	output reg  [7:0] dbg_asic_val,

	output            dbg_fdc_motor_sel,
	output            dbg_fdc_data_sel,
	output reg        dbg_motor,
	output            dbg_fdc_image_ready,
	output      [7:0] dbg_fdc_dout,
	output      [7:0] dbg_fdc_state,
	output      [7:0] dbg_fdc_msr,
	output     [31:0] dbg_fdc_seek_pos,
	output            dbg_fdc_trackinfo_dirty,
	output     [16:0] dbg_fdc_sector_pos,
	output      [7:0] dbg_fdc_byte_count,
	output      [7:0] dbg_fdc_status0,
	output      [7:0] dbg_fdc_status1,
	output      [7:0] dbg_fdc_status2,
	output reg        dbg_cpu_di_latch,
	output reg [15:0] dbg_cpu_di_addr,
	output reg        dbg_cpu_di_fdc_sel,
	output reg        dbg_cpu_di_io_rd,
	output reg  [7:0] dbg_cpu_di_fdc_dout,
	output reg  [7:0] dbg_cpu_di_expected,
	output reg  [7:0] dbg_cpu_di_fdc_bus,
	output reg  [7:0] dbg_cpu_di_ram_dout,
	output reg  [7:0] dbg_cpu_di_top_bus,
	output reg  [7:0] dbg_cpu_di_mb_bus,
	output reg  [7:0] dbg_cpu_di_msr,
	output reg  [7:0] dbg_cpu_di_fdc_state,
	output      [7:0] dbg_cpu_di_reg,
	output            dbg_reset,
	output      [2:0] dbg_mcycle,
	output      [2:0] dbg_tstate,
	output      [7:0] dbg_ir,
	output      [2:0] dbg_mcmax,
	output            dbg_cen_p,
	output            dbg_cpu_waitn,

	// Simulation-only frame foundations. Raw CRTC/ASIC sync is kept separate
	// from the selected monitor tuple. Payload is intentionally labelled shared:
	// crt_filter.SHIFT can already affect VRAM byte selection before RGB here.
	output            dbg_raw_hsync,
	output            dbg_raw_vsync,
	output            dbg_raw_de,
	output            dbg_selected_hsync,
	output            dbg_selected_vsync,
	output            dbg_selected_hblank,
	output            dbg_selected_vblank,
	output     [11:0] dbg_video_rgb,
	output     [13:0] dbg_video_ma,
	output      [4:0] dbg_video_ra,
	output     [14:0] dbg_video_vram_addr,
	output     [15:0] dbg_video_vram_word,
	output      [7:0] dbg_video_vram_byte,
	output            dbg_sdram_vram_req,
	output            dbg_cart_image_valid,
	output            dbg_cart_service_busy,
	output            dbg_cpr_load_error,
	output      [1:0] dbg_sync_filter
`ifdef B7_DARK_SILICON_MUTATION
	,
	output reg  [4:0] b7_mutation_id,
	output reg        b7_mutation_enable,
	input             b7_crtc_type,
	output     [11:0] b7_rgb,
	output            b7_hsync,
	output            b7_vsync,
	output            b7_de,
	output     [13:0] b7_ma,
	output      [4:0] b7_ra,
	output      [1:0] b7_mode,
	output      [7:0] b7_audio_l,
	output      [7:0] b7_audio_r,
	output            b7_cpu_irq_n
`endif
);

	// 16 MHz dot clock enable from 64 MHz clk
	reg [1:0] cdiv;
	always @(posedge clk) begin
		if (dbg_reset) cdiv <= 2'd0;
		else cdiv <= cdiv + 2'd1;
	end
	wire fixture_ce_16 = (cdiv == 2'd0);

	// Exact Amstrad.sv enable topology: both registered enables are derived
	// from the same free-running three-bit divider.
	reg [2:0] production_div = 3'd0;
	reg       production_ce_16 = 1'b0;
	reg       production_ce_u765 = 1'b0;
	always @(posedge clk) begin
		production_div     <= production_div + 3'd1;
		production_ce_16   <= (production_div[1:0] == 2'd0);
		production_ce_u765 <= (production_div == 3'd0);
	end
	wire ce_16 = production_clocking ? production_ce_16 : fixture_ce_16;

	// Model capabilities
	wire plus_mode;
	wire plus_ram_128k;
	wire plus_has_fdc;
	wire plus_has_tape;

	plus_model_select model_sel (
		.plus_model(plus_model_i),
		.plus_mode(plus_mode),
		.ram_128k(plus_ram_128k),
		.has_fdc(plus_has_fdc),
		.has_tape(plus_has_tape)
	);

	wire plus_gx4000 = (plus_model_i == 2'b01);
	wire plus_exp_n  = 1'b1;

	// Reset sequencing matching Amstrad.sv
	reg [2:0] cpr_apply_cnt = 3'd0;
	reg       cpr_finish_pending = 1'b0;
	reg       old_cpr_download = 1'b0;
	reg       old_cpr_download_reset = 1'b0;

	wire cart_service_busy;
	wire cart_image_valid;

	wire reset_base = reset_btn;
	reg  sys_reset;

	always @(posedge clk) begin
		old_cpr_download <= cpr_download;
		if (old_cpr_download & ~cpr_download) cpr_finish_pending <= 1'b1;
		if (cpr_finish_pending && !cart_service_busy) begin
			cpr_finish_pending <= 1'b0;
			cpr_apply_cnt <= 3'd7;
		end else if (cpr_apply_cnt != 3'd0) begin
			cpr_apply_cnt <= cpr_apply_cnt - 3'd1;
		end

		old_cpr_download_reset <= cpr_download;
		sys_reset <= reset_base | cpr_download | cpr_finish_pending |
		             (old_cpr_download_reset & ~cpr_download) | (cpr_apply_cnt != 3'd0);
	end

	assign dbg_reset = sys_reset;

	// CPR parser & Cartridge Memory Service
	wire        cart_load_begin, cart_load_commit, cart_load_abort;
	wire        cart_load_valid, cart_load_ready, cart_load_error;
	wire  [5:0] cart_load_page;
	wire [14:0] cart_load_offset;
	wire  [7:0] cart_load_data;
	wire        cpr_ioctl_wait;

	assign ioctl_wait = cpr_ioctl_wait;

	plus_cpr_parser cpr_parser (
		.clk(clk),
		.reset(reset_base),
		.cpr_download(cpr_download),
		.ioctl_wr(ioctl_wr),
		.ioctl_addr(ioctl_addr),
		.ioctl_dout(ioctl_dout),
		.ioctl_wait(cpr_ioctl_wait),
		.load_begin(cart_load_begin),
		.load_commit(cart_load_commit),
		.load_abort(cart_load_abort),
		.load_valid(cart_load_valid),
		.load_page(cart_load_page),
		.load_offset(cart_load_offset),
		.load_data(cart_load_data),
		.load_ready(cart_load_ready),
		.load_error(cart_load_error)
	);

	wire        cart_mem_req, cart_mem_write, cart_mem_ack;
	wire  [1:0] cart_mem_bank;
	wire [22:0] cart_mem_addr;
	wire  [7:0] cart_mem_wdata, cart_mem_rdata;

	wire        plus_cart_valid, plus_cart_ready;
	wire  [4:0] plus_cart_page;
	wire [13:0] plus_cart_offset;
	wire  [7:0] plus_cart_data;
	wire        plus_cart_own, plus_cart_stall;
	wire  [7:0] plus_cart_dout;

	plus_cartridge_memory
`ifdef B7_DARK_SILICON_MUTATION
	#(.CLEAR_BYTES(20'd32))
`endif
	cartridge_memory (
		.clk(clk),
		.cold_reset(reset_base),
		.detach(1'b0),
		.load_begin(cart_load_begin),
		.load_commit(cart_load_commit),
		.load_abort(cart_load_abort),
		.load_valid(cart_load_valid),
		.load_page(cart_load_page),
		.load_offset(cart_load_offset),
		.load_data(cart_load_data),
		.load_ready(cart_load_ready),
		.load_error(cart_load_error),
		.cpu_valid(plus_cart_valid),
		.cpu_page(plus_cart_page),
		.cpu_offset(plus_cart_offset),
		.cpu_ready(plus_cart_ready),
		.cpu_data(plus_cart_data),
		.image_valid(cart_image_valid),
		.busy(cart_service_busy),
		.mem_req(cart_mem_req),
		.mem_write(cart_mem_write),
		.mem_bank(cart_mem_bank),
		.mem_addr(cart_mem_addr),
		.mem_wdata(cart_mem_wdata),
		.mem_ack(cart_mem_ack),
		.mem_rdata(cart_mem_rdata)
	);

	// CPU bus & Motherboard signals
	wire [15:0] cpu_addr;
	wire  [7:0] cpu_dout;
	wire  [7:0] cpu_din;
	wire        cpu_m1, cpu_mreq, cpu_iorq, cpu_rd, cpu_wr;
	wire  [7:0] plus_io_bus_byte;
	wire        romen;
	wire        mem_rd, mem_wr;
	wire [22:0] ram_a;
	wire  [7:0] ram_dout;
	wire        plus_aspage_on;
	wire  [7:0] plus_vec_byte;
	wire        plus_vec_valid;
	wire  [7:0] plus_asic_dout;
	wire        plus_asic_rd;
	wire        plus_asic_unlocked;

	wire io_rd = cpu_rd & cpu_iorq;
	wire io_wr = cpu_wr & cpu_iorq;

	// MMU instance
	plus_mmu mmu (
		.clk(clk),
		.reset(sys_reset),
		.plus_mode(plus_mode),
		.gx4000(plus_gx4000),
		.io_rd(io_rd),
		.io_wr(io_wr),
		.mem_rd(mem_rd),
		.A(cpu_addr),
		.D(io_rd ? plus_io_bus_byte : cpu_dout),
		.rom_en(romen),
		.exp_n(plus_exp_n),
		.cart_valid(plus_cart_valid),
		.cart_page(plus_cart_page),
		.cart_offset(plus_cart_offset),
		.cart_ready(plus_cart_ready),
		.cart_data(plus_cart_data),
		.cart_busy(cart_service_busy),
		.cart_own(plus_cart_own),
		.cart_stall(plus_cart_stall),
		.cart_dout(plus_cart_dout),
		.asic_page_on(plus_aspage_on),
		.asic_unlocked(plus_asic_unlocked),
		.sna_load(1'b0),
		.sna_rmr2(8'd0),
		.sna_unlock(1'b0)
	);

	// SDRAM memory controller
	wire [15:0] sdram_dq;
	wire        unused_sdram_clk, unused_sdram_cke, unused_sdram_ncs;
	wire [15:0] vram_dout;
	wire [14:0] vram_addr;
	// The P10 fixture selects the 6128+ Plus model, whose production RAM
	// bank is zero. Keep this as a named bank term so both CPU and video SDRAM
	// clients use the same production-equivalent selector.
	wire  [1:0] mem_bank = 2'b00;

	assign sdram_dq = memory_dq_oe ? memory_dq : 16'hzzzz;
	assign observed_dq = sdram_dq;

	wire plus_aspage_sel = plus_mode & plus_aspage_on &
	                       (mem_rd | mem_wr) & (cpu_addr[15:14] == 2'b01);

	sdram sdram_inst (
		.SDRAM_DQ(sdram_dq),
		.SDRAM_A(sdram_a),
		.SDRAM_DQML(sdram_dqml),
		.SDRAM_DQMH(sdram_dqmh),
		.SDRAM_BA(sdram_ba),
		.SDRAM_nCS(unused_sdram_ncs),
		.SDRAM_nWE(sdram_nwe),
		.SDRAM_nRAS(sdram_nras),
		.SDRAM_nCAS(sdram_ncas),
		.SDRAM_CLK(unused_sdram_clk),
		.SDRAM_CKE(unused_sdram_cke),
		.init(init),
		.clk(clk),
		.clkref(clkref),
		.bank(mem_bank),
		.din(cpu_dout),
		.dout(ram_dout),
		.addr(ram_a),
		.oe(sys_reset ? 1'b0 : mem_rd & ~plus_cart_own & ~plus_aspage_sel),
		.we(sys_reset ? 1'b0 : mem_wr & ~plus_aspage_sel),
		.cart_req(cart_mem_req),
		.cart_wr(cart_mem_write),
		.cart_bank(cart_mem_bank),
		.cart_addr(cart_mem_addr),
		.cart_din(cart_mem_wdata),
		.cart_dout(cart_mem_rdata),
		.cart_ack(cart_mem_ack),
		.vram_dout(vram_dout),
		// Match Amstrad.sv: motherboard vram_addr is a 15-bit word address,
		// mapped into the physical SDRAM video region as {2'b10,addr,1'b0}.
		.vram_addr({5'd0,2'b10,vram_addr,1'b0}),
		.vram_bank(mem_bank),
		.tape_addr(23'd0),
		.tape_din(8'd0),
		.tape_dout(),
		.tape_wr(1'b0),
		.tape_wr_ack(),
		.tape_rd(1'b0),
		.tape_rd_ack()
	);

	// CPU DIN MUX
	wire [7:0] u765_dout;
	wire [7:0] fdc_dout = (dbg_fdc_data_sel & io_rd) ? u765_dout : 8'hFF;
	wire [7:0] cpu_din_bus = ram_dout & fdc_dout;
	assign cpu_din = plus_vec_valid ? plus_vec_byte :
	                 plus_asic_rd   ? plus_asic_dout :
	                 plus_cart_own  ? plus_cart_dout : cpu_din_bus;

	// Motherboard outputs retained by the B7 production-path signature.
	wire [1:0] mb_mode;
	wire [3:0] mb_red;
	wire [3:0] mb_green;
	wire [3:0] mb_blue;
	wire       mb_hblank;
	wire       mb_vblank;
	wire       mb_hsync;
	wire       mb_vsync;
	wire [7:0] mb_audio_l;
	wire [7:0] mb_audio_r;

	Amstrad_motherboard mb (
		.reset(sys_reset),
		.clk(clk),
		.ce_16(ce_16),
		.plus_mode(plus_mode),
		.plus_unlocked(plus_asic_unlocked),
		.plus_ram_128k(plus_ram_128k),
		.plus_has_fdc(plus_has_fdc),
		.plus_has_tape(plus_has_tape),
		.plus_mem_wait(plus_cart_stall),
		.plus_aspage_on(plus_aspage_on),
		.plus_asic_dout(plus_asic_dout),
		.plus_asic_rd(plus_asic_rd),
		.plus_vec_byte(plus_vec_byte),
		.plus_vec_valid(plus_vec_valid),
		.right_shift_mod(1'b0),
		.keypad_mod(1'b0),
		.ps2_key(11'd0),
		.ps2_mouse(25'd0),
		.joy1_sel(),
		.joy2_sel(),
		.key_nmi(),
		.key_reset(),
		.Fn(),
		.no_wait(1'b1),
		.ppi_jumpers(4'd0),
		.crtc_type(
`ifdef B7_DARK_SILICON_MUTATION
			b7_crtc_type
`else
			1'b0
`endif
		),
		.sync_filter(SYNC_FILTER),
		.sna_load(1'b0),
		.sna_cpu_dir(212'd0),
		.sna_crtc_addr(5'd0),
		.sna_crtc_regs(144'd0),
		.sna_ga_inksel(5'd0),
		.sna_ga_palette(136'd0),
		.sna_ga_config(8'd0),
		.sna_ram_config(8'd0),
		.sna_rom_select(8'd0),
		.sna_ppi_a(8'd0),
		.sna_ppi_b(8'd0),
		.sna_ppi_c(8'd0),
		.sna_ppi_control(8'h9b),
		.sna_psg_addr(4'd0),
		.sna_psg_regs(128'd0),
		.plus_sna_wr(1'b0),
		.plus_sna_addr(14'd0),
		.plus_sna_data(8'd0),
		.plus_asic_reset(sys_reset),
		.joy1(7'd0),
		.joy2(7'd0),
		.tape_in(1'b0),
		.tape_out(),
		.tape_motor(),
		.audio_l(mb_audio_l),
		.audio_r(mb_audio_r),
		.mode(mb_mode),
		.hblank(mb_hblank),
		.vblank(mb_vblank),
		.hsync(mb_hsync),
		.vsync(mb_vsync),
		.red(mb_red),
		.green(mb_green),
		.blue(mb_blue),
		.field(),
		.vram_din(vram_dout),
		.vram_addr(vram_addr),
		.rom_map(256'd0),
		.ram64k(!plus_ram_128k),
		.mem_rd(mem_rd),
		.mem_wr(mem_wr),
		.mem_addr(ram_a),
		.romen(romen),
		.phi_n(),
		.phi_en_n(),
		.phi_en_p(),
		.cpu_addr(cpu_addr),
		.cpu_dout(cpu_dout),
		.cpu_din(cpu_din),
		.iorq(cpu_iorq),
		.mreq(cpu_mreq),
		.rd(cpu_rd),
		.wr(cpu_wr),
		.m1(cpu_m1),
		.io_bus_byte(plus_io_bus_byte),
		.ga_ready(),
		.nmi(1'b0),
		.irq(force_irq),
		.cursor()
	);

	// FDC select decode and motor latch
	plus_fdc_decode fdc_decode (
		.addr(cpu_addr),
		.plus_mode(plus_mode),
		.plus_has_fdc(plus_has_fdc),
		.fdc_disabled(1'b0),
		.motor_sel(dbg_fdc_motor_sel),
		.u765_sel(dbg_fdc_data_sel)
	);

	// Legacy P10a controller phase. Real-u765 diagnostics select the shared
	// production divider above instead.
	reg [2:0] fdc_div = 3'd0;
	always @(posedge clk) fdc_div <= fdc_div + 3'd1;
	wire fixture_ce_u765 = (fdc_div == 3'd0);
	wire ce_u765 = production_clocking ? production_ce_u765 : fixture_ce_u765;

	// Match Amstrad.sv's mount-ready lifetime: mounting either drive updates
	// only that drive, and ordinary machine resets do not eject the image.
	reg [1:0] fdc_ready = 2'b00;
	always @(posedge clk) if(fdc_img_mounted[0]) fdc_ready[0] <= |fdc_img_size;
	always @(posedge clk) if(fdc_img_mounted[1]) fdc_ready[1] <= |fdc_img_size;

	u765 #(4000) fdc (
		.clk_sys(clk),
		.ce(ce_u765),
		.reset(sys_reset),
		.ready(fdc_ready),
		.motor({dbg_motor, dbg_motor}),
		.available(2'b11),
		.fast(1'b0),
		.a0(cpu_addr[0] | (dbg_fdc_data_sel & io_wr)),
		.nRD(~(dbg_fdc_data_sel & io_rd)),
		.nWR(~(dbg_fdc_data_sel & io_wr)),
		.din(cpu_dout),
		.dout(u765_dout),
		.img_mounted(fdc_img_mounted),
		.img_wp(fdc_img_wp),
		.img_size(fdc_img_size),
		.sd_lba(fdc_sd_lba),
		.sd_rd(fdc_sd_rd),
		.sd_wr(fdc_sd_wr),
		.sd_ack(fdc_sd_ack),
		.sd_buff_addr(fdc_sd_buff_addr),
		.sd_buff_dout(fdc_sd_buff_dout),
		.sd_buff_din(fdc_sd_buff_din),
		.sd_buff_wr(fdc_sd_buff_wr)
	);

	assign dbg_fdc_image_ready = fdc.fdc.image_ready[0];
	assign dbg_fdc_dout = u765_dout;
	assign dbg_fdc_state = fdc.fdc.state[7:0];
	assign dbg_fdc_msr = fdc.m_status;
	assign dbg_fdc_seek_pos = fdc.i_seek_pos;
	assign dbg_fdc_trackinfo_dirty = fdc.fdc.image_trackinfo_dirty[0];
	assign dbg_fdc_sector_pos = fdc.fdc.sector_byte_pos[0][0];
	assign dbg_fdc_byte_count = fdc.fdc.i_byte_clk_cnt;
	assign dbg_fdc_status0 = fdc.fdc.status[0];
	assign dbg_fdc_status1 = fdc.fdc.status[1];
	assign dbg_fdc_status2 = fdc.fdc.status[2];

	// T80pa captures external DI in the middle of T3, on the wrapper's
	// negative CPU-enable phase. Capture the bus selection and controller
	// values from that exact edge; observing the first IORQ clock is too early
	// to prove which byte the CPU actually consumed.
	wire cpu_di_latch_edge = mb.CPU.cen_n && mb.CPU.cen_pol &&
	                         (mb.CPU.u0.tstate == 3'b011) && mb.CPU.busak;
	assign dbg_cpu_di_reg = mb.CPU.di_reg;
	always @(posedge clk) begin
		if (sys_reset) begin
			dbg_cpu_di_latch     <= 1'b0;
			dbg_cpu_di_addr      <= 16'd0;
			dbg_cpu_di_fdc_sel   <= 1'b0;
			dbg_cpu_di_io_rd     <= 1'b0;
			dbg_cpu_di_fdc_dout  <= 8'd0;
			dbg_cpu_di_expected  <= 8'd0;
			dbg_cpu_di_fdc_bus   <= 8'd0;
			dbg_cpu_di_ram_dout  <= 8'd0;
			dbg_cpu_di_top_bus   <= 8'd0;
			dbg_cpu_di_mb_bus    <= 8'd0;
			dbg_cpu_di_msr       <= 8'd0;
			dbg_cpu_di_fdc_state <= 8'd0;
		end else begin
			dbg_cpu_di_latch <= cpu_di_latch_edge;
			if (cpu_di_latch_edge) begin
				dbg_cpu_di_addr      <= cpu_addr;
				dbg_cpu_di_fdc_sel   <= dbg_fdc_data_sel;
				dbg_cpu_di_io_rd     <= io_rd;
				dbg_cpu_di_fdc_dout  <= u765_dout;
				dbg_cpu_di_expected  <= cpu_addr[0] ? fdc.m_data : fdc.m_status;
				dbg_cpu_di_fdc_bus   <= fdc_dout;
				dbg_cpu_di_ram_dout  <= ram_dout;
				dbg_cpu_di_top_bus   <= cpu_din;
				dbg_cpu_di_mb_bus    <= mb.cpu_data_bus;
				dbg_cpu_di_msr       <= fdc.m_status;
				dbg_cpu_di_fdc_state <= fdc.fdc.state[7:0];
			end
		end
	end

	always @(posedge clk) begin
		reg old_wr;
		old_wr <= io_wr;
		if (sys_reset) begin
			dbg_motor <= 1'b0;
		end else if (~old_wr && io_wr && dbg_fdc_motor_sel) begin
			dbg_motor <= cpu_dout[0];
		end
	end

	// CRTC & ASIC write monitors
	always @(posedge clk) begin
		reg old_wr;
		old_wr <= io_wr;
		if (sys_reset) begin
			dbg_crtc_wr  <= 1'b0;
			dbg_crtc_reg <= 5'd0;
			dbg_crtc_val <= 8'd0;
			dbg_asic_wr  <= 1'b0;
			dbg_asic_addr <= 14'd0;
			dbg_asic_val  <= 8'd0;
		end else begin
			dbg_crtc_wr <= 1'b0;
			dbg_asic_wr <= 1'b0;
			if (~old_wr && io_wr) begin
				if (!cpu_addr[14] && !cpu_addr[9]) begin
					if (!cpu_addr[8]) dbg_crtc_reg <= cpu_dout[4:0];
					else begin
						dbg_crtc_val <= cpu_dout;
						dbg_crtc_wr  <= 1'b1;
					end
				end
			end
			if (mem_wr && plus_aspage_on && (cpu_addr[15:14] == 2'b01)) begin
				dbg_asic_wr   <= 1'b1;
				dbg_asic_addr <= cpu_addr[13:0];
				dbg_asic_val  <= cpu_dout;
			end
		end
	end

	// PC and bus trace assignments
	always @(posedge clk) begin
		if (sys_reset) dbg_pc <= 16'h0000;
		else if (cpu_m1 && cpu_mreq && cpu_rd) dbg_pc <= cpu_addr;
	end

	assign dbg_addr       = cpu_addr;
	assign dbg_dout       = cpu_dout;
	assign dbg_din        = cpu_din;
	assign dbg_m1_n       = ~cpu_m1;
	assign dbg_mreq_n     = ~cpu_mreq;
	assign dbg_iorq_n     = ~cpu_iorq;
	assign dbg_rd_n       = ~cpu_rd;
	assign dbg_wr_n       = ~cpu_wr;
	assign dbg_wait_n     = ~plus_cart_stall;

	assign dbg_cart_own   = plus_cart_own;
	assign dbg_cart_page  = plus_cart_page;
	assign dbg_cart_stall = plus_cart_stall;
	assign dbg_cart_dout  = plus_cart_dout;

	assign dbg_rmr2       = {3'b101, mmu.asic_page_on ? 2'b11 : mmu.rmr2_pos, mmu.rmr2_page};
	assign dbg_asic_page_on = plus_aspage_on;
	assign dbg_unlock_done = mmu.unlock_detector.unlocked;
	assign dbg_unlock_seq  = mmu.unlock_detector.sequence_index;

	assign dbg_int_n      = ~force_irq;
	assign dbg_int_ack    = cpu_m1 & cpu_iorq;
	assign dbg_vec_byte   = plus_vec_byte;
	assign dbg_vec_valid  = plus_vec_valid;

	assign dbg_mcycle = mb.CPU.u0.mcycle;
	assign dbg_tstate = mb.CPU.u0.tstate;
	assign dbg_ir     = mb.CPU.u0.ir;
	assign dbg_mcmax  = mb.CPU.u0.mc_max;
	assign dbg_cen_p  = mb.CPU.cen_p;
	assign dbg_cpu_waitn = mb.CPU.wait_n;

	// These taps stay in this simulation-only wrapper so the production
	// motherboard interface remains unchanged. `hs_sel`/`vs_sel` enter the
	// filter; motherboard outputs form the selected monitor tuple. Payload is
	// shared and filter-dependent because crtc_shift participates in vram_d.
	assign dbg_raw_hsync          = mb.hs_sel;
	assign dbg_raw_vsync          = mb.vs_sel;
	assign dbg_raw_de             = mb.de_sel;
	assign dbg_selected_hsync     = mb_hsync;
	assign dbg_selected_vsync     = mb_vsync;
	assign dbg_selected_hblank    = mb_hblank;
	assign dbg_selected_vblank    = mb_vblank;
	assign dbg_video_rgb          = {mb_red, mb_green, mb_blue};
	assign dbg_video_ma           = mb.ma_sel;
	assign dbg_video_ra           = mb.ra_sel;
	assign dbg_video_vram_addr    = vram_addr;
	assign dbg_video_vram_word    = vram_dout;
	assign dbg_video_vram_byte    = mb.vram_d;
	assign dbg_sdram_vram_req     = sdram_inst.vram_req;

	assign dbg_cart_image_valid  = cart_image_valid;
	assign dbg_cart_service_busy = cart_service_busy;
	assign dbg_cpr_load_error    = cart_load_error;
	assign dbg_sync_filter       = SYNC_FILTER;

`ifdef B7_DARK_SILICON_MUTATION
	// B7 is a Verilator-only path-ownership audit.  These are selected
	// motherboard outputs and CPU-bound signals; no raw mutation-control bit
	// enters the signature.  p10_boot_test_top.v is absent from files.qip and
	// this block is additionally unavailable unless the B7-only define is
	// supplied by sim/plus/Makefile.
	assign b7_rgb       = {mb_red, mb_green, mb_blue};
	assign b7_hsync     = mb_hsync;
	assign b7_vsync     = mb_vsync;
	assign b7_de        = mb.de_sel;
	assign b7_ma        = mb.ma_sel;
	assign b7_ra        = mb.ra_sel;
	assign b7_mode      = mb_mode;
	assign b7_audio_l   = mb_audio_l;
	assign b7_audio_r   = mb_audio_r;
	assign b7_cpu_irq_n = mb.INT_n & ~force_irq;

	// Runtime-selected corruptions are applied at module output boundaries,
	// before the production ownership muxes.  Constants are deliberately
	// local: no mutation touches reset, clock, plus_mode, or the signature
	// probes themselves.  The paired active/inactive-mode runs prove whether
	// each corrupted boundary is selected.
	// synthesis translate_off
	reg [8*64-1:0] b7_mutation_name;
	initial begin
		b7_mutation_name = "";
		b7_mutation_id = 5'd0;
		b7_mutation_enable = 1'b0;
		if ($value$plusargs("mutate_module=%s", b7_mutation_name)) begin
			if      (b7_mutation_name == "asic_video")             b7_mutation_id = 5'd1;
			else if (b7_mutation_name == "asic_sprites")           b7_mutation_id = 5'd2;
			else if (b7_mutation_name == "asic_dma")               b7_mutation_id = 5'd3;
			else if (b7_mutation_name == "asic_regs")              b7_mutation_id = 5'd4;
			else if (b7_mutation_name == "asic_ga_timing")         b7_mutation_id = 5'd5;
			else if (b7_mutation_name == "asic_unlock")             b7_mutation_id = 5'd6;
			else if (b7_mutation_name == "plus_mmu")                b7_mutation_id = 5'd7;
			else if (b7_mutation_name == "plus_sprite_ram")         b7_mutation_id = 5'd8;
			else if (b7_mutation_name == "plus_cartridge_memory")   b7_mutation_id = 5'd9;
			else if (b7_mutation_name == "CRTC")                    b7_mutation_id = 5'd10;
			else if (b7_mutation_name == "crtc_type0_engine")       b7_mutation_id = 5'd11;
			else if (b7_mutation_name == "crtc_type1_engine")       b7_mutation_id = 5'd12;
			else if (b7_mutation_name == "ga40010")                 b7_mutation_id = 5'd13;
			else if (b7_mutation_name == "negative_control")        b7_mutation_id = 5'd14;
			b7_mutation_enable = (b7_mutation_id != 5'd0);
		end
	end
	always @(b7_mutation_enable or b7_mutation_id) begin
		release mb.asic_vid.RGB_R;
		release mb.asic_vid.RGB_G;
		release mb.asic_vid.RGB_B;
		release mb.plus_sprites.SPR_EN;
		release mb.plus_sprites.SPR_RGB;
		release mb.dma_sound.dma_int_set;
		release mb.asic_page.pal_rdata;
		release mb.asic_ga.HSYNC_O;
		release mmu.unlock_detector.unlocked;
		release mmu.cart_dout;
		release mmu.asic_page_on;
		release mb.asic_page.spr_host_rdata;
		release cartridge_memory.cpu_data;
		release mb.crtc.MA;
		release mb.crtc.RA;
		release mb.crtc.DE;
		release mb.crtc.HSYNC;
		release mb.crtc.VSYNC;
		release mb.crtc.crtc_type0_engine.de_index;
		release mb.crtc.crtc_type0_engine.line_new;
		release mb.crtc.crtc_type0_engine.line_next;
		release mb.crtc.crtc_type0_engine.vsync_line_fire;
		release mb.crtc.crtc_type1_engine.de_index;
		release mb.crtc.crtc_type1_engine.line_new;
		release mb.crtc.crtc_type1_engine.line_next;
		release mb.crtc.crtc_type1_engine.vsync_line_fire;
		release mb.GateArray.HSYNC_O;
		release mb.GateArray.VSYNC_O;
		release mb.GateArray.RED_OE_N;
		release mb.GateArray.RED;
		release mb.asic_ga.MODE;

		if (b7_mutation_enable) begin
			case (b7_mutation_id)
				5'd1: begin // asic_video
					force mb.asic_vid.RGB_R = 4'hf;
					force mb.asic_vid.RGB_G = 4'h1;
					force mb.asic_vid.RGB_B = 4'he;
				end
				5'd2: begin // asic_sprites
					force mb.plus_sprites.SPR_EN = 1'b1;
					force mb.plus_sprites.SPR_RGB = 12'hd3a;
				end
				5'd3: begin // asic_dma
					force mb.dma_sound.dma_int_set = 3'b111;
				end
				5'd4: begin // asic_regs
					force mb.asic_page.pal_rdata = 12'ha5c;
				end
				5'd5: begin // asic_ga_timing
					force mb.asic_ga.HSYNC_O = 1'b1;
				end
				5'd6: begin // asic_unlock
					force mmu.unlock_detector.unlocked = 1'b0;
				end
				5'd7: begin // plus_mmu
					force mmu.cart_dout = 8'h00;
					force mmu.asic_page_on = 1'b0;
				end
				5'd8: begin // plus_sprite_ram
					force mb.asic_page.spr_host_rdata = 4'hf;
				end
				5'd9: begin // plus_cartridge_memory
					force cartridge_memory.cpu_data = 8'h00;
				end
				5'd10: begin // CRTC wrapper
					force mb.crtc.MA = 14'h2aaa;
					force mb.crtc.RA = 5'h1f;
					force mb.crtc.DE = 1'b1;
					force mb.crtc.HSYNC = 1'b1;
					force mb.crtc.VSYNC = 1'b1;
				end
				5'd11: begin // crtc_type0_engine
					force mb.crtc.crtc_type0_engine.de_index = 2'b11;
					force mb.crtc.crtc_type0_engine.line_new = 1'b1;
					force mb.crtc.crtc_type0_engine.line_next = 5'h1f;
					force mb.crtc.crtc_type0_engine.vsync_line_fire = 1'b1;
				end
				5'd12: begin // crtc_type1_engine
					force mb.crtc.crtc_type1_engine.de_index = 2'b11;
					force mb.crtc.crtc_type1_engine.line_new = 1'b1;
					force mb.crtc.crtc_type1_engine.line_next = 5'h1f;
					force mb.crtc.crtc_type1_engine.vsync_line_fire = 1'b1;
				end
				5'd13: begin // classic ga40010
					force mb.GateArray.HSYNC_O = 1'b1;
					force mb.GateArray.VSYNC_O = 1'b1;
					force mb.GateArray.RED_OE_N = 1'b0;
					force mb.GateArray.RED = 1'b1;
				end
				5'd14: begin // negative control: MODE is deliberately unconnected
					force mb.asic_ga.MODE = 2'b11;
				end
				default: ;
			endcase
		end
	end
	// synthesis translate_on
`endif
endmodule

// Quartus supplies this primitive.
module altddio_out
#(
	parameter extend_oe_disable = "OFF",
	parameter intended_device_family = "Cyclone V",
	parameter invert_output = "OFF",
	parameter lpm_hint = "UNUSED",
	parameter lpm_type = "altddio_out",
	parameter oe_reg = "UNREGISTERED",
	parameter power_up_high = "OFF",
	parameter width = 1
)
(
	input                 datain_h,
	input                 datain_l,
	input                 outclock,
	output                dataout,
	input                 aclr,
	input                 aset,
	input                 oe,
	input                 outclocken,
	input                 sclr,
	input                 sset
);

assign dataout = outclock ? datain_h : datain_l;

endmodule
