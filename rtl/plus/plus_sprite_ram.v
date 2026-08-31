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

	wire [3:0] host_even_q;
	wire [3:0] host_odd_q;
	wire [3:0] video_even_q;
	wire [3:0] video_odd_q;
	reg       host_bank_q;
	reg       host_valid_q;
	reg       video_valid_q;

	wire host_rd_en = host_rd && !reset;
	wire host_wr_en = host_wr && !reset;
	wire video_rd_en = video_rd && !reset;

	assign host_rdata = host_valid_q ? (host_bank_q ? host_odd_q : host_even_q) : 4'd0;
	assign video_rdata = video_valid_q ? {video_odd_q, video_even_q} : 8'd0;

	initial begin
		host_bank_q = 1'b0;
		host_valid_q = 1'b0;
		video_valid_q = 1'b0;
	end

	// Quartus 17 duplicates an inferred 2-read/1-write array into two simple
	// dual-port memories. One explicit bidirectional dual-port primitive per
	// nibble bank guarantees that the host and video reads share one M10K.
	plus_sprite_ram_bank even_bank
	(
		.clk(clk),
		.host_rd(host_rd_en && !host_addr[0]),
		.host_wr(host_wr_en && !host_addr[0]),
		.host_addr(host_addr[11:1]),
		.host_wdata(host_wdata),
		.host_q(host_even_q),
		.video_rd(video_rd_en),
		.video_addr(video_addr),
		.video_q(video_even_q)
	);

	plus_sprite_ram_bank odd_bank
	(
		.clk(clk),
		.host_rd(host_rd_en && host_addr[0]),
		.host_wr(host_wr_en && host_addr[0]),
		.host_addr(host_addr[11:1]),
		.host_wdata(host_wdata),
		.host_q(host_odd_q),
		.video_rd(video_rd_en),
		.video_addr(video_addr),
		.video_q(video_odd_q)
	);

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

/* verilator lint_off DECLFILENAME */
module plus_sprite_ram_bank
(
	input             clk,
	input             host_rd,
	input             host_wr,
	input      [10:0] host_addr,
	input       [3:0] host_wdata,
	output      [3:0] host_q,
	input             video_rd,
	input      [10:0] video_addr,
	output      [3:0] video_q
);

`ifndef PLUS_SPRITE_RAM_ALTSYNCRAM
	reg [3:0] mem [0:2047];
	reg [3:0] host_q_sim;
	reg [3:0] video_q_sim;

	assign host_q = host_q_sim;
	assign video_q = video_q_sim;

	integer i;
	initial begin
		for (i = 0; i < 2048; i = i + 1)
			mem[i] = 4'd0;
		host_q_sim = 4'd0;
		video_q_sim = 4'd0;
	end

	always @(posedge clk) begin
		if (host_rd)
			host_q_sim <= mem[host_addr];
		if (host_wr)
			mem[host_addr] <= host_wdata;
	end

	always @(posedge clk) begin
		if (video_rd)
			video_q_sim <= mem[video_addr];
	end
`else
	altsyncram ram
	(
		.clock0(clk),
		.clock1(clk),
		.address_a(host_addr),
		.address_b(video_addr),
		.data_a(host_wdata),
		.data_b(4'd0),
		.wren_a(host_wr),
		.wren_b(1'b0),
		.rden_a(host_rd),
		.rden_b(video_rd),
		.q_a(host_q),
		.q_b(video_q),
		.aclr0(1'b0),
		.aclr1(1'b0),
		.addressstall_a(1'b0),
		.addressstall_b(1'b0),
		.byteena_a(1'b1),
		.byteena_b(1'b1),
		.clocken0(1'b1),
		.clocken1(1'b1),
		.clocken2(1'b1),
		.clocken3(1'b1),
		.eccstatus()
	);

	defparam
		ram.numwords_a = 2048,
		ram.widthad_a = 11,
		ram.width_a = 4,
		ram.numwords_b = 2048,
		ram.widthad_b = 11,
		ram.width_b = 4,
		ram.address_reg_b = "CLOCK1",
		ram.clock_enable_input_a = "BYPASS",
		ram.clock_enable_input_b = "BYPASS",
		ram.clock_enable_output_a = "BYPASS",
		ram.clock_enable_output_b = "BYPASS",
		ram.indata_reg_b = "CLOCK1",
		ram.intended_device_family = "Cyclone V",
		ram.lpm_type = "altsyncram",
		ram.operation_mode = "BIDIR_DUAL_PORT",
		ram.outdata_aclr_a = "NONE",
		ram.outdata_aclr_b = "NONE",
		ram.outdata_reg_a = "UNREGISTERED",
		ram.outdata_reg_b = "UNREGISTERED",
		ram.power_up_uninitialized = "FALSE",
		ram.ram_block_type = "M10K",
		ram.read_during_write_mode_mixed_ports = "OLD_DATA",
		ram.read_during_write_mode_port_a = "OLD_DATA",
		ram.read_during_write_mode_port_b = "OLD_DATA",
		ram.width_byteena_a = 1,
		ram.width_byteena_b = 1,
		ram.wrcontrol_wraddress_reg_b = "CLOCK1";
`endif

endmodule
/* verilator lint_on DECLFILENAME */
