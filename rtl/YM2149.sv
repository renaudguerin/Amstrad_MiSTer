//
// Copyright (c) MikeJ - Jan 2005
// Copyright (c) 2016-2018 Sorgelig
//
// All rights reserved
//
// Redistribution and use in source and synthezised forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// Redistributions in synthesized form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// Neither the name of the author nor the names of other contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//


// BDIR  BC  MODE
//   0   0   inactive
//   0   1   read value
//   1   0   write value
//   1   1   set address
//

module YM2149
(
	input        CLK,       // Global clock
	input        CE,        // PSG Clock enable
	input        RESET,     // Chip RESET (set all Registers to '0', active hi)
	input        BDIR,      // Bus Direction (0 - read , 1 - write)
	input        BC,        // Bus control
	input  [7:0] DI,        // Data In
	output [7:0] DO,        // Data Out
	output [7:0] CHANNEL_A, // PSG Output channel A
	output [7:0] CHANNEL_B, // PSG Output channel B
	output [7:0] CHANNEL_C, // PSG Output channel C

	input        SEL,
	input        MODE,
	
	output [5:0] ACTIVE,

	input  [7:0] IOA_in,
	output [7:0] IOA_out,

	input  [7:0] IOB_in,
	output [7:0] IOB_out,

	input        SNA_LOAD,
	input  [3:0] SNA_ADDR,
	input [127:0] SNA_REGS
);

assign ACTIVE  = ~ymreg[7][5:0];
assign IOA_out = ymreg[14];
assign IOB_out = ymreg[15];

reg [7:0] addr;
reg [7:0] ymreg[16];

