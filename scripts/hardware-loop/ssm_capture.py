#!/usr/bin/env python3
"""Reader and decoder for the experimental SSM capture region (B4 phase 2).

This implements the host side of [docs/ssm-capture-abi.md](../../docs/ssm-capture-abi.md),
which is versioned separately from the format-1 event ring in ``ssm_ring.py``.
An experimental capture build therefore cannot change what the ordinary marker
path reads.

Nothing here has run against hardware. The recorder is compile-time off in the
core, the DDR3 region is not reserved, and the raster profiles below are
declared assumptions rather than measurements. What the decoder *will* do is
refuse to invent: a capture whose windows were reused, whose history has gaps,
whose sync does not match its profile or whose samples were lost produces an
explicit incomplete result with the raw trace attached, never a plausible
rectangle.

Offline use, on a region dumped from a device by some other means::

    python3 ssm_capture.py decode --region dump.bin --capture 0 \\
        --profile cpc-native-progressive --out capture.ppm --meta capture.json

``dd-commands`` prints the read commands for a region without contacting
anything.
"""

from __future__ import annotations

import argparse
import base64
import binascii
import json
import struct
import sys
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple

MAGIC = 0x53534D43  # "SSMC"
SUPPORTED_ABI = 1

HEADER_BYTES = 64
DESC_BYTES = 32
RECORD_BYTES = 64
DEFAULT_BASE = 0x3100_0000

DESC_OFFSET = 0x40
RECORD_OFFSET = 0x200

STATE_INVALID = 0
STATE_SEALED = 2

# Capture record status bits, from the ABI document.
ST_USABLE = 1 << 0
ST_PREV0 = 1 << 1
ST_PREV1 = 1 << 2
ST_HISTORY_INCOMPLETE = 1 << 3
ST_LOST = 1 << 4
ST_CUT_ON_BOUNDARY = 1 << 5

WIN_FLAG_LOST = 1 << 0

# Applied configuration bits, per amstrad_video_output.sv:80 and docs/ssm-capture-abi.md
CONFIG_NATIVE_CADENCE = 1 << 0  # Bit 0: obs_native_cadence
CONFIG_RAW_CRT        = 1 << 1  # Bit 1: raw_crt
CONFIG_PIXEL_RATE_SEL = 1 << 2  # Bit 2: pixel_rate_select
CONFIG_HQ2X           = 1 << 3  # Bit 3: hq2x
CONFIG_MIX_MASK       = 0x70    # Bits 6:4: mix[2:0]
CONFIG_PLUS_MODE      = 1 << 7  # Bit 7: plus_mode
CONFIG_CADENCE_MASK   = (CONFIG_NATIVE_CADENCE | CONFIG_RAW_CRT |
                         CONFIG_PIXEL_RATE_SEL | CONFIG_HQ2X)


class PayloadList(list):
    """List of (logical, sample_word) tuples, preserving descriptor loss/flags."""
    def __init__(self, iterable=(), descriptor: Optional[Any] = None):
        super().__init__(iterable)
        self.descriptor = descriptor


class LiveCapture(tuple):
    """3-tuple (header, record, payloads) with .descriptors dictionary."""
    def __new__(cls, header: Any, record: Any,
                payloads: Dict[int, List[Tuple[int, int]]],
                descriptors: Optional[Dict[int, Any]] = None):
        obj = super().__new__(cls, (header, record, payloads))
        obj.header = header
        obj.record = record
        obj.payloads = payloads
        obj.descriptors = descriptors or {}
        return obj


class SsmCaptureError(Exception):
    """The capture region could not be read or understood."""


class SsmCaptureTransportError(SsmCaptureError):
    """The device read itself failed; nothing can be said about the region."""


class SsmCaptureTruncatedError(SsmCaptureError):
    """Fewer bytes came back than the layout needs."""


class SsmCaptureFormatError(SsmCaptureError):
    """The bytes are the right length but are not a region we support."""


class SsmCaptureNotInitializedError(SsmCaptureFormatError):
    """No recorder header is present yet at this address."""


def _u64(data: bytes, offset: int, what: str) -> int:
    if len(data) < offset + 8:
        raise SsmCaptureTruncatedError(f"{what}: need {offset + 8} bytes, have {len(data)}")
    return struct.unpack_from("<Q", data, offset)[0]


