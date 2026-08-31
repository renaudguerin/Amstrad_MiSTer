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
	output [11:0] pal_rdata,   // palette word {G,R,B} nibbles

	// Register bytes later phases consume (stored from P2 on)
	output [7:0] pri, splt, sscr, ivr, ssa_hi, ssa_lo, dcsr,

	// Interrupt-merger interface (P3): DCSR bit 7 tracks "last INT ack
	// was raster" — the merger owns the persistent level (set on a raster
	// acknowledge, cleared by the next non-raster acknowledge) and this
	// module only reflects it.
	input        intack_raster,

	// Interrupt-vector supply (reference §7): on every INT acknowledge the
	// ASIC drives (IVR & &F8) | source onto the data bus, raster highest
	// priority. DMA sources land with P7; until then the source field is
	// raster (%110) whenever a raster interrupt is pending — the no-
	// pending behaviour is unspecified on hardware (named assumption).
	input        intack,        // M1 & IORQ cycle in progress
	input        int_pending,   // raster interrupt asserted (INT_N low)
	output [7:0] vec_byte,
	output       vec_valid,     // high while the vector occupies the bus

	// Per-channel DMA interrupt request lines (reference section 9): the
	// P7 DMA engine asserts these on an INT instruction; they OR-set the
	// DCSR flag bits here so write-one-to-clear is observable now rather
	// than arriving implicit with P7. Tied low until P7 lands.
	input  [2:0] dma_int_set,  // {ch2, ch1, ch0}

	// ---- P4 video-side sprite services ----
	// Row-fetch port into spr_ram for the sprite engine (P4): a
	// registered second read, preempted whenever the CPU port owns a
	// page read cycle. ADDR = {sprite[3:0], row[3:0], byte[2:0]};
	// DATA returns that byte position's two consecutive nibbles
	// {odd nibble, even nibble}, ACK pulses one clock after each
	// granted edge.
	input               sprq_req,
	input        [10:0] sprq_addr,
	output       [7:0]  sprq_data,
	output reg          sprq_ack,

	// Pixel-data access indicator (reference §5 blanking side effect):
	// asserted while the CPU reads OR writes any byte of a sprite's
	// 256-byte image area; IDX identifies the sprite. Register-region
	// accesses (&6000s) never assert it — writes there do not blank.
	output              spr_acc_en,
	output       [3:0]  spr_acc_idx,
	output              spr_wr_en,
	output       [11:0] spr_wr_addr,
	output       [3:0]  spr_wr_data,

	// Live attribute view for the sprite engine (§3/§4 storage).
	output [159:0] spr_x_view,   // sprite n X[9:0] at [n*10 +: 10]
	output [143:0] spr_y_view,   // sprite n Y[8:0] at [n*9 +: 9]
	output  [63:0] spr_mag_view, // sprite n magnification at [n*4 +: 4]

	// Sprite colour entries 17..31 (colour c at [(c-1)*12 +: 12], §5/§6).
	output [179:0] spr_pal_view,

	// ---- P7 DMA Sound Engine channels ----
	output [7:0] sar0_lo, output [7:0] sar0_hi, output [7:0] ppr0, output sar0_wr,
	output [7:0] sar1_lo, output [7:0] sar1_hi, output [7:0] ppr1, output sar1_wr,
	output [7:0] sar2_lo, output [7:0] sar2_hi, output [7:0] ppr2, output sar2_wr,
	output [2:0] dcsr_ena_out,
	input  [2:0] dcsr_ena_clr,
	output       dma_int_req,

	// ---- P8 SNA v3 snapshot loading ----
	input        sna_wr,
	input [13:0] sna_addr,
	input  [7:0] sna_data
);

	//------------------------------------------------------------------
	// Storage
	//------------------------------------------------------------------

	// Sprite pixel RAM: 16 sprites × 256 bytes, low nibble used (§3/§5).
	// Dedicated M10K-compatible dual-port module (plus_sprite_ram).
	wire [3:0] spr_host_rdata;
	wire [7:0] spr_video_rdata;

	// Sprite attribute registers (§3/§4):
	//   &6000 + 8*n: X low  8 bits
	//   &6001 + 8*n: X high 2 bits (bits 1:0; top 6 bits ignored/zero)
	//   &6002 + 8*n: Y low  8 bits
	//   &6003 + 8*n: Y high 1 bit  (bit 0; top 7 bits ignored/zero)
	//   &6004 + 8*n: magnification (bits 3:0; top 4 bits ignored/zero)
	// &6005-&6007: unused per-sprite offsets (writes ignored, open bus)
	reg [7:0] spr_x_lo [0:15];
	reg [1:0] spr_x_hi [0:15];
	reg [7:0] spr_y_lo [0:15];
	reg       spr_y_hi [0:15];
	reg [7:0] spr_mag  [0:15];

	// Palette entries 0..31 (§6): 12-bit RGB {G[3:0], R[3:0], B[3:0]}
	//   even byte (&6400+2*c): {R[3:0], B[3:0]}
	//   odd  byte (&6401+2*c): {4'b0,   G[3:0]}
	// Initialized to zero (black) by POR (§6).
	reg [11:0] pal [0:31];
	reg [11:0] pal_r;

	// Control / status registers (§3/§7)
	reg [7:0] pri_r;
	reg [7:0] splt_r;
	reg [7:0] sscr_r;
	reg [7:0] ivr_r;
	reg [7:0] ssa_hi_r;
	reg [7:0] ssa_lo_r;
	reg       dcsr_stat;
	reg [2:0] dcsr_flags; // [0]=ch0, [1]=ch1, [2]=ch2
	reg [2:0] dcsr_ena;   // [0]=ch0, [1]=ch1, [2]=ch2

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
	// DCSR: bit 7 = last-raster, bit 6 = ch0 INT, bit 5 = ch1 INT, bit 4 = ch2 INT, bits 2:0 = enables
	assign dcsr   = {dcsr_stat | intack_raster, dcsr_flags[0], dcsr_flags[1], dcsr_flags[2], 1'b0, dcsr_ena};

	wire        eff_cs   = (asic_cs && mem_wr) || sna_wr;
	wire [13:0] eff_addr = sna_wr ? sna_addr : A[13:0];
	wire  [7:0] eff_data = sna_wr ? sna_data : D_in;
	wire  [1:0] eff_wsel = eff_addr[13:12];

	assign sar0_wr = !reset && eff_cs && (eff_wsel == 2'b10) && (eff_addr[11:4] == 8'hC0) && (eff_addr[3:0] == 4'h0 || eff_addr[3:0] == 4'h1);
	assign sar1_wr = !reset && eff_cs && (eff_wsel == 2'b10) && (eff_addr[11:4] == 8'hC0) && (eff_addr[3:0] == 4'h4 || eff_addr[3:0] == 4'h5);
	assign sar2_wr = !reset && eff_cs && (eff_wsel == 2'b10) && (eff_addr[11:4] == 8'hC0) && (eff_addr[3:0] == 4'h8 || eff_addr[3:0] == 4'h9);

	wire [7:0] next_sar0_lo = (sar0_wr && eff_addr[3:0] == 4'h0) ? eff_data : sar_lo[0];
	wire [7:0] next_sar0_hi = (sar0_wr && eff_addr[3:0] == 4'h1) ? eff_data : sar_hi[0];
	wire [7:0] next_sar1_lo = (sar1_wr && eff_addr[3:0] == 4'h4) ? eff_data : sar_lo[1];
	wire [7:0] next_sar1_hi = (sar1_wr && eff_addr[3:0] == 4'h5) ? eff_data : sar_hi[1];
	wire [7:0] next_sar2_lo = (sar2_wr && eff_addr[3:0] == 4'h8) ? eff_data : sar_lo[2];
	wire [7:0] next_sar2_hi = (sar2_wr && eff_addr[3:0] == 4'h9) ? eff_data : sar_hi[2];

	assign sar0_lo = next_sar0_lo;
	assign sar0_hi = next_sar0_hi;
	assign ppr0    = ppr[0];
	assign sar1_lo = next_sar1_lo;
	assign sar1_hi = next_sar1_hi;
	assign ppr1    = ppr[1];
	assign sar2_lo = next_sar2_lo;
	assign sar2_hi = next_sar2_hi;
	assign ppr2    = ppr[2];
	assign dcsr_ena_out = dcsr_ena;
	assign dma_int_req  = |dcsr_flags;

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

	wire eff_r_sprreg = (eff_wsel == 2'b10) && (eff_addr[11:7] == 5'd0);
	wire eff_r_pal    = (eff_wsel == 2'b10) && (eff_addr[11:6] == 6'h10);
	wire eff_r_raster = (eff_wsel == 2'b10) && (eff_addr[11:4] == 8'h80);
	wire eff_r_dma    = (eff_wsel == 2'b10) && (eff_addr[11:4] == 8'hC0);

	// DCSR flag write-one-to-clear window: a &6C0F write while the page is
	// selected (reference section 9).
	// DCSR bit 6 = ch0, bit 5 = ch1, bit 4 = ch2
	wire       dcsr_w1c_hit = !reset && asic_cs && mem_wr && !sna_wr &&
	                          (wsel == 2'b10) && (A[11:4] == 8'hC0) &&
	                          (A[3:0] == 4'hF);
	wire       dcsr_sna_hit = !reset && sna_wr &&
	                          (sna_addr[13:12] == 2'b10) && (sna_addr[11:4] == 8'hC0) &&
	                          (sna_addr[3:0] == 4'hF);
	wire [2:0] dcsr_w1c_mask = {D_in[4], D_in[5], D_in[6]}; // [2]=ch2, [1]=ch1, [0]=ch0
	wire       auto_clr_dma = intack && !intack_d && !ivr_r[0] && !int_pending;
	wire [2:0] auto_clr_mask = (dcsr_flags[2]) ? 3'b100 :
	                           (dcsr_flags[1]) ? 3'b010 :
	                           (dcsr_flags[0]) ? 3'b001 : 3'b000;

	// Palette entry index: 64 bytes / 2 bytes per entry over &6400-&643F.
	wire [4:0] pal_idx     = A[5:1];
	wire [4:0] eff_pal_idx = eff_addr[5:1];

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
		// Reset dominates page writes: this used to be an else-if until the
		// legacy-translate hoist split the chain (review part-B blocker 1) -
		// a write cycle coinciding with reset could otherwise override
		// reset-defined fields such as IVR bit 0.
		// DCSR flag bits: DMA INT requests set them on any clock edge;
		// a &6C0F write clears by ones. A simultaneous set-and-clear
		// resolves set-dominant (no arbitration rule in the sources).
		// Gated by !reset so reset dominates (review part-B blocker 1).
		if (!reset) begin
			if (dcsr_sna_hit) begin
				dcsr_flags <= {sna_data[4], sna_data[5], sna_data[6]}; // bit 6=ch0, bit 5=ch1, bit 4=ch2
				dcsr_ena   <= sna_data[2:0];
				dcsr_stat  <= sna_data[7];
			end
			else begin
				dcsr_flags <= ((dcsr_flags | dma_int_set) &
				              (dcsr_w1c_hit ? ~dcsr_w1c_mask : 3'b111)) &
				              (auto_clr_dma ? ~auto_clr_mask : 3'b111);
				if (dcsr_w1c_hit)
					dcsr_ena <= (D_in[2:0] & ~dcsr_ena_clr);
				else
					dcsr_ena <= (dcsr_ena & ~dcsr_ena_clr);
			end
		end

		if (!reset && eff_cs) begin
			if (eff_wsel == 2'b10) begin
				if (eff_r_sprreg) begin
					case (eff_addr[2:0])
					3'd0: spr_x_lo[eff_addr[6:3]] <= eff_data;
					3'd1: spr_x_hi[eff_addr[6:3]] <= eff_data[1:0]; // 10-bit signed (§4)
					3'd2: spr_y_lo[eff_addr[6:3]] <= eff_data;
					// +3 stores Y-high; the magnification write-mirror
					// claim for +3 is the unresolved ⚠ ASIC-REF §4 note.
					3'd3: spr_y_hi[eff_addr[6:3]] <= eff_data[0];   // 9-bit signed (§4)
					default: spr_mag[eff_addr[6:3]] <= eff_data;    // +4..+7 mirrors (§4)
					endcase
				end
				else if (eff_r_pal) begin
					// Low byte: D7-D4 RED, D3-D0 BLUE; high byte:
					// D3-D0 GREEN, D7-D4 unused-reads-0 (§6). Stored
					// in the documented word layout {G,R,B}.
					if (!eff_addr[0])
						pal[eff_pal_idx] <= {pal[eff_pal_idx][11:8], eff_data[7:4], eff_data[3:0]};
					else
						pal[eff_pal_idx] <= {eff_data[3:0], pal[eff_pal_idx][7:4], pal[eff_pal_idx][3:0]};
				end
				else if (eff_r_raster) begin
					case (eff_addr[3:0])
					4'h0: pri_r    <= eff_data;
					4'h1: splt_r   <= eff_data;
					4'h2: ssa_hi_r <= eff_data;
					4'h3: ssa_lo_r <= eff_data;
					4'h4: sscr_r   <= eff_data;
					4'h5: ivr_r    <= eff_data;
					default: ; // &6806/&6807: writes have no effect (§3)
					endcase
				end
				else if (eff_r_dma) begin
					case (eff_addr[3:0])
					4'h0: sar_lo[0] <= eff_data;
					4'h1: sar_hi[0] <= eff_data;
					4'h2: ppr[0]    <= eff_data;
					4'h4: sar_lo[1] <= eff_data;
					4'h5: sar_hi[1] <= eff_data;
					4'h6: ppr[1]    <= eff_data;
					4'h8: sar_lo[2] <= eff_data;
					4'h9: sar_hi[2] <= eff_data;
					4'hA: ppr[2]    <= eff_data;
					4'hF: ; // dcsr_ena handled with dcsr_ena_clr priority above
					default: ;
					endcase
				end
			end
			// eff_wsel 01/11 (&5000s / &7000s): writes ignored (§3)
		end

		// Legacy PENR/INKR translation (reference section 6): pens 0-15 + border
		// only, keyed on changes to the legacy register shadow. This sits OUTSIDE
		// the asic_cs gate on purpose: legacy writes arrive on the &7Fxx I/O port,
		// which never asserts the page chip-select (review part-B flag). A legacy
		// change and a CPU palette write on the same clock edge resolve in favour
		// of the legacy update (source order); no arbitration rule exists in the
		// sources.
		if (!reset) begin
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
				rdata   = {4'h0, spr_host_rdata};
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
					// Bit 6 = ch0 INT, bit 5 = ch1 INT, bit 4 = ch2 INT
					rdata   = {dcsr_stat | intack_raster, dcsr_flags[0], dcsr_flags[1], dcsr_flags[2], 1'b0, dcsr_ena};
					renable = 1'b1;
				end
				else if (r_raster && A[3]) begin
					// ADC Paddle registers &6808-&680F (§11; default reading: 3F 3F 3F 3F 3F 00 3F 00)
					case (A[2:0])
					3'h0: begin rdata = 8'h3F; renable = 1'b1; end
					3'h1: begin rdata = 8'h3F; renable = 1'b1; end
					3'h2: begin rdata = 8'h3F; renable = 1'b1; end
					3'h3: begin rdata = 8'h3F; renable = 1'b1; end
					3'h4: begin rdata = 8'h3F; renable = 1'b1; end
					3'h5: begin rdata = 8'h00; renable = 1'b1; end
					3'h6: begin rdata = 8'h3F; renable = 1'b1; end
					3'h7: begin rdata = 8'h00; renable = 1'b1; end
					endcase
				end
				// &6800-&6807 are write-only: reads fall through to open
				// bus (§4).
			end
			// wsel 01/11: unmapped, open bus (§4)
		end
	end

	assign D_out = rdata | {8{~renable}};
	assign pal_rdata = pal_r;

	// Vector byte: (IVR & &F8) | source; source = %110 (raster) while a
	// raster interrupt pends (reference §7 table; DMA codes arrive P7).
	// The source field is sampled on the FIRST clock edge of the
	// acknowledge cycle and held for its duration: INT_N rises one edge
	// into the cycle (irqack is combinational), so an unsampled
	// int_pending would drop the raster bits before the CPU latches the
	// byte at cycle end (review finding 2).
	reg       intack_d;
	reg [2:0] ack_src;
	always @(posedge clk) begin
		if (reset) begin
			intack_d <= 1'b0;
			ack_src  <= 3'd0;
		end
		else begin
			intack_d <= intack;
			if (intack && !intack_d) begin
				if (int_pending)
					ack_src <= 3'b110;
				else if (dcsr_flags[2])
					ack_src <= 3'b000;
				else if (dcsr_flags[1])
					ack_src <= 3'b010;
				else if (dcsr_flags[0])
					ack_src <= 3'b100;
				else
					ack_src <= 3'b000;
			end
		end
	end

	wire [7:0] vec_src = ivr_r & 8'hF8;
	assign vec_byte  = vec_src | {5'd0, ack_src};
	assign vec_valid = intack;

	// Video-side read port (registered, independent of the CPU port).
	// Own reset here: a second always writing this reg must carry the
	// reset itself or Quartus sees two constant drivers (error 10028).
	always @(posedge clk) begin
		if (reset) pal_r <= 12'd0;
		else       pal_r <= pal[pal_raddr];
	end

	//------------------------------------------------------------------
	// P4 sprite-engine services.
	//
	// Row fetch: a registered read of plus_sprite_ram. CPU reads
	// preempt acknowledgement in asic_regs; a preempted grant simply
	// does not assert ACK, and the engine holds REQ until served.
	//------------------------------------------------------------------
	wire sprq_grant = sprq_req && !(asic_cs && mem_rd);

	wire spr_host_wr = !reset && eff_cs && (eff_wsel == 2'b00);
	wire spr_host_rd = asic_cs && mem_rd && (wsel == 2'b00);

	plus_sprite_ram spr_ram_inst (
		.clk        (clk),
		.reset      (reset),
		.host_rd    (spr_host_rd),
		.host_wr    (spr_host_wr),
		.host_addr  (eff_addr[11:0]),
		.host_wdata (eff_data[3:0]),
		.host_rdata (spr_host_rdata),
		.video_rd   (sprq_req),
		.video_addr (sprq_addr),
		.video_rdata(spr_video_rdata)
	);

	assign sprq_data = spr_video_rdata;

	always @(posedge clk) begin
		if (reset) begin
			sprq_ack  <= 1'b0;
		end
		else begin
			sprq_ack  <= sprq_grant;
		end
	end

	// Access indicator: any CPU cycle inside a sprite image area
	// (&4000-&4FFF), read or write (reference §5 blanking side effect).
	assign spr_acc_en   = asic_cs && (mem_rd || mem_wr) && (wsel == 2'b00);
	assign spr_acc_idx  = A[11:8];
	assign spr_wr_en    = (asic_cs && mem_wr && (wsel == 2'b00)) || (sna_wr && (sna_addr[13:12] == 2'b00));
	assign spr_wr_addr  = sna_wr ? sna_addr[11:0] : A[11:0];
	assign spr_wr_data  = sna_wr ? sna_data[3:0] : D_in[3:0];

	// Live attribute/palette views for the sprite engine.
	genvar gi;
	generate
		for (gi = 0; gi < 16; gi = gi + 1) begin: g_sprview
			assign spr_x_view[gi*10 +: 10] = {spr_x_hi[gi], spr_x_lo[gi]};
			assign spr_y_view[gi*9  +: 9 ] = {spr_y_hi[gi], spr_y_lo[gi]};
			assign spr_mag_view[gi*4 +: 4] = spr_mag[gi][3:0];
		end
		for (gi = 0; gi < 15; gi = gi + 1) begin: g_sprpal
			assign spr_pal_view[gi*12 +: 12] = pal[17 + gi];
		end
	endgenerate

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
