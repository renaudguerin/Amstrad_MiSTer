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
never points at a half-written record. It also publishes the header with
`written = 0` when the observer is *enabled*, before any marker arrives, so a
second core load cannot hand this reader the previous run's records. That is a
bounded startup boundary and not a session identity: an old empty header still
looks like a fresh one, which is why `startup_seen` below is only meaningful
next to a controlled start.

`written` counts records that reached the ring and `dropped` counts markers the
writer could not enqueue. `dropped` saturates at 255, so that value means "at
least 255".

Read coherence: a poll reads the header, the whole ring and then the header
again. A record is only handed to the caller if the second header proves its
slot cannot have been reused during the read. One new event is enough to lose
the oldest slot a reader still needs, and the core writes a record before it
commits the header, so the margin allows for one uncounted record. Anything
outside the margin is counted in `lost` and reported, never quietly returned.
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

# SSM v1.1 reserved codes. The standard reserves "#0000 and all #FFxx codes
# (this represents 178 values)", which is 1 + the 177 legal values of LL with
# HH = #FF -- the arithmetic confirms the byte set in `ssm_byte_allowed`.
#
# Everything else is an ordinary code, and an ordinary code IS a screenshot
# request: "it can use the read SSM code to generate a screenshot immediately
# after reading the #HH byte". SHAKER assigns one such code per test screen and
# the portal's SHAKER_SCREENSHOT_CODE.xlsx maps them to reference images.
# #FFFE is not the general screenshot trigger; it is the variant that says
# "name this one from the CSL screenshot_name instead of from the code".
CODE_SYNC = 0x0000
CODE_SCREENSHOT = 0xFFFE
CODE_SNAPSHOT = 0xFFFF
CODE_SIKOVIEW_START = 0xFFFD
CODE_SIKOVIEW_BREAK = 0xFFFC


class SsmRingError(Exception):
    """The ring could not be read, or the bytes are not one we understand."""


class SsmTransportError(SsmRingError):
    """The device read itself failed: `dd` error, empty output, bad base64.

    This is never "no marker yet". It means the command did not produce the
    bytes, so nothing at all can be said about the ring.
    """


class SsmTruncatedError(SsmRingError):
    """Fewer bytes came back than the layout needs."""


class SsmFormatError(SsmRingError):
    """The bytes are the right length but do not describe a ring we support."""


class SsmNotInitializedError(SsmFormatError):
    """No SSM header is present at this address yet.

    Expected for a bounded window after a core load, while the observer has not
    been enabled or has not finished publishing its startup header. A caller
    that keeps seeing this past its startup budget has a real problem: the OSD
    bit is off, the base is wrong for this framework build, or the core is not
    the one that was loaded.
    """


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
            raise SsmTruncatedError(f"header is {len(data)} bytes, expected {HEADER_BYTES}")
        word0, word1 = struct.unpack_from("<QQ", data, 0)
        self.magic = word0 & 0xFFFFFFFF
        self.format_version = (word0 >> 32) & 0xFFFF
        self.entries = (word0 >> 48) & 0xFF
        self.written = word1 & 0xFFFFFFFF
        self.dropped = (word1 >> 32) & 0xFF

    def validate(self) -> None:
        if self.magic != MAGIC:
            raise SsmNotInitializedError(
                f"ring magic is 0x{self.magic:08X}, expected 0x{MAGIC:08X}. The core "
                "publishes the magic when the observer is enabled, so this means the "
                "SSM OSD option is off, the core has not finished loading, or the "
                "DDR3 base is wrong for this framework build."
            )
        if self.format_version != SUPPORTED_FORMAT:
            raise SsmFormatError(
                f"ring format version {self.format_version}, this reader implements "
                f"{SUPPORTED_FORMAT}"
            )
        if not 1 <= self.entries <= 255:
            raise SsmFormatError(f"implausible ring entry count {self.entries}")

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
        raise SsmTruncatedError(
            f"ring image is {len(data)} bytes, expected at least {needed}"
        )
    records = []
    for slot in range(header.entries):
        offset = HEADER_BYTES + slot * RECORD_BYTES
        word_a, word_b = struct.unpack_from("<QQ", data, offset)
        records.append(SsmRecord(word_a, word_b, slot))
    return header, records


