#!/usr/bin/env python3
"""SNA snapshot DDR3 save stream pull tool (B18).

Pulls a classic SNA snapshot published by the running core into a DDR3 slot at
0x3E000000 via SSH and /dev/mem mmap.

This is a development aid, not the user-facing save path: the core cannot write the
file to SD by itself yet. See "Status and limitation" in docs/b18-sna-save.md.

Publication layout in DDR3 (64-bit little-endian words):
    +0x00 (word 0): generation {32'd0, gen} with gen in 1..0xFFFFFFFE.
                    0xFFFFFFFFFFFFFFFF indicates publication in progress / invalid.
    +0x08 (word 1): {32'd0, file_bytes / 4}. Legal values:
                    0x8040 (128K snapshot, 0x20100 bytes)
                    0x4040 (64K snapshot, 0x10100 bytes)
    +0x10 onward:   the .sna file bytes in order (256-byte header, then RAM).

File header checks:
    bytes 0-7:      "MV - SNA"
    byte 0x10:      3 (SNA v3)
    bytes 0x6B-0x6C: memory size in KB (128 for 0x8040 length, 64 for 0x4040).
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
import tempfile
import time
from pathlib import Path
from typing import Any, List, Optional, Tuple

# Ensure hardware-loop directory is in sys.path for direct imports
_hw_loop_dir = str(Path(__file__).resolve().parent)
if _hw_loop_dir not in sys.path:
    sys.path.insert(0, _hw_loop_dir)

from driver import DriverError, SSHTransport, validate_port, validate_ssh_target
from ssm_ring import SsmTransportError, decode_payload

DEFAULT_BASE = 0x3E000000
DEFAULT_PORT = 22
DEFAULT_TIMEOUT = 20.0
DEFAULT_RETRIES = 5
DEFAULT_RETRY_DELAY = 0.1
DEFAULT_POLL_INTERVAL = 1.0

# Legal length values in word 1 (in 32-bit words)
LEN_WORDS_128K = 0x8040  # 0x20100 bytes (131328) -> 128 KB RAM + 256 B header
LEN_WORDS_64K = 0x4040   # 0x10100 bytes (65792)  -> 64 KB RAM + 256 B header
LEGAL_LEN_WORDS = (LEN_WORDS_128K, LEN_WORDS_64K)

SNA_SIGNATURE = b"MV - SNA"
SNA_VERSION = 3


class SnaPullError(Exception):
    """Base exception for SNA pull failures."""


class NoSnapshotError(SnaPullError):
    """No valid snapshot is present at the DDR3 slot."""


class TornReadError(SnaPullError):
    """Snapshot read was torn (core published during read) and retries were exhausted."""


class SnaTransportError(SnaPullError):
    """Transport failure while reading /dev/mem on the device."""


class SnaTruncatedError(SnaPullError):
    """Device returned fewer bytes than requested."""


class SnaWaitTimeoutError(SnaPullError):
    """Timed out waiting for a new snapshot generation."""


class SlotHeader:
    __slots__ = ("generation", "len_words", "file_bytes", "raw")

    def __init__(self, generation: int, len_words: int, raw: bytes):
        self.generation = generation
        self.len_words = len_words
        self.file_bytes = len_words * 4
        self.raw = raw

    def __repr__(self) -> str:
        return f"SlotHeader(gen={self.generation}, len_words=0x{self.len_words:04X}, file_bytes={self.file_bytes})"


def read_command(base: int, size: int) -> str:
    """Shell command that reads `size` bytes from physical `base` via mmap and prints base64."""
    if base % 16 != 0:
        raise SnaPullError(f"base 0x{base:08X} is not 16-byte aligned")
    if size <= 0:
        raise SnaPullError(f"size must be positive, got {size}")
    return (
        f"python3 -c 'import mmap,os,sys,base64; "
        f"b={base}; s={size}; p=os.sysconf(\"SC_PAGE_SIZE\"); "
        f"pb=b&~(p-1); o=b-pb; f=os.open(\"/dev/mem\",os.O_RDONLY|os.O_SYNC); "
        f"m=mmap.mmap(f,o+s,mmap.MAP_SHARED,mmap.PROT_READ,offset=pb); "
        f"sys.stdout.buffer.write(base64.b64encode(m[o:o+s])); "
        f"m.close(); os.close(f)'"
    )


def read_mem(transport: Any, base: int, size: int, timeout: float = DEFAULT_TIMEOUT) -> bytes:
    """Read `size` bytes from physical memory at `base` over the transport."""
    cmd = read_command(base, size)
    try:
        res = transport.run_cmd(cmd, timeout=timeout)
    except DriverError as exc:
        raise SnaTransportError(str(exc)) from exc

    if res.exit_code != 0:
        raise SnaTransportError(f"reading /dev/mem failed ({res.exit_code}): {res.stderr.strip()}")

    try:
        data = decode_payload(res.stdout)
    except SsmTransportError as exc:
        raise SnaTransportError(str(exc)) from exc

    if len(data) < size:
        raise SnaTruncatedError(f"read {len(data)} bytes from device, expected {size}")
    return data[:size]


def parse_slot_header(data: bytes) -> SlotHeader:
    """Validate and parse the 16-byte slot header.

    Raises NoSnapshotError if the header does not represent a valid snapshot.
    """
    if len(data) < 16:
        raise SnaTruncatedError(f"slot header is {len(data)} bytes, expected 16")

    word0, word1 = struct.unpack_from("<QQ", data, 0)
    if word0 == 0xFFFFFFFFFFFFFFFF:
        raise NoSnapshotError("no snapshot: publication in progress / invalid generation (0xFFFFFFFFFFFFFFFF)")

    gen_low = word0 & 0xFFFFFFFF
    gen_high = (word0 >> 32) & 0xFFFFFFFF
    if gen_high != 0:
        raise NoSnapshotError(f"no snapshot: nonzero high 32 bits in word 0 (0x{gen_high:08X})")
    if not (1 <= gen_low <= 0xFFFFFFFE):
        raise NoSnapshotError(f"no snapshot: generation 0x{gen_low:08X} out of valid range 1..0xFFFFFFFE")

    len_words = word1 & 0xFFFFFFFF
    len_high = (word1 >> 32) & 0xFFFFFFFF
    if len_high != 0:
        raise NoSnapshotError(f"no snapshot: nonzero high 32 bits in word 1 (0x{len_high:08X})")
    if len_words not in LEGAL_LEN_WORDS:
        raise NoSnapshotError(f"no snapshot: illegal length word 0x{len_words:04X}, expected 0x8040 or 0x4040")

    return SlotHeader(generation=gen_low, len_words=len_words, raw=bytes(data[:16]))


def validate_sna_payload(payload: bytes, len_words: int) -> None:
    """Validate SNA file header contents within the payload.

    Raises NoSnapshotError if signature, version, or memory size mismatch.
    """
    expected_len = len_words * 4
    if len(payload) < expected_len:
        raise SnaTruncatedError(f"payload is {len(payload)} bytes, expected {expected_len}")

    if payload[:8] != SNA_SIGNATURE:
        raise NoSnapshotError(f"no snapshot: bad SNA signature {payload[:8]!r}, expected {SNA_SIGNATURE!r}")

    if payload[0x10] != SNA_VERSION:
        raise NoSnapshotError(f"no snapshot: byte 0x10 is {payload[0x10]}, expected {SNA_VERSION}")

    mem_size_kb = struct.unpack_from("<H", payload, 0x6B)[0]
    expected_kb = 128 if len_words == LEN_WORDS_128K else 64
    if mem_size_kb != expected_kb:
        raise NoSnapshotError(
            f"no snapshot: memory size {mem_size_kb} KB at 0x6B does not match length 0x{len_words:04X} ({expected_kb} KB)"
        )


def pull_snapshot(
    transport: Any,
    base: int = DEFAULT_BASE,
    max_retries: int = DEFAULT_RETRIES,
    retry_delay: float = DEFAULT_RETRY_DELAY,
    timeout: float = DEFAULT_TIMEOUT,
) -> Tuple[SlotHeader, bytes]:
    """Perform a coherent pull of the SNA snapshot from the DDR3 slot.

    Reads the 16-byte slot header; if valid, reads 16 + file_bytes bytes; then
    reads the 16-byte slot header again. Accepts only when both headers are valid
    and identical and the image's own first 16 bytes match too.

    Returns (SlotHeader, payload_bytes).
    """
    for attempt in range(max_retries):
        hdr1_raw = read_mem(transport, base, 16, timeout=timeout)
        if attempt == 0:
            hdr1 = parse_slot_header(hdr1_raw)
        else:
            try:
                hdr1 = parse_slot_header(hdr1_raw)
            except NoSnapshotError:
                if attempt + 1 < max_retries:
                    time.sleep(retry_delay)
                    continue
                raise TornReadError(f"snapshot read torn after {max_retries} attempts: slot header invalid during publication")

        total_bytes = 16 + hdr1.file_bytes
        image = read_mem(transport, base, total_bytes, timeout=timeout)
        hdr2_raw = read_mem(transport, base, 16, timeout=timeout)

        try:
            hdr2 = parse_slot_header(hdr2_raw)
        except NoSnapshotError:
            hdr2 = None

        is_coherent = (
            hdr2 is not None
            and hdr1.generation == hdr2.generation
            and hdr1.len_words == hdr2.len_words
            and hdr1_raw == hdr2_raw
            and image[:16] == hdr1_raw
        )

        if not is_coherent:
            if attempt + 1 < max_retries:
                time.sleep(retry_delay)
                continue
            raise TornReadError(
                f"snapshot read torn after {max_retries} attempts: core was publishing during read"
            )

        payload = image[16:16 + hdr1.file_bytes]
        validate_sna_payload(payload, hdr1.len_words)
        return hdr1, payload

    raise TornReadError(f"snapshot read torn after {max_retries} attempts")


def wait_for_generation(
    transport: Any,
    base: int = DEFAULT_BASE,
    wait_seconds: float = 30.0,
    after_generation: Optional[int] = None,
    poll_interval: float = DEFAULT_POLL_INTERVAL,
    timeout: float = DEFAULT_TIMEOUT,
) -> int:
    """Poll the 16-byte slot header until a valid generation different from after_generation appears.

    If after_generation is omitted, the starting generation (or None if no snapshot
    is present at start) is used as baseline.
    """
    if after_generation is not None:
        baseline = after_generation
    else:
        try:
            raw = read_mem(transport, base, 16, timeout=timeout)
            hdr = parse_slot_header(raw)
            baseline = hdr.generation
        except NoSnapshotError:
            baseline = None

    start_time = time.monotonic()
    deadline = start_time + wait_seconds

    while True:
        try:
            raw = read_mem(transport, base, 16, timeout=timeout)
            hdr = parse_slot_header(raw)
            if baseline is None or hdr.generation != baseline:
                return hdr.generation
        except NoSnapshotError:
            pass

        now = time.monotonic()
        if now >= deadline:
            target_desc = f"generation {baseline}" if baseline is not None else "none"
            raise SnaWaitTimeoutError(
                f"timed out after {wait_seconds:.1f}s waiting for new snapshot (after {target_desc})"
            )

        sleep_time = min(poll_interval, max(0.0, deadline - now))
        time.sleep(sleep_time)


def wait_and_pull(
    transport: Any,
    base: int = DEFAULT_BASE,
    wait_seconds: float = 30.0,
    after_generation: Optional[int] = None,
    poll_interval: float = DEFAULT_POLL_INTERVAL,
    max_retries: int = DEFAULT_RETRIES,
    retry_delay: float = DEFAULT_RETRY_DELAY,
    timeout: float = DEFAULT_TIMEOUT,
) -> Tuple[SlotHeader, bytes]:
    """Wait for a new snapshot generation and pull it coherently."""
    wait_for_generation(
        transport=transport,
        base=base,
        wait_seconds=wait_seconds,
        after_generation=after_generation,
        poll_interval=poll_interval,
        timeout=timeout,
    )
    return pull_snapshot(
        transport=transport,
        base=base,
        max_retries=max_retries,
        retry_delay=retry_delay,
        timeout=timeout,
    )


def write_snapshot_file(out_path: Path, data: bytes) -> None:
    """Write snapshot data to out_path atomically using a temporary file in the same directory."""
    out_path = out_path.resolve()
    out_dir = out_path.parent
    out_dir.mkdir(parents=True, exist_ok=True)
    temp_path: Optional[Path] = None
    try:
        with tempfile.NamedTemporaryFile(dir=out_dir, prefix=f".{out_path.name}.", suffix=".tmp", delete=False) as f:
            temp_path = Path(f.name)
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        os.replace(temp_path, out_path)
        temp_path = None
    finally:
        if temp_path is not None and temp_path.exists():
            try:
                temp_path.unlink()
            except OSError:
                pass


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Pull SNA snapshot from MiSTer DDR3 slot.")
    parser.add_argument("--target", required=True, help="SSH target (e.g. root@mister)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="SSH port (default: 22)")
    parser.add_argument(
        "--base",
        type=lambda s: int(s, 0),
        default=DEFAULT_BASE,
        help="DDR3 physical byte address (default: 0x3E000000)",
    )
    parser.add_argument("--out", required=True, help="Output .sna file path")
    parser.add_argument(
        "--wait",
        type=float,
        default=None,
        metavar="SECONDS",
        help="Wait for a new snapshot generation (timeout in seconds)",
    )
    parser.add_argument(
        "--after-generation",
        type=int,
        default=None,
        metavar="N",
        help="Wait for generation different from N (used with --wait)",
    )

    args = parser.parse_args(argv)

    try:
        validate_ssh_target(args.target)
        validate_port(args.port)
        transport = SSHTransport(target=args.target, port=args.port)
        out_path = Path(args.out)

        if args.wait is not None:
            header, payload = wait_and_pull(
                transport=transport,
                base=args.base,
                wait_seconds=args.wait,
                after_generation=args.after_generation,
            )
        else:
            if args.after_generation is not None:
                hdr_raw = read_mem(transport, args.base, 16)
                hdr = parse_slot_header(hdr_raw)
                if hdr.generation == args.after_generation:
                    raise NoSnapshotError(f"no snapshot: generation {hdr.generation} matches --after-generation")
            header, payload = pull_snapshot(transport, base=args.base)

        write_snapshot_file(out_path, payload)
        print(f"Generation {header.generation}, {header.file_bytes} bytes written to {out_path}")
        return 0
    except (SnaPullError, DriverError, ValueError) as exc:
        msg = str(exc).strip().replace("\n", " ")
        sys.stderr.write(f"Error: {msg}\n")
        return 1
    except Exception as exc:
        msg = str(exc).strip().replace("\n", " ")
        sys.stderr.write(f"Error: {msg}\n")
        return 1


if __name__ == "__main__":
    sys.exit(main())
