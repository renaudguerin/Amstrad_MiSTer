// B8-3 palette event bench top: production motherboard in Plus mode with a
// C++-driven bus source, exercising the real legacy I/O -> ASIC palette path.
//
// Composition mirrors b8_field_bench_top.v (production Amstrad_motherboard,
// tie-off stubs for ga40010/YM2149/hid, C++-driven T80pa from b8_palette_cpu),
// except:
//  - the CPU source supports I/O writes (legacy PENR/INKR at &7Fxx) AND
//    memory writes/reads (ASIC palette page at &6400-&643F) through the
//    production decode;
//  - cpu_din is muxed from the production plus_asic_dout when plus_asic_rd
//    (the exact Amstrad.sv selection for the enabled page), so page reads are
//    real bus reads, not hierarchical palette peeks;
//  - asic_page_on stands in for plus_mmu's captured RMR2 state (as in the
//    p1 bench); plus_mode=1.
// C++ programs legacy colours and page colours over the bus and reads the
// page back over the bus. No C++ pokes colour-shadow arrays as write events.

module b8_palette_bench_top (
	input        clk,
	input        reset,
	input        asic_reset_i,
	input        plus_mode_i,
	input        asic_page_on,
	output [1:0] mode_o,
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
	wire [7:0]  plus_asic_dout;
	wire        plus_asic_rd;
	wire [7:0]  vec_byte;
	wire        vec_valid;
	// Production-equivalent CPU read mux for the enabled ASIC page
	// (Amstrad.sv: plus_asic_rd ? plus_asic_dout : bus). The bench has no
	// SDRAM/cartridge sources, so the idle level is FF.
	wire [7:0] cpu_din = plus_asic_rd ? plus_asic_dout : 8'hFF;

	Amstrad_motherboard mb
	(
	// SSM opcode-fetch tap: unused here, wired explicitly so the -Wall
	// lint stays quiet about it.
	.ssm_m1_fetch(),
	.ssm_bus_data(),
		.reset(reset),
		.clk(clk),
		.ce_16(ce_16),

		.plus_mode(plus_mode_i),
		.plus_unlocked(1'b0),
		.plus_ram_128k(1'b0),
		.plus_has_fdc(1'b0),
		.plus_has_tape(1'b0),
		.plus_mem_wait(1'b0),
		.plus_aspage_on(asic_page_on),
		.plus_asic_dout(plus_asic_dout),
		.plus_vec_byte(vec_byte),
		.plus_vec_valid(vec_valid),
		.plus_asic_rd(plus_asic_rd),

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
		.sync_filter(2'd2), // raw path: palette is filter-independent
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
		// Production split: machine reset drives the GA/CPU, ASIC-only
		// reset pulses plus_asic_reset while the GA shadows are retained.
		.plus_asic_reset(reset | asic_reset_i),

		.tape_in(1'b0),
		.tape_out(),
		.tape_motor(),

		.audio_l(),
		.audio_r(),

		.mode(mode_o),

		.red(),
		.green(),
		.blue(),
		.hblank(),
		.vblank(),
		.hsync(),
		.vsync(),
		.field(),

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

	assign asic_rd_o = plus_asic_rd;

endmodule
