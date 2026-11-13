//============================================================================
//
//  ScreenShot Management (SSM) marker detector and DDR3 event ring.
//
//  Implements SSM v1.2 (Longshot / Logon System, September 2026;
//  the user-owned standard is kept with the ignored SHAKER media; files at
//  https://shaker.logonsystem.eu). Credit: Longshot / Logon System.
//
//  This module implements no CRTC behaviour. The optional private diagnostic
//  only copies existing state; CRTC rule attribution remains with its owners.
//  The Logon System credit above is for the SSM standard itself.
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
//    controlled start
//    (hardware observations).
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
	// window; see hardware observations
	// for the standing caveat that this base is convention-derived, not yet
	// read back from a device.
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

	// Opcode-fetch tap. CPU-owned validity qualifies each physical
	// `~M1_n & ~MREQ_n & ~RD_n` read throughout its data interval, including
	// prefixes and the initial HALT opcode as a marker separator. Repeated
	// halted reads and NMI acknowledge dummy reads carry substituted internal
	// NOPs and are excluded. This module takes the qualified level's falling
	// edge to count each wait-stated fetch once. Maskable INT acknowledge
	// leaves MREQ_n high and produces no memory-opcode fetch.
	input             m1_fetch,
	input       [7:0] bus_data,
	// Passive motherboard state. Sampled with tick/position at HH fetch end.
	input      [20:0] sync_observation,

	// Output-side raster position, used to stamp each event. Taken from the
	// video timing rather than from CRTC registers: phase 2 compares against
	// frames coming out of the output chain, which is the same timebase.
	input             ce_pix,
	input             hsync,
	input             vsync,
	input             field,

	// Observation for simulation and harness sanity checks. `event_stb` is one
	// clock wide when a marker is detected.
	output reg [15:0] last_code,
	output reg [31:0] event_count,
	output reg  [7:0] dropped_count,
	output            event_stb,

	// DDR3 master, Avalon-style with waitrequest.
	output reg [28:0] ddram_addr,
	output reg [63:0] ddram_din,
	output      [7:0] ddram_be,
	output      [7:0] ddram_burstcnt,
	output reg        ddram_we,
	input             ddram_busy
`ifdef SSM_S_PASSIVE_TRACE
	// Temporary original-S observer: physical M1 mapping, never CPU register PC.
	, input s_trace_enable, s_trace_qualified
	, input [15:0] s_trace_addr
	, input [127:0] s_trace_snapshot
`endif
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
// clock's `tick` and raster position are the marker's timestamp.
wire fetch_end = m1_fetch_d & ~m1_fetch;

reg [31:0] tick_at_fetch = 32'd0;
reg [20:0] observation_at_fetch = 21'd0;
reg [42:0] pos_at_fetch  = 43'd0;   // {field, hpos, line, frame}

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
		observation_at_fetch <= 21'd0;
		pos_at_fetch  <= 43'd0;
	end
	else if (fetch_end) begin
		tick_at_fetch <= tick;
		observation_at_fetch <= sync_observation;
		pos_at_fetch  <= {field, hpos, line, frame};
	end
end

