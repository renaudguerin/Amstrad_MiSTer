// B18 slice 1: production-T80 snapshot freeze-point harness.
//
// Production T80pa (GHDL-translated netlist) clocked by the real ga40010
// divider, enables and READY, with the motherboard WAIT equation. Adds what
// the B8-1 harness (crtc_t80_top.sv) ties off: REG and INSN_START observation,
// controllable INT_n/NMI_n, a clock-enable hold (the proposed save freeze,
// gating CEN_p/CEN_n exactly as sna_hold does in Amstrad_motherboard.v) and an
// external WAIT (the hardware reference the hold is compared against).
// No CRTC: freeze behaviour depends on the CPU, the enables and WAIT only.
`timescale 1ns/1ps

module t80_freeze_top(
  input clk,
  input reset,
  input cpu_reset,

  input int_n,
  input nmi_n,
  input cpu_hold,   // gate both T80pa clock enables (save freeze)
  input ext_wait,   // assert Z80 WAIT (hardware reference)

  // RAM programming interface (256-byte RAM, A[7:0] aliased)
  input prog_we,
  input [7:0] prog_addr,
  input [7:0] prog_data,
  input [7:0] peek_addr,
  output [7:0] peek_data,

  output [211:0] reg_state,
  output insn_start,

  output phi_p,
  output phi_n,
  output ga_ready,

  output [15:0] t80_a,
  output [7:0] t80_dout,
  output t80_mreq_n,
  output t80_iorq_n,
  output t80_rd_n,
  output t80_wr_n,
  output t80_m1_n,
  output t80_rfsh_n,
  output t80_halt_n,

  // B18 slice 4a snapshot capture controller interface
  input save_req,
  input release_req,
  input admit,
  input rampage_ok,
  input use_controller,
  output ctrl_hold,
  output captured,
  output refused,
  output cancelled,
  output [2047:0] header
);

  // Production clock divider from Amstrad.sv
  reg [2:0] div = 0;
  reg ce_16 = 0;
  always @(posedge clk) begin
    div <= div + 1'b1;
    ce_16 <= (div[1:0] == 0);
  end

  reg [7:0] ram [0:255];
  integer i;
  initial begin
    for (i = 0; i < 256; i = i + 1)
      ram[i] = 8'h00;
  end

  wire [15:0] A;
  wire [7:0] DO;
  wire M1_n, MREQ_n, IORQ_n, RD_n, WR_n, RFSH_n, HALT_n, BUSAK_n;

  wire mem_rd = ~(RD_n | MREQ_n);
  wire mem_wr = ~(WR_n | MREQ_n);
  wire int_ack = ~(M1_n | IORQ_n);

  // IM 1/IM 0 acknowledge reads FF from the idle CPC bus.
  wire [7:0] DI = mem_rd ? ram[A[7:0]] : 8'hFF;

  always @(posedge clk) begin
    if (prog_we)
      ram[prog_addr] <= prog_data;
    else if (mem_wr)
      ram[A[7:0]] <= DO;
  end
  assign peek_data = ram[peek_addr];

  wire phi_en_p, phi_en_n, ready_o;

  // Motherboard wait equation (Amstrad_motherboard.v), plus the reference WAIT.
  wire cpu_wait_n = (ready_o | (IORQ_n & MREQ_n)) & ~ext_wait;

  // Constant recognisable hw_hdr pattern: byte i = i.
  wire [8*135-1:0] hw_hdr;
  genvar gi;
  generate
    for (gi = 0; gi < 135; gi = gi + 1) begin : gen_hw_hdr
      assign hw_hdr[gi*8 +: 8] = gi[7:0];
    end
  endgenerate

  wire ctrl_hold_o;
  wire captured_o;
  wire refused_o;
  wire cancelled_o;
  wire busy_o;
  wire [2047:0] header_o;

  sna_save_capture capture(
    .clk(clk),
    .reset(reset),
    .save_req(save_req),
    .admit(admit),
    .rampage_ok(rampage_ok),
    .release_req(release_req),
    .insn_start(insn_start),
    .halt_n(HALT_n),
    .cpu_reg(reg_state),
    .hw_hdr(hw_hdr),
    .hold(ctrl_hold_o),
    .captured(captured_o),
    .refused(refused_o),
    .cancelled(cancelled_o),
    .busy(busy_o),
    .header(header_o)
  );

  assign ctrl_hold = ctrl_hold_o;
  assign captured = captured_o;
  assign refused = refused_o;
  assign cancelled = cancelled_o;
  assign header = header_o;

  wire effective_hold = cpu_hold | (use_controller & ctrl_hold_o);

  T80pa cpu(
    .RESET_n(~(reset | cpu_reset)),
    .CLK(clk),
    .CEN_p(phi_en_p & ~effective_hold),
    .CEN_n(phi_en_n & ~effective_hold),
    .WAIT_n(cpu_wait_n),
    .INT_n(int_n),
    .NMI_n(nmi_n),
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
    .HSYNC_I(1'b0),
    .VSYNC_I(1'b0),
    .DISPEN(1'b0),
    .CCLK_EN_P(),
    .CCLK_EN_N(),
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
    .SNA_CONFIG(8'd0),
    .SNAP_INKSEL(),
    .SNAP_BORDER(),
    .SNAP_INKR(),
    .SNAP_HROMEN(),
    .SNAP_LROMEN(),
    .SNAP_MODE(),
    .SNAP_INTCNT(),
    .SNAP_HCNT()
  );

  assign phi_p = phi_en_p;
  assign phi_n = phi_en_n;
  assign ga_ready = ready_o;

  assign t80_a = A;
  assign t80_dout = DO;
  assign t80_mreq_n = MREQ_n;
  assign t80_iorq_n = IORQ_n;
  assign t80_rd_n = RD_n;
  assign t80_wr_n = WR_n;
  assign t80_m1_n = M1_n;
  assign t80_rfsh_n = RFSH_n;
  assign t80_halt_n = HALT_n;

  // Unused here; kept named so lint shows intent rather than a dangling net.
  wire unused_ok = int_ack | BUSAK_n | busy_o;

endmodule
