`timescale 1ns/1ps

// Test-only top module for B18 slices 4b/4c: SNA save stream, DDR3 mux and
// SDRAM cartridge-port mux. Instantiates the production rtl/sdram.v,
// rtl/sna_save_stream.v, rtl/sna_ddr_mux.v and rtl/sna_cart_mux.v, with the
// cartridge memory service replaced by a scripted held-request client (cs_*).
module sna_save_stream_test_top
(
	input                 clk,
	input                 clkref,
	input                 reset,
	input                 stream_reset,
	input                 mux_reset,
	input                 init,

	// Stream control
	input                 start,
	input                 ram128,
	input          [1:0]  bank,
	input       [2047:0]  header,
	output                done,
	output                active,

	// Stream internals / status observation
	output                stream_cart_req,
	output         [1:0]  stream_cart_bank,
	output        [22:0]  stream_cart_addr,
	output         [7:0]  stream_cart_dout,
	output                stream_cart_ack,

	// Scripted cartridge-service client on sna_cart_mux client A (read-only)
	input                 cs_req,
	input          [1:0]  cs_bank,
	input         [22:0]  cs_addr,
	output                cs_ack,
	output                debug_cart_owner_b,

	output                stream_ddr_request,
	output                stream_ddr_grant,

	// Scripted Master A (SSM marker stand-in)
	input         [28:0]  a_addr,
	input         [63:0]  a_din,
	input          [7:0]  a_be,
	input          [7:0]  a_burstcnt,
	input                 a_we,
	output                a_busy,

	// DDR3 slave interface (to C++ DDR3 memory model)
	input                 ddram_busy,
	output        [28:0]  ddram_addr,
	output        [63:0]  ddram_din,
	output         [7:0]  ddram_be,
	output         [7:0]  ddram_burstcnt,
	output                ddram_we,

	// Tape and VRAM ports to SDRAM for Case 4 (Fairness & Refresh)
	input                 tape_rd,
	output                tape_rd_ack,
	input         [22:0]  tape_addr,
	input         [22:0]  vram_addr,
	input          [1:0]  vram_bank,
	output        [15:0]  vram_dout,

	// SDRAM physical pins (observed / driven by C++ SDRAM model)
	input         [15:0]  memory_dq,
	input                 memory_dq_oe,
	output        [15:0]  observed_dq,
	output        [12:0]  sdram_a,
	output         [1:0]  sdram_ba,
	output                sdram_dqml,
	output                sdram_dqmh,
	output                sdram_nwe,
	output                sdram_nras,
	output                sdram_ncas,
	output         [2:0]  debug_q,
	output         [1:0]  debug_mode,
	output                debug_refresh_due,
	output         [5:0]  debug_cart_grants,
	output         [3:0]  debug_stream_state
);

	assign debug_stream_state = dut_stream.state;

	wire [15:0] sdram_dq;
	wire        unused_sdram_clk;
	wire        unused_sdram_cke;
	wire        unused_sdram_ncs;
	wire  [7:0] unused_dout;
	wire  [7:0] unused_tape_dout;
	wire        unused_tape_wr_ack;

	assign sdram_dq    = memory_dq_oe ? memory_dq : 16'hzzzz;
	assign observed_dq = sdram_dq;

	// Interconnect: stream <-> cart mux client B, cart mux <-> sdram cart port
	wire        cart_req;
	wire  [1:0] cart_bank;
	wire [22:0] cart_addr;
	wire  [7:0] cart_dout;
	wire        cart_ack;

	wire        sdram_cart_req;
	wire        sdram_cart_wr;
	wire  [1:0] sdram_cart_bank;
	wire [22:0] sdram_cart_addr;
	wire  [7:0] sdram_cart_din;
	wire        sdram_cart_ack;
	wire        sdram_cart_grant;

	assign stream_cart_req  = cart_req;
	assign stream_cart_bank = cart_bank;
	assign stream_cart_addr = cart_addr;
	assign stream_cart_dout = cart_dout;
	assign stream_cart_ack  = cart_ack;

	sna_cart_mux dut_cart_mux
	(
		.clk(clk),
		.clkref(clkref),
		.a_req(cs_req),
		.a_wr(1'b0),
		.a_bank(cs_bank),
		.a_addr(cs_addr),
		.a_din(8'd0),
		.a_ack(cs_ack),
		.a_grant(),
		.b_req(cart_req),
		.b_bank(cart_bank),
		.b_addr(cart_addr),
		.b_ack(cart_ack),
		.cart_req(sdram_cart_req),
		.cart_wr(sdram_cart_wr),
		.cart_bank(sdram_cart_bank),
		.cart_addr(sdram_cart_addr),
		.cart_din(sdram_cart_din),
		.cart_ack(sdram_cart_ack),
		.cart_grant(sdram_cart_grant)
	);

	assign debug_cart_owner_b = dut_cart_mux.owner_b;

	// Interconnect: stream <-> ddr mux master B
	wire        b_request;
	wire        b_grant;
	wire [28:0] b_addr;
	wire [63:0] b_din;
	wire  [7:0] b_be;
	wire  [7:0] b_burstcnt;
	wire        b_we;
	wire        b_busy;

	assign stream_ddr_request = b_request;
	assign stream_ddr_grant   = b_grant;

	sdram dut_sdram
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
		.cart_req(sdram_cart_req),
		.cart_wr(sdram_cart_wr),
		.cart_bank(sdram_cart_bank),
		.cart_addr(sdram_cart_addr),
		.cart_din(sdram_cart_din),
		.cart_dout(cart_dout),
		.cart_ack(sdram_cart_ack),
		.cart_grant(sdram_cart_grant),
		.vram_dout(vram_dout),
		.vram_addr(vram_addr),
		.vram_bank(vram_bank),
		.tape_addr(tape_addr),
		.tape_din(8'd0),
		.tape_dout(unused_tape_dout),
		.tape_wr(1'b0),
		.tape_wr_ack(unused_tape_wr_ack),
		.tape_rd(tape_rd),
		.tape_rd_ack(tape_rd_ack)
	);

	assign debug_q           = dut_sdram.q;
	assign debug_mode        = dut_sdram.mode;
	assign debug_refresh_due = dut_sdram.refresh_due;
	assign debug_cart_grants = dut_sdram.cart_grants_since_refresh;

	wire effective_stream_reset = reset | stream_reset;

	sna_save_stream dut_stream
	(
		.clk(clk),
		.reset(effective_stream_reset),
		.start(start),
		.ram128(ram128),
		.bank(bank),
		.header(header),
		.clkref(clkref),
		.cart_req(cart_req),
		.cart_bank(cart_bank),
		.cart_addr(cart_addr),
		.cart_dout(cart_dout),
		.cart_ack(cart_ack),
		.ddr_grant(b_grant),
		.ddr_request(b_request),
		.ddram_addr(b_addr),
		.ddram_din(b_din),
		.ddram_be(b_be),
		.ddram_burstcnt(b_burstcnt),
		.ddram_we(b_we),
		.ddram_busy(b_busy),
		.done(done),
		.active(active)
	);

	wire effective_mux_reset = reset | mux_reset;

	sna_ddr_mux dut_mux
	(
		.clk(clk),
		.reset(effective_mux_reset),
		.a_addr(a_addr),
		.a_din(a_din),
		.a_be(a_be),
		.a_burstcnt(a_burstcnt),
		.a_we(a_we),
		.a_busy(a_busy),
		.b_request(b_request),
		.b_grant(b_grant),
		.b_addr(b_addr),
		.b_din(b_din),
		.b_be(b_be),
		.b_burstcnt(b_burstcnt),
		.b_we(b_we),
		.b_busy(b_busy),
		.ddram_addr(ddram_addr),
		.ddram_din(ddram_din),
		.ddram_be(ddram_be),
		.ddram_burstcnt(ddram_burstcnt),
		.ddram_we(ddram_we),
		.ddram_busy(ddram_busy)
	);

endmodule

// Quartus primitive stub for SDRAM_CLK generation
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

/* verilator lint_off UNUSEDSIGNAL */
wire _unused = aclr | aset | oe | outclocken | sclr | sset;
/* verilator lint_on UNUSEDSIGNAL */

assign dataout = outclock ? datain_h : datain_l;

endmodule
