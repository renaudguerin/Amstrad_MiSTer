// B9 MID FRAME diagnostic top; instrumentation only, no CRTC rule implementation.
// Skeleton copy of sim/crtc_t80_top.sv with: 64 KiB RAM (full SHAKER payload
// at 0x3900 + screen at 0xC000 + SMC areas), PPI Port-B (F5xx) bit 0 driven
// from raw CRTC VSYNC (rtl/Amstrad_motherboard.v: ppi_ipb bit0 = vs_sel;
// upper bits fixed 1), and
// added trace ports for the B9 discriminator (tog_stage/ivm/parities/VSYNC).
// Production T80pa, real GA40010 divider/enables/WAIT and tracked CRTC RTL
// are preserved unchanged (same instantiation as sim/crtc_t80_top.sv).
`timescale 1ns/1ps

module b9_t80_top(
  input clk,
  input reset,
  input cpu_reset,
  input crtc_type,
  input sna_load,
  input [4:0] sna_addr,
  input [143:0] sna_regs,

  // RAM programming interface (16-bit: full 64 KiB)
  input prog_we,
  input [15:0] prog_addr,
  input [7:0] prog_data,
  input [15:0] peek_addr,
  output [7:0] peek_data,
  output [211:0] reg_state,
  output insn_start,

  // CRTC observability (as in crtc_t80_top)
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
  output write_active,

  // B9 discriminator trace ports
  output line_new_o,
  output r8_write_hit_o,
  output [1:0] tog_stage,
  output tog_enter,
  output ivm,
  output parity_c9_o,
  output parity_frame_o,
  output parity_r6_o,
  output stage_a_edge,
  output stage_b_edge,
  output line_poke,
  output line_poke_bit,
  output vs_o,
  output vsync_o,
  output hsync_o,
  output [1:0] r8_il,
  output [4:0] r9_o,
  output [6:0] r7_o,
  output [6:0] r6_o,
  output [6:0] r4_o
);

  // Production clock divider from Amstrad.sv (unchanged)
  reg [2:0] div = 0;
  reg ce_16 = 0;
  always @(posedge clk) begin
    div <= div + 1'b1;
    ce_16 <= (div[1:0] == 0);
  end

  // 64 KiB RAM: payload 0x3900..0xA13A, screen 0xC000, SMC, stack, vectors
  reg [7:0] ram [0:65535];
  integer i;
  initial begin
    for (i = 0; i < 65536; i = i + 1)
      ram[i] = 8'h00;
  end

  // CPU signals
  wire [15:0] A;
  wire [7:0] DO;
  wire M1_n, MREQ_n, IORQ_n, RD_n, WR_n, RFSH_n, HALT_n, BUSAK_n;
  wire [7:0] crtc_dout;

  // Motherboard address decoding for CRTC (exact logic from Amstrad_motherboard.v)
  wire io_rd = ~(RD_n | IORQ_n);
  wire io_wr = ~(WR_n | IORQ_n);
  wire crtc_enable = io_rd | io_wr;
  wire crtc_ncs = A[14];
  wire crtc_rnw = A[9];
  wire crtc_rs  = A[8];

  wire mem_rd = ~(RD_n | MREQ_n);
  wire mem_wr = ~(WR_n | MREQ_n);

  // GA signals (declared before DI: PPI bit0 taps raw CRTC VSYNC)
  wire phi_en_p, phi_en_n, cclk_en_p, cclk_en_n, ready_o;
  wire hs, vs, de;

  // PPI Port-B read (F5xx): bit0 = raw CRTC VSYNC per
  // rtl/Amstrad_motherboard.v (ppi_ipb bit0 = vs_sel). Upper bits are
  // quasi-static on HW (tape/jumpers); fixed to 1 here since only bit0 is
  // polled (RRA) by the 0x91F2/0x9F52 loops. PPI control writes (F7xx) are
  // ignored: Port B is hardwired input mode (see run note).
  wire ppi_b_sel = io_rd & (A[15:8] == 8'hF5);
  wire [7:0] ppi_b_data = vs ? 8'hFF : 8'hFE;

  wire [7:0] DI = mem_rd ? ram[A] : ((io_rd & ~crtc_ncs) ? crtc_dout : (ppi_b_sel ? ppi_b_data : 8'hFF));

  always @(posedge clk) begin
    if (prog_we)
      ram[prog_addr] <= prog_data;
    else if (mem_wr)
      ram[A] <= DO;
  end

  assign peek_data = ram[peek_addr];

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

  // Assignments (same as crtc_t80_top plus B9 trace)
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

  assign line_new_o = crtc.line_new;
  assign r8_write_hit_o = crtc.crtc_type1_engine.r8_write_hit;
  assign tog_stage = crtc.crtc_type1_engine.tog_stage;
  assign tog_enter = crtc.crtc_type1_engine.tog_enter;
  assign ivm = crtc.crtc_type1_engine.ivm;
  assign parity_c9_o = crtc.parity_c9;
  assign parity_frame_o = crtc.parity_frame;
  assign parity_r6_o = crtc.parity_r6;
  assign stage_a_edge = crtc.crtc_type1_engine.stage_a_edge;
  assign stage_b_edge = crtc.crtc_type1_engine.stage_b_edge;
  assign line_poke = crtc.crtc_type1_engine.line_poke;
  assign line_poke_bit = crtc.crtc_type1_engine.line_poke_bit;
  assign vs_o = vs;
  assign vsync_o = crtc.VSYNC;
  assign hsync_o = crtc.HSYNC;
  assign r8_il = crtc.R8_interlace;
  assign r9_o = crtc.R9_v_max_line;
  assign r7_o = crtc.R7_v_sync_pos;
  assign r6_o = crtc.R6_v_displayed;
  assign r4_o = crtc.R4_v_total;

endmodule
