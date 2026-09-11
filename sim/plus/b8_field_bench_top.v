// B8-2 Plus FIELD bench top: production motherboard in Plus mode on the
// NORMAL sync-filter path, with the production scaler consumer attached.
//
// Composition mirrors p1_mobo_bench_top.v (production Amstrad_motherboard,
// tie-off stubs for ga40010/YM2149/hid, C++-driven T80pa from b8_cpu),
// except:
//  - sync_filter comes from the bench input and the B8-2 test drives the
//    normal filtered path (1 = Raw pixels since B6: the complete regenerated
//    acquisition tuple, native pixel bytes), never Raw CRT (2).  Blanking is
//    unconnected here, so this bench sees the same selected sync either way;
//  - the extracted production consumer (rtl/video_interlace.v, the exact
//    Amstrad.sv history+enable) observes the motherboard's SELECTED filtered
//    VSYNC and FIELD pins, with the scandoubler path forced on
//    (scale=0, forced=1, so scandoubler = !interlace).
// C++ asserts on the consumer's interlace/scandoubler outputs (real RTL
// execution), keeping the raw asic_video VSYNC phase only as an oracle for
// MID-vs-seam expectations via hierarchical taps.

module b8_field_bench_top (
	input        clk,
	input        reset,
	// selected-machine outputs, for C++ assertions
	output [1:0] mode_o,
	output       hsync_o,
	output       vsync_o,      // selected + filtered VSYNC (consumer clock)
	output       vsync_field,  // selected FIELD (VGA_F1)
	output [2:0] interlace_o,  // production consumer history
	output       scandoubler_o,// production consumer enable
	input  [1:0] sync_filter_i,// B8-2 drives 1 (normal filtered path)
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
	wire [7:0] cpu_din = 8'hFF;

	wire mb_vsync, mb_field;

	Amstrad_motherboard mb
	(
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
		.sync_filter(sync_filter_i),
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
		.plus_asic_reset(reset),

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
		.hsync(hsync_o),
		.vsync(mb_vsync),
		.field(mb_field),

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

	assign vsync_o = mb_vsync;
	assign vsync_field = mb_field;

	// Production scaler consumer: the exact Amstrad.sv decision.
	video_interlace consumer
	(
		.clk(clk),
		.vsync_in(mb_vsync),
		.field_in(mb_field),
		.scale(2'd0),
		.forced_scandoubler(1'b1),
		.interlace(interlace_o),
		.scandoubler(scandoubler_o)
	);

endmodule
