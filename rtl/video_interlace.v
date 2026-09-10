//============================================================================
// MiSTer scaler interlace history + scandoubler enable (B8-2 extraction).
//
// Behavioral extraction of the Amstrad.sv top-level block that sampled
// VGA_F1 at each selected-VSYNC rise into a 3-bit shift and gated the
// scandoubler with it: the same shift-on-rise / enable-while-all-zero
// decision, shared by production (Amstrad.sv) and the B8-2 fixture so the
// test executes the real downstream decision instead of a copied C++ model.
// There is no reset input (matching the extracted block); the explicit
// initial block pins deterministic zero startup in simulation independently
// of simulator defaults; runtime history sampling matches the extracted block.
//
// This verifies the local downstream decision only: full vendor mixer/ASCAL
// field handling (sys/ascal.vhd) is not simulated here.
//============================================================================

module video_interlace
(
	input            clk,
	input            vsync_in,   // selected (filtered) VSYNC from the motherboard
	input            field_in,   // selected FIELD (VGA_F1) from the motherboard
	input      [1:0] scale,      // OSD scaler mode; nonzero forces scandoubler path
	input            forced_scandoubler,
	output     [2:0] interlace,  // last three FIELD samples at VSYNC rises
	output           scandoubler // production video_mixer enable
);

reg [2:0] interlace_r;
reg       old_vs;

initial begin
	interlace_r = 3'd0;
	old_vs = 1'b0;
end

always @(posedge clk) begin
	old_vs <= vsync_in;
	if (~old_vs & vsync_in)
		interlace_r <= {interlace_r[1:0], field_in};
end

assign interlace = interlace_r;
// Matches the extracted expression (scale || forced) && !interlace, written
// width-clean for -Wall: nonzero scale, or forced, while history is all zero.
assign scandoubler = (scale != 2'd0 || forced_scandoubler) && (interlace_r == 3'd0);

endmodule
