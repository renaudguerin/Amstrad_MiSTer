//============================================================================
//  Amstrad Plus ASIC sprite pixel RAM (P10j).
//
//  Backs the 4096×4-bit sprite pixel storage (&4000-&4FFF) with two logical
//  2048×4-bit banks (even and odd nibbles) targeting Cyclone V M10K block RAM.
//
//  Dual-port architecture:
//    - Port A (Host): 12-bit address (host_addr[11:0]), 4-bit data.
//      Accepts CPU and SNA writes; returns registered read data exactly one
//      64 MHz edge after host_rd. host_addr[0] selects the odd bank (1) or
//      even bank (0); host_addr[11:1] addresses the 2048 locations.
//    - Port B (Video): 11-bit address (video_addr[10:0]), 8-bit packed data.
//      Returns {odd bank nibble, even bank nibble} in one registered read.
//
//  Mixed-port read-during-write semantics:
//    Read-first / old-data on write collisions.
//    Untouched locations initialize to zero (defined-zero FPGA model assumption).
//============================================================================

module plus_sprite_ram
(
	input             clk,
	input             reset,

	// Host port (CPU and SNA)
	input             host_rd,
	input             host_wr,
	input      [11:0] host_addr,
	input       [3:0] host_wdata,
	output reg  [3:0] host_rdata,

	// Video port (sprite row-fetch)
	input             video_rd,
	input      [10:0] video_addr,
	output reg  [7:0] video_rdata
);

	(* ramstyle = "M10K" *) reg [3:0] bank_even [0:2047];
	(* ramstyle = "M10K" *) reg [3:0] bank_odd  [0:2047];

	integer i;
	initial begin
		for (i = 0; i < 2048; i = i + 1) begin
			bank_even[i] = 4'd0;
			bank_odd[i]  = 4'd0;
		end
		host_rdata  = 4'd0;
		video_rdata = 8'd0;
	end

	always @(posedge clk) begin
		if (reset) begin
			// Reset only the observable pipeline registers. Pixel storage is
			// not part of the ASIC reset contract and must survive reset.
			host_rdata  <= 4'd0;
			video_rdata <= 8'd0;
		end
		else begin
			if (video_rd)
				video_rdata <= {bank_odd[video_addr], bank_even[video_addr]};

			if (host_rd)
				host_rdata <= host_addr[0] ? bank_odd[host_addr[11:1]] : bank_even[host_addr[11:1]];

			if (host_wr) begin
				if (host_addr[0])
					bank_odd[host_addr[11:1]] <= host_wdata;
				else
					bank_even[host_addr[11:1]] <= host_wdata;
			end
		end
	end

endmodule
