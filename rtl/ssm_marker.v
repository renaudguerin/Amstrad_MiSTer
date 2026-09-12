//============================================================================
//
//  ScreenShot Management (SSM) marker detector and DDR3 event ring.
//
//  Implements SSM v1.1 (Longshot / Logon System, July 2026,
//  docs/references/SSM-STANDARD-EN.pdf; standards and SHAKER files at
//  https://shaker.logonsystem.eu). Credit: Longshot / Logon System.
//
//  This module observes CRTC behaviour nowhere, so it carries no ACCC
//  attribution; the Logon System credit above is for the SSM standard itself.
//
//  A SHAKER build from version 2.5 marks each of its test screens by executing
//  two consecutive undefined-ED instructions, #ED #LL #ED #HH. Those are two
//  NOPs on real hardware, so the same disc runs identically on a CPC, on an
//  emulator and here; an observer that watches the opcode-fetch stream can
//  label a capture with the test that produced it. This module is that
//  observer. It is passive: it never drives the CPU, the CRTC or video
//  timing, and with `enable` low it is held in reset and issues no DDR3
//  traffic at all.
//
//  Events are published to a ring in the core-reserved DDR3 window, which the
//  host reads with `dd if=/dev/mem`. No Main patch and no device daemon is
//  needed. The alternatives were checked and rejected: `status_set` only
//  updates the saved CFG, and `info_req` only paints OSD text; neither
//  reaches user space.
//
//  Two contracts matter to anything downstream and are implemented here:
//
//  * Enabling the observer publishes the header with `written = 0` before any
//    marker arrives. Without that, a second core load leaves the previous
//    run's records in the same physical window and the host replays them.
//    This is a bounded startup boundary, not a session identity: an old empty
//    header still looks like a fresh one, so the host has to combine it with a
//    controlled start (docs/csl-ssm-implementation-plan.md).
//
//  * A marker's timestamp is latched at the HH opcode-fetch completion edge
//    and carried through recognition. The registered `hit` is two clocks
//    later; using it would place the cut after samples that belong on the far
//    side of the marker.
//
//============================================================================

module ssm_marker #(
	// Byte address of the ring inside the core-reserved DDR3 window.
	// DDRAM_ADDR is a 64-bit word index (sys/sys_top.v derives the HDMI
	// palette address the same way, `LFB_BASE[31:3]`), so the word index is
	// this value >> 3. 0x30000000 is the MiSTer convention for the core
	// window; see docs/csl-ssm-implementation-plan.md for the standing
	// caveat that this base is convention-derived, not yet read back from a
	// device.
	parameter [31:0] DDR_BASE   = 32'h3000_0000,
	parameter        SLOT_BITS  = 6,               // ring holds 1<<SLOT_BITS records
	parameter [31:0] MAGIC      = 32'h5353_4D31   // "SSM1"
)
(
	input             clk,
	input             reset,
	// OSD toggle. Low holds the detector in reset and gates every DDR3
	// write. It exists to keep DDR3 quiet and to exclude the rare program
	// that executes undefined-ED sequences by accident, not because the
	// detector can disturb the machine.
	input             enable,

	// Raw opcode-fetch tap. `m1_fetch` is the motherboard's existing
	// `~M1_n & ~MREQ_n & ~RD_n` level, the same condition it already uses
	// for Plus open-bus behaviour, so wait states and T80pa clock enables
	// are handled identically. It is a level spanning the whole M1 read,
	// so this module takes its falling edge: sampling the level would count
	// one wait-stated fetch several times. INT acknowledge leaves MREQ_n
	// high and so produces no fetch at all.
	input             m1_fetch,
	input       [7:0] bus_data,

	// Output-side raster position, used to stamp each event. Taken from the
	// video timing rather than from CRTC registers: phase 2 compares against
	// frames coming out of the output chain, which is the same timebase.
	input             ce_pix,
	input             hsync,
	input             vsync,
	input             field,

	// Logical sample count of the phase 2 sample recorder, if one is
	// compiled in. Tie to zero otherwise: the ring does not use it, and the
	// recorder needs the cut latched here, at the fetch boundary, rather
	// than reconstructed from the later `hit`.
	input      [63:0] sample_count,

	// Observation for simulation, for the runner's sanity checks and for the
	// phase 2 recorder. `event_stb` is one clock wide; every `event_*` output
	// beside it is valid on that clock.
	output reg [15:0] last_code,
	output reg [31:0] event_count,
	output reg  [7:0] dropped_count,
	output            event_stb,
	output            event_capture,
	output     [15:0] event_code,
	output     [31:0] event_tick,
	output     [63:0] event_cut,
	output     [63:0] event_rec_a,
	output     [63:0] event_rec_b,

	// DDR3 master, Avalon-style with waitrequest.
	output reg [28:0] ddram_addr,
	output reg [63:0] ddram_din,
	output      [7:0] ddram_be,
	output      [7:0] ddram_burstcnt,
	output reg        ddram_we,
	input             ddram_busy
);

