#!/usr/bin/env python3
"""Reader for the SSM event ring the core publishes in DDR3.

`rtl/ssm_marker.v` writes a 16-byte header plus a 64-entry ring of 16-byte
records at the core-reserved DDR3 base. Nothing on the device serves it: the
host reads the physical memory directly over the existing SSH transport with
BusyBox `dd if=/dev/mem`, so there is no Main patch and no daemon.

Layout, little-endian throughout, matching the RTL comment:

    +0x00  magic ("SSM1") | format version | ring entry count
    +0x08  records written | records dropped
    +0x10  record 0 word A, +0x18 record 0 word B, one pair per slot

    word A  [15:0] code  [39:16] frame  [49:40] line  [57:50] hpos  [58] field
    word B  [15:0] sequence number  [47:16] core clock tick

The core writes a record pair before it updates the header, so a header read
never points at a half-written record.

`written` counts records that reached the ring and `dropped` counts markers the
writer could not enqueue. A reader that falls more than one ring behind sees
`written` jump by more than the entry count, which is the only honest way to
say that records were overwritten.
"""

from __future__ import annotations

import base64
import binascii
import struct
from typing import Any, Dict, List, Optional, Tuple

MAGIC = 0x53534D31  # "SSM1"
SUPPORTED_FORMAT = 1
HEADER_BYTES = 16
RECORD_BYTES = 16
DEFAULT_BASE = 0x3000_0000

# SSM v1.1 reserved codes.
CODE_SYNC = 0x0000
CODE_SCREENSHOT = 0xFFFE
CODE_SNAPSHOT = 0xFFFF
CODE_SIKOVIEW_START = 0xFFFD
CODE_SIKOVIEW_BREAK = 0xFFFC


class SsmRingError(Exception):
    """The bytes read back are not a ring this reader understands."""


class SsmRecord:
    __slots__ = ("code", "frame", "line", "hpos", "field", "seq", "tick", "slot")

    def __init__(self, word_a: int, word_b: int, slot: int):
        self.code = word_a & 0xFFFF
        self.frame = (word_a >> 16) & 0xFFFFFF
        self.line = (word_a >> 40) & 0x3FF
        self.hpos = (word_a >> 50) & 0xFF
        self.field = (word_a >> 58) & 1
        self.seq = word_b & 0xFFFF
        self.tick = (word_b >> 16) & 0xFFFFFFFF
        self.slot = slot

    def as_dict(self) -> Dict[str, Any]:
        return {
            "code": f"{self.code:04X}",
            "frame": self.frame,
            "line": self.line,
            "hpos": self.hpos,
            "field": self.field,
            "seq": self.seq,
            "tick": self.tick,
            "slot": self.slot,
        }

    def __repr__(self) -> str:  # pragma: no cover - debugging aid
        return f"SsmRecord(#{self.code:04X} frame={self.frame} line={self.line})"


class SsmHeader:
    __slots__ = ("magic", "format_version", "entries", "written", "dropped")

    def __init__(self, data: bytes):
        if len(data) < HEADER_BYTES:
            raise SsmRingError(f"header is {len(data)} bytes, expected {HEADER_BYTES}")
        word0, word1 = struct.unpack_from("<QQ", data, 0)
        self.magic = word0 & 0xFFFFFFFF
        self.format_version = (word0 >> 32) & 0xFFFF
        self.entries = (word0 >> 48) & 0xFF
        self.written = word1 & 0xFFFFFFFF
        self.dropped = (word1 >> 32) & 0xFF

    def validate(self) -> None:
        if self.magic != MAGIC:
            raise SsmRingError(
                f"ring magic is 0x{self.magic:08X}, expected 0x{MAGIC:08X}. The core "
                "writes the magic on its first marker, so this usually means the SSM "
                "OSD option is off, no marker has been seen yet, or the DDR3 base is "
                "wrong for this framework build."
            )
        if self.format_version != SUPPORTED_FORMAT:
            raise SsmRingError(
                f"ring format version {self.format_version}, this reader implements "
                f"{SUPPORTED_FORMAT}"
            )
        if not 1 <= self.entries <= 255:
            raise SsmRingError(f"implausible ring entry count {self.entries}")

    def as_dict(self) -> Dict[str, Any]:
        return {
            "magic": f"{self.magic:08X}",
            "format_version": self.format_version,
            "entries": self.entries,
            "written": self.written,
            "dropped": self.dropped,
        }


