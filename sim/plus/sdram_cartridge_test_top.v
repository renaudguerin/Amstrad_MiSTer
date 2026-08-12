`timescale 1ns/1ps

// Test-only shell around the production SDRAM controller. The memory-side DQ
// driver is deliberately external: the C++ model observes real ACTIVE/READ/
// WRITE commands and supplies CAS-latency read data on the physical bus.
module sdram_cartridge_test_top
(
	input             clk,
	input             clkref,
	input             init,

	input       [1:0] bank,
	input       [7:0] din,
	output      [7:0] dout,
	input      [22:0] addr,
	input             oe,
	input             we,

	input             cart_req,
	input             cart_wr,
	input       [1:0] cart_bank,
	input      [22:0] cart_addr,
	input       [7:0] cart_din,
	output      [7:0] cart_dout,
	output            cart_ack,

	input      [22:0] vram_addr,
	input       [1:0] vram_bank,
	output     [15:0] vram_dout,

	input      [22:0] tape_addr,
	input       [7:0] tape_din,
	output      [7:0] tape_dout,
	input             tape_wr,
	output            tape_wr_ack,
	input             tape_rd,
	output            tape_rd_ack,

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
	output      [2:0] debug_q,
	output      [1:0] debug_mode,
	output            debug_refresh_due,
	output      [5:0] debug_cart_grants,

	// A second, complete path joins the production cartridge service to a
	// production SDRAM controller. It shares only clock/init with the direct
	// controller above and has its own externally-modelled physical DQ bus.
	input             service_cold_reset,
	input             service_detach,
	input             service_load_begin,
	input             service_load_commit,
	input             service_load_abort,
	input             service_load_valid,
	input       [5:0] service_load_page,
	input      [14:0] service_load_offset,
	input       [7:0] service_load_data,
	output            service_load_ready,
	output            service_load_error,
	input             service_cpu_valid,
	input       [4:0] service_cpu_page,
	input      [13:0] service_cpu_offset,
	output            service_cpu_ready,
	output      [7:0] service_cpu_data,
	output            service_image_valid,
	output            service_busy,
	output            service_mem_req,
	output            service_mem_write,
	output      [1:0] service_mem_bank,
	output     [22:0] service_mem_addr,
	output      [7:0] service_mem_wdata,
	input      [15:0] integration_memory_dq,
	input             integration_memory_dq_oe,
	output     [15:0] integration_observed_dq,
	output     [12:0] integration_sdram_a,
	output      [1:0] integration_sdram_ba,
	output            integration_sdram_dqml,
	output            integration_sdram_dqmh,
	output            integration_sdram_nwe,
	output            integration_sdram_nras,
	output            integration_sdram_ncas
);

wire [15:0] sdram_dq;
wire        unused_sdram_clk;
wire        unused_sdram_cke;
wire        unused_sdram_ncs;

assign sdram_dq = memory_dq_oe ? memory_dq : 16'hzzzz;
assign observed_dq = sdram_dq;

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
	.bank(bank),
	.din(din),
	.dout(dout),
	.addr(addr),
	.oe(oe),
	.we(we),
	.cart_req(cart_req),
	.cart_wr(cart_wr),
	.cart_bank(cart_bank),
	.cart_addr(cart_addr),
	.cart_din(cart_din),
	.cart_dout(cart_dout),
	.cart_ack(cart_ack),
	.vram_dout(vram_dout),
	.vram_addr(vram_addr),
	.vram_bank(vram_bank),
	.tape_addr(tape_addr),
	.tape_din(tape_din),
	.tape_dout(tape_dout),
	.tape_wr(tape_wr),
	.tape_wr_ack(tape_wr_ack),
	.tape_rd(tape_rd),
	.tape_rd_ack(tape_rd_ack)
);

assign debug_q = dut.q;
assign debug_mode = dut.mode;
assign debug_refresh_due = dut.refresh_due;
assign debug_cart_grants = dut.cart_grants_since_refresh;

wire [15:0] integration_sdram_dq;
wire  [7:0] service_mem_rdata;
wire        service_mem_ack;
wire        integration_unused_sdram_clk;
wire        integration_unused_sdram_cke;
wire        integration_unused_sdram_ncs;
wire  [7:0] integration_unused_dout;
wire [15:0] integration_unused_vram_dout;
wire  [7:0] integration_unused_tape_dout;
wire        integration_unused_tape_wr_ack;
wire        integration_unused_tape_rd_ack;

assign integration_sdram_dq = integration_memory_dq_oe ?
	                            integration_memory_dq : 16'hzzzz;
assign integration_observed_dq = integration_sdram_dq;

plus_cartridge_memory #(.CLEAR_BYTES(20'd4)) service
(
	.clk(clk),
	.cold_reset(service_cold_reset),
	.detach(service_detach),
	.load_begin(service_load_begin),
	.load_commit(service_load_commit),
	.load_abort(service_load_abort),
	.load_valid(service_load_valid),
	.load_page(service_load_page),
	.load_offset(service_load_offset),
	.load_data(service_load_data),
	.load_ready(service_load_ready),
	.load_error(service_load_error),
	.cpu_valid(service_cpu_valid),
	.cpu_page(service_cpu_page),
	.cpu_offset(service_cpu_offset),
	.cpu_ready(service_cpu_ready),
	.cpu_data(service_cpu_data),
	.image_valid(service_image_valid),
	.busy(service_busy),
	.mem_req(service_mem_req),
	.mem_write(service_mem_write),
	.mem_bank(service_mem_bank),
	.mem_addr(service_mem_addr),
	.mem_wdata(service_mem_wdata),
	.mem_ack(service_mem_ack),
	.mem_rdata(service_mem_rdata)
);

sdram integration_dut
(
	.SDRAM_DQ(integration_sdram_dq),
	.SDRAM_A(integration_sdram_a),
	.SDRAM_DQML(integration_sdram_dqml),
	.SDRAM_DQMH(integration_sdram_dqmh),
	.SDRAM_BA(integration_sdram_ba),
	.SDRAM_nCS(integration_unused_sdram_ncs),
	.SDRAM_nWE(integration_sdram_nwe),
	.SDRAM_nRAS(integration_sdram_nras),
	.SDRAM_nCAS(integration_sdram_ncas),
	.SDRAM_CLK(integration_unused_sdram_clk),
	.SDRAM_CKE(integration_unused_sdram_cke),
	.init(init),
	.clk(clk),
	.clkref(clkref),
	.bank(2'b00),
	.din(8'd0),
	.dout(integration_unused_dout),
	.addr(23'd0),
	.oe(1'b0),
	.we(1'b0),
	.cart_req(service_mem_req),
	.cart_wr(service_mem_write),
	.cart_bank(service_mem_bank),
	.cart_addr(service_mem_addr),
	.cart_din(service_mem_wdata),
	.cart_dout(service_mem_rdata),
	.cart_ack(service_mem_ack),
	.vram_dout(integration_unused_vram_dout),
	.vram_addr(23'd0),
	.vram_bank(2'b00),
	.tape_addr(23'd0),
	.tape_din(8'd0),
	.tape_dout(integration_unused_tape_dout),
	.tape_wr(1'b0),
	.tape_wr_ack(integration_unused_tape_wr_ack),
	.tape_rd(1'b0),
	.tape_rd_ack(integration_unused_tape_rd_ack)
);

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
