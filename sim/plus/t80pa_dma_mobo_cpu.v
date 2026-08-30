// Simulation-only PHI-aligned bus master for p10_dma_mobo_test_top.v.
//
// The production motherboard instantiates the VHDL T80pa by module name.  This
// Verilog-2001 replacement keeps that exact pin contract while changing only
// the CPU implementation for this fixture.  Bus changes happen on PHI_EN_P;
// a transaction retires on PHI_EN_N only after the motherboard's wait_n is
// high, matching the phase relationship that matters for the DMA/PPI seam.
// The script first programs a short CRTC3 frame and the ASIC-page DMA
// registers, then sweeps real PPI reads and PSG-control writes with explicit
// idle gaps so DMA LOAD can start before, during, and after a CPU I/O cycle.

module T80pa (
	input  wire        reset_n,
	input  wire        clk,
	input  wire        cen_p,
	input  wire        cen_n,
	output reg  [15:0] a,
	output reg  [7:0]  do,
	input  wire [7:0]  di,
	output reg         rd_n,
	output reg         wr_n,
	output reg         iorq_n,
	output reg         mreq_n,
	output reg         m1_n,
	output wire        rfsh_n,
	input  wire        busrq_n,
	input  wire        int_n,
	input  wire        nmi_n,
	input  wire        wait_n,
	input  wire        DIRSet,
	input  wire [211:0] DIR
);
	localparam [1:0] S_GAP = 2'd0, S_CYCLE = 2'd1, S_DONE = 2'd2;
	localparam [5:0] SETUP_LAST = 6'd35;
	localparam [7:0] OP_COUNT = 8'd96;
	localparam [7:0] IDLE_GAP = 8'd11;

	reg [1:0] st;
	reg [5:0] setup_step;
	reg [7:0] op_count;
	reg [7:0] gap_count;
	reg [7:0] stall_count;
	reg [31:0] operation_count;
	reg [31:0] wait_stall_count;
	reg [31:0] cclk_wait_count;
	reg [31:0] phase_error_count;
	reg [25:0] sbus;
	reg [28:0] bus_q;
	reg        bus_change_allowed_d;
	reg        cycle_read;

	// These public marks are consumed through the enclosing motherboard top's
	// diagnostic outputs.  They also make the fake CPU state inspectable in a
	// failing Verilator run without changing the production module ports.
	reg dbg_done          /* verilator public_flat_rd */;
	reg dbg_timing_error  /* verilator public_flat_rd */;
	reg dbg_read_error    /* verilator public_flat_rd */;
	reg [31:0] dbg_wait_stalls /* verilator public_flat_rd */;
	reg [31:0] dbg_cclk_waits  /* verilator public_flat_rd */;
	reg [31:0] dbg_operations  /* verilator public_flat_rd */;

	assign rfsh_n = 1'b1;

	// {memory_cycle, read_cycle, address, data}.  CRTC3 setup follows the
	// ordinary register write decode in Amstrad_motherboard.v.  R8=0 keeps
	// this fixture on the non-interlaced path; the DMA expectation is
	// independent of CRTC3 interlace timing.
	function [25:0] setup_bus(input [5:0] k);
		begin
			case (k)
			6'd0:  setup_bus = {1'b0, 1'b0, 16'h0800, 8'h00}; // R0 index
			6'd1:  setup_bus = {1'b0, 1'b0, 16'h0900, 8'h3F}; // R0 = 63
			6'd2:  setup_bus = {1'b0, 1'b0, 16'h0800, 8'h01}; // R1 index
			6'd3:  setup_bus = {1'b0, 1'b0, 16'h0900, 8'h28}; // R1 = 40
			6'd4:  setup_bus = {1'b0, 1'b0, 16'h0800, 8'h02}; // R2 index
			6'd5:  setup_bus = {1'b0, 1'b0, 16'h0900, 8'h32}; // R2 = 50
			6'd6:  setup_bus = {1'b0, 1'b0, 16'h0800, 8'h03}; // R3 index
			6'd7:  setup_bus = {1'b0, 1'b0, 16'h0900, 8'h22}; // h2, v2
			6'd8:  setup_bus = {1'b0, 1'b0, 16'h0800, 8'h04}; // R4 index
			6'd9:  setup_bus = {1'b0, 1'b0, 16'h0900, 8'h06}; // R4 = 6
			6'd10: setup_bus = {1'b0, 1'b0, 16'h0800, 8'h05}; // R5 index
			6'd11: setup_bus = {1'b0, 1'b0, 16'h0900, 8'h00}; // R5 = 0
			6'd12: setup_bus = {1'b0, 1'b0, 16'h0800, 8'h06}; // R6 index
			6'd13: setup_bus = {1'b0, 1'b0, 16'h0900, 8'h28}; // R6 = 40
			6'd14: setup_bus = {1'b0, 1'b0, 16'h0800, 8'h07}; // R7 index
			6'd15: setup_bus = {1'b0, 1'b0, 16'h0900, 8'h05}; // R7 = 5
			6'd16: setup_bus = {1'b0, 1'b0, 16'h0800, 8'h08}; // R8 index
			6'd17: setup_bus = {1'b0, 1'b0, 16'h0900, 8'h00}; // R8 = 0
			6'd18: setup_bus = {1'b0, 1'b0, 16'h0800, 8'h09}; // R9 index
			6'd19: setup_bus = {1'b0, 1'b0, 16'h0900, 8'h07}; // R9 = 7
			// Program R7 for IOA input, select AY R14, then make Port A input
			// and leave the AY in read mode on keyboard row 8.
			6'd20: setup_bus = {1'b0, 1'b0, 16'hF700, 8'h82};
			6'd21: setup_bus = {1'b0, 1'b0, 16'hF400, 8'h07};
			6'd22: setup_bus = {1'b0, 1'b0, 16'hF600, 8'hC8}; // select R7
			6'd23: setup_bus = {1'b0, 1'b0, 16'hF600, 8'h00};
			6'd24: setup_bus = {1'b0, 1'b0, 16'hF400, 8'h80}; // IOA input
			6'd25: setup_bus = {1'b0, 1'b0, 16'hF600, 8'h88}; // write R7
			6'd26: setup_bus = {1'b0, 1'b0, 16'hF600, 8'h00};
			6'd27: setup_bus = {1'b0, 1'b0, 16'hF400, 8'h0E};
			6'd28: setup_bus = {1'b0, 1'b0, 16'hF600, 8'hC8}; // select R14
			6'd29: setup_bus = {1'b0, 1'b0, 16'hF600, 8'h00};
			6'd30: setup_bus = {1'b0, 1'b0, 16'hF700, 8'h9B}; // Port A input
			6'd31: setup_bus = {1'b0, 1'b0, 16'hF600, 8'h48}; // AY read, row 8
			// ASIC page DMA registers.  vram_din is the LOAD instruction, so
			// SAR is only used to exercise the production decode/feedback.
			6'd32: setup_bus = {1'b1, 1'b0, 16'h6C00, 8'h00}; // SAR0 low
			6'd33: setup_bus = {1'b1, 1'b0, 16'h6C01, 8'h20}; // SAR0 high
			6'd34: setup_bus = {1'b1, 1'b0, 16'h6C02, 8'h00}; // PPR0 = 0
			6'd35: setup_bus = {1'b1, 1'b0, 16'h6C0F, 8'h01}; // DCSR ch0 enable
			default: setup_bus = {1'b0, 1'b0, 16'hFFFF, 8'hFF};
			endcase
		end
	endfunction

	function [25:0] operation_bus(input [7:0] k);
		begin
			// Each Port-A read is preceded by AY read control. Explicit idle
			// slots separate some reads from PSG writes while other pairs stay
			// adjacent, producing base, PPI-only +1, and PSG +2 LOAD classes.
			case (k[2:0])
			3'd0: operation_bus = {1'b0, 1'b0, 16'hF600, 8'h48};
			3'd1: operation_bus = {1'b0, 1'b1, 16'hF400, 8'hFF};
			3'd3: operation_bus = {1'b0, 1'b0, 16'hF600, 8'h88};
			3'd5: operation_bus = {1'b0, 1'b0, 16'hF600, 8'h48};
			3'd6: operation_bus = {1'b0, 1'b1, 16'hF400, 8'hFF};
			default: operation_bus = {1'b0, 1'b0, 16'hFFFF, 8'hFF};
			endcase
		end
	endfunction

	function bus_is_idle(input [7:0] k);
		begin
			bus_is_idle = (k[2:0] == 3'd2) || (k[2:0] == 3'd4) ||
			              (k[2:0] == 3'd7);
		end
	endfunction

	task bus_idle;
		begin
			a <= 16'hFFFF;
			do <= 8'hFF;
			rd_n <= 1'b1;
			wr_n <= 1'b1;
			iorq_n <= 1'b1;
			mreq_n <= 1'b1;
			m1_n <= 1'b1;
		end
	endtask

	always @(posedge clk) begin
		if (!reset_n) begin
			st <= S_GAP;
			setup_step <= 6'd0;
			op_count <= 8'd0;
			gap_count <= 8'd0;
			stall_count <= 8'd0;
			operation_count <= 32'd0;
			wait_stall_count <= 32'd0;
			cclk_wait_count <= 32'd0;
			phase_error_count <= 32'd0;
			dbg_done <= 1'b0;
			dbg_timing_error <= 1'b0;
			dbg_read_error <= 1'b0;
			dbg_wait_stalls <= 32'd0;
			dbg_cclk_waits <= 32'd0;
			dbg_operations <= 32'd0;
			cycle_read <= 1'b0;
			bus_q <= {16'hFFFF, 8'hFF, 1'b1, 1'b1, 1'b1, 1'b1, 1'b1};
			bus_change_allowed_d <= 1'b0;
			bus_idle;
		end
		else begin
			if (({a, do, rd_n, wr_n, iorq_n, mreq_n, m1_n} != bus_q) &&
			    !bus_change_allowed_d) begin
				phase_error_count <= phase_error_count + 32'd1;
				dbg_timing_error <= 1'b1;
			end
			bus_q <= {a, do, rd_n, wr_n, iorq_n, mreq_n, m1_n};
			// Bus outputs change through nonblocking assignments on this edge;
			// validate that change at the following edge against the phase that
			// authorized it now.
			bus_change_allowed_d <= cen_p || cen_n;

			case (st)
			S_GAP: begin
				bus_idle;
				if (gap_count != 8'd0) begin
					if (cen_p) gap_count <= gap_count - 8'd1;
				end
				else if (cen_p) begin
					if (setup_step <= SETUP_LAST) begin
						sbus = setup_bus(setup_step);
						a <= sbus[23:8];
						do <= sbus[7:0];
						rd_n <= sbus[24] ? 1'b0 : 1'b1;
						wr_n <= sbus[24] ? 1'b1 : 1'b0;
						iorq_n <= sbus[25] ? 1'b1 : 1'b0;
						mreq_n <= sbus[25] ? 1'b0 : 1'b1;
						m1_n <= 1'b1;
						cycle_read <= 1'b0;
						stall_count <= 8'd0;
						st <= S_CYCLE;
					end
					else if (op_count < OP_COUNT) begin
						if (bus_is_idle(op_count)) begin
							op_count <= op_count + 8'd1;
							gap_count <= IDLE_GAP;
						end
						else begin
							sbus = operation_bus(op_count);
							a <= sbus[23:8];
							do <= sbus[7:0];
							rd_n <= sbus[24] ? 1'b0 : 1'b1;
							wr_n <= sbus[24] ? 1'b1 : 1'b0;
							iorq_n <= 1'b0;
							mreq_n <= 1'b1;
							m1_n <= 1'b1;
							cycle_read <= sbus[24];
							stall_count <= 8'd0;
							st <= S_CYCLE;
						end
					end
					else begin
						dbg_done <= 1'b1;
						st <= S_DONE;
					end
				end
			end
			S_CYCLE: begin
				if (cen_n && !wait_n) begin
					stall_count <= stall_count + 8'd1;
					wait_stall_count <= wait_stall_count + 32'd1;
					dbg_wait_stalls <= wait_stall_count + 32'd1;
					cclk_wait_count <= cclk_wait_count + 32'd1;
					dbg_cclk_waits <= cclk_wait_count + 32'd1;
				end
				if (cen_n && wait_n) begin
					if (cycle_read && (a == 16'hF400) && (di != 8'hDF))
						dbg_read_error <= 1'b1;
					bus_idle;
					operation_count <= operation_count + 32'd1;
					dbg_operations <= operation_count + 32'd1;
					if (setup_step <= SETUP_LAST)
						setup_step <= setup_step + 6'd1;
					else
						op_count <= op_count + 8'd1;
					gap_count <= (setup_step <= SETUP_LAST) ? 8'd2 : 8'd1;
					st <= S_GAP;
				end
			end
			S_DONE: bus_idle;
			default: st <= S_GAP;
			endcase
		end
	end

	// Keep the unused CPU inputs live in strict Verilator lint runs.
	wire unused = &{1'b0, clk, cen_p, cen_n, di, busrq_n, int_n, nmi_n,
		wait_n, DIRSet, DIR, cycle_read, stall_count, phase_error_count};
endmodule
