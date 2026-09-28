#!/usr/bin/env python3
"""Run the PA7 companion on production T80; outputs are model predictions."""
import argparse
import json
from pathlib import Path
import subprocess
import pa7_dma_overlap
import plus_hw_probes_sim

ROOT = Path(__file__).resolve().parents[2]


def counterfactual(obj, hypothesis):
    """Disposable models only: preserve checked-in production RTL byte for byte."""
    source = ROOT / 'rtl/plus/asic_ga_timing.v'
    s = source.read_text()
    def replace(old, new):
        nonlocal s
        assert s.count(old) == 1, f'counterfactual anchor changed: {old}'
        s = s.replace(old, new)
    if hypothesis == 'late':
        # Shift the shared internal HSYNC path (counter, monitor and VSYNC-delay
        # consumers) one character; remove the compatible output delay.
        # PRI/DMA inputs are unchanged. This is H4, observationally equivalent
        # to H2 in the pre-pended-DMA experiment.
        replace('wire hsync_n = ~HSYNC_I;', 'wire hsync_n = ~pri_hs_d;')
        replace('assign INT_N = programmed_int_n & (classic_int_n | classic_delivery_n | (pri != 8\'d0));',
                'assign INT_N = classic_raw_n;')
    elif hypothesis == 'retain':
        # Freeze ACK provenance for its complete interval: a DMA ACK does not
        # clear or suppress compatible creation, even if delivery matures during it.
        replace('wire int_ack_active = int_reset | intack;', """wire int_ack_active = int_reset | intack;
    reg cf_raster_ack;
    always @(posedge clk) begin
        if (reset || SNA_LOAD) cf_raster_ack <= 1'b0;
        else if (intack && !intack_d) cf_raster_ack <= !INT_N;
    end
    wire cf_raster = intack_d ? cf_raster_ack : !INT_N;
    wire cf_classic_clear = irq_reset | ((intack | irqack_rst) & cf_raster);""")
        replace("classic_int_n    <= 1'b1;", """if (cf_classic_clear) classic_int_n <= 1'b1;
                else if ((pri == 8'd0) && ~intcnt_comb[5] & cnt5)
                    classic_int_n <= 1'b0;""")
        replace("if (int_ack_active) begin\n\t\t\t\tclassic_delivery_n <= 1'b1;",
                "if (cf_classic_clear) begin\n\t\t\t\tclassic_delivery_n <= 1'b1;")
    else:
        raise ValueError(hypothesis)
    obj.mkdir(parents=True, exist_ok=True)
    target = obj / 'asic_ga_timing_counterfactual.v'
    target.write_text('// DIAGNOSTIC HYPOTHESIS; not a proposed production repair.\n' + s)
    return {source: target}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', type=Path, help='default: output_files/pa7-dma-overlap/sim/<hypothesis>')
    p.add_argument('--pages', nargs='+', type=int, default=list(range(4)))
    p.add_argument('--delay', type=int, default=3294)
    p.add_argument('--frames', type=int, default=400)
    p.add_argument('--no-build', action='store_true')
    p.add_argument('--hypothesis', choices=['production', 'late', 'retain'], default='production')
    p.add_argument('--cxx', default='/opt/homebrew/opt/llvm/bin/clang++')
    a = p.parse_args()
    if any(n not in range(4) for n in a.pages):
        p.error('pages must be 0..3')
    obj = ROOT / 'sim/plus/obj_dir' / ('plus_hw_probes_sim' if a.hypothesis == 'production' else 'pa7_overlap_' + a.hypothesis)
    overrides = None if a.hypothesis == 'production' else counterfactual(obj, a.hypothesis)
    binary = obj / 'plus_hw_probes_sim' if a.no_build else plus_hw_probes_sim.build_model(obj, a.cxx, overrides)
    a.out = a.out or ROOT / 'output_files/pa7-dma-overlap/sim' / a.hypothesis
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / 'run.json').write_text(json.dumps(dict(hypothesis=a.hypothesis, pages=a.pages,
        delay=a.delay, frames=a.frames, binary=str(binary)), indent=2) + '\n')
    for n in a.pages:
        cpr = pa7_dma_overlap.build(a.out, n, a.delay)
        subprocess.run([str(binary), str(cpr), str(a.out / f'{n:02d}'), str(a.frames), '--overlap'], check=True)


if __name__ == '__main__':
    main()
