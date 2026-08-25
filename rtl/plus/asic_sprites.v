//============================================================================
//  Amstrad Plus ASIC (AMS40489) hardware sprite engine — P4.
//
//  Behavioural companion to asic_video: consumes the CRTC type-3 counter
//  taps and composites the sprite plane between the screen and the border
//  (asic-reference §5 priority: border > sprite 0 > ... > sprite 15 >
//  screen). The final precedence mux lives in asic_video; this module only
//  decides WHERE a sprite pixel exists and WHAT palette entry it selects.
//
//  Sources: docs/plus/references/asic-reference.md §4/§5 ([ARNOLD §2.1],
//  [ARNOLD-REV], [KT], [QUASAR]); each rule cites its source below. MAME
//  amstrad_m.cpp amstrad_plus_update_video_sprites() was used as the
//  behavioural cross-reference for magnification/priority/palette-entry
//  mechanics (architecture §6); where MAME approximates (it clips sprites
//  to the DE window and renders whole lines at DE-fall), this module
//  follows the written sources instead.
//
//  Coordinate model ([KT], "Describing how various comparisons are
//  calculated"):
//   - Vertical compare line = (LineCounter<<3) | (RasterCounter & 7),
//     i.e. {LINE, ROW[2:0]} from the CRTC taps; NOT gated by Vertical
//     Displayed ([KT]: "This value is *not* dependant on Vertical
//     Displayed"). A sprite occupies compare lines [Y, Y + (16<<ymag))
//     on that modular scale (MAME's signed range check agrees).
//   - Horizontal position hp is a free-running 10-bit dot counter within
//     the line ({char,dot} on a 64-character grid), cleared when the CRTC
//     character counter wraps (HWRAP). A sprite occupies [X,
//     X + (16<<xmag)). The scale wraps at exactly 64 characters, so a line
//     longer than 64 chars (R0 > 64) passes the sprite window again —
//     [KT]: "If CRTC R0>64, then the sprites may repeat horizontally".
//     X is compared on its stored 10 bits (unsigned view of the signed
//     field), so negative X values alias into the far end of the scale,
//     the exact horizontal counterpart of the modular Y compare above.
//
//  Staging architecture: each sprite has TWO row banks. The active bank
//  feeds emission; at every line seam the engine promotes the inactive
//  bank when its row tag matches the new source row (zero-latency swap),
//  else promotes-and-retags it for refill. While a line runs, a background
//  walker speculatively fills the inactive bank with the predicted next
//  source row (current row + 1 while the vertical window continues), so
//  the common static-Y case never shows an unfetched pixel. Mismatches
//  (Y/mag rewrite, R9-wrap jump, first frame) fall back to urgent refill:
//  the walker's urgent half covers the current character immediately and
//  the rest of the row trails by a few characters. Emission is gated on
//  bank validity, delivery bits AND live row-tag equality, so a mid-line
//  Y rewrite blanks the sprite rather than showing stale rows.
//
//  Bandwidth model (named assumption ⚠ ASIC-REF §5): real hardware reads
//  sprite RAM with an undocumented internal mechanism. This model stages
//  row bytes through one shared port in asic_regs, one byte per clock when
//  uncontended (~60 grants per 16-dot character at the production 4:1
//  clock ratio) against a worst-case urgent demand of 32 bytes/character
//  (sixteen fully overlapped x1 sprites) plus 2 bytes/character of
//  speculative steady-state traffic. CPU page traffic preempts individual
//  grants; a CPU pixel-data access invalidates the accessed sprite's
//  staged banks so the display re-reads the fresh image (the underlying
//  RAM is never corrupted, reference §5). Sustained overload beyond port
//  capacity leaves late sprites transparent for the affected characters
//  and self-heals next character; no vector pins beyond-capacity
//  behaviour because no rule documents it.
//
//  Named model choices (each pinned by a vector):
//   - An X rewrite mid-window cuts the sprite immediately and it reappears
//     under the new X ([ARNOLD-REV §2.1]: "changing X/Y mid-display cuts
//     the sprite and continues it at the new position").
//   - Y/magnification rewrites take effect at scanline granularity (new
//     rows stage behind the seam); finer-grained behaviour undocumented.
//   - CPU access blanking ([ARNOLD-REV §2.1]): while asic_regs reports a
//     pixel-data access to sprite n, sprite n alone is suppressed plus a
//     short registered tail; hole shape beyond that is unmeasured
//     (⚠ pending capture). Register writes never blank (§5).
//   - Sprite colour c maps to palette entry 16+c (reference §5/§6);
//     MAME's 0x2420+2c indexing agrees.
//============================================================================

