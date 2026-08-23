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
			default: ;  // 10/11 status, 12-17 land with their phases
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
wire       adj_end       = ((raster + 5'd1) >= R5_v_total_adj);

always @(posedge CLOCK) begin
	if (!nRESET) begin
		charline <= 7'd0;
		raster   <= 5'd0;
		in_adj   <= 1'b0;
	end
	else if (CLKEN) begin
		if (hcc_last) begin
			if (in_adj) begin
				if (adj_end) begin
					in_adj   <= 1'b0;
					raster   <= 5'd0;
					charline <= 7'd0;
				end
				else begin
					raster <= raster + 5'd1;
				end
			end
			else if (enter_adj) begin
				in_adj <= 1'b1;
				raster <= 5'd0;
				// charline deliberately frozen at R4 (ACCC §11.2.6)
			end
			else if (frame_wrap) begin
				raster   <= 5'd0;
				charline <= 7'd0;
			end
			else if (c9_done) begin
				raster   <= 5'd0;
				charline <= charline + 7'd1;
			end
			else begin
				raster <= raster + 5'd1;
			end
		end
	end
end

assign HCC  = hcc;
assign LINE = charline;
assign ROW  = raster;
assign ADJ  = in_adj;

endmodule
