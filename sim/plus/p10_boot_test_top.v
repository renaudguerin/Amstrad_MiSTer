`timescale 1ns/1ps

// ===========================================================================
// P10a Real-T80 Production-Path CPR Boot Harness Top
//
// Connects the production CPR parser, cartridge memory service, real SDRAM,
// plus_mmu, plus_model_select, and Amstrad_motherboard (with real T80pa CPU)
// under production reset sequencing and I/O decode.
// ===========================================================================

module p10_boot_test_top
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
	output            dbg_reset,
	output      [2:0] dbg_mcycle,
	output      [2:0] dbg_tstate,
	output      [7:0] dbg_ir,
	output      [2:0] dbg_mcmax,
	output            dbg_cen_p,
	output            dbg_cpu_waitn
);

	// 16 MHz dot clock enable from 64 MHz clk
	reg [1:0] cdiv;
	always @(posedge clk) begin
		if (dbg_reset) cdiv <= 2'd0;
		else cdiv <= cdiv + 2'd1;
	end
	wire ce_16 = (cdiv == 2'd0);

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

	plus_cartridge_memory cartridge_memory (
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
		.sna_load(1'b0),
		.sna_rmr2(8'd0),
		.sna_unlock(1'b0)
	);

	// SDRAM memory controller
	wire [15:0] sdram_dq;
	wire        unused_sdram_clk, unused_sdram_cke, unused_sdram_ncs;
	wire [15:0] vram_dout;

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
		.bank(2'b00),
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
		.vram_addr(23'd0),
		.vram_bank(2'b00),
		.tape_addr(23'd0),
		.tape_din(8'd0),
		.tape_dout(),
		.tape_wr(1'b0),
		.tape_wr_ack(),
		.tape_rd(1'b0),
		.tape_rd_ack()
	);

	// CPU DIN MUX
	wire [7:0] fdc_dout = 8'hFF;
	wire [7:0] cpu_din_bus = ram_dout & fdc_dout;
	assign cpu_din = plus_vec_valid ? plus_vec_byte :
	                 plus_asic_rd   ? plus_asic_dout :
	                 plus_cart_own  ? plus_cart_dout : cpu_din_bus;

	// Motherboard
	wire motherboard_int_n;

	Amstrad_motherboard mb (
		.reset(sys_reset),
		.clk(clk),
		.ce_16(ce_16),
		.plus_mode(plus_mode),
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
		.crtc_type(1'b0),
		.sync_filter(1'b0),
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
		.joy1(7'd0),
		.joy2(7'd0),
		.tape_in(1'b0),
		.tape_out(),
		.tape_motor(),
		.audio_l(),
		.audio_r(),
		.mode(),
		.hblank(),
		.vblank(),
		.hsync(),
		.vsync(),
		.red(),
		.green(),
		.blue(),
		.field(),
		.vram_din(vram_dout),
		.vram_addr(),
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
	assign dbg_fdc_motor_sel = (!plus_mode || plus_has_fdc) && !cpu_addr[10] && cpu_addr[9] && !cpu_addr[8];
	assign dbg_fdc_data_sel  = (!plus_mode || plus_has_fdc) && !cpu_addr[10] && cpu_addr[9] && cpu_addr[8] && cpu_addr[4];

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
