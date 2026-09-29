#!/usr/bin/env python3
"""Build the five-case PA3 original-Plus follow-up cartridge (sjasmplus)."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess

import plus_hw_probes as common

HERE = Path(__file__).resolve().parent
CASES = [
    ('normal_late', 'NORMAL TERMINAL LATE', 16, 295, 48,
     'ALL ADJUSTMENT RED; FRAME 0-7 RED'),
    ('normal_early24', 'NORMAL TERMINAL EARLY24', 16, 295, 24,
     'ADJUSTMENT GREEN; FRAME 0-7 RED'),
    ('adjustment_late', 'LAST ADJUSTMENT LATE', 16, 311, 48,
     'FRAME 0 RED; FRAME 1-7 GREEN'),
    ('adjustment_early24', 'LAST ADJUSTMENT EARLY24', 16, 311, 24,
     'FRAME 0 RED; FRAME 1-7 GREEN'),
    ('r5_zero', 'R5=0 TERMINAL LATE CONTROL', 0, 295, 48,
     'FRAME 0 RED; FRAME 1-7 GREEN'),
]


def screen_text(index, *_):
    name, title, r5, line, c0, prediction = CASES[index]
    entries = [
        (4, 2, f'PA3 FOLLOW-UP  {index+1}/5  {title}'),
        (5, 2, f'R4=36 R5={r5} R9=7  ARM LINE {line} C{c0}  SSA0200'),
        (6, 2, f'AMSPIRIT PREDICTION: {prediction}'),
        (7, 2, 'OBSERVATION ONLY - PHOTOGRAPH COLOUR STRIP AND CASE LABEL'),
        (8, 2, 'ANY KEY OR JOYSTICK FIRE: NEXT CASE (WRAPS 5 TO 1)'),
        (10, 2, 'HANDLER: WAITING'),
        (11, 2, 'CYAN DASH: BORDER C41-48'),
    ]
    return common.text(entries) + common.text(entries, base=0x0400, invert=True)


def build(destination, start=0):
    if start not in range(len(CASES)):
        raise ValueError('start must be 0..4')
    sjasm = shutil.which('sjasmplus')
    if not sjasm:
        raise SystemExit('sjasmplus not found on PATH')
    destination = Path(destination).resolve()
    destination.mkdir(parents=True, exist_ok=True)
    stem = f'pa3-followup-start{start:02d}'
    tests = [('init_' + case[0], case[1], [], case[-1]) for case in CASES]
    inc = common.generate(start, tests=tests, render_text=screen_text)
    inc += 'pa3_reached_text:\n'
    entries = [(10, 2, 'HANDLER: RAN    ')]
    inc += '\n'.join(common.text(entries) + common.text(entries, base=0x0400, invert=True))
    inc += '\n    db 0xFF\n'
    # Retain the common program's unused PA7 A13 placement contract.
    inc += '    if $ < 0xA000\n    ds 0xA000-$,0\n    endif\n'
    (destination / 'plus_hw_probes.inc').write_text(inc)
    # Assemble the standard program unchanged, followed by companion routines.
    source = destination / 'pa3_followup_build.asm'
    source.write_text('    include "plus_hw_probes.asm"\n    include "pa3_followup.asm"\n')
    raw = destination / (stem + '.bin')
    subprocess.run([sjasm, '--nologo', '--msg=war', f'--inc={destination}', f'--inc={HERE}',
                    f'--raw={raw}', f'--lst={destination / (stem + ".lst")}',
                    f'--sym={destination / (stem + ".sym")}', str(source)], check=True)
    code = raw.read_bytes()
    assert len(code) <= 0x3F00, 'code must stay below BF00 variables'
    bank = code + bytes(0x4000 - len(code))
    payload = b'AMS!cb00' + struct.pack('<I', len(bank)) + bank
    data = b'RIFF' + struct.pack('<I', len(payload)) + payload
    cpr = destination / (stem + '.cpr')
    cpr.write_bytes(data)
    meta = dict(sha256=hashlib.sha256(data).hexdigest(), program_bytes=len(code), start=start,
                cases=[dict(name=n, label=label, r5=r5, enable_line=line, enable_c0=c0,
                            amspirit_prediction=prediction)
                       for n, label, r5, line, c0, prediction in CASES],
                evidence='Emulator/simulation predictions, not original-machine acceptance',
                handler_status=dict(address='BF18', waiting=0, reached=1))
    cpr.with_suffix('.json').write_text(json.dumps(meta, indent=2) + '\n')
    print(f'{cpr}: {len(code)} bytes, sha256={meta["sha256"]}', flush=True)
    return cpr


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path,
                        default=HERE.parents[1] / 'output_files/pa3-followup/cartridge')
    parser.add_argument('--start', type=int, choices=range(len(CASES)), default=0)
    args = parser.parse_args()
    build(args.output_dir, args.start)