localparam [28:0] BASE_WORD = DDR_BASE[31:3];

assign ddram_be       = 8'hFF;
assign ddram_burstcnt = 8'd1;

wire active = enable & ~reset;

//----------------------------------------------------------------------------
// Raster position and free-running tick
//----------------------------------------------------------------------------
//
// `frame` counts VSYNC rising edges. It is an ordering and correlation key,
// never an answer to "which image": the SHAKER author's 2026-09-12 reply
// records that "frame" has two incompatible meanings in common use and that
// either can occur several times within one displayed image. `line` and
// `hpos` are the fields that actually locate the marker, and they are what
// locates the current/previous seam in a capture taken at the instruction.

reg [23:0] frame = 24'd0;
reg  [9:0] line  = 10'd0;
reg  [7:0] hpos  = 8'd0;
reg [31:0] tick  = 32'd0;
reg        hsync_d = 1'b0, vsync_d = 1'b0;

always @(posedge clk) begin
	tick <= tick + 1'd1;
	if (!active) begin
		frame   <= 24'd0;
		line    <= 10'd0;
		hpos    <= 8'd0;
		tick    <= 32'd0;
		hsync_d <= hsync;
		vsync_d <= vsync;
	end
	else if (ce_pix) begin
		hsync_d <= hsync;
		vsync_d <= vsync;
		hpos    <= hpos + 1'd1;
		if (hsync & ~hsync_d) begin
			hpos <= 8'd0;
			line <= line + 1'd1;
		end
		if (vsync & ~vsync_d) begin
			line  <= 10'd0;
			frame <= frame + 1'd1;
		end
	end
end

//----------------------------------------------------------------------------
// Opcode-fetch tap and the fetch-completion boundary
//----------------------------------------------------------------------------

reg       m1_fetch_d = 1'b0;
reg [7:0] fetch_data = 8'd0;
reg       fetch_stb  = 1'b0;

// The fetch is complete on the first clock where the level is seen low. This
// clock's `tick`, raster position and sample count are the marker's cut. A
// sample consumed on this same clock edge is *excluded*: `sample_count`
// increments at the edge that ends this cycle, so the value read here is the
// count before that sample.
wire fetch_end = m1_fetch_d & ~m1_fetch;

reg [31:0] tick_at_fetch = 32'd0;
reg [42:0] pos_at_fetch  = 43'd0;   // {field, hpos, line, frame}
reg [63:0] cut_at_fetch  = 64'd0;

always @(posedge clk) begin
	m1_fetch_d <= m1_fetch;
	fetch_stb  <= fetch_end;
	if (m1_fetch) fetch_data <= bus_data;
	if (!active) begin
		m1_fetch_d <= 1'b0;
		fetch_stb  <= 1'b0;
	end
end

always @(posedge clk) begin
	if (!active) begin
		tick_at_fetch <= 32'd0;
		pos_at_fetch  <= 43'd0;
		cut_at_fetch  <= 64'd0;
	end
	else if (fetch_end) begin
		tick_at_fetch <= tick;
		pos_at_fetch  <= {field, hpos, line, frame};
		cut_at_fetch  <= sample_count;
	end
end

//----------------------------------------------------------------------------
// Marker recognition
//----------------------------------------------------------------------------

