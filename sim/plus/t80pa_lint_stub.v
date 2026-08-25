// Simulation-only stub for the Verilator motherboard lint pass AND the
// plus_mode=1 motherboard bench (`make -C sim/plus motherboard-lint` /
// `mobo-bench`). T80pa is VHDL (rtl/T80/T80pa.vhd); Verilator has no VHDL
// front end, so both builds need a stand-in. The two targets want different
// behaviour, so the module lives in two files with the same port list and
// each Makefile target picks one:
//
//   - t80pa_lint_stub.v  (this file): ties every output off; used by the
//     static lint pass, which only checks pin wiring.
//
//   - t80pa_bench_cpu.v: a scripted fake bus master used by mobo-bench.
//
// Both keep the FULL port list of the real entity so pin wiring, widths,
// directions, and implicit-net bugs at that boundary are still checked.
// Never synthesise either file; never add them to files.qip.

module T80pa (
	input  wire         reset_n,
	input  wire         clk,
	input  wire         cen_p,
	input  wire         cen_n,
	output wire [15:0]  a,
	output wire [7:0]   do,
	input  wire [7:0]   di,
	output wire         rd_n,
	output wire         wr_n,
	output wire         iorq_n,
	output wire         mreq_n,
	output wire         m1_n,
	output wire         rfsh_n,
	input  wire         busrq_n,
	input  wire         int_n,
	input  wire         nmi_n,
	input  wire         wait_n,
	input  wire         DIRSet,
	input  wire [211:0] DIR
);
	assign a      = 16'h0000;
	assign do     = 8'h00;
	assign rd_n   = 1'b1;
	assign wr_n   = 1'b1;
	assign iorq_n = 1'b1;
	assign mreq_n = 1'b1;
	assign m1_n   = 1'b1;
	assign rfsh_n = 1'b1;
	wire unused = &{1'b0, reset_n, clk, cen_p, cen_n, di, busrq_n,
			int_n, nmi_n, wait_n, DIRSet, DIR, 1'b0};
endmodule
