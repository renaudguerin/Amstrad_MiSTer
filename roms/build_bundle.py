#!/usr/bin/env python3
"""Build a classic CPC boot bundle from user-supplied OS and BASIC ROMs."""

import argparse
from pathlib import Path

PAGE_SIZE = 16 * 1024
LANGUAGES = {"en": "EN", "fr": "FR", "es": "SP", "da": "DA"}
MODEL_OFFSETS = {"6128": 0, "664": 4, "464": 8}


def read_image(path: Path, size: int) -> bytes:
    image = path.read_bytes()
    if len(image) != size:
        raise ValueError(f"{path} has {len(image)} bytes, expected {size}")
    return image


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="directory of OS_<model>_<lang>.rom and BASIC_<model>_<lang>.rom images")
    parser.add_argument("language", choices=LANGUAGES)
    parser.add_argument("output", type=Path, help="new .rom file to select in MiSTer")
    parser.add_argument("--base", type=Path, default=Path(__file__).with_name("boot.rom"),
                        help="existing ten-page bundle that supplies AMSDOS and Multiface 2")
    args = parser.parse_args()

    try:
        bundle = bytearray(read_image(args.base, 10 * PAGE_SIZE))
        suffix = LANGUAGES[args.language]
        for model, offset in MODEL_OFFSETS.items():
            start = offset * PAGE_SIZE
            os_rom = read_image(args.source / f"OS_{model}_{suffix}.rom", PAGE_SIZE)
            basic_rom = read_image(args.source / f"BASIC_{model}_{suffix}.rom", PAGE_SIZE)
            bundle[start:start + 2 * PAGE_SIZE] = os_rom + basic_rom
        with args.output.open("xb") as output_file:
            output_file.write(bundle)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
