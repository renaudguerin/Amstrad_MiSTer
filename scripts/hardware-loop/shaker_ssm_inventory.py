#!/usr/bin/env python3
"""Static SSM marker inventory for a SHAKER disc image.

Answers, without hardware and without executing anything, which SSM codes a
SHAKER build actually emits. That question turned out to gate the whole
`#FFFE` capture path: see
`docs/shaker-ssm-marker-inventory-2026-09-12.md`.

Reads an EDSK or standard DSK image, walks the AMSDOS DATA-format directory,
reassembles each module binary, and scans it for `ED LL ED HH` pairs whose
bytes are both legal SSM values. The reassembly is self-checking: an AMSDOS
binary header declares its own length, and a mismatch against the number of
bytes the directory yields means the extraction is wrong and the scan cannot
be trusted.

Usage:
    python3 scripts/hardware-loop/shaker_ssm_inventory.py docs/references/Shaker_CSL/shaker26.dsk

The disc images are user-owned and untracked; nothing here writes to the
repository or reproduces disc content, it only counts and locates byte
patterns.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# SSM v1.1 byte ranges, plus FE/FF which the standard reserves for its own
# codes and therefore uses itself. Same set as rtl/ssm_marker.v.
def ssm_byte_allowed(b: int) -> bool:
    return (
        b <= 0x3F
        or 0x7F <= b <= 0x9F
        or 0xA4 <= b <= 0xA7
        or 0xAC <= b <= 0xAF
        or 0xB4 <= b <= 0xB7
        or 0xBC <= b <= 0xBF
        or b >= 0xC0
    )


RESERVED_NAMES = {
    0x0000: "sync, releases CSL wait_ssm0000",
    0xFFFC: "Sikoview logging breakpoint",
    0xFFFD: "Sikoview logging start",
    0xFFFE: "screenshot",
    0xFFFF: "snapshot",
}

# AMSDOS DATA format: 9 sectors per track, ids C1..C9, 512-byte sectors,
# 1 KB blocks, directory in the first four logical sectors.
SECTORS_PER_TRACK = 9
FIRST_SECTOR_ID = 0xC1
RECORDS_PER_SECTOR = 4          # 512 bytes / 128-byte CP/M records
DIRECTORY_SECTORS = 4


class DiskError(Exception):
    pass


def read_tracks(data: bytes) -> Dict[Tuple[int, int], Dict[int, bytes]]:
    """Return {(track, side): {sector_id: bytes}} for an EDSK or DSK image."""
    if data[:8] == b"EXTENDED":
        tracks_n, sides_n = data[0x30], data[0x31]
        sizes = [data[0x34 + i] * 256 for i in range(tracks_n * sides_n)]
    elif data[:8] == b"MV - CPC":
        tracks_n, sides_n = data[0x30], data[0x31]
        fixed = struct.unpack_from("<H", data, 0x32)[0]
        sizes = [fixed] * (tracks_n * sides_n)
    else:
        raise DiskError("not an EDSK or standard DSK image")

    out: Dict[Tuple[int, int], Dict[int, bytes]] = {}
    offset = 0x100
    for index, size in enumerate(sizes):
        if size == 0:
            continue                      # unformatted track
        block = data[offset : offset + size]
        if block[:10] != b"Track-Info":
            raise DiskError(f"track {index} has no Track-Info block")
        track, side, count = block[0x10], block[0x11], block[0x15]
        sectors: Dict[int, bytes] = {}
        position = 0x100
        for s in range(count):
            entry = 0x18 + s * 8
            sector_id = block[entry + 2]
            declared = struct.unpack_from("<H", block, entry + 6)[0]
            length = declared or (128 << block[entry + 3])
            sectors[sector_id] = block[position : position + length]
            position += length
        out[(track, side)] = sectors
        offset += size
    return out


def logical_sector(tracks: Dict[Tuple[int, int], Dict[int, bytes]], index: int) -> bytes:
    track, within = divmod(index, SECTORS_PER_TRACK)
    try:
        return tracks[(track, 0)][FIRST_SECTOR_ID + within]
    except KeyError as exc:
        raise DiskError(f"logical sector {index} is missing from the image") from exc


def directory(tracks: Dict[Tuple[int, int], Dict[int, bytes]]) -> List[Dict[str, object]]:
    raw = b"".join(logical_sector(tracks, i) for i in range(DIRECTORY_SECTORS))
    entries = []
    for offset in range(0, len(raw), 32):
        entry = raw[offset : offset + 32]
        if entry[0] == 0xE5:              # deleted
            continue
        name = entry[1:9].decode("ascii", "replace").strip()
        # AMSDOS uses the high bits of the extension for attributes.
        ext = bytes(b & 0x7F for b in entry[9:12]).decode("ascii", "replace").strip()
        entries.append({
            "user": entry[0], "name": name, "ext": ext,
            "extent": entry[12], "records": entry[15],
            "blocks": [b for b in entry[16:32] if b],
        })
    return entries


def extract(tracks: Dict[Tuple[int, int], Dict[int, bytes]], name: str, ext: str) -> bytes:
    parts = [e for e in directory(tracks) if e["name"] == name and e["ext"] == ext]
    if not parts:
        return b""
    parts.sort(key=lambda e: e["extent"])
    out = b""
    for part in parts:
        remaining = int(part["records"])
        for block in part["blocks"]:
            for half in (0, 1):
                if remaining <= 0:
                    break
                out += logical_sector(tracks, block * 2 + half)
                remaining -= RECORDS_PER_SECTOR
    return out


def amsdos_body(raw: bytes) -> Tuple[int, int, bytes]:
    """Strip the 128-byte AMSDOS header and return (load, declared, body).

    The declared length is the file's own claim, so comparing it against what
    the directory yielded is a free correctness check on the reassembly.
    """
    if len(raw) < 128:
        raise DiskError("file is shorter than an AMSDOS header")
    load = struct.unpack_from("<H", raw, 21)[0]
    declared = struct.unpack_from("<H", raw, 24)[0]
    body = raw[128 : 128 + declared] if declared else raw[128:]
    return load, declared, body


def scan(body: bytes) -> List[Tuple[int, int]]:
    """Return [(offset, code)] for every legal SSM marker, non-overlapping."""
    hits, i = [], 0
    while i + 3 < len(body):
        if (body[i] == 0xED and body[i + 2] == 0xED
                and ssm_byte_allowed(body[i + 1]) and ssm_byte_allowed(body[i + 3])):
            hits.append((i, (body[i + 3] << 8) | body[i + 1]))
            i += 4
        else:
            i += 1
    return hits


def inventory(image: Path, prefixes: Optional[List[str]] = None) -> Dict[str, object]:
    tracks = read_tracks(image.read_bytes())
    names = sorted({str(e["name"]) for e in directory(tracks) if e["ext"] == "BIN"})
    if prefixes:
        names = [n for n in names if any(n.startswith(p) for p in prefixes)]

    modules, totals, exact = [], {}, True
    for name in names:
        load, declared, body = amsdos_body(extract(tracks, name, "BIN"))
        hits = scan(body)
        counts: Dict[int, int] = {}
        for _, code in hits:
            counts[code] = counts.get(code, 0) + 1
        for code, n in counts.items():
            totals[code] = totals.get(code, 0) + n
        ok = declared == len(body)
        exact &= ok
        modules.append({
            "name": name, "load": load, "declared": declared,
            "extracted": len(body), "extraction_exact": ok,
            "codes": counts,
            "addresses": {f"{code:04X}": [f"{load + off:04X}" for off, c in hits if c == code]
                          for code in sorted(counts)},
        })
    return {"image": str(image), "modules": modules, "totals": totals,
            "extraction_exact": exact}


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", type=Path, nargs="+", help="DSK/EDSK image(s)")
    parser.add_argument("--prefix", action="append", default=None,
                        help="Only inventory .BIN files starting with this (repeatable)")
    args = parser.parse_args(argv)

    status = 0
    for image in args.image:
        if not image.is_file():
            print(f"missing: {image}", file=sys.stderr)
            status = 1
            continue
        report = inventory(image, args.prefix)
        print(f"=== {image.name} ===")
        for module in report["modules"]:
            flag = "" if module["extraction_exact"] else "  !! EXTRACTION MISMATCH"
            codes = ", ".join(
                f"#{code:04X}x{n}" + (f" ({RESERVED_NAMES[code]})" if code in RESERVED_NAMES else "")
                for code, n in sorted(module["codes"].items())
            ) or "none"
            print(f"  {module['name']:<10} load=&{module['load']:04X} "
                  f"len={module['declared']:>6}  {codes}{flag}")
            if not module["extraction_exact"]:
                status = 1
        print("  totals: " + (", ".join(f"#{c:04X}x{n}" for c, n in sorted(report["totals"].items()))
                              or "no SSM markers at all"))
    return status


if __name__ == "__main__":
    raise SystemExit(main())