class CaptureHeader:
    __slots__ = ("magic", "abi_version", "flags", "windows", "window_samples",
                 "capture_slots", "pin_prev", "hold_ticks", "epoch",
                 "payload_offset", "descriptor_offset", "record_offset",
                 "captures_published", "image_loss_count", "forced_expiry_count",
                 "capture_dropped")

    def __init__(self, data: bytes):
        w0 = _u64(data, 0x00, "header word 0")
        w1 = _u64(data, 0x08, "header word 1")
        w2 = _u64(data, 0x10, "header word 2")
        w3 = _u64(data, 0x18, "header word 3")
        w4 = _u64(data, 0x20, "header word 4")
        w5 = _u64(data, 0x28, "header word 5")
        w6 = _u64(data, 0x30, "header word 6")
        w7 = _u64(data, 0x38, "header word 7")
        self.magic = w0 & 0xFFFFFFFF
        self.abi_version = (w0 >> 32) & 0xFFFF
        self.flags = (w0 >> 48) & 0xFFFF
        self.windows = w1 & 0xFFFFFFFF
        self.window_samples = (w1 >> 32) & 0xFFFFFFFF
        self.capture_slots = w2 & 0xFFFFFFFF
        self.pin_prev = (w2 >> 32) & 0xFFFFFFFF
        self.hold_ticks = w3 & 0xFFFFFFFF
        self.epoch = (w3 >> 32) & 0xFFFFFFFF
        self.payload_offset = w4
        self.descriptor_offset = w5 & 0xFFFFFFFF
        self.record_offset = (w5 >> 32) & 0xFFFFFFFF
        self.captures_published = w6 & 0xFFFFFFFF
        self.image_loss_count = (w6 >> 32) & 0xFFFFFFFF
        self.forced_expiry_count = w7 & 0xFFFFFFFF
        self.capture_dropped = (w7 >> 32) & 0xFFFFFFFF

    @property
    def ready(self) -> bool:
        return bool(self.flags & 1)

    def validate(self) -> None:
        if self.magic != MAGIC:
            raise SsmCaptureNotInitializedError(
                f"capture magic is 0x{self.magic:08X}, expected 0x{MAGIC:08X}. The "
                "recorder publishes it when it is enabled and initialised, so this "
                "means the recorder is not compiled in, not enabled, or the base "
                "address is wrong."
            )
        if self.abi_version != SUPPORTED_ABI:
            raise SsmCaptureFormatError(
                f"capture ABI version {self.abi_version}, this reader implements "
                f"{SUPPORTED_ABI}"
            )
        if self.pin_prev > 2:
            raise SsmCaptureFormatError(
                f"capture ABI v1 represents at most two predecessors, got {self.pin_prev}")
        if not self.ready:
            raise SsmCaptureNotInitializedError(
                "the recorder header is present but not marked ready"
            )
        if not 1 <= self.windows <= 1024:
            raise SsmCaptureFormatError(f"implausible window count {self.windows}")
        if self.window_samples == 0 or self.window_samples % 2:
            raise SsmCaptureFormatError(
                f"window holds {self.window_samples} samples; two samples share a "
                "64-bit write, so the count must be even and non-zero"
            )
        if not 1 <= self.capture_slots <= 1024:
            raise SsmCaptureFormatError(f"implausible capture slot count {self.capture_slots}")
        if self.descriptor_offset < HEADER_BYTES:
            raise SsmCaptureFormatError(f"descriptor offset 0x{self.descriptor_offset:X} overlaps header")
        desc_end = self.descriptor_offset + self.windows * DESC_BYTES
        if self.record_offset < desc_end:
            raise SsmCaptureFormatError(
                f"record offset 0x{self.record_offset:X} overlaps descriptors (ends at 0x{desc_end:X})"
            )
        rec_end = self.record_offset + self.capture_slots * RECORD_BYTES
        if self.payload_offset < rec_end:
            raise SsmCaptureFormatError(
                f"payload offset 0x{self.payload_offset:X} overlaps records (ends at 0x{rec_end:X})"
            )

    def as_dict(self) -> Dict[str, Any]:
        return {
            "magic": f"{self.magic:08X}",
            "abi_version": self.abi_version,
            "ready": self.ready,
            "windows": self.windows,
            "window_samples": self.window_samples,
            "capture_slots": self.capture_slots,
            "pin_prev": self.pin_prev,
            "hold_ticks": self.hold_ticks,
            "epoch": self.epoch,
            "payload_offset": self.payload_offset,
            "captures_published": self.captures_published,
            "image_loss_count": self.image_loss_count,
            "forced_expiry_count": self.forced_expiry_count,
            "capture_dropped": self.capture_dropped,
        }


class WindowDescriptor:
    __slots__ = ("index", "generation", "state", "first_logical_sample",
                 "sample_count", "flags", "epoch")

    def __init__(self, data: bytes, index: int, offset: int = 0):
        w0 = _u64(data, offset + 0x00, f"window {index} word 0")
        self.index = index
        self.generation = w0 & 0xFFFFFFFF
        self.state = (w0 >> 32) & 0xFFFFFFFF
        self.first_logical_sample = _u64(data, offset + 0x08, f"window {index} word 1")
        self.sample_count = _u64(data, offset + 0x10, f"window {index} word 2")
        w3 = _u64(data, offset + 0x18, f"window {index} word 3")
        self.flags = w3 & 0xFFFFFFFF
        self.epoch = (w3 >> 32) & 0xFFFFFFFF

    @property
    def sealed(self) -> bool:
        return self.state == STATE_SEALED

    @property
    def lost(self) -> bool:
        return bool(self.flags & WIN_FLAG_LOST)

    def as_dict(self) -> Dict[str, Any]:
        return {
            "index": self.index, "generation": self.generation, "state": self.state,
            "sealed": self.sealed, "lost": self.lost,
            "first_logical_sample": self.first_logical_sample,
            "sample_count": self.sample_count, "epoch": self.epoch,
        }


class CaptureRecord:
    __slots__ = ("slot", "code", "frame", "line", "hpos", "field", "seq", "tick",
                 "cut", "windows", "status", "format", "applied_config", "epoch",
                 "capture_index", "expiry_tick")

    def __init__(self, data: bytes, slot: int, offset: int = 0):
        word_a = _u64(data, offset + 0x00, f"record {slot} word 0")
        word_b = _u64(data, offset + 0x08, f"record {slot} word 1")
        self.slot = slot
        # Byte-identical to the format-1 event record pair.
        self.code = word_a & 0xFFFF
        self.frame = (word_a >> 16) & 0xFFFFFF
        self.line = (word_a >> 40) & 0x3FF
        self.hpos = (word_a >> 50) & 0xFF
        self.field = (word_a >> 58) & 1
        self.seq = word_b & 0xFFFF
        self.tick = (word_b >> 16) & 0xFFFFFFFF
        self.cut = _u64(data, offset + 0x10, f"record {slot} word 2")
        self.windows = []
        for k in range(3):
            w = _u64(data, offset + 0x18 + 8 * k, f"record {slot} window ref {k}")
            self.windows.append((w & 0xFFFFFFFF, (w >> 32) & 0xFFFFFFFF))
        w6 = _u64(data, offset + 0x30, f"record {slot} word 6")
        self.status = w6 & 0xFFFF
        self.format = (w6 >> 16) & 0xFF
        self.applied_config = (w6 >> 24) & 0xFF
        self.epoch = (w6 >> 32) & 0xFFFFFFFF
        w7 = _u64(data, offset + 0x38, f"record {slot} word 7")
        self.capture_index = w7 & 0xFFFFFFFF
        self.expiry_tick = (w7 >> 32) & 0xFFFFFFFF

    @property
    def loss_flagged(self) -> bool:
        return bool(self.status & ST_LOST)

    @property
    def history_incomplete(self) -> bool:
        return bool(self.status & ST_HISTORY_INCOMPLETE)

    @property
    def cut_on_window_boundary(self) -> bool:
        return bool(self.status & ST_CUT_ON_BOUNDARY)

    def referenced_windows(self) -> List[Tuple[int, int]]:
        """(index, generation) for the cut's window and the retained prehistory.

        Order is oldest first, which is the order their samples must be applied.
        """
        out: List[Tuple[int, int]] = []
        if self.status & ST_PREV1:
            out.append(self.windows[2])
        if self.status & ST_PREV0:
            out.append(self.windows[1])
        if self.status & ST_USABLE:
            out.append(self.windows[0])
        return out

    def as_dict(self) -> Dict[str, Any]:
        return {
            "slot": self.slot,
            "code": f"{self.code:04X}",
            "frame": self.frame, "line": self.line, "hpos": self.hpos,
            "field": self.field, "seq": self.seq, "tick": self.tick,
            "cut": self.cut,
            "current_window": {"index": self.windows[0][0], "generation": self.windows[0][1]},
            "previous_windows": [
                {"index": self.windows[1][0], "generation": self.windows[1][1],
                 "present": bool(self.status & ST_PREV0)},
                {"index": self.windows[2][0], "generation": self.windows[2][1],
                 "present": bool(self.status & ST_PREV1)},
            ],
            "status": self.status,
            "format": self.format,
            "applied_config": self.applied_config,
            "history_incomplete": bool(self.status & ST_HISTORY_INCOMPLETE),
            "loss_flagged": bool(self.status & ST_LOST),
            "cut_on_window_boundary": bool(self.status & ST_CUT_ON_BOUNDARY),
            "epoch": self.epoch,
            "capture_index": self.capture_index,
            "expiry_tick": self.expiry_tick,
        }