// SSM v1.1 lists the byte values a marker may carry: #00-#3F, #7F-#9F,
// #A4-#A7, #AC-#AF, #B4-#B7, #BC-#BF and #C0-#FD. Those are the ED opcodes
// the Z80A leaves undefined, minus #FE and #FF, which the standard holds back
// for its own reserved codes. #ED #FE #ED #FF (screenshot) and
// #ED #FF #ED #FF (snapshot) use exactly those two values, so the matcher has
// to accept them: the restriction is a reservation for the standard, not a
// property of the processor.
function ssm_byte_allowed(input [7:0] b);
	ssm_byte_allowed =
		   (b <= 8'h3F)
		|| (b >= 8'h7F && b <= 8'h9F)
		|| (b >= 8'hA4 && b <= 8'hA7)
		|| (b >= 8'hAC && b <= 8'hAF)
		|| (b >= 8'hB4 && b <= 8'hB7)
		|| (b >= 8'hBC && b <= 8'hBF)
		|| (b >= 8'hC0);
endfunction

localparam [1:0] S_ED1 = 2'd0, S_LL = 2'd1, S_ED2 = 2'd2, S_HH = 2'd3;

reg  [1:0] state = S_ED1;
reg  [7:0] ll = 8'd0;
reg        hit = 1'b0;
reg [15:0] hit_code = 16'd0;
reg [31:0] hit_tick = 32'd0;
reg [42:0] hit_pos  = 43'd0;
reg [63:0] hit_cut  = 64'd0;

// SSM v1.1 reserves #0000 and every #FFxx. #FFFE is the screenshot-with-a-CSL-
// name variant; every non-reserved code is itself a screenshot request. The
// other reserved codes are telemetry only and must not pin sample windows.
wire hit_is_capture = (hit_code == 16'hFFFE)
                   || !((hit_code == 16'h0000) || (hit_code[15:8] == 8'hFF));

assign event_stb     = hit;
assign event_capture = hit & hit_is_capture;
assign event_code    = hit_code;
assign event_tick    = hit_tick;
assign event_cut     = hit_cut;

always @(posedge clk) begin
	hit <= 1'b0;

	if (!active) begin
		state <= S_ED1;
		ll    <= 8'd0;
	end
	else if (fetch_stb) begin
		case (state)
			S_ED1: state <= (fetch_data == 8'hED) ? S_LL : S_ED1;

			// #ED is itself an allowed value, so #ED #ED is a complete
			// undefined-ED instruction with LL = #ED rather than a restart.
			S_LL: begin
				if (ssm_byte_allowed(fetch_data)) begin
					ll    <= fetch_data;
					state <= S_ED2;
				end
				else begin
					// A real ED instruction, for example #ED #4B
					// (LD BC,(nn)). Its operand bytes are not opcode
					// fetches, so they never reach this matcher.
					state <= S_ED1;
				end
			end

			// The spec's worked example, #ED #3F #00 #ED #3E #ED #3D, lands
			// here on the #00: any byte other than #ED resets the wait for
			// the second pair, and the run then yields exactly #3D3E.
			S_ED2: state <= (fetch_data == 8'hED) ? S_HH : S_ED1;

			S_HH: begin
				if (ssm_byte_allowed(fetch_data)) begin
					hit      <= 1'b1;
					hit_code <= {fetch_data, ll};
					// The boundary values latched when this HH fetch
					// completed, two clocks ago.
					hit_tick <= tick_at_fetch;
					hit_pos  <= pos_at_fetch;
					hit_cut  <= cut_at_fetch;
				end
				state <= S_ED1;
			end
		endcase
	end
end

//----------------------------------------------------------------------------
// Event record and DDR3 ring writer
//----------------------------------------------------------------------------
//
// Ring layout, all little-endian 64-bit words from DDR_BASE:
//   +0x00  magic | format version | ring entries
//   +0x08  written records | dropped | (reserved)
//   +0x10  record 0 word A, +0x18 record 0 word B, then one pair per slot.
//
// The record pair is written before the header, so a host that reads the
// header never sees a count pointing at a half-written record.
//
// `event_count` counts every marker the detector saw; `written` counts the
// ones that reached the ring. The writer holds one pending event, which is
// ample when SHAKER emits a marker per screen, and a marker arriving while
// that slot is full increments `dropped` instead of vanishing.
//
// `dropped` saturates at 255: that value means "at least 255", not "exactly
// 255". A drop that lands on the same clock as the header write would leave
// the published count behind the register, so the writer republishes the
// header from idle whenever the two disagree. Once the writer is idle,
// written + dropped equals every event the detector saw.

localparam [2:0] W_MAGIC = 3'd0, W_INIT_HDR = 3'd1, W_IDLE = 3'd2,
                 W_RECA  = 3'd3, W_RECB     = 3'd4, W_HDR  = 3'd5,
                 W_REPUB = 3'd6;

localparam [7:0] RING_ENTRIES = 8'd1 << SLOT_BITS;

reg  [2:0] wstate = W_MAGIC;
reg [31:0] written = 32'd0;
reg  [7:0] pub_dropped = 8'd0;

reg [63:0] rec_a = 64'd0;
reg [63:0] rec_b = 64'd0;
reg        pend  = 1'b0;
reg        quiesce_pending = 1'b0;

wire [SLOT_BITS-1:0] slot     = written[SLOT_BITS-1:0];
wire          [28:0] slot_off = {{(28-SLOT_BITS){1'b0}}, slot, 1'b0};

// Latched at the marker so the record describes the instant it was seen,
// not the instant the DDR3 port got around to accepting the write.
wire [63:0] capture_a = {5'd0, hit_pos, hit_code};
wire [63:0] capture_b = {16'd0, hit_tick, event_count[15:0]};

assign event_rec_a = capture_a;
assign event_rec_b = capture_b;

always @(posedge clk) begin
	if (!active) begin
		// Disable and reset must not abandon a request the slave has not
		// accepted yet: dropping `we` while waitrequest is asserted is a
		// protocol violation, and the observer's lack of feedback into the
		// CPU does not make that harmless. Hold the whole request until the
		// slave takes it, then go quiet.
		if (ddram_we & ddram_busy) begin
			quiesce_pending <= 1'b1;
		end
		else begin
			quiesce_pending <= 1'b0;
			ddram_we      <= 1'b0;
			wstate        <= W_MAGIC;
			written       <= 32'd0;
			event_count   <= 32'd0;
			dropped_count <= 8'd0;
			pub_dropped   <= 8'd0;
			last_code     <= 16'd0;
			pend          <= 1'b0;
		end
	end
	else if (quiesce_pending) begin
		// Disable or reset was asserted while a write was stalled. Hold the
		// request until accepted, discard any new events, and reinitialize.
		if (!ddram_busy) begin
			quiesce_pending <= 1'b0;
			ddram_we        <= 1'b0;
			wstate          <= W_MAGIC;
			written         <= 32'd0;
			event_count     <= 32'd0;
			dropped_count   <= 8'd0;
			pub_dropped     <= 8'd0;
			last_code       <= 16'd0;
			pend            <= 1'b0;
		end
	end
	else begin
		if (hit) begin
			event_count <= event_count + 1'd1;
			last_code   <= hit_code;
			if (!pend) begin
				pend  <= 1'b1;
				rec_a <= capture_a;
				rec_b <= capture_b;
			end
			else if (dropped_count != 8'hFF) begin
				dropped_count <= dropped_count + 1'd1;
			end
		end

		// Avalon: hold address/data/we until the slave drops waitrequest.
		if (!ddram_busy) begin
			ddram_we <= 1'b0;
			case (wstate)
				// Enabling publishes the header before any marker, so the
				// host cannot read a previous run's count as this run's.
				W_MAGIC: begin
					ddram_addr <= BASE_WORD;
					ddram_din  <= {8'd0, RING_ENTRIES, 16'd1, MAGIC};
					ddram_we   <= 1'b1;
					wstate     <= W_INIT_HDR;
				end

				W_INIT_HDR: begin
					ddram_addr <= BASE_WORD + 29'd1;
					ddram_din  <= 64'd0;
					ddram_we   <= 1'b1;
					pub_dropped <= 8'd0;
					wstate     <= W_IDLE;
				end

				W_IDLE: begin
					if (pend) wstate <= W_RECA;
					else if (pub_dropped != dropped_count) wstate <= W_REPUB;
				end

				W_RECA: begin
					ddram_addr <= BASE_WORD + 29'd2 + slot_off;
					ddram_din  <= rec_a;
					ddram_we   <= 1'b1;
					wstate     <= W_RECB;
				end

				W_RECB: begin
					ddram_addr <= BASE_WORD + 29'd3 + slot_off;
					ddram_din  <= rec_b;
					ddram_we   <= 1'b1;
					wstate     <= W_HDR;
				end

				W_HDR: begin
					ddram_addr  <= BASE_WORD + 29'd1;
					ddram_din   <= {24'd0, dropped_count, written + 32'd1};
					ddram_we    <= 1'b1;
					written     <= written + 1'd1;
					pub_dropped <= dropped_count;
					pend        <= 1'b0;
					wstate      <= W_IDLE;
				end

				// A drop that landed on the header cycle above is published
				// here instead of waiting for an event that may never come.
				W_REPUB: begin
					ddram_addr  <= BASE_WORD + 29'd1;
					ddram_din   <= {24'd0, dropped_count, written};
					ddram_we    <= 1'b1;
					pub_dropped <= dropped_count;
					wstate      <= W_IDLE;
				end

				default: wstate <= W_IDLE;
			endcase
		end
	end
end

endmodule