`ifdef SSM_S_PASSIVE_TRACE
`ifdef SSM_DT_PASSIVE_TRACE
// DT-only specialization; all state is passive and shares the existing writer.
reg [15:0] s_fetch_addr;
reg s_fetch_ok=0;
reg s_started=0, s_frozen=0;
reg [2:0] s_count=0;
reg [7:0] s_error=0;
reg [127:0] s_samples[0:6];
reg [31:0] s_ticks[0:6];
reg s_flush=0,s_done=0,s_writing=0,s_header_pending=0;
reg [3:0] s_index=0;
reg [31:0] s_result_tick=0;
reg [63:0] s_rec_a,s_rec_b;
wire s_header_accepted=s_header_pending&&ddram_we&&!ddram_busy;
reg [2:0] dt_marker_stage=0;
reg [7:0] dt_marker_low=0;
reg dt_f7_pair_seen=0,dt_f7_verified=0,dt_header_acked=0;
reg [31:0] dt_f7_marker_tick=0;
wire dt_marker_ret=fetch_end&&s_fetch_ok&&dt_marker_stage==4&&s_fetch_addr==16'hA389&&fetch_data==8'hC9;
wire dt_arm_now=dt_marker_ret&&dt_marker_low==8'hF8&&!s_started;
wire dt_finish_now=dt_marker_ret&&dt_marker_low==8'hF7&&s_started;
reg dt_running=0,dt_bracket_started=0,dt_complete=0;
reg dt_tail_pending=0,dt_tail_end=0;
reg [1:0] dt_tail_slot=0,dt_next=0;
reg [15:0] dt_tail_addr=0;
reg [5:0] dt_mask=0;
reg dt_raw_d=0,dt_gb_d=0,dt_hs_d=0;
wire dt_raw=s_trace_snapshot[95],dt_gb=s_trace_snapshot[97],dt_hs=s_trace_snapshot[115];
wire dt_raw_rise=dt_raw&&!dt_raw_d,dt_gb_rise=dt_gb&&!dt_gb_d,dt_hs_rise=dt_hs&&!dt_hs_d;
wire dt_first=fetch_end&&s_fetch_ok&&s_fetch_addr==16'h69F1&&fetch_data==8'hED&&s_started&&!s_frozen&&!dt_bracket_started;
wire dt_last=fetch_end&&s_fetch_ok&&s_fetch_addr==16'h69F1&&fetch_data==8'hED&&dt_running&&!s_frozen;
wire dt_window=(dt_running||dt_first)&&!s_frozen&&!dt_last&&!dt_finish_now;
reg [3:0] dt_raw_count=0,dt_gb_count=0;
reg dt_raw_overflow=0,dt_gb_overflow=0;
reg [1:0] dt_owner=0;
reg dt_owner_valid=0;
wire [1:0] dt_owner_now=dt_raw_rise?dt_raw_count[1:0]:dt_owner;
wire dt_owner_ok=dt_raw_rise?(dt_raw_count<3):dt_owner_valid;
reg [17:0] dt_black[0:2];
reg [2:0] dt_black_overflow=0,dt_seen_ga=0,dt_seen_de=0;
reg [23:0] dt_elapsed=0;
reg dt_elapsed_overflow=0;
reg [9:0] dt_lines=0;
reg dt_line_overflow=0;
reg [31:0] dt_end_tick=0;
integer dt_i;
always @(posedge clk) begin : dt_observe
 reg [7:0] err;
 reg [5:0] mask;
 if(!active||!s_trace_enable) begin
  s_started<=0;s_frozen<=0;s_count<=0;s_error<=0;s_fetch_ok<=0;
  dt_marker_stage<=0;dt_marker_low<=0;dt_f7_pair_seen<=0;dt_f7_verified<=0;
  dt_running<=0;dt_bracket_started<=0;dt_complete<=0;dt_tail_pending<=0;dt_tail_end<=0;dt_next<=0;
  dt_mask<=0;dt_raw_d<=dt_raw;dt_gb_d<=dt_gb;dt_hs_d<=dt_hs;
  dt_raw_count<=0;dt_gb_count<=0;dt_raw_overflow<=0;dt_gb_overflow<=0;
  dt_owner<=0;dt_owner_valid<=0;dt_seen_ga<=0;dt_seen_de<=0;dt_black_overflow<=0;
  dt_elapsed<=0;dt_elapsed_overflow<=0;dt_lines<=0;dt_line_overflow<=0;dt_end_tick<=0;
  for(dt_i=0;dt_i<3;dt_i=dt_i+1)dt_black[dt_i]<=0;
  for(dt_i=0;dt_i<7;dt_i=dt_i+1)begin s_samples[dt_i]<=0;s_ticks[dt_i]<=0;end
 end else begin
  err=s_error;mask=dt_mask;
  dt_raw_d<=dt_raw;dt_gb_d<=dt_gb;dt_hs_d<=dt_hs;
  if(m1_fetch) begin
   s_fetch_addr<=s_trace_addr;
   s_fetch_ok<=s_trace_qualified&&(!m1_fetch_d||(s_fetch_ok&&s_fetch_addr==s_trace_addr));
  end
  // Authenticate the actual undefined-ED pair and its following RET.
  if(fetch_end) begin
   if(!s_fetch_ok)dt_marker_stage<=0;
   else case(dt_marker_stage)
    0:if(s_fetch_addr==16'hA385&&fetch_data==8'hED)dt_marker_stage<=1;
    1:if(s_fetch_addr==16'hA386&&(fetch_data==8'hF8||fetch_data==8'hF7))begin dt_marker_low<=fetch_data;dt_marker_stage<=2;end else dt_marker_stage<=0;
    2:if(s_fetch_addr==16'hA387&&fetch_data==8'hED)dt_marker_stage<=3;else dt_marker_stage<=0;
    3:if(s_fetch_addr==16'hA388&&fetch_data==8'h02)begin
       dt_marker_stage<=4;
       if(dt_marker_low==8'hF7&&s_started)begin dt_f7_pair_seen<=1;dt_f7_marker_tick<=tick;end
      end else dt_marker_stage<=0;
    default:dt_marker_stage<=0;
   endcase
  end
  if(dt_arm_now)begin s_started<=1;s_count<=7;dt_f7_pair_seen<=0;dt_f7_verified<=0;end
  if(dt_first)begin
   dt_running<=1;dt_bracket_started<=1;
   s_samples[0]<=s_trace_snapshot;s_ticks[0]<=tick;
   dt_tail_pending<=1;dt_tail_slot<=0;dt_tail_addr<=16'h69F2;dt_tail_end<=0;
  end
  if(fetch_end&&s_started&&!s_frozen&&!dt_first)begin
   if(dt_last)begin
    if(dt_tail_pending)err=err|8'd4;
    s_frozen<=1;dt_running<=0;dt_end_tick<=tick;
    dt_tail_pending<=1;dt_tail_end<=1;dt_tail_addr<=16'h69F2;
   end else if(dt_tail_pending)begin
    dt_tail_pending<=0;
    if(!s_fetch_ok)err=err|8'd1;
    else if(s_fetch_addr!=dt_tail_addr)err=err|8'd4;
    else if(fetch_data!=8'h49)err=err|8'd2;
    else begin mask[dt_tail_slot]=1;dt_next<=dt_tail_slot+1'b1;end
   end else if(s_fetch_addr==16'h69F1||s_fetch_addr==16'h69FC||s_fetch_addr==16'h6A07)begin
    if(!s_fetch_ok)err=err|8'd1;
    else if(fetch_data!=8'hED)err=err|8'd2;
    else if(dt_running&&((s_fetch_addr==16'h69FC&&dt_next==1)||(s_fetch_addr==16'h6A07&&dt_next==2)))begin
     s_samples[dt_next]<=s_trace_snapshot;s_ticks[dt_next]<=tick;
     dt_tail_pending<=1;dt_tail_end<=0;dt_tail_slot<=dt_next;dt_tail_addr<=s_fetch_addr+16'd1;
    end else err=err|8'd4;
   end
  end
  // Closing prefix fixes the window immediately; its following49 validates it.
  if(fetch_end&&s_frozen&&dt_tail_pending&&dt_tail_end)begin
   dt_tail_pending<=0;dt_tail_end<=0;
   if(!s_fetch_ok)err=err|8'd1;
   else if(s_fetch_addr!=dt_tail_addr)err=err|8'd4;
   else if(fetch_data!=8'h49)err=err|8'd2;
   else dt_complete<=1;
  end
  if(dt_window)begin
   if(dt_elapsed!=24'hFFFFFF)dt_elapsed<=dt_elapsed+1'b1;
   else begin dt_elapsed_overflow<=1;err=err|8'd64;end
   if(dt_hs_rise)begin if(dt_lines!=10'h3FF)dt_lines<=dt_lines+1'b1;else begin dt_line_overflow<=1;err=err|8'd64;end end
   if(dt_raw_rise)begin
    if(dt_raw_count!=15)dt_raw_count<=dt_raw_count+1'b1;else dt_raw_overflow<=1;
    if(dt_raw_count<3)begin
     s_samples[3+dt_raw_count]<=s_trace_snapshot;s_ticks[3+dt_raw_count]<=tick;
     mask[3+dt_raw_count]=1;dt_owner<=dt_raw_count[1:0];dt_owner_valid<=1;
    end else begin dt_owner_valid<=0;err=err|8'd16;end
   end
   if(dt_gb_rise)begin if(dt_gb_count!=15)dt_gb_count<=dt_gb_count+1'b1;else dt_gb_overflow<=1;end
   if(dt_owner_ok)begin
    if(dt_gb)dt_seen_ga[dt_owner_now]<=1;
    if(dt_gb&&!s_trace_snapshot[98]&&!s_trace_snapshot[99])dt_seen_de[dt_owner_now]<=1;
    if(dt_gb&&!s_trace_snapshot[98]&&!s_trace_snapshot[99]&&s_trace_snapshot[100])begin
     if(dt_black[dt_owner_now]!=18'h3FFFF)dt_black[dt_owner_now]<=dt_black[dt_owner_now]+1'b1;
     else begin dt_black_overflow[dt_owner_now]<=1;err=err|8'd32;end
    end
   end
  end
  if(dt_finish_now&&!dt_f7_verified)begin
   dt_f7_verified<=1;s_frozen<=1;dt_running<=0;
   if(!dt_complete||mask[2:0]!=7)err=err|8'd8;
   s_samples[6]<={1'b0,!dt_complete,dt_complete,dt_bracket_started,s_started,err,
     dt_line_overflow,dt_lines,dt_elapsed_overflow,dt_elapsed,
     dt_black_overflow,dt_seen_de,dt_seen_ga,dt_black[2],dt_black[1],dt_black[0],
     dt_gb_overflow,dt_gb_count,dt_raw_overflow,dt_raw_count,mask};
   s_ticks[6]<=dt_complete?dt_end_tick:tick;
  end
  s_error<=err;dt_mask<=mask;
 end
end
wire s_status=s_index=={s_count,1'b0};
wire [2:0] s_sample_index=s_index[3:1];
wire [63:0] s_payload=s_status?{32'h44545231,8'd0,s_error,8'd1,5'd0,s_count}:
 (s_index[0]?s_samples[s_sample_index][127:64]:s_samples[s_sample_index][63:0]);
wire [15:0] s_code=s_status?16'hFF2F:(s_index[0]?16'hFF28:16'hFF20)+{13'd0,s_sample_index};
wire [31:0] s_tick=s_status?s_result_tick:s_ticks[s_sample_index];
`else
// One acquisition per SSM-enable epoch. Pure observers; no machine feedback.
// Capture cut: pre-edge live state when the completed M1 read first goes low.
reg [15:0] s_fetch_addr;
reg s_fetch_ok;
reg s_tail_pending = 0;
reg [1:0] s_tail_pulse = 0;
reg [15:0] s_tail_addr;
reg [7:0] s_tail_op;
reg s_started = 0, s_frozen = 0;
reg [2:0] s_count = 0;
reg [7:0] s_error = 0;
reg [127:0] s_samples [0:6];
reg [31:0] s_ticks [0:6];
reg s_flush = 0, s_done = 0, s_writing = 0, s_header_pending = 0;
reg [3:0] s_index = 0;
reg [31:0] s_result_tick = 0;
reg [63:0] s_rec_a, s_rec_b;
function [15:0] s_pc(input [2:0] i);
 case(i)
  0:s_pc=16'hA4CF; 1:s_pc=16'hA4DE; 2:s_pc=16'hA067;
  3:s_pc=16'hA06B; 4:s_pc=16'h5BFB; 5:s_pc=16'h5B0A;
  default:s_pc=16'h5B17;
 endcase
endfunction
function [7:0] s_op(input [2:0] i);
 case(i)
  0:s_op=8'h06; 1,3:s_op=8'h11; 6:s_op=8'h29; default:s_op=8'hED;
 endcase
endfunction
wire s_target = s_fetch_addr==16'hA4CF || s_fetch_addr==16'hA4DE ||
 s_fetch_addr==16'hA067 || s_fetch_addr==16'hA06B ||
 s_fetch_addr==16'h5BFB || s_fetch_addr==16'h5B0A || s_fetch_addr==16'h5B17;
wire s_header_accepted = s_header_pending && ddram_we && !ddram_busy;
always @(posedge clk) begin
 if (!active || !s_trace_enable) begin
  s_started<=0; s_frozen<=0; s_count<=0; s_error<=0; s_fetch_ok<=0;
  s_tail_pending<=0; s_tail_pulse<=0;
 end else begin
  if (m1_fetch) begin
   s_fetch_addr<=s_trace_addr;
   s_fetch_ok<=s_trace_qualified &&
     (!m1_fetch_d || (s_fetch_ok && s_fetch_addr==s_trace_addr));
  end
  if (s_header_accepted) s_frozen<=1;
  if (fetch_end && !s_frozen) begin
   if (s_tail_pending && s_error==0) begin
    if (!s_fetch_ok) s_error<=1;
    else if (s_fetch_addr!=s_tail_addr) s_error<=4;
    else if (fetch_data!=s_tail_op) s_error<=2;
    else if (s_tail_pulse==1) begin
     s_tail_addr<=16'hA069; s_tail_op<=8'hED; s_tail_pulse<=2;
    end else if (s_tail_pulse==2) begin
     s_tail_addr<=16'hA06A; s_tail_op<=8'h71; s_tail_pulse<=0;
    end else s_tail_pending<=0;
   end else if (!s_started && s_fetch_addr==16'h5AF3) begin
    s_started<=1;
    if (!s_fetch_ok) s_error<=1;
    else if (fetch_data!=8'hCD) s_error<=2;
   end else if (s_started && s_error==0 && s_count<7 && s_target) begin
    if (!s_fetch_ok) s_error<=1;
    else if (s_fetch_addr==s_pc(s_count)) begin
     if (fetch_data!=s_op(s_count)) s_error<=2;
     else begin
      s_samples[s_count]<=s_trace_snapshot;
      s_ticks[s_count]<=tick;
      s_count<=s_count+1'b1;
      // ED prefix completion is the capture cut; validate its next M1 too.
      // A067 must also be followed by the original A069 ED71 R8 clear.
      if (s_count==2 || s_count==4 || s_count==5) begin
       s_tail_pending<=1; s_tail_addr<=s_fetch_addr+16'd1;
       s_tail_op<=s_count==5 ? 8'h71 : 8'h49;
       s_tail_pulse<=s_count==2 ? 2'd1 : 2'd0;
      end
     end
    end else if (!(s_count==1 && s_fetch_addr==16'hA4CF && fetch_data==8'h06))
     s_error<=4;
   end
  end
 end
end
// FF20..26 / FF28..2E are opaque 64-bit payload halves, not raster/B22.
// FF2F status: high32 STR1, error[23:16] (8=incomplete), version[15:8], count[7:0].
wire s_status = s_index=={s_count,1'b0};
wire [2:0] s_sample_index = s_index[3:1];
wire [63:0] s_payload = s_status ?
 {32'h53545231,8'd0,(s_error | (s_count==7 ? 8'd0 : 8'd8)),8'd1,5'd0,s_count} :
 (s_index[0] ? s_samples[s_sample_index][127:64] : s_samples[s_sample_index][63:0]);
wire [15:0] s_code = s_status ? 16'hFF2F :
 (s_index[0] ? 16'hFF28 : 16'hFF20)+{13'd0,s_sample_index};
wire [31:0] s_tick = s_status ? s_result_tick : s_ticks[s_sample_index];
`endif
`endif

//----------------------------------------------------------------------------
// Marker recognition
//----------------------------------------------------------------------------

// SSM v1.2 lists the byte values a marker may carry: #00-#3F, #7F-#9F,
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
reg [20:0] hit_observation = 21'd0;

assign event_stb = hit;

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

			// SSM v1.2 requires all four opcode bytes to be consecutive. The
			// worked example #ED #3F #00 #ED #3E #ED #3D lands here on #00:
			// any intervening opcode resets the first pair, so only #3D3E is
			// emitted. Data/operand reads never enter this fetch-only stream.
			S_ED2: state <= (fetch_data == 8'hED) ? S_HH : S_ED1;

			S_HH: begin
				if (ssm_byte_allowed(fetch_data)) begin
					hit      <= 1'b1;
					hit_code <= {fetch_data, ll};
					// The boundary values latched when this HH fetch
					// completed, two clocks ago.
					hit_tick <= tick_at_fetch;
					hit_observation <= observation_at_fetch;
					hit_pos  <= pos_at_fetch;
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
// Format 2 uses former reserved bits: A[63:59]=observation[20:16],
// B[63:48]=observation[15:0]. Observation: requested[1:0], applied[3:2],
// line estimate CE4[12:4], hs4[13], SHIFT[14], no_hsync[15], mask[16],
// arm[17], training syncs[19:18], reserved zero[20].
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
wire [63:0] capture_a = {hit_observation[20:16], hit_pos, hit_code};
wire [63:0] capture_b = {hit_observation[15:0], hit_tick, event_count[15:0]};

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
`ifdef SSM_S_PASSIVE_TRACE
			s_flush<=0; s_done<=0; s_writing<=0; s_header_pending<=0; s_index<=0;
`ifdef SSM_DT_PASSIVE_TRACE
            dt_header_acked<=0;
`endif
`endif
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
`ifdef SSM_S_PASSIVE_TRACE
			s_flush<=0; s_done<=0; s_writing<=0; s_header_pending<=0; s_index<=0;
`ifdef SSM_DT_PASSIVE_TRACE
            dt_header_acked<=0;
`endif
`endif
		end
	end
	else begin
`ifdef SSM_S_PASSIVE_TRACE
`ifdef SSM_DT_PASSIVE_TRACE
        if(dt_arm_now) dt_header_acked<=0;
        if(s_header_accepted)begin
            s_header_pending<=0;
            if(dt_f7_pair_seen && s_result_tick==dt_f7_marker_tick) dt_header_acked<=1;
        end
        if(s_trace_enable && s_started && !s_done && dt_header_acked && dt_f7_verified) s_flush<=1;
`else
        if (s_header_accepted) begin
            s_header_pending<=0;
            if (s_trace_enable && s_started && !s_done) s_flush<=1;
        end
`endif
`endif
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
					ddram_din  <= {8'd0, RING_ENTRIES, 16'd2, MAGIC};
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
`ifdef SSM_S_PASSIVE_TRACE
                    else if (s_flush && !s_done && !hit) begin
                        s_rec_a<={s_payload[47:0],s_code};
                        s_rec_b<={s_payload[63:48],s_tick,written[15:0]};
                        s_writing<=1;
                        wstate<=W_RECA;
                    end
`endif
				end

				W_RECA: begin
					ddram_addr <= BASE_WORD + 29'd2 + slot_off;
					ddram_din  <=
`ifdef SSM_S_PASSIVE_TRACE
                        s_writing ? s_rec_a :
`endif
                        rec_a;
					ddram_we   <= 1'b1;
					wstate     <= W_RECB;
				end

				W_RECB: begin
					ddram_addr <= BASE_WORD + 29'd3 + slot_off;
					ddram_din  <=
`ifdef SSM_S_PASSIVE_TRACE
                        s_writing ? s_rec_b :
`endif
                        rec_b;
					ddram_we   <= 1'b1;
					wstate     <= W_HDR;
				end

				W_HDR: begin
					ddram_addr  <= BASE_WORD + 29'd1;
					ddram_din   <= {24'd0, dropped_count, written + 32'd1};
					ddram_we    <= 1'b1;
					written     <= written + 1'd1;
					pub_dropped <= dropped_count;
`ifdef SSM_S_PASSIVE_TRACE
                    if (s_writing) begin
                        s_writing<=0;
                        if (s_status) begin s_done<=1; s_flush<=0; end
                        else s_index<=s_index+1'b1;
                    end else begin
                        pend<=0;
                        if (rec_a[15:0]==
`ifdef SSM_DT_PASSIVE_TRACE
                            16'h02F7
`else
                            16'h02D3
`endif
                        ) begin
                            s_header_pending<=1; s_result_tick<=rec_b[47:16];
                        end
                    end
`else
					pend        <= 1'b0;
`endif
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
