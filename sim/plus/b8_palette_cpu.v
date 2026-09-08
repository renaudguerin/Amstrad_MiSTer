// Bench-only C++-driven bus source for the B8-3 palette fixture.
//
// Replaces the scripted fake CPU (t80pa_bench_cpu.v) inside the B8-3 bench
// build only: same T80pa port list as the motherboard instantiates, but no
// fixed script. After reset it idles the bus and raises dbg_done; C++ then
// issues real bus cycles through the production motherboard decode by
// writing bench_addr/bench_data/bench_op, raising bench_req, waiting for
// bench_ack, and lowering bench_req again.
//
// bench_op: 0 = I/O write (&7Fxx legacy PENR/INKR), 1 = memory write (ASIC
// page), 2 = memory read (ASIC page, result latched into bench_rdata while
// the bus is still held, then read by C++ after ack).
// Each request holds its cycle long enough for the system-clocked register
// files to sample it (HOLD >= one full sequencer ring). Control registers
// below are bench scaffolding, read from C++ via Verilator --public-flat-rw;
// production RTL is untouched.

module T80pa (
	input  wire         reset_n,
	input  wire         clk,
	input  wire         cen_p,
	input  wire         cen_n,
	output reg  [15:0]  a,
	output reg  [7:0]   do,
	input  wire [7:0]   di,
	output reg          rd_n,
	output reg          wr_n,
	output reg          iorq_n,
	output reg          mreq_n,
	output reg          m1_n,
	output wire         rfsh_n,
	input  wire         busrq_n,
	input  wire         int_n,
	input  wire         nmi_n,
	input  wire         wait_n,
	input  wire         DIRSet,
	input  wire [211:0] DIR
);
	reg [15:0] bench_addr /* verilator public_flat_rw */;
	reg [7:0]  bench_data /* verilator public_flat_rw */;
	reg [1:0]  bench_op /* verilator public_flat_rw */;
	reg        bench_req  /* verilator public_flat_rw */;
	reg        bench_ack  /* verilator public_flat_rw */;
	reg [7:0]  bench_rdata /* verilator public_flat_rw */;
	reg        dbg_done   /* verilator public_flat_rd */;

	localparam [1:0] S_IDLE = 2'd0, S_CYC = 2'd1, S_ACK = 2'd2;
	localparam [1:0] OP_IOWR = 2'd0, OP_MEMWR = 2'd1, OP_MEMRD = 2'd2;
	// HOLD spans several full sequencer rings (S advances on cen_16, one
	// ring is 8 cen = 32 clks): the legacy GA file latches only on its
	// S0&S7 window, so a short hold can miss it depending on phase.
	localparam [7:0] HOLD = 8'd127;

	reg [1:0] st;
	reg [7:0] cnt;
	reg [7:0] settle;

	assign rfsh_n = 1'b1;

	task bus_idle;
		begin
			iorq_n <= 1'b1; mreq_n <= 1'b1; m1_n <= 1'b1;
			rd_n <= 1'b1; wr_n <= 1'b1;
			a <= 16'hFFFF; do <= 8'hFF;
		end
	endtask

	always @(posedge clk) begin
		if (!reset_n) begin
			st <= S_IDLE; cnt <= 8'd0; settle <= 8'd0;
			bench_addr <= 16'hFFFF; bench_data <= 8'hFF;
			bench_op <= 2'd0;
			bench_req <= 1'b0; bench_ack <= 1'b0;
			bench_rdata <= 8'hFF;
			dbg_done <= 1'b0;
			bus_idle;
		end else begin
			case (st)
			S_IDLE: begin
				bus_idle;
				if (!dbg_done) begin
					if (settle != 8'd64) settle <= settle + 8'd1;
					else dbg_done <= 1'b1;
				end
				else if (bench_req && !bench_ack) begin
					a <= bench_addr;
					m1_n <= 1'b1;
					case (bench_op)
					OP_IOWR: begin
						do <= bench_data;
						wr_n <= 1'b0; rd_n <= 1'b1;
						iorq_n <= 1'b0; mreq_n <= 1'b1;
					end
					OP_MEMWR: begin
						do <= bench_data;
						wr_n <= 1'b0; rd_n <= 1'b1;
						iorq_n <= 1'b1; mreq_n <= 1'b0;
					end
					OP_MEMRD: begin
						do <= 8'hFF;
						wr_n <= 1'b1; rd_n <= 1'b0;
						iorq_n <= 1'b1; mreq_n <= 1'b0;
					end
					default: begin
						do <= 8'hFF;
						wr_n <= 1'b1; rd_n <= 1'b1;
						iorq_n <= 1'b1; mreq_n <= 1'b1;
					end
					endcase
					cnt <= HOLD;
					st <= S_CYC;
				end
			end
			S_CYC: begin
				if (cnt != 8'd0) cnt <= cnt - 8'd1;
				else begin
					// Sample the read byte while the bus is still held:
					// asic_regs D_out is combinational under cs+rd.
					if (bench_op == OP_MEMRD) bench_rdata <= di;
					bus_idle;
					bench_ack <= 1'b1;
					st <= S_ACK;
				end
			end
			S_ACK: begin
				bus_idle;
				if (!bench_req) begin
					bench_ack <= 1'b0;
					st <= S_IDLE;
				end
			end
			default: begin bus_idle; st <= S_IDLE; end
			endcase
		end
	end

	// Silence unused-input warnings without touching behaviour.
	wire unused = &{1'b0, cen_p, cen_n, di, busrq_n, int_n, nmi_n, wait_n,
			DIRSet, DIR, 1'b0};
endmodule