# ---------------------------------------------------------------------------
# Raster profiles
# ---------------------------------------------------------------------------


class RasterProfile:
    """A named, explicitly declared raster the decoder is willing to reconstruct.

    These numbers are assumptions written down so they can be argued with, not
    measurements from a device. `measured` stays False until a profile is
    derived from a real capture, and it travels into the decode metadata so no
    output can be mistaken for a validated one.
    """

    def __init__(self, name: str, line_dots: int, lines_per_field: int,
                 active_dots: int, active_lines: int, interlaced: bool,
                 fields_required: int, line_tolerance: int = 0,
                 phase_tolerance: int = 0, measured: bool = False):
        self.name = name
        self.line_dots = line_dots
        self.lines_per_field = lines_per_field
        self.active_dots = active_dots
        self.active_lines = active_lines
        self.interlaced = interlaced
        self.fields_required = fields_required
        self.line_tolerance = line_tolerance
        self.phase_tolerance = phase_tolerance
        self.measured = measured

    def as_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name, "line_dots": self.line_dots,
            "lines_per_field": self.lines_per_field,
            "active_dots": self.active_dots, "active_lines": self.active_lines,
            "interlaced": self.interlaced, "fields_required": self.fields_required,
            "line_tolerance": self.line_tolerance,
            "phase_tolerance": self.phase_tolerance,
            "measured_on_hardware": self.measured,
        }


PROFILES: Dict[str, RasterProfile] = {
    # 64 us lines at 16 MHz, 312 lines: the ordinary CPC raster. The active
    # rectangle is the core's usual 768x272 acquisition window (the same one
    # sim/b6_video_output_test.cpp counts), declared here rather than measured.
    "cpc-native-progressive": RasterProfile(
        "cpc-native-progressive", line_dots=1024, lines_per_field=312,
        active_dots=768, active_lines=272, interlaced=False, fields_required=1,
        line_tolerance=2, phase_tolerance=8),
    # The same raster with alternating half-line VSYNC phase. SHAKER module B
    # test 1 needs this one; it is listed separately because a decoder that
    # guesses interlace from the data is a decoder that can hide a
    # displacement.
    "cpc-native-interlace": RasterProfile(
        "cpc-native-interlace", line_dots=1024, lines_per_field=312,
        active_dots=768, active_lines=544, interlaced=True, fields_required=2,
        line_tolerance=2, phase_tolerance=8),
    # Fixture-sized rasters, so the decoder's rules can be tested without
    # building a megabyte of samples.
    "test-tiny-progressive": RasterProfile(
        "test-tiny-progressive", line_dots=8, lines_per_field=6,
        active_dots=4, active_lines=4, interlaced=False, fields_required=1),
    "test-tiny-interlace": RasterProfile(
        "test-tiny-interlace", line_dots=8, lines_per_field=6,
        active_dots=4, active_lines=8, interlaced=True, fields_required=2,
        phase_tolerance=0),
}


def sample_fields(word: int) -> Dict[str, int]:
    """Unpack one 32-bit sample, per the ABI's sample word table."""
    return {
        "r": word & 0xFF,
        "g": (word >> 8) & 0xFF,
        "b": (word >> 16) & 0xFF,
        "hbl": (word >> 24) & 1,
        "vbl": (word >> 25) & 1,
        "hs": (word >> 26) & 1,
        "vs": (word >> 27) & 1,
        "field": (word >> 28) & 1,
    }


