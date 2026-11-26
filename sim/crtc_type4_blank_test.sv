`timescale 1ns/1ps
module crtc_type4_blank_test;
	reg clock = 0, clken = 0, nreset = 0, pixen = 0;
	reg sna_load = 0, sna_hs = 0;
	reg [7:0] sna_hcc = 0;
	reg [1:0] snapshot_mode = 0;
	reg use_ga_mode = 0, ga_iorq_n = 1;
	reg [1:0] target_mode = 1;
	reg crtc_write = 0, crtc_data_port = 0;
	reg [7:0] crtc_data = 0;
	reg [15:0] video_word = 0;
	reg [79:0] inks = {16{5'd11}};
	reg [4:0] border = 5'd11;
	reg [143:0] regs = 0;
	wire [1:0] hs, vs;
	wire [3:0] red [0:1];
	wire [3:0] green [0:1];
	wire [3:0] blue [0:1];
	wire ga_char, ga_mode_update;
	wire [1:0] ga_mode;

	asic_ga_timing ga (
		.clk(clock), .cen_16(pixen), .fast(1'b0), .RESET_N(nreset),
		.plus_unlocked(1'b0), .A(2'b01), .D({6'b100000, target_mode}), .MREQ_N(1'b1),
		.M1_N(1'b1), .RD_N(1'b1), .IORQ_N(ga_iorq_n),
		.HSYNC_I(hs[1]), .VSYNC_I(vs[1]),
		.CCLK(), .CCLK_EN_P(), .CCLK_EN_N(ga_char),
		.PHI_N(), .PHI_EN_N(), .PHI_EN_P(), .RAS_N(), .CAS_N(),
		.CASAD_N(), .READY(), .CPU_N(), .MWE_N(), .E244_N(),
		.ROMEN_N(), .RAMRD_N(), .ROM(),
		.pri(8'd0), .crtc_line(9'd0), .crtc_adj(1'b0), .intack(1'b0),
		.int_last_raster(), .HSYNC_O(), .VSYNC_O(), .SYNC_N(),
		.INT_N(), .VBLANK(), .MODE_SYNC_EN(), .ASIC_MODE_SYNC_EN(ga_mode_update),
		.MODE(), .BORDER_O(), .INKR_O(), .GAMODE_O(ga_mode),
		.LEGACY_PAL_WR(), .LEGACY_PAL_ADDR(), .LEGACY_PAL_DATA(),
		.SNA_LOAD(sna_load), .SNA_INKSEL(5'd0), .SNA_PALETTE(136'd0),
		.SNA_CONFIG({6'd0, snapshot_mode}), .SNA_VSDELAY(2'd0), .SNA_INTCNT(6'd0),
		.SNA_INT(1'b0), .SNA_VS(1'b0), .SNA_HS(sna_hs)
	);

	for (genvar profile = 0; profile < 2; profile++) begin : video
		asic_video #(.ASIC_40226(profile != 0)) dut (
			.CLOCK(clock), .CLKEN(use_ga_mode ? ga_char : clken), .nRESET(nreset),
			.ENABLE(crtc_write), .nCS(1'b0), .R_nW(1'b0),
			.RS(crtc_data_port), .DI(crtc_data), .DO(),
			.HSYNC(hs[profile]), .VSYNC(vs[profile]), .DE(), .FIELD(), .FRAME_FIELD(),
			.MA(), .RA(), .HCC(), .LINE(), .ROW(), .ADJ(),
			.SPLT(8'd0), .SSA(14'd0), .SSCR(8'd0),
			.PIXEN(pixen), .VIDEOD(video_word), .MODE_UPDATE_EN(use_ga_mode && ga_mode_update),
			.GAMODE(ga_mode),
			.BORDER_I(border), .INKR_I(inks),
			.RGB_R(red[profile]), .RGB_G(green[profile]), .RGB_B(blue[profile]),
			.PEN(), .HWRAP(), .SPR_EN(1'b0), .SPR_RGB(12'd0),
			.PAL_EN(1'b0), .PAL_ADDR(), .PAL_RGB(12'd0),
			.SNA_LOAD(sna_load), .SNA_ADDR(5'd0), .SNA_REGS(regs), .SNA_HCC(sna_hcc),
			.SNA_LINE(7'd0), .SNA_RASTER(5'd0), .SNA_VTA(5'd0),
			.SNA_HSW(4'd0), .SNA_VSW(4'd0), .SNA_VS(1'b0), .SNA_HS(sna_hs),
			.SNA_ADJ(1'b0), .SNA_MODE(snapshot_mode));
	end

	task automatic step(input bit pixel, character);
		pixen = pixel;
		clken = character;
		#5 clock = 1;
		#5 clock = 0;
	endtask

	function automatic logic [15:0] word_for(input int dot);
		return dot % 32 < 16 ? 16'h548d : 16'he926;
	endfunction

	function automatic logic [11:0] pixel_for(input int dot);
		logic [15:0] source;
		int bit_index;
		source = word_for(dot);
		bit_index = dot % 16;
		bit_index = bit_index < 8 ? 7 - bit_index : 23 - bit_index;
		return source[bit_index] ? 12'h00f : 12'hf00;
	endfunction

	task automatic verify_mode2_advance;
		nreset = 0;
		step(0, 0);
		nreset = 1;
		regs[0*8 +: 8] = 7;
		regs[1*8 +: 8] = 4;
		regs[2*8 +: 8] = 12;
		regs[4*8 +: 8] = 31;
		regs[6*8 +: 8] = 31;
		regs[7*8 +: 8] = 24;
		snapshot_mode = 2;
		inks[0*5 +: 5] = 12; // bright red
		inks[1*5 +: 5] = 21; // bright blue
		sna_hs = 0;
		sna_load = 1;
		step(0, 0);
		sna_load = 0;
		for (int dot = 0; dot <= 193; dot++) begin
			video_word = word_for(dot);
			step(1, dot % 16 == 0);
			// The nibble sequence straddles both the odd-byte latch and
			// the next even-byte latch. FR §9.1 p.48 puts the 40226
			// mode-2 stream one dot ahead, including the two DE edges.
			if (dot >= 128) begin
				if ({red[0], green[0], blue[0]} !==
				    (dot > 128 && dot <= 192 ? pixel_for(dot - 1) : 12'hfff))
					$fatal(1, "40489 mode-2 RGB at dot %0d", dot);
				if ({red[1], green[1], blue[1]} !==
				    (dot < 192 ? pixel_for(dot) : 12'hfff))
					$fatal(1, "40226 mode-2 RGB lead missing at dot %0d", dot);
			end
			repeat (3) step(0, 0);
		end
	endtask

	task automatic verify_ink_change(input bit use_mode2, use_border);
		nreset = 0;
		step(0, 0);
		nreset = 1;
		regs[0*8 +: 8] = 7;
		regs[1*8 +: 8] = use_border ? 0 : 4;
		regs[2*8 +: 8] = 12;
		regs[4*8 +: 8] = 31;
		regs[6*8 +: 8] = 31;
		regs[7*8 +: 8] = 24;
		snapshot_mode = use_mode2 ? 2'd2 : 2'd0;
		video_word = 0;
		inks[0*5 +: 5] = 12; // bright red in mode 0 or mode 2
		border = use_border ? 11 : 12;
		sna_hs = 0;
		sna_load = 1;
		step(0, 0);
		sna_load = 0;
		// The Gate Array's shadow updates on the SNA_LOAD edge. The
		// newly restored border must already drive the first resumed dot.
		if (use_border) border = 12;
		for (int dot = 0; dot <= 158; dot++) begin
			if (dot == 147) begin
				if (use_border) border = 21;
				else inks[0*5 +: 5] = 21;
			end
			step(1, dot % 16 == 0);
			if (use_border && dot == 0 && {red[1], green[1], blue[1]} !== 12'hf00)
				$fatal(1, "40226 leaked the pre-restore colour on the first resumed dot");
			if (dot >= 144) begin
				if ({red[0], green[0], blue[0]} !== (dot < 147 ? 12'hf00 : 12'h00f))
					$fatal(1, "40489 legacy colour changed at dot %0d", dot);
				// FR §9.2.2 pp.49–50: the change is on the second byte.
				// In Mode 2 that byte is presented one dot earlier.
				if ({red[1], green[1], blue[1]} !==
				    (dot < (use_mode2 ? 152 : 153) ? 12'hf00 : 12'h00f))
					$fatal(1, "40226 legacy colour phase at dot %0d in mode %0d, border=%0d",
					       dot, use_mode2 ? 2 : 0, use_border);
			end
			repeat (3) step(0, 0);
		end
	endtask

	task automatic verify_colours(input logic [1:0] blank, input int dot);
		for (int profile = 0; profile < 2; profile++) begin
			if ({red[profile], green[profile], blue[profile]}
			    !== (blank[profile] ? 12'h000 : 12'hfff))
				$fatal(1, "profile %0d RGB mask wrong at dot %0d", profile, dot);
		end
	endtask

	localparam logic [3:0] MODE1_DOTS [10] = '{1, 2, 2, 1, 1, 0, 0, 2, 2, 3};
	localparam logic [4:0] TEST_HW [16] =
		'{12,21,18,13,19,29,23,10,17,3,26,11,4,15,14,25};
	localparam logic [11:0] TEST_RGB [16] = '{
		12'hf00, 12'h00f, 12'h0f0, 12'hf0f, 12'h0ff, 12'h60f, 12'h06f, 12'hff0,
		12'h0f6, 12'hff6, 12'h6f0, 12'hfff, 12'h006, 12'hf6f, 12'hf60, 12'h6f6
	};

	task automatic verify_mode_change(input logic [1:0] old_mode, input logic [1:0] mode,
	                                 input logic [3:0] width, input bit rewrite_width,
	                                 input logic [7:0] mixed_byte,
	                                 input logic [3:0] expected [10],
	                                 input logic [11:0] ordinary_first_dot);
		bit aligned;
		int raw_fall, qualification, unblank;
		raw_fall = 48 + 16 * int'(width);
		qualification = raw_fall + 16;
		unblank = qualification + 2;
		target_mode = mode;
		use_ga_mode = 1;
		nreset = 0;
		step(0, 0);
		nreset = 1;
		regs = 0;
		regs[0*8 +: 8] = 31;
		regs[1*8 +: 8] = 20;
		regs[2*8 +: 8] = 3;
		regs[3*8 +: 8] = {4'd0, width};
		regs[4*8 +: 8] = 31;
		regs[6*8 +: 8] = 31;
		regs[7*8 +: 8] = 24;
		sna_hcc = 31;
		sna_hs = 0;
		snapshot_mode = old_mode;
		for (int ink = 0; ink < 16; ink++)
			inks[ink*5 +: 5] = TEST_HW[ink];
		video_word = {8'h6c, mixed_byte};
		sna_load = 1;
		step(0, 0);
		sna_load = 0;
		for (int dot = 0; dot < 16; dot++) begin
			pixen = 1;
			#1;
			if (ga_char) begin
				aligned = 1;
				break;
			end
			step(1, 0);
			repeat (3) step(0, 0);
		end
		if (!aligned) $fatal(1, "expected the real GA character enable");

		for (int dot = 0; dot <= unblank + 9; dot++) begin
			if (dot == 8) ga_iorq_n = 0;
			if (dot == 24) ga_iorq_n = 1;
			// Rewrite R3 after the actual pulse and before GA qualification.
			if (rewrite_width) case (dot - raw_fall)
				1: begin
					crtc_write = 1;
					crtc_data_port = 0;
					crtc_data = 3;
				end
				4, 9: crtc_write = 0;
				5: begin
					crtc_write = 1;
					crtc_data_port = 1;
					crtc_data = width == 2 ? 8'd3 : 8'd2;
				end
				default: ;
			endcase
			step(1, 0);
			if (hs[1] !== (dot >= 48 && dot < raw_fall))
				$fatal(1, "40226 test raw HSYNC phase at dot %0d", dot);
			if (ga_mode_update !== (dot == qualification))
				$fatal(1, "40226 test GA qualification at dot %0d", dot);
			if (dot == qualification && ga_mode !== mode)
				$fatal(1, "GA did not accept the Mode %0d RMR write", mode);
			if (dot >= 66 && dot < unblank && {red[1], green[1], blue[1]} !== 12'h000)
				$fatal(1, "40226 Mode %0d to Mode %0d blackout at dot %0d", old_mode, mode, dot);
			// FR §9.3.4.5 pp.69–73: one old dot, then five or six
			// mixed dots before ordinary decoding of the next byte.
			if (width == 2 && dot >= unblank &&
			    {red[1], green[1], blue[1]} !== TEST_RGB[expected[dot - unblank]])
				$fatal(1, "40226 Mode %0d to Mode %0d dot %0d rewrite=%0b: RGB=%03x expected=%03x",
				       old_mode, mode, dot, rewrite_width, {red[1], green[1], blue[1]}, TEST_RGB[expected[dot - unblank]]);
			if (dot == unblank && {red[0], green[0], blue[0]} !== ordinary_first_dot)
				$fatal(1, "40489 mode change leaked the 40226 transition");
			if (width == 3 && dot == unblank && {red[1], green[1], blue[1]} !== 12'h0f0)
				$fatal(1, "40226 mixed byte triggered by a write after a three-character pulse");
			repeat (3) step(0, 0);
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
		verify_mode2_advance();
		verify_ink_change(0, 0);
		verify_ink_change(1, 0);
		verify_ink_change(0, 1);
		verify_ink_change(1, 1);
		verify_mode_change(2, 1, 2, 0, 8'h29, MODE1_DOTS, 12'h0f0);
		verify_mode_change(2, 1, 2, 1, 8'h29, MODE1_DOTS, 12'h0f0);
		verify_mode_change(2, 1, 3, 1, 8'h29, MODE1_DOTS, 12'h0f0);

		// Independent bit patterns make b0, b1, b2, b3 and b4 distinguishable.
		// Next byte 6C emits 6,6,6 in Mode 0, 2,2,3 in Mode 1 and 2,2,2 in Mode 3.
		verify_mode_change(2, 0, 2, 0, 8'ha6, '{1,4,4,4,4,4,4,6,6,6}, 12'hf6f);
		verify_mode_change(2, 0, 2, 0, 8'h51, '{0,3,3,0,0,0,0,6,6,6}, 12'hf00);
		verify_mode_change(2, 0, 2, 0, 8'h2b, '{1,2,2,5,5,5,5,6,6,6}, 12'hf60);
		verify_mode_change(2, 3, 2, 0, 8'ha6, '{1,0,0,0,0,0,0,2,2,2}, 12'h00f);
		verify_mode_change(2, 3, 2, 0, 8'h51, '{0,3,3,0,0,0,0,2,2,2}, 12'hf00);
		verify_mode_change(2, 3, 2, 0, 8'h2b, '{1,2,2,1,1,1,1,2,2,2}, 12'h0f0);

		// FR §9.3.4.5.2 p.70: Mode 0 supplies its full first pen.
		verify_mode_change(0, 1, 2, 0, 8'h2b, '{14,2,2,0,0,3,3,2,2,3}, 12'h0f0);
		verify_mode_change(0, 2, 2, 0, 8'ha6, '{13,1,0,1,0,0,0,1,1,0}, 12'hf00);
		verify_mode_change(0, 3, 2, 0, 8'ha6, '{13,1,1,2,2,2,2,2,2,2}, 12'h00f);
		// FR §9.3.4.5.3 p.71: Mode 1 reuses the second pair, then
		// takes later pens from the portion of the byte still shifting.
		verify_mode_change(1, 0, 2, 0, 8'hb7, '{1,14,14,3,3,3,3,6,6,6}, 12'hf6f);
		verify_mode_change(1, 2, 2, 0, 8'hb7, '{1,0,1,1,0,1,0,1,1,0}, 12'hf00);
		verify_mode_change(1, 3, 2, 0, 8'hb7, '{1,2,2,3,3,3,3,2,2,2}, 12'h00f);
		// FR §9.3.4.5.4 p.73: Mode 3 starts with its two-bit first
		// pen, then shares the Mode 0 rearrangement of the remaining bits.
		verify_mode_change(3, 0, 2, 0, 8'h2b, '{2,14,14,8,8,8,8,6,6,6}, 12'hf60);
		verify_mode_change(3, 1, 2, 0, 8'ha6, '{1,1,1,2,2,3,3,2,2,3}, 12'h00f);
		verify_mode_change(3, 2, 2, 0, 8'h2b, '{2,0,0,1,0,1,0,1,1,0}, 12'hf00);

		$display("PASS CRTC4 horizontal blackout, Mode 2 lead, ink phase and mixed byte");
		$finish;
	end
endmodule
