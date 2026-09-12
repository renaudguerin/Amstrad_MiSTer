//============================================================================
//
//  Experimental SSM sample recorder (backlog B4, phase 2 prototype).
//
//  Credit: the SSM standard this serves is Longshot / Logon System
//  (https://shaker.logonsystem.eu). This module observes no CRTC behaviour and
//  so carries no ACCC attribution.
//
//  What it does: append native-cadence video samples, in time order, into
//  fixed-size windows of DDR3, and publish a capture record when the SSM
//  detector recognises a screenshot marker. The host reconstructs the picture
//  from the retained stream. Nothing here is addressed by sync: an extra VSYNC
//  or a repainted line appends another sample instead of overwriting earlier
//  evidence, which is the whole reason a later marker can still be served.
//
//  Contracts this implements, from docs/csl-ssm-implementation-plan.md
//  ("Pinned experimental implementation contracts") and the ABI written down
//  in docs/ssm-capture-abi.md:
//
//  * A sample's address comes from its logical index, never from a count of
//    the samples that were actually stored. A dropped packed word leaves a
//    hole at its own address and flags its window; it never slides later
//    samples earlier in time.
//  * A window's descriptor is invalidated (state cleared, generation changed)
//    before any of its payload is rewritten, and is sealed only after every
//    payload beat of that window has been accepted. Both facts come from
//    running open, payload and seal through one ordered queue.
//  * A capture pins the window holding its cut plus PIN_PREV preceding
//    windows for HOLD_TICKS. When the pool is exhausted the oldest pinned
//    window is force-expired and counted; the CPC is never stalled.
//  * Initialisation invalidates every descriptor before the header's magic
//    advertises the recorder as ready.
//
//  What it deliberately does not do: prove that the retained history contains
//  a complete picture, provide identity across FPGA reconfiguration, or claim
//  any device bandwidth. The enable epoch distinguishes runs within one
//  configuration only.
//
//  The instantiation in Amstrad.sv is compile-time off by default. Enabling it
//  needs a separately reviewed DDR3 allocation and an authorised build.
//
//============================================================================

module ssm_sample_recorder #(
	// Byte base of the capture region. Must not overlap the event ring.
	parameter [31:0] CAP_BASE      = 32'h3100_0000,
	// Byte offset, from CAP_BASE, of the first window payload.
	parameter [31:0] PAYLOAD_OFF   = 32'h0010_0000,
	parameter        WINDOWS       = 8,
	parameter        WIN_IDX_BITS  = 3,      // ceil(log2(WINDOWS))
	parameter        SAMPLE_BITS   = 19,     // samples per window = 1<<SAMPLE_BITS
	parameter        PIN_PREV      = 2,
	parameter [31:0] HOLD_TICKS    = 32'h1000_0000,   // 2^28 clk ticks
	parameter        FIFO_BITS     = 6,      // packed-word queue depth = 1<<FIFO_BITS
	parameter        CAPTURE_SLOTS = 16,
	parameter        CAP_IDX_BITS  = 4,
	parameter [31:0] MAGIC         = 32'h5353_4D43,   // "SSMC"
	parameter [15:0] ABI_VERSION   = 16'd1
)
(
	input             clk,
	input             reset,
	input             enable,

	// Native observation tap: converted RGB24 with its aligned sync tuple,
	// qualified by the pixel enable of that same boundary.
	input             smp_ce,
	input       [7:0] smp_r,
	input       [7:0] smp_g,
	input       [7:0] smp_b,
	input             smp_hbl,
	input             smp_vbl,
	input             smp_hs,
	input             smp_vs,
	input             smp_field,

	// Applied B6 mode and cadence configuration
	input       [7:0] applied_config,

	// Screenshot marker, one clock wide, with its fetch-boundary cut.
	input             cap_stb,
	input      [63:0] cap_rec_a,
	input      [63:0] cap_rec_b,
	input      [63:0] cap_cut,

	// Handed to ssm_marker so the cut is latched at the HH fetch boundary.
	output     [63:0] sample_count,

	output reg        ready,
	output reg [31:0] image_loss_count,
	output reg [31:0] forced_expiry_count,
	output reg [31:0] capture_alloc,
	output reg [31:0] capture_dropped,
	output reg [31:0] captures_published,
	output reg [31:0] epoch,

	output reg [28:0] ddram_addr,
	output reg [63:0] ddram_din,
	output      [7:0] ddram_be,
	output      [7:0] ddram_burstcnt,
	output reg        ddram_we,
	input             ddram_busy
);

localparam IDX_BITS   = SAMPLE_BITS - 1;          // packed words per window
localparam FIFO_DEPTH = 1 << FIFO_BITS;
localparam ENTRY_BITS = 2 + WIN_IDX_BITS + IDX_BITS + 24 + 64;