module asic_sprites
(
	input        CLOCK,
	input        PIXEN,      // 16 MHz dot enable
	input        CLKEN,      // 1 µs character-clock enable (boundary strobe)
	input        HWRAP,      // asic_video: CLKEN & (hcc == R0) — line seam
	input        nRESET,

	// CRTC taps (asic_video). ROW[2:0] enters the [KT] compare formula;
	// LINE is the character-line counter C4.
	input  [6:0] LINE,
	input  [4:0] ROW,

	// Attribute view from asic_regs (live register file):
	// sprite n X[9:0] at [n*10 +: 10], Y[8:0] at [n*9 +: 9],
	// magnification nibble {xcode,ycode} at [n*4 +: 4].
	input [159:0] SPR_X,
	input [143:0] SPR_Y,
	input  [63:0] SPR_MAG,

	// Sprite colour entries 17..31 from the ASIC palette: colour c (1..15)
	// at [(c-1)*12 +: 12], word layout {G,R,B} (reference §6).
	input [179:0] SPR_PAL,

	// Pixel-data access indicator from asic_regs (blanking side effect).
	input        ACC_EN,
	input  [3:0] ACC_IDX,

	// Row-fetch port into the sprite pixel RAM (asic_regs). Pipelined:
	// REQ held until ACK pulses one clock after a granted edge; the CPU
	// port preempts individual grants. ADDR = {sprite[3:0], row[3:0],
	// byte[2:0]} addressing the row-major 16x16 nibble image.
	output reg          FQ_REQ,
	output reg   [10:0] FQ_ADDR,
	input        [7:0]  FQ_DATA,    // {odd nibble, even nibble}
	input               FQ_ACK,

	// Composited plane into asic_video (combinational, stable per dot).
	output              SPR_EN,
	output      [11:0]  SPR_RGB,    // {R,G,B} nibbles
	output      [3:0]   SPR_IDX,    // winning sprite (observability)
	output      [15:0]  SPR_WIN     // per-dot window&line active mask
);

/* verilator lint_off WIDTH */