def records_since(header: SsmHeader, records: List[SsmRecord], consumed: int,
                  written_after: Optional[int] = None) -> List[SsmRecord]:
    """Return the records written after `consumed`, oldest first.

    `consumed` is a count of records the caller has already handled.
    `written_after` is the `written` count read *after* the ring image; pass it
    to exclude slots the writer may have reused while the image was being read.

    The margin is deliberately conservative. The core writes a record pair
    before it commits the header, so at the moment the second header says
    `written_after`, slot `written_after` may already hold its successor. A
    reader may therefore only trust indices at or above
    `written_after - entries + 1`.
    """
    if header.written <= consumed:
        return []
    oldest_kept = header.written - header.entries
    if written_after is not None:
        oldest_kept = max(oldest_kept, written_after - header.entries + 1)
    first = max(consumed, oldest_kept, 0)
    if first >= header.written:
        return []
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
    text = stdout.strip()
    if not text:
        raise SsmTransportError(
            "the device returned no bytes for the ring read; dd produced nothing, "
            "which is a transport or permission failure and not an empty ring"
        )
    try:
        return base64.b64decode(text, validate=True)
    except (binascii.Error, ValueError) as exc:
        raise SsmTransportError(f"malformed base64 from the device: {exc}") from exc


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
        self.restarts = 0
        self.startup_seen = False
        self.last_margin: Optional[int] = None

    def _read(self, header_only: bool) -> bytes:
        command = read_command(self.base, self.entries, header_only=header_only)
        result = self.transport.run_cmd(command, timeout=self.timeout)
        if result.exit_code != 0:
            raise SsmTransportError(
                f"reading /dev/mem failed ({result.exit_code}): {result.stderr.strip()}"
            )
        return decode_payload(result.stdout)

    def header(self) -> SsmHeader:
        header = parse_header(self._read(header_only=True))
        if header.written == 0:
            self.startup_seen = True
        return header

    def poll(self) -> Tuple[SsmHeader, List[SsmRecord]]:
        """Return the header and every record this reader can trust.

        Reads the image, then the header again, and only returns records whose
        slots the second header proves were not reused during the read.
        Anything else is counted in `lost`.
        """
        header, records = parse_ring(self._read(header_only=False))
        after = parse_header(self._read(header_only=True))

        if header.written == 0:
            self.startup_seen = True

        # Enabling the observer republishes the header with written = 0, so a
        # count that went backwards is a restart, not a lost record.
        if header.written < self.consumed:
            self.restarts += 1
            self.consumed = 0

        new = records_since(header, records, self.consumed,
                            written_after=after.written)
        self.lost += max(0, (header.written - self.consumed) - len(new))
        self.last_margin = self.entries - (after.written - self.consumed)
        self.consumed = header.written
        return header, new

    def reset(self) -> None:
        self.consumed = 0
        self.lost = 0
        self.restarts = 0
        self.startup_seen = False
        self.last_margin = None


def is_reserved(code: int) -> bool:
    """True for #0000 and every #FFxx, the codes the standard holds back."""
    return code == CODE_SYNC or (code >> 8) == 0xFF


def is_screenshot_request(code: int) -> bool:
    """True if this code asks for a screenshot.

    Any non-reserved code does, named from the code itself. #FFFE also does,
    named from the CSL `screenshot_name` instead. The remaining reserved codes
    do not.
    """
    return code == CODE_SCREENSHOT or not is_reserved(code)


def suggested_name(emulator: str, crtc: str, code: int, extension: str = "png") -> str:
    """SSM v1.1 suggested image name: <Emulator>_<CRTC>_<HHLL code>.<ext>."""
    return f"{emulator}_{crtc}_{code:04X}.{extension}"
