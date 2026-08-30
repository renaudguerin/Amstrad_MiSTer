// Production Plus input-path fixture (P10 hardware follow-up).
//
// The repository's Amstrad_motherboard.v is a Verilog-2001 source because
// its T80pa instance uses the Verilog-only `do` pin name.  Verilator cannot
// parse that source and the real SystemVerilog YM2149/HID sources in one
// language mode.  This fixture therefore uses the explicitly permitted
// minimal exact input wrapper below: it copies the motherboard's PPI,
// YM2149, and HID wiring verbatim, while the top-level joydb selection is
// the same as Amstrad.sv.  No Plus RTL is substituted in the tested path.
// The inputs model a CPC 6128+ (not GX4000): Plus mode is asserted and the
// 128K/FDC/tape capability flags are not relevant to this input slice.

module p10_input_test_top (
	input        clk,
	input        reset,
	input [10:0] ps2_key,
	input  [6:0] user_in,
	input  [1:0] snac_player,
	input  [6:0] joy1_usb,
	output [7:0] key_matrix_o,
	output [7:0] port_c_o,
	output [7:0] psg_addr_o,
	output [6:0] joy1_selected_o,
	output       done_o,
	output [2:0] read_count_o,
	output [7:0] read0_o,
	output [7:0] read1_o,
	output [7:0] read2_o,
	output [7:0] read3_o,
	output [7:0] read4_o
);
	reg [1:0] cdiv;
	always @(posedge clk) begin
		if (reset) cdiv <= 2'd0;
		else cdiv <= (cdiv == 2'd3) ? 2'd0 : cdiv + 2'd1;
	end
	wire ce_16 = (cdiv == 2'd0);

	// This is the exact top-level joydb selection from Amstrad.sv, with
	// OSD_STATUS held inactive.  The explicit DB9 packing is important:
	// joydb's 16-bit internal layout is not the motherboard's 7-bit layout.
	wire [15:0] joydb_1;
	wire [15:0] joydb_2;
	wire        joydb_1ena;
	wire        joydb_2ena;
	joydb joydb_i
	(
		.USER_IN(user_in),
		.snac_player(snac_player),
		.joystick1(joydb_1),
		.joystick2(joydb_2),
		.joystick1_en(joydb_1ena),
		.joystick2_en(joydb_2ena)
	);
	wire [6:0] joy1_db9 = {joydb_1[10], joydb_1[6], joydb_1[4], joydb_1[3:0]};
	wire [6:0] joy2_db9 = {joydb_2[10], joydb_2[6], joydb_2[4], joydb_2[3:0]};
	wire [6:0] joy1 = joydb_1ena ? joy1_db9 : joy1_usb;
	wire [6:0] joy2 = joydb_2ena ? joy2_db9 : joydb_1ena ? joy1_usb : 7'd0;

	wire [15:0] cpu_addr;
	wire [7:0]  cpu_do;
	wire [7:0]  cpu_di;
	wire        cpu_rd_n;
	wire        cpu_wr_n;
	wire        cpu_iorq_n;
	wire        cpu_mreq_n;
	wire        cpu_m1_n;

	T80pa_input_bench_cpu cpu
	(
		.reset_n(~reset),
		.clk(clk),
		.cen_p(ce_16),
		.cen_n(ce_16),
		.a(cpu_addr),
		.cpu_do(cpu_do),
		.di(cpu_di),
		.rd_n(cpu_rd_n),
		.wr_n(cpu_wr_n),
		.iorq_n(cpu_iorq_n),
		.mreq_n(cpu_mreq_n),
		.m1_n(cpu_m1_n),
		.wait_n(1'b1),
		.done_o(done_o),
		.read_count_o(read_count_o),
		.read0_o(read0_o),
		.read1_o(read1_o),
		.read2_o(read2_o),
		.read3_o(read3_o),
		.read4_o(read4_o)
	);

	p10_input_motherboard mb
	(
		.clk(clk),
		.reset(reset),
		.ce_16(ce_16),
		.cpu_addr(cpu_addr),
		.cpu_do(cpu_do),
		.cpu_di(cpu_di),
		.cpu_rd_n(cpu_rd_n),
		.cpu_wr_n(cpu_wr_n),
		.cpu_iorq_n(cpu_iorq_n),
		.cpu_mreq_n(cpu_mreq_n),
		.plus_mode(1'b1),
		.joy1(joy1),
		.joy2(joy2),
		.ps2_key(ps2_key),
		.key_matrix_o(key_matrix_o),
		.port_c_o(port_c_o),
		.psg_addr_o(psg_addr_o),
		.joy1_selected_o(joy1_selected_o)
	);
endmodule

// Minimal exact input slice of rtl/Amstrad_motherboard.v.  Signal names and
// equations intentionally match the production source around its i8255,
// YM2149, and hid instances.  The video, MMU, and classic Gate Array are
// outside the input-path acceptance surface and are not duplicated here.
module p10_input_motherboard (
	input        clk,
	input        reset,
	input        ce_16,
	input [15:0] cpu_addr,
	input  [7:0] cpu_do,
	output [7:0] cpu_di,
	input        cpu_rd_n,
	input        cpu_wr_n,
	input        cpu_iorq_n,
	input        cpu_mreq_n,
	input        plus_mode,
	input  [6:0] joy1,
	input  [6:0] joy2,
	input [10:0] ps2_key,
	output [7:0] key_matrix_o,
	output [7:0] port_c_o,
	output [7:0] psg_addr_o,
	output [6:0] joy1_selected_o
);
	wire io_rd = ~(cpu_rd_n | cpu_iorq_n);
	wire io_wr = ~(cpu_wr_n | cpu_iorq_n);
	wire [7:0] ppi_dout;
	wire [7:0] portC;
	wire [7:0] portAout;
	wire [7:0] portAin;
	wire [7:0] kbd_out;
	wire [7:0] psg_do;
	wire [7:0] ch_a, ch_b, ch_c;
	wire [5:0] psg_active;
	wire [7:0] psg_ioa_out, psg_iob_out;
	reg [7:0] cpu_psg_addr;

	// This is the production motherboard's psg-address shadow.  It is kept
	// as a visible tap even though YM2149 also retains its own address latch.
	always @(posedge clk) begin
		if (reset) cpu_psg_addr <= 8'd0;
		else if (portC[7] && portC[6]) cpu_psg_addr <= portAout;
	end

	// Exact production PPI instantiation (Plus mode keeps Port C output
	// drive even when control word 0x9B marks the legacy direction as input).
	i8255 PPI
	(
		.reset(reset),
		.clk_sys(clk),
		.addr(cpu_addr[9:8]),
		.idata(cpu_do),
		.odata(ppi_dout),
		.cs(~cpu_addr[11]),
		.we(io_wr),
		.oe(io_rd),
		.ipa(portAin),
		.opa(portAout),
		.ipb(8'hFF),
		.opb(),
		.ipc(8'hFF),
		.opc(portC),
		.plus_mode(plus_mode),
		.sna_load(1'b0),
		.sna_opa(8'd0),
		.sna_opb(8'd0),
		.sna_opc(8'd0),
		.sna_control(8'h9B)
	);

	// Exact production YM2149 bus mux in the non-DMA case.  The Plus DMA
	// owner is absent from this input-only slice, so the PPI Port C controls
	// BDIR/BC directly.
	YM2149 PSG
	(
		.RESET(reset),
		.CLK(clk),
		.CE(ce_16),
		.SEL(1'b0),
		.MODE(1'b0),
		.BC(portC[6]),
		.BDIR(portC[7]),
		.DI(portAout),
		.DO(psg_do),
		.CHANNEL_A(ch_a),
		.CHANNEL_B(ch_b),
		.CHANNEL_C(ch_c),
		.ACTIVE(psg_active),
		.IOA_out(psg_ioa_out),
		.IOB_out(psg_iob_out),
		.IOA_in(kbd_out),
		.IOB_in(8'hFF),
		.SNA_LOAD(1'b0),
		.SNA_ADDR(4'd0),
		.SNA_REGS(128'd0)
	);
	assign portAin = psg_do;

	// Exact production HID instance and row/joystick sources.
	hid HID
	(
		.reset(reset),
		.clk(clk),
		.right_shift_mod(1'b0),
		.keypad_mod(1'b0),
		.ps2_key(ps2_key),
		.ps2_mouse(25'd0),
		.joystick1(joy1),
		.joystick2(joy2),
		.Y(portC[3:0]),
		.X(kbd_out),
		.key_nmi(),
		.key_reset(),
		.Fn()
	);

	assign cpu_di = ppi_dout;
	assign key_matrix_o = kbd_out;
	assign port_c_o = portC;
	assign psg_addr_o = cpu_psg_addr;
	assign joy1_selected_o = joy1;

	// Keep the intentionally unused memory-cycle pins explicit; the real
	// motherboard's PPI cs is likewise independent of MREQ for I/O cycles.
	wire unused = &{1'b0, cpu_mreq_n, cpu_iorq_n, cpu_wr_n, cpu_rd_n,
		cpu_addr, cpu_do, cpu_di, io_rd, io_wr, ch_a, ch_b, ch_c,
		psg_active, psg_ioa_out, psg_iob_out};
endmodule
