//============================================================================
//  Amstrad Plus ASIC (AMS40489 "Arnold V") integrated video — CRTC type 3
//  foundation: register file and horizontal character counter.
//
//  This module is the Plus-stream counterpart of the classic rtl/CRTC.v
//  wrapper. Per docs/plus/architecture.md the ASIC's CRTC-3 is a new
//  behavioural implementation, never a third personality inside the classic
//  core. P1 scope (roadmap §5, architecture §4): CRTC3 counters, MA/RA, DE,
//  both sync widths, VSYNC gate, +1µs display alignment; exact readback
//  quirks stay in P5.
//
//  Technical information sourced from the "Amstrad CPC CRTC Compendium" by
//  Longshot (CC BY-NC-ND).
//
//  The counter behaviour of this module follows The Amstrad CPC CRTC
//  Compendium v1.10, Serge Querne (Longshot / Logon System),
//  https://shaker.logonsystem.eu -- licensed CC BY-NC-ND 4.0. Its attribution
//  directive requires this notice in the source of CRTC emulation modules and
//  in the credits of any distributed product built from them. Individual
//  rules cite their ACCC section at the point of implementation.
//
//  Deliberate P1 exclusions (each must land with its own vectors later):
//   - Interlace (R8 bits 1:0 stored but not acted on): the type-3 IVM
//     counting/parity machinery is its own rule set (ACCC §19.5.5, §19.8.4)
//     and no P1 exit criterion needs it.
//   - Status registers R10/R11 contents and the mod-8 read map: P5 owns all
//     readback semantics (ACCC §21.2.3, §21.3.4); DO is open-bus neutral.
//   - Light pen R16/R17: no light-pen input exists on CPC hardware.
//   - The R4=0-at-C0=0-with-Rom-select I/O race (ACCC §12.5 p.101) is a Z80
//     bus-level ASIC race owned by the register-interface layer, not the
//     counter engine.
//============================================================================

module asic_video
(
	input            CLOCK,
	input            CLKEN,      // 1 µs character-clock enable
	input            nRESET,

	// CRTC register bus (type-3 port decode sits upstream of this module)
	input            ENABLE,
	input            nCS,
	input            R_nW,
	input            RS,
	input      [7:0] DI,
	output     [7:0] DO,

	// Raster timing outputs. DE follows the SKEW-DISPTMG programming
	// (ACCC §19.2, types 0/3/4); MA is the current video pointer VMA and
	// RA the scanline address C9 (which carries the adjustment index on
	// types 3/4, ACCC §11.2.6).
	output           DE,
	output    [13:0] MA,
	output     [4:0] RA,

	// Horizontal counter tap for later raster consumers (PRI/sprites/DMA)
	output     [7:0] HCC,

	// Vertical counter taps: C4 char-row counter, C9 scanline-in-row
	// counter (which doubles as the vertical-adjustment line index on
	// types 3/4, ACCC §11.2.6), and the adjustment-active flag.
	output     [6:0] LINE,
	output     [4:0] ROW,
	output           ADJ
);

/* verilator lint_off WIDTH */

//----------------------------------------------------------------------
// Register file
//
// Writes decode the full five-bit index (ACCC §28.1.9 notes reads are
// mod-8 on types 3/4 while writes reach the selected storage register;
// software stores into R14/R15 and reads them back through slots 6/7).
// Select indices 10/11 (ASIC status, read-only), 16/17 (light pen) have
// no storage here (§21.2.3 table, p.246).
//----------------------------------------------------------------------

reg  [4:0] addr;

/* verilator lint_off UNUSEDSIGNAL */
reg [7:0] R0_h_total;
reg [7:0] R1_h_displayed;
// R1/R2/R3 gain consumers with the display and sync phases; this scaffold
// pragma block shrinks as each phase lands.
/* verilator lint_off UNUSEDSIGNAL */
reg [7:0] R2_h_sync_pos;
reg [3:0] R3_h_sync_width;
reg [3:0] R3_v_sync_width;
reg [6:0] R6_v_displayed;
reg [6:0] R7_v_sync_pos;
reg [1:0] R8_skew;         // bits 5:4, SKEW-DISPTMG (types 0/3/4, §19.1)
/* verilator lint_on UNUSEDSIGNAL */
// R8 bits 1:0 select interlace modes and stay stored-but-inert for the
// whole P1 foundation: the type-3 IVM counting/parity machinery (ACCC
// §19.5.5, §19.8.4) is deliberately out of P1 scope (see header). The
// register keeps its storage so a later phase adds behaviour without a
// bus-contract change.
/* verilator lint_off UNUSEDSIGNAL */
reg [1:0] R8_interlace;
/* verilator lint_on UNUSEDSIGNAL */
reg [6:0] R4_v_total;
reg [4:0] R5_v_total_adj;
reg [4:0] R9_v_max_line;
reg [5:0] R12_start_addr_h;
reg [7:0] R13_start_addr_l;
/* verilator lint_on UNUSEDSIGNAL */

