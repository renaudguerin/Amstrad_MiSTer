//============================================================================
//  SNA header capture for the selected video / Gate-Array restore fields
//  (B8-5 slice B).
//
//  Amstrad.sv used to decode these header offsets inline. They moved here so
//  the P8 fixture drives the SAME decoder from a real byte stream instead of
//  poking pre-decoded values at the owners: a mis-mapped offset or a stale
//  field now fails in simulation rather than only in Quartus. The Z80, PPI,
//  PSG, RAM-configuration and memory-size bytes stay in Amstrad.sv; they are
//  outside this restore slice.
//
//  Reference: docs/references/Snapshot (.SNA) file format.md
//    v1 header  2E        GA index of selected pen
//               2F-3F     GA current palette (16 pens + border, HW colours)
//               40        GA multi configuration (RMR)
//               42        CRTC index of selected register
//               43-54     CRTC register data R0..R17
//               55        current ROM selection
//    v3 header  10        snapshot version
//               A9        CRTC horizontal character counter   (note 11)
//               AB        CRTC character-line counter         (note 2)
//               AC        CRTC raster-line counter            (note 3)
//               AD        CRTC vertical total adjust counter  (note 4)
//               AE        CRTC horizontal sync width counter  (note 5)
//               AF        CRTC vertical sync width counter    (note 6)
//               B0-B1     CRTC state flags: b0 VSYNC active,
//                         b1 HSYNC active, b7 vertical total adjust active
//                         (notes 7-10; B1 is reserved and ignored)
//               B2        GA vsync delay counter              (note 14)
//               B3        GA interrupt scanline counter       (note 12)
//               B4        interrupt request flag              (note 13)
//
//  The v3 group is gated by `v3`: a v1/v2 file never writes those offsets, so
//  publishing zeroes keeps the owners on their deterministic reset phase
//  instead of applying a field the file does not contain. Every field also
//  clears on each new download, so a CPC+ snapshot cannot leak state into a
//  following plain one (or vice versa).
//
//  AE/AF are documented as 0-16 while the counters they seed are free-running
//  nibbles compared against R3l/R3h. 16 and 0 are the same nibble, so the low
//  four bits are the faithful mapping.
//============================================================================

