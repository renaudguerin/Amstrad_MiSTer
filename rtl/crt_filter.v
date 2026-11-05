//------------------------------------------------------------------------------
//
// Extracted to separate entity, converted to verilog, optimized and tweaked
// (c) 2018 Sorgelig
//
//------------------------------------------------------------------------------
//
//    {@{@{@{@{@{@
//  {@{@{@{@{@{@{@{@  This code is covered by CoreAmstrad synthesis r005
//  {@    {@{@    {@  A core of Amstrad CPC 6128 running on MiST-board platform
//  {@{@{@{@{@{@{@{@
//  {@  {@{@{@{@  {@  CoreAmstrad is implementation of FPGAmstrad on MiST-board
//  {@{@        {@{@   Contact : renaudhelias@gmail.com
//  {@{@{@{@{@{@{@{@   @see http://code.google.com/p/mist-board/
//    {@{@{@{@{@{@     @see FPGAmstrad at CPCWiki
//
//------------------------------------------------------------------------------

// https://sourceforge.net/p/jemu/code/HEAD/tree/JEMU/src/jemu/system/cpc/GateArray.java

// altera message_off 10027
module crt_filter
(
	input      CLK,
	input      CE_4,
	input      HSYNC_I,
	input      VSYNC_I,
	output reg HSYNC_O,
	output reg VSYNC_O,
	output reg HBLANK,
	output     HBLANK_LIVE,
	output reg VBLANK,
	output     SHIFT,
	output reg EXTRA_BYTE,
	output reg START_GEN,
	output reg END_GEN,
	// Passive acquisition/history state; bit layout is the SSM v2 payload
	// above the two requested/applied mode fields supplied by the motherboard.
	output [15:0] SYNC_OBSERVATION
);

wire resync = 1;
reg hs4,shift;
// PRIVATE bounded measured-pair output phase experiment. The legacy
// period/SHIFT reference remains independent of this output offset.
reg legacy_hsync;
reg phase_epoch, phase_pair_valid;
reg signed [10:0] phase_first, phase_pending, phase_active;
reg [9:0] phase_age;
reg [8:0] phase_period;
initial begin
    phase_epoch = 0; phase_pair_valid = 0;
    phase_first = 0; phase_pending = 0; phase_active = 0;
    phase_age = 1023; phase_period = 0;
end

// French ACCC v1.11 section14.4 pp134-135: the ordinary4/5 pair
// differs by eight M2 pixels. This classifies the measured pulse bin,
// not numeric R3 or arbitrary JIT identity; acquisition/SHIFT stay intact.
initial begin EXTRA_BYTE = 0; START_GEN = 0; END_GEN = 0; end
// Module scope exposes observation without synthesis hierarchical references.
reg [8:0] hSyncSize;
reg       hSyncReg;
reg [1:0] syncs;
assign SHIFT = shift ^ hs4;

// generate HSync if original is absent for almost whole frame
reg hsync, no_hsync;

// Hybrid live blanking for the scaler.  HSYNC_I is the Gate Array/ASIC force-
// blank event and therefore owns the live horizontal phase, but the pulse is
// not by itself a complete acquisition blank.  The production ratio is
// 64 MHz CLK / 4 MHz CE_4, so 1024 master clocks preserve the 64-CE Full
// blank width without quantising a three-pixel-M2 (12-master-clock) R2.JIT
// displacement to the next CE_4 edge.  CRTC/ASIC HSYNC changes after a CLK
// edge; the remainder therefore starts when that edge is observed one CLK
// later and expires exactly 1024 clocks after the original pin transition.
// Further edges inside the window do not restart it.  A longer live pulse
// still wins directly, and the established Full blank takes over when the
// filter has classified horizontal sync as absent.
localparam [9:0] LIVE_HBLANK_REMAINDER = 10'd1022;
reg live_hblank_ext;
reg old_live_hsync;
reg [9:0] live_hblank_count;

initial begin
	live_hblank_ext = 0;
	old_live_hsync = 0;
	live_hblank_count = 0;
end

assign HBLANK_LIVE = no_hsync ? HBLANK : (HSYNC_I | live_hblank_ext);

always @(posedge CLK) begin : liveblankgen
	old_live_hsync <= HSYNC_I;
	// A transition observed on the expiry edge belongs to the next window.
	// Earlier in-window transitions remain ignored so malformed short cadence
	// cannot extend the acquisition blank indefinitely.
	if (~old_live_hsync & HSYNC_I &
			(~live_hblank_ext || live_hblank_count == 0)) begin
		live_hblank_ext <= 1;
		live_hblank_count <= LIVE_HBLANK_REMAINDER;
	end
	else if (live_hblank_ext) begin
		if (live_hblank_count == 0)
			live_hblank_ext <= 0;
		else
			live_hblank_count <= live_hblank_count - 1'd1;
	end
