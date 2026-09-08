// Production T80pa executed-instruction validation harness for B8-1.
// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND).
// Connects production T80pa -> real ga40010 divider/enables/WAIT -> CRTC.
`timescale 1ns/1ps

module crtc_t80_top(
  input clk,
  input reset,
  input cpu_reset,
  input crtc_type,
  input sna_load,
  input [4:0] sna_addr,
  input [143:0] sna_regs,

  // RAM programming interface
  input prog_we,
  input [7:0] prog_addr,
  input [7:0] prog_data,

  // CRTC observability
  output [7:0] c0,
  output [6:0] row,
  output [4:0] line,
  output [4:0] r5,
  output [7:0] r0,
  output [13:0] ma,
  output pending,
  output in_adj,
  output vde,
  output vde_r,
  output arm,
  output vma_flag,
  output parity_flag,
  output [4:0] crtc_addr,

  // GA & clock observability
  output [7:0] seq,
  output ce16,
  output phi_p,
  output phi_n,
  output cclk_n,
  output cclk_p,
  output ga_ready,

  // CPU bus observability
  output [15:0] t80_a,
  output [7:0] t80_dout,
  output [7:0] t80_din,
  output t80_mreq_n,
  output t80_iorq_n,
  output t80_rd_n,
  output t80_wr_n,
  output t80_m1_n,
  output t80_halt_n,
  output write_active
);

  // Production clock divider from Amstrad.sv
  reg [2:0] div = 0;
  reg ce_16 = 0;
  always @(posedge clk) begin
    div <= div + 1'b1;
    ce_16 <= (div[1:0] == 0);
  end

  // 256-byte RAM for executed test program and data
  reg [7:0] ram [0:255];
  integer i;
  initial begin
    for (i = 0; i < 256; i = i + 1)
      ram[i] = 8'h00;
  end

  // CPU signals
  wire [15:0] A;
  wire [7:0] DO;
  wire M1_n, MREQ_n, IORQ_n, RD_n, WR_n, RFSH_n, HALT_n, BUSAK_n;
  wire [7:0] crtc_dout;

  // Motherboard address decoding for CRTC (exact logic from Amstrad_motherboard.v)
  // Port &BC00: A[14]=0, A[9]=0, A[8]=0 -> Select register
  // Port &BD00: A[14]=0, A[9]=0, A[8]=1 -> Write data
  // Port &BE00: A[14]=0, A[9]=1, A[8]=0 -> Read status
  // Port &BF00: A[14]=0, A[9]=1, A[8]=1 -> Read data
  wire io_rd = ~(RD_n | IORQ_n);
  wire io_wr = ~(WR_n | IORQ_n);
  wire crtc_enable = io_rd | io_wr;
  wire crtc_ncs = A[14];
  wire crtc_rnw = A[9];
  wire crtc_rs  = A[8];

  wire mem_rd = ~(RD_n | MREQ_n);
  wire mem_wr = ~(WR_n | MREQ_n);

  wire [7:0] DI = mem_rd ? ram[A[7:0]] : ((io_rd & ~crtc_ncs) ? crtc_dout : 8'hFF);

  always @(posedge clk) begin
    if (prog_we)
      ram[prog_addr] <= prog_data;
    else if (mem_wr)
      ram[A[7:0]] <= DO;
  end

  // GA signals
  wire phi_en_p, phi_en_n, cclk_en_p, cclk_en_n, ready_o;
  wire hs, vs, de;

  // Motherboard wait state logic: ready | (IORQ_n & MREQ_n)
  wire cpu_wait_n = ready_o | (IORQ_n & MREQ_n);

  T80pa cpu(
    .RESET_n(~(reset | cpu_reset)),
    .CLK(clk),
    .CEN_p(phi_en_p),
    .CEN_n(phi_en_n),
    .WAIT_n(cpu_wait_n),
    .INT_n(1'b1),
    .NMI_n(1'b1),
    .BUSRQ_n(1'b1),
    .M1_n(M1_n),
    .MREQ_n(MREQ_n),
    .IORQ_n(IORQ_n),
    .RD_n(RD_n),
    .WR_n(WR_n),
    .RFSH_n(RFSH_n),
    .HALT_n(HALT_n),
    .BUSAK_n(BUSAK_n),
    .OUT0(1'b0),
    .A(A),
    .DI(DI),
    .DO(DO),
    .R800_mode(1'b0),
    .REG(),
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
    .HSYNC_I(hs),
    .VSYNC_I(vs),
    .DISPEN(de),
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
    .INT_N(),
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
    .SNA_CONFIG(8'd0)
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
    .VSYNC(vs),
    .HSYNC(hs),
    .DE(de),
    .FIELD(),
    .CURSOR(),
    .MA(ma),
    .RA()
  );

  // Assignments
  assign c0 = crtc.hcc;
  assign row = crtc.row;
  assign line = crtc.line;
  assign r5 = crtc.R5_v_total_adj;
  assign r0 = crtc.R0_h_total;
  assign pending = crtc_type ? crtc.crtc_type1_engine.rfd_r0_pending : crtc.crtc_type0_engine.type0_r0_widen_pending;
  assign in_adj = crtc.in_adj;
  assign vde = crtc.vde;
  assign vde_r = crtc.vde_r;
  assign arm = crtc.crtc_type1_engine.rfd_arm;
  assign vma_flag = crtc.crtc_type1_engine.rfd_vma_flag;
  assign parity_flag = crtc.crtc_type1_engine.rfd_parity_flag;
  assign crtc_addr = crtc.addr;

  assign seq = ga.S;
  assign ce16 = ce_16;
  assign phi_p = phi_en_p;
  assign phi_n = phi_en_n;
  assign cclk_n = cclk_en_n;
  assign cclk_p = cclk_en_p;
  assign ga_ready = ready_o;

  assign t80_a = A;
  assign t80_dout = DO;
  assign t80_din = DI;
  assign t80_mreq_n = MREQ_n;
  assign t80_iorq_n = IORQ_n;
  assign t80_rd_n = RD_n;
  assign t80_wr_n = WR_n;
  assign t80_m1_n = M1_n;
  assign t80_halt_n = HALT_n;
  assign write_active = io_wr & ~crtc_ncs & ~crtc_rnw;

endmodule
