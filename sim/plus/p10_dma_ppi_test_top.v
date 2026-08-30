// Production-shaped P10e DMA/PPI/PSG/input concurrency fixture.
//
// The complete rtl/Amstrad_motherboard.v cannot be elaborated with the real
// children under Verilator: its VHDL T80pa instance uses `.do`, which is a
// SystemVerilog keyword.  This seam therefore keeps the exact motherboard
// equations around rtl/Amstrad_motherboard.v:200, 757-810 while instantiating
// the real asic_dma, asic_ga_timing, i8255, YM2149, hid, and joydb modules.
// The omitted boundary is only the VHDL CPU and the unrelated video/MMU/
// classic Gate Array children; CPU bus pins are driven by the deterministic
// C++ script below.
//
// Source-derived expectations are from
// docs/plus/references/asic-reference.md §9, "Timing & bus interaction":
// LOAD is >=8 CCLK cycles, +1 when a CPU access simultaneously uses the 8255,
// and +2 when that access is a PSG register write; the 8255, selected AY
// register, and AY read/write state are restored afterwards.

module p10_dma_ppi_test_top (
	input        clk,
	input        reset,
	input        hsync_i,

	// Scripted Z80 bus, using the same active-low pin convention as T80pa.
	input  [15:0] cpu_addr_i,
	input   [7:0] cpu_do_i,
	input         cpu_rd_n_i,
	input         cpu_wr_n_i,
	input         cpu_iorq_n_i,
	input         cpu_mreq_n_i,
	input         cpu_m1_n_i,

	// Top-level input sources.  The joydb -> DB9 packing is copied from
	// Amstrad.sv and p10_input_test_top.v.
	input  [10:0] ps2_key_i,
	input  [6:0]  user_in_i,
	input  [1:0]  snac_player_i,
	input  [6:0]  joy1_usb_i,
	input  [6:0]  joy2_usb_i,

	// Direct register/configuration seam for the already-decoded ASIC page.
	// These pins stand in for asic_regs' SAR/PPR/DCSR outputs; only the DMA
	// register decoder is outside this focused concurrency surface.
	input  [15:0] dma_sar0_i,
	input         dma_sar0_wr_i,
	input   [7:0] dma_ppr0_i,
	input   [2:0] dma_ena_i,
	input  [15:0] dma_instr_addr_i,
	input  [15:0] dma_instr_i,

	output  [7:0] cpu_di_o,
	output        wait_n_o,
	output        dma_ppi_wait_o,
	output        cpu_ppi_access_o,
	output        cpu_psg_write_o,
	output        ppi_cs_o,
	output        ppi_we_o,
	output        ppi_oe_o,
	output  [7:0] ppi_port_a_o,
	output  [7:0] ppi_port_c_o,
	output  [7:0] psg_addr_o,
	output  [7:0] psg_do_o,
	output        psg_bdir_o,
	output        psg_bc1_o,
	output  [7:0] psg_di_o,
	output        dma_load_owner_o,
	output        dma_load_busy_o,
	output        dma_psg_active_o,
	output        dma_ram_req_o,
	output [15:0] dma_ram_addr_o,
	output        cclk_en_p_o,
	output        cclk_en_n_o,
	output        phi_en_p_o,
	output        phi_en_n_o,
	output        timing_ready_o,
	output  [7:0] key_matrix_o,
	output  [6:0] joy1_selected_o,
	output  [6:0] joy2_selected_o
);
	reg [1:0] cdiv;
	always @(posedge clk) begin
		if (reset) cdiv <= 2'd0;
		else cdiv <= (cdiv == 2'd3) ? 2'd0 : cdiv + 2'd1;
	end
	wire ce_16 = (cdiv == 2'd0);

	wire [15:0] cpu_addr = cpu_addr_i;
	wire  [7:0] cpu_do = cpu_do_i;
	wire        cpu_rd_n = cpu_rd_n_i;
	wire        cpu_wr_n = cpu_wr_n_i;
	wire        cpu_iorq_n = cpu_iorq_n_i;
	wire        cpu_mreq_n = cpu_mreq_n_i;
	wire        cpu_m1_n = cpu_m1_n_i;
	wire io_rd = ~(cpu_rd_n | cpu_iorq_n);
	wire io_wr = ~(cpu_wr_n | cpu_iorq_n);

	// The production Plus timing owner supplies CCLK_EN_P and READY to both
	// the Z80 and the AY.  Its unrelated raster/register outputs are tied off.
	wire cclk_en_p;
	wire cclk_en_n;
	wire phi_en_p;
	wire phi_en_n;
	wire timing_ready;
	asic_ga_timing timing
	(
		.clk(clk),
		.cen_16(ce_16),
		.fast(1'b0),
		.RESET_N(~reset),
		.plus_unlocked(1'b0),
		.A(cpu_addr[15:14]),
		.D(cpu_do),
		.MREQ_N(cpu_mreq_n),
		.M1_N(cpu_m1_n),
		.RD_N(cpu_rd_n),
		.IORQ_N(cpu_iorq_n),
		.HSYNC_I(hsync_i),
		.VSYNC_I(1'b1),
		.CCLK(),
		.CCLK_EN_P(cclk_en_p),
		.CCLK_EN_N(cclk_en_n),
		.PHI_N(),
		.PHI_EN_N(phi_en_n),
		.PHI_EN_P(phi_en_p),
		.RAS_N(),
		.CAS_N(),
		.CASAD_N(),
		.READY(timing_ready),
		.CPU_N(),
		.MWE_N(),
		.E244_N(),
		.ROMEN_N(),
		.RAMRD_N(),
		.ROM(),
		.pri(8'd0),
		.crtc_line(9'd0),
		.crtc_adj(1'b1),
		.intack(1'b0),
		.int_last_raster(),
		.HSYNC_O(),
		.VSYNC_O(),
		.SYNC_N(),
		.INT_N(),
		.VBLANK(),
		.MODE_SYNC_EN(),
		.MODE(),
		.BORDER_O(),
		.INKR_O(),
		.GAMODE_O()
	);

	// This is the exact production wait expression from
	// rtl/Amstrad_motherboard.v:203-204 with plus_mem_wait/no_wait absent from
	// this seam.  dma_ppi_wait and the two PPI strobes are the expressions at
	// lines 765 and 783-784 respectively.
	wire dma_load_owner;
	wire cpu_ppi_access = ~cpu_addr[11] & (io_rd | io_wr);
	wire cpu_ppi_write = cpu_ppi_access & io_wr;
	reg cpu_ppi_started;
	reg [7:0] cpu_ppi_read_latch;
	always @(posedge clk) begin
		if (reset) begin
			cpu_ppi_started <= 1'b0;
			cpu_ppi_read_latch <= 8'hFF;
		end
		else begin
			if (!cpu_ppi_access) cpu_ppi_started <= 1'b0;
			else if (!dma_load_owner) cpu_ppi_started <= 1'b1;
			if (cpu_ppi_access && io_rd && !dma_load_owner && !cpu_ppi_started)
				cpu_ppi_read_latch <= ppi_dout;
		end
	end
	wire dma_ppi_wait = dma_load_owner & cpu_ppi_access & ~cpu_ppi_started;
	wire wait_n = (timing_ready | (cpu_iorq_n & cpu_mreq_n)) & ~dma_ppi_wait;
	wire cpu_bsr_pc7 = cpu_ppi_write & (cpu_addr[9:8] == 2'b11) &
		~cpu_do[7] & (cpu_do[3:1] == 3'd7);
	wire cpu_bsr_pc6 = cpu_ppi_write & (cpu_addr[9:8] == 2'b11) &
		~cpu_do[7] & (cpu_do[3:1] == 3'd6);
	wire [1:0] cpu_pc76_after = (cpu_addr[9:8] == 2'b10) ? cpu_do[7:6] :
		{cpu_bsr_pc7 ? cpu_do[0] : portC[7],
		 cpu_bsr_pc6 ? cpu_do[0] : portC[6]};
	wire cpu_psg_write = cpu_ppi_write & (cpu_pc76_after == 2'b10) &
		((cpu_addr[9:8] == 2'b00) | (cpu_addr[9:8] == 2'b10) |
		 cpu_bsr_pc7 | cpu_bsr_pc6);

	// The DMA fetch is a deterministic RAM service at the configured SAR;
	// no main-memory arbitration is under test here.
	wire [15:0] dma_ram_data = (dma_ram_addr == dma_instr_addr_i) ?
		dma_instr_i : 16'h0000;
	wire [15:0] dma_ram_addr;
	wire        dma_ram_req;

	wire [2:0] dma_ena_clr;
	wire [2:0] dma_int_set;
	wire [15:0] sar0_addr_unused;
	wire [15:0] sar1_addr_unused;
	wire [15:0] sar2_addr_unused;
	wire        dma_psg_bdir_w;
	wire        dma_psg_bc1_w;
	wire [7:0]  dma_psg_dout_w;
	wire        dma_psg_active;
	wire        dma_load_busy;
	asic_dma dma_sound
	(
		.clk(clk),
		.reset(reset),
		.cclk_en_p(cclk_en_p),
		.cclk_en_n(cclk_en_n),
		.hsync(hsync_i),
		.sar0_lo(dma_sar0_i[7:0]),
		.sar0_hi(dma_sar0_i[15:8]),
		.ppr0(dma_ppr0_i),
		.sar0_wr(dma_sar0_wr_i),
		.sar1_lo(8'd0),
		.sar1_hi(8'd0),
		.ppr1(8'd0),
		.sar1_wr(1'b0),
		.sar2_lo(8'd0),
		.sar2_hi(8'd0),
		.ppr2(8'd0),
		.sar2_wr(1'b0),
		.dcsr_ena(dma_ena_i),
		.dcsr_ena_clr(dma_ena_clr),
		.dma_int_set(dma_int_set),
		.sar0_addr(sar0_addr_unused),
		.sar1_addr(sar1_addr_unused),
		.sar2_addr(sar2_addr_unused),
		.ram_req(dma_ram_req),
		.ram_addr(dma_ram_addr),
		.ram_data(dma_ram_data),
		.cpu_psg_addr(cpu_psg_addr),
		.cpu_ppi_access(cpu_ppi_access),
		.cpu_psg_write(cpu_psg_write),
		.dma_load_owner(dma_load_owner),
		.dma_load_busy(dma_load_busy),
		.psg_bdir(dma_psg_bdir_w),
		.psg_bc1(dma_psg_bc1_w),
		.psg_dout(dma_psg_dout_w),
		.psg_active(dma_psg_active)
	);

	wire [7:0] ppi_dout;
	wire [7:0] portC;
	wire [7:0] portAout;
	wire [7:0] portAin;
	wire [7:0] psg_do;
	wire [7:0] kbd_out;
	reg  [7:0] cpu_psg_addr;

	// Exact production address shadow from rtl/Amstrad_motherboard.v:767-771:
	// DMA owns the AY bus while active, so its restore write cannot be
	// mistaken for a CPU-selected register.
	always @(posedge clk) begin
		if (reset) cpu_psg_addr <= 8'd0;
		else if (!dma_psg_active && portC[7] && portC[6])
			cpu_psg_addr <= portAout;
	end

	i8255 PPI
	(
		.reset(reset),
		.clk_sys(clk),
		.addr(cpu_addr[9:8]),
		.idata(cpu_do),
		.odata(ppi_dout),
		.cs(~cpu_addr[11]),
		.we(io_wr & ~dma_ppi_wait),
		.oe(io_rd & ~dma_ppi_wait),
		.ipa(portAin),
		.opa(portAout),
		.ipb(8'hFF),
		.opb(),
		.ipc(8'hFF),
		.opc(portC),
		.plus_mode(1'b1),
		.sna_load(1'b0),
		.sna_opa(8'd0),
		.sna_opb(8'd0),
		.sna_opc(8'd0),
		.sna_control(8'h9B)
	);

	wire psg_bc_mux = dma_psg_active ? dma_psg_bc1_w : portC[6];
	wire psg_bdir_mux = dma_psg_active ? dma_psg_bdir_w : portC[7];
	wire [7:0] psg_di_mux = dma_psg_active ? dma_psg_dout_w : portAout;
	YM2149 PSG
	(
		.RESET(reset),
		.CLK(clk),
		.CE(cclk_en_p),
		.SEL(1'b0),
		.MODE(1'b0),
		.BC(psg_bc_mux),
		.BDIR(psg_bdir_mux),
		.DI(psg_di_mux),
		.DO(psg_do),
		.CHANNEL_A(),
		.CHANNEL_B(),
		.CHANNEL_C(),
		.ACTIVE(),
		.IOA_in(kbd_out),
		.IOB_in(8'hFF),
		.IOA_out(),
		.IOB_out(),
		.SNA_LOAD(1'b0),
		.SNA_ADDR(4'd0),
		.SNA_REGS(128'd0)
	);
	assign portAin = psg_do;

	// Exact Amstrad.sv joydb selection/DB9 packing and motherboard HID pins.
	wire [15:0] joydb_1;
	wire [15:0] joydb_2;
	wire        joydb_1ena;
	wire        joydb_2ena;
	joydb joydb_i
	(
		.USER_IN(user_in_i),
		.snac_player(snac_player_i),
		.joystick1(joydb_1),
		.joystick2(joydb_2),
		.joystick1_en(joydb_1ena),
		.joystick2_en(joydb_2ena)
	);
	wire [6:0] joy1_db9 = {joydb_1[10], joydb_1[6], joydb_1[4], joydb_1[3:0]};
	wire [6:0] joy2_db9 = {joydb_2[10], joydb_2[6], joydb_2[4], joydb_2[3:0]};
	wire [6:0] joy1 = joydb_1ena ? joy1_db9 : joy1_usb_i;
	wire [6:0] joy2 = joydb_2ena ? joy2_db9 : joydb_1ena ? joy1_usb_i : joy2_usb_i;

	hid HID
	(
		.reset(reset),
		.clk(clk),
		.right_shift_mod(1'b0),
		.keypad_mod(1'b0),
		.ps2_key(ps2_key_i),
		.ps2_mouse(25'd0),
		.joystick1(joy1),
		.joystick2(joy2),
		.Y(portC[3:0]),
		.X(kbd_out),
		.key_nmi(),
		.key_reset(),
		.Fn()
	);

	assign cpu_di_o            = (cpu_ppi_started && io_rd) ?
		cpu_ppi_read_latch : ppi_dout;
	assign wait_n_o           = wait_n;
	assign dma_ppi_wait_o     = dma_ppi_wait;
	assign cpu_ppi_access_o   = cpu_ppi_access;
	assign cpu_psg_write_o    = cpu_psg_write;
	assign ppi_cs_o           = ~cpu_addr[11];
	assign ppi_we_o           = io_wr & ~dma_ppi_wait;
	assign ppi_oe_o           = io_rd & ~dma_ppi_wait;
	assign ppi_port_a_o       = portAout;
	assign ppi_port_c_o       = portC;
	assign psg_addr_o         = cpu_psg_addr;
	assign psg_do_o           = psg_do;
	assign psg_bdir_o         = psg_bdir_mux;
	assign psg_bc1_o          = psg_bc_mux;
	assign psg_di_o           = psg_di_mux;
	assign dma_load_owner_o   = dma_load_owner;
	assign dma_load_busy_o    = dma_load_busy;
	assign dma_psg_active_o   = dma_psg_active;
	assign dma_ram_req_o     = dma_ram_req;
	assign dma_ram_addr_o    = dma_ram_addr;
	assign cclk_en_p_o       = cclk_en_p;
	assign cclk_en_n_o       = cclk_en_n;
	assign phi_en_p_o        = phi_en_p;
	assign phi_en_n_o        = phi_en_n;
	assign timing_ready_o    = timing_ready;
	assign key_matrix_o      = kbd_out;
	assign joy1_selected_o   = joy1;
	assign joy2_selected_o   = joy2;

	// Keep all diagnostic-only outputs live in strict lint builds.  These are
	// deliberately not part of the production path.
	wire unused = &{1'b0, cpu_m1_n, cpu_mreq_n, cpu_rd_n, cpu_wr_n, cpu_iorq_n,
		cpu_addr, cpu_do, ce_16, phi_en_p, phi_en_n, cclk_en_n, dma_ena_clr,
		dma_int_set, sar0_addr_unused, sar1_addr_unused, sar2_addr_unused,
		psg_bdir_o, psg_bc1_o, psg_di_o, dma_ram_req_o, dma_ram_addr_o,
		ppi_cs_o, ppi_port_a_o, ppi_port_c_o, psg_addr_o, psg_do_o,
		joy2_selected_o};
endmodule
