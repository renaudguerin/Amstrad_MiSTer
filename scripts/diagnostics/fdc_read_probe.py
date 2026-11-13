#!/usr/bin/env python3
"""Build visible 6128 Plus FDC probes and a distinctly named copy of their disk.

No firmware is included. Requires sjasmplus; expected data comes only from the
input EDSK's physical track 0/head 0, sector ID C=0/H=0/R=0x41/N=2.
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
READ_COMMAND = bytes.fromhex('46 00 00 00 41 02 41 1e ff')


def sector_payload(image):
    """Walk EDSK track sizes and sector descriptors, not a hardcoded file LBA."""
    if len(image) < 256 or not image.startswith(b'EXTENDED CPC DSK File\r\nDisk-Info\r\n'):
        raise ValueError('input must be an extended CPC DSK image')
    tracks, sides = image[0x30:0x32]
    if not tracks or sides not in (1, 2) or tracks * sides > 204:
        raise ValueError('invalid EDSK track/side table')
    offset = 256
    for index in range(tracks * sides):
        size = image[0x34 + index] * 256
        track, head = divmod(index, sides)
        if track == head == 0:
            if size < 256 or offset + size > len(image):
                raise ValueError('missing/truncated physical track 0/head 0')
            block = image[offset:offset + size]
            if not block.startswith(b'Track-Info\r\n') or block[0x10:0x12] != b'\0\0':
                raise ValueError('track header does not describe track 0/head 0')
            count = block[0x15]
            if 0x18 + count * 8 > 256:
                raise ValueError('sector descriptors exceed track header')
            data_offset = 256
            found = []
            for slot in range(count):
                desc = block[0x18 + slot * 8:0x20 + slot * 8]
                length = int.from_bytes(desc[6:8], 'little')
                if data_offset + length > size:
                    raise ValueError('sector payload exceeds track block')
                if desc[:4] == bytes((0, 0, 0x41, 2)):
                    if length != 512:
                        raise ValueError('selected sector must have exactly 512 bytes (no weak copies)')
                    found.append((block[data_offset:data_offset + length], dict(
                        track=track, head=head, slot=slot, chrn=list(desc[:4]),
                        st1=desc[4], st2=desc[5], bytes=length,
                        file_offset=offset + data_offset)))
                data_offset += length
            if len(found) != 1:
                raise ValueError('need exactly one C0/H0/R41/N2 sector on track 0/head 0')
            return found[0]
        offset += size
    raise ValueError('physical track 0/head 0 not found')


def read_transfer():
    """Unroll the payload service path; DE counts only non-ready polls."""
    lines = []
    for index in range(512):
        wait, ready = f'read_wait_{index}', f'read_ready_{index}'
        lines += [f'{wait}: in a,(c)', f'    jp m,{ready}',
                  '    dec e', f'    jr nz,{wait}',
                  '    dec d', f'    jr nz,{wait}', '    jp read_timeout',
                  f'{ready}: cp 0xF0', '    jp c,read_early',
                  '    inc c', '    ini', '    inc b', '    dec c']
    return '\n'.join(lines) + '\n'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def build(disk, destination, hold_active=False):
    assembler = shutil.which('sjasmplus')
    if not assembler:
        raise SystemExit('sjasmplus not found on PATH')
    image = disk.read_bytes()
    expected, sector = sector_payload(image)
    destination.mkdir(parents=True, exist_ok=True)
    media = destination / 'fdc-read-probe-media.dsk'
    media.write_bytes(image)
    (destination / 'fdc-read-probe-expected.bin').write_bytes(expected)
    font = []
    for code in range(32, 91):
        rows = FONT.get(chr(code), FONT[' ']).split()
        font.append('    db ' + ','.join(str(int(row, 2) << 2) for row in rows) + ',0')
    (destination / 'fdc_read_probe.inc').write_text('font:\n' + '\n'.join(font) + '\n')
    (destination / 'fdc_read_transfer.inc').write_text(read_transfer())
    variants = ['fdc-read-probe'] + (['fdc-read-probe-hold-active'] if hold_active else [])
    artifacts = {}
    for stem in variants:
        raw = destination / (stem + '.bin')
        command = [assembler, '--nologo', '--msg=war', f'--inc={destination}',
                   f'--raw={raw}', f'--lst={destination / (stem + ".lst")}',
                   f'--sym={destination / (stem + ".sym")}']
        if stem.endswith('hold-active'):
            command += ['-DHOLD_ACTIVE=1']
        subprocess.run(command + [str(HERE / 'fdc_read_probe.asm')], check=True)
        code = raw.read_bytes()
        if len(code) > 0x3E00:
            raise ValueError('program overlaps reserved stack area BE00-BFFF')
        bank = code.ljust(16384, b'\0')
        payload = b'AMS!cb00' + struct.pack('<I', len(bank)) + bank
        cpr = b'RIFF' + struct.pack('<I', len(payload)) + payload
        (destination / (stem + '.cpr')).write_bytes(cpr)
        artifacts[stem] = dict(program_bytes=len(code), binary_sha256=sha(code),
                               cpr_sha256=sha(cpr))
        print(f'{stem}.cpr: {len(code)} program bytes, sha256={sha(cpr)}')
    metadata = dict(disk_source=str(disk.resolve()), media=media.name,
                    disk_bytes=len(image), disk_sha256=sha(image), sector=sector,
                    expected_sha256=sha(expected), read_command=READ_COMMAND.hex(' '),
                    variants=artifacts, hold_sample_limit=65535 if hold_active else None,
                    acceptance='Assembly/layout only; cold boot and real FDC behavior pending')
    (destination / 'fdc-read-probe.json').write_text(json.dumps(metadata, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disk', type=Path, default=HERE.parents[1] / 'rtl/u765/test.dsk')
    parser.add_argument('--output-dir', type=Path, default=HERE.parents[1] / 'output_files/fdc-read-probe')
    parser.add_argument('--hold-active', action='store_true', help='also build the live-MSR/no-data-consumption variant')
    args = parser.parse_args()
    try:
        build(args.disk, args.output_dir.resolve(), args.hold_active)
    except (ValueError, OSError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
