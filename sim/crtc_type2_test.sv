`timescale 1ns/1ps
module crtc_type2_test;
	reg clock = 0, clken = 0, nreset = 0;
	reg enable = 0, cs_n = 1, read_port = 0, rs = 0;
	reg [7:0] di = 0;
	wire [7:0] dout;
	wire hs, vs, de;
	wire [13:0] ma;
	wire [4:0] ra;

	crtc_type2 dut (
		.CLOCK(clock), .CLKEN(clken), .nRESET(nreset), .ENABLE(enable),
		.nCS(cs_n), .R_nW(read_port), .RS(rs), .DI(di), .DO(dout),
		.HSYNC(hs), .VSYNC(vs), .DE(de), .MA(ma), .RA(ra));

	task automatic step(input bit character);
		clken = character;
		#5 clock = 1;
		#5 clock = 0;
	endtask

	task automatic bus_write(input bit data_port, input logic [7:0] value);
		enable = 1;
		cs_n = 0;
		read_port = 0;
		rs = data_port;
		di = value;
		step(0);
		enable = 0;
	endtask

	task automatic write_reg(input logic [7:0] addr, value);
		bus_write(0, addr);
		bus_write(1, value);
	endtask

	task automatic verify_read(input bit data_port, input logic [7:0] expected);
		enable = 1;
		read_port = 1;
		rs = data_port;
		#1;
		if (dout !== expected)
			$fatal(1, "port %b read %02x, expected %02x", data_port, dout, expected);
		enable = 0;
	endtask

	task automatic set_timing(input logic [7:0] sync_width);
		write_reg(0, 31);
		write_reg(1, 8);
		write_reg(2, 12);
		write_reg(3, sync_width);
		write_reg(4, 19);
		write_reg(5, 3);
		write_reg(6, 12);
		write_reg(7, 5);
		write_reg(8, 8'h30);
		write_reg(9, 1);
		write_reg(12, 8'h12);
		write_reg(13, 8'h80);
	endtask

	task automatic find_origin(input int last_raster = 1, input logic [4:0] first_ra = 0);
		for (int tick = 0; tick < 3 * (20 * (last_raster + 1) + 3) * 32; tick++) begin
			step(1);
			if (ma == 14'h1280 && ra == first_ra && de) return;
		end
		$fatal(1, "no frame at the programmed origin");
	endtask

	task automatic verify_mid_vsync(input logic [7:0] mode, sync_row, input bit ghost);
		nreset = 0;
		step(0);
		nreset = 1;
		set_timing(8'hf3);
		write_reg(2, ghost ? 8'd12 : 8'd18);
		write_reg(7, sync_row);
		write_reg(8, mode);
		find_origin(1, mode[1:0] == 3 ? 5'd1 : 5'd0);

		// The first programmed frame is odd. Even interlace starts at
		// C0=R0/2=15, including when R7 is the new frame's origin
	// (FR §19.7.2 p.219). Both phases count 16 C0=0 edges,
	// independent of the R3 high nibble (FR §16.1 p.161).
		for (int field = 0; field < 2; field++) begin
		int onset, release_tick;
			bit hidden;
			onset = 2 * int'(sync_row) * 32 + (field == 1 && mode[0] ? 15 : 0);
		release_tick = (2 * int'(sync_row) + 16) * 32;
			hidden = ghost && field == 1;
			for (int tick = 0; tick < 43 * 32; tick++) begin
			if (vs !== (!hidden && tick >= onset && tick < release_tick))
					$fatal(1, "R8=%02x R7=%0d field=%0d x=%0d y=%0d ghost=%0b VSYNC=%0b",
					       mode, sync_row, field, tick % 32, tick / 32, ghost, vs);
				if (hidden && tick == onset + 1) begin
					// The midpoint coincides with the HSYNC release
					// character. Once clear, a progressive R7 trigger
					// still cannot bypass the ghost (FR §16.4.3 p.171).
					write_reg(8, 8'h30);
					write_reg(7, 127);
					write_reg(7, sync_row);
					if (vs) $fatal(1, "MID-VSYNC ghost did not consume the sync interval");
				end
				step(1);
			end
		end
	endtask

	task automatic verify_ghost_vsync;
		// At x=15 the 3-character HSYNC has just fallen. The whole
		// character still blocks the VSYNC pin (FR §16.4.3 p.171).
		repeat (15) step(1);
		if (hs || vs) $fatal(1, "unexpected sync before the R7 write");
		write_reg(7, 0);
		if (vs) $fatal(1, "R7 write on the character after HSYNC asserted VSYNC");
		step(1);
		write_reg(7, 127);
		write_reg(7, 0);
		if (vs) $fatal(1, "ghost VSYNC did not consume the pending sync interval");

		// The ghost started on y=1. By y=18 the same type of write
		// outside the sync window can reach the pin again.
		repeat (17 * 32 - 15) step(1);
		write_reg(7, 9);
		if (!vs) $fatal(1, "ghost VSYNC did not release the next visible pulse");
	endtask

	task automatic verify_adjustment_wrap;
		nreset = 0;
		step(0);
		nreset = 1;
		set_timing(8'hf3);
		find_origin();
		repeat (41 * 32 + 2) step(1);
		if (de) $fatal(1, "expected second R5 adjustment line");

		// C5+1 has passed 1, so the active adjustment continues
		// round the 5-bit counter to that target (FR §11.3.1 p.87).
		write_reg(5, 1);
		for (int tick = 0; tick < 32 * 32 - 2; tick++) begin
			if (de) $fatal(1, "frame restarted before C5 wrapped, tick=%0d", tick);
			step(1);
		end
		if (ma !== 14'h1280 || ra !== 0 || !de)
			$fatal(1, "adjustment did not retire at the wrapped target");
	endtask

	task automatic verify_hsync_collision;
		nreset = 0;
		step(0);
		nreset = 1;
		write_reg(0, 3);
		write_reg(2, 1);
		write_reg(3, 4);

		// R0+1 equals the width. Returning to R2 on the expiry
		// edge holds HSYNC while C3 wraps (FR §15.3.1–2 p.150).
		for (int tick = 0; tick < 22; tick++) begin
			step(1);
			if (!hs) $fatal(1, "HSYNC dropped on a simultaneous restart, tick=%0d", tick);
		end
	endtask

	task automatic verify_line_start_sync(
		input logic [7:0] position, width,
		input bit expect_hs, expect_vs, expect_de
	);
		nreset = 0;
		step(0);
		nreset = 1;
		set_timing(width);
		find_origin();
		write_reg(2, position);
		repeat (10 * 32) step(1);
		// Natural R7 compare sees the outgoing HSYNC, while display
		// enable sees the incoming HSYNC at x=0 (FR §15.6 p.157).
		if ({hs, vs, de} !== {expect_hs, expect_vs, expect_de})
			$fatal(1, "R2=%0d R3=%0d line-start HS/VS/DE was %b%b%b",
			       position, width, hs, vs, de);
	endtask

	task automatic verify_zero_r1_pointer;
		nreset = 0;
		step(0);
		nreset = 1;
		set_timing(8'h03);
		write_reg(4, 0);
		write_reg(5, 0);
		find_origin();
		step(1);
		write_reg(1, 0);
		repeat (33) step(1);

		// On the last line, R1=0 has already been compared at C0=0.
		// R12/R13 written at C0=2 reach VMA' on the next C0=0
		// through the AND stage only (FR §17.4.3 p.186).
		write_reg(12, 8'h10);
		write_reg(13, 8'hc1);
		repeat (30) step(1);
		if (ma !== 14'h1080 || ra !== 0)
			$fatal(1, "R1=0 AND-only reload was %04x at frame origin", ma);
	endtask

	task automatic verify_zero_r1_row_advance;
		nreset = 0;
		step(0);
		nreset = 1;
		set_timing(8'h03);
		find_origin();
		repeat (32 + 9) step(1);

		// The ordinary row already latched VMA at R1=8. The next
		// line starts at C9=0, where R9=1 forbids a new R1=0 latch.
		write_reg(1, 0);
		repeat (32 - 9) step(1);
		if (ma !== 14'h1288 || ra !== 0)
			$fatal(1, "R1=0 discarded the pending row pointer, MA=%04x", ma);
	endtask

	task automatic verify_hcc_overflow_address;
		nreset = 0;
		step(0);
		nreset = 1;
		set_timing(8'h03);
		find_origin();
		repeat (32 + 25) step(1);
		write_reg(0, 9);

		// C0 overruns the new R0 and wraps from 255 to 0. This is
		// still the same raster. FR §17.1 p.178 advances MA for
		// every character whether or not it can be displayed.
		repeat (256 - 25) step(1);
		if (ma !== 14'h1380 || ra !== 1 || de)
			$fatal(1, "character overflow rewound MA or started a line, MA=%04x", ma);

		// The second R1 match on the same raster captures the
		// advanced address. The real R0 seam transfers it to the next row.
		repeat (10) step(1);
		if (ma !== 14'h1388 || ra !== 0 || !de)
			$fatal(1, "post-overflow R1 did not capture advanced MA, MA=%04x", ma);
	endtask

	task automatic verify_ivm(input logic [7:0] last_raster, input bit odd_frame);
		logic [13:0] next_row_addr;
		nreset = 0;
		step(0);
		nreset = 1;
		set_timing(8'h03);
		write_reg(6, 2);
		write_reg(9, last_raster);
		find_origin(int'(last_raster));

		// R6 was reached in the preceding progressive frame. The first
		// programmed origin is odd and the next is even (FR §19.5.4 p.213).
		if (!odd_frame) repeat ((20 * (int'(last_raster) + 1) + 3) * 32) step(1);
		repeat (5 * 32 + 3) step(1);
		if (ma !== 14'h1283 || ra !== 5'd5 || !de)
			$fatal(1, "progressive MA/RA changed at the IVM midpoint, MA=%04x RA=%02x", ma, ra);

		// C9.IVM has already reset at C9=3 and advanced once. A new
		// R9 changes the comparator but keeps that counter history. Two
		// R8 writes at C0=3 switch RA without a character clock
		// (FR §19.8.3 pp.227–228).
		write_reg(9, 11);
		write_reg(8, 8'h33);
		if (ma !== 14'h1283 || ra !== {4'd1, odd_frame} || !de)
			$fatal(1, "live R8=3 missed C9.IVM history, MA=%04x RA=%02x", ma, ra);
		write_reg(8, 8'h32);
		if (ma !== 14'h1283 || ra !== 5'd5 || !de)
			$fatal(1, "live R8=2 did not restore progressive RA, MA=%04x RA=%02x", ma, ra);
		write_reg(9, last_raster);
		write_reg(8, 8'h33);

		// R9=7 reaches the VMA capture limit again on the last raster.
		// R9=6 ends the row one raster early, so it repeats the base.
		// Both advance at the next midpoint while C4 stays in row 1
		// (FR §19.4.3 pp.204–205 and §19.8.3 p.227).
		next_row_addr = last_raster[0] ? 14'h1288 : 14'h1280;
		repeat ((int'(last_raster) - 4) * 32 - 3) step(1);
		if (ma !== next_row_addr || ra !== {4'd0, odd_frame} || !de)
			$fatal(1, "R9=%0d IVM row seam MA=%04x RA=%02x", last_raster, ma, ra);
		repeat (4 * 32) step(1);
		if (ma !== next_row_addr + 14'd8 || ra !== {4'd0, odd_frame} || !de)
			$fatal(1, "R9=%0d IVM midpoint MA=%04x RA=%02x", last_raster, ma, ra);
		repeat ((int'(last_raster) - 3) * 32) step(1);
		if (de) $fatal(1, "R9=%0d IVM did not blank at C4=R6", last_raster);
	endtask

	initial begin
		step(0);
		nreset = 1;
		set_timing(8'h10);

		// Skip power-up and find a frame origin at the programmed display address.
		for (int tick = 0; tick < 3 * 43 * 32; tick++) begin
			step(1);
			if (hs) break;
		end
		if (!hs) $fatal(1, "R3 low nibble zero must produce HSYNC (ACCC §14.6)");
		find_origin();
		for (int tick = 0; tick < 43 * 32; tick++) begin
			int x, y;
			x = tick % 32;
			y = tick / 32;
			if (hs !== (x >= 12 && x < 28))
				$fatal(1, "HSYNC mismatch at (%0d,%0d)", x, y);
			if (vs !== (y >= 10 && y < 26))
				$fatal(1, "VSYNC mismatch at (%0d,%0d)", x, y);
			if (de !== (x < 8 && y < 24))
				$fatal(1, "DE mismatch at (%0d,%0d)", x, y);
			if (y < 40 && (ra !== 5'(y % 2) || ma !== 14'('h1280 + 8 * (y / 2) + x)))
				$fatal(1, "MA/RA %04x/%02x mismatch at (%0d,%0d)", ma, ra, x, y);
			step(1);
		end
		if (ma !== 14'h1280 || ra !== 0 || !de)
			$fatal(1, "R5 frame did not return to its display origin");

		// Nonzero R3 uses its horizontal nibble only. R8 display skew is ignored.
		write_reg(3, 8'hf3);
		for (int x = 0; x < 32; x++) begin
			if (hs !== (x >= 12 && x < 15) || de !== (x < 8))
				$fatal(1, "nonzero R3 or display skew mismatch at x=%0d", x);
			step(1);
		end
		verify_ghost_vsync();
		verify_adjustment_wrap();

		// AmSpirit 1.18.3 and FR §21.2.2 p.245 preserve cursor
		// readback through the five-bit register-select aliases.
		write_reg(14, 8'hff);
		verify_read(1, 8'h3f);
		verify_read(0, 8'hff);
		write_reg(15, 8'ha5);
		verify_read(1, 8'ha5);
		bus_write(0, 8'h2e);
		verify_read(1, 8'h3f);
		bus_write(0, 8'h2f);
		verify_read(1, 8'ha5);
		write_reg(16, 8'hff);
		verify_read(1, 8'h00);
		write_reg(17, 8'hff);
		verify_read(1, 8'h00);
		write_reg(31, 8'hff);
		verify_read(1, 8'h00);
		cs_n = 1;
		enable = 1;
		#1;
		if (dout !== 8'hff) $fatal(1, "deselected CRTC must release the data bus");
		enable = 0;
		verify_hsync_collision();
		verify_line_start_sync(0, 6, 1, 1, 0);
		verify_line_start_sync(18, 14, 0, 0, 1);
		verify_line_start_sync(18, 15, 1, 0, 0);
		verify_zero_r1_pointer();
		verify_zero_r1_row_advance();
		verify_hcc_overflow_address();
		verify_ivm(7, 1);
		verify_ivm(7, 0);
		verify_ivm(6, 1);
		verify_ivm(6, 0);
		verify_mid_vsync(8'h31, 5, 0);
		verify_mid_vsync(8'h33, 0, 0);
		verify_mid_vsync(8'h32, 5, 0);
		verify_mid_vsync(8'h33, 5, 1);

		$display("PASS CRTC2 progressive timing, I/O, IVM addressing and MID-VSYNC");
		$finish;
	end
endmodule