end

always @(posedge CLK) begin : hsyncgen
	reg [15:0] dcnt;
	reg [10:0] hsz, hcnt;

	reg old_vsync, old_hsync;
	reg no_hsync_next;
	
	if(CE_4) begin
		if(&dcnt) begin
			no_hsync_next <= 1;
			// VSYNC normally commits the absence detector, but a missing or
			// stuck VSYNC must not leave Live blanking permanently asserted.
			// Reuse the cadence learned from healthy raw HSYNC edges.
			if(|hsz) no_hsync <= 1;
		end
		else dcnt <= dcnt + 1'd1;
		
		old_hsync <= HSYNC_I;
		if(~old_hsync & HSYNC_I) begin
			dcnt <= 0;
			if(!no_hsync) begin
				hsz <= dcnt[10:0];
			end
			else if(!(|hsz)) begin
				hsz <= dcnt[10:0];
				hsync <= 1;
				hcnt <= 0;
			end
		end
		
		if(no_hsync && (|hsz)) begin
			hcnt <= hcnt + 1'd1;
			if(hcnt == 13) hsync <= 0;
			if(hcnt == hsz) begin
				hsync <= 1;
				hcnt <= 0;
			end
		end
		
		old_vsync <= VSYNC_I;
		if(~old_vsync & VSYNC_I) begin
			no_hsync <= no_hsync_next;
			no_hsync_next <= 0;
			hsz <= 0;
		end
	end

end

reg hsync_mask;
initial hsync_mask = 0; // separate from the decl: newer Verilator flags
                        // declaration initializers later assigned in
                        // always blocks (PROCASSINIT), older ones reject
                        // the lint name — this form is portable.
// check for too frequent HSYNCs (S&KOH)
always @(posedge CLK) begin : hsyncfilt
	reg old_hsync;
	reg [8:0] line_time;

	if(CE_4) begin
		old_hsync <= HSYNC_I;
		if (hsync_mask) begin
			if (~&line_time) line_time <= line_time + 1'd1;
			if (!HSYNC_I & line_time >= 190) hsync_mask <= 0; // clear the mask after enough time
		end

		if (HSYNC_I & ~old_hsync & !hsync_mask) line_time <= 0; // new line detected
		if (~HSYNC_I & old_hsync & !hsync_mask) hsync_mask <= 1; // start to mask after the first hsync seen
	end
end

wire hsync_i = no_hsync ? hsync : (HSYNC_I & ~hsync_mask);

// RGB-only metadata follows complete pulses in the existing admitted stream.
// Seed1 counts the rise sample; a fall16 CE4 later observes width16.
// Saturation prevents a long pulse wrapping into the ordinary4 bin.
reg rgb_meta_old_hs, rgb_meta_open, rgb_meta_real;
reg [5:0] rgb_meta_width;
initial begin
    rgb_meta_old_hs = 0; rgb_meta_open = 0; rgb_meta_real = 0;
    rgb_meta_width = 0;