class DecodeResult:
    """What a decode attempt produced, including when it produced no image.

    `status` is one of:

    ``complete``   every rule the profile asks for held, and the active
                   rectangle is fully populated.
    ``incomplete`` the samples are real but the profile's requirements are not
                   met: gaps, loss, missing sync, unsupported phase or too
                   little history. The partial surface and the raw trace are
                   both kept.
    ``pending``    the capture's own window has not been sealed yet, so its
                   payload is not transferable. Read again later.
    ``lost``       a referenced window's generation has changed, so the samples
                   the capture pinned are gone.
    """

    def __init__(self, status: str, profile: RasterProfile, record: CaptureRecord):
        self.status = status
        self.profile = profile
        self.record = record
        self.reasons: List[str] = []
        self.surface: Dict[Tuple[int, int], Tuple[int, int, int]] = {}
        self.raw: List[Tuple[int, int]] = []
        self.metadata: Dict[str, Any] = {}

    @property
    def complete(self) -> bool:
        return self.status == "complete"

    @property
    def bounds(self) -> Optional[Dict[str, int]]:
        if not self.surface:
            return None
        lines = sorted({line for line, _ in self.surface})
        dots = sorted({dot for _, dot in self.surface})
        return {
            "min_line": lines[0], "max_line": lines[-1],
            "min_dot": dots[0], "max_dot": dots[-1],
            "width": dots[-1] - dots[0] + 1,
            "height": lines[-1] - lines[0] + 1,
        }

    @property
    def crop_origin(self) -> Optional[Tuple[int, int]]:
        if not self.surface:
            return None
        lines = sorted({line for line, _ in self.surface})
        dots = sorted({dot for _, dot in self.surface})
        return (lines[0], dots[0])

    def as_dict(self) -> Dict[str, Any]:
        return {
            "status": self.status,
            "complete": self.complete,
            "reasons": self.reasons,
            "profile": self.profile.as_dict(),
            "capture": self.record.as_dict(),
            "bounds": self.bounds,
            "crop_origin": self.crop_origin,
            "metadata": self.metadata,
            "raw_sample_count": len(self.raw),
        }

    def to_ppm(self) -> bytes:
        """Binary PPM of the populated rectangle; unknown cells are magenta.

        PPM keeps this dependency-free. The magenta fill is deliberately not a
        plausible picture colour: an incomplete result must look incomplete.
        """
        if not self.surface:
            raise SsmCaptureError("nothing was decoded, so there is no image to write")
        lines = sorted({line for line, _ in self.surface})
        dots = sorted({dot for _, dot in self.surface})
        width = dots[-1] - dots[0] + 1
        height = lines[-1] - lines[0] + 1
        body = bytearray()
        for line in range(lines[0], lines[-1] + 1):
            for dot in range(dots[0], dots[-1] + 1):
                pixel = self.surface.get((line, dot))
                body.extend(bytes(pixel) if pixel else b"\xff\x00\xff")
        return b"P6\n%d %d\n255\n" % (width, height) + bytes(body)


def decode_samples(samples: Sequence[Tuple[int, int]], cut: int,
                   profile: RasterProfile, record: CaptureRecord,
                   history_lost: bool = False) -> DecodeResult:
    """Apply ordered samples through the cut onto invalid-initialised history.

    `samples` is (logical index, 32-bit word), ascending; indices at or after
    `cut` are not applied, because the cut is exclusive. Everything the decoder
    cannot justify is reported rather than smoothed: a line whose length does
    not match the profile, a VSYNC at an unsupported phase, a gap in the index
    sequence and a window that reported lost samples all keep the result out of
    ``complete``.
    """
    result = DecodeResult("incomplete", profile, record)
    result.raw = list(samples)

    applied = [(index, word) for index, word in samples if index < cut]
    result.metadata["samples_available"] = len(samples)
    result.metadata["samples_applied"] = len(applied)
    result.metadata["cut"] = cut

    if not applied:
        result.reasons.append("no samples before the cut")
        return result

    if applied[-1][0] != cut - 1:
        result.reasons.append(
            f"retained samples end at logical index {applied[-1][0]}, expected cut - 1 ({cut - 1})"
        )

    gaps = 0
    for (a, _), (b, _) in zip(applied, applied[1:]):
        if b != a + 1:
            gaps += 1
    result.metadata["index_gaps"] = gaps
    if gaps:
        result.reasons.append(f"{gaps} gap(s) in the retained sample indices")
    if history_lost:
        result.reasons.append("a retained window reported lost samples")

    prev_hs = prev_vs = 0
    dot = 0
    line = 0
    line_lengths: List[int] = []
    field_phases: List[int] = []
    core_fields: List[int] = []
    field_lengths: List[int] = []
    field_parity: Optional[int] = None
    fields_seen = 0
    unsupported_phase = False
    started = False           # only place pixels once a VSYNC has been seen

    for index, word in applied:
        s = sample_fields(word)

        if s["hs"] and not prev_hs:
            line_lengths.append(dot)
            dot = 0
            line += 1
        if s["vs"] and not prev_vs:
            # The phase of the VSYNC rise inside its line is what tells the two
            # physical fields apart. The core's own FIELD signal is kept beside
            # it for checking, never used in its place.
            field_phases.append(dot)
            core_fields.append(s["field"])
            if started:
                field_lengths.append(line)
            fields_seen += 1
            line = 0
            if profile.interlaced:
                half = profile.line_dots // 2
                if dot <= profile.phase_tolerance:
                    field_parity = 0
                elif abs(dot - half) <= profile.phase_tolerance:
                    field_parity = 1
                else:
                    unsupported_phase = True
                    field_parity = None
            else:
                field_parity = 0
                if dot > profile.phase_tolerance:
                    unsupported_phase = True
            started = True

        prev_hs, prev_vs = s["hs"], s["vs"]

        if started and not s["hbl"] and not s["vbl"]:
            if profile.interlaced:
                if field_parity is None:
                    dot += 1
                    continue
                physical = 2 * line + field_parity
            else:
                physical = line
            result.surface[(physical, dot)] = (s["r"], s["g"], s["b"])

        dot += 1

    if started:
        field_lengths.append(line + 1)

    # Line lengths are measured between HSYNC rises; the first entry is the
    # partial line the trace started in and is not a measurement.
    measured_lines = line_lengths[1:]
    bad_lines = [n for n in measured_lines
                 if abs(n - profile.line_dots) > profile.line_tolerance]
    result.metadata["lines_measured"] = len(measured_lines)
    result.metadata["lines_off_profile"] = len(bad_lines)
    result.metadata["fields_seen"] = fields_seen
    result.metadata["field_lengths"] = field_lengths
    result.metadata["vsync_phases"] = field_phases
    result.metadata["core_field_bits"] = core_fields
    result.metadata["active_cells"] = len(result.surface)

    if not fields_seen:
        result.reasons.append("no VSYNC in the retained history")
    if fields_seen < profile.fields_required:
        result.reasons.append(
            f"{fields_seen} field(s) before the cut, profile needs "
            f"{profile.fields_required}")
    if bad_lines:
        result.reasons.append(
            f"{len(bad_lines)} line(s) are not {profile.line_dots} dots long; the "
            "displacement is preserved in the raw trace and not corrected here")
    if unsupported_phase:
        result.reasons.append(
            "a VSYNC arrived at a phase this profile does not describe")

    bad_fields = [fl for fl in field_lengths if abs(fl - profile.lines_per_field) > 0]
    if bad_fields:
        result.reasons.append(
            f"field length {bad_fields[0]} lines does not match profile {profile.lines_per_field}"
        )

    # Core FIELD is evidence, not the placement rule. Disagreement is worth
    # reporting because it points at one of the two being wrong.
    if profile.interlaced and len(field_phases) == len(core_fields):
        half = profile.line_dots // 2
        derived = [1 if abs(p - half) <= profile.phase_tolerance else 0
                   for p in field_phases]
        result.metadata["derived_field_parity"] = derived
        disagree = sum(1 for d, c in zip(derived, core_fields) if d != c)
        result.metadata["field_disagreements"] = disagree
        if disagree:
            result.reasons.append(
                f"the measured HS/VS phase and the core FIELD signal disagree on "
                f"{disagree} field(s)")

    needed = profile.active_lines * profile.active_dots
    result.metadata["active_cells_needed"] = needed
    if len(result.surface) < needed:
        result.reasons.append(
            f"{len(result.surface)} of {needed} active cells were written; no finite "
            "prehistory can supply an address that was never refreshed")

    # Physical active row and column geometry validation
    lines_present = sorted({line_idx for line_idx, _ in result.surface})
    if len(lines_present) != profile.active_lines:
        result.reasons.append(
            f"{len(lines_present)} active line(s) populated, profile needs {profile.active_lines}"
        )
    elif lines_present:
        if lines_present[-1] - lines_present[0] + 1 != len(lines_present):
            result.reasons.append(f"active lines are not contiguous: {lines_present}")
        ref_dots = None
        for l in lines_present:
            row_dots = sorted([d for line_idx, d in result.surface if line_idx == l])
            if len(row_dots) != profile.active_dots:
                result.reasons.append(
                    f"line {l} has {len(row_dots)} active dots, expected {profile.active_dots}"
                )
                break
            if row_dots[-1] - row_dots[0] + 1 != len(row_dots):
                result.reasons.append(f"line {l} active dots are not contiguous")
                break
            if ref_dots is None:
                ref_dots = row_dots
            elif row_dots != ref_dots:
                result.reasons.append(
                    f"line {l} dot range {row_dots} differs from first line {ref_dots}"
                )
                break

    if lines_present and result.surface:
        result.metadata["crop_origin"] = {
            "line": lines_present[0],
            "dot": min(d for l, d in result.surface if l == lines_present[0]),
        }

    if record is not None:
        if record.format != 1:
            result.reasons.append(f"record format {record.format} is unsupported (expected 1)")
        if (record.applied_config & CONFIG_CADENCE_MASK) != CONFIG_NATIVE_CADENCE:
            result.reasons.append(
                f"applied config 0x{record.applied_config:02X} does not select supported native cadence (Raw, alternate rate and HQ2x must be off) for profile '{profile.name}'"
            )
        if not (record.status & ST_USABLE):
            result.reasons.append("current window is not marked usable (ST_USABLE)")

    if not result.reasons:
        result.status = "complete"
    else:
        result.status = "incomplete"
    return result


