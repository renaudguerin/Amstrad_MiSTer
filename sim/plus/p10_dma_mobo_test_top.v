// Production-motherboard DMA/PPI concurrency fixture (P10 hardware round 3).
//
// This top instantiates the real Amstrad_motherboard and replaces only its
// VHDL T80pa boundary with t80pa_dma_mobo_cpu.v.  The motherboard's other
// Verilog-2001 children use the existing full-port-list simulation stubs in
// motherboard_lint_stubs.v.  Consequently the Plus ASIC timing, CRTC3,
// ASIC-page decoder, DMA engine, 8255, and their production wait/strobe
// equations are all exercised together.
//
// The diagnostic taps are simulation-only.  They make the production
// dma_load_owner/dma_load_busy, PPI gating, and CPU wait relationship visible
// to the C++ trace without adding ports to production RTL.

module p10_dma_mobo_test_top (
	input        clk,
	input        reset,
	input        plus_aspage_on_i,
	input [15:0] vram_din_i,
	input [10:0] ps2_key_i,
	input        tape_in_i,

	output [15:0] cpu_addr_o,
	output        cpu_iorq_o,
	output        cpu_mreq_o,
	output        cpu_rd_o,
	output        cpu_wr_o,
	output        cpu_m1_o,
	output [7:0]  cpu_di_o,
	output [7:0]  cpu_do_o,
	output        cpu_wait_n_o,
	output        dma_load_owner_o,
	output        dma_load_busy_o,
	output        dma_ppi_wait_o,
	output        cpu_ppi_access_o,
	output        cpu_psg_write_o,
	output        ppi_we_o,
	output        ppi_oe_o,
	output        ppi_cs_o,
	output [7:0]  ppi_port_a_o,
	output [7:0]  ppi_port_c_o,
	output        dma_psg_active_o,
	output        cclk_en_p_o,
	output        cclk_en_n_o,
	output        phi_en_p_o,
	output        phi_en_n_o,
	output        ready_o,
	output        hsync_o,
	output        cpu_done_o,
	output        cpu_timing_error_o,
	output        cpu_read_error_o,
	output [31:0] cpu_wait_stalls_o,
	output [31:0] cpu_cclk_waits_o,
	output [31:0] cpu_operations_o
);
	reg [1:0] cdiv;
	always @(posedge clk) begin
		if (reset) cdiv <= 2'd0;
		else cdiv <= (cdiv == 2'd3) ? 2'd0 : cdiv + 2'd1;
	end
	wire ce_16 = (cdiv == 2'd0);

	wire [15:0] cpu_addr;
	wire [7:0]  cpu_dout;
	wire [7:0]  cpu_din = 8'hFF;
	wire        cpu_iorq;
	wire        cpu_mreq;
	wire        cpu_rd;
	wire        cpu_wr;
	wire        cpu_m1;
	wire        ppi_cs = ~cpu_addr[11];
	wire [7:0]  plus_asic_dout;
	wire        plus_asic_rd;
	wire [7:0]  plus_vec_byte;
	wire        plus_vec_valid;
	wire        hblank;
	wire        vblank;
	wire        hsync;
	wire        vsync;
	wire        field;
	wire [1:0]  mode;
	wire [3:0]  red;
	wire [3:0]  green;
	wire [3:0]  blue;
	wire [14:0] vram_addr;
	wire [22:0] mem_addr;
	wire        mem_rd;
	wire        mem_wr;
	wire        romen;
	wire        phi_n;
	wire        phi_en_n;
	wire        phi_en_p;
	wire        io_bus_byte;
	wire        ga_ready;
	wire        cursor;
	wire [7:0] audio_l;
	wire [7:0] audio_r;
	wire        tape_out;
	wire        tape_motor;
	wire        joy1_sel;
	wire        joy2_sel;
	wire        key_nmi;
	wire        key_reset;
	wire [9:0] Fn;

	Amstrad_motherboard mb
	(
		.reset(reset),
		.clk(clk),
		.ce_16(ce_16),

		.plus_mode(1'b1),
		.plus_unlocked(1'b0),
		.plus_ram_128k(1'b0),
		.plus_has_fdc(1'b0),
		.plus_has_tape(1'b1),
		.plus_mem_wait(1'b0),
		.plus_aspage_on(plus_aspage_on_i),
		.plus_asic_dout(plus_asic_dout),
		.plus_vec_byte(plus_vec_byte),
		.plus_vec_valid(plus_vec_valid),
		.plus_asic_rd(plus_asic_rd),

		.joy1(7'd0),
		.joy2(7'd0),
		.right_shift_mod(1'b0),
		.keypad_mod(1'b0),
		.ps2_key(ps2_key_i),
		.ps2_mouse(25'd0),
		.joy1_sel(joy1_sel),
		.joy2_sel(joy2_sel),
		.key_nmi(key_nmi),
		.key_reset(key_reset),
		.Fn(Fn),

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
		.plus_asic_reset(reset),

		.tape_in(tape_in_i),
		.tape_out(tape_out),
		.tape_motor(tape_motor),

		.audio_l(audio_l),
		.audio_r(audio_r),

		.mode(mode),

		.red(red),
		.green(green),
		.blue(blue),
		.hblank(hblank),
		.vblank(vblank),
		.hsync(hsync),
		.vsync(vsync),
		.field(field),

		.vram_din(vram_din_i),
		.vram_addr(vram_addr),

		.rom_map(256'd0),
		.ram64k(1'b0),
		.mem_addr(mem_addr),
		.mem_rd(mem_rd),
		.mem_wr(mem_wr),
		.romen(romen),
		.phi_n(phi_n),
		.phi_en_n(phi_en_n),
		.phi_en_p(phi_en_p),
		.cpu_addr(cpu_addr),
		.cpu_dout(cpu_dout),
		.cpu_din(cpu_din),
		.iorq(cpu_iorq),
		.mreq(cpu_mreq),
		.rd(cpu_rd),
		.wr(cpu_wr),
		.m1(cpu_m1),
		.io_bus_byte(),
		.ga_ready(ga_ready),
		.irq(1'b0),
		.nmi(1'b0),
		.cursor(cursor)
	);

	// The production wait equation is evaluated at the motherboard's CPU
	// boundary.  These derived signals use the motherboard's active-high pin
	// outputs and therefore retain the exact active-low expression:
	// (ready | (IORQ_N & MREQ_N)) & ~dma_ppi_wait.
	wire dma_load_owner = mb.dma_sound.dma_load_owner;
	wire dma_load_busy = mb.dma_sound.dma_load_busy;
	wire cpu_ppi_access = (~cpu_addr[11]) && cpu_iorq && (cpu_rd || cpu_wr);
	wire dma_ppi_wait = mb.dma_ppi_wait;
	wire ppi_we = cpu_iorq && cpu_wr && !dma_ppi_wait;
	wire ppi_oe = cpu_iorq && cpu_rd && !dma_ppi_wait;
	wire cpu_wait_n = (ga_ready || (!cpu_iorq && !cpu_mreq)) && !dma_ppi_wait;
	wire cpu_psg_write = mb.cpu_psg_write;

	// PPI and DMA diagnostics.  Internal references are restricted to this
	// simulation top; production Amstrad_motherboard ports are untouched.
	assign cpu_addr_o         = cpu_addr;
	assign cpu_iorq_o         = cpu_iorq;
	assign cpu_mreq_o         = cpu_mreq;
	assign cpu_rd_o           = cpu_rd;
	assign cpu_wr_o           = cpu_wr;
	assign cpu_m1_o           = cpu_m1;
	assign cpu_di_o           = mb.cpu_data_bus;
	assign cpu_do_o           = cpu_dout;
	assign cpu_wait_n_o       = cpu_wait_n;
	assign dma_load_owner_o   = dma_load_owner;
	assign dma_load_busy_o    = dma_load_busy;
	assign dma_ppi_wait_o     = dma_ppi_wait;
	assign cpu_ppi_access_o   = cpu_ppi_access;
	assign cpu_psg_write_o    = cpu_psg_write;
	assign ppi_we_o           = ppi_we;
	assign ppi_oe_o           = ppi_oe;
	assign ppi_cs_o           = ppi_cs;
	assign ppi_port_a_o       = mb.portAout;
	assign ppi_port_c_o       = mb.portC;
	assign dma_psg_active_o   = mb.psg_dma_active;
	assign cclk_en_p_o        = mb.plus_cclk_en_p;
	assign cclk_en_n_o        = mb.plus_cclk_en_n;
	assign phi_en_p_o         = phi_en_p;
	assign phi_en_n_o         = phi_en_n;
	assign ready_o            = ga_ready;
	assign hsync_o            = hsync;
	assign cpu_done_o         = mb.CPU.dbg_done;
	assign cpu_timing_error_o = mb.CPU.dbg_timing_error;
	assign cpu_read_error_o   = mb.CPU.dbg_read_error;
	assign cpu_wait_stalls_o  = mb.CPU.dbg_wait_stalls;
	assign cpu_cclk_waits_o   = mb.CPU.dbg_cclk_waits;
	assign cpu_operations_o   = mb.CPU.dbg_operations;

	wire unused = &{1'b0, cpu_dout, plus_asic_dout, plus_asic_rd,
		plus_vec_byte, plus_vec_valid, hblank, vblank, vsync, field, mode,
		red, green, blue, vram_addr, mem_addr, mem_rd, mem_wr, romen, phi_n,
		joy1_sel, joy2_sel, key_nmi, key_reset, Fn, audio_l, audio_r,
		tape_out, tape_motor, cursor, ppi_cs, ppi_port_a_o, ppi_port_c_o,
		dma_psg_active_o, cclk_en_n_o, phi_en_n_o, ready_o, cpu_di_o,
		cpu_m1_o, cpu_mreq_o, cpu_rd_o, cpu_wr_o};
endmodule
