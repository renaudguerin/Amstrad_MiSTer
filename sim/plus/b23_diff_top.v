// B23 CPU differential top: production GHDL T80pa side-by-side with fixture TV80
`timescale 1ns / 1ps

module b23_diff_top (
    input             clk,
    input             reset_n,
    input             cen_p,
    input             cen_n,
    input             wait_n,
    input             int_n,
    input             nmi_n,
    input             busrq_n,
    input             out0,
    input             r800_mode,

    // T80pa (production GHDL netlist)
    input      [7:0]  t80_di,
    output            t80_m1_n,
    output            t80_mreq_n,
    output            t80_iorq_n,
    output            t80_rd_n,
    output            t80_wr_n,
    output            t80_rfsh_n,
    output            t80_halt_n,
    output            t80_busak_n,
    output     [15:0] t80_a,
    output     [7:0]  t80_do,

    // TV80pa (fixture TV80 wrapper with IOWait=1)
    input      [7:0]  tv80_di,
    output            tv80_m1_n,
    output            tv80_mreq_n,
    output            tv80_iorq_n,
    output            tv80_rd_n,
    output            tv80_wr_n,
    output            tv80_rfsh_n,
    output            tv80_halt_n,
    output            tv80_busak_n,
    output     [15:0] tv80_a,
    output     [7:0]  tv80_do,

    // TV80pa (mutation probe with IOWait=0)
    input      [7:0]  tv80_noiowait_di,
    output            tv80_noiowait_m1_n,
    output            tv80_noiowait_mreq_n,
    output            tv80_noiowait_iorq_n,
    output            tv80_noiowait_rd_n,
    output            tv80_noiowait_wr_n,
    output            tv80_noiowait_rfsh_n,
    output            tv80_noiowait_halt_n,
    output            tv80_noiowait_busak_n,
    output     [15:0] tv80_noiowait_a,
    output     [7:0]  tv80_noiowait_do
);

    wire [211:0] t80_reg;
    wire         t80_insn_start;
    wire [211:0] tv80_reg;
    wire         tv80_insn_start;
    wire [211:0] tv80_noiowait_reg;
    wire         tv80_noiowait_insn_start;

    T80pa u_t80 (
        .RESET_n(reset_n),
        .CLK(clk),
        .CEN_p(cen_p),
        .CEN_n(cen_n),
        .WAIT_n(wait_n),
        .INT_n(int_n),
        .NMI_n(nmi_n),
        .BUSRQ_n(busrq_n),
        .M1_n(t80_m1_n),
        .MREQ_n(t80_mreq_n),
        .IORQ_n(t80_iorq_n),
        .RD_n(t80_rd_n),
        .WR_n(t80_wr_n),
        .RFSH_n(t80_rfsh_n),
        .HALT_n(t80_halt_n),
        .BUSAK_n(t80_busak_n),
        .OUT0(out0),
        .A(t80_a),
        .DI(t80_di),
        .DO(t80_do),
        .R800_mode(r800_mode),
        .REG(t80_reg),
        .INSN_START(t80_insn_start),
        .DIRSet(1'b0),
        .DIR(212'b0)
    );

    TV80pa #(
        .Mode(0),
        .IOWait(1)
    ) u_tv80 (
        .reset_n(reset_n),
        .clk(clk),
        .cen_p(cen_p),
        .cen_n(cen_n),
        .wait_n(wait_n),
        .int_n(int_n),
        .nmi_n(nmi_n),
        .busrq_n(busrq_n),
        .m1_n(tv80_m1_n),
        .mreq_n(tv80_mreq_n),
        .iorq_n(tv80_iorq_n),
        .rd_n(tv80_rd_n),
        .wr_n(tv80_wr_n),
        .rfsh_n(tv80_rfsh_n),
        .halt_n(tv80_halt_n),
        .busak_n(tv80_busak_n),
        .OUT0(out0),
        .a(tv80_a),
        .di(tv80_di),
        .do(tv80_do),
        .R800_mode(r800_mode),
        .REG(tv80_reg),
        .INSN_START(tv80_insn_start),
        .DIRSet(1'b0),
        .DIR(212'b0)
    );

    TV80pa #(
        .Mode(0),
        .IOWait(0)
    ) u_tv80_noiowait (
        .reset_n(reset_n),
        .clk(clk),
        .cen_p(cen_p),
        .cen_n(cen_n),
        .wait_n(wait_n),
        .int_n(int_n),
        .nmi_n(nmi_n),
        .busrq_n(busrq_n),
        .m1_n(tv80_noiowait_m1_n),
        .mreq_n(tv80_noiowait_mreq_n),
        .iorq_n(tv80_noiowait_iorq_n),
        .rd_n(tv80_noiowait_rd_n),
        .wr_n(tv80_noiowait_wr_n),
        .rfsh_n(tv80_noiowait_rfsh_n),
        .halt_n(tv80_noiowait_halt_n),
        .busak_n(tv80_noiowait_busak_n),
        .OUT0(out0),
        .a(tv80_noiowait_a),
        .di(tv80_noiowait_di),
        .do(tv80_noiowait_do),
        .R800_mode(r800_mode),
        .REG(tv80_noiowait_reg),
        .INSN_START(tv80_noiowait_insn_start),
        .DIRSet(1'b0),
        .DIR(212'b0)
    );

endmodule
