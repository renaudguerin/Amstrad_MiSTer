// plus_mode=1 motherboard-level bench top (queued P1 review follow-up:
// the first bench that elaborates and runs Amstrad_motherboard itself).
//
// Instantiates the production motherboard with plus_mode=1, a scripted
// fake CPU (t80pa_bench_cpu.v), and tie-off stubs for ga40010/YM2149/hid
// (motherboard_lint_stubs.v — the motherboard's `.do(` pin forces
// --language 1364-2001, so those SystemVerilog children cannot join).
// Everything on the Plus path (asic_ga_timing, asic_video, CRTC-side
// muxes) is the real production RTL.

module p1_mobo_bench_top (
	input        clk,
	input        reset,
	// selected-machine outputs, for C++ assertions
	output [1:0] mode_o,
	output       hsync_o,
	output       vsync_o,
	output [3:0] red_o,
	output [3:0] green_o,
	output [3:0] blue_o,
	output       vsync_field,
	input        asic_page_on,
	input        plus_mode_i,
	output [7:0] vec_byte_o,
	output       vec_valid_o,
	output       asic_rd_o
);
	reg [1:0] cdiv;
	always @(posedge clk) begin
		if (reset) cdiv <= 2'd0;
		else cdiv <= (cdiv == 2'd3) ? 2'd0 : cdiv + 2'd1;
	end
	wire ce_16 = (cdiv == 2'd0); // 16 MHz dot-clock enable from 64 MHz clk
	wire [15:0] cpu_addr;
	wire        cpu_mreq, cpu_rd, cpu_m1;
	// Bench ROM bytes for the scripted opcode-fetch cycles. They seed the
	// production open-bus latch before each write-side-effect IN.
	wire [7:0] cpu_din = (cpu_mreq && cpu_rd && cpu_m1) ?
		((cpu_addr == 16'hF079) ? 8'h79 :
		 (cpu_addr == 16'hF003) ? 8'h03 :
		 (cpu_addr == 16'hF006) ? 8'h06 :
		 (cpu_addr == 16'hF066) ? 8'h66 : 8'hFF) : 8'hFF;

	Amstrad_motherboard mb
	(
		.reset(reset),
		.clk(clk),
		.ce_16(ce_16),

		.plus_mode(plus_mode_i),
		.plus_ram_128k(1'b0),
		.plus_has_fdc(1'b0),
		.plus_has_tape(1'b0),
		.plus_mem_wait(1'b0),
		.plus_aspage_on(asic_page_on),
		.plus_asic_dout(),
		.plus_vec_byte(vec_byte_o),
		.plus_vec_valid(vec_valid_o),
		.plus_asic_rd(asic_rd_o),

		.joy1(7'd0),
		.joy2(7'd0),
		.right_shift_mod(1'b0),
		.keypad_mod(1'b0),
		.ps2_key(11'd0),
		.ps2_mouse(25'd0),
		.joy1_sel(),
		.joy2_sel(),
		.key_nmi(),
		.key_reset(),
		.Fn(),

		.ppi_jumpers(4'd0),
		.crtc_type(1'b0),
		.sync_filter(1'b0),
		.no_wait(1'b0),

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
		.sna_ppi_control(8'd0),
		.sna_psg_addr(4'd0),
		.sna_psg_regs(128'd0),

		.plus_sna_wr(1'b0),
		.plus_sna_addr(14'd0),
		.plus_sna_data(8'd0),

		.tape_in(1'b0),
		.tape_out(),
		.tape_motor(),

		.audio_l(),
		.audio_r(),

		.mode(mode_o),

		.red(red_o),
		.green(green_o),
		.blue(blue_o),
		.hblank(),
		.vblank(),
		.hsync(hsync_o),
		.vsync(vsync_o),
		.field(vsync_field),

		.vram_din(16'd0),
		.vram_addr(),

		.rom_map(256'd0),
		.ram64k(1'b0),
		.mem_addr(),
		.mem_rd(),
		.mem_wr(),
		.romen(),
		.phi_n(),
		.phi_en_n(),
		.phi_en_p(),
		.cpu_addr(cpu_addr),
		.cpu_dout(),
		.cpu_din(cpu_din),
		.iorq(),
		.mreq(cpu_mreq),
		.rd(cpu_rd),
		.wr(),
		.m1(cpu_m1),
		.io_bus_byte(),
		.ga_ready(),
		.irq(1'b0),
		.nmi(1'b0),
		.cursor()
	);

endmodule
