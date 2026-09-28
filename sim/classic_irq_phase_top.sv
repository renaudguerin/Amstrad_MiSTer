// Classic CPC interrupt integration harness.
// Real production CRTC (type 0 and type 1) + ga40010 syncgen_sync +
// production T80pa (GHDL netlist) with normal motherboard WAIT.
// Sourced from Amstrad CPC CRTC Compendium (CC BY-NC-ND) and Zilog UM0080.
`timescale 1ns/1ps

module classic_irq_phase_top(
  input clk,
  input reset,
  input cpu_reset,
  input crtc_type,
  input mask_cpu_irq, // calibration only: keep second GA request off the CPU pin

  // RAM programming interface (64KB RAM)
  input prog_we,
  input [15:0] prog_addr,
  input [7:0] prog_data,
  input [15:0] peek_addr,
  output [7:0] peek_data,

  // CRTC snapshot initialization
  input sna_load,
  input [4:0] sna_addr,
  input [143:0] sna_regs,

  // Clock & Gate Array observability
  output phi_p,
  output phi_n,
  output cclk_n,
  output cclk_p,
  output ga_ready,
  output crtc_hs_o,
  output ga_irq_o,
  output [7:0] sequencer,
  output [5:0] icount,

  // CRTC counter observability
  output [6:0] crtc_row,
  output [4:0] crtc_line,
  output [7:0] crtc_hcc,

  // CPU bus observability
  output [15:0] t80_a,
  output [7:0] t80_dout,
  output [7:0] t80_din,
  output t80_mreq_n,
  output t80_iorq_n,
  output t80_rd_n,
  output t80_wr_n,
  output t80_m1_n,
  output t80_rfsh_n,
  output t80_halt_n,
  output insn_start,
  output [211:0] reg_state,

  // T80 internal cycle observability
  output [2:0] mc,
  output [2:0] ts,
  output cp_intcycle
);

  // Production clock divider from Amstrad.sv
  reg [2:0] div = 0;
  reg ce_16 = 0;
  always @(posedge clk) begin
    div <= div + 1'b1;
    ce_16 <= (div[1:0] == 0);
  end

  // 64KB RAM
  reg [7:0] ram [0:65535];
  integer i;
  initial begin
    for (i = 0; i < 65536; i = i + 1)
      ram[i] = 8'h00;
  end

  wire [15:0] A;
  wire [7:0] DO;
  wire M1_n, MREQ_n, IORQ_n, RD_n, WR_n, RFSH_n, HALT_n;
  wire [7:0] crtc_dout;

  wire mem_rd = ~(RD_n | MREQ_n);
  wire mem_wr = ~(WR_n | MREQ_n);
  wire io_rd  = ~(RD_n | IORQ_n);
  wire io_wr  = ~(WR_n | IORQ_n);

  wire crtc_enable = io_rd | io_wr;
  wire crtc_ncs    = A[14];
  wire crtc_rnw    = A[9];
  wire crtc_rs     = A[8];

  // IM 1 acknowledge reads FF from the idle CPC bus.
  // Memory read outranks IORQ to guard against early spurious IORQ during T80 reset.
  wire [7:0] DI = mem_rd ? ram[A] : ((io_rd & ~crtc_ncs) ? crtc_dout : 8'hFF);

  always @(posedge clk) begin
    if (prog_we)
      ram[prog_addr] <= prog_data;
    else if (mem_wr)
      ram[A] <= DO;
  end
  assign peek_data = ram[peek_addr];

  // GA signals
  wire phi_en_p, phi_en_n, cclk_en_p, cclk_en_n, ready_o;
  wire crtc_hs, crtc_vs, crtc_de;
  wire ga_irq;
  wire [5:0] ga_intcnt;

  // Motherboard wait equation: ready | (IORQ_n & MREQ_n)
  wire cpu_wait_n = ready_o | (IORQ_n & MREQ_n);

  T80pa cpu(
    .RESET_n(~(reset | cpu_reset)),
    .CLK(clk),
    .CEN_p(phi_en_p),
    .CEN_n(phi_en_n),
    .WAIT_n(cpu_wait_n),
    .INT_n(ga_irq | mask_cpu_irq),
    .NMI_n(1'b1),
    .BUSRQ_n(1'b1),
    .M1_n(M1_n),
    .MREQ_n(MREQ_n),
    .IORQ_n(IORQ_n),
    .RD_n(RD_n),
    .WR_n(WR_n),
    .RFSH_n(RFSH_n),
    .HALT_n(HALT_n),
    .BUSAK_n(),
    .OUT0(1'b0),
    .A(A),
    .DI(DI),
    .DO(DO),
    .R800_mode(1'b0),
    .REG(reg_state),
    .INSN_START(insn_start),
    .DIRSet(1'b0),
    .DIR(212'd0)
  );

  ga40010 ga(
    .clk(clk),
    .cen_16(ce_16),
    .fast(1'b0),
    .RESET_N(~reset),
    .A(A[15:14]),
    .D(DO),
    .MREQ_N(MREQ_n),
    .M1_N(M1_n),
    .RD_N(RD_n),
    .IORQ_N(IORQ_n),
    .HSYNC_I(crtc_hs),
    .VSYNC_I(crtc_vs),
    .DISPEN(crtc_de),
    .CCLK_EN_P(cclk_en_p),
    .CCLK_EN_N(cclk_en_n),
    .PHI_EN_P(phi_en_p),
    .PHI_EN_N(phi_en_n),
    .PHI_N(),
    .CCLK(),
    .RAS_N(),
    .CAS_N(),
    .CASAD_N(),
    .READY(ready_o),
    .CPU_N(),
    .MWE_N(),
    .E244_N(),
    .ROMEN_N(),
    .RAMRD_N(),
    .ROM(),
    .MODE(),
    .HSYNC_O(),
    .VSYNC_O(),
    .SYNC_N(),
    .INT_N(ga_irq),
    .VBLANK(),
    .BLUE_OE_N(),
    .BLUE(),
    .GREEN_OE_N(),
    .GREEN(),
    .RED_OE_N(),
    .RED(),
    .SNA_LOAD(1'b0),
    .SNA_INKSEL(5'd0),
    .SNA_PALETTE(136'd0),
    .SNA_CONFIG(8'd0),
    .SNAP_INKSEL(),
    .SNAP_BORDER(),
    .SNAP_INKR(),
    .SNAP_HROMEN(),
    .SNAP_LROMEN(),
    .SNAP_MODE(),
    .SNAP_INTCNT(ga_intcnt),
    .SNAP_HCNT()
  );

  CRTC crtc(
    .CLOCK(clk),
    .CLKEN(cclk_en_n),
    .nCLKEN(cclk_en_p),
    .nRESET(~reset),
    .CRTC_TYPE(crtc_type),
    .ENABLE(crtc_enable),
    .nCS(crtc_ncs),
    .R_nW(crtc_rnw),
    .RS(crtc_rs),
    .DI(DO),
    .DO(crtc_dout),
    .SNA_LOAD(sna_load),
    .SNA_ADDR(sna_addr),
    .SNA_REGS(sna_regs),
    .VSYNC(crtc_vs),
    .HSYNC(crtc_hs),
    .DE(crtc_de),
    .FIELD(),
    .CURSOR(),
    .MA(),
    .RA()
  );

  assign phi_p = phi_en_p;
  assign phi_n = phi_en_n;
  assign cclk_n = cclk_en_n;
  assign cclk_p = cclk_en_p;
  assign ga_ready = ready_o;
  assign crtc_hs_o = crtc_hs;
  assign ga_irq_o = ga_irq;

  assign crtc_row = crtc.row;
  assign crtc_line = crtc.line;
  assign crtc_hcc = crtc.hcc;

  assign t80_a = A;
  assign t80_dout = DO;
  assign t80_din = DI;
  assign t80_mreq_n = MREQ_n;
  assign t80_iorq_n = IORQ_n;
  assign t80_rd_n = RD_n;
  assign t80_wr_n = WR_n;
  assign t80_m1_n = M1_n;
  assign t80_rfsh_n = RFSH_n;
  assign t80_halt_n = HALT_n;

  assign sequencer = ga.S;
  assign icount = ga_intcnt;
  assign mc = cpu.mcycle;
  assign ts = cpu.tstate;
  assign cp_intcycle = ~cpu.intcycle_n;

endmodule
