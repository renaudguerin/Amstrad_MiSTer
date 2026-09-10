//
// TV80pa - Pseudo-asynchronous Z80 top level wrapper for TV80 core
// Matches the exact interface and timing of Sorgelig's T80pa.vhd
//

`timescale 1ns / 1ps

module T80pa #(
  parameter Mode = 0
) (
  input          reset_n,
  input          clk,
  input          cen_p,
  input          cen_n,
  input          wait_n,
  input          int_n,
  input          nmi_n,
  input          busrq_n,
  output         m1_n,
  output reg     mreq_n,
  output reg     iorq_n,
  output reg     rd_n,
  output reg     wr_n,
  output         rfsh_n,
  output         halt_n,
  output         busak_n,
  input          OUT0,
  output [15:0]  a,
  input  [7:0]   di,
  output [7:0]   do,
  input          R800_mode,
  output [211:0] REG,
  input          DIRSet,
  input  [211:0] DIR
);

  wire       intcycle_n;
  wire       iorq;
  wire       noread;
  wire       write;
  wire       busak;
  reg  [7:0] di_reg;
  wire [2:0] mcycle;
  wire [2:0] tstate;
  reg        cen_pol;
  wire       cen;

  assign cen = cen_p & ~cen_pol;
  assign busak_n = busak;

  tv80_core #(
    .Mode(Mode),
    .IOWait(1)
  ) u0 (
    .cen(cen),
    .m1_n(m1_n),
    .iorq(iorq),
    .noread(noread),
    .write(write),
    .rfsh_n(rfsh_n),
    .halt_n(halt_n),
    .wait_n(1'b1),
    .int_n(int_n),
    .nmi_n(nmi_n),
    .reset_n(reset_n),
    .busrq_n(busrq_n),
    .busak_n(busak),
    .clk(clk),
    .A(a),
    .DInst(di),    // valid   at beginning of T3 (sampled by the core at T2)
    .DI(di_reg),   // latched at middle    of T3, before the wrapper drops RD
    .DO(do),
    .REG(REG),
    .MC(mcycle),
    .TS(tstate),
    .out0(OUT0),
    .r800_mode(R800_mode),
    .intcycle_n(intcycle_n),
    .DIRSet(DIRSet),
    .DIR(DIR)
  );

  always @(posedge clk) begin
    if (!reset_n) begin
      wr_n        <= 1'b1;
      rd_n        <= 1'b1;
      iorq_n      <= 1'b1;
      mreq_n      <= 1'b1;
      di_reg      <= 8'h00;
      cen_pol     <= 1'b0;
    end else if (cen_p && !cen_pol) begin
      cen_pol <= 1'b1;
      if (mcycle == 3'b001) begin
        if (tstate == 3'b010) begin
          // The opcode fetch ends here, but an interrupt acknowledge holds
          // IORQ across the whole M1.
          iorq_n <= intcycle_n;
          mreq_n <= 1'b1;
          rd_n   <= 1'b1;
        end
      end else begin
        if (tstate == 3'b001 && iorq) begin
          wr_n   <= ~write;
          rd_n   <= write;
          iorq_n <= 1'b0;
        end
      end
    end else if (cen_n && cen_pol) begin
      if (tstate == 3'b010) begin
        cen_pol <= ~wait_n;
      end else begin
        cen_pol <= 1'b0;
      end

      if (tstate == 3'b011 && busak) begin
        di_reg <= di;
      end

      // T80pa.vhd stages IORQ_n through a 2-bit IntCycle shift register that
      // only advances on M1/T1. That relies on T80's interrupt M1 being
      // stretched to 5 T-states plus auto-wait; this core runs a plain
      // 4-T-state M1, so the shift never reaches the output and IORQ_n would
      // stay high through the acknowledge. Drive it directly from IntCycle
      // instead: IORQ low for the whole acknowledge M1, and no refresh MREQ
      // overlapping it.
      if (mcycle == 3'b001) begin
        if (tstate == 3'b001) begin
          rd_n   <= ~intcycle_n;
          mreq_n <= ~intcycle_n;
          iorq_n <=  intcycle_n;
        end
        if (tstate == 3'b011) begin
          rd_n   <= 1'b1;
          mreq_n <= ~intcycle_n;
        end
        if (tstate == 3'b100) begin
          mreq_n <= 1'b1;
          iorq_n <= 1'b1;
        end
      end else begin
        if (!noread && !iorq) begin
          if (tstate == 3'b001) begin
            rd_n   <= write;
            mreq_n <= 1'b0;
          end
        end
        if (tstate == 3'b010) begin
          wr_n <= ~write;
        end
        if (tstate == 3'b011) begin
          wr_n   <= 1'b1;
          rd_n   <= 1'b1;
          iorq_n <= 1'b1;
          mreq_n <= 1'b1;
        end
      end
    end
  end

endmodule
