#!/usr/bin/env python3
"""Build the Plus/GX4000 hardware-probe cartridge (one self-labelled screen per test).

Needs sjasmplus on PATH. The program is plus_hw_probes.asm; this script generates its
include file (test table, screen text, PRI phase bands, font), assembles it and wraps
the 16 KiB bank as a CPR. --start selects the first screen (simulation runs).
"""
import argparse
import hashlib
import json
import shutil
import struct
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
VERSION = 'V3'

# RTL predictions are simulation readings of the production model at the commit
# named in README.md, not hardware claims.
TESTS = [
    ('init_title', 'TITLE', [], ''),
    ('init_width1', 'A1 PRI WIDTH R3=1 (LINES 3-10)', [
        'MARKER: SHORT GREEN LINE JUST ABOVE 2ND GRID LINE',
        'COMPARE MARKER END WITH A3'], 'RTL: MARKER, IRQ/FRAME=01'),
    ('init_width2', 'A2 PRI WIDTH R3=2 (LINES 3-10)', [
        'MARKER: SHORT GREEN LINE JUST ABOVE 2ND GRID LINE',
        'COMPARE MARKER END WITH A3'], 'RTL: MARKER ENDS 139 DOTS IN, IRQ/FRAME=01'),
    ('init_width3', 'A3 PRI WIDTH R3=3 (REFERENCE)', [
        'SAME HANDLER AS PRI-WIDTH-3.CPR (HW: ~136 DOTS)',
        'MARKER: SHORT GREEN LINE JUST ABOVE 2ND GRID LINE'], 'RTL: MARKER ENDS 139 DOTS IN, IRQ/FRAME=01'),
    ('init_phase', 'B PRI WRITE PHASE, HSYNC C0=49-59', [
        'BAND: PRI:=CURRENT LINE WRITTEN AT C0 SHOWN RIGHT',
        'YELLOW MARK AT BAND LEFT = IRQ; MOVES RIGHT FROM 49'], 'RTL: MARK C0 45-60, NONE 61-63, IRQ=16'),
    ('init_cross49', 'C1 PRI=7 R2=49 R3=8 (REFERENCE)', [
        'YELLOW MARKS ABOVE 2ND GRID LINE = IRQS',
        'IRQ/FRAME COUNTS THEM'], 'RTL: IRQ/FRAME=01'),
    ('init_cross57', 'C2 PRI=7 R2=57 R3=8 (CROSSES LINE)', [
        'YELLOW MARKS ABOVE 2ND GRID LINE = IRQS',
        'IRQ/FRAME COUNTS THEM'], 'RTL: IRQ/FRAME=02 (LINE ENTRY + ORDINARY)'),
    ('init_cross58', 'C3 PRI=7 R2=58 R3=8 (CROSSES LINE)', [
        'YELLOW MARKS ABOVE 2ND GRID LINE = IRQS',
        'IRQ/FRAME COUNTS THEM'], 'RTL: IRQ/FRAME=02 (LINE ENTRY + ORDINARY)'),
    ('init_cross62', 'C4 PRI=7 R2=62 R3=8 (CROSSES LINE)', [
        'YELLOW MARKS ABOVE 2ND GRID LINE = IRQS',
        'IRQ/FRAME COUNTS THEM'], 'RTL: IRQ/FRAME=02 (LINE ENTRY + ORDINARY)'),
    ('init_cross63', 'C5 PRI=7 R2=63 R3=8 (CROSSES LINE)', [
        'YELLOW MARKS ABOVE 2ND GRID LINE = IRQS',
        'IRQ/FRAME COUNTS THEM'], 'RTL: IRQ/FRAME=02 (ORDINARY + NEXT LINE ENTRY)'),
    ('init_split54', 'D1 SPLT=54', [], 'RTL: GREEN FROM LINE 55'),
    ('init_split55', 'D2 SPLT=55 (ALSO MATCHES LINE 311)', [], 'RTL: LINE 0 RED, GREEN FROM LINE 1 (LINE-311 CAPTURE HELD)'),
    ('init_split56', 'D3 SPLT=56', [], 'RTL: GREEN FROM LINE 57'),
    ('init_split57', 'D4 SPLT=57', [], 'RTL: GREEN FROM LINE 58'),
    ('init_r9_off0', 'E1 R9=11 VSCROLL 0', [], 'RTL: ROWS 00-15 IN ORDER'),
    ('init_r9_off5', 'E2 R9=11 VSCROLL 5', [], 'RTL: ROWS 00, 03, 06, 09...'),
    ('init_mirror', 'F SPRITE WRITE MIRRORS', [
        'ALL SET X1 VIA +4, THEN &0A (X2) WRITTEN AT OFFSET',
        'BIG SPRITE = OFFSET WRITES MAGNIFICATION'], 'RTL: +5 +6 +7 BIG, +3 SMALL'),
    ('init_edge', 'G1 SPRITE LEFT EDGE', [
        'LOOK FOR A 1-DOT WHITE COLUMN AT DISPLAY LEFT',
        'X4 SPRITES ARE 64 DOTS WIDE, X1 16 DOTS'], 'RTL: 1-DOT COLUMN FOR X=-63 AND X=-15 ONLY'),
    ('init_mask', 'G2 SSCR BIT 7 MASK OVER SPRITES', [
        'SSCR=&80: BORDER COVERS FIRST 16 DOTS',
        'SPRITES AT X=0, 8, 16 (16 DOTS WIDE)'], 'RTL: X=0 HIDDEN, X=8 RIGHT HALF, X=16 WHOLE'),
    ('init_cross56', 'C6 PRI=7 R2=56 R3=8 (ENDS AT LINE START)', [
        'YELLOW MARKS ABOVE 2ND GRID LINE = IRQS',
        'IRQ/FRAME COUNTS THEM'], 'RTL: IRQ/FRAME=02 (LINE ENTRY + ORDINARY)'),
    ('init_cross50w14', 'C7 PRI=7 R2=50 R3=14 (ENDS AT LINE START)', [
        'CRTC3 DEMO PLASMA/SPHERE/WOLVERINE TIMING',
        'IRQ/FRAME COUNTS THEM'], 'RTL: IRQ/FRAME=02 (LINE ENTRY + ORDINARY)'),
    ('init_ssa_late', 'H1 SPLT=55, SSA REWRITTEN L311 C0~52', [
        'SSA=ROW 04, ROW 05 WRITTEN ON LINE 311 AFTER C0=R1',
        'TOP GREEN TEXT = SSA THE LINE-311 SPLIT CAPTURED'], 'RTL: TOP SHOWS SSA ROW 04 (RTL SAMPLES AT C0=R1)'),
    ('init_ssa_early', 'H2 SPLT=55, SSA REWRITTEN L311 C0~30', [
        'CONTROL: ROW 05 WRITTEN BEFORE C0=R1',
        'TOP GREEN TEXT = SSA THE LINE-311 SPLIT CAPTURED'], 'RTL: TOP SHOWS SSA ROW 05'),
    ('init_split55s7', 'D5 SPLT=55 WITH VSCROLL 7', [], 'RTL: RED TO LINE 55 (LINE-0 ROW CAPTURE REPLACES HELD SPLIT)'),
    ('init_bars_off0', 'E3 R9=3 ROWS 6-11, VSCROLL 0', [
        'BAR LENGTH = 3 BYTES PER SOURCE ROW (ROW 6 = 9)'], 'RTL: BARS STEP ONCE PER 4-LINE ROW'),
    ('init_bars_off2', 'E4 R9=3 ROWS 6-11, VSCROLL 2', [
        'BAR LENGTH = 3 BYTES PER SOURCE ROW (ROW 6 = 9)'], 'RTL: BARS STEP ON 3 LINES OF EVERY 4-LINE ROW'),
]

