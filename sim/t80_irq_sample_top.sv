// T80 maskable-INT sampling-edge bench top.
//
// Production T80pa (GHDL-translated netlist, same $(T80_NETLIST) the freeze
// bench uses) with direct CEN_p/CEN_n control, so INT/WAIT/NMI/DIRSet stimuli
// can be placed with master-tick precision against pin-observable M1 edges.
// No GA40010, no CRTC, no ASIC: the bench isolates the CPU sampling rule.
//
// Observation is deliberately pin-level (M1_n/MREQ_n/RD_n/WR_n/IORQ_n,
// HALT_n, INSN_START, REG) rather than the core's internal MCycle/TState:
// the anchors must not mirror the implementation's own T-count. Interrupt
// acknowledge is M1_n and IORQ_n low together (the Z80 M1+IORQ cycle).

module t80_irq_sample_top(
  input clk,
  input reset_n,
  input cp,       // CEN_p: one master tick in 16
  input cn,       // CEN_n: opposite phase, one master tick in 16
  input wait_n,
  input irq_n,
  input nmi_n,
  input dirset,
  input [211:0] dir,
  input [7:0] din,
  output [15:0] addr,
  output [7:0] dout,
  output m1n,
  output mreqn,
  output rdn,
  output wrn,
  output iorqn,
  output halt_n,
  output insn,
  output [211:0] regs
);

  T80pa cpu(
    .CLK(clk),
    .RESET_n(reset_n),
    .CEN_p(cp),
    .CEN_n(cn),
    .WAIT_n(wait_n),
    .INT_n(irq_n),
    .NMI_n(nmi_n),
    .BUSRQ_n(1'b1),
    .M1_n(m1n),
    .MREQ_n(mreqn),
    .IORQ_n(iorqn),
    .RD_n(rdn),
    .WR_n(wrn),
    .HALT_n(halt_n),
    .A(addr),
    .DI(din),
    .DO(dout),
    .R800_mode(1'b0),
    .OUT0(1'b0),
    .REG(regs),
    .INSN_START(insn),
    .DIRSet(dirset),
    .DIR(dir)
  );

endmodule