// Write to PSG
reg env_reset;
always @(posedge CLK) begin
	if(RESET) begin
		ymreg     <= '{default:0};
		ymreg[7]  <= '1;
		addr      <= '0;
		env_reset <= 0;
	end else if(SNA_LOAD) begin
		integer i;
		for (i = 0; i < 16; i = i + 1) ymreg[i] <= SNA_REGS[(i*8) +: 8];
		addr <= {4'h0, SNA_ADDR};
		env_reset <= 1;
	end else begin
		env_reset <= 0;
		if(BDIR) begin
			if(BC) addr <= DI;
			else if(!addr[7:4]) begin
				ymreg[addr[3:0]] <= DI;
				env_reset <= (addr == 13);
			end
		end
	end
end

// Read from PSG
assign DO = dout;
reg [7:0] dout;
always_comb begin
	dout = 8'hFF;
	if(~BDIR & BC & !addr[7:4]) begin
		case(addr[3:0])
			 0: dout = ymreg[0];
			 1: dout = ymreg[1][3:0];
			 2: dout = ymreg[2];
			 3: dout = ymreg[3][3:0];
			 4: dout = ymreg[4];
			 5: dout = ymreg[5][3:0];
			 6: dout = ymreg[6][4:0];
			 7: dout = ymreg[7];
			 8: dout = ymreg[8][4:0];
			 9: dout = ymreg[9][4:0];
			10: dout = ymreg[10][4:0];
			11: dout = ymreg[11];
			12: dout = ymreg[12];
			13: dout = ymreg[13][3:0];
			14: dout = ymreg[7][6] ? ymreg[14] & IOA_in : IOA_in;
			15: dout = ymreg[7][7] ? ymreg[15] & IOA_in : IOB_in;
		endcase
	end
end

reg ena_div;
reg ena_div_noise;

//  p_divider
always @(posedge CLK) begin
	reg [3:0]  cnt_div;
	reg        noise_div;

	if(CE) begin
		ena_div <= 0;
		ena_div_noise <= 0;
		if(!cnt_div) begin
			cnt_div <= {SEL, 3'b111};
			ena_div <= 1;
            
			noise_div <= (~noise_div);
			if (noise_div) ena_div_noise <= 1;
		end else begin
			cnt_div <= cnt_div - 1'b1;
		end
	end
end


reg [2:0] noise_gen_op;

//  p_noise_gen
always @(posedge CLK) begin
	reg [16:0] poly17;
	reg [4:0]  noise_gen_cnt;

	if(CE) begin
		if (ena_div_noise) begin
			if (!ymreg[6][4:0] || (noise_gen_cnt >= ymreg[6][4:0] - 1'd1)) begin
				noise_gen_cnt <= 0;
				poly17 <= {(poly17[0] ^ poly17[2] ^ !poly17), poly17[16:1]};
			end else begin
				noise_gen_cnt <= noise_gen_cnt + 1'd1;
			end
			noise_gen_op <= {3{poly17[0]}};
		end
	end
end

wire [11:0] tone_gen_freq[1:3];
assign tone_gen_freq[1] = {ymreg[1][3:0], ymreg[0]};
assign tone_gen_freq[2] = {ymreg[3][3:0], ymreg[2]};
assign tone_gen_freq[3] = {ymreg[5][3:0], ymreg[4]};

reg [3:1] tone_gen_op;

//p_tone_gens
always @(posedge CLK) begin
	integer i;
	reg [11:0] tone_gen_cnt[1:3];

	if(CE) begin
		// looks like real chips count up - we need to get the Exact behaviour ..
	
		for (i = 1; i <= 3; i = i + 1) begin
			if(ena_div) begin
				if (!tone_gen_freq[i] || (tone_gen_cnt[i] >= (tone_gen_freq[i] - 1'd1))) begin
					tone_gen_cnt[i] <= 0;
					tone_gen_op[i]  <= ~tone_gen_op[i];
				end else begin
					tone_gen_cnt[i] <= tone_gen_cnt[i] + 1'd1;
				end
			end
		end
	end
end

reg env_ena;
wire [15:0] env_gen_comp = {ymreg[12], ymreg[11]} ? {ymreg[12], ymreg[11]} - 1'd1 : 16'd0;

//p_envelope_freq
always @(posedge CLK) begin
	reg [15:0] env_gen_cnt;

	if(CE) begin
		env_ena <= 0;
		if(ena_div) begin
			if (env_gen_cnt >= env_gen_comp) begin
				env_gen_cnt <= 0;
				env_ena <= 1;
			end else begin
				env_gen_cnt <= (env_gen_cnt + 1'd1);
			end
		end
	end
end

reg [4:0] env_vol;

wire is_bot    = (env_vol == 5'b00000);
wire is_bot_p1 = (env_vol == 5'b00001);
wire is_top_m1 = (env_vol == 5'b11110);
wire is_top    = (env_vol == 5'b11111);

always @(posedge CLK) begin
	reg env_hold;
	reg env_inc;

	// envelope shapes
	// C AtAlH
	// 0 0 x x  \___
	//
	// 0 1 x x  /___
	//
	// 1 0 0 0  \\\\
	//
	// 1 0 0 1  \___
	//
	// 1 0 1 0  \/\/
	//           ___
	// 1 0 1 1  \
	//
	// 1 1 0 0  ////
	//           ___
	// 1 1 0 1  /
	//
	// 1 1 1 0  /\/\
	//
	// 1 1 1 1  /___

	if(env_reset | RESET) begin
		// load initial state
		if(!ymreg[13][2]) begin		// attack
			env_vol <= 5'b11111;
			env_inc <= 0;		// -1
		end else begin
			env_vol <= 5'b00000;
			env_inc <= 1;		// +1
		end
		env_hold <= 0;
	end
	else if(CE) begin
		if (env_ena) begin
			if (!env_hold) begin
				if (env_inc) env_vol <= (env_vol + 5'b00001);
					else env_vol <= (env_vol + 5'b11111);
			end

			// envelope shape control.
			if(!ymreg[13][3]) begin
				if(!env_inc) begin	// down
					if(is_bot_p1) env_hold <= 1;
				end else if (is_top) env_hold <= 1;
			end else if(ymreg[13][0]) begin		// hold = 1
				if(!env_inc) begin	// down
					if(ymreg[13][1]) begin		// alt
						if(is_bot) env_hold <= 1;
					end else if(is_bot_p1) env_hold <= 1;
				end else if(ymreg[13][1]) begin	// alt
					if(is_top) env_hold <= 1;
				end else if(is_top_m1) env_hold <= 1;
			end else if(ymreg[13][1]) begin		// alternate
				if(env_inc == 1'b0) begin		// down
					if(is_bot_p1) env_hold <= 1;
					if(is_bot) begin
						env_hold <= 0;
						env_inc  <= 1;
					end
				end else begin
					if(is_top_m1) env_hold <= 1;
					if(is_top) begin
						env_hold <= 0;
						env_inc  <= 0;
					end
				end
			end
		end
	end
end

reg [5:0] A,B,C;
always @(posedge CLK) begin
	A <= {MODE, ~((ymreg[7][0] | tone_gen_op[1]) & (ymreg[7][3] | noise_gen_op[0])) ? 5'd0 : ymreg[8][4]  ? env_vol[4:0] : { ymreg[8][3:0],  ymreg[8][3]}};
	B <= {MODE, ~((ymreg[7][1] | tone_gen_op[2]) & (ymreg[7][4] | noise_gen_op[1])) ? 5'd0 : ymreg[9][4]  ? env_vol[4:0] : { ymreg[9][3:0],  ymreg[9][3]}};
	C <= {MODE, ~((ymreg[7][2] | tone_gen_op[3]) & (ymreg[7][5] | noise_gen_op[2])) ? 5'd0 : ymreg[10][4] ? env_vol[4:0] : {ymreg[10][3:0], ymreg[10][3]}};
end

// Keep this as a case lookup rather than an initialized unpacked array.  The
// latter is accepted by newer simulator versions but is rejected by the
// older simulator/Quartus toolchain used for the production build.
function [7:0] volume_table;
	input [5:0] index;
	begin
		case (index)
			// YM2149
			6'h00: volume_table = 8'h00;
			6'h01: volume_table = 8'h01;
			6'h02: volume_table = 8'h01;
			6'h03: volume_table = 8'h02;
			6'h04: volume_table = 8'h02;
			6'h05: volume_table = 8'h03;
			6'h06: volume_table = 8'h03;
			6'h07: volume_table = 8'h04;
			6'h08: volume_table = 8'h06;
			6'h09: volume_table = 8'h07;
			6'h0a: volume_table = 8'h09;
			6'h0b: volume_table = 8'h0a;
			6'h0c: volume_table = 8'h0c;
			6'h0d: volume_table = 8'h0e;
			6'h0e: volume_table = 8'h11;
			6'h0f: volume_table = 8'h13;
			6'h10: volume_table = 8'h17;
			6'h11: volume_table = 8'h1b;
			6'h12: volume_table = 8'h20;
			6'h13: volume_table = 8'h25;
			6'h14: volume_table = 8'h2c;
			6'h15: volume_table = 8'h35;
			6'h16: volume_table = 8'h3e;
			6'h17: volume_table = 8'h47;
			6'h18: volume_table = 8'h54;
			6'h19: volume_table = 8'h66;
			6'h1a: volume_table = 8'h77;
			6'h1b: volume_table = 8'h88;
			6'h1c: volume_table = 8'ha1;
			6'h1d: volume_table = 8'hc0;
			6'h1e: volume_table = 8'he0;
			6'h1f: volume_table = 8'hff;

			// AY8910
			6'h20: volume_table = 8'h00;
			6'h21: volume_table = 8'h00;
			6'h22: volume_table = 8'h03;
			6'h23: volume_table = 8'h03;
			6'h24: volume_table = 8'h04;
			6'h25: volume_table = 8'h04;
			6'h26: volume_table = 8'h06;
			6'h27: volume_table = 8'h06;
			6'h28: volume_table = 8'h0a;
			6'h29: volume_table = 8'h0a;
			6'h2a: volume_table = 8'h0f;
			6'h2b: volume_table = 8'h0f;
			6'h2c: volume_table = 8'h15;
			6'h2d: volume_table = 8'h15;
			6'h2e: volume_table = 8'h22;
			6'h2f: volume_table = 8'h22;
			6'h30: volume_table = 8'h28;
			6'h31: volume_table = 8'h28;
			6'h32: volume_table = 8'h41;
			6'h33: volume_table = 8'h41;
			6'h34: volume_table = 8'h5b;
			6'h35: volume_table = 8'h5b;
			6'h36: volume_table = 8'h72;
			6'h37: volume_table = 8'h72;
			6'h38: volume_table = 8'h90;
			6'h39: volume_table = 8'h90;
			6'h3a: volume_table = 8'hb5;
			6'h3b: volume_table = 8'hb5;
			6'h3c: volume_table = 8'hd7;
			6'h3d: volume_table = 8'hd7;
			6'h3e: volume_table = 8'hff;
			6'h3f: volume_table = 8'hff;
		endcase
	end
endfunction

assign CHANNEL_A = volume_table(A);
assign CHANNEL_B = volume_table(B);
assign CHANNEL_C = volume_table(C);

endmodule
