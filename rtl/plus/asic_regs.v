//============================================================================
//  Amstrad Plus ASIC register page (&4000-&7FFF when enabled) — P2.
//
//  Backs the ASIC page captured by plus_mmu (RMR2 D4D3=11, unlock-gated):
//  sprite pixel RAM, sprite position/magnification registers, the 32×12-bit
//  palette with its secondary legacy PENR/INKR translation, the raster/
//  interrupt/DMA register bytes later phases consume, and the documented
//  read-back rules (masks, mirrors, open bus).
//
//  Sources: docs/plus/references/asic-reference.md §2-§6 ([ARNOLD App.1],
//  [ARNOLD-REV], [KT], [QUASAR]); sections cited at each rule below.
//
//  Bus contract: the caller asserts asic_cs for memory accesses inside the
//  enabled page; D_out participates in the motherboard's wired-AND CPU data
//  mux and is therefore HIGH-NEUTRAL — undriven/unmapped reads present 8'hFF
//  (the explicit open-bus response, reference §4: unmapped areas read the
//  instruction byte on real hardware; this core has no instruction
//  visibility, so it contributes the neutral level instead, which the
//  wired-AND renders identically once every other source also abstains).
//
//  No write-through: writes land only in the registers/RAM here. Reaching
//  the "writes do not hit RAM underneath" system guarantee (reference §2) is
//  the caller's job (main-memory cycle suppression when asic_page_on), and
//  is pinned by the motherboard-level bench, not by this leaf.
//
//  Deliberate scope notes (each tagged where implemented):
//   - Magnification write mirror on offset +3: [ARNOLD-REV] and [KT]
//     conflict; the reference instructs treating +4..+7 as magnification
//     mirrors and leaves +3's write behaviour to hardware verification.
//     +3 here stores the Y-high byte (the reading that keeps Y intact).
//     ⚠ ASIC-REF §4
//   - ADC (&6808-&680F) and DMA bus behaviour are later phases; their
//     regions follow the unmapped rule until landed (DMA register BYTES
//     are stored now so P7 needs no back-channel).
//   - Power-up contents: sprite RAM, palette and position registers are
//     defined-zero here; the reference marks most of them N (undefined),
//     and the border specifically undefined. Zero matches Verilator and
//     FPGA init; named model assumption like asic_ga_timing's INKR reset.
//
//  This module implements no CRTC behaviour, so no Compendium attribution
//  applies; the legacy-colour translation table carries the same [KT]
//  provenance as asic_video's copy (entries cross-checked against the
//  ga40010 DAC equations during P1 extraction).
//============================================================================