localparam [28:0] CAP_WORD  = CAP_BASE[31:3];
localparam [28:0] HDR_WORD  = CAP_WORD;             // 8 words, 64 bytes
localparam [28:0] DESC_WORD = CAP_WORD + 29'd8;     // WINDOWS * 4 words, +0x40
localparam [28:0] REC_WORD  = CAP_WORD + 29'd64;    // CAPTURE_SLOTS * 8, +0x200
localparam [28:0] PAY_WORD  = CAP_WORD + PAYLOAD_OFF[31:3];

localparam [31:0] WINDOWS32     = WINDOWS;
localparam [31:0] WIN_SAMPLES32 = 32'd1 << SAMPLE_BITS;
localparam [19:0] WIN_SAMPLES20 = 20'd1 << SAMPLE_BITS;
localparam [31:0] PIN_PREV32    = PIN_PREV;
localparam [31:0] CAPSLOTS32    = CAPTURE_SLOTS;

localparam [1:0] CMD_PAY = 2'd0, CMD_OPEN = 2'd1, CMD_SEAL = 2'd2;

localparam [31:0] ST_INVALID = 32'd0;
localparam [31:0] ST_SEALED  = 32'd2;

localparam PAD32_WIN = 32 - WIN_IDX_BITS;

assign ddram_be       = 8'hFF;
assign ddram_burstcnt = 8'd1;

wire active = enable & ~reset;

integer k;

//----------------------------------------------------------------------------
// Enable epoch and tick
//----------------------------------------------------------------------------
//
// The epoch distinguishes runs within one FPGA configuration. It survives a
// disable on purpose, so two runs of the same load differ. It says nothing
// about a *different* load, which is why the host still needs the controlled
// startup handshake described in the plan.

reg [31:0] tick = 32'd0;

initial epoch = 32'd0;

always @(posedge clk) begin
	if (!active) tick <= 32'd0;
	else         tick <= tick + 1'd1;
end

//----------------------------------------------------------------------------
// Window pool
//----------------------------------------------------------------------------

reg [31:0] w_gen       [0:WINDOWS-1];
reg [31:0] w_pin_until [0:WINDOWS-1];
reg        w_pinned    [0:WINDOWS-1];
reg        w_lost      [0:WINDOWS-1];

