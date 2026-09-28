#!/usr/bin/env python3
"""Original-Plus PA7 overlap records through the production T80/ASIC fixture."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
import pa7_dma_overlap
import plus_hw_probes_sim

# Authority: docs/plus/pa7-dma-overlap-original-plus-2026-09-28.md,
# full photograph transcription (all four pages, 17 rows x eight repeats).
# Expectations are the observed software records, not internal ASIC timing.
ANCHORS = (0, -1, 0, 0)
CPR_SHA256 = '0f499bc6c50f935b0f9a455ec953db44a97a6a2db5da57aa215914c80c7c7106'
OFFSETS = (-1600, 24, 0, *range(-3, 3), *range(-6, 2))


def expected_record(page, row):
    dma = [4, 0x40 if page == 2 else 0, 0]
    if row == 0:
        pre, post, events = 0xc0, 0x80, [dma, [6, 0x80, 1]]
    elif row == 1 or (row >= 9 and OFFSETS[row] >= ANCHORS[page] - 1):
        pre, post, events = 0xc0, 0, [[6, 0xc0, 0], dma]
    elif row == 2:
        pre, post, events = 0xc0, 0, [dma]
    elif row < 9:
        pre, post = 0x80, 0x80
        events = [[6, 0x80, int(OFFSETS[row] < ANCHORS[page])]]
    else:
        pre, post, events = 0xc0, 0x80, [dma, [6, 0x80, 0]]
    # N, PRE, POST, FL, three vector/status/marker tuples, DIFF, REP.
    # Byte15 is reserved and not a photographed field.
    return ([len(events), pre, post, 0] + sum(events, []) +
            [255] * (3 * (3 - len(events))) + [0, 8])


def check(path, page):
    data = json.loads(path.read_text())
    if data['page'] != page or data['status'] != 0x80 or len(data['records']) != 17:
        raise RuntimeError(f'{path}: incomplete or wrong page')
    failures = 0
    for row, record in enumerate(data['records']):
        expected = expected_record(page, row)
        if record[:15] != expected:
            failures += 1
            print(f'FAIL page={page + 1} row={row} offset={OFFSETS[row]}: '
                  f'expected={expected} actual={record[:15]}', flush=True)
    print(f'page {page + 1}: {17 - failures}/17 hardware records match', flush=True)
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'output_files/pa7-dma-repair/regression')
    parser.add_argument('--cxx', default=None)
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    obj = ROOT / 'sim/plus/obj_dir/pa7_dma_overlap'
    binary = obj / 'plus_hw_probes_sim' if args.no_build else plus_hw_probes_sim.build_model(obj, args.cxx)
    args.out.mkdir(parents=True, exist_ok=True)
    cartridges = []
    for page in range(4):
        cpr = pa7_dma_overlap.build(args.out, page, 3294)
        if page == 0 and hashlib.sha256(cpr.read_bytes()).hexdigest() != CPR_SHA256:
            raise RuntimeError('cartridge differs from original-Plus hardware input')
        cartridges.append(cpr)

    def run(page):
        prefix = args.out / f'{page:02d}'
        cpr = cartridges[page]
        subprocess.run([str(binary), str(cpr), str(prefix), '400', '--overlap'], check=True)
        return check(args.out / f'{page:02d}-overlap-results.json', page)

    with ThreadPoolExecutor(max_workers=4) as pool:
        failures = sum(pool.map(run, range(4)))
    print(f'pa7-dma-overlap: {68 - failures} passed, {failures} failed', flush=True)
    return int(failures != 0)


if __name__ == '__main__':
    sys.exit(main())