always @(posedge CLOCK) begin
	if (!nRESET) begin
		addr             <= 5'd0;
		R0_h_total       <= 8'd0;
		R1_h_displayed   <= 8'd0;
		R2_h_sync_pos    <= 8'd0;
		{R3_v_sync_width, R3_h_sync_width} <= 8'd0;
		R4_v_total       <= 7'd0;
		R5_v_total_adj   <= 5'd0;
		R6_v_displayed   <= 7'd0;
		R7_v_sync_pos    <= 7'd0;
		{R8_skew, R8_interlace} <= 4'd0;
		R9_v_max_line    <= 5'd0;
		R12_start_addr_h <= 6'd0;
		R13_start_addr_l <= 8'd0;
	end
	else if (ENABLE & ~nCS & ~R_nW) begin
		if (~RS) begin
			addr <= DI[4:0];
		end
		else begin
			case (addr)
			5'd00: R0_h_total     <= DI;
			5'd01: R1_h_displayed <= DI;
			5'd02: R2_h_sync_pos  <= DI;
			5'd03: {R3_v_sync_width, R3_h_sync_width} <= DI;
			5'd04: R4_v_total     <= DI[6:0];
			5'd05: R5_v_total_adj <= DI[4:0];
			5'd06: R6_v_displayed <= DI[6:0];
			5'd07: R7_v_sync_pos  <= DI[6:0];
			5'd08: {R8_skew, R8_interlace} <= {DI[5:4], DI[1:0]};
			5'd09: R9_v_max_line  <= DI[4:0];
			5'd12: R12_start_addr_h <= DI[5:0];
			5'd13: R13_start_addr_l <= DI;
			default: ;  // 10/11 status, 14-17 land with later phases
			endcase
		end
	end
end

// P5 owns every readback semantic including the mod-8 select map (§21.2.3);
// until then the data pin stays at the unselected level.
assign DO = 8'hFF;

//----------------------------------------------------------------------
// Horizontal character counter C0 ("HCC")
//
// C0 counts 0..R0 inclusive and wraps via equality against the live R0
// value (ACCC §13.1, p.102). Type 3 accepts any R0 without disturbing
// other counters — there is no type-0-style freeze machinery (§13.5,
// p.121); an R0 lowered below the current C0 simply makes the equality
// unreachable until C0 has overflowed its full eight-bit range and come
// back round (§13.5 "main subtlety" contrast; §28.1.1 general form).
//----------------------------------------------------------------------

reg [7:0] hcc;

wire        hcc_last = (hcc == R0_h_total);
wire [7:0]  hcc_next = hcc_last ? 8'h00 : hcc + 8'd1;

always @(posedge CLOCK) begin
	if (!nRESET) hcc <= 8'h00;
	else if (CLKEN) hcc <= hcc_next;
end

//----------------------------------------------------------------------
// Vertical counters: C9 (raster line within the character row) and C4
// (character row within the frame), plus the vertical-adjustment state.
//
// All decisions sample the live register values at the line-end edge
// (C0==R0); a bus write landing mid-line is therefore considered at that
// line's end, and a write landing exactly on the decision edge itself is
// seen by the *next* decision (the documented same-edge hazard window;
// vectors stay inside the documented mid-line windows).
//
// C9/R9 completion uses ">=" not equality: on types 3/4 an R9 lowered
// below the current C9 forces next-C9=0 with normal row accounting,
// "it is impossible to overflow C9" (ACCC §10.3.4 p.77). Equality alone
// is the natural case; the greater-than term is the forced reset.
//
// Entering adjustment from the last character row does NOT increment C4:
// it stays equal to R4 for every adjustment line (ACCC §11.2.6 p.84),
// and R6==R4 consequently covers those lines too (§18.2.4 note).
// Adjustment ends when the next adjustment-line index C9+1 has reached
// R5 — or would pass it after a mid-adjustment R5 shrink; both end the
// management on the same line ("Whether with R5 or R9, it is impossible
// to overflow C9", ACCC §11.3.3 p.86).
//
// An R4 lowered below the current C4 makes the frame-end equality
// unreachable: C4 free-runs through its seven-bit range (ACCC §12.5
// p.101, "overflow of the C4 counter"), in contrast to C9 above.
//----------------------------------------------------------------------

reg  [6:0] charline;
reg  [4:0] raster;
reg        in_adj;

