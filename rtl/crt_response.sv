// SPDX-License-Identifier: GPL-2.0-or-later
// Qualitative receiver/renderer for the explicit experimental CRT-response profile.
// Contract: docs/investigations/video-boundary/v1-rtl-candidate-2026-11-06.md
// No CRTC chip rules are implemented here: this is an explicitly qualitative
// digital monitor, with fixed frozen-preset gains and no producer feedback.
module crt_response (
    input logic clk, ce_16, reset, context_clear, owner,
    input logic ga_hs, ga_vs, source_missing,
    input logic [11:0] rgb12,
    input logic raw_vblank, hs, vs, hblank, vblank, field, frame_field,
    output logic [11:0] rgb12_out,
    output logic raw_vblank_out, available,
    output wire hs_out, vs_out, hblank_out, vblank_out, field_out, frame_field_out
);
    // Full-clock tuple transport: the value sampled at edge t appears after
    // edge t+256. This pipeline continues during every lifecycle invalidation.
    logic [5:0] timing_delay [0:256];
    assign {hs_out,vs_out,hblank_out,vblank_out,field_out,frame_field_out} = timing_delay[256];
    logic signed [6:0] offset_delay [0:255];
    logic [12:0] history [0:127];
    logic [6:0] write_ptr = 0;
    logic [7:0] fill = 0;
    logic old_owner=0, old_hs=0, old_ga=0, old_vs=0, old_missing=0;
    logic [15:0] tick=0;

    // Modular timestamps: all admitted live intervals fit signed 16-bit
    // subtraction, including two windows and a capped tip across rollover.
    function automatic integer distance(input logic [15:0] a, b);
        distance = int'($signed(a-b));
    endfunction
    function automatic integer magnitude(input integer v);
        magnitude = v<0 ? -v : v;
    endfunction
    function automatic integer clip(input integer v, lo, hi);
        clip = v<lo ? lo : (v>hi ? hi : v);
    endfunction

    logic primed=0, have_hs=0, pulse_open=0, wait_low=1, pulse_bad=0;
    logic [15:0] last_hs, rise_tick;
    integer period_nominal, nominal_tip, coordinate, healthy;
    integer candidate_period, candidate_tip, candidate_width;
    integer line_count, line_rise, line_width;
    integer absolute_phase, line_phase, line_offset;
    // Strict-prior eligibility is explicit: retain the previous completed
    // result when another window finalizes on this acquisition HS edge.
    integer latest_phase, previous_phase;
    logic latest_valid=0, previous_valid=0;
    logic [15:0] latest_tick, previous_tick;

    // Two physical-coordinate records. Rise ownership never changes when
    // acquisition moves, including an old high spanning the newer deadline.
    logic win_valid [0:1], win_closed [0:1];
    logic [15:0] win_base [0:1], win_centre [0:1], win_deadline [0:1];
    integer win_coordinate [0:1], win_count [0:1], win_pulls [0:1];
    integer win_best_error [0:1], win_best_width [0:1], win_best_pull [0:1];
    integer active_window, oldest_window, pulse_owner;

    // One same-edge arithmetic event. Capture the original finalize operands
    // before acquisition HS can change the clipping origin or clear the epoch.
    logic finish_pending, finish_has_tip;
    integer finish_coordinate, finish_error, finish_width;
    integer finish_pulls, finish_best_pull, finish_clip_coordinate;
    integer finish_target;

    task automatic clear_receiver;
        integer k;
        begin
            // An HS guard may cancel an event before the shared tail runs.
            finish_pending=0;
            primed=0; have_hs=0; healthy=0; line_count=0;
            candidate_period=0; candidate_tip=0; candidate_width=0;
            pulse_open=0; wait_low=ga_hs || ga_vs; pulse_bad=0;
            // Invalidation discards the estimate, not the already emitted line.
            // A receiver-only clear changes displacement at the next own HS;
            // pixel lifecycle invalidation separately clears it immediately.
            absolute_phase=0; coordinate=0; line_phase=0;
            latest_valid=0; previous_valid=0;
            active_window=0; oldest_window=0; pulse_owner=-1;
            for(k=0;k<2;k=k+1) begin win_valid[k]=0; win_closed[k]=0; end
        end
    endtask
    task automatic start_window(input integer w, input logic [15:0] base_tick);
        begin
            win_valid[w]=1; win_closed[w]=0; win_base[w]=base_tick;
            win_coordinate[w]=coordinate;
            win_centre[w]=16'(int'(base_tick)+coordinate);
            win_deadline[w]=16'(int'(base_tick)+coordinate+period_nominal-period_nominal/2);
            win_count[w]=0; win_pulls[w]=0;
            win_best_error[w]=0; win_best_width[w]=0; win_best_pull[w]=0;
        end
    endtask
    task automatic add_tip(input integer w, width, input logic [15:0] rise);
        integer error, pull;
        begin
            error=distance(rise,win_centre[w]);
            // Frozen gains -16/+8 dots, width weighted against ref256.
            // In Q8, width*gain is exact (ref and fraction cancel).
            pull=error<0 ? -(width<<<4) : (width<<<3);
            win_pulls[w]=win_pulls[w]+pull;
            if(win_count[w]==0 || magnitude(error)<magnitude(win_best_error[w]) ||
               (magnitude(error)==magnitude(win_best_error[w]) && error<win_best_error[w])) begin
                win_best_error[w]=error; win_best_width[w]=width; win_best_pull[w]=pull;
            end
            win_count[w]=win_count[w]+1;
        end
    endtask
    task automatic complete_tip(input logic qualified);
        integer width;
        begin
            width=distance(tick,rise_tick);
            if(qualified && !pulse_bad && width>=8 && width<=256) begin
                line_count=line_count+1; line_rise=distance(rise_tick,last_hs); line_width=width;
                if(primed && pulse_owner>=0 && win_valid[pulse_owner]) add_tip(pulse_owner,width,rise_tick);
            end
            pulse_open=0;
        end
    endtask
    task automatic finish_windows;
        integer k, w;
        begin
            for(k=0;k<2;k=k+1) begin
                w=oldest_window;
                if(primed && win_valid[w] && win_closed[w] && !(pulse_open && pulse_owner==w)) begin
                    // With L>=641 and opening coordinates within +/-192,
                    // consecutive deadlines are >=257 clocks apart. An old
                    // owned pulse releases by deadline+255, so only one event
                    // can finalize per edge. See the timing-sharing proof in
                    // docs/investigations/video-boundary/v1-rtl-candidate-2026-11-06.md.
                    // synthesis translate_off
                    if(finish_pending) $fatal(1,"multiple same-edge finalizations violate admission bounds");
                    // synthesis translate_on
                    finish_pending=1; finish_has_tip=(win_count[w]!=0);
                    finish_coordinate=win_coordinate[w]; finish_error=win_best_error[w];
                    finish_width=win_best_width[w]; finish_pulls=win_pulls[w];
                    finish_best_pull=win_best_pull[w]; finish_clip_coordinate=coordinate;
                    previous_valid=latest_valid; previous_tick=latest_tick; previous_phase=latest_phase;
                    // Placeholder old phase: same-edge HS excludes this tick.
                    // The shared tail replaces it unless the epoch is cleared.
                    latest_valid=1; latest_tick=tick; latest_phase=absolute_phase;
                    win_valid[w]=0; oldest_window=1-w;
                end
            end
        end
    endtask

    integer i, p, step, age;
    logic invalidate_pixels;
    logic [15:0] base_next;
    always @(posedge clk) begin
        finish_pending=0; finish_has_tip=0;
        finish_coordinate=0; finish_error=0; finish_width=0;
        finish_pulls=0; finish_best_pull=0; finish_clip_coordinate=0; finish_target=0;
        tick <= tick+16'd1;
        timing_delay[0] <= {hs,vs,hblank,vblank,field,frame_field};
        for(i=1;i<257;i=i+1) timing_delay[i] <= timing_delay[i-1];
        invalidate_pixels=reset || context_clear || owner!=old_owner;
        if(invalidate_pixels) begin clear_receiver(); line_offset=0; end
        else if(source_missing || source_missing!=old_missing) clear_receiver();
        // Bound elapsed time before modular timestamps can alias a much longer
        // physical line into an admitted one. Only receiver state is cleared.
        else if(have_hs && distance(tick,last_hs)>=16384) clear_receiver();
        else begin
            // Fall at the exact cap is admitted before overlong rejection.
            if(ga_vs && !old_vs) begin pulse_bad=1; wait_low=1; end
            if(!ga_hs) begin
                if(old_ga && pulse_open) complete_tip(1);
                if(!ga_vs) wait_low=0;
            end
            if(pulse_open && distance(tick,rise_tick)>=256) begin complete_tip(0); wait_low=1; end
            finish_windows();
            // Half-open deadlines seal before an equal-clock new rise. An
            // already-high tip retains its stamped owner through completion.
            if(primed && distance(tick,win_deadline[active_window])>=0) begin
                base_next=win_base[active_window]+16'(period_nominal);
                win_closed[active_window]=1;
                finish_windows();
                if(win_valid[1-active_window]) clear_receiver();
                else begin active_window=1-active_window; start_window(active_window,base_next); end
            end
            if(ga_hs && !old_ga && !ga_vs && !wait_low) begin
                pulse_open=1; pulse_bad=0; rise_tick=tick;
                pulse_owner=primed ? active_window : -1;
            end
            // A new rise cannot introduce a newly ready window. Retain this
            // original collector call deliberately while sharing arithmetic.
            finish_windows();
            if(hs && !old_hs) begin
                p=distance(tick,last_hs);
                if(primed) begin
                    step=p-period_nominal;
                    if(magnitude(step)>192 || magnitude(coordinate+step)>192) clear_receiver();
                    else coordinate=coordinate+step;
                end
                if(!primed && have_hs) begin
                    // MODEL LIMIT (B26): two records require L>640 for the
                    // frozen ±192 coordinate/256 cap bounds. Signed timestamp
                    // arithmetic additionally bounds L<=16383; other periods
                    // deliberately retain native position and re-prime.
                    // See docs/investigations/video-boundary/v1-rtl-candidate-2026-11-06.md.
                    if(line_count==1 && p>640 && p<=16383) begin
                        if(p==candidate_period && line_rise==candidate_tip && line_width==candidate_width) healthy=healthy+1;
                        else begin candidate_period=p; candidate_tip=line_rise; candidate_width=line_width; healthy=1; end
                        if(healthy>=8) begin
                            period_nominal=p; nominal_tip=line_rise; coordinate=0; absolute_phase=0;
                            primed=1; active_window=0; oldest_window=0;
                            base_next=tick+16'(nominal_tip);
                            if(nominal_tip>p/2) base_next=base_next-16'(p);
                            start_window(0,base_next);
                            // Completed pre-prime tip belongs only if in the
                            // newly opened physical window; no double counting.
                            if(distance(last_hs+16'(line_rise),win_deadline[0])<0 &&
                               distance(last_hs+16'(line_rise),win_centre[0])>=-p/2)
                                add_tip(0,line_width,last_hs+16'(line_rise));
                            if(pulse_open) begin
                                if(distance(rise_tick,win_centre[0])>=-p/2 &&
                                   distance(rise_tick,win_deadline[0])<0) pulse_owner=0;
                                else pulse_owner=-1;
                            end
                        end
                    end else begin healthy=0; candidate_period=0; end
                end
                if(primed) begin
                    if(latest_valid && distance(tick,latest_tick)>0) line_phase=latest_phase;
                    else if(previous_valid && distance(tick,previous_tick)>0) line_phase=previous_phase;
                    line_offset=clip((line_phase-coordinate*64+128)>>>8,-48,48);
                end else line_offset=0;
                last_hs=tick; have_hs=1; line_count=0;
            end
            // Same-edge tail after HS control: a just-finished result was not
            // eligible at this HS. This separates phase arithmetic from offset
            // selection without delaying publication. Use captured pre-HS clip
            // origin; clear_receiver cancels this event on an invalid step.
            if(finish_pending) begin
                if(finish_has_tip) begin
                    finish_target=finish_coordinate*64 + finish_error*64 +
                        (finish_width-256)*32 + finish_pulls-finish_best_pull;
                    finish_target=clip(finish_target,finish_clip_coordinate*64-12288,
                        finish_clip_coordinate*64+12288);
                    absolute_phase=absolute_phase+((finish_target-absolute_phase)>>>3);
                end
                latest_phase=absolute_phase;
            end
        end
        // A clear branch may consume this very HS edge. Commit fallback and
        // retain the fresh line anchor even when normal collection was skipped.
        if(!invalidate_pixels && !primed && hs && !old_hs) begin
            line_offset=0; last_hs=tick; have_hs=1; line_count=0;
        end
        old_owner<=owner; old_hs<=hs; old_ga<=ga_hs; old_vs<=ga_vs; old_missing<=source_missing;
        // Each sampled line displacement travels with its own HS through an
        // entire delay pipeline. No pending-register overwrite on short lines.
        if(invalidate_pixels) begin
            for(i=0;i<256;i=i+1) offset_delay[i] <= 0;
            write_ptr<=0; fill<=0; rgb12_out<=0; raw_vblank_out<=1; available<=0;
        end else begin
            offset_delay[0]<=7'(line_offset);
            for(i=1;i<256;i=i+1) offset_delay[i]<=offset_delay[i-1];
            if(ce_16) begin
                // Synchronous RAM: read address uses PRE-write pointer/fill.
                // On edge t, emit history[t/4-age] while writing t/4. The old
                // offset_delay[255] is the offset sampled at t-256, aligned
                // with the tuple appearing from timing_delay[256] this edge.
                // Ages16..112 cannot collide with the same-edge write.
                age=64-int'($signed(offset_delay[255]));
                history[write_ptr]<={raw_vblank,rgb12};
                write_ptr<=write_ptr+7'd1;
                if(fill<128) fill<=fill+8'd1;
                available<=int'(fill)>=age;
                if(int'(fill)>=age) {raw_vblank_out,rgb12_out}<=history[7'(int'(write_ptr)-age)];
                else begin rgb12_out<=0; raw_vblank_out<=1; end
            end
        end
    end
endmodule