end
always @(posedge CLK) begin : rgbmetadata
    if (CE_4) begin
        rgb_meta_old_hs <= hsync_i;
        if (!rgb_meta_old_hs && hsync_i) begin
            rgb_meta_open <= 1;
            rgb_meta_width <= 6'd1;
            rgb_meta_real <= !no_hsync;
            if (!no_hsync) START_GEN <= ~START_GEN;
        end else if (rgb_meta_open && hsync_i && !(&rgb_meta_width)) begin
            rgb_meta_width <= rgb_meta_width + 1'd1;
        end
        // Fallback pulses may close metadata, but never claim a raw4 class.
        if (no_hsync) rgb_meta_real <= 0;
        if (rgb_meta_old_hs && !hsync_i && rgb_meta_open) begin
            EXTRA_BYTE <= rgb_meta_real && !no_hsync &&
                          (rgb_meta_width == 6'd16);
            END_GEN <= ~END_GEN;
            rgb_meta_open <= 0;
            rgb_meta_real <= 0;
        end
    end
end

assign SYNC_OBSERVATION = {syncs, hSyncReg, hsync_mask, no_hsync, SHIFT, hs4, hSyncSize};


// Generate HSync,VSync for monitor
// HSync: delayed by 2us for set, immediate reset and limited by 4us.
// VSync: delayed by 2 lines for set, immediate reset and limited by 2 lines.
always @(posedge CLK) begin : syncgen
	reg       old_hsync;
	reg       old_vsync,old_vs;
	reg [8:0] hSyncCount;
	reg [9:0] hSyncCount2x;
	reg [3:0] vSyncCount;
	reg [8:0] vSyncFlt;
	integer phase_value, phase_sum, phase_effective;
	reg phase_anchor;

	localparam HFLT_SZ = 50*4;
	localparam VFLT_SZ = 260;

	if(CE_4) begin
		old_hsync <= hsync_i;

		if(resync) begin
			if(~&hSyncCount) hSyncCount = hSyncCount + 1'd1;
			if(~old_hsync & hsync_i) old_vs <= VSYNC_I;

			//re-align restored hsync to the first hsync of vsync
			if((~old_vs & VSYNC_I & ~old_hsync & hsync_i) || (hSyncCount >= hSyncSize)) begin
				hSyncCount = 0;
				if(~old_hsync & hsync_i) begin
					hSyncReg <= 1;
				end
			end
			
			// Calc line size from length of 2 first lines after VSync
			// 2 lines are needed to neutralize fake interlace video
			if(~&hSyncCount2x) hSyncCount2x = hSyncCount2x + 1'd1;
			if(~old_hsync & hsync_i) begin
				// The estimate below consumes the updated training count on this edge.
				// Preserve the existing blocking order after exposing syncs for observation.
				/* verilator lint_off BLKSEQ */
				if(~VSYNC_I & ~&syncs) syncs = syncs + 1'd1;
				if(VSYNC_I) {syncs,hSyncCount2x} = 0;
				/* verilator lint_on BLKSEQ */
				if(syncs == 2) hSyncSize <= hSyncCount2x[9:1];
			end
		end
		else begin
			if(hSyncCount < HFLT_SZ) hSyncCount = hSyncCount + 1'd1;
			else if(~old_hsync & hsync_i) begin
				hSyncCount = 0;
				hSyncReg <= 1;
			end
		end

		if(old_hsync & ~hsync_i & hSyncReg) begin
			hSyncReg <= 0;
			// This legacy SHIFT/hs4 count includes the current CE_4 edge
			// through the blocking update above.
			if(hSyncCount > 7*4) hs4 <= 0;
			if((hSyncCount >= 4*4-1) && (hSyncCount < 6*4-1)) begin
				if(hSyncCount == 4*4-1) hs4 <= 1;
				shift <= 1;
			end
		end

		if(hSyncCount == 2*4) begin
			legacy_hsync <= 1;
			shift <= 0;
			old_vsync <= VSYNC_I;
			
			if(~&vSyncFlt) vSyncFlt <= vSyncFlt + 1'd1;

			if(VSYNC_I) begin
				if(~old_vsync && (vSyncFlt > VFLT_SZ)) begin
					vSyncCount = 0;
					vSyncFlt <= 0;
				end
				else if(~&vSyncCount) vSyncCount = vSyncCount + 1'd1;
			end
			
			if(vSyncCount == 1) VSYNC_O <= 1;
			if(!vSyncCount || (vSyncCount == 3)) VSYNC_O <= 0;
		end

		//force VSYNC disable earlier
		if(~VSYNC_I) VSYNC_O <= 0;

		if(hSyncCount == 6*4) legacy_hsync <= 0;

        // Measurement uses the updated legacy count; output phase never
        // feeds back into that counter, training, width or SHIFT logic.
        phase_anchor = (~old_vs & VSYNC_I & ~old_hsync & hsync_i);
        phase_value = $signed({23'd0, hSyncCount});
        if (phase_value > ($signed({23'd0, hSyncSize}) >>> 1))
            phase_value = phase_value - $signed({23'd0, hSyncSize});
        phase_sum = $signed({{21{phase_first[10]}}, phase_first}) + phase_value;
        phase_effective = $signed({{21{phase_active[10]}}, phase_active});
        if (hSyncCount == 0) begin
            phase_effective = $signed({{21{phase_pending[10]}}, phase_pending});
            phase_active <= phase_pending;
        end
        if (phase_age != 1023) phase_age <= phase_age + 1'd1;
        if (no_hsync || hSyncSize < 32 || hSyncSize != phase_period) begin
            phase_epoch <= 0;
            phase_pair_valid <= 0;
            phase_pending <= 0;
        end
        if (phase_anchor) begin
            phase_epoch <= !no_hsync;
            phase_pair_valid <= 0;
            phase_pending <= 0;
            phase_active <= 0;
            phase_effective = 0;
            phase_period <= hSyncSize;
        end
        if (~old_hsync & hsync_i) begin
            phase_age <= 0;
            if (phase_epoch && !phase_anchor && !no_hsync &&
                    hSyncSize >= 32 && phase_period == hSyncSize && syncs == 3) begin
                // A one-line interval is P + current_phase - previous_phase.
                // Two samples in the unchanged +/-8 cohort can differ by16.
                if (phase_value >= -8 && phase_value <= 8 &&
                        phase_age + 1 >= hSyncSize - 16 &&
                        phase_age + 1 <= hSyncSize + 16) begin
                    if (!phase_pair_valid) begin
                        phase_first <= phase_value[10:0];
                        phase_pair_valid <= 1;
                    end else begin
                        phase_pair_valid <= 0;
                        if ((phase_sum % 2) == 0)
                            // Even signed /2, narrowed explicitly to11bits.
                            phase_pending <= phase_sum[11:1];
                        else phase_pending <= 0;
                    end
                end else begin
                    phase_epoch <= 0;
                    phase_pair_valid <= 0;
                    phase_pending <= 0;
                end
            end else begin
                phase_pair_valid <= 0;
                phase_pending <= 0;
            end
        end
        // A completed pair is pending until the following reference origin.
        // The effective value also covers an offset-8 rise on that origin.
        if ($signed({23'd0, hSyncCount}) == 32'sd8 + phase_effective) HSYNC_O <= 1;
        if ($signed({23'd0, hSyncCount}) == 32'sd24 + phase_effective) HSYNC_O <= 0;

	end
end

always @(posedge CLK) begin : blankgen

	localparam  BEGIN_VBORDER = 4 * 8 - 2;
	localparam  END_VBORDER = 37 * 8 + 6;

	localparam  BEGIN_HBORDER = 49;
	localparam  END_HBORDER = 241;

	reg old_vs;
	reg old_hs;
	reg old_line;

	reg [8:0] vborder;
	reg [8:0] hborder;

	if (CE_4) begin

		if(~&hborder) hborder <= hborder + 1'd1;
		old_hs <= HSYNC_O;
		if (~old_hs & HSYNC_O) begin
			hborder <= 0;
			HBLANK <= 1;
        end

        // Retain the original vertical line clock; blank boundaries below
        // share the actual horizontal acquisition phase.
        old_line <= legacy_hsync;
        if (~old_line & legacy_hsync) begin
			if(~&vborder) vborder <= vborder + 1'd1;
			old_vs <= VSYNC_O;
			if(~old_vs & VSYNC_O) begin
				vborder <= 0;
				VBLANK <= 1;
			end
		end

		if(hborder == BEGIN_HBORDER) begin
			HBLANK <= 0;
		end
        if (hborder == BEGIN_HBORDER) begin
            if(vborder == BEGIN_VBORDER) VBLANK <= 0;
		end
		
		if(hborder == END_HBORDER) begin
			HBLANK <= 1;
		end
        if (hborder == END_HBORDER) begin
            if(vborder == END_VBORDER) VBLANK <= 1;
		end
	end
end

endmodule

// Keep the mode contract in one production-owned seam so the focused fixture
// can pin the complete output tuple rather than only crt_filter internals.
/* verilator lint_off DECLFILENAME */
module crt_filter_output_select
(
	input  [1:0] MODE,
	input        HSYNC_FILTERED,
	input        VSYNC_FILTERED,
	input        HBLANK_FILTERED,
	input        VBLANK_FILTERED,
	input        HSYNC_RAW,
	input        VSYNC_RAW,
	input        HBLANK_RAW,
	input        VBLANK_RAW,
	output       HSYNC_OUT,
	output       VSYNC_OUT,
	output       HBLANK_OUT,
	output       VBLANK_OUT
);

// B6 output policy (docs/investigations/video-boundary/b6-video-boundary.md,
// "Mode and transition contract").  Full (0) and Raw pixels (1) share the
// complete filtered acquisition tuple: raw sync effects live in pixel values,
// never as holes in acquisition DE.  Only Raw CRT (2) substitutes the
// GA-shaped monitor sync, the raw CRTC horizontal blank and the GA vertical
// blank.  The caller's
// applied-mode register normalises reserved value 3 to Full; comparing
// against 2 alone keeps this seam safe even if it does not.
assign HSYNC_OUT  = (MODE != 2'd2) ? HSYNC_FILTERED  : HSYNC_RAW;
assign VSYNC_OUT  = (MODE != 2'd2) ? VSYNC_FILTERED  : VSYNC_RAW;
assign HBLANK_OUT = (MODE != 2'd2) ? HBLANK_FILTERED : HBLANK_RAW;
assign VBLANK_OUT = (MODE != 2'd2) ? VBLANK_FILTERED : VBLANK_RAW;

endmodule
/* verilator lint_on DECLFILENAME */
