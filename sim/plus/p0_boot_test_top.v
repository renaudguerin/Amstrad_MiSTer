`timescale 1ns/1ps

// P0 boot integration: the production CPR parser feeds the production
// cartridge memory service on a real SDRAM controller. The C++ model
// observes physical SDRAM commands and supplies CAS-latency read data,
// exactly like sdram_cartridge_test_top.
module p0_boot_test_top
(
	input             clk,
	input             clkref,
	input             init,

	input             reset,

	input             cpr_download,
	input             ioctl_wr,
	input      [24:0] ioctl_addr,
	input      [7:0]  ioctl_dout,
	output            ioctl_wait,

	input             cpu_valid,
	input      [4:0]  cpu_page,
	input      [13:0] cpu_offset,
	output            cpu_ready,
	output      [7:0] cpu_data,

	input             use_mmu,
	input             mmu_mem_rd,
	input      [15:0] mmu_A,
	output            mmu_cart_own,
	output            mmu_cart_stall,
	output      [7:0] mmu_cart_dout,

	output            image_valid,
	output            service_busy,

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
	output      [2:0] debug_q
);

wire        load_begin, load_commit, load_abort;
wire        load_valid, load_ready, load_error;
wire  [5:0] load_page;
wire [14:0] load_offset;
wire  [7:0] load_data;

wire        mem_req, mem_write, mem_ack;
wire  [1:0] mem_bank;
wire [22:0] mem_addr;
wire  [7:0] mem_wdata, mem_rdata;

wire [15:0] sdram_dq;
wire        unused_sdram_clk, unused_sdram_cke, unused_sdram_ncs;
wire  [7:0] unused_dout;
wire [15:0] unused_vram_dout;
wire  [7:0] unused_tape_dout;
wire        unused_tape_wr_ack, unused_tape_rd_ack;

wire        mmu_cart_valid, mmu_cart_ready;
wire  [4:0] mmu_cart_page;
wire [13:0] mmu_cart_offset;
wire  [7:0] mmu_cart_data;
wire        service_cpu_valid = use_mmu ? mmu_cart_valid : cpu_valid;
wire  [4:0] service_cpu_page = use_mmu ? mmu_cart_page : cpu_page;
wire [13:0] service_cpu_offset = use_mmu ? mmu_cart_offset : cpu_offset;

assign sdram_dq = memory_dq_oe ? memory_dq : 16'hzzzz;
assign observed_dq = sdram_dq;

plus_mmu mmu
(
	.clk(clk),
	.reset(reset),
	.plus_mode(1'b1),
	.gx4000(1'b0),
	.io_wr(1'b0),
	.mem_rd(mmu_mem_rd),
	.A(mmu_A),
	.D(8'h00),
	.rom_en(1'b1),
	.exp_n(1'b1),
	.cart_valid(mmu_cart_valid),
	.cart_page(mmu_cart_page),
	.cart_offset(mmu_cart_offset),
	.cart_ready(mmu_cart_ready),
	.cart_data(mmu_cart_data),
	.cart_busy(service_busy),
	.cart_own(mmu_cart_own),
	.cart_stall(mmu_cart_stall),
	.cart_dout(mmu_cart_dout),
	.asic_page_on()
);

plus_cpr_parser parser
(
	.clk(clk),
	.reset(reset),

	.cpr_download(cpr_download),
	.ioctl_wr(ioctl_wr),
	.ioctl_addr(ioctl_addr),
	.ioctl_dout(ioctl_dout),
	.ioctl_wait(ioctl_wait),

	.load_begin(load_begin),
	.load_commit(load_commit),
	.load_abort(load_abort),
	.load_valid(load_valid),
	.load_page(load_page),
	.load_offset(load_offset),
	.load_data(load_data),
	.load_ready(load_ready),
	.load_error(load_error)
);

plus_cartridge_memory service
(
	.clk(clk),
	.cold_reset(reset),
	.detach(1'b0),

	.load_begin(load_begin),
	.load_commit(load_commit),
	.load_abort(load_abort),
	.load_valid(load_valid),
	.load_page(load_page),
	.load_offset(load_offset),
	.load_data(load_data),
	.load_ready(load_ready),
	.load_error(load_error),

	.cpu_valid(service_cpu_valid),
	.cpu_page(service_cpu_page),
	.cpu_offset(service_cpu_offset),
	.cpu_ready(mmu_cart_ready),
	.cpu_data(mmu_cart_data),

	.image_valid(image_valid),
	.busy(service_busy),

	.mem_req(mem_req),
	.mem_write(mem_write),
	.mem_bank(mem_bank),
	.mem_addr(mem_addr),
	.mem_wdata(mem_wdata),
	.mem_ack(mem_ack),
	.mem_rdata(mem_rdata)
);

assign cpu_ready = mmu_cart_ready;
assign cpu_data = mmu_cart_data;

sdram dut
(
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
	.din(8'd0),
	.dout(unused_dout),
	.addr(23'd0),
	.oe(1'b0),
	.we(1'b0),
	.cart_req(mem_req),
	.cart_wr(mem_write),
	.cart_bank(mem_bank),
	.cart_addr(mem_addr),
	.cart_din(mem_wdata),
	.cart_dout(mem_rdata),
	.cart_ack(mem_ack),
	.vram_dout(unused_vram_dout),
	.vram_addr(23'd0),
	.vram_bank(2'b00),
	.tape_addr(23'd0),
	.tape_din(8'd0),
	.tape_dout(unused_tape_dout),
	.tape_wr(1'b0),
	.tape_wr_ack(unused_tape_wr_ack),
	.tape_rd(1'b0),
	.tape_rd_ack(unused_tape_rd_ack)
);

assign debug_q = dut.q;

endmodule

// Quartus supplies this primitive. The functional test only needs the module
// to elaborate; SDRAM_CLK phase is outside the controller arbitration contract.
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