// ROW feeds the compare formula through bits [2:0] only.
wire unused = &{ROW[4:3], 1'b0};

//----------------------------------------------------------------------
// Per-sprite decoded attributes and vertical compare (combinational).
// Everything derives from the live attribute buses and the CRTC taps so
// rewrites are visible where the rules say they are.
//----------------------------------------------------------------------

wire [9:0] vline = {LINE, ROW[2:0]};   // (LineCounter<<3)|(RasterCounter&7) [KT]

reg [9:0]  c_xa   [0:15];   // stored X, 10-bit unsigned view
reg [1:0]  c_xc   [0:15];   // X mag code (00 off, 01 x1, 10 x2, 11 x4)
reg [1:0]  c_yc   [0:15];   // Y mag code
reg [6:0]  c_wid  [0:15];   // horizontal window width in dots
reg [9:0]  c_hgt  [0:15];   // vertical window height in compare lines
reg [2:0]  c_ysh  [0:15];   // ymag shift
reg [2:0]  c_xsh  [0:15];   // xmag shift
reg [9:0]  c_diff [0:15];   // vline - Y (10-bit modular)
reg [3:0]  c_srow [0:15];   // source row index within the image
reg [15:0] c_lact;          // line-active (vertical window) mask
reg [15:0] c_ena;           // sprite enabled (both mag codes nonzero)

integer i;
always @(*) begin
	for (i = 0; i < 16; i = i + 1) begin
		c_xa[i]   = SPR_X[i*10 +: 10];
		c_xc[i]   = SPR_MAG[i*4+3 -: 2];
		c_yc[i]   = SPR_MAG[i*4+1 -: 2];
		c_xsh[i]  = (c_xc[i] == 2'd1) ? 3'd0 :
		            (c_xc[i] == 2'd2) ? 3'd1 : 3'd2;
		c_ysh[i]  = (c_yc[i] == 2'd1) ? 3'd0 :
		            (c_yc[i] == 2'd2) ? 3'd1 : 3'd2;
		c_wid[i]  = 7'd16 << c_xsh[i];
		c_hgt[i]  = 10'd16 << c_ysh[i];
		c_diff[i] = vline - {{2{SPR_Y[i*9+8]}}, SPR_Y[i*9 +: 8]};
		c_ena[i]  = (c_xc[i] != 2'd0) && (c_yc[i] != 2'd0);
		c_lact[i] = c_ena[i] && (c_diff[i] < c_hgt[i]);
		c_srow[i] = c_diff[i][5:0] >> c_ysh[i];  // diff < hgt keeps [3:0]
	end
end

//----------------------------------------------------------------------
// Horizontal window state.
//
// hp counts dots continuously through the line and clears at HWRAP, so
// hp mod 1024 IS the {char,dot} position on the 64-character scale. A
// window arms when hp equals the stored X (once per 1024-dot pass — the
// documented R0>64 repeat falls out), advances one dot per PIXEN, and
// retires after c_wid dots. An X rewrite while a window is open kills
// emission combinationally (xs_q shadow mismatch) and the next equality
// re-arms under the new X ("cuts the sprite", [ARNOLD-REV §2.1]).
//----------------------------------------------------------------------

reg  [9:0]   hp;
reg  [15:0]  sx_on;
reg  [111:0] sx_cnt;   // 7 bits per sprite: dots consumed (up to 64)
reg  [159:0] xs_q;      // X shadows (rewrite detection)

reg [3:0] blank_cnt [0:15];

// Combinational window terms shared by emission, next-state and snapshots.
reg [15:0] c_xeq;       // hp hits stored X this dot
reg [15:0] c_chg;       // X rewritten since the shadow was sampled
reg [15:0] c_wact;      // emission window open this dot
reg [5:0]  c_t    [0:15];
reg [3:0]  c_spix [0:15];// source pixel index = t >> xshift
reg [15:0] n_on;        // next-state window enables
reg [111:0] n_cnt;

always @(*) begin
	for (i = 0; i < 16; i = i + 1) begin
		c_xeq[i] = (hp == c_xa[i]);
		c_chg[i] = (xs_q[i*10 +: 10] != c_xa[i]);
		c_t[i]   = c_xeq[i] ? 6'd0 : sx_cnt[i*7 +: 7];
		c_spix[i]= c_t[i] >> c_xsh[i];
		n_on[i]  = c_xeq[i] ||
		           (sx_on[i] && !c_chg[i] &&
		            ({1'b0, sx_cnt[i*7 +: 7]} != c_wid[i]));
		if (c_xeq[i])
			n_cnt[i*7 +: 7] = 7'd1;
		else if (sx_on[i] && !c_chg[i] &&
		         ({1'b0, sx_cnt[i*7 +: 7]} != c_wid[i]))
			n_cnt[i*7 +: 7] = sx_cnt[i*7 +: 7] + 7'd1;
		else
			n_cnt[i*7 +: 7] = sx_cnt[i*7 +: 7];
	end
	c_wact = n_on & ~c_chg;
end

always @(posedge CLOCK) begin
	if (!nRESET) begin
		hp     <= 10'd0;
		sx_on  <= 16'd0;
		sx_cnt <= 112'd0;
		xs_q   <= 160'd0;
	end
	else if (PIXEN) begin
		if (CLKEN && HWRAP) begin
			hp     <= 10'd0;      // seams close every window
			sx_on  <= 16'd0;
			sx_cnt <= 112'd0;
			xs_q   <= SPR_X;
		end
		else begin
			hp    <= hp + 10'd1;
			sx_on <= n_on;
			sx_cnt<= n_cnt;
			xs_q  <= SPR_X;
		end
	end
end

//----------------------------------------------------------------------
// Access-blanking side effect ([ARNOLD-REV §2.1]): a pixel-data access to
// sprite n suppresses sprite n only; staged banks are invalidated in the
// fetch block below so the display re-reads the fresh image. Retriggerable
// tail counter; the exact hole shape is unmeasured (named assumption,
// module header). Register writes never assert ACC_EN (asic_regs decode).
//----------------------------------------------------------------------

always @(posedge CLOCK) begin
	if (!nRESET) begin
		for (i = 0; i < 16; i = i + 1) blank_cnt[i] <= 4'd0;
	end
	else begin
		for (i = 0; i < 16; i = i + 1) begin
			if (ACC_EN && (ACC_IDX == i[3:0]))
				blank_cnt[i] <= 4'd8;
			else if (PIXEN && blank_cnt[i] != 4'd0)
				blank_cnt[i] <= blank_cnt[i] - 4'd1;
		end
	end
end

//----------------------------------------------------------------------
// Dual-banked row staging and the fetch machinery.
//
// Index conventions:
//   word w = {sprite[3:0], bank, byte[2:0]}  (8 bits) -> rb_dat/sreq/sdone
//   meta m = {sprite[3:0], bank}             (5 bits) -> srowtag/sval
//   FIFO entry = {word[8:0], ram_addr[10:0]} (20 bits)
//----------------------------------------------------------------------

reg  [7:0]   rb_dat [0:255];
reg  [255:0] sreq;             // requested (or delivered)
reg  [255:0] sdone;            // delivered (usable for emission)
reg  [127:0] srowtag;          // [meta*4 +: 4]: staged row per {sprite,bank}
reg  [31:0]  sval;             // [meta]: bank contents are tagged/meaningful
reg  [15:0]  abit;             // active (emitting) bank per sprite

reg         d1w;               // one clock after the seam (taps = new line)
reg         d2w;               // two clocks after (post-swap state visible)

// Speculation health: a predicted next row exists iff the vertical window
// continues onto the next compare line.
reg [15:0] c_predok;
always @(*) begin
	for (i = 0; i < 16; i = i + 1)
		c_predok[i] = c_lact[i] && ((c_diff[i] + 10'd1) < c_hgt[i]);
end

// Walk FSM: slots 0..31 emission-critical (first/second missing byte of
// the ACTIVE bank); slots 32..63 speculative (two lowest missing bytes of
// the INACTIVE bank). Both halves wrap continuously.
reg  [7:0] walk;
reg        walk_act;
wire [3:0] wk_s    = walk[6:3];
wire       wk_spec = walk[7];   // 0: ACTIVE banks, 1: INACTIVE (spec)
wire [2:0] wk_byte = walk[2:0];
wire       wk_go   = walk_act &&
                     c_ena[wk_s] &&
                     (walk[7] ? sval[{wk_s[3:0], ~abit[wk_s]}] : 1'b1);
wire       wk_bank = wk_spec ? ~abit[wk_s] : abit[wk_s];
wire [7:0] wk_word = {wk_s, wk_bank, wk_byte};
wire [3:0] wk_row  = wk_spec ? srowtag[{wk_s[3:0], ~abit[wk_s]}*4 +: 4]
                             : c_srow[wk_s];

// The walker IS the port server: a fresh candidate is issued straight
// into the request registers (no intermediate queue); the walk stalls
// while a grant is in flight, giving roughly one byte every other clock —
// ample against the demand budget in the header note.
reg  [7:0]  fq_tag;            // word of the request currently on the port
wire        do_pop  = FQ_REQ && FQ_ACK;

wire [7:0]  pb_word  = wk_word;
wire        pb_fresh = wk_go && !sreq[pb_word] && !sdone[pb_word];

always @(posedge CLOCK) begin
	if (!nRESET) begin
		sreq    <= 256'd0;
		sdone   <= 256'd0;
		srowtag <= 128'd0;
		sval    <= 32'd0;
		abit    <= 16'd0;
		d1w     <= 1'b0;
		d2w     <= 1'b0;
		walk    <= 8'd0;
		walk_act<= 1'b0;
		FQ_REQ  <= 1'b0;
		FQ_ADDR <= 11'd0;
		fq_tag  <= 8'd0;
	end
	else begin
		d1w <= CLKEN && HWRAP;
		d2w <= d1w;

		//------------------------------------------------------------
		// Seam maintenance (one clock after the seam: LINE/ROW now show
		// the new line). Promote the inactive bank on row-tag match,
		// otherwise promote-and-retag it for refill. The outgoing active
		// bank becomes the speculation target for the predicted row.
		//------------------------------------------------------------
		if (d1w) begin
			for (i = 0; i < 16; i = i + 1) begin
				if (!c_ena[i]) begin
					// Disabled sprite: no banks staged, nothing
					// promoted or speculated (sval stays clear so
					// re-enabling refills from scratch).
					sval [{i[3:0], abit[i]}]   <= 1'b0;
					sval [{i[3:0], ~abit[i]}]  <= 1'b0;
				end
				else if (sval[{i[3:0], ~abit[i]}] &&
				         (srowtag[{i[3:0], ~abit[i]}*4 +: 4]
				          == c_srow[i])) begin
					// ZERO-MISS PROMOTE: speculation prefilled
					// the other bank with exactly this row.
					abit[i] <= ~abit[i];
					if (c_predok[i]) begin
						sdone[{i[3:0], abit[i]}*8 +: 8] <= 8'd0;
						sreq [{i[3:0], abit[i]}*8 +: 8] <= 8'd0;
						srowtag[{i[3:0], abit[i]}*4 +: 4]
							<= c_srow[i] + 4'd1;
						sval [{i[3:0], abit[i]}] <= 1'b1;
					end
					else begin
						sval [{i[3:0], abit[i]}] <= 1'b0;
					end
				end
				else if (sval[{i[3:0], abit[i]}] &&
				         (srowtag[{i[3:0], abit[i]}*4 +: 4]
				          == c_srow[i])) begin
					// KEEP: the running bank already shows this
					// row (row repeated across the seam); ready
					// the idle bank for speculating next row.
					if (c_predok[i]) begin
						sdone[{i[3:0], ~abit[i]}*8 +: 8] <= 8'd0;
						sreq [{i[3:0], ~abit[i]}*8 +: 8] <= 8'd0;
						srowtag[{i[3:0], ~abit[i]}*4 +: 4]
							<= c_srow[i] + 4'd1;
						sval [{i[3:0], ~abit[i]}] <= 1'b1;
					end
					else begin
						sval [{i[3:0], ~abit[i]}] <= 1'b0;
					end
				end
				else begin
					// REFILL: neither bank holds this row —
					// promote-and-retag the other bank; the
					// outgoing bank becomes the speculation
					// target for the predicted next row.
					sdone[{i[3:0], ~abit[i]}*8 +: 8] <= 8'd0;
					sreq [{i[3:0], ~abit[i]}*8 +: 8] <= 8'd0;
					srowtag[{i[3:0], ~abit[i]}*4 +: 4] <= c_srow[i];
					sval [{i[3:0], ~abit[i]}] <= 1'b1;
					abit[i] <= ~abit[i];
					if (c_predok[i]) begin
						sdone[{i[3:0], abit[i]}*8 +: 8] <= 8'd0;
						sreq [{i[3:0], abit[i]}*8 +: 8] <= 8'd0;
						srowtag[{i[3:0], abit[i]}*4 +: 4]
							<= c_srow[i] + 4'd1;
						sval [{i[3:0], abit[i]}] <= 1'b1;
					end
					else begin
						sval [{i[3:0], abit[i]}] <= 1'b0;
					end
				end
			end
		end

		//------------------------------------------------------------
		// CPU pixel-data access: drop both staged banks of that sprite
		// (tags stay; cleared delivery/request bits force a clean
		// re-read through both the urgent and speculative paths).
		//------------------------------------------------------------
		if (ACC_EN) begin
			sdone[ACC_IDX*16 +: 16] <= 16'd0;
			sreq [ACC_IDX*16 +: 16] <= 16'd0;
		end

		//------------------------------------------------------------
		// Port completion: the handshake ALWAYS completes (dropping it
		// would wedge REQ high); the payload write alone yields to a
		// same-edge seam swap or access flush, since bytes answered for
		// an outgoing configuration must not outlive it.
		//------------------------------------------------------------
		if (do_pop) begin
			FQ_REQ <= 1'b0;
			if (!d1w && !ACC_EN) begin
				rb_dat[fq_tag] <= FQ_DATA;
				sdone[fq_tag]  <= 1'b1;
			end
		end

		//------------------------------------------------------------
		// Walker-server: runs CONTINUOUSLY once armed (urgent slots
		// first, then speculative), wrapping 0..255; one candidate per
		// clock with a one-clock stall per in-flight grant. A per-
		// boundary restart would never reach the speculative half
		// within one character period, which starved prefilling.
		//------------------------------------------------------------
		if (!walk_act) begin
			if (d2w) begin
				walk_act <= 1'b1;
				walk     <= 6'd0;
			end
		end
		else if (!FQ_REQ) begin
			if (pb_fresh) begin
				FQ_ADDR <= {wk_s, wk_row, wk_byte};
				fq_tag  <= pb_word;
				FQ_REQ  <= 1'b1;
				sreq[pb_word] <= 1'b1;
			end
			// Whole 16-slot sprite blocks of disabled sprites are
			// skipped in a single clock so lap time scales with the
			// number of ENABLED sprites, not 16.
			if (!c_ena[wk_s])
				walk <= {walk[7:4] + 8'd1, 4'd0};
			else
				walk <= walk + 8'd1;
		end
	end
end

//----------------------------------------------------------------------
// Emission and priority (combinational, stable per dot).
//
// Byte/nibble split indexes the active bank; bank validity, delivery bits
// and the live row-tag equality keep unwritten or stale bytes transparent
// (staging/bandwidth notes above). Priority: lowest sprite index wins
// among opaque pixels (reference §5; MAME renders 15 first / 0 last,
// matching).
//----------------------------------------------------------------------

reg [15:0] c_opq;
reg [4:0]  c_meta [0:15];
reg [7:0]  c_wsel [0:15];
reg [7:0]  c_word [0:15];
reg [3:0]  c_nibv [0:15];

always @(*) begin
	for (i = 0; i < 16; i = i + 1) begin
		c_meta[i] = {i[3:0], abit[i]};
		c_wsel[i] = {i[3:0], abit[i], c_spix[i][3:1]};
		c_word[i] = rb_dat[c_wsel[i]];
		c_nibv[i] = c_spix[i][0] ? c_word[i][7:4] : c_word[i][3:0];
		c_opq[i]  = c_wact[i] && c_lact[i] &&
		            sval[c_meta[i]] &&
		            (srowtag[c_meta[i]*4 +: 4] == c_srow[i]) &&
		            sdone[c_wsel[i]] &&
		            (c_nibv[i] != 4'd0) &&
		            (blank_cnt[i] == 4'd0);
	end
end

reg win_found;
reg [3:0] win_idx;
reg [3:0] win_nib;
always @(*) begin
	win_found = 1'b0;
	win_idx   = 4'd0;
	win_nib   = 4'd0;
	for (i = 15; i >= 0; i = i - 1) begin
		if (c_opq[i]) begin
			win_found = 1'b1;
			win_idx   = i[3:0];
			win_nib   = c_nibv[i];
		end
	end
end

// Palette word {G,R,B} -> output channel order {R,G,B}. Explicit case:
// nested bit-selects of part-selects are not portable 1364-2001.
reg [11:0] win_word;
always @(*) begin
	case (win_nib)
		4'd1:  win_word = SPR_PAL[11:0];
		4'd2:  win_word = SPR_PAL[23:12];
		4'd3:  win_word = SPR_PAL[35:24];
		4'd4:  win_word = SPR_PAL[47:36];
		4'd5:  win_word = SPR_PAL[59:48];
		4'd6:  win_word = SPR_PAL[71:60];
		4'd7:  win_word = SPR_PAL[83:72];
		4'd8:  win_word = SPR_PAL[95:84];
		4'd9:  win_word = SPR_PAL[107:96];
		4'd10: win_word = SPR_PAL[119:108];
		4'd11: win_word = SPR_PAL[131:120];
		4'd12: win_word = SPR_PAL[143:132];
		4'd13: win_word = SPR_PAL[155:144];
		4'd14: win_word = SPR_PAL[167:156];
		4'd15: win_word = SPR_PAL[179:168];
		default: win_word = 12'd0;
	endcase
end

assign SPR_EN  = win_found;
assign SPR_IDX = win_idx;
assign SPR_WIN = c_wact & c_lact;
assign SPR_RGB = {win_word[7:4], win_word[11:8], win_word[3:0]};

/* verilator lint_on WIDTH */

endmodule