module asic_regs
(
	input        clk,
	input        reset,

	// CPU-side memory bus into the enabled page
	input        asic_cs,      // A[15:14]==01, plus_mode & asic_page_on & mem access
	input        mem_wr,
	input        mem_rd,
	input [13:0] A,            // page offset (&0000 = &4000)
	input  [7:0] D_in,

	// Wired-AND-neutral read data: 1s wherever this module does not answer
	output [7:0] D_out,

	// Legacy Gate Array register shadow (asic_ga_timing outputs); changes
	// are translated into palette entries 0-16 through the fixed [KT] table
	// (reference §6, secondary port).
	input  [4:0]  leg_border,
	input [79:0]  leg_inkr,     // entry k at [k*5 +: 5]

	// Video-side palette read port (free-running, no CPU interference —
	// reference §6 "dual-ported").
	input  [4:0]  pal_raddr,
	output [11:0] pal_rdata,   // {R,G,B} nibbles

	// Register bytes later phases consume (stored from P2 on)
	output [7:0] pri, splt, sscr, ivr, ssa_hi, ssa_lo, dcsr,

	// Interrupt-merger interface (P3): DCSR bit 7 tracks "last INT ack
	// was raster" — the merger owns the persistent level (set on a raster
	// acknowledge, cleared by the next non-raster acknowledge) and this
	// module only reflects it.
	input        intack_raster
);

	//------------------------------------------------------------------
	// Storage
	//------------------------------------------------------------------

	// Sprite pixel RAM: 16 sprites × 256 bytes, low nibble used (§3/§5).
	// No reset branch: contents are undefined at POR per the reference;
	// FPGA power-up init provides the defined-zero model assumption.
	reg [3:0] spr_ram [0:4095];

	// Sprite position/magnification: raw written fields kept for the exact
	// read-back rules (§4). spr_mag is stored from P2 for the P4 sprite
	// engine, which consumes it directly.
	/* verilator lint_off UNUSEDSIGNAL */
	reg [7:0] spr_x_lo [0:15];
	reg [1:0] spr_x_hi [0:15];
	reg [7:0] spr_y_lo [0:15];
	reg       spr_y_hi [0:15];
	reg [7:0] spr_mag  [0:15];
	/* verilator lint_on UNUSEDSIGNAL */

	// Palette: 32 entries × 12 bits {R,G,B} (§6).
	reg [11:0] pal [0:31];
	reg [11:0] pal_r;

	// Raster/interrupt/DMA register bytes (§3; behaviour lands P3/P6/P7).
	reg [7:0] pri_r, splt_r, sscr_r, ivr_r, ssa_hi_r, ssa_lo_r;
	// DCSR fields (reference §9): bit7 read-only status driven by the
	// interrupt merger; bits 6:4 DMA flags write-1-to-clear; bits 2:0
	// channel enables plain R/W. intack_dma is the merger's non-raster
	// status sampled for reads.
	// dcsr_stat is the merger's persistent last-ack-was-raster level.
	reg       dcsr_stat;
	reg [2:0] dcsr_flags;
	reg [2:0] dcsr_ena;
	// SAR/PPR bytes are stored from P2 so P7 needs no back-channel; the
	// DMA engine consumes them directly.
	/* verilator lint_off UNUSEDSIGNAL */
	reg [7:0] sar_lo [0:2];
	reg [7:0] sar_hi [0:2];
	reg [7:0] ppr    [0:2];
	/* verilator lint_on UNUSEDSIGNAL */

	assign pri    = pri_r;
	assign splt   = splt_r;
	assign sscr   = sscr_r;
	assign ivr    = ivr_r;
	assign ssa_hi = ssa_hi_r;
	assign ssa_lo = ssa_lo_r;
	assign dcsr = {dcsr_stat | intack_raster, dcsr_flags, 1'b0, dcsr_ena};

	//------------------------------------------------------------------
	// Legacy colour translation (reference §6): fixed ROM table, [KT]
	// measured values; same data as asic_video's legacy_colour but stored
	// directly in the palette's documented {G,R,B} word order. Written as
	// plain case assignments — no function-local temporaries — because
	// older Verilator (5.020) flags inlined blocking assignments (BLKSEQ).
	function [11:0] legacy_colour_gbr(input [4:0] hw);
		begin
			case (hw)
				5'd00: legacy_colour_gbr = {4'd6,  4'd6,  4'd6 };
				5'd01: legacy_colour_gbr = {4'd6,  4'd6,  4'd6 };
				5'd02: legacy_colour_gbr = {4'd15, 4'd0,  4'd6 };
				5'd03: legacy_colour_gbr = {4'd15, 4'd15, 4'd6 };
				5'd04: legacy_colour_gbr = {4'd0,  4'd0,  4'd6 };
				5'd05: legacy_colour_gbr = {4'd0,  4'd15, 4'd6 };
				5'd06: legacy_colour_gbr = {4'd6,  4'd0,  4'd6 };
				5'd07: legacy_colour_gbr = {4'd6,  4'd15, 4'd6 };
				5'd08: legacy_colour_gbr = {4'd0,  4'd15, 4'd6 };
				5'd09: legacy_colour_gbr = {4'd15, 4'd15, 4'd6 };
				5'd10: legacy_colour_gbr = {4'd15, 4'd15, 4'd0 };
				5'd11: legacy_colour_gbr = {4'd15, 4'd15, 4'd15};
				5'd12: legacy_colour_gbr = {4'd0,  4'd15, 4'd0 };
				5'd13: legacy_colour_gbr = {4'd15, 4'd15, 4'd0 };
				5'd14: legacy_colour_gbr = {4'd6,  4'd15, 4'd0 };
				5'd15: legacy_colour_gbr = {4'd6,  4'd15, 4'd15};
				5'd16: legacy_colour_gbr = {4'd0,  4'd0,  4'd6 };
				5'd17: legacy_colour_gbr = {4'd15, 4'd0,  4'd6 };
				5'd18: legacy_colour_gbr = {4'd15, 4'd0,  4'd0 };
				5'd19: legacy_colour_gbr = {4'd15, 4'd0,  4'd15};
				5'd20: legacy_colour_gbr = {4'd0,  4'd0,  4'd0 };
				5'd21: legacy_colour_gbr = {4'd0,  4'd0,  4'd15};
				5'd22: legacy_colour_gbr = {4'd6,  4'd0,  4'd0 };
				5'd23: legacy_colour_gbr = {4'd6,  4'd0,  4'd15};
				5'd24: legacy_colour_gbr = {4'd0,  4'd6,  4'd6 };
				5'd25: legacy_colour_gbr = {4'd15, 4'd6,  4'd6 };
				5'd26: legacy_colour_gbr = {4'd15, 4'd6,  4'd0 };
				5'd27: legacy_colour_gbr = {4'd15, 4'd6,  4'd15};
				5'd28: legacy_colour_gbr = {4'd0,  4'd6,  4'd0 };
				5'd29: legacy_colour_gbr = {4'd0,  4'd6,  4'd15};
				5'd30: legacy_colour_gbr = {4'd6,  4'd6,  4'd0 };
				5'd31: legacy_colour_gbr = {4'd6,  4'd6,  4'd15};
				default: legacy_colour_gbr = 12'h000;
			endcase
		end
	endfunction

	reg [79:0] leg_inkr_q;
	reg [4:0]  leg_border_q;
	integer k;

	//------------------------------------------------------------------
	// Address decode helpers (page offsets; &4000 stripped)
	//   &4000-&4FFF  sprite pixel RAM          (wsel 00)
	//   &5000-&5FFF  unused                    (wsel 01)
	//   &6000-&607F  sprite registers          (wsel 10, A[11:7]==0)
	//   &6400-&643F  palette                   (wsel 10, A[11:6]==6'h10)
	//   &6800-&680F  raster/interrupt bytes    (wsel 10, A[11:4]==8'h80)
	//   &6C00-&6C0F  DMA registers             (wsel 10, A[11:4]==8'hC0)
	//   rest         unused / unmapped         (open bus on read)
	//------------------------------------------------------------------

	wire [1:0] wsel = A[13:12];
	wire       r_sprreg = (wsel == 2'b10) && (A[11:7] == 5'd0);
	wire       r_pal    = (wsel == 2'b10) && (A[11:6] == 6'h10);
	wire       r_raster = (wsel == 2'b10) && (A[11:4] == 8'h80);
	wire       r_dma    = (wsel == 2'b10) && (A[11:4] == 8'hC0);

	// Palette entry index: 64 bytes / 2 bytes per entry over &6400-&643F.
	wire [4:0] pal_idx = A[5:1];

	always @(posedge clk) begin
		if (reset) begin
			pri_r    <= 8'd0;   // POR=0            (§7)
			splt_r   <= 8'd0;   // POR=0            (§3)
			sscr_r   <= 8'd0;   // POR=0            (§3)
			ivr_r    <= 8'b00000001; // bit0=1 at reset (§3/§7)
			ssa_hi_r <= 8'd0;
			ssa_lo_r <= 8'd0;
			dcsr_stat  <= 1'b0;
			dcsr_flags <= 3'd0;
			dcsr_ena   <= 3'd0;
			sar_lo[0]<= 8'd0; sar_hi[0] <= 8'd0; ppr[0] <= 8'd0;
			sar_lo[1]<= 8'd0; sar_hi[1] <= 8'd0; ppr[1] <= 8'd0;
			sar_lo[2]<= 8'd0; sar_hi[2] <= 8'd0; ppr[2] <= 8'd0;
			for (k = 0; k < 16; k = k + 1) begin
				spr_x_lo[k] <= 8'd0;
				spr_x_hi[k] <= 2'd0;
				spr_y_lo[k] <= 8'd0;
				spr_y_hi[k] <= 1'b0;
				spr_mag[k]  <= 8'd0; // magnification cleared at reset (§5)
			end
			for (k = 0; k < 32; k = k + 1) pal[k] <= 12'd0;
			leg_inkr_q   <= {80{1'b1}}; // != reset INKR: forces first translate
			leg_border_q <= 5'b11111;   // != reset border 16
		end
		if (asic_cs) begin
			if (mem_wr) begin
				if (wsel == 2'b00) begin
					// Sprite pixel data, masked to the low nibble (§3/§4).
					spr_ram[A[11:0]] <= D_in[3:0];
				end
				else if (wsel == 2'b10) begin
					if (r_sprreg) begin
						case (A[2:0])
						3'd0: spr_x_lo[A[6:3]] <= D_in;
						3'd1: spr_x_hi[A[6:3]] <= D_in[1:0]; // 10-bit signed (§4)
						3'd2: spr_y_lo[A[6:3]] <= D_in;
						// +3 stores Y-high; the magnification write-mirror
						// claim for +3 is the unresolved ⚠ ASIC-REF §4 note.
						3'd3: spr_y_hi[A[6:3]] <= D_in[0];   // 9-bit signed (§4)
						default: spr_mag[A[6:3]] <= D_in;    // +4..+7 mirrors (§4)
						endcase
					end
					else if (r_pal) begin
						// Low byte: D7-D4 RED, D3-D0 BLUE; high byte:
						// D3-D0 GREEN, D7-D4 unused-reads-0 (§6). Stored
						// in the documented word layout {G,R,B}.
						if (!A[0])
							pal[pal_idx] <= {pal[pal_idx][11:8], D_in[7:4], D_in[3:0]};
						else
							pal[pal_idx] <= {D_in[3:0], pal[pal_idx][7:4], pal[pal_idx][3:0]};
					end
					else if (r_raster) begin
						case (A[3:0])
						4'h0: pri_r    <= D_in;
						4'h1: splt_r   <= D_in;
						4'h2: ssa_hi_r <= D_in;
						4'h3: ssa_lo_r <= D_in;
						4'h4: sscr_r   <= D_in;
						4'h5: ivr_r    <= D_in;
						default: ; // &6806/&6807: writes have no effect (§3)
						endcase
					end
					else if (r_dma) begin
						case (A[3:0])
						4'h0: sar_lo[0] <= D_in;
						4'h1: sar_hi[0] <= D_in;
						4'h2: ppr[0]    <= D_in;
						4'h4: sar_lo[1] <= D_in;
						4'h5: sar_hi[1] <= D_in;
						4'h6: ppr[1]    <= D_in;
						4'h8: sar_lo[2] <= D_in;
						4'h9: sar_hi[2] <= D_in;
						4'hA: ppr[2]    <= D_in;
						4'hF: begin // writable ONLY at &6C0F (reference §4)
							dcsr_flags <= dcsr_flags & ~D_in[6:4]; // w1c (§9)
							dcsr_ena   <= D_in[2:0];
						end // bit7 is merger-driven, not CPU-writable
						default: ; // &6C03/&6C0B-&6C0E unused (§3)
						endcase
					end
				end
				// wsel 01/11 (&5000s / &7000s): writes ignored (§3)
			end

			// Legacy PENR/INKR translation (§6): pens 0-15 + border only.
			if (leg_inkr != leg_inkr_q || leg_border != leg_border_q) begin
				for (k = 0; k < 16; k = k + 1)
					if (leg_inkr[k*5 +: 5] != leg_inkr_q[k*5 +: 5])
						pal[k] <= legacy_colour_gbr(leg_inkr[k*5 +: 5]);
				if (leg_border != leg_border_q)
					pal[16] <= legacy_colour_gbr(leg_border);
				leg_inkr_q   <= leg_inkr;
				leg_border_q <= leg_border;
			end
		end
	end

	//------------------------------------------------------------------
	// Reads (reference §4 rules; wired-AND neutral otherwise)
	//------------------------------------------------------------------

	reg [7:0] rdata;
	reg       renable; // 0 = unmapped/write-only/open bus -> contribute FF

	// Sprite-position read rules (§4):
	//   X high byte: written&3==3 reads &FF, else written&3
	//   Y high byte: written&1==1 reads &FF, else written&1
	function [7:0] xhi_read(input [1:0] v);
		xhi_read = (v == 2'b11) ? 8'hFF : {6'd0, v};
	endfunction
	function [7:0] yhi_read(input v);
		yhi_read = v ? 8'hFF : 8'd0;
	endfunction

	always @(*) begin
		rdata   = 8'hFF;
		renable = 1'b0;
		if (asic_cs && mem_rd) begin
			if (wsel == 2'b00) begin
				// Sprite pixel RAM reads return written & &0F (§4).
				rdata   = {4'h0, spr_ram[A[11:0]]};
				renable = 1'b1;
			end
			else if (wsel == 2'b10) begin
				if (r_sprreg) begin
					case (A[2:0])
					// Reading +4..+7 mirrors +0..+3 respectively (§4);
					// magnification itself is write-only (§4).
					3'd0, 3'd4: begin rdata = spr_x_lo[A[6:3]]; renable = 1'b1; end
					3'd1, 3'd5: begin rdata = xhi_read(spr_x_hi[A[6:3]]); renable = 1'b1; end
					3'd2, 3'd6: begin rdata = spr_y_lo[A[6:3]]; renable = 1'b1; end
					3'd3, 3'd7: begin rdata = yhi_read(spr_y_hi[A[6:3]]); renable = 1'b1; end
					endcase
				end
				else if (r_pal) begin
					// Even byte: D7-D4 RED, D3-D0 BLUE; odd byte: D3-D0
					// GREEN with the top nibble reading 0 (§6).
					rdata   = A[0] ? {4'h0, pal[pal_idx][11:8]}
					               : {pal[pal_idx][7:4], pal[pal_idx][3:0]};
					renable = 1'b1;
				end
				else if (r_dma) begin
					// DCSR readable across the whole &6C00-&6C0F range;
					// SAR/PPR are not readable (§4).
					rdata   = {dcsr_stat | intack_raster, dcsr_flags, 1'b0, dcsr_ena};
					renable = 1'b1;
				end
				// &6800-&6807 are write-only: reads fall through to open
				// bus (§4). ADC lands with its phase.
			end
			// wsel 01/11: unmapped, open bus (§4)
		end
	end

	assign D_out = rdata | {8{~renable}};
	assign pal_rdata = pal_r;

	// Video-side read port (registered, independent of the CPU port).
	// Own reset here: a second always writing this reg must carry the
	// reset itself or Quartus sees two constant drivers (error 10028).
	always @(posedge clk) begin
		if (reset) pal_r <= 12'd0;
		else       pal_r <= pal[pal_raddr];
	end

	// Quartus maps these synthesizable initial values to FPGA power-up
	// state; the reset branch above defines the simulated values. The
	// undefined-at-POR reference fields are a named zero assumption (header).
	initial begin
		pri_r = 8'd0; splt_r = 8'd0; sscr_r = 8'd0;
		ivr_r = 8'b00000001; ssa_hi_r = 8'd0; ssa_lo_r = 8'd0;
		dcsr_stat = 1'b0; dcsr_flags = 3'd0; dcsr_ena = 3'd0;
		leg_inkr_q = {80{1'b1}};
		leg_border_q = 5'b11111;
		pal_r = 12'd0;
	end

endmodule