def decode_capture_from_region(region: bytes, capture_index: int,
                               profile: RasterProfile) -> DecodeResult:
    """Decode one capture out of a dump of the whole region from CAP_BASE."""
    header = CaptureHeader(region[:HEADER_BYTES])
    header.validate()
    if capture_index >= header.captures_published:
        raise SsmCaptureError(
            f"capture {capture_index} is not published yet; the header counts "
            f"{header.captures_published}"
        )
    slot = capture_index % header.capture_slots
    rec_off = header.record_offset + slot * RECORD_BYTES
    record = CaptureRecord(region, slot, offset=rec_off)
    if record.capture_index != capture_index:
        raise SsmCaptureError(
            f"slot {slot} holds capture {record.capture_index}, not {capture_index}; "
            "it has already been reused"
        )

    descriptors = {}
    for index in range(header.windows):
        off = header.descriptor_offset + index * DESC_BYTES
        descriptors[index] = WindowDescriptor(region, index, offset=off)

    result_status = None
    reasons: List[str] = []
    history_lost = False
    samples: List[Tuple[int, int]] = []

    if record.epoch != header.epoch:
        result_status = "lost"
        reasons.append(f"record epoch {record.epoch} does not match header epoch {header.epoch}")

    if not (record.status & ST_USABLE):
        result_status = "lost"
        reasons.append("current window is not marked usable (ST_USABLE)")

    if header.pin_prev >= 1 and not (record.status & ST_PREV0):
        if result_status != "lost":
            result_status = "incomplete"
        reasons.append(f"header requires pin_prev={header.pin_prev} >= 1, but predecessor window 0 is missing")

    if header.pin_prev >= 2 and not (record.status & ST_PREV1):
        if result_status != "lost":
            result_status = "incomplete"
        reasons.append(f"header requires pin_prev={header.pin_prev} >= 2, but predecessor window 1 is missing")

    if record.format != 1:
        result_status = "lost"
        reasons.append(f"record format {record.format} is unsupported (expected 1)")

    if (record.applied_config & CONFIG_CADENCE_MASK) != CONFIG_NATIVE_CADENCE:
        if result_status != "lost":
            result_status = "incomplete"
        reasons.append(
            f"applied config 0x{record.applied_config:02X} does not select supported native cadence (Raw, alternate rate and HQ2x must be off) for profile '{profile.name}'"
        )

    if record.loss_flagged:
        result_status = "lost"
        reasons.append("record indicates known sample loss (ST_LOST)")

    if record.history_incomplete:
        if result_status != "lost":
            result_status = "incomplete"
        reasons.append("record indicates incomplete history (ST_HISTORY_INCOMPLETE)")

    refs = record.referenced_windows()
    if not refs:
        result = DecodeResult("lost", profile, record)
        result.reasons.append("the capture references no usable window")
        return result

    for index, generation in refs:
        desc = descriptors.get(index)
        if desc is None:
            result_status = "lost"
            reasons.append(f"window {index} is outside the pool")
            continue
        if desc.epoch != header.epoch:
            result_status = "lost"
            reasons.append(
                f"window {index} epoch {desc.epoch} does not match header epoch {header.epoch}"
            )
            continue
        if desc.generation != generation:
            result_status = "lost"
            reasons.append(
                f"window {index} now carries generation {desc.generation}, the capture "
                f"pinned {generation}: those samples have been reused")
            continue
        if not desc.sealed:
            if result_status not in ("lost", "incomplete"):
                result_status = "pending"
            reasons.append(f"window {index} is not sealed yet")
            continue
        history_lost = history_lost or desc.lost
        samples.extend(window_samples(region, header, desc))

    if result_status in ("lost", "pending"):
        result = DecodeResult(result_status, profile, record)
        result.reasons = reasons
        result.raw = sorted(samples)
        result.metadata["windows"] = [descriptors[i].as_dict() for i, _ in refs
                                      if i in descriptors]
        result.metadata["header"] = header.as_dict()
        return result

    samples.sort()
    result = decode_samples(samples, record.cut, profile, record,
                            history_lost=history_lost)
    result.reasons = reasons + result.reasons
    result.metadata["windows"] = [descriptors[i].as_dict() for i, _ in refs
                                  if i in descriptors]
    result.metadata["header"] = header.as_dict()
    if header.image_loss_count and "a retained window reported lost samples" not in result.reasons:
        result.metadata["recorder_image_loss_total"] = header.image_loss_count
    if result_status == "incomplete" and result.status == "complete":
        result.status = "incomplete"
    if result.reasons and result.status == "complete":
        result.status = "incomplete"
    return result