# B bands: PRI:=T written at C0 = 45 + i on line T = 8*i+7, sync on T-2; the
# marker lands on line T+1, the first raster of character row i+1.
BAND_C0 = list(range(45, 64))
SLED_MAX = 96
SLED_BASE = 24  # calibrated in simulation: sled NOPs for C0=45 (see README)
# H: line-255 interrupt to the line-311 SSA write, calibrated in simulation.
SSA_COARSE = 500
SSA_SLED = {'late': 36, 'early': 14}  # C0 ~52 and ~30

EXTRA_GLYPHS = {
    '+': '00000 00100 00100 11111 00100 00100 00000',
    '/': '00001 00010 00010 00100 01000 01000 10000',
    '(': '00010 00100 01000 01000 01000 00100 00010',
    ')': '01000 00100 00010 00010 00010 00100 01000',
    '.': '00000 00000 00000 00000 00000 01100 01100',
    ',': '00000 00000 00000 00000 01100 00100 01000',
    '>': '01000 00100 00010 00001 00010 00100 01000',
    '<': '00010 00100 01000 10000 01000 00100 00010',
    '?': '01110 10001 00001 00010 00100 00000 00100',
    '&': '01100 10010 10100 01000 10101 10010 01101',
    '*': '00000 10101 01110 11111 01110 10101 00000',
    "'": '00100 00100 01000 00000 00000 00000 00000',
    '~': '00000 00000 01000 10101 00010 00000 00000',
}


