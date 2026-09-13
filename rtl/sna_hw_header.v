//============================================================================
//  SNA v3 hardware header formatter for classic CPC hardware state
//  (offsets 0x2E..0xB4) (B18 slice 3).
//
//  Purely combinational module that assembles classic hardware observations
//  into the SNA v3 header bytes (offsets 0x2E to 0xB4, 135 bytes total).
//
//  Reference: docs/references/Snapshot (.SNA) file format.md
//    0x2E       GA pen select (0-15)
//    0x2F-0x3E  GA palette pens 0-15 (hardware colours)
//    0x3F       GA border colour (hardware colour)
//    0x40       GA multi-configuration register (0x80 | {hromen, lromen, mode})
//    0x41       RAM configuration ({5'b0, RAMmap} on 6128, 0 on 464/664)
//    0x42       CRTC register select (0-31)
//    0x43-0x54  CRTC registers R0-R17 (R16, R17 = 0)
//    0x55       ROM selection (last byte written)
//    0x56       PPI port A inputs (regardless of direction)
//    0x57       PPI port B inputs (tape, jumpers, vsync)
//    0x58       PPI port C outputs
//    0x59       PPI control register (bit 7 forced to 1)
//    0x5A       PSG register select (low nibble 0-15)
//    0x5B-0x6A  PSG registers 0-15
//    0x6B-0x6C  Memory size in KB (little endian: 128 for 6128, 64 for 464/664)
//    0x6D       CPC type (0=464, 1=664, 2=6128)
//    0x6E-0x9B  Unused (set to 0)
//    0x9C       FDD motor drive state (0=off, 1=on)
//    0x9D-0xA0  FDD current physical tracks for drives A-D (C and D = 0)
//    0xA1       Printer data / strobe register
//    0xA2-0xA3  Unused (0)
//    0xA4       CRTC type (0=type 0, 1=type 1)
//    0xA5-0xA8  Unused (0)
//    0xA9       CRTC horizontal character counter (HCC)
//    0xAA       Unused (0)
//    0xAB       CRTC character-line counter (Row / C4)
//    0xAC       CRTC raster-line counter (Line / C9)
//    0xAD       CRTC vertical total adjust counter (type 1: C5; type 0: Line in adj)
//    0xAE       CRTC horizontal sync width counter (HSC)
//    0xAF       CRTC vertical sync width elapsed counter (vsw_elapsed)
//    0xB0-0xB1  CRTC state flags (bit 0: VSYNC_r, bit 1: HSYNC, bit 7: in_adj)
//    0xB2       GA VSYNC delay counter (hcnt 00->2, 01->1, other->0)
//    0xB3       GA interrupt scanline counter (intcnt)
//    0xB4       GA interrupt request pending flag (~INT_N)
//============================================================================