def window_samples(region: bytes, header: CaptureHeader,
                   desc: WindowDescriptor) -> List[Tuple[int, int]]:
    """(logical index, sample word) for every sample a sealed window holds."""
    base = header.payload_offset + desc.index * header.window_samples * 4
    count = min(desc.sample_count, header.window_samples)
    end = base + count * 4
    if len(region) < end:
        raise SsmCaptureTruncatedError(
            f"window {desc.index} payload needs {end} bytes, the region dump has "
            f"{len(region)}"
        )
    out: List[Tuple[int, int]] = []
    for k in range(count):
        word = struct.unpack_from("<I", region, base + k * 4)[0]
        out.append((desc.first_logical_sample + k, word))
    return out


# ---------------------------------------------------------------------------
# Device reads
# ---------------------------------------------------------------------------


def read_command(base: int, offset: int, length: int) -> str:
    """Read-only `dd` for one aligned run of the region, as one base64 line.

    BusyBox has no `iflag=skip_bytes`, so the skip has to be in whole blocks.
    ABI descriptors have 32-byte stride, so block=32 allows reading every
    ABI structure (header, descriptors, records, and window payloads).
    """
    block = 32
    start = base + offset
    if start % block:
        raise SsmCaptureError(f"read at 0x{start:08X} is not {block}-byte aligned")
    count = (length + block - 1) // block
    return (
        f"dd if=/dev/mem bs={block} skip={start // block} count={count} 2>/dev/null "
        "| base64 | tr -d '\\n'"
    )


def decode_payload(stdout: str) -> bytes:
    text = stdout.strip()
    if not text:
        raise SsmCaptureTransportError(
            "the device returned no bytes; dd produced nothing, which is a transport "
            "or permission failure and not an empty region"
        )
    try:
        return base64.b64decode(text, validate=True)
    except (binascii.Error, ValueError) as exc:
        raise SsmCaptureTransportError(f"malformed base64 from the device: {exc}") from exc


