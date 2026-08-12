//
// sdram.v
//
// sdram controller implementation
// Copyright (c) 2018 Sorgelig
//
// Based on sdram module by Till Harbaum
// 
// This source file is free software: you can redistribute it and/or modify 
// it under the terms of the GNU General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or 
// (at your option) any later version. 
// 
// This source file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License 
// along with this program.  If not, see <http://www.gnu.org/licenses/>. 
//

module sdram
(

	// interface to the MT48LC16M16 chip
	inout      [15:0] SDRAM_DQ,   // 16 bit bidirectional data bus
	output reg [12:0] SDRAM_A,    // 13 bit multiplexed address bus
	output            SDRAM_DQML, // byte mask
	output            SDRAM_DQMH, // byte mask
	output reg  [1:0] SDRAM_BA,   // two banks
	output            SDRAM_nCS,  // a single chip select
	output reg        SDRAM_nWE,  // write enable
	output reg        SDRAM_nRAS, // row address select
	output reg        SDRAM_nCAS, // columns address select
	output            SDRAM_CLK,
	output            SDRAM_CKE,

	// cpu/chipset interface
	input             init,			// init signal after FPGA config to initialize RAM
	input             clk,			// sdram is accessed at up to 128MHz
	input             clkref,		// reference clock to sync to
	
	input       [1:0] bank,
	input       [7:0] din,			// data input from chipset/cpu
	output      [7:0] dout,			// data output to chipset/cpu
	input      [22:0] addr,       // 25 bit byte address
	input             oe,         // cpu/chipset requests read
	input             we,         // cpu/chipset requests write

	// Held cartridge request interface. The requester keeps every input
	// stable until cart_ack. A request may remain asserted for the next
	// transaction, provided its fields change only after the acknowledge.
	input             cart_req,
	input             cart_wr,
	input       [1:0] cart_bank,
	input      [22:0] cart_addr,
	input       [7:0] cart_din,
	output reg  [7:0] cart_dout,
	output reg        cart_ack,

	output reg [15:0] vram_dout,
	input      [22:0] vram_addr,
	input       [1:0] vram_bank,

	input      [22:0] tape_addr,
	input       [7:0] tape_din,
	output reg  [7:0] tape_dout,

	input             tape_wr,
	output reg        tape_wr_ack,

	input             tape_rd,
	output reg        tape_rd_ack
);

assign SDRAM_CKE = 1;
assign SDRAM_nCS = 0;
assign {SDRAM_DQMH, SDRAM_DQML} = SDRAM_A[12:11];

assign dout = (oe | we) ? ram_dout : 8'hFF;
// no burst configured
localparam RASCAS_DELAY   = 3'd2;   // tRCD=20ns -> 2 cycles@64MHz
localparam BURST_LENGTH   = 3'b000; // 000=1, 001=2, 010=4, 011=8
localparam ACCESS_TYPE    = 1'b0;   // 0=sequential, 1=interleaved
localparam CAS_LATENCY    = 3'd2;   // 2/3 allowed
localparam OP_MODE        = 2'b00;  // only 00 (standard operation) allowed
localparam NO_WRITE_BURST = 1'b1;   // 0= write burst enabled, 1=only single access write