def font():
    import pri_alias_probe
    glyphs = dict(pri_alias_probe.FONT, **EXTRA_GLYPHS)
    rows = []
    for code in range(32, 127):
        char = chr(code)
        bits = glyphs.get(char, glyphs[' ']).split()
        rows.append('    db ' + ','.join(f'0x{int(r, 2) << 2:02X}' for r in bits) + ',0 ; ' + repr(char))
    return rows


def text(entries, base=0xC000, invert=False):
    """entries: (row, col, string); returns asm lines for print_block."""
    out = []
    for row, col, string in entries:
        s = string.upper().replace('"', "'")
        out.append(f'    db {1 if invert else 0} : dw 0x{base + row * 80 + col:04X} : db "{s}",0')
    return out


def screen_text(index, init, title, notes, prediction):
    n = len(TESTS) - 1
    footer = f'PLUS HW PROBES {VERSION}  {index:02d}/{n:02d}  ANY KEY OR FIRE: NEXT'
    if init == 'init_title':
        lines = [(1, 2, f'PLUS/GX4000 HARDWARE PROBES {VERSION}'),
                 (2, 2, 'PHOTOGRAPH EACH SCREEN. ANY KEY OR JOYSTICK FIRE = NEXT SCREEN.')]
        half = (len(TESTS)) // 2
        for k, (_, t, _, _) in enumerate(TESTS[1:], 1):
            col, row = (2, 3 + k) if k <= half else (42, 3 + k - half)
            lines.append((row, col, f'{k:02d} {t}'[:38]))
        lines.append((24, 2, footer))
        return text(lines)
    label = [(0, 2, f'{index:02d} {title}'), (1, 2, prediction)] + [(2 + k, 2, s) for k, s in enumerate(notes)]
    if init.startswith('init_split'):
        # Red C000 bank shows rows 0-3; the green SSA bank repeats a label.
        return text(label + [(3, 2, 'RED = R12/R13 BANK C000. GREEN = SSA BANK 0000'), (4, 2, footer)])
    if init.startswith('init_ssa'):
        bank0 = [(r, 2, f'SSA ROW {r:02d}') for r in range(4, 22)]
        bank0 += [(6 + r, 16, t) for r, (_, _, t) in enumerate(label[:4])] + [(10, 16, footer)]
        return text(label + [(4, 2, footer)]) + text(bank0, base=0, invert=True)
    if init.startswith('init_bars'):
        return text(label[:4] + [(4, 2, footer)])
    if init.startswith('init_r9'):
        rows = [(r, 2, f'ROW {r:02d}') for r in range(1, 16)]
        return text([(0, 2, f'ROW 00 {index:02d} {title}'), (0, 40, prediction)] + rows
                    + [(15, 12, footer)])
    # Column 16 keeps text clear of a crossing HSYNC's blanking (C4/C5: C0 0-6).
    lines = [(20 + r, 16, s) for r, _, s in label[:4]] + [(24, 16, footer)]
    if init == 'init_phase':
        lines += [(i + 1, 66, f'C0={c0}') for i, c0 in enumerate(BAND_C0)]
    if init == 'init_mirror':
        lines += [(10, 8, 'CTRL'), (10, 20, '+3'), (10, 32, '+5'), (10, 44, '+6'), (10, 56, '+7')]
    if init == 'init_edge':
        lines += [(4, 20, 'X=-64 MAG X4'), (8, 20, 'X=-63 MAG X4'), (12, 20, 'X=-16 MAG X1'),
                  (16, 20, 'X=-15 MAG X1'), (19, 20, 'X=0 MAG X1')]
    if init == 'init_mask':
        lines += [(4, 20, 'X=0'), (8, 20, 'X=8'), (12, 20, 'X=16')]
    return text(lines)


