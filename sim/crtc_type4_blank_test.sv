`timescale 1ns/1ps
module crtc_type4_blank_test;
	reg clock = 0, clken = 0, nreset = 0, pixen = 0;
	reg sna_load = 0, sna_hs = 0;
	reg [143:0] regs = 0;
	wire [1:0] hs, vs;
	wire [3:0] red [0:1];
	wire [3:0] green [0:1];
	wire [3:0] blue [0:1];

	for (genvar profile = 0; profile < 2; profile++) begin : video
		asic_video #(.ASIC_40226(profile != 0)) dut (
			.CLOCK(clock), .CLKEN(clken), .nRESET(nreset),
			.ENABLE(1'b0), .nCS(1'b1), .R_nW(1'b1), .RS(1'b0), .DI(8'd0), .DO(),
			.HSYNC(hs[profile]), .VSYNC(vs[profile]), .DE(), .FIELD(),
			.MA(), .RA(), .HCC(), .LINE(), .ROW(), .ADJ(),
			.SPLT(8'd0), .SSA(14'd0), .SSCR(8'd0),
			.PIXEN(pixen), .VIDEOD(16'd0), .MODE_UPDATE_EN(1'b0), .GAMODE(2'd0),
			.BORDER_I(5'd11), .INKR_I({16{5'd11}}),
			.RGB_R(red[profile]), .RGB_G(green[profile]), .RGB_B(blue[profile]),
			.PEN(), .HWRAP(), .SPR_EN(1'b0), .SPR_RGB(12'd0),
			.PAL_EN(1'b0), .PAL_ADDR(), .PAL_RGB(12'd0),
			.SNA_LOAD(sna_load), .SNA_ADDR(5'd0), .SNA_REGS(regs), .SNA_HCC(8'd0),
			.SNA_LINE(7'd0), .SNA_RASTER(5'd0), .SNA_VTA(5'd0),
			.SNA_HSW(4'd0), .SNA_VSW(4'd0), .SNA_VS(1'b0), .SNA_HS(sna_hs),
			.SNA_ADJ(1'b0), .SNA_MODE(2'd0));
	end

	task automatic step(input bit pixel, character);
		pixen = pixel;
		clken = character;
		#5 clock = 1;
		#5 clock = 0;
	endtask

	task automatic verify_colours(input logic [1:0] blank, input int dot);
		for (int profile = 0; profile < 2; profile++) begin
			if ({red[profile], green[profile], blue[profile]}
			    !== (blank[profile] ? 12'h000 : 12'hfff))
				$fatal(1, "profile %0d RGB mask wrong at dot %0d", profile, dot);
		end
	endtask

	initial begin
		step(0, 0);
		nreset = 1;
		regs[0*8 +: 8] = 31;
		regs[2*8 +: 8] = 4;
		regs[3*8 +: 8] = 2;
		sna_load = 1;
		step(0, 0);
		sna_load = 0;
		for (int dot = 0; dot < 120; dot++) begin
			// First pixel of each group of four system clocks, as on board.
			step(1, dot % 16 == 0);
			if (hs !== {2{dot >= 48 && dot < 80}} || vs[0] !== vs[1])
				$fatal(1, "raw sync mismatch at dot %0d", dot);
			// ACCC §14.7.2 describes a two-mode-2-pixel difference.
			// Both edges move together; pulse width and IRQ input are shared.
			verify_colours({dot >= 66 && dot < 98, dot >= 64 && dot < 96}, dot);
			repeat (3) step(0, 0);
		end

		// Restoring inside HSYNC must mask the very first resumed pixel.
		sna_hs = 1;
		sna_load = 1;
		step(0, 0);
		sna_load = 0;
		step(1, 0);
		verify_colours(2'b11, 120);

		$display("PASS CRTC4 horizontal blackout offset");
		$finish;
	end
endmodule
