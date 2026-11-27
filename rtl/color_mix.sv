//
//
// Copyright (c) 2018 Sorgelig
//
// This program is GPL v2+ Licensed.
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

module color_mix
(
	input            clk_vid,
	input            ce_pix,
	input      [2:0] mix,

	input      [1:0] R_in,
	input      [1:0] G_in,
	input      [1:0] B_in,
	input            HSync_in,
	input            VSync_in,
	input            HBlank_in,
	input            VBlank_in,

	output reg [7:0] R_out,
	output reg [7:0] G_out,
	output reg [7:0] B_out,
	output reg       HSync_out,
	output reg       VSync_out,
	output reg       HBlank_out,
	output reg       VBlank_out
);

// Every DAC palette entry is constant. Evaluate the monitor arithmetic on
// the entry itself so the selected pixel reaches the output register through
// a lookup, with the same truncation and no multiplier on the video path.
function automatic [39:0] monitor_entry(input [23:0] rgb);
    reg [15:0] weighted;
    reg [7:0] intensity;
    begin
        weighted = rgb[23:16] * 16'd54 + rgb[15:8] * 16'd183 + rgb[7:0] * 16'd18;
        intensity = weighted[15:8];
        monitor_entry = {rgb, intensity, intensity - {2'b00, weighted[15:10]}};
    end
endfunction

reg [39:0] entry;
wire [23:0] rgb = entry[39:16];
wire [7:0] intensity = entry[15:8];
wire [7:0] amber_green = entry[7:0];

// https://www.grimware.org/doku.php/documentations/devices/gatearray
always @(*) begin
	casex ({ mix[0], R_in, G_in, B_in })
	// GA palette
	7'b0_00_00_00: entry = monitor_entry(24'h000201);
	7'b0_00_00_01: entry = monitor_entry(24'h00026B);
	7'b0_00_00_10: entry = monitor_entry(24'h0C02F4);
	7'b0_X1_00_00: entry = monitor_entry(24'h6C0201);
	7'b0_X1_00_X1: entry = monitor_entry(24'h690268);
	7'b0_X1_00_10: entry = monitor_entry(24'h6C02F2);
	7'b0_10_00_00: entry = monitor_entry(24'hF30506);
	7'b0_10_00_X1: entry = monitor_entry(24'hF00268);
	7'b0_10_00_10: entry = monitor_entry(24'hF302F4);
	7'b0_00_X1_00: entry = monitor_entry(24'h027801);
	7'b0_00_X1_X1: entry = monitor_entry(24'h007868);
	7'b0_00_X1_10: entry = monitor_entry(24'h0C7BF4);
	7'b0_X1_X1_00: entry = monitor_entry(24'h6E7B01);
	7'b0_X1_X1_X1: entry = monitor_entry(24'h6E7D6B);
	7'b0_X1_X1_10: entry = monitor_entry(24'h6E7BF6);
	7'b0_10_X1_00: entry = monitor_entry(24'hF37D0D);
	7'b0_10_X1_X1: entry = monitor_entry(24'hF37D6B);
	7'b0_10_X1_10: entry = monitor_entry(24'hFA80F9);
	7'b0_00_10_00: entry = monitor_entry(24'h02F001);
	7'b0_00_10_X1: entry = monitor_entry(24'h00F36B);
	7'b0_00_10_10: entry = monitor_entry(24'h0FF3F2);
	7'b0_X1_10_00: entry = monitor_entry(24'h71F504);
	7'b0_X1_10_X1: entry = monitor_entry(24'h71F36B);
	7'b0_X1_10_10: entry = monitor_entry(24'h71F3F4);
	7'b0_10_10_00: entry = monitor_entry(24'hF3F30D);
	7'b0_10_10_X1: entry = monitor_entry(24'hF3F36D);
	7'b0_10_10_10: entry = monitor_entry(24'hFFF3F9);
	// ASIC palette
	7'b1_00_00_00: entry = monitor_entry(24'h020702);
	7'b1_00_00_01: entry = monitor_entry(24'h050663);
	7'b1_00_00_10: entry = monitor_entry(24'h0507f1);
	7'b1_X1_00_00: entry = monitor_entry(24'h670600);
	7'b1_X1_00_X1: entry = monitor_entry(24'h680764);
	7'b1_X1_00_10: entry = monitor_entry(24'h6807F1);
	7'b1_10_00_00: entry = monitor_entry(24'hFD0704);
	7'b1_10_00_X1: entry = monitor_entry(24'hFF0764);
	7'b1_10_00_10: entry = monitor_entry(24'hFD07F2);
	7'b1_00_X1_00: entry = monitor_entry(24'h046703);
	7'b1_00_X1_X1: entry = monitor_entry(24'h046764);
	7'b1_00_X1_10: entry = monitor_entry(24'h0567F1);
	7'b1_X1_X1_00: entry = monitor_entry(24'h686704);
	7'b1_X1_X1_X1: entry = monitor_entry(24'h686764);
	7'b1_X1_X1_10: entry = monitor_entry(24'h6867F1);
	7'b1_10_X1_00: entry = monitor_entry(24'hFD6704);
	7'b1_10_X1_X1: entry = monitor_entry(24'hFD6763);
	7'b1_10_X1_10: entry = monitor_entry(24'hFD67F1);
	7'b1_00_10_00: entry = monitor_entry(24'h04F502);
	7'b1_00_10_X1: entry = monitor_entry(24'h04F562);
	7'b1_00_10_10: entry = monitor_entry(24'h04F5F1);
	7'b1_X1_10_00: entry = monitor_entry(24'h68F500);
	7'b1_X1_10_X1: entry = monitor_entry(24'h68F564);
	7'b1_X1_10_10: entry = monitor_entry(24'h68F5F1);
	7'b1_10_10_00: entry = monitor_entry(24'hFEF504);
	7'b1_10_10_X1: entry = monitor_entry(24'hFDF563);
	7'b1_10_10_10: entry = monitor_entry(24'hFDF5F0);

	default: entry = 40'd0; //invalid
	endcase
end

always @(posedge clk_vid) begin
	if(ce_pix) begin
		{R_out, G_out, B_out} <= 0;

		case(mix)
			0,
			1: {R_out, G_out, B_out} <= rgb;                              // color
			2: {       G_out       } <= {          intensity            }; // green
			3: {R_out, G_out       } <= {intensity, amber_green}; // amber
			4: {       G_out, B_out} <= {          intensity, intensity  }; // cyan
			5: {R_out, G_out, B_out} <= {intensity, intensity, intensity  }; // gray
		endcase

		HSync_out  <= HSync_in;
		VSync_out  <= VSync_in;
		HBlank_out <= HBlank_in;
		VBlank_out <= VBlank_in;
	end
end

endmodule