wire       c9_done       = in_adj ? 1'b0 : (raster >= R9_v_max_line);
wire       last_charline = (charline == R4_v_total);
wire       enter_adj     = c9_done & last_charline &
                           (R5_v_total_adj != 5'd0);
wire       frame_wrap    = c9_done & last_charline &
                           (R5_v_total_adj == 5'd0);
wire       adj_end_n     = ((raster + 5'd1) >= R5_v_total_adj);

// Next-state values at the line-end edge, shared with the video-pointer
// and display-enable logic so every consumer sees one consistent seam.
// Adjustment end restarts the frame: C4=C9=0 (ACCC §11.3 general).
wire [4:0] raster_n      = in_adj ? (adj_end_n ? 5'd0 : raster + 5'd1) :
                           c9_done ? 5'd0 : raster + 5'd1;
wire [6:0] charline_n    = (in_adj & adj_end_n) ? 7'd0 :
                           in_adj ? charline :
                           enter_adj ? charline :
                           frame_wrap ? 7'd0 :
                           c9_done ? charline + 7'd1 : charline;
wire       adj_n         = in_adj ? !adj_end_n : enter_adj;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		charline <= 7'd0;
		raster   <= 5'd0;
		in_adj   <= 1'b0;
	end
	else if (CLKEN) begin
		if (hcc_last) begin
			in_adj   <= adj_n;
			raster   <= raster_n;
			charline <= charline_n;
		end
	end
end

//----------------------------------------------------------------------
// Video pointer: VMA (current) and VMA' (row-start latch).
//
// Two-stage {R12,R13} -> VMA' -> VMA behaviour, as on type 0 (ACCC
// §20.3.4 p.243): at every C0=0 while C4=0 BOTH pointers reload from
// R12/R13 — note there is no C9 term in the type-3/4 condition, unlike
// type 0's C4=C9=C0=0 (§20.3.1). At any other line start VMA loads from
// VMA'. The row-end capture VMA' <- VMA fires on the live comparison
// C0==R1 && C9==R9 and is suppressed during vertical adjustment, which
// instead re-solidifies the captured row base each adjustment line
// ("without updating the video pointer", ACCC §11.2.6 p.84).
//
// With R1>R0 the capture can never fire, so every row re-displays the
// frozen VMA' base — character-line repetition (ACCC §17.2 p.179) with
// no spurious border substitution on types 3/4 (§17.6.2/§19.2.4).
//----------------------------------------------------------------------

reg [13:0] vma;
reg [13:0] vma_latch;

wire row_latch_event = CLKEN && !in_adj &&
                       (hcc == R1_h_displayed) &&
                       (raster == R9_v_max_line);

always @(posedge CLOCK) begin
	if (!nRESET) begin
		vma       <= 14'd0;
		vma_latch <= 14'd0;
	end
	else if (CLKEN) begin
		if (row_latch_event) vma_latch <= vma;

		if (hcc_last) begin
			// Line start. Statement order encodes the §20.3.4 priority:
			// a degenerate simultaneous capture (only reachable when
			// R1==0) loses to the unconditional frame-start reload.
			if (!adj_n) begin
				if (charline_n == 7'd0) begin
					vma       <= {R12_start_addr_h, R13_start_addr_l};
					vma_latch <= {R12_start_addr_h, R13_start_addr_l};
				end
				else begin
					vma <= vma_latch;
				end
			end
			else begin
				vma <= vma_latch;
			end
		end
		else if (!hcc_last) begin
			// VMA increments on every character cell processed,
			// displayed or border (ACCC §17.1 p.176).
			vma <= vma + 14'd1;
		end
	end
end

//----------------------------------------------------------------------
// Display enable.
//
// Horizontal: DISPTMG on at C0=0, off from C0=R1 (ACCC §17.1 p.175).
// The registered clear makes an R1==R0 line show exactly one border
// character at C0=R0 (the interline blip, §17.6.1), while R1>R0 keeps
// the whole line displayed because the equality never fires (types 3/4
// substitute nothing, §17.6.2). R1=0 clears every line start: no
// characters are ever displayed (§17.1/§18.3.1).
//
// Vertical: the R6 test runs ONLY at the beginning of a line on types
// 3/4 — mid-line R6 updates are not considered until the next line
// start ("The update of R6 during the line is therefore not considered",
// ACCC §18.2.4 p.189). There is no R6==0 special case (§18.3.4). Because
// C4 stays ==R4 through adjustment, R6==R4 borders those lines too
// (§18.2.4 note).
//
// SKEW-DISPTMG (R8 bits 5:4; types 0/3/4, §19.2): 00 immediate, 01/10
// delay both border edges by one/two characters, 11 BORDER ON (output
// suppressed). Only the visible edges shift; pointer bookkeeping is
// unaffected (§19.2.3 p.194).
//----------------------------------------------------------------------

reg       hde;
reg       vde;
reg [1:0] dde;

wire      disp_raw = hde & vde;

always @(posedge CLOCK) begin
	if (!nRESET) begin
		hde <= 1'b0;
		vde <= 1'b0;
		dde <= 2'b00;
	end
	else if (CLKEN) begin
		if (hcc_last) begin
			hde <= (R1_h_displayed != 8'd0);
			vde <= (charline_n != R6_v_displayed);
		end
		else if ((hcc + 8'd1) == R1_h_displayed) begin
			hde <= 1'b0;
		end
		dde <= {dde[0], disp_raw};
	end
end

assign DE = (R8_skew == 2'b00) ? disp_raw :
            (R8_skew == 2'b01) ? dde[0] :
            (R8_skew == 2'b10) ? dde[1] : 1'b0;

assign HCC  = hcc;
assign LINE = charline;
assign ROW  = raster;
assign ADJ  = in_adj;
assign MA   = vma;
assign RA   = raster;

endmodule
