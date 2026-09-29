#!/usr/bin/env python3
"""Build B25.BIN in an AMSDOS DATA-format DSK; RUN\"B25 loads at &1000.

Requires sjasmplus. No firmware or copyrighted media is included. The 40-track
single-sided image uses sequential C1..C9 sectors and 1K CP/M allocation blocks.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess

from pri_alias_probe import FONT

HERE = Path(__file__).resolve().parent
CASES = ['NOP', 'HALT', 'RET NC 0', 'RET NC 1', 'INC HL 0', 'INC HL 1',
         'ADD DE 0', 'ADD DE 1', 'ADD DE 2']


def textblock(name, entries):
    lines = [name + ':']
    for row, col, text in entries:
        assert col + len(text) <= 80
        lines.append(f'    dw 0x{0xC000 + row * 80 + col:04X} : db "{text}",0')
    return lines + ['    dw 0']


def include():
    lines = textblock('busy_text', [(2, 2, 'B25 V1 - MEASURING 72 TRIALS - PLEASE WAIT')])
    entries = [(0, 2, 'CLASSIC B25 V1 - HEX RESULTS - RECORD MACHINE AND CRTC TYPE'),
               (1, 2, 'PC = RETURN PC MINUS 4000   HL = CAPTURED COUNT   PAD = NOPS'),
               (2, 2, 'CASE      REG'),
               (2, 16, '1    2    3    4    5    6    7    8'),
               (21, 2, 'FULL VIDEO MODE ON MISTER. PHOTOGRAPH ALL TRIALS AND BOTH PARITIES.'),
               (22, 2, 'ANY KEY OR FIRE: RERUN. RESET: EXIT. CRTC1 UNITS MAY DIFFER.'),
               (23, 2, 'TECHNICAL INFORMATION SOURCED FROM THE AMSTRAD CPC CRTC COMPENDIUM'),
               (24, 2, 'BY LONGSHOT - CC BY-NC-ND. NOP-HALT CONTROLS. RET NC C=1. ADD DE=1.')]
    for i, name in enumerate(CASES):
        entries += [(3 + i * 2, 2, f'{name:10}PC'), (4 + i * 2, 2, '          HL')]
    lines += textblock('table_text', entries)
    lines.append('font:')
    glyphs = dict(FONT, **{'.': '00000 00000 00000 00000 00000 01100 01100'})
    for code in range(32, 91):
        rows = glyphs.get(chr(code), FONT[' ']).split()
        lines.append('    db ' + ','.join(str(int(r, 2) << 2) for r in rows) + ',0')
    return '\n'.join(lines) + '\n'


def disk(binary):
    header = bytearray(128)
    header[1:12] = b'B25     BIN'
    header[18] = 2
    struct.pack_into('<H', header, 21, 0x1000)
    struct.pack_into('<H', header, 24, len(binary))
    struct.pack_into('<H', header, 26, 0x1000)
    header[64:67] = len(binary).to_bytes(3, 'little')
    struct.pack_into('<H', header, 67, sum(header[:67]))
    file = bytes(header) + binary
    records = (len(file) + 127) // 128
    blocks = (len(file) + 1023) // 1024
    assert blocks <= 16, 'single directory extent only'
    data = bytearray(b'\xE5' * (40 * 9 * 512))
    entry = bytearray(32)
    entry[1:12] = b'B25     BIN'
    entry[15] = records
    entry[16:16 + blocks] = bytes(range(2, 2 + blocks))
    data[:32] = entry
    data[2048:2048 + len(file)] = file
    disk_header = bytearray(256)
    disk_header[:34] = b'MV - CPCEMU Disk-File\r\nDisk-Info\r\n'
    disk_header[34:48] = b'B25 diagnostic'
    disk_header[48:50] = bytes([40, 1])
    struct.pack_into('<H', disk_header, 50, 256 + 9 * 512)
    out = bytearray(disk_header)
    for track in range(40):
        h = bytearray(256)
        h[:12] = b'Track-Info\r\n'
        h[16] = track
        h[20:24] = bytes([2, 9, 0x4E, 0xE5])
        for sector in range(9):
            h[24 + sector * 8:32 + sector * 8] = bytes([track, 0, 0xC1 + sector, 2, 0, 0, 0, 0])
        out += h + data[track * 4608:(track + 1) * 4608]
    return bytes(out)


def build(out):
    out.mkdir(parents=True, exist_ok=True)
    (out / 'classic_b25.inc').write_text(include())
    raw = out / 'classic-b25.bin'
    subprocess.run([shutil.which('sjasmplus') or 'sjasmplus', '--nologo', '--msg=war',
                    f'--inc={out}', f'--raw={raw}', f'--sym={out / "classic-b25.sym"}',
                    f'--lst={out / "classic-b25.lst"}', str(HERE / 'classic_b25.asm')], check=True)
    image = disk(raw.read_bytes())
    path = out / 'classic-b25.dsk'
    path.write_bytes(image)
    metadata = dict(version=1, sha256=hashlib.sha256(image).hexdigest(), load=0x1000,
                    entry=0x1000, status=0x8000, completed=0xA5, results=0x8100,
                    trials=8, cases=CASES, record='little-endian uint16 PC, uint16 HL',
                    sled=0x4000, binary_bytes=raw.stat().st_size)
    (out / 'classic-b25.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'{path}: {metadata["binary_bytes"]} program bytes, sha256={metadata["sha256"]}')
    return path


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=HERE.parents[1] / 'output_files/classic-b25')
    build(parser.parse_args().output_dir.resolve())