wire [WINDOWS-1:0] pinned_now;
genvar g;
generate
	for (g = 0; g < WINDOWS; g = g + 1) begin : pin_eval
		wire signed [31:0] remain = $signed(tick) - $signed(w_pin_until[g]);
		assign pinned_now[g] = w_pinned[g] & (remain[31] == 1'b1);
	end
endgenerate

reg [WIN_IDX_BITS-1:0] cur_win   = {WIN_IDX_BITS{1'b0}};
reg             [31:0] cur_gen   = 32'd0;
reg             [63:0] cur_first = 64'd0;
reg  [SAMPLE_BITS-1:0] win_pos   = {SAMPLE_BITS{1'b0}};

// The three most recently sealed windows, newest first. Two are the pinned
// prehistory; the third covers a cut that lands exactly on a window boundary,
// where the last included sample belongs to the window before the open one.
reg [WIN_IDX_BITS-1:0] prev_idx [0:2];
reg             [31:0] prev_gen [0:2];
reg                    prev_ok  [0:2];

//----------------------------------------------------------------------------
// Capture target selection
//----------------------------------------------------------------------------

wire cut_in_cur = (cap_cut > cur_first);

wire [WIN_IDX_BITS-1:0] c_cur_idx = cut_in_cur ? cur_win     : prev_idx[0];
wire             [31:0] c_cur_gen = cut_in_cur ? cur_gen     : prev_gen[0];
wire                    c_cur_ok  = cut_in_cur ? 1'b1        : prev_ok[0];
wire [WIN_IDX_BITS-1:0] c_p0_idx  = cut_in_cur ? prev_idx[0] : prev_idx[1];
wire             [31:0] c_p0_gen  = cut_in_cur ? prev_gen[0] : prev_gen[1];
wire                    c_p0_ok   = cut_in_cur ? prev_ok[0]  : prev_ok[1];
wire [WIN_IDX_BITS-1:0] c_p1_idx  = cut_in_cur ? prev_idx[1] : prev_idx[2];
wire             [31:0] c_p1_gen  = cut_in_cur ? prev_gen[1] : prev_gen[2];
wire                    c_p1_ok   = cut_in_cur ? prev_ok[1]  : prev_ok[2];

reg [WINDOWS-1:0] cap_pin_mask;
always @(*) begin
	cap_pin_mask = {WINDOWS{1'b0}};
	if (cap_stb & accepting_input & !capq_full) begin
		if (c_cur_ok) cap_pin_mask[c_cur_idx] = 1'b1;
		if (c_p0_ok)  cap_pin_mask[c_p0_idx]  = 1'b1;
		if (c_p1_ok)  cap_pin_mask[c_p1_idx]  = 1'b1;
	end
end

wire cap_lost = (c_cur_ok & w_lost[c_cur_idx])
              | (c_p0_ok  & w_lost[c_p0_idx])
              | (c_p1_ok  & w_lost[c_p1_idx]);

wire [31:0] cap_status = {26'd0,
                          ~cut_in_cur,                 // bit5 cut on a window boundary
                          cap_lost,                    // bit4 known loss in the set
                          ~(c_p0_ok & c_p1_ok),        // bit3 history incomplete
                          c_p1_ok,                     // bit2
                          c_p0_ok,                     // bit1
                          c_cur_ok};                   // bit0 usable at all

wire [511:0] cap_words = {
	{tick + HOLD_TICKS, capture_alloc},                        // word 7
	{epoch, applied_config, 8'd1, cap_status[15:0]},           // word 6: epoch, config, format=1, status
	{c_p1_gen, {PAD32_WIN{1'b0}}, c_p1_idx},                   // word 5
	{c_p0_gen, {PAD32_WIN{1'b0}}, c_p0_idx},                   // word 4
	{c_cur_gen, {PAD32_WIN{1'b0}}, c_cur_idx},                 // word 3
	cap_cut,                                                   // word 2
	cap_rec_b,                                                 // word 1
	cap_rec_a                                                  // word 0
};

//----------------------------------------------------------------------------
// Window selection for a rotation
//----------------------------------------------------------------------------
//
// Pins from a capture landing on this same clock are applied first, so a
// capture and a rotation on one edge have a deterministic order: the capture
// wins and the rotation cannot reuse a window it just pinned.
//
// Preference order:
//  1. Unused windows (w_gen == 0) outside current window.
//  2. Unpinned windows outside recent rolling prehistory (prev_idx[0..2]),
//     choosing the oldest generation.
//  3. Under pressure: oldest pinned window outside rolling prehistory.
//  4. Fallback if no non-history window exists: unpinned/oldest in rolling prehistory.
//  5. Forced expiry fallback: oldest generation among all non-cur windows.

integer j;
reg [WINDOWS-1:0]      pin_eff;
reg [WIN_IDX_BITS-1:0] pick_free;
reg                    pick_free_ok;
reg [WIN_IDX_BITS-1:0] pick_old;
reg             [31:0] pick_old_gen;

reg [WIN_IDX_BITS-1:0] pick_unused;
reg                    pick_unused_ok;
reg [WIN_IDX_BITS-1:0] pick_nonhist;
reg                    pick_nonhist_ok;
reg             [31:0] pick_nonhist_gen;
reg [WIN_IDX_BITS-1:0] pick_old_nonhist;
reg                    pick_old_nonhist_ok;
reg             [31:0] pick_old_nonhist_gen;
reg [WIN_IDX_BITS-1:0] pick_hist;
reg                    pick_hist_ok;
reg             [31:0] pick_hist_gen;

always @(*) begin
	pin_eff              = pinned_now | cap_pin_mask;
	pick_unused          = {WIN_IDX_BITS{1'b0}};
	pick_unused_ok       = 1'b0;
	pick_nonhist         = {WIN_IDX_BITS{1'b0}};
	pick_nonhist_ok      = 1'b0;
	pick_nonhist_gen     = 32'hFFFF_FFFF;
	pick_old_nonhist     = {WIN_IDX_BITS{1'b0}};
	pick_old_nonhist_ok  = 1'b0;
	pick_old_nonhist_gen = 32'hFFFF_FFFF;
	pick_hist            = {WIN_IDX_BITS{1'b0}};
	pick_hist_ok         = 1'b0;
	pick_hist_gen        = 32'hFFFF_FFFF;
	pick_old             = {WIN_IDX_BITS{1'b0}};
	pick_old_gen         = 32'hFFFF_FFFF;

	for (j = 0; j < WINDOWS; j = j + 1) begin
		if (j[WIN_IDX_BITS-1:0] != cur_win) begin
			if ((prev_ok[0] && (j[WIN_IDX_BITS-1:0] == prev_idx[0])) ||
			    (prev_ok[1] && (j[WIN_IDX_BITS-1:0] == prev_idx[1])) ||
			    (prev_ok[2] && (j[WIN_IDX_BITS-1:0] == prev_idx[2]))) begin
				if (!pin_eff[j]) begin
					if (w_gen[j] < pick_hist_gen) begin
						pick_hist_gen = w_gen[j];
						pick_hist     = j[WIN_IDX_BITS-1:0];
						pick_hist_ok  = 1'b1;
					end
				end
			end
			else begin
				if (!pin_eff[j]) begin
					if (w_gen[j] == 32'd0 && !pick_unused_ok) begin
						pick_unused    = j[WIN_IDX_BITS-1:0];
						pick_unused_ok = 1'b1;
					end
					if (w_gen[j] < pick_nonhist_gen) begin
						pick_nonhist_gen = w_gen[j];
						pick_nonhist     = j[WIN_IDX_BITS-1:0];
						pick_nonhist_ok  = 1'b1;
					end
				end
				else if (!cap_pin_mask[j]) begin
					if (w_gen[j] < pick_old_nonhist_gen) begin
						pick_old_nonhist_gen = w_gen[j];
						pick_old_nonhist     = j[WIN_IDX_BITS-1:0];
						pick_old_nonhist_ok  = 1'b1;
					end
				end
			end

			if (!cap_pin_mask[j] && (w_gen[j] < pick_old_gen)) begin
				pick_old_gen = w_gen[j];
				pick_old     = j[WIN_IDX_BITS-1:0];
			end
		end
	end

	if (pick_unused_ok) begin
		pick_free    = pick_unused;
		pick_free_ok = 1'b1;
	end
	else if (pick_nonhist_ok) begin
		pick_free    = pick_nonhist;
		pick_free_ok = 1'b1;
	end
	else if (pick_old_nonhist_ok) begin
		pick_free    = pick_old_nonhist;
		pick_free_ok = 1'b0;
	end
	else if (pick_hist_ok) begin
		pick_free    = pick_hist;
		pick_free_ok = 1'b1;
	end
	else begin
		pick_free    = pick_old;
		pick_free_ok = 1'b0;
	end
end

//----------------------------------------------------------------------------
// Ordered command queue
//----------------------------------------------------------------------------
//
// One queue carries payload words, window opens and window seals, so their
// order at the DDR3 port is exactly their order in time. Two slots of headroom
// are reserved so an open or a seal can never be dropped; only payload words
// are ever lost, and losing one flags its window.

reg [ENTRY_BITS-1:0] fifo [0:FIFO_DEPTH-1];
reg   [FIFO_BITS:0] wr_ptr = 0;
reg   [FIFO_BITS:0] rd_ptr = 0;

wire [FIFO_BITS:0] fifo_used = wr_ptr - rd_ptr;
wire fifo_empty  = (wr_ptr == rd_ptr);
wire fifo_cmd_ok = (fifo_used < FIFO_DEPTH);
wire fifo_pay_ok = (fifo_used < (FIFO_DEPTH - 2));

wire [ENTRY_BITS-1:0] head  = fifo[rd_ptr[FIFO_BITS-1:0]];
wire            [1:0] h_cmd = head[ENTRY_BITS-1 -: 2];
wire [WIN_IDX_BITS-1:0] h_win  = head[64+24+IDX_BITS +: WIN_IDX_BITS];
wire     [IDX_BITS-1:0] h_idx  = head[64+24 +: IDX_BITS];
wire             [23:0] h_meta = head[64 +: 24];
wire             [63:0] h_data = head[63:0];

//----------------------------------------------------------------------------
// Capture record queue, two deep
//----------------------------------------------------------------------------

reg [511:0] capq       [0:1];
reg [CAP_IDX_BITS-1:0] capq_index [0:1];
reg   [1:0] capq_wr = 2'd0;
reg   [1:0] capq_rd = 2'd0;
wire  [1:0] capq_used = capq_wr - capq_rd;
wire        capq_empty = (capq_wr == capq_rd);
wire        capq_full  = (capq_used == 2'd2);

//----------------------------------------------------------------------------
// Sample packing
//----------------------------------------------------------------------------
//
// R 7:0, G 15:8, B 23:16, HBlank 24, VBlank 25, HSync 26, VSync 27, FIELD 28,
// 31:29 reserved zero. The earlier sample occupies the low half of the 64-bit
// write.

reg [63:0] logical = 64'd0;
assign sample_count = logical;

wire [31:0] smp_word = {3'd0, smp_field, smp_vs, smp_hs, smp_vbl, smp_hbl,
                        smp_b, smp_g, smp_r};

reg [31:0] pack_lo = 32'd0;

// Rotation sequencer. R_OPEN also runs once at startup, to open window 0.
localparam [1:0] R_BOOT = 2'd0, R_SEAL = 2'd1, R_OPEN = 2'd2, R_RUN = 2'd3;
reg  [1:0] rot = R_BOOT;
reg        rot_lost = 1'b0;
reg        rot_was_seal = 1'b0;
reg [WIN_IDX_BITS-1:0] next_win = {WIN_IDX_BITS{1'b0}};
reg             [31:0] next_gen = 32'd1;
reg [WIN_IDX_BITS-1:0] seal_win = {WIN_IDX_BITS{1'b0}};
reg             [63:0] seal_first = 64'd0;
reg                    seal_lost = 1'b0;

wire win_full = (win_pos == {SAMPLE_BITS{1'b1}});
reg quiesce_pending = 1'b0;

wire open_collision  = (rot == R_OPEN) & cap_pin_mask[next_win];
wire [WIN_IDX_BITS-1:0] open_win = open_collision ? pick_free : next_win;
wire open_forced_add = open_collision & ~pick_free_ok;

reg [7:0] active_applied_config = 8'd0;
reg       config_changed        = 1'b0;
reg       reinit_producer       = 1'b0;
// Reject the transition edge as well as subsequent traffic while old accepted
// writes drain. Readiness alone reflects the preceding clock's configuration.
wire accepting_input = ready && !config_changed &&
                       (applied_config == active_applied_config);


//----------------------------------------------------------------------------
// Producer: samples, rotation, pinning and capture enqueue
//----------------------------------------------------------------------------

always @(posedge clk) begin
	if (!active || quiesce_pending || reinit_producer) begin
		rot                 <= R_BOOT;
		rot_lost            <= 1'b0;
		rot_was_seal        <= 1'b0;
		logical             <= 64'd0;
		win_pos             <= {SAMPLE_BITS{1'b0}};
		cur_first           <= 64'd0;
		cur_win             <= {WIN_IDX_BITS{1'b0}};
		cur_gen             <= 32'd0;
		next_win            <= {WIN_IDX_BITS{1'b0}};
		next_gen            <= 32'd1;
		image_loss_count    <= 32'd0;
		forced_expiry_count <= 32'd0;
		capture_alloc       <= 32'd0;
		capture_dropped     <= 32'd0;
		capq_wr             <= 2'd0;
		wr_ptr              <= 0;
		for (k = 0; k < WINDOWS; k = k + 1) begin
			w_gen[k]       <= 32'd0;
			w_pinned[k]    <= 1'b0;
			w_pin_until[k] <= 32'd0;
			w_lost[k]      <= 1'b0;
		end
		for (k = 0; k < 3; k = k + 1) begin
			prev_idx[k] <= {WIN_IDX_BITS{1'b0}};
			prev_gen[k] <= 32'd0;
			prev_ok[k]  <= 1'b0;
		end
	end
	else begin
		if (config_ack) begin
			config_changed <= 1'b0;
		end
		else if (ready && (applied_config != active_applied_config)) begin
			config_changed <= 1'b1;
			// Invalidate rolling history immediately so no subsequent capture can
			// link samples across different video configurations.
			prev_ok[0]     <= 1'b0;
			prev_ok[1]     <= 1'b0;
			prev_ok[2]     <= 1'b0;
		end
		// --- pins first
		if (cap_stb & accepting_input) begin
			if (!capq_full) begin
				if (c_cur_ok) begin
					w_pinned[c_cur_idx]    <= 1'b1;
					w_pin_until[c_cur_idx] <= tick + HOLD_TICKS;
				end
				if (c_p0_ok) begin
					w_pinned[c_p0_idx]    <= 1'b1;
					w_pin_until[c_p0_idx] <= tick + HOLD_TICKS;
				end
				if (c_p1_ok) begin
					w_pinned[c_p1_idx]    <= 1'b1;
					w_pin_until[c_p1_idx] <= tick + HOLD_TICKS;
				end
				capq[capq_wr[0]]       <= cap_words;
				capq_index[capq_wr[0]] <= capture_alloc[CAP_IDX_BITS-1:0];
				capq_wr                <= capq_wr + 1'd1;
				capture_alloc          <= capture_alloc + 1'd1;
			end
			else begin
				// Never silently replace a record that has not been written.
				capture_dropped <= capture_dropped + 1'd1;
			end
		end

		// --- sample stream
		if (accepting_input & smp_ce) begin
			logical <= logical + 1'd1;
			if (rot == R_RUN) begin
				win_pos <= win_pos + 1'd1;
				if (!win_pos[0]) begin
					pack_lo <= smp_word;
				end
				else if (fifo_pay_ok) begin
					fifo[wr_ptr[FIFO_BITS-1:0]] <=
						{CMD_PAY, cur_win, win_pos[SAMPLE_BITS-1:1], 24'd0,
						 smp_word, pack_lo};
					wr_ptr <= wr_ptr + 1'd1;
				end
				else begin
					// The packed word is gone, but its address is not reused:
					// later words keep their own positions, so the stream is
					// never compacted. The window carries the loss.
					w_lost[cur_win]  <= 1'b1;
					image_loss_count <= image_loss_count + 32'd2;
				end

				if (win_full) begin
					rot        <= R_SEAL;
					rot_lost   <= 1'b0;
					seal_win   <= cur_win;
					seal_first <= cur_first;
					seal_lost  <= w_lost[cur_win] | ~fifo_pay_ok;
				end
			end
			else begin
				// Cannot happen with a native dot enable on the core clock: a
				// rotation takes two clocks and dots are four apart. If it
				// ever did, the sample is lost and the new window says so.
				rot_lost         <= 1'b1;
				image_loss_count <= image_loss_count + 1'd1;
			end
		end

		// --- rotation sequencer
		case (rot)
			R_BOOT: begin
				if (ready) begin
					next_win     <= {WIN_IDX_BITS{1'b0}};
					next_gen     <= 32'd1;
					rot_was_seal <= 1'b0;
					rot          <= R_OPEN;
				end
			end

			R_SEAL: begin
				// The seal is queued behind every payload word of this
				// window, so it cannot be published before them. The sample
				// path only pushes while rot is R_RUN, so there is no second
				// writer of this queue slot on this clock.
				if (fifo_cmd_ok) begin
					fifo[wr_ptr[FIFO_BITS-1:0]] <=
						{CMD_SEAL, seal_win, {IDX_BITS{1'b0}},
						 3'd0, seal_lost, WIN_SAMPLES20, seal_first};
					wr_ptr <= wr_ptr + 1'd1;
					if (pick_free_ok) begin
						next_win <= pick_free;
					end
					else begin
						// Every other window is still pinned or in rolling history.
						// Force-expire the oldest non-history pinned window first
						// (or fallback) and count it; the CPC is never stalled to
						// conceal the shortage.
						next_win            <= pick_free;
						forced_expiry_count <= forced_expiry_count + 1'd1;
					end
					next_gen     <= cur_gen + 1'd1;
					rot_was_seal <= 1'b1;
					rot          <= R_OPEN;
				end
			end

			R_OPEN: begin
				if (fifo_cmd_ok) begin
					fifo[wr_ptr[FIFO_BITS-1:0]] <=
						{CMD_OPEN, open_win, {IDX_BITS{1'b0}}, 24'd0,
						 32'd0, next_gen};
					wr_ptr <= wr_ptr + 1'd1;
					if (rot_was_seal) begin
						prev_idx[2] <= prev_idx[1];
						prev_gen[2] <= prev_gen[1];
						prev_ok[2]  <= prev_ok[1];
						prev_idx[1] <= prev_idx[0];
						prev_gen[1] <= prev_gen[0];
						prev_ok[1]  <= prev_ok[0];
						prev_idx[0] <= cur_win;
						prev_gen[0] <= cur_gen;
						prev_ok[0]  <= 1'b1;
					end
					if (open_forced_add) begin
						forced_expiry_count <= forced_expiry_count + 1'd1;
					end
					cur_win            <= open_win;
					cur_gen            <= next_gen;
					cur_first          <= logical;
					win_pos            <= {SAMPLE_BITS{1'b0}};
					w_gen[open_win]    <= next_gen;
					w_pinned[open_win] <= 1'b0;
					w_lost[open_win]   <= rot_lost;
					rot                <= R_RUN;
				end
			end

			default: ;
		endcase
	end
end

//----------------------------------------------------------------------------
// The writer: the only thing in this module that touches DDR3
//----------------------------------------------------------------------------

localparam [2:0] X_INIT_CLR  = 3'd0,
                 X_INIT_HDR  = 3'd1,
                 X_INIT_DESC = 3'd2,
                 X_INIT_RDY  = 3'd3,
                 X_IDLE      = 3'd4,
                 X_FIFO      = 3'd5,
                 X_CAP       = 3'd6,
                 X_HDRUP     = 3'd7;

reg  [2:0] xstate = X_INIT_CLR;
reg  [3:0] xstep  = 4'd0;
reg  [WIN_IDX_BITS-1:0] xwin = {WIN_IDX_BITS{1'b0}};
reg [31:0] wgen_wr [0:WINDOWS-1];
reg [63:0] pub6 = 64'd0;
reg [63:0] pub7 = 64'd0;
reg active_writer_d = 1'b0;
reg config_ack = 1'b0;
integer k2;

wire [28:0] desc_base      = DESC_WORD + {{(29-WIN_IDX_BITS-2){1'b0}}, h_win, 2'b00};
wire [28:0] init_desc_base = DESC_WORD + {{(29-WIN_IDX_BITS-2){1'b0}}, xwin, 2'b00};
wire [28:0] pay_win_off    = {{(29-WIN_IDX_BITS){1'b0}}, h_win} << IDX_BITS;
wire [28:0] pay_addr       = PAY_WORD + pay_win_off + {{(29-IDX_BITS){1'b0}}, h_idx};
wire [28:0] rec_base       = REC_WORD
                           + ({{(29-CAP_IDX_BITS){1'b0}}, capq_index[capq_rd[0]]} << 3);
wire  [2:0] cap_xword      = xstep[2:0] - 3'd1;

wire [63:0] hdr_word6 = {image_loss_count, captures_published};
wire [63:0] hdr_word7 = {capture_dropped, forced_expiry_count};
wire        hdr_dirty = (pub6 != hdr_word6) | (pub7 != hdr_word7);

always @(posedge clk) begin
	active_writer_d <= active;
	// Revoking internal readiness does not cancel an outstanding DDR request.
	if (ready && applied_config != active_applied_config) ready <= 1'b0;

	if (active & ~active_writer_d) epoch <= epoch + 1'd1;

	if (!active) begin
		ready <= 1'b0;
		// Never abandon a request the slave has not accepted: dropping `we`
		// under waitrequest is a protocol violation even for an observer.
		if (ddram_we & ddram_busy) begin
			quiesce_pending <= 1'b1;
		end
		else begin
			quiesce_pending    <= 1'b0;
			ddram_we           <= 1'b0;
			xstate             <= X_INIT_CLR;
			xstep              <= 4'd0;
			xwin               <= {WIN_IDX_BITS{1'b0}};
			rd_ptr             <= 0;
			capq_rd            <= 2'd0;
			captures_published <= 32'd0;
			pub6               <= 64'd0;
			pub7               <= 64'd0;
			for (k2 = 0; k2 < WINDOWS; k2 = k2 + 1) wgen_wr[k2] <= 32'd0;
		end
	end
	else if (quiesce_pending) begin
		ready <= 1'b0;
		if (!ddram_busy) begin
			quiesce_pending    <= 1'b0;
			ddram_we           <= 1'b0;
			xstate             <= X_INIT_CLR;
			xstep              <= 4'd0;
			xwin               <= {WIN_IDX_BITS{1'b0}};
			rd_ptr             <= 0;
			capq_rd            <= 2'd0;
			captures_published <= 32'd0;
			pub6               <= 64'd0;
			pub7               <= 64'd0;
			for (k2 = 0; k2 < WINDOWS; k2 = k2 + 1) wgen_wr[k2] <= 32'd0;
		end
	end
	else if (!ddram_busy) begin
		ddram_we <= 1'b0;
		case (xstate)
			// Clear ready/magic on the first accepted initialization beat.
			X_INIT_CLR: begin
				ddram_we              <= 1'b1;
				ddram_addr            <= HDR_WORD;
				ddram_din             <= 64'd0;
				xstep                 <= 4'd0;
				xstate                <= X_INIT_HDR;
				active_applied_config <= applied_config;
				config_ack            <= 1'b1;
				reinit_producer       <= 1'b0;
			end

			// Header words 1..7 next. Word 0 carries the magic and is
			// written last, after every descriptor has been invalidated, so a
			// host that recognises the region sees a consistent one.
			X_INIT_HDR: begin
				config_ack <= 1'b0;
				ddram_we   <= 1'b1;
				ddram_addr <= HDR_WORD + {25'd0, (xstep + 4'd1)};
				case (xstep[2:0])
					3'd0: ddram_din <= {WIN_SAMPLES32, WINDOWS32};
					3'd1: ddram_din <= {PIN_PREV32, CAPSLOTS32};
					3'd2: ddram_din <= {epoch, HOLD_TICKS};
					3'd3: ddram_din <= {32'd0, PAYLOAD_OFF};
					3'd4: ddram_din <= {32'h0000_0200, 32'h0000_0040};
					default: ddram_din <= 64'd0;   // counters, refreshed later
				endcase
				if (xstep == 4'd6) begin
					xstep  <= 4'd0;
					xstate <= X_INIT_DESC;
				end
				else xstep <= xstep + 1'd1;
			end

			X_INIT_DESC: begin
				ddram_we   <= 1'b1;
				ddram_addr <= init_desc_base;
				ddram_din  <= {ST_INVALID, 32'd0};
				if (xwin == (WINDOWS[WIN_IDX_BITS-1:0] - 1'b1)) xstate <= X_INIT_RDY;
				xwin <= xwin + 1'd1;
			end

			X_INIT_RDY: begin
				ddram_we   <= 1'b1;
				ddram_addr <= HDR_WORD;
				ddram_din  <= {16'd1, ABI_VERSION, MAGIC};   // flags bit0 = ready
				ready      <= 1'b1;
				xstate     <= X_IDLE;
			end

			X_IDLE: begin
				if (config_changed && fifo_empty && capq_empty) begin
					ready                  <= 1'b0;
					epoch                  <= epoch + 1'd1;
					reinit_producer        <= 1'b1;
					xstate                 <= X_INIT_CLR;
					xstep                  <= 4'd0;
					xwin                   <= {WIN_IDX_BITS{1'b0}};
					rd_ptr                 <= 0;
					capq_rd                <= 2'd0;
					captures_published     <= 32'd0;
					pub6                   <= 64'd0;
					pub7                   <= 64'd0;
					for (k2 = 0; k2 < WINDOWS; k2 = k2 + 1) wgen_wr[k2] <= 32'd0;
				end
				else if (!fifo_empty)      xstate <= X_FIFO;
				else if (!capq_empty) xstate <= X_CAP;
				else if (hdr_dirty)   xstate <= X_HDRUP;
			end

			X_FIFO: begin
				ddram_we <= 1'b1;
				case (h_cmd)
					CMD_PAY: begin
						ddram_addr <= pay_addr;
						ddram_din  <= h_data;
						rd_ptr     <= rd_ptr + 1'd1;
						xstate     <= X_IDLE;
					end

					CMD_OPEN: begin
						// Generation changes and the state clears in one
						// write, before any payload of this window is
						// rewritten.
						ddram_addr     <= desc_base;
						ddram_din      <= {ST_INVALID, h_data[31:0]};
						wgen_wr[h_win] <= h_data[31:0];
						rd_ptr         <= rd_ptr + 1'd1;
						xstate         <= X_IDLE;
					end

					default: begin   // CMD_SEAL
						case (xstep[1:0])
							2'd0: begin
								ddram_addr <= desc_base + 29'd1;
								ddram_din  <= h_data;              // first sample
								xstep      <= 4'd1;
							end
							2'd1: begin
								ddram_addr <= desc_base + 29'd2;
								ddram_din  <= {44'd0, h_meta[19:0]};
								xstep      <= 4'd2;
							end
							2'd2: begin
								ddram_addr <= desc_base + 29'd3;
								ddram_din  <= {epoch, 28'd0, h_meta[23:20]};
								xstep      <= 4'd3;
							end
							default: begin
								// Sealed only now, after every payload beat of
								// this window was accepted ahead of it.
								ddram_addr <= desc_base;
								ddram_din  <= {ST_SEALED, wgen_wr[h_win]};
								xstep      <= 4'd0;
								rd_ptr     <= rd_ptr + 1'd1;
								xstate     <= X_IDLE;
							end
						endcase
					end
				endcase
			end

			X_CAP: begin
				ddram_we <= 1'b1;
				if (xstep == 4'd0) begin
					// Beat 0: Invalidate Word 7 (commit index) before writing other words
					ddram_addr <= rec_base + 29'd7;
					ddram_din  <= 64'hFFFF_FFFF_FFFF_FFFF;
					xstep      <= 4'd1;
				end
				else if (xstep <= 4'd7) begin
					// Beats 1..7: Words 0..6
					ddram_addr <= rec_base + {25'd0, (xstep - 4'd1)};
					ddram_din  <= capq[capq_rd[0]][{cap_xword, 6'd0} +: 64];
					xstep      <= xstep + 1'd1;
				end
				else begin
					// Beat 8: Commit Word 7 (expiry_tick, capture_index) last
					ddram_addr         <= rec_base + 29'd7;
					ddram_din          <= capq[capq_rd[0]][{3'd7, 6'd0} +: 64];
					xstep              <= 4'd0;
					capq_rd            <= capq_rd + 1'd1;
					captures_published <= captures_published + 1'd1;
					xstate             <= X_IDLE;
				end
			end

			// Counters are published after the data they describe.
			X_HDRUP: begin
				ddram_we <= 1'b1;
				if (xstep == 4'd0) begin
					ddram_addr <= HDR_WORD + 29'd6;
					ddram_din  <= hdr_word6;
					pub6       <= hdr_word6;
					xstep      <= 4'd1;
				end
				else begin
					ddram_addr <= HDR_WORD + 29'd7;
					ddram_din  <= hdr_word7;
					pub7       <= hdr_word7;
					xstep      <= 4'd0;
					xstate     <= X_IDLE;
				end
			end

			default: xstate <= X_IDLE;
		endcase
	end
end

endmodule
