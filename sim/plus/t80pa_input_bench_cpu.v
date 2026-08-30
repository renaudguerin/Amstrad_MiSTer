// Fake Z80 bus master for p10_input_test_top.v.
//
// The script uses the real PPI/YM2149/hid path.  PPI control is deliberately
// written as 0x9B before the AY R7/R14 transaction: Plus hardware keeps Port
// C as an output row selector even though the legacy control word would make
// those bits inputs on a classic 6128.

module T80pa_input_bench_cpu (
	input  reset_n,
	input  clk,
	input  cen_p,
	input  cen_n,
	output reg [15:0] a,
	output reg  [7:0] cpu_do,
	input  [7:0] di,
	output reg rd_n,
	output reg wr_n,
	output reg iorq_n,
	output reg mreq_n,
	output reg m1_n,
	input  wait_n,
	output reg done_o,
	output reg [2:0] read_count_o,
	output reg [7:0] read0_o,
	output reg [7:0] read1_o,
	output reg [7:0] read2_o,
	output reg [7:0] read3_o,
	output reg [7:0] read4_o
);
	localparam [2:0] S_GAP = 3'd0, S_CYC = 3'd1, S_DONE = 3'd2;
	localparam [5:0] NSTEPS = 6'd20;
	localparam [7:0] HOLD = 8'd31;
	localparam [7:0] GAP  = 8'd15;

	reg [2:0] st;
	reg [5:0] step;
	reg [7:0] cnt;
	reg [7:0] sbus_data;
	reg [15:0] sbus_addr;
	reg sbus_read;

	// PPI addresses use A14=1 in the production map; A[9:8] still selects
	// the normal PPI A/C/control registers while the CRTC is deselected.
	function [23:0] step_bus(input [5:0] k);
		begin
			case (k)
			6'd0:  step_bus = {16'h4300, 8'h82}; // A output for PSG writes
			6'd1:  step_bus = {16'h4000, 8'h07}; // PPI A = AY R7
			6'd2:  step_bus = {16'h4200, 8'hC8}; // select R7, row 8
			6'd3:  step_bus = {16'h4200, 8'h00}; // neutral PPI/YM bus
			6'd4:  step_bus = {16'h4000, 8'h80}; // R7 bit 6: IOA input
			6'd5:  step_bus = {16'h4200, 8'h88}; // write R7, row 8
			6'd6:  step_bus = {16'h4200, 8'h00}; // commit write, neutral
			6'd7:  step_bus = {16'h4000, 8'h0E}; // PPI A = AY R14
			6'd8:  step_bus = {16'h4200, 8'hC8}; // select R14, row 8
			6'd9:  step_bus = {16'h4200, 8'h00}; // neutral after address select
			6'd10: step_bus = {16'h4300, 8'h9B}; // read mode; Plus C still drives
			6'd11: step_bus = {16'h4200, 8'h08}; // row 8, PSG inactive
			6'd12: step_bus = {16'h4200, 8'h48}; // R14 read, row 8
			6'd13: step_bus = {16'h4000, 8'hFF}; // read row 8, idle
			6'd14: step_bus = {16'h4000, 8'hFF}; // read row 8, PS2 A injected
			6'd15: step_bus = {16'h4200, 8'h09}; // row 9, PSG inactive
			6'd16: step_bus = {16'h4200, 8'h49}; // R14 read, row 9
			6'd17: step_bus = {16'h4000, 8'hFF}; // read row 9, SNAC idle
			6'd18: step_bus = {16'h4000, 8'hFF}; // read row 9, SNAC fire 1
			6'd19: step_bus = {16'h4000, 8'hFF}; // read row 9, USB fire 1
			default: step_bus = {16'hFFFF, 8'hFF};
			endcase
		end
	endfunction

	function step_is_read(input [5:0] k);
		begin
			case (k)
			6'd13, 6'd14, 6'd17, 6'd18, 6'd19: step_is_read = 1'b1;
			default: step_is_read = 1'b0;
			endcase
		end
	endfunction

	task bus_idle;
		begin
			rd_n <= 1'b1;
			wr_n <= 1'b1;
			iorq_n <= 1'b1;
			mreq_n <= 1'b1;
			m1_n <= 1'b1;
			a <= 16'hFFFF;
			cpu_do <= 8'hFF;
		end
	endtask

	always @(posedge clk) begin
		if (!reset_n) begin
			st <= S_GAP;
			step <= 6'd0;
			cnt <= 8'd0;
			sbus_data <= 8'hFF;
			sbus_addr <= 16'hFFFF;
			sbus_read <= 1'b0;
			done_o <= 1'b0;
			read_count_o <= 3'd0;
			read0_o <= 8'hFF;
			read1_o <= 8'hFF;
			read2_o <= 8'hFF;
			read3_o <= 8'hFF;
			read4_o <= 8'hFF;
			bus_idle;
		end else begin
			case (st)
			S_GAP: begin
				bus_idle;
				if (cnt != 8'd0) begin
					cnt <= cnt - 8'd1;
				end else if (!done_o) begin
					{sbus_addr, sbus_data} = step_bus(step);
					sbus_read = step_is_read(step);
					a <= sbus_addr;
					cpu_do <= sbus_data;
					rd_n <= sbus_read ? 1'b0 : 1'b1;
					wr_n <= sbus_read ? 1'b1 : 1'b0;
					iorq_n <= 1'b0;
					mreq_n <= 1'b1;
					m1_n <= 1'b1;
					cnt <= HOLD;
					st <= S_CYC;
				end
			end
			S_CYC: begin
				if (cnt != 8'd0) begin
					cnt <= cnt - 8'd1;
				end else begin
					if (step_is_read(step)) begin
						case (read_count_o)
						3'd0: read0_o <= di;
						3'd1: read1_o <= di;
						3'd2: read2_o <= di;
						3'd3: read3_o <= di;
						3'd4: read4_o <= di;
						default: ;
						endcase
						read_count_o <= read_count_o + 3'd1;
					end
					bus_idle;
					if (step == NSTEPS - 6'd1) begin
						done_o <= 1'b1;
						st <= S_DONE;
					end else begin
						step <= step + 6'd1;
						cnt <= GAP;
						st <= S_GAP;
					end
				end
			end
			S_DONE: bus_idle;
			default: st <= S_GAP;
			endcase
		end
	end

	// cen_p/cen_n/wait_n are part of the fake Z80 contract but this focused
	// script intentionally runs one stable I/O transaction at a time.
	wire unused = &{1'b0, cen_p, cen_n, wait_n};
endmodule
