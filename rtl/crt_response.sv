// SPDX-License-Identifier: GPL-2.0-or-later
// Qualitative receiver/renderer for the explicit experimental CRT-response profile.
// Contract: docs/investigations/video-boundary/v1-rtl-candidate-2026-11-06.md
// No CRTC chip rules are implemented here: this is an explicitly qualitative
// digital monitor, with fixed frozen-preset gains and no producer feedback.
//
// Timing structure. The receiver is split in two so no clock carries the
// whole event chain:
//  - the FRONT keeps the same-edge event order of the reviewed single-edge
//    receiver (tips, windows, coordinate, priming) but holds every interval
//    as an age register (tick - stamp, modulo 2^16) that increments each
//    clock, so no decision waits for a timestamp subtraction;
//  - the BACK owns the phase estimate and the line displacement. It consumes
//    one event record per clock through four register stages. Nothing in the
//    front reads back state, so the back is the front's view delayed by a
//    fixed number of clocks. The displacement register therefore stands in
//    for stage 4 of the former 256-stage displacement delay and the remaining
//    chain is 251 stages: presentation latency is unchanged.
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
    // line_offset is the displacement sampled four clocks ago; offset_delay[j]
    // is the one sampled 5+j clocks ago.
    logic signed [6:0] line_offset = 0;
    logic signed [6:0] offset_delay [0:250];
    logic [12:0] history [0:127];
    logic [6:0] write_ptr = 0;
    logic [7:0] fill = 0;
    logic old_owner=0, old_hs=0, old_ga=0, old_vs=0, old_missing=0;
    logic [15:0] tick=0;

    function automatic logic [16:0] mag16(input logic signed [15:0] v);
        logic signed [16:0] e;
        begin e=v; mag16 = e<0 ? -e : e; end
    endfunction

    // ---------------------------------------------------------------- front
    logic primed=0, have_hs=0, pulse_open=0, wait_low=1, pulse_bad=0;
    // Ages valid for the current edge: tick-last_hs, tick-rise_tick and, per
    // window, tick-base, tick-centre, tick-deadline. All modulo 2^16: every
    // admitted live interval fits signed 16-bit subtraction.
    logic signed [15:0] a_hs_q=0, a_rise_q=0;
    logic signed [15:0] d_base_q [0:1], d_ctr_q [0:1], d_dl_q [0:1];
    logic signed [15:0] a_hs, a_hs_n, a_rise, a_rise_n;
    logic signed [15:0] d_base [0:1], d_ctr [0:1], d_dl [0:1];
    logic signed [15:0] d_base_n [0:1], d_ctr_n [0:1], d_dl_n [0:1];

    logic signed [15:0] period_nominal=0, half_up=0;
    logic signed [9:0] coordinate=0;
    logic [3:0] healthy=0;
    logic [1:0] line_count=0;           // saturating: only ==1 is observed
    logic signed [15:0] candidate_period=0, candidate_tip=0, line_rise=0;
    logic [9:0] candidate_width=0, line_width=0;

    // Two physical-coordinate records. Rise ownership never changes when
    // acquisition moves, including an old high spanning the newer deadline.
    logic win_valid [0:1], win_closed [0:1], win_has [0:1];
    logic signed [9:0] win_coordinate [0:1];
    logic signed [24:0] win_pulls [0:1];
    logic signed [15:0] win_best_error [0:1];
    logic [9:0] win_best_width [0:1];
    logic signed [13:0] win_best_pull [0:1];
    logic active_window=0, oldest_window=0, pulse_owner=0, pulse_owned=0;

    // Event record handed to the back at the end of the edge.
    logic f_pending, f_has, did_clear, did_prime;
    logic signed [9:0] f_coord, f_clipc;
    logic signed [15:0] f_err;
    logic [9:0] f_width;
    logic signed [24:0] f_pulls;
    logic signed [13:0] f_bpull;

    task automatic clear_receiver;
        begin
            // An HS guard may cancel a finalization captured on this edge.
            f_pending=0; did_clear=1;
            primed=0; have_hs=0; healthy=0; line_count=0;
            candidate_period=0; candidate_tip=0; candidate_width=0;
            pulse_open=0; wait_low=ga_hs || ga_vs; pulse_bad=0;
            // Invalidation discards the estimate, not the already emitted line.
            // A receiver-only clear changes displacement at the next own HS;
            // pixel lifecycle invalidation separately clears it immediately.
            coordinate=0;
            active_window=0; oldest_window=0; pulse_owned=0;
            win_valid[0]=0; win_valid[1]=0; win_closed[0]=0; win_closed[1]=0;
        end
    endtask
    // db is tick-base for the new window; hu is period-period/2.
    task automatic start_window(input logic w, input logic signed [15:0] db, hu);
        logic signed [15:0] c16;
        begin
            c16=16'(coordinate);
            win_valid[w]=1; win_closed[w]=0;
            d_base[w]=db;            d_base_n[w]=db+16'sd1;
            d_ctr[w]=db-c16;         d_ctr_n[w]=db-c16+16'sd1;
            d_dl[w]=db-c16-hu;       d_dl_n[w]=db-c16-hu+16'sd1;
            win_coordinate[w]=coordinate;
            win_has[w]=0; win_pulls[w]=0;
            win_best_error[w]=0; win_best_width[w]=0; win_best_pull[w]=0;
        end
    endtask
    task automatic add_tip(input logic w, input logic [9:0] width, input logic signed [15:0] error);
        logic signed [13:0] pull;
        begin
            // Frozen gains -16/+8 dots, width weighted against ref256.
            // In Q8, width*gain is exact (ref and fraction cancel).
            pull=error[15] ? -$signed({width,4'b0}) : $signed({1'b0,width,3'b0});
            win_pulls[w]=win_pulls[w]+25'(pull);
            if(!win_has[w] || mag16(error)<mag16(win_best_error[w]) ||
               (mag16(error)==mag16(win_best_error[w]) && error<win_best_error[w])) begin
                win_best_error[w]=error; win_best_width[w]=width; win_best_pull[w]=pull;
            end
            win_has[w]=1;
        end
    endtask
    task automatic complete_tip(input logic qualified);
        begin
            // width is a_rise; both ages still hold their registered value here.
            if(qualified && !pulse_bad && a_rise>=16'sd8 && a_rise<=16'sd256) begin
                if(line_count!=2'd3) line_count=line_count+2'd1;
                line_rise=a_hs-a_rise; line_width=a_rise[9:0];
                if(primed && pulse_owned && win_valid[pulse_owner])
                    add_tip(pulse_owner,a_rise[9:0],d_ctr[pulse_owner]-a_rise);
            end
            pulse_open=0;
        end
    endtask
    // With L>=641 and opening coordinates within +/-192, consecutive deadlines
    // are >=257 clocks apart. An old owned pulse releases by deadline+255, so
    // only one window can finalize per edge and it is always the oldest. See
    // the timing-sharing proof in
    // docs/investigations/video-boundary/v1-rtl-candidate-2026-11-06.md.
    task automatic finish_windows;
        logic w;
        begin
            w=oldest_window;
            if(primed && win_valid[w] && win_closed[w] && !(pulse_open && pulse_owned && pulse_owner==w)) begin
                // synthesis translate_off
                if(f_pending) $fatal(1,"multiple same-edge finalizations violate admission bounds");
                // synthesis translate_on
                f_pending=1; f_has=win_has[w];
                f_coord=win_coordinate[w]; f_err=win_best_error[w];
                f_width=win_best_width[w]; f_pulls=win_pulls[w];
                f_bpull=win_best_pull[w]; f_clipc=coordinate;
                win_valid[w]=0; oldest_window=!w;
            end
        end
    endtask

    integer i;
    logic invalidate_pixels, hs_rise, tip_late;
    logic signed [15:0] db_next, p, q;
    logic signed [16:0] step;
    logic signed [17:0] moved;
    integer age;

    // ----------------------------------------------------------------- back
    // r0: the front's record for edge t. Stage 1 sums the target, stage 2
    // clips it, stage 3 owns the phase state, stage 4 the displacement.
    logic r0_clr=0, r0_prime=0, r0_fin=0, r0_has=0, r0_hs=0, r0_primed=0;
    logic signed [9:0] r0_coord=0, r0_fcoord=0, r0_clipc=0;
    logic signed [15:0] r0_err=0;
    logic [9:0] r0_width=0;
    logic signed [24:0] r0_pulls=0;
    logic signed [13:0] r0_bpull=0;
    logic [15:0] r0_tick=0;
    logic r1_clr=0, r1_prime=0, r1_fin=0, r1_has=0, r1_hs=0, r1_primed=0;
    logic signed [9:0] r1_coord=0;
    logic signed [27:0] r1_sum=0;
    logic signed [16:0] r1_lo=0, r1_hi=0;
    logic [15:0] r1_tick=0;
    logic r2_clr=0, r2_prime=0, r2_fin=0, r2_has=0, r2_hs=0, r2_primed=0;
    logic signed [9:0] r2_coord=0;
    logic signed [15:0] r2_target=0;
    logic [15:0] r2_tick=0;
    logic r3_hs=0, r3_primed=0;
    logic signed [9:0] r3_coord=0;
    // The estimate moves toward a target clipped to coordinate*64 +/- 12288
    // with |coordinate|<=192, so it and every published phase fit 16 bits.
    logic signed [15:0] absolute_phase=0, line_phase=0, latest_phase=0, previous_phase=0;
    // Strict-prior eligibility is explicit: retain the previous completed
    // result when another window finalizes on this acquisition HS edge.
    logic latest_valid=0, previous_valid=0;
    logic [15:0] latest_tick=0, previous_tick=0;
    logic signed [16:0] b_delta;
    logic signed [15:0] b_abs;
    logic b_latest_ok, b_previous_ok;
    logic signed [17:0] b_shift;
    logic signed [9:0] b_off;

    always @(posedge clk) begin
        f_pending=0; f_has=0; did_clear=0; did_prime=0;
        f_coord=0; f_clipc=0; f_err=0; f_width=0; f_pulls=0; f_bpull=0;
        a_hs=a_hs_q; a_hs_n=a_hs_q+16'sd1; a_rise=a_rise_q; a_rise_n=a_rise_q+16'sd1;
        for(i=0;i<2;i=i+1) begin
            d_base[i]=d_base_q[i]; d_base_n[i]=d_base_q[i]+16'sd1;
            d_ctr[i]=d_ctr_q[i];   d_ctr_n[i]=d_ctr_q[i]+16'sd1;
            d_dl[i]=d_dl_q[i];     d_dl_n[i]=d_dl_q[i]+16'sd1;
        end
        tick <= tick+16'd1;
        timing_delay[0] <= {hs,vs,hblank,vblank,field,frame_field};
        for(i=1;i<257;i=i+1) timing_delay[i] <= timing_delay[i-1];
        hs_rise=hs && !old_hs;
        invalidate_pixels=reset || context_clear || owner!=old_owner;
        if(invalidate_pixels) clear_receiver();
        else if(source_missing || source_missing!=old_missing) clear_receiver();
        // Bound elapsed time before modular timestamps can alias a much longer
        // physical line into an admitted one. Only receiver state is cleared.
        else if(have_hs && a_hs>=16'sd16384) clear_receiver();
        else begin
            // Fall at the exact cap is admitted before overlong rejection.
            if(ga_vs && !old_vs) begin pulse_bad=1; wait_low=1; end
            if(!ga_hs) begin
                if(old_ga && pulse_open) complete_tip(1);
                if(!ga_vs) wait_low=0;
            end
            if(pulse_open && a_rise>=16'sd256) begin complete_tip(0); wait_low=1; end
            finish_windows();
            // Half-open deadlines seal before an equal-clock new rise. An
            // already-high tip retains its stamped owner through completion.
            if(primed && !d_dl[active_window][15]) begin
                db_next=d_base[active_window]-period_nominal;
                win_closed[active_window]=1;
                finish_windows();
                if(win_valid[!active_window]) clear_receiver();
                else begin active_window=!active_window; start_window(active_window,db_next,half_up); end
            end
            if(ga_hs && !old_ga && !ga_vs && !wait_low) begin
                pulse_open=1; pulse_bad=0; a_rise=0; a_rise_n=16'sd1;
                pulse_owned=primed; pulse_owner=active_window;
            end
            if(hs_rise) begin
                p=a_hs;
                if(primed) begin
                    step=17'(p)-17'(period_nominal);
                    moved=18'(coordinate)+18'(step);
                    if(step>17'sd192 || step<-17'sd192 || moved>18'sd192 || moved<-18'sd192) clear_receiver();
                    else coordinate=moved[9:0];
                end
                if(!primed && have_hs) begin
                    // MODEL LIMIT (B26): two records require L>640 for the
                    // frozen ±192 coordinate/256 cap bounds. Signed timestamp
                    // arithmetic additionally bounds L<=16383; other periods
                    // deliberately retain native position and re-prime.
                    // See docs/investigations/video-boundary/v1-rtl-candidate-2026-11-06.md.
                    if(line_count==2'd1 && p>16'sd640 && p<=16'sd16383) begin
                        if(p==candidate_period && line_rise==candidate_tip && line_width==candidate_width) healthy=healthy+4'd1;
                        else begin candidate_period=p; candidate_tip=line_rise; candidate_width=line_width; healthy=1; end
                        if(healthy>=4'd8) begin
                            period_nominal=p; half_up=p-(p>>>1); coordinate=0; did_prime=1;
                            primed=1; active_window=0; oldest_window=0;
                            // The first window is centred on the nominal tip,
                            // one period back when the tip is in the late half.
                            tip_late=line_rise>(p>>>1);
                            db_next=tip_late ? p-line_rise : -line_rise;
                            start_window(0,db_next,half_up);
                            // Completed pre-prime tip belongs only if in the
                            // newly opened physical window; no double counting.
                            // Its rise is last_hs+line_rise, so its error to the
                            // new centre is exactly 0 (late half) or -p (early
                            // half, outside the window).
                            if(tip_late) add_tip(0,line_width,16'sd0);
                            if(pulse_open) begin
                                q=db_next-a_rise;
                                pulse_owned=(q>=-(p>>>1)) && ($signed(16'(q-half_up))<16'sd0);
                                pulse_owner=0;
                            end
                        end
                    end else begin healthy=0; candidate_period=0; end
                end
                a_hs=0; a_hs_n=16'sd1; have_hs=1; line_count=0;
            end
        end
        // A clear branch may consume this very HS edge. Retain the fresh line
        // anchor even when normal collection was skipped.
        if(!invalidate_pixels && !primed && hs_rise) begin
            a_hs=0; a_hs_n=16'sd1; have_hs=1; line_count=0;
        end
        a_hs_q<=a_hs_n; a_rise_q<=a_rise_n;
        for(i=0;i<2;i=i+1) begin
            d_base_q[i]<=d_base_n[i]; d_ctr_q[i]<=d_ctr_n[i]; d_dl_q[i]<=d_dl_n[i];
        end
        old_owner<=owner; old_hs<=hs; old_ga<=ga_hs; old_vs<=ga_vs; old_missing<=source_missing;

        // Record for the back. An HS on an invalidating edge samples nothing.
        r0_clr<=did_clear; r0_prime<=did_prime; r0_fin<=f_pending; r0_has<=f_has;
        r0_fcoord<=f_coord; r0_clipc<=f_clipc; r0_err<=f_err; r0_width<=f_width;
        r0_pulls<=f_pulls; r0_bpull<=f_bpull;
        r0_hs<=hs_rise && !invalidate_pixels; r0_primed<=primed; r0_coord<=coordinate;
        r0_tick<=tick;

        // Stage 1: target sum and clip bounds around the captured pre-HS origin.
        r1_clr<=r0_clr; r1_prime<=r0_prime; r1_fin<=r0_fin; r1_has<=r0_has;
        r1_hs<=r0_hs; r1_primed<=r0_primed; r1_coord<=r0_coord; r1_tick<=r0_tick;
        r1_sum<=(28'(r0_fcoord)<<<6) + (28'(r0_err)<<<6) +
            ((28'($signed({1'b0,r0_width}))-28'sd256)<<<5) + 28'(r0_pulls) - 28'(r0_bpull);
        r1_lo<=(17'(r0_clipc)<<<6)-17'sd12288; r1_hi<=(17'(r0_clipc)<<<6)+17'sd12288;
        // Stage 2: clip.
        r2_clr<=r1_clr; r2_prime<=r1_prime; r2_fin<=r1_fin; r2_has<=r1_has;
        r2_hs<=r1_hs; r2_primed<=r1_primed; r2_coord<=r1_coord; r2_tick<=r1_tick;
        r2_target<=r1_sum<28'(r1_lo) ? r1_lo[15:0] : (r1_sum>28'(r1_hi) ? r1_hi[15:0] : r1_sum[15:0]);
        // Stage 3: phase state, in the original same-edge order: clear or
        // prime, finalization bookkeeping, HS selection, then relaxation. A
        // result finalized on the HS edge itself is not eligible at that HS.
        b_latest_ok=latest_valid && $signed(r2_tick-latest_tick)>16'sd0;
        b_previous_ok=previous_valid && $signed(r2_tick-previous_tick)>16'sd0;
        b_delta=17'(r2_target)-17'(absolute_phase);
        b_abs=r2_has ? absolute_phase+16'(b_delta>>>3) : absolute_phase;
        if(r2_hs && r2_primed) begin
            if(b_latest_ok) line_phase<=latest_phase;
            else if(!r2_fin && b_previous_ok) line_phase<=previous_phase;
        end
        if(r2_fin) begin
            previous_valid<=latest_valid; previous_tick<=latest_tick; previous_phase<=latest_phase;
            latest_valid<=1; latest_tick<=r2_tick; latest_phase<=b_abs; absolute_phase<=b_abs;
        end
        if(r2_prime) absolute_phase<=0;
        if(r2_clr) begin
            absolute_phase<=0; line_phase<=0; latest_valid<=0; previous_valid<=0;
        end
        r3_hs<=r2_hs; r3_primed<=r2_primed; r3_coord<=r2_coord;
        // Stage 4: displacement of the line whose HS this record carries.
        b_shift=(18'(line_phase)-(18'(r3_coord)<<<6)+18'sd128)>>>8;
        b_off=b_shift<-18'sd48 ? -10'sd48 : (b_shift>18'sd48 ? 10'sd48 : b_shift[9:0]);
        if(r3_hs) line_offset<=r3_primed ? b_off[6:0] : 7'sd0;

        // Each sampled line displacement travels with its own HS through an
        // entire delay pipeline. No pending-register overwrite on short lines.
        if(invalidate_pixels) begin
            // Lines sampled before the invalidation are still in the back:
            // drop their displacement with the rest of the delay.
            r1_hs<=0; r2_hs<=0; r3_hs<=0; line_offset<=0;
            for(i=0;i<251;i=i+1) offset_delay[i] <= 0;
            write_ptr<=0; fill<=0; rgb12_out<=0; raw_vblank_out<=1; available<=0;
        end else begin
            offset_delay[0]<=line_offset;
            for(i=1;i<251;i=i+1) offset_delay[i]<=offset_delay[i-1];
            if(ce_16) begin
                // Synchronous RAM: read address uses PRE-write pointer/fill.
                // On edge t, emit history[t/4-age] while writing t/4. The old
                // offset_delay[250] is the offset sampled at t-256, aligned
                // with the tuple appearing from timing_delay[256] this edge.
                // Ages16..112 cannot collide with the same-edge write.
                age=64-int'($signed(offset_delay[250]));
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
