// Bench-only fake CPU for the plus_mode=1 motherboard-level bench
// (`make -C sim/plus mobo-bench`). Amstrad_motherboard instantiates its bus
// master by name, so this module replaces the VHDL T80pa inside that bench
// build only. The lint pass uses the tie-off twin t80pa_lint_stub.v instead;
// production never sees either.
//
// Behaviour: after reset release it plays a fixed script of I/O write
// cycles that programs a sane CRTC type-3 frame through the motherboard's
// bus decode (index/value addresses chosen with A[14]=0 to select the CRTC
// side, A[9]=0 for write, A[8] picking index vs value, and A[11]=1 so the
// PPI stays deselected), then issues legacy GA-register writes to the ASIC
// path (&7Fxx). It then idles, counts interrupts seen on its int_n input,
// acknowledges the first one Z80-style (M1+IORQ low), and keeps counting.
// The dbg_* exports are unconnected at the instantiation site and are read
// from C++ via Verilator --public-flat-rw.

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
	// Bench-only observation state, read from C++ via --public-flat-rw.
	// Declared as public internal signals rather than ports: unconnected
	// output ports get dead-code-eliminated by Verilator even under
	// --public-flat-rw, but public-marked registers are retained.
	reg [5:0]  dbg_step       /* verilator public_flat_rd */;
	reg        dbg_done       /* verilator public_flat_rd */;
	reg        dbg_ack_done   /* verilator public_flat_rd */;
	reg        dbg_int_level  /* verilator public_flat_rd */;
	reg [31:0] dbg_int_fires  /* verilator public_flat_rd */;
	localparam [2:0] S_GAP = 3'd0, S_CYC = 3'd1, S_PRIME = 3'd2,
	                 S_WAIT = 3'd3, S_ACK = 3'd4;

	localparam [5:0] NSTEPS = 6'd34;
	localparam [7:0] HOLD = 8'd47; // >= one full sequencer ring (32 clks)
	localparam [7:0] GAP  = 8'd15;
	localparam [7:0] ACKS = 8'd31;

	reg [2:0]  st;
	reg [5:0]  step;
	reg [7:0]  cnt;
	reg        int_d;
	reg        primed;
	reg        cyc_mem; // current scripted cycle is a memory cycle
	reg [24:0] sbus;    // step_bus scratch (blocking-assigned, read same pass)

	assign rfsh_n = 1'b1;

	// {mem_type, addr[15:0], data[7:0]}: mem_type 0 = I/O write (GA/CRTC
	// decode), 1 = memory write (ASIC page). The distinction matters at
	// the pins — IORQ vs MREQ — exactly like a real Z80.
	function [24:0] step_bus(input [5:0] k);
		begin
			case (k)
			6'd0:  step_bus = {16'h0800, 8'h00}; // CRTC index phase
			6'd1:  step_bus = {16'h0900, 8'h3F}; // R0 = 63
			6'd2:  step_bus = {16'h0800, 8'h01};
			6'd3:  step_bus = {16'h0900, 8'h28}; // R1 = 40
			6'd4:  step_bus = {16'h0800, 8'h02};
			6'd5:  step_bus = {16'h0900, 8'h32}; // R2 = 50
			6'd6:  step_bus = {16'h0800, 8'h03};
			6'd7:  step_bus = {16'h0900, 8'h22}; // R3 = h2,v2
			6'd8:  step_bus = {16'h0800, 8'h04};
			6'd9:  step_bus = {16'h0900, 8'h06}; // R4 = 6
			6'd10: step_bus = {1'b0, 16'h0800, 8'h05};
			6'd11: step_bus = {1'b0, 16'h0900, 8'h00}; // R5 = 0
			6'd12: step_bus = {1'b0, 16'h0800, 8'h06};
			6'd13: step_bus = {1'b0, 16'h0900, 8'h28}; // R6 = 40
			6'd14: step_bus = {1'b0, 16'h0800, 8'h07};
			6'd15: step_bus = {1'b0, 16'h0900, 8'h05}; // R7 = 5
			6'd16: step_bus = {1'b0, 16'h0800, 8'h09};
			6'd17: step_bus = {1'b0, 16'h0900, 8'h07}; // R9 = 7
			6'd18: step_bus = {1'b0, 16'h7F00, 8'h83}; // GA RMR: mode 3
			6'd19: step_bus = {1'b0, 16'h7F00, 8'h82}; // GA RMR: mode 2
			6'd20: step_bus = {1'b0, 16'h7F00, 8'h05}; // ink select 5
			6'd21: step_bus = {1'b0, 16'h7F00, 8'h55}; // INKR[5] = 0x15
			6'd22: step_bus = {1'b0, 16'h7F00, 8'h10}; // ink select 0x10 -> border
			6'd23: step_bus = {1'b0, 16'h7F00, 8'h44}; // border = 0x04
			// P2 m5: with the ASIC page forced on by the bench, memory
			// cycles at &4000-&5FFF hit the sprite RAM (offsets 0-255 =
			// sprite 0) and the palette block.
			6'd24: step_bus = {1'b1, 16'h4000, 8'hA5}; // sprite 0 pixel byte 0
			6'd25: step_bus = {1'b1, 16'h4001, 8'h5C};
			6'd26: step_bus = {1'b1, 16'h4100, 8'h3E}; // sprite 1 first pixel
			6'd27: step_bus = {1'b1, 16'h5000, 8'hFF}; // unused region: ignored
			6'd28: step_bus = {1'b1, 16'h6400, 8'h0F}; // pen 0 low byte R=0 B=F
			6'd29: step_bus = {1'b1, 16'h6401, 8'h03}; // pen 0 high byte G=3
			6'd30: step_bus = {1'b1, 16'h6420, 8'h21}; // border low byte
			6'd31: step_bus = {1'b1, 16'h6000, 8'h66}; // sprite 0 X lo
			6'd32: step_bus = {1'b1, 16'h6805, 8'hDA}; // &6805 IVR = 0xDA
			6'd33: step_bus = {1'b1, 16'h6805, 8'hDA}; // (hold; single write)
			default: step_bus = {1'b0, 16'h0000, 8'hFF};
			endcase
		end
	endfunction

	// Nonblocking throughout: newer Verilator makes mixed blocking/
	// nonblocking assignment to the same variable an error, and this task
	// runs inside the clocked block below.
	task bus_idle;
		begin
			iorq_n <= 1'b1; mreq_n <= 1'b1; m1_n <= 1'b1;
			rd_n <= 1'b1; wr_n <= 1'b1;
			a <= 16'hFFFF; do <= 8'hFF;
		end
	endtask

	always @(posedge clk) begin
		if (!reset_n) begin
			st <= S_GAP; step <= 6'd0; cnt <= 8'd0;
			int_d <= 1'b1; primed <= 1'b0;
			dbg_step <= 6'd0; dbg_done <= 1'b0; dbg_ack_done <= 1'b0;
			dbg_int_level <= 1'b1; dbg_int_fires <= 32'd0;
			bus_idle;
		end else begin
			dbg_int_level <= int_n;
			case (st)
			S_GAP: begin
				bus_idle;
				if (cnt != 8'd0) begin
					cnt <= cnt - 8'd1;
				end else if (!dbg_done) begin
					// Capture the step first: function-result bit-select
					// is SystemVerilog, and cyc_mem would still hold last
					// cycle's value here anyway (NBA semantics).
					sbus   = step_bus(step);
					{cyc_mem, a, do} <= sbus;
					dbg_step         <= step;
					wr_n   <= 1'b0;
					rd_n   <= 1'b1;
					m1_n   <= 1'b1;
					iorq_n <= sbus[24] ? 1'b1 : 1'b0;
					mreq_n <= sbus[24] ? 1'b0 : 1'b1;
					cnt    <= HOLD;
					st     <= S_CYC;
				end else if (!primed) begin
					// Script finished: clear any power-on-low interrupt
					// level once, so subsequent fires are genuine.
					m1_n <= 1'b0; iorq_n <= 1'b0;
					rd_n <= 1'b1; wr_n <= 1'b1; mreq_n <= 1'b1;
					cnt <= ACKS;
					st <= S_PRIME;
				end else begin
					st <= S_WAIT;
				end
			end
			S_CYC: begin
				if (cnt != 8'd0) begin
					cnt <= cnt - 8'd1;
				end else begin
					bus_idle;
					if (step == NSTEPS - 6'd1) dbg_done <= 1'b1;
					else step <= step + 6'd1;
					cnt <= GAP;
					st <= S_GAP;
				end
			end
			S_PRIME: begin
				if (cnt != 8'd0) begin
					cnt <= cnt - 8'd1;
				end else begin
					bus_idle;
					primed <= 1'b1;
					dbg_ack_done <= 1'b1;
					int_d <= 1'b1;
					st <= S_GAP;
				end
			end
			S_WAIT: begin
				if (int_d & ~int_n) begin
					dbg_int_fires <= dbg_int_fires + 32'd1;
					m1_n <= 1'b0; iorq_n <= 1'b0;
					rd_n <= 1'b1; wr_n <= 1'b1; mreq_n <= 1'b1;
					a <= 16'hFFFF; do <= 8'hFF;
					cnt <= ACKS;
					st <= S_ACK;
				end
				int_d <= int_n;
			end
			S_ACK: begin
				if (cnt != 8'd0) begin
					cnt <= cnt - 8'd1;
				end else begin
					bus_idle;
					int_d <= 1'b1;
					st <= S_WAIT;
				end
			end
			default: st <= S_GAP;
			endcase
		end
	end

	// Silence unused-input warnings without touching behaviour.
	wire unused = &{1'b0, cen_p, cen_n, di, busrq_n, nmi_n, wait_n,
			DIRSet, DIR, 1'b0};
endmodule