localparam MODE = { 3'b000, NO_WRITE_BURST, OP_MODE, CAS_LATENCY, ACCESS_TYPE, BURST_LENGTH}; 

localparam STATE_IDLE  = 3'd0;   // first state in cycle
localparam STATE_START = 3'd1;   // state in which a new command can be started
localparam STATE_CONT  = STATE_START + RASCAS_DELAY; // 3 command can be continued
localparam STATE_READ  = STATE_CONT + CAS_LATENCY + 3'd1; // 6
localparam STATE_READY = STATE_READ + 3'd1; // 7
localparam STATE_LAST  = 3'd7;   // last state in cycle

reg  [2:0] q;
reg [22:0] a;
reg        wr;
reg        ram_req=0;
reg        vram_req=0;
reg        tape_req=0;
reg        cart_active=0;
reg  [1:0] active_bank;
reg  [7:0] cart_write_data;
reg  [5:0] cart_grants_since_refresh=0;
reg        refresh_due=0;

localparam MODE_NORMAL = 2'b00;
localparam MODE_RESET  = 2'b01;
localparam MODE_LDM    = 2'b10;
localparam MODE_PRE    = 2'b11;

reg [1:0] mode = MODE_RESET;
reg [4:0] reset = 5'h1f;
reg       init_old = 0;

// access manager
always @(posedge clk) begin
	reg [22:0] old_addr;
	reg old_rd, old_we, old_ref;

	old_rd<=oe;
	old_we<=we;
	old_ref<=clkref;

	if(q==STATE_IDLE) begin
		ram_req <= 0;
		vram_req <= 0;
		tape_req <= 0;
		cart_active <= 0;
		wr <= 0;

		// No client is admitted during the complete SDRAM initialization
		// sequence. With no admitted client a normal-mode slot is refresh.
		if((mode == MODE_NORMAL) && (reset == 0) && !init && !init_old &&
		   ((~old_rd & oe) | (~old_we & we))) begin
			ram_req <= 1;
			wr <= we;
			a <= addr;
		end
		// A due refresh outranks every new client except the edge-triggered
		// main port. Keeping refresh_due set across a main grant makes that
		// exception bounded rather than losing the refresh obligation.
		else if((mode == MODE_NORMAL) && (reset == 0) && !init && !init_old &&
		        refresh_due) begin
			refresh_due <= 0;
			cart_grants_since_refresh <= 0;
		end
		else if((mode == MODE_NORMAL) && (reset == 0) && !init && !init_old && cart_req) begin
			cart_active <= 1;
			wr <= cart_wr;
			a <= cart_addr;
			active_bank <= cart_bank;
			cart_write_data <= cart_din;
			if(cart_grants_since_refresh == 6'd31) begin
				cart_grants_since_refresh <= 6'd32;
				refresh_due <= 1;
			end
			else cart_grants_since_refresh <= cart_grants_since_refresh + 1'd1;
		end
		else if((mode == MODE_NORMAL) && (reset == 0) && !init && !init_old &&
		        (tape_rd | tape_wr)) begin
			tape_req <= 1;
			wr <= tape_wr;
			a <= tape_addr;
		end
		else if((mode == MODE_NORMAL) && (reset == 0) && !init && !init_old &&
		        (old_addr[15:1] != vram_addr[15:1])) begin
			vram_req <= 1;
			old_addr <= vram_addr;
			a <= vram_addr;
		end
		else if(mode == MODE_NORMAL) begin
			// This is the ordinary idle-slot refresh path.
			refresh_due <= 0;
			cart_grants_since_refresh <= 0;
		end
	end

	q <= q + 3'd1;
	if(~old_ref & clkref) q <= 0;
end

// initialization 
always @(posedge clk) begin
	init_old <= init;

	if(init_old & ~init) reset <= 5'h1f;
	else if(q == STATE_LAST) begin
		if(reset != 0) begin
			reset <= reset - 5'd1;
			if(reset == 14)     mode <= MODE_PRE;
			else if(reset == 3) mode <= MODE_LDM;
			else                mode <= MODE_RESET;
		end
		else mode <= MODE_NORMAL;
	end
end

localparam CMD_NOP             = 3'b111;
localparam CMD_ACTIVE          = 3'b011;
localparam CMD_READ            = 3'b101;
localparam CMD_WRITE           = 3'b100;
localparam CMD_BURST_TERMINATE = 3'b110;
localparam CMD_PRECHARGE       = 3'b010;
localparam CMD_AUTO_REFRESH    = 3'b001;
localparam CMD_LOAD_MODE       = 3'b000;

reg [7:0] ram_dout;
reg [15:0] sdram_dq_out;
reg        sdram_dq_oe;

assign SDRAM_DQ = sdram_dq_oe ? sdram_dq_out : 16'hzzzz;

// SDRAM state machines
always @(posedge clk) begin
	reg [15:0] data;

	casex({ram_req|vram_req|tape_req|cart_active,wr,mode,q})
		{2'b1X, MODE_NORMAL, STATE_START}: {SDRAM_nRAS, SDRAM_nCAS, SDRAM_nWE} <= CMD_ACTIVE;
		{2'b11, MODE_NORMAL, STATE_CONT }: {SDRAM_nRAS, SDRAM_nCAS, SDRAM_nWE} <= CMD_WRITE;
		{2'b10, MODE_NORMAL, STATE_CONT }: {SDRAM_nRAS, SDRAM_nCAS, SDRAM_nWE} <= CMD_READ;
		{2'b0X, MODE_NORMAL, STATE_START}: {SDRAM_nRAS, SDRAM_nCAS, SDRAM_nWE} <= CMD_AUTO_REFRESH;

		// init
		{2'bXX,    MODE_LDM, STATE_START}: {SDRAM_nRAS, SDRAM_nCAS, SDRAM_nWE} <= CMD_LOAD_MODE;
		{2'bXX,    MODE_PRE, STATE_START}: {SDRAM_nRAS, SDRAM_nCAS, SDRAM_nWE} <= CMD_PRECHARGE;

		                          default: {SDRAM_nRAS, SDRAM_nCAS, SDRAM_nWE} <= CMD_NOP;
	endcase

	casex({ram_req|vram_req|tape_req|cart_active,mode,q})
		{1'b1,  MODE_NORMAL, STATE_START}: SDRAM_A <= a[21:9];
		{1'b1,  MODE_NORMAL, STATE_CONT }: SDRAM_A <= {~a[0] & wr, a[0] & wr, 2'b10, a[22], a[8:1]};

		// init
		{1'bX,     MODE_LDM, STATE_START}: SDRAM_A <= MODE;
		{1'bX,     MODE_PRE, STATE_START}: SDRAM_A <= 13'b0010000000000;

		                          default: SDRAM_A <= 13'b0000000000000;
	endcase

	if(q == STATE_START) begin
		SDRAM_BA <= (mode == MODE_NORMAL) ?
		            (cart_active ? active_bank : tape_req ? 2'b10 :
		             vram_req ? vram_bank : bank) : 2'b00;
		if(ram_req & wr) ram_dout <= din;
	end

	data <= SDRAM_DQ;
	sdram_dq_oe <= 0;
	if(q == STATE_CONT && wr) begin
		sdram_dq_out <= tape_req ? {tape_din, tape_din} :
		                cart_active ? {cart_write_data, cart_write_data} : {din, din};
		sdram_dq_oe <= 1;
	end

	tape_wr_ack	<= 0;
	cart_ack <= 0;
	// Raise acknowledge for the complete READY phase. Read data is sampled
	// on the edge entering READY, so a synchronous requester consumes the
	// acknowledge and may update a still-held request before the next IDLE
	// arbitration edge. This avoids re-granting the old request.
	if(q == STATE_READ && cart_active) begin
		if(!wr) cart_dout <= a[0] ? SDRAM_DQ[15:8] : SDRAM_DQ[7:0];
		cart_ack <= 1;
	end
	if (q == STATE_READY) begin
		if (~wr & ram_req) ram_dout <= a[0] ? data[15:8] : data[7:0];
		else if (vram_req) vram_dout<=data;
		else if (~wr & tape_req) tape_dout <= a[0] ? data[15:8] : data[7:0];
		if(tape_req) begin
			if(wr) tape_wr_ack <= 1;
			else tape_rd_ack <= ~tape_rd_ack;
		end
	end
end

altddio_out
#(
	.extend_oe_disable("OFF"),
	.intended_device_family("Cyclone V"),
	.invert_output("OFF"),
	.lpm_hint("UNUSED"),
	.lpm_type("altddio_out"),
	.oe_reg("UNREGISTERED"),
	.power_up_high("OFF"),
	.width(1)
)
sdramclk_ddr
(
	.datain_h(1'b0),
	.datain_l(1'b1),
	.outclock(clk),
	.dataout(SDRAM_CLK),
	.aclr(1'b0),
	.aset(1'b0),
	.oe(1'b1),
	.outclocken(1'b1),
	.sclr(1'b0),
	.sset(1'b0)
);

endmodule