class CaptureReader:
    """Reads captures over an SSHTransport-shaped object.

    Payload reads are bracketed by descriptor reads, and only a window whose
    generation, epoch *and* sealed state are unchanged across the pair is accepted.
    """

    def __init__(self, transport: Any, base: int = DEFAULT_BASE, timeout: float = 30.0):
        self.transport = transport
        self.base = base
        self.timeout = timeout

    def _read(self, offset: int, length: int) -> bytes:
        command = read_command(self.base, offset, length)
        result = self.transport.run_cmd(command, timeout=self.timeout)
        if getattr(result, "exit_code", 1) != 0:
            raise SsmCaptureTransportError(
                f"reading /dev/mem failed ({result.exit_code}): {result.stderr.strip()}"
            )
        data = decode_payload(result.stdout)
        if len(data) < length:
            raise SsmCaptureTruncatedError(
                f"read at +0x{offset:X} returned {len(data)} bytes, expected {length}"
            )
        return data[:length]

    def header(self) -> CaptureHeader:
        header = CaptureHeader(self._read(0, HEADER_BYTES))
        header.validate()
        return header

    def descriptor(self, header: CaptureHeader, index: int) -> WindowDescriptor:
        off = header.descriptor_offset + index * DESC_BYTES
        return WindowDescriptor(self._read(off, DESC_BYTES), index)

    def record(self, header: CaptureHeader, capture_index: int) -> CaptureRecord:
        if capture_index >= header.captures_published:
            raise SsmCaptureError(
                f"capture {capture_index} is not published yet; the header counts "
                f"{header.captures_published}"
            )
        slot = capture_index % header.capture_slots
        off = header.record_offset + slot * RECORD_BYTES
        rec = CaptureRecord(self._read(off, RECORD_BYTES), slot)
        if rec.epoch != header.epoch:
            raise SsmCaptureError(
                f"record epoch {rec.epoch} does not match header epoch {header.epoch}"
            )
        if rec.capture_index != capture_index:
            raise SsmCaptureError(
                f"slot {slot} holds capture {rec.capture_index}, not {capture_index}; "
                "it has already been reused"
            )
        return rec

    def record_commit_identity(self, header: CaptureHeader, slot: int) -> Tuple[int, int, int, int, int]:
        """Read upper 32 bytes of record slot (words 4-7) for commit identity validation.

        Returns (capture_index, epoch, format, applied_config, expiry_tick).
        """
        off = header.record_offset + slot * RECORD_BYTES + 32
        data = self._read(off, 32)
        w6 = _u64(data, 0x10, f"record slot {slot} word 6")
        format = (w6 >> 16) & 0xFF
        applied_config = (w6 >> 24) & 0xFF
        epoch = (w6 >> 32) & 0xFFFFFFFF
        w7 = _u64(data, 0x18, f"record slot {slot} word 7")
        capture_index = w7 & 0xFFFFFFFF
        expiry_tick = (w7 >> 32) & 0xFFFFFFFF
        if capture_index == 0xFFFFFFFF:
            raise SsmCaptureError(f"slot {slot} word 7 is invalid (0xFFFFFFFF); publication in progress")
        return (capture_index, epoch, format, applied_config, expiry_tick)

    def window_payload(self, header: CaptureHeader, index: int,
                       generation: int) -> PayloadList:
        """Read one window's payload, verifying sealed state, generation, and epoch.

        Reading the descriptor, the payload and the descriptor again is what
        rules out a reuse that started before the first read as well as one
        that started during it: only an unchanged generation, matching epoch
        *and* a sealed state on both sides can be trusted.
        """
        before = self.descriptor(header, index)
        if before.epoch != header.epoch:
            raise SsmCaptureError(
                f"window {index} carries epoch {before.epoch}, expected {header.epoch}"
            )
        if before.generation != generation:
            raise SsmCaptureError(
                f"window {index} carries generation {before.generation}, expected "
                f"{generation}: the capture's samples have been reused"
            )
        if not before.sealed:
            raise SsmCaptureError(f"window {index} is not sealed yet")
        base = header.payload_offset + index * header.window_samples * 4
        count = min(before.sample_count, header.window_samples)
        data = self._read(base, count * 4)
        after = self.descriptor(header, index)
        if after.epoch != header.epoch:
            raise SsmCaptureError(
                f"window {index} epoch changed while reading: "
                f"{before.epoch} -> {after.epoch}"
            )
        if (after.generation, after.state) != (before.generation, before.state):
            raise SsmCaptureError(
                f"window {index} was reused while it was being read "
                f"(generation {before.generation} state {before.state} became "
                f"{after.generation}/{after.state})"
            )
        samples = [(before.first_logical_sample + k,
                    struct.unpack_from("<I", data, k * 4)[0]) for k in range(count)]
        return PayloadList(samples, descriptor=after)

    def read_live_capture(self, capture_index: int) -> LiveCapture:
        """Atomically read a complete capture, bracketing header, record and payloads."""
        h_before = self.header()
        if capture_index >= h_before.captures_published:
            raise SsmCaptureError(
                f"capture {capture_index} is not published yet; the header counts "
                f"{h_before.captures_published}"
            )
        slot = capture_index % h_before.capture_slots
        ident_before = self.record_commit_identity(h_before, slot)
        if ident_before[0] != capture_index:
            raise SsmCaptureError(
                f"slot {slot} holds capture {ident_before[0]}, not {capture_index}; "
                "it has already been reused"
            )
        if ident_before[1] != h_before.epoch:
            raise SsmCaptureError(
                f"record epoch {ident_before[1]} does not match header epoch {h_before.epoch}"
            )
        rec = self.record(h_before, capture_index)
        payloads: Dict[int, List[Tuple[int, int]]] = {}
        descriptors: Dict[int, WindowDescriptor] = {}
        for win_idx, win_gen in rec.referenced_windows():
            payload = self.window_payload(h_before, win_idx, win_gen)
            payloads[win_idx] = payload
            if getattr(payload, "descriptor", None) is not None:
                descriptors[win_idx] = payload.descriptor
        ident_after = self.record_commit_identity(h_before, slot)
        if ident_after != ident_before:
            raise SsmCaptureError(
                f"slot {slot} record commit identity changed while reading: "
                f"{ident_before} -> {ident_after}"
            )
        h_after = self.header()
        if h_after.epoch != h_before.epoch or not h_after.ready:
            raise SsmCaptureError("recorder was reset or disabled while reading live capture")
        return LiveCapture(h_after, rec, payloads, descriptors)


