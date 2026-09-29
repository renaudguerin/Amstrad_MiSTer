#!/usr/bin/env python3
"""Execute the PA3 companion on production T80 and check bus timing/visible marker.

These are diagnostic execution contracts from the recorded V4 variants in
asic-audit-probes-v4.md, not an assertion of original ASIC capture behaviour.
The shared motherboard/CPU/bus/display interaction can shift instruction timing;
checking assembled bytes alone would not detect it.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import subprocess

from pa3_followup import build
from plus_hw_probes_sim import build_model, ROOT


def verify(path, case):
    lines = Path(path).read_text().splitlines()
    def positions(prefix):
        return [tuple(map(int, re.search(r' line=(\d+) c0=(\d+) dot=(\d+)', s).groups()))
                for s in lines if s.startswith(prefix)]
    # Recorded V4 source deltas: early24 preserves disable; +1024 NOPs
    # moves the entire window exactly16 lines. R5=0 wraps after line295.
    enables = [(295, 48, 0), (295, 24, 0), (311, 48, 0), (311, 24, 0), (295, 48, 0)]
    disable_line = 296 if case < 2 else 0
    assert positions('WR 6801=27 ') == [enables[case]], (case, 'enable', positions('WR 6801=27 '))
    assert positions('WR 6801=0 ') == [(disable_line, 10, 0)], (case, 'disable')
    # Check the actual bus writes are after R1=40 and before HSYNC C49;
    # unlike the first pulse, this second one must be in the visible border.
    assert (disable_line, 41, 0) in positions('WR 6421=f '), (case, 'visible marker start')
    assert (disable_line, 48, 0) in positions('WR 6421=0 '), (case, 'visible marker end')
    assert any(s.startswith('HS_RISE ') and f' line={disable_line} c0=49 ' in s for s in lines), (case, 'HSYNC')
    return dict(case=case, enable=enables[case], disable=[disable_line, 10, 0],
                visible_marker=[disable_line, 41, 48])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'output_files/pa3-followup/rtl')
    parser.add_argument('--frames', type=int, default=40)
    parser.add_argument('--no-build', action='store_true')
    parser.add_argument('--cxx', default='/opt/homebrew/opt/llvm/bin/clang++')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    obj = ROOT / 'sim/plus/obj_dir/plus_hw_probes_sim'
    binary = obj / 'plus_hw_probes_sim' if args.no_build else build_model(obj, args.cxx)
    # Each start uses its own generated include/source directory, so all
    # listings remain independently reproducible after the full batch.
    cartridges = [build(args.out / f'case{n:02d}', n) for n in range(5)]
    def run(n):
        prefix = args.out / f'{n:02d}'
        result = subprocess.run([str(binary), str(cartridges[n]), str(prefix), str(args.frames)],
                                capture_output=True, text=True)
        prefix.with_suffix('.log').write_text(result.stdout + result.stderr)
        result.check_returncode()
        summary = verify(str(prefix) + '-events.txt', n)
        print(f'PA3 case{n}: timing PASS; {result.stdout.strip()}', flush=True)
        return summary
    with ThreadPoolExecutor(max_workers=5) as pool:
        results = list(pool.map(run, range(5)))
    (args.out / 'timing.json').write_text(json.dumps(results, indent=2) + '\n')
    print('pa3_followup_sim: PASS 5 production-T80 timing/visible-border cases')


if __name__ == '__main__':
    main()