module plus_sna_header
(
	input          clk,
	input          sna_download,

	// Header byte stream: `wr` strobes one byte of the first 256 bytes of the
	// snapshot file, `addr` is its file offset.
	input          wr,
	input    [7:0] addr,
	input    [7:0] data,

	output   [7:0] version,
	output         v3,

	// v1 fields (always published)
	output   [4:0] ga_inksel,
	output [135:0] ga_palette,   // pen k at [k*8 +: 8], border at [128 +: 8]
	output   [7:0] ga_config,
	output   [4:0] crtc_addr,
	output [143:0] crtc_regs,    // Rn at [n*8 +: 8]
	output   [7:0] rom_select,

	// v3 fields (zero unless version == 3)
	output   [7:0] crtc_hcc,
	output   [6:0] crtc_line,
	output   [4:0] crtc_raster,
	output   [4:0] crtc_vta,
	output   [3:0] crtc_hsw,
	output   [3:0] crtc_vsw,
	output         crtc_vs,
	output         crtc_hs,
	output         crtc_adj,
	output   [1:0] ga_vsdelay,
	output   [5:0] ga_intcnt,
	output         int_pending
);

	reg   [7:0] version_r;
	reg   [4:0] ga_inksel_r;
	reg [135:0] ga_palette_r;
	reg   [7:0] ga_config_r;
	reg   [4:0] crtc_addr_r;
	reg [143:0] crtc_regs_r;
	reg   [7:0] rom_select_r;
	// Each v3 field is stored at its documented width, not as a raw byte: the
	// unused high bits are reserved/zero in the format and keeping them would
	// only be dead state.
	reg   [7:0] crtc_hcc_r;    // A9: 0-255
	reg   [6:0] crtc_line_r;   // AB: 0-127
	reg   [4:0] crtc_raster_r; // AC: 0-31
	reg   [4:0] crtc_vta_r;    // AD: 0-31
	reg   [3:0] crtc_hsw_r;    // AE: 0-16, wrapping nibble (16 == 0)
	reg   [3:0] crtc_vsw_r;    // AF: 0-16, wrapping nibble
	reg         b0_vs_r;       // B0 bit 0
	reg         b0_hs_r;       // B0 bit 1
	reg         b0_adj_r;      // B0 bit 7
	reg   [1:0] ga_vsdelay_r;  // B2: 0-2
	reg   [5:0] ga_intcnt_r;   // B3: 0-51
	reg         int_req_r;     // B4

	reg         sna_download_d;

	assign version    = version_r;
	assign v3         = (version_r == 8'd3);

	assign ga_inksel  = ga_inksel_r;
	assign ga_palette = ga_palette_r;
	assign ga_config  = ga_config_r;
	assign crtc_addr  = crtc_addr_r;
	assign crtc_regs  = crtc_regs_r;
	assign rom_select = rom_select_r;

	assign crtc_hcc    = v3 ? crtc_hcc_r    : 8'd0;
	assign crtc_line   = v3 ? crtc_line_r   : 7'd0;
	assign crtc_raster = v3 ? crtc_raster_r : 5'd0;
	assign crtc_vta    = v3 ? crtc_vta_r    : 5'd0;
	assign crtc_hsw    = v3 ? crtc_hsw_r    : 4'd0;
	assign crtc_vsw    = v3 ? crtc_vsw_r    : 4'd0;
	assign crtc_vs     = v3 ? b0_vs_r       : 1'b0;
	assign crtc_hs     = v3 ? b0_hs_r       : 1'b0;
	assign crtc_adj    = v3 ? b0_adj_r      : 1'b0;
	assign ga_vsdelay  = v3 ? ga_vsdelay_r  : 2'd0;
	assign ga_intcnt   = v3 ? ga_intcnt_r   : 6'd0;
	assign int_pending = v3 ? int_req_r     : 1'b0;

	always @(posedge clk) begin
		sna_download_d <= sna_download;

		// Every field clears on each new download, before any byte of the new
		// file is decoded.
		if (sna_download && !sna_download_d) begin
			version_r     <= 8'd0;
			ga_inksel_r   <= 5'd0;
			ga_palette_r  <= 136'd0;
			ga_config_r   <= 8'd0;
			crtc_addr_r   <= 5'd0;
			crtc_regs_r   <= 144'd0;
			rom_select_r  <= 8'd0;
			crtc_hcc_r    <= 8'd0;
			crtc_line_r   <= 7'd0;
			crtc_raster_r <= 5'd0;
			crtc_vta_r    <= 5'd0;
			crtc_hsw_r    <= 4'd0;
			crtc_vsw_r    <= 4'd0;
			b0_vs_r       <= 1'b0;
			b0_hs_r       <= 1'b0;
			b0_adj_r      <= 1'b0;
			ga_vsdelay_r  <= 2'd0;
			ga_intcnt_r   <= 6'd0;
			int_req_r     <= 1'b0;
		end
		else if (wr) begin
			case (addr)
			8'h10: version_r     <= data;
			8'h2e: ga_inksel_r   <= data[4:0];
			8'h40: ga_config_r   <= data;
			8'h42: crtc_addr_r   <= data[4:0];
			8'h55: rom_select_r  <= data;
			8'ha9: crtc_hcc_r    <= data;
			8'hab: crtc_line_r   <= data[6:0];
			8'hac: crtc_raster_r <= data[4:0];
			8'had: crtc_vta_r    <= data[4:0];
			8'hae: crtc_hsw_r    <= data[3:0];
			8'haf: crtc_vsw_r    <= data[3:0];
			8'hb0: {b0_adj_r, b0_hs_r, b0_vs_r} <= {data[7], data[1], data[0]};
			8'hb2: ga_vsdelay_r  <= data[1:0];
			8'hb3: ga_intcnt_r   <= data[5:0];
			8'hb4: int_req_r     <= data[0];
			default: ;
			endcase

			if (addr >= 8'h2f && addr <= 8'h3f)
				ga_palette_r[((addr - 8'h2f) * 8) +: 8] <= data;
			if (addr >= 8'h43 && addr <= 8'h54)
				crtc_regs_r[((addr - 8'h43) * 8) +: 8] <= data;
		end
	end

	initial begin
		version_r      = 8'd0;
		ga_inksel_r    = 5'd0;
		ga_palette_r   = 136'd0;
		ga_config_r    = 8'd0;
		crtc_addr_r    = 5'd0;
		crtc_regs_r    = 144'd0;
		rom_select_r   = 8'd0;
		crtc_hcc_r     = 8'd0;
		crtc_line_r    = 7'd0;
		crtc_raster_r  = 5'd0;
		crtc_vta_r     = 5'd0;
		crtc_hsw_r     = 4'd0;
		crtc_vsw_r     = 4'd0;
		b0_vs_r        = 1'b0;
		b0_hs_r        = 1'b0;
		b0_adj_r       = 1'b0;
		ga_vsdelay_r   = 2'd0;
		ga_intcnt_r    = 6'd0;
		int_req_r      = 1'b0;
		sna_download_d = 1'b0;
	end

endmodule