def parse_header(data: bytes) -> SsmHeader:
    header = SsmHeader(data)
    header.validate()
    return header


def parse_ring(data: bytes) -> Tuple[SsmHeader, List[SsmRecord]]:
    """Parse a full ring image into its header and slots, in slot order."""
    header = parse_header(data)
    needed = HEADER_BYTES + header.entries * RECORD_BYTES
    if len(data) < needed:
        raise SsmRingError(f"ring image is {len(data)} bytes, expected at least {needed}")
    records = []
    for slot in range(header.entries):
        offset = HEADER_BYTES + slot * RECORD_BYTES
        word_a, word_b = struct.unpack_from("<QQ", data, offset)
        records.append(SsmRecord(word_a, word_b, slot))
    return header, records


def records_since(header: SsmHeader, records: List[SsmRecord], consumed: int) -> List[SsmRecord]:
    """Return the records written after `consumed`, oldest first.

    `consumed` is a count of records the caller has already handled. Records
    older than the ring can hold are gone; the caller compares the returned
    count against `header.written - consumed` to see how many were lost.
    """
    if header.written <= consumed:
        return []
    first = max(consumed, header.written - header.entries)
    return [records[index % header.entries] for index in range(first, header.written)]


def ring_size_bytes(entries: int = 64) -> int:
    return HEADER_BYTES + entries * RECORD_BYTES


def read_command(base: int = DEFAULT_BASE, entries: int = 64, header_only: bool = False) -> str:
    """Shell command that prints the ring as one base64 line.

    `dd` is given a block size that divides the base address, because BusyBox
    has no `iflag=skip_bytes`. /dev/mem is read-only here.
    """
    total = HEADER_BYTES if header_only else ring_size_bytes(entries)
    block = 16
    if base % block:
        raise SsmRingError(f"ring base 0x{base:08X} is not {block}-byte aligned")
    count = (total + block - 1) // block
    return (
        f"dd if=/dev/mem bs={block} skip={base // block} count={count} 2>/dev/null "
        "| base64 | tr -d '\\n'"
    )


def decode_payload(stdout: str) -> bytes:
    try:
        return base64.b64decode(stdout.strip(), validate=True)
    except (binascii.Error, ValueError) as exc:
        raise SsmRingError(f"malformed base64 from the device: {exc}") from exc


class SsmRingReader:
    """Polls the ring over an SSHTransport-shaped object."""

    def __init__(self, transport: Any, base: int = DEFAULT_BASE, entries: int = 64,
                 timeout: float = 20.0):
        self.transport = transport
        self.base = base
        self.entries = entries
        self.timeout = timeout
        self.consumed = 0
        self.lost = 0

    def _read(self, header_only: bool) -> bytes:
        command = read_command(self.base, self.entries, header_only=header_only)
        result = self.transport.run_cmd(command, timeout=self.timeout)
        if result.exit_code != 0:
            raise SsmRingError(
                f"reading /dev/mem failed ({result.exit_code}): {result.stderr.strip()}"
            )
        return decode_payload(result.stdout)

    def header(self) -> SsmHeader:
        return parse_header(self._read(header_only=True))

    def poll(self) -> Tuple[SsmHeader, List[SsmRecord]]:
        """Return the header and every record written since the last poll."""
        header, records = parse_ring(self._read(header_only=False))
        new = records_since(header, records, self.consumed)
        self.lost += max(0, (header.written - self.consumed) - len(new))
        self.consumed = header.written
        return header, new

    def reset(self) -> None:
        self.consumed = 0
        self.lost = 0


def suggested_name(emulator: str, crtc: str, code: int, extension: str = "png") -> str:
    """SSM v1.1 suggested image name: <Emulator>_<CRTC>_<HHLL code>.<ext>."""
    return f"{emulator}_{crtc}_{code:04X}.{extension}"