def generate(start):
    inc = [f'START_TEST equ {start}', f'NTESTS equ {len(TESTS)}', f'SLED_MAX equ {SLED_MAX}',
           f'SSA_COARSE equ {SSA_COARSE}', f"SSA_SLED_LATE equ {SSA_SLED['late']}",
           f"SSA_SLED_EARLY equ {SSA_SLED['early']}"]
    s = [8 * i + 5 for i in range(len(BAND_C0))]
    t = [8 * i + 7 for i in range(len(BAND_C0))]
    inc.append(f'BAND_S0 equ {s[0]}')
    inc.append('band_table:')
    for i, c0 in enumerate(BAND_C0):
        sled = SLED_BASE + (c0 - BAND_C0[0])
        assert 0 <= sled <= SLED_MAX
        nxt = s[i + 1] if i + 1 < len(s) else 0
        inc.append(f'    db {sled},{t[i]},{nxt} ; write C0={c0} on line {t[i]}')
    inc.append('test_table:')
    for k, (init, *_rest) in enumerate(TESTS):
        inc.append(f'    dw {init},text_{k}')
    for k, test in enumerate(TESTS):
        inc.append(f'text_{k}:')
        inc += screen_text(k, *test)
        inc.append('    db 0xFF')
    inc.append('count_text:')
    inc += text([(20, 62, 'IRQ/FRAME=')])
    inc.append('    db 0xFF')
    inc.append('split_bank0_text:')
    inc += text([(0, 2, 'GREEN = SSA BANK 0000'), (1, 2, 'RED = R12/R13 BANK C000')], base=0, invert=True)
    inc.append('    db 0xFF')
    inc.append('font:')
    inc += font()
    return '\n'.join(inc) + '\n'


def build(destination, start):
    sjasm = shutil.which('sjasmplus')
    if not sjasm:
        raise SystemExit('sjasmplus not found on PATH')
    destination.mkdir(parents=True, exist_ok=True)
    stem = 'plus-hw-probes' if start == 0 else f'plus-hw-probes-start{start:02d}'
    (destination / 'plus_hw_probes.inc').write_text(generate(start))
    raw = destination / f'{stem}.bin'
    subprocess.run([sjasm, '--nologo', '--msg=war', f'--inc={destination}', f'--raw={raw}',
                    f'--lst={destination / (stem + ".lst")}', f'--sym={destination / (stem + ".sym")}',
                    str(HERE / 'plus_hw_probes.asm')], check=True)
    code = raw.read_bytes()
    assert len(code) <= 0x3F00, 'code must stay below the BF00 variables'
    bank = code + bytes(0x4000 - len(code))
    payload = b'AMS!cb00' + struct.pack('<I', len(bank)) + bank
    cpr = b'RIFF' + struct.pack('<I', len(payload)) + payload
    (destination / f'{stem}.cpr').write_bytes(cpr)
    meta = dict(sha256=hashlib.sha256(cpr).hexdigest(), program_bytes=len(code), start=start,
                tests=[t[1] for t in TESTS], band_c0=BAND_C0, sled_base=SLED_BASE)
    (destination / f'{stem}.json').write_text(json.dumps(meta, indent=2) + '\n')
    print(f'{destination / (stem + ".cpr")}: {len(code)} bytes, sha256={meta["sha256"]}')


if __name__ == '__main__':
    import sys
    sys.path.insert(0, str(HERE))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=HERE.parents[1] / 'output_files/plus-hw-probes')
    parser.add_argument('--start', type=int, default=0, help='first screen (simulation)')
    args = parser.parse_args()
    build(args.output_dir, args.start)
