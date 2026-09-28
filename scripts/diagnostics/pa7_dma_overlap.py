#!/usr/bin/env python3
"""Build the standalone PA7 DMA/compatible-IRQ observation cartridge (sjasmplus)."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess

from plus_hw_probes import font, text

HERE = Path(__file__).resolve().parent
DEFAULT_DELAY = 3294
PAGES = ['W8 NOP AUTO', 'W8 LD A,(HL) AUTO', 'W8 NOP MANUAL', 'W12 NOP AUTO']
OFFSETS = [-1600, 24, 0, *range(-3, 3), *range(-6, 2)]
LABELS = ['DMA FAR EARLY', 'DMA LATE', 'DMA ONLY'] + [f'NO DMA {n:+d}' for n in range(-3, 3)] + [f'DMA {n:+d}' for n in range(-6, 2)]


def generate(start, delay):
    inc = [f'START_PAGE equ {start}', f'DELAY_BASE equ {delay}',
           'case_delay_table: dw ' + ','.join(str(delay + n) for n in OFFSETS),
           'case_kind_table: db 0,1,2,3,3,3,3,3,3,4,4,4,4,4,4,4,4',
           'page_text_table: dw page_0,page_1,page_2,page_3']
    for page, title in enumerate(PAGES):
        inc += [f'page_{page}:'] + text([
            (0, 1, f'PA7 DMA / COMPATIBLE IRQ   {page+1}/4   {title}'),
            (1, 1, 'OBSERVATIONS ONLY - 8 TRIALS/ROW - PHOTOGRAPH AFTER DONE'),
            (2, 1, 'V:04=DMA0 06=RASTER   M:0=BEFORE MARK 1=AFTER F=UNUSED'),
            (3, 1, 'S/PRE/POST:DCSR   DIFF:REPEAT MISMATCH   FL:ANOMALY'),
            (4, 1, 'CASE             N  V1 S1 M V2 S2 M V3 S3 M  PRE  POST DIFF REP  FL')])
        inc += text([(5+n, 1, label) for n, label in enumerate(LABELS)])
        inc += text([(22, 1, 'ALL VALUES HEX. OFFSETS ARE MICROSECONDS FROM CALIBRATED BASE.')])
        inc += ['    db 0xFF']
    inc += ['progress_text:'] + text([(23, 1, 'RUNNING CASE=    REP=     PLEASE WAIT')]) + ['    db 0xFF']
    inc += ['done_text:'] + text([(23, 1, 'DONE - PHOTOGRAPH THIS PAGE. ANY KEY: NEXT PAGE.          ')]) + ['    db 0xFF']
    inc += ["unlock_seq: db 0xFF,0x00,0xFF,0x77,0xB3,0x51,0xA8,0xD4,0x62,0x39,0x9C,0x46,0x2B,0x15,0x8A,0xCD,0xEE",
            'palette: db 0,0, 0xFF,0x0F, 0x08,0',
            'crtc_std: db 0,63,1,40,2,49,3,0x88,4,38,5,0,6,25,7,30,8,0,9,7,12,0x30,13,0,0xFF',
            'font:'] + font()
    return '\n'.join(inc) + '\n'


def build(destination, start=0, delay=DEFAULT_DELAY):
    if start not in range(4) or not 1600 <= delay <= 3313:
        raise ValueError('start must be 0..3; delay must keep all patches inside the timing buffer')
    sjasm = shutil.which('sjasmplus')
    if not sjasm:
        raise SystemExit('sjasmplus not found on PATH')
    destination = Path(destination).resolve()
    destination.mkdir(parents=True, exist_ok=True)
    stem = f'pa7-dma-overlap-start{start:02d}'
    (destination / 'pa7_dma_overlap.inc').write_text(generate(start, delay))
    raw = destination / f'{stem}.bin'
    subprocess.run([sjasm, '--nologo', '--msg=war', f'--inc={destination}', f'--raw={raw}',
                    f'--lst={destination / (stem + ".lst")}', f'--sym={destination / (stem + ".sym")}',
                    str(HERE / 'pa7_dma_overlap.asm')], check=True)
    code = raw.read_bytes()
    assert len(code) <= 0x3000, 'program must stay below B000 results'
    bank = code + bytes(0x4000 - len(code))
    payload = b'AMS!cb00' + struct.pack('<I', len(bank)) + bank
    data = b'RIFF' + struct.pack('<I', len(payload)) + payload
    cpr = destination / f'{stem}.cpr'
    cpr.write_bytes(data)
    meta = dict(sha256=hashlib.sha256(data).hexdigest(), program_bytes=len(code), start=start,
                delay_base=delay, page_delay_adjustments=[0, 0, 0, 4], pages=PAGES, case_offsets=OFFSETS,
                records=dict(address='B000', rows=17, stride=16,
                             fields=['count', 'pre_dcsr', 'post_dcsr', 'flags',
                                     'vector1', 'status1', 'mark1', 'vector2', 'status2', 'mark2',
                                     'vector3', 'status3', 'mark3', 'disagreements', 'completed', 'reserved']))
    cpr.with_suffix('.json').write_text(json.dumps(meta, indent=2) + '\n')
    print(f'{cpr}: {len(code)} bytes, sha256={meta["sha256"]}', flush=True)
    return cpr


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=HERE.parents[1] / 'output_files/pa7-dma-overlap/cartridge')
    parser.add_argument('--start', type=int, choices=range(4), default=0)
    parser.add_argument('--delay', type=int, default=DEFAULT_DELAY)
    args = parser.parse_args()
    build(args.output_dir, args.start, args.delay)
