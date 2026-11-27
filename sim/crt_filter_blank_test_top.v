// Test fixture wrapping rtl/crt_filter.v for live blanking seam verification.
// Exposes raw sync inputs and regenerated/live sync and blanking outputs.

module crt_filter_blank_test_top (
	input  CLK,
	input  CE_4,
	input  HSYNC_I,
	input  VSYNC_I,
	input  BYTE_OWNER,
	input  H_CONTEXT_CLEAR,
	input  WRAPPED_HSYNC_REARM,
	input  [1:0] SYNC_FILTER,
	input  HSYNC_RAW,
	input  VSYNC_RAW,
	input  HBLANK_RAW,
	input  VBLANK_RAW,
	output HSYNC_O,
	output VSYNC_O,
	output HBLANK,
	output HBLANK_LIVE,
	output VBLANK,
	output SHIFT,
	output START_GEN, END_GEN,
	output [15:0] SYNC_OBSERVATION,
	output HSYNC_SELECTED,
	output VSYNC_SELECTED,
	output HBLANK_SELECTED,
	output VBLANK_SELECTED,
	output NO_HSYNC_DEBUG
);

	crt_filter dut (
		.CLK(CLK),
		.CE_4(CE_4),
		.HSYNC_I(HSYNC_I),
		.VSYNC_I(VSYNC_I),
		.HSYNC_O(HSYNC_O),
		.VSYNC_O(VSYNC_O),
		.HBLANK(HBLANK),
		.HBLANK_LIVE(HBLANK_LIVE),
		.VBLANK(VBLANK),
		.SHIFT(SHIFT),
		.EXTRA_BYTE(), .START_GEN(START_GEN), .END_GEN(END_GEN),
		.SYNC_OBSERVATION(SYNC_OBSERVATION),
		.SOURCE_MISSING(NO_HSYNC_DEBUG),
		.H_CONTEXT_CLEAR(H_CONTEXT_CLEAR),
		.WRAPPED_HSYNC_REARM(WRAPPED_HSYNC_REARM),
		.BYTE_PHASE_RECOVERY(BYTE_OWNER && SYNC_FILTER == 2'd0)
	);

	crt_filter_output_select selector (
		.MODE(SYNC_FILTER),
		.HSYNC_FILTERED(HSYNC_O),
		.VSYNC_FILTERED(VSYNC_O),
		.HBLANK_FILTERED(HBLANK),
		.VBLANK_FILTERED(VBLANK),
		.HSYNC_RAW(HSYNC_RAW),
		.VSYNC_RAW(VSYNC_RAW),
		.HBLANK_RAW(HBLANK_RAW),
		.VBLANK_RAW(VBLANK_RAW),
		.HSYNC_OUT(HSYNC_SELECTED),
		.VSYNC_OUT(VSYNC_SELECTED),
		.HBLANK_OUT(HBLANK_SELECTED),
		.VBLANK_OUT(VBLANK_SELECTED)
	);

endmodule