def decode_live_capture(reader: CaptureReader, capture_index: int,
                        profile: RasterProfile,
                        out_dir: Optional[str] = None,
                        out_prefix: Optional[str] = None) -> DecodeResult:
    """Read a live capture atomically and decode it against the requested profile."""
    live = reader.read_live_capture(capture_index)
    header, record, payloads = live.header, live.record, live.payloads
    descriptors = live.descriptors

    result_status = None
    reasons: List[str] = []
    history_lost = False
    samples: List[Tuple[int, int]] = []

    if record.epoch != header.epoch:
        result_status = "lost"
        reasons.append(f"record epoch {record.epoch} does not match header epoch {header.epoch}")

    if not (record.status & ST_USABLE):
        result_status = "lost"
        reasons.append("current window is not marked usable (ST_USABLE)")

    if header.pin_prev >= 1 and not (record.status & ST_PREV0):
        if result_status != "lost":
            result_status = "incomplete"
        reasons.append(f"header requires pin_prev={header.pin_prev} >= 1, but predecessor window 0 is missing")

    if header.pin_prev >= 2 and not (record.status & ST_PREV1):
        if result_status != "lost":
            result_status = "incomplete"
        reasons.append(f"header requires pin_prev={header.pin_prev} >= 2, but predecessor window 1 is missing")

    if record.format != 1:
        result_status = "lost"
        reasons.append(f"record format {record.format} is unsupported (expected 1)")

    if (record.applied_config & CONFIG_CADENCE_MASK) != CONFIG_NATIVE_CADENCE:
        if result_status != "lost":
            result_status = "incomplete"
        reasons.append(
            f"applied config 0x{record.applied_config:02X} does not select supported native cadence (Raw, alternate rate and HQ2x must be off) for profile '{profile.name}'"
        )

    if record.loss_flagged:
        result_status = "lost"
        reasons.append("record indicates known sample loss (ST_LOST)")

    if record.history_incomplete:
        if result_status != "lost":
            result_status = "incomplete"
        reasons.append("record indicates incomplete history (ST_HISTORY_INCOMPLETE)")

    refs = record.referenced_windows()
    if not refs:
        result = DecodeResult("lost", profile, record)
        result.reasons.append("the capture references no usable window")
        return result

    for index, generation in refs:
        desc = descriptors.get(index)
        if desc is None:
            try:
                desc = reader.descriptor(header, index)
                descriptors[index] = desc
            except Exception:
                desc = None
        if desc is not None and desc.lost:
            history_lost = True
        win_samples = payloads.get(index, [])
        samples.extend(win_samples)

    samples.sort()
    result = decode_samples(samples, record.cut, profile, record,
                            history_lost=history_lost)
    result.reasons = reasons + result.reasons
    result.metadata["windows"] = [descriptors[i].as_dict() for i, _ in refs
                                  if i in descriptors]
    result.metadata["header"] = header.as_dict()
    if header.image_loss_count and "a retained window reported lost samples" not in result.reasons:
        result.metadata["recorder_image_loss_total"] = header.image_loss_count
    if result_status == "incomplete" and result.status == "complete":
        result.status = "incomplete"
    if result_status == "lost":
        result.status = "lost"
    if result.reasons and result.status == "complete":
        result.status = "incomplete"

    if out_dir is not None:
        import pathlib
        out_path = pathlib.Path(out_dir)
        out_path.mkdir(parents=True, exist_ok=True)
        prefix = out_prefix or f"capture_{capture_index:04d}"

        raw_file = out_path / f"{prefix}.raw"
        with open(raw_file, "wb") as f:
            for _, word in result.raw:
                f.write(struct.pack("<I", word))

        meta_file = out_path / f"{prefix}.json"
        with open(meta_file, "w", encoding="utf-8") as f:
            json.dump(result.as_dict(), f, indent=2)

        if result.surface:
            ppm_file = out_path / f"{prefix}.ppm"
            with open(ppm_file, "wb") as f:
                f.write(result.to_ppm())

    return result


# ---------------------------------------------------------------------------
# Offline entry point
# ---------------------------------------------------------------------------


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Read and decode the experimental SSM capture region.")
    sub = parser.add_subparsers(dest="command", required=True)

    decode = sub.add_parser("decode", help="Decode one capture from a region dump")
    decode.add_argument("--region", required=True,
                        help="File holding the region as dumped from CAP_BASE")
    decode.add_argument("--capture", type=int, default=0)
    decode.add_argument("--profile", default="cpc-native-progressive",
                        choices=sorted(PROFILES))
    decode.add_argument("--out", default=None, help="Write a binary PPM here")
    decode.add_argument("--meta", default=None, help="Write decode metadata JSON here")

    live = sub.add_parser("live", help="Retrieve and decode one capture from a live device")
    live.add_argument("--capture", type=int, default=0, help="Capture index to retrieve")
    live.add_argument("--profile", default="cpc-native-progressive",
                      choices=sorted(PROFILES))
    live.add_argument("--target", default=None, help="SSH target host (e.g. root@192.168.1.50)")
    live.add_argument("--port", type=int, default=22, help="SSH port")
    live.add_argument("--base", type=lambda v: int(v, 0), default=DEFAULT_BASE)
    live.add_argument("--out-dir", default=None, help="Directory to write artifacts (.raw, .json, .ppm)")
    live.add_argument("--out-prefix", default=None, help="Artifact filename prefix")

    dump = sub.add_parser("dd-commands",
                          help="Print the read commands for a region; contacts nothing")
    dump.add_argument("--base", type=lambda v: int(v, 0), default=DEFAULT_BASE)
    dump.add_argument("--windows", type=int, default=8)
    dump.add_argument("--window-samples", type=int, default=1 << 19)
    dump.add_argument("--payload-offset", type=lambda v: int(v, 0), default=0x0010_0000)

    args = parser.parse_args(argv)

    if args.command == "dd-commands":
        print(read_command(args.base, 0, HEADER_BYTES))
        print(read_command(args.base, DESC_OFFSET, args.windows * DESC_BYTES))
        print(read_command(args.base, RECORD_OFFSET, 16 * RECORD_BYTES))
        for index in range(args.windows):
            print(read_command(args.base,
                               args.payload_offset + index * args.window_samples * 4,
                               args.window_samples * 4))
        return 0

    if args.command == "live":
        try:
            from driver import SSHTransport
        except ImportError:
            SSHTransport = None
        if SSHTransport is None or not args.target:
            print("live retrieval requires --target and SSHTransport", file=sys.stderr)
            return 2
        transport = SSHTransport(args.target, port=args.port)
        reader = CaptureReader(transport, base=args.base)
        try:
            result = decode_live_capture(
                reader, args.capture, PROFILES[args.profile],
                out_dir=args.out_dir, out_prefix=args.out_prefix,
            )
        except SsmCaptureError as exc:
            print(f"live decode failed: {exc}", file=sys.stderr)
            return 2
        print(f"status={result.status} cells={len(result.surface)} "
              f"samples={len(result.raw)} reasons={len(result.reasons)}")
        for reason in result.reasons:
            print(f"  - {reason}")
        return 0 if result.complete else 1

    with open(args.region, "rb") as handle:
        region = handle.read()
    try:
        result = decode_capture_from_region(region, args.capture, PROFILES[args.profile])
    except SsmCaptureError as exc:
        print(f"decode failed: {exc}", file=sys.stderr)
        return 2

    meta = result.as_dict()
    if args.meta:
        with open(args.meta, "w", encoding="utf-8") as handle:
            json.dump(meta, handle, indent=2)
    if args.out and result.surface:
        with open(args.out, "wb") as handle:
            handle.write(result.to_ppm())
    print(f"status={result.status} cells={len(result.surface)} "
          f"samples={len(result.raw)} reasons={len(result.reasons)}")
    for reason in result.reasons:
        print(f"  - {reason}")
    return 0 if result.complete else 1


if __name__ == "__main__":
    raise SystemExit(main())
