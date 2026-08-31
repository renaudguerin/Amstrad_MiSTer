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
	output      [3:0] host_rdata,

	// Video port (sprite row-fetch)
	input             video_rd,
	input      [10:0] video_addr,
	output      [7:0] video_rdata
);

	(* ramstyle = "M10K" *) reg [3:0] bank_even [0:2047];
	(* ramstyle = "M10K" *) reg [3:0] bank_odd  [0:2047];
	reg [3:0] host_even_q;
	reg [3:0] host_odd_q;
	reg [3:0] video_even_q;
	reg [3:0] video_odd_q;
	reg       host_bank_q;
	reg       host_valid_q;
	reg       video_valid_q;

	wire host_rd_en = host_rd && !reset;
	wire host_wr_en = host_wr && !reset;
	wire video_rd_en = video_rd && !reset;

	assign host_rdata = host_valid_q ? (host_bank_q ? host_odd_q : host_even_q) : 4'd0;
	assign video_rdata = video_valid_q ? {video_odd_q, video_even_q} : 8'd0;

	integer i;
	initial begin
		for (i = 0; i < 2048; i = i + 1) begin
			bank_even[i] = 4'd0;
			bank_odd[i]  = 4'd0;
		end
		host_even_q = 4'd0;
		host_odd_q = 4'd0;
		video_even_q = 4'd0;
		video_odd_q = 4'd0;
		host_bank_q = 1'b0;
		host_valid_q = 1'b0;
		video_valid_q = 1'b0;
	end

	// Each bank has one host read/write process and one independent video-read
	// process. Keeping the bank outputs separate prevents Quartus 17 from
	// implementing the conditional host read as an asynchronous register-array
	// mirror alongside the inferred M10K.
	always @(posedge clk) begin
		if (host_rd_en && !host_addr[0])
			host_even_q <= bank_even[host_addr[11:1]];
		if (host_wr_en && !host_addr[0])
			bank_even[host_addr[11:1]] <= host_wdata;
	end

	always @(posedge clk) begin
		if (video_rd_en)
			video_even_q <= bank_even[video_addr];
	end

	always @(posedge clk) begin
		if (host_rd_en && host_addr[0])
			host_odd_q <= bank_odd[host_addr[11:1]];
		if (host_wr_en && host_addr[0])
			bank_odd[host_addr[11:1]] <= host_wdata;
	end

	always @(posedge clk) begin
		if (video_rd_en)
			video_odd_q <= bank_odd[video_addr];
	end

	// Reset only the observable pipeline selectors/valid bits. Pixel storage
	// and inferred RAM output registers are not reset and survive ASIC reset.
	always @(posedge clk) begin
		if (reset) begin
			host_bank_q <= 1'b0;
			host_valid_q <= 1'b0;
			video_valid_q <= 1'b0;
		end
		else begin
			if (host_rd) begin
				host_bank_q <= host_addr[0];
				host_valid_q <= 1'b1;
			end
			if (video_rd)
				video_valid_q <= 1'b1;
		end
	end

endmodule