module sna_hw_header
(
	// Gate Array observations
	input  [4:0]   ga_inksel,
	input  [4:0]   ga_border,
	input  [79:0]  ga_inkr,
	input          ga_hromen,
	input          ga_lromen,
	input  [1:0]   ga_mode,
	input  [5:0]   ga_intcnt,
	input  [4:0]   ga_hcnt,
	input          ga_int_n,

	// MMU observations
	input  [2:0]   mmu_rammap,
	input  [7:0]   mmu_rom_select_shadow,

	// CRTC observations
	input  [4:0]   crtc_addr,
	input  [127:0] crtc_regs,
	input  [7:0]   crtc_hcc,
	input  [6:0]   crtc_row,
	input  [4:0]   crtc_line,
	input  [4:0]   crtc_c5,
	input          crtc_in_adj,
	input  [3:0]   crtc_hsc,
	input          crtc_hsync,
	input          crtc_vsync_r,
	input  [3:0]   crtc_vsw_elapsed,

	// PPI observations
	input  [7:0]   ppi_porta_in,
	input  [7:0]   ppi_portb_in,
	input  [7:0]   ppi_opc_r,
	input  [7:0]   ppi_mode,

	// PSG observations
	input  [7:0]   psg_addr,
	input  [127:0] psg_regs,

	// Motherboard / System observations
	input  [7:0]   printer_data,
	input  [1:0]   model,
	input          crtc_type,
	input          fdc_motor,
	input  [7:0]   fdc_pcn_a,
	input  [7:0]   fdc_pcn_b,

	// Packed 135 header bytes: hdr[(o - 8'h2E)*8 +: 8] gives byte at offset o
	output reg [8*135-1:0] hdr
);

	wire unused_bits = &{1'b0, ppi_mode[7], psg_addr[7:4], 1'b0};

	integer i;

	always @(*) begin
		hdr = {1080{1'b0}};

		// 2E: GA pen select
		hdr[(8'h2E - 8'h2E)*8 +: 8] = {3'b000, ga_inksel};

		// 2F-3E: GA palette pens 0-15
		for (i = 0; i < 16; i = i + 1) begin
			hdr[(32'h2F + i - 32'h2E)*8 +: 8] = {3'b000, ga_inkr[i*5 +: 5]};
		end

		// 3F: GA border colour
		hdr[(8'h3F - 8'h2E)*8 +: 8] = {3'b000, ga_border};

		// 40: GA multi-config (bit 7=1, bit 4 strobe saved as 0)
		hdr[(8'h40 - 8'h2E)*8 +: 8] = {1'b1, 2'b00, 1'b0, ga_hromen, ga_lromen, ga_mode};

		// 41: RAM config (6128 map: RAMmap; 464/664: 0)
		hdr[(8'h41 - 8'h2E)*8 +: 8] = (model == 2'd0) ? {5'b00000, mmu_rammap} : 8'h00;

		// 42: CRTC register select
		hdr[(8'h42 - 8'h2E)*8 +: 8] = {3'b000, crtc_addr};

		// 43-52: CRTC registers R0-R15
		for (i = 0; i < 16; i = i + 1) begin
			hdr[(32'h43 + i - 32'h2E)*8 +: 8] = crtc_regs[i*8 +: 8];
		end

		// 53: CRTC register R16 (saved as 0)
		hdr[(8'h53 - 8'h2E)*8 +: 8] = 8'h00;

		// 54: CRTC register R17 (saved as 0)
		hdr[(8'h54 - 8'h2E)*8 +: 8] = 8'h00;

		// 55: ROM select shadow
		hdr[(8'h55 - 8'h2E)*8 +: 8] = mmu_rom_select_shadow;

		// 56: PPI port A inputs
		hdr[(8'h56 - 8'h2E)*8 +: 8] = ppi_porta_in;

		// 57: PPI port B inputs
		hdr[(8'h57 - 8'h2E)*8 +: 8] = ppi_portb_in;

		// 58: PPI port C outputs
		hdr[(8'h58 - 8'h2E)*8 +: 8] = ppi_opc_r;

		// 59: PPI control (bit 7 forced to 1)
		hdr[(8'h59 - 8'h2E)*8 +: 8] = {1'b1, ppi_mode[6:0]};

		// 5A: PSG register select (normalized to low nibble)
		hdr[(8'h5A - 8'h2E)*8 +: 8] = {4'h0, psg_addr[3:0]};

		// 5B-6A: PSG registers 0-15
		for (i = 0; i < 16; i = i + 1) begin
			hdr[(32'h5B + i - 32'h2E)*8 +: 8] = psg_regs[i*8 +: 8];
		end

		// 6B: Memory size low byte (128 for 6128, 64 otherwise)
		hdr[(8'h6B - 8'h2E)*8 +: 8] = (model == 2'd0) ? 8'd128 : 8'd64;

		// 6C: Memory size high byte
		hdr[(8'h6C - 8'h2E)*8 +: 8] = 8'd0;

		// 6D: CPC type (0=464, 1=664, 2=6128)
		hdr[(8'h6D - 8'h2E)*8 +: 8] = (model == 2'd0) ? 8'd2 : (model == 2'd1) ? 8'd1 : 8'd0;

		// 6E-9B: Unused in classic SNA (default 0)

		// 9C: FDD motor drive state (0=off, 1=on)
		hdr[(8'h9C - 8'h2E)*8 +: 8] = {7'b0000000, fdc_motor};

		// 9D: FDD drive A track (pcn[0])
		hdr[(8'h9D - 8'h2E)*8 +: 8] = fdc_pcn_a;

		// 9E: FDD drive B track (pcn[1])
		hdr[(8'h9E - 8'h2E)*8 +: 8] = fdc_pcn_b;

		// 9F: FDD drive C track (absent, 0)
		hdr[(8'h9F - 8'h2E)*8 +: 8] = 8'h00;

		// A0: FDD drive D track (absent, 0)
		hdr[(8'hA0 - 8'h2E)*8 +: 8] = 8'h00;

		// A1: Printer data / strobe register
		hdr[(8'hA1 - 8'h2E)*8 +: 8] = printer_data;

		// A2-A3: Unused (default 0)

		// A4: CRTC type (0=type 0, 1=type 1)
		hdr[(8'hA4 - 8'h2E)*8 +: 8] = {7'b0000000, crtc_type};

		// A5-A8: Unused (default 0)

		// A9: CRTC horizontal character counter (HCC)
		hdr[(8'hA9 - 8'h2E)*8 +: 8] = crtc_hcc;

		// AA: Unused (default 0)

		// AB: CRTC character-line counter (row / C4)
		hdr[(8'hAB - 8'h2E)*8 +: 8] = {1'b0, crtc_row};

		// AC: CRTC raster-line counter (line / C9)
		hdr[(8'hAC - 8'h2E)*8 +: 8] = {3'b000, crtc_line};

		// AD: CRTC vertical total adjust counter (type 1: c5; type 0: line in adj)
		hdr[(8'hAD - 8'h2E)*8 +: 8] = crtc_type ? {3'b000, crtc_c5} : (crtc_in_adj ? {3'b000, crtc_line} : 8'h00);

		// AE: CRTC horizontal sync width counter (hsc)
		hdr[(8'hAE - 8'h2E)*8 +: 8] = {4'h0, crtc_hsc};

		// AF: CRTC vertical sync width elapsed counter (vsw_elapsed)
		hdr[(8'hAF - 8'h2E)*8 +: 8] = {4'h0, crtc_vsw_elapsed};

		// B0: CRTC state flags low (bit 0: VSYNC_r, bit 1: HSYNC, bit 7: in_adj)
		hdr[(8'hB0 - 8'h2E)*8 +: 8] = {crtc_in_adj, 5'b00000, crtc_hsync, crtc_vsync_r};

		// B1: CRTC state flags high (reserved, 0)
		hdr[(8'hB1 - 8'h2E)*8 +: 8] = 8'h00;

		// B2: GA VSYNC delay counter (hcnt 00->2, 01->1, other->0)
		hdr[(8'hB2 - 8'h2E)*8 +: 8] = (ga_hcnt == 5'h00) ? 8'd2 : (ga_hcnt == 5'h01) ? 8'd1 : 8'd0;

		// B3: GA interrupt scanline counter (intcnt)
		hdr[(8'hB3 - 8'h2E)*8 +: 8] = {2'b00, ga_intcnt};

		// B4: GA interrupt request pending flag (~INT_N)
		hdr[(8'hB4 - 8'h2E)*8 +: 8] = {7'b0000000, ~ga_int_n};
	end

endmodule
