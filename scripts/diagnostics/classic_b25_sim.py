#!/usr/bin/env python3
"""Extract the actual DSK directory entry and execute its body on production T80.

Packaging extraction is independent of the builder. This is NOT a firmware or
FDC boot test: verify RUN\"B25 separately in an AMSDOS machine/emulator.
"""
import argparse
import importlib.util
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('model', type=Path)
    p.add_argument('--out', type=Path, default=ROOT / 'output_files/classic-b25')
    args = p.parse_args()
    import classic_b25
    disk = classic_b25.build(args.out)
    spec = importlib.util.spec_from_file_location('inventory', ROOT / 'scripts/hardware-loop/shaker_ssm_inventory.py')
    inventory = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(inventory)
    raw = inventory.extract(inventory.read_tracks(disk.read_bytes()), 'B25', 'BIN')
    load, length, body = inventory.amsdos_body(raw)
    assert load == 0x1000 and len(body) == length
    assert int.from_bytes(raw[26:28], 'little') == load
    assert sum(raw[:67]) == int.from_bytes(raw[67:69], 'little')
    assert body == (args.out / 'classic-b25.bin').read_bytes()
    extracted = args.out / 'disk-extracted.bin'
    extracted.write_bytes(body)
    subprocess.run([str(args.model.resolve()), str(extracted), str(args.out / 'screen')], check=True)
    # Render the CPC's standard 80-byte x 25-row, eight-raster memory layout.
    for crtc in (0, 1):
        screen = (args.out / f'screen-crtc{crtc}.scr').read_bytes()
        pixels = bytearray()
        for y in range(200):
            for x in range(640):
                value = screen[(y // 8) * 80 + (y % 8) * 2048 + x // 8]
                pixels.append(255 if value & (128 >> (x % 8)) else 0)
        (args.out / f'screen-crtc{crtc}.pgm').write_bytes(b'P5\n640 200\n255\n' + pixels)


if __name__ == '__main__':
    main()
