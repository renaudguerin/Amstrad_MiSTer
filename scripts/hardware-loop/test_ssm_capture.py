"""Offline tests for the experimental SSM capture reader and decoder (B4 phase 2).

The regions here are built byte by byte from docs/ssm-capture-abi.md, not
captured from the simulator, so a field the RTL moves breaks these tests instead
of quietly agreeing with the move.

The cases worth their place are the ones where a decoder could plausibly cheat:
a window that has been reused underneath the reader, a cut that must exclude the
sample on its own edge, an address repainted twice inside one window, a field
whose physical placement comes from measured sync phase rather than from the
core's FIELD bit, and every shape of missing history. None of those may produce
a complete image.
"""

from __future__ import annotations

import struct
import unittest
from typing import Any, Dict, List, Optional, Sequence, Tuple

import ssm_capture
from ssm_capture import PROFILES, CaptureRecord, SsmCaptureError

WINDOW_SAMPLES = 128
WINDOWS = 4
CAPTURE_SLOTS = 4
PAYLOAD_OFFSET = 0x400

# Tiny raster, in generator coordinates: eight dots per line, HSync on dots 0-1,
# horizontal blanking over dots 0-3, so four active dots per line. Six lines per
# field: line 0 carries VSync, lines 0-1 are vertically blanked, lines 2-5 are
# active.
DOTS = 8
LINES = 6
HALF_LINE = DOTS // 2


def sample(r: int, g: int, b: int, hbl: int, vbl: int, hs: int, vs: int,
           field: int) -> int:
    return (r | (g << 8) | (b << 16) | (hbl << 24) | (vbl << 25)
            | (hs << 26) | (vs << 27) | (field << 28))


def field_words(colour: Tuple[int, int, int], odd: bool) -> List[int]:
    """One field of the tiny raster.

    `odd` puts the VSync rise half a line later, which is the only thing that
    tells the two physical fields apart. The core FIELD bit is set to match, so
    a test can also break the agreement deliberately.
    """
    out: List[int] = []
    vs_dot = HALF_LINE if odd else 0
    for line in range(LINES):
        for dot in range(DOTS):
            hs = 1 if dot < 2 else 0
            hbl = 1 if dot < 4 else 0
            vbl = 1 if line < 2 else 0
            vs = 1 if (line == 0 and vs_dot <= dot < vs_dot + 2) else 0
            r, g, b = colour if not vbl and not hbl else (0, 0, 0)
            out.append(sample(r, g, b, hbl, vbl, hs, vs, 1 if odd else 0))
    return out


def pack_header(windows: int = WINDOWS, window_samples: int = WINDOW_SAMPLES,
                capture_slots: int = CAPTURE_SLOTS,
                payload_offset: int = PAYLOAD_OFFSET, captures_published: int = 1,
                image_loss: int = 0, forced: int = 0, dropped: int = 0,
                epoch: int = 1, ready: bool = True,
                magic: int = ssm_capture.MAGIC, abi: int = 1,
                pin_prev: int = 2) -> bytes:
    flags = 1 if ready else 0
    return struct.pack(
        "<QQQQQQQQ",
        magic | (abi << 32) | (flags << 48),
        windows | (window_samples << 32),
        capture_slots | (pin_prev << 32),
        0x1000_0000 | (epoch << 32),
        payload_offset,
        ssm_capture.DESC_OFFSET | (ssm_capture.RECORD_OFFSET << 32),
        captures_published | (image_loss << 32),
        forced | (dropped << 32),
    )


def pack_descriptor(generation: int, state: int, first: int, count: int,
                    flags: int = 0, epoch: int = 1) -> bytes:
    return struct.pack("<QQQQ", generation | (state << 32), first, count,
                       flags | (epoch << 32))


def pack_record(code: int, cut: int, current: Tuple[int, int],
                prev0: Optional[Tuple[int, int]] = None,
                prev1: Optional[Tuple[int, int]] = None,
                capture_index: int = 0, epoch: int = 1,
                extra_status: int = 0, format: int = 1,
                applied_config: int = ssm_capture.CONFIG_NATIVE_CADENCE,
                history_incomplete: bool = False) -> bytes:
    status = ssm_capture.ST_USABLE | extra_status
    if prev0:
        status |= ssm_capture.ST_PREV0
    if prev1:
        status |= ssm_capture.ST_PREV1
    if history_incomplete:
        status |= ssm_capture.ST_HISTORY_INCOMPLETE
    p0 = prev0 or (0, 0)
    p1 = prev1 or (0, 0)
    w6 = (status & 0xFFFF) | ((format & 0xFF) << 16) | ((applied_config & 0xFF) << 24) | ((epoch & 0xFFFFFFFF) << 32)
    return struct.pack(
        "<QQQQQQQQ",
        code, 0, cut,
        current[0] | (current[1] << 32),
        p0[0] | (p0[1] << 32),
        p1[0] | (p1[1] << 32),
        w6,
        capture_index | (0x1000_0000 << 32),
    )


def build_region(descriptors: Dict[int, bytes], records: Sequence[bytes],
                 payloads: Dict[int, Sequence[int]], **header_kwargs: Any) -> bytes:
    header = pack_header(**header_kwargs)
    windows = header_kwargs.get("windows", WINDOWS)
    window_samples = header_kwargs.get("window_samples", WINDOW_SAMPLES)
    payload_offset = header_kwargs.get("payload_offset", PAYLOAD_OFFSET)
    region = bytearray(payload_offset + windows * window_samples * 4)
    region[0:len(header)] = header
    for index, blob in descriptors.items():
        off = ssm_capture.DESC_OFFSET + index * ssm_capture.DESC_BYTES
        region[off:off + len(blob)] = blob
    for slot, blob in enumerate(records):
        off = ssm_capture.RECORD_OFFSET + slot * ssm_capture.RECORD_BYTES
        region[off:off + len(blob)] = blob
    for index, words in payloads.items():
        base = payload_offset + index * window_samples * 4
        for k, word in enumerate(words):
            struct.pack_into("<I", region, base + k * 4, word)
    return bytes(region)


def one_window_region(words: Sequence[int], cut: int, *, generation: int = 1,
                      state: int = ssm_capture.STATE_SEALED, flags: int = 0,
                      record_generation: Optional[int] = None,
                      **header_kwargs: Any) -> bytes:
    header_kwargs.setdefault("pin_prev", 0)
    return build_region(
        descriptors={0: pack_descriptor(generation, state, 0, WINDOW_SAMPLES, flags)},
        records=[pack_record(0x0101, cut, (0, record_generation
                                           if record_generation is not None
                                           else generation))],
        payloads={0: words},
        **header_kwargs,
    )


class TestProgressiveDecode(unittest.TestCase):
    def setUp(self):
        self.profile = PROFILES["test-tiny-progressive"]

    def test_one_field_reconstructs_every_active_cell(self):
        words = field_words((0xE0, 0x10, 0x20), odd=False)
        region = one_window_region(words, cut=len(words))
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "complete", result.reasons)
        self.assertEqual(len(result.surface), 4 * 4)
        # Active lines are the four that follow the vertical blanking, and the
        # active dots are the four after the horizontal blanking.
        self.assertEqual(sorted({line for line, _ in result.surface}), [2, 3, 4, 5])
        self.assertEqual(sorted({dot for _, dot in result.surface}), [4, 5, 6, 7])
        self.assertEqual(result.surface[(2, 4)], (0xE0, 0x10, 0x20))

    def test_a_cut_excludes_the_sample_on_its_own_edge(self):
        red = field_words((0xF0, 0, 0), odd=False)
        blue = field_words((0, 0, 0xF0), odd=False)
        words = red + blue
        # The first active sample of the second field: line 2, dot 4.
        repaint = len(red) + 2 * DOTS + 4
        self.assertEqual(ssm_capture.sample_fields(words[repaint])["b"], 0xF0)

        before = ssm_capture.decode_capture_from_region(
            one_window_region(words, cut=repaint), 0, self.profile)
        self.assertEqual(before.surface[(2, 4)], (0xF0, 0, 0),
                         "the sample on the cut was applied")
        after = ssm_capture.decode_capture_from_region(
            one_window_region(words, cut=repaint + 1), 0, self.profile)
        self.assertEqual(after.surface[(2, 4)], (0, 0, 0xF0),
                         "the sample before the cut was not applied")

    def test_an_address_repainted_twice_keeps_the_value_its_cut_asks_for(self):
        red = field_words((0xF0, 0, 0), odd=False)
        blue = field_words((0, 0, 0xF0), odd=False)
        words = red + blue
        first = ssm_capture.decode_capture_from_region(
            one_window_region(words, cut=len(red)), 0, self.profile)
        second = ssm_capture.decode_capture_from_region(
            one_window_region(words, cut=len(words)), 0, self.profile)
        self.assertEqual(first.status, "complete", first.reasons)
        self.assertEqual(second.status, "complete", second.reasons)
        for cell in first.surface:
            self.assertEqual(first.surface[cell], (0xF0, 0, 0))
            self.assertEqual(second.surface[cell], (0, 0, 0xF0))

    def test_no_vsync_gives_an_explicit_incomplete_result_not_a_rectangle(self):
        words = [sample(0x30, 0x30, 0x30, 0, 0, 0, 0, 0) for _ in range(48)]
        region = one_window_region(words, cut=len(words))
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "incomplete")
        self.assertTrue(any("no VSYNC" in r for r in result.reasons), result.reasons)
        self.assertEqual(result.surface, {}, "an image was invented without sync")
        self.assertEqual(len(result.raw), WINDOW_SAMPLES,
                         "the raw trace was not retained")

    def test_an_extra_sync_is_another_field_rather_than_an_overwrite(self):
        # A short field: VSync arrives again after only four lines. The samples
        # are appended, so the later field lands on the same physical lines and
        # the earlier ones stay in the trace.
        words = field_words((0x10, 0x20, 0x30), odd=False)[:4 * DOTS]
        words += field_words((0x40, 0x50, 0x60), odd=False)
        region = one_window_region(words, cut=len(words))
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.metadata["fields_seen"], 2)
        self.assertEqual(result.surface[(2, 4)], (0x40, 0x50, 0x60))
        self.assertEqual(len(result.raw), WINDOW_SAMPLES)

    def test_a_line_that_is_not_profile_length_is_reported_not_corrected(self):
        words = field_words((0x10, 0x20, 0x30), odd=False)
        # Drop one dot out of the middle of line 3, so that line measures 7.
        del words[3 * DOTS + 5]
        region = one_window_region(words, cut=len(words))
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "incomplete")
        self.assertGreaterEqual(result.metadata["lines_off_profile"], 1)
        self.assertTrue(any("dots long" in r for r in result.reasons), result.reasons)


class TestInterlaceDecode(unittest.TestCase):
    def setUp(self):
        self.profile = PROFILES["test-tiny-interlace"]

    def test_two_fields_land_on_alternating_physical_lines(self):
        words = field_words((0x11, 0x22, 0x33), odd=False)
        words += field_words((0x44, 0x55, 0x66), odd=True)
        region = one_window_region(words, cut=len(words))
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "complete", result.reasons)
        lines = sorted({line for line, _ in result.surface})
        self.assertEqual(lines, [4, 5, 6, 7, 8, 9, 10, 11])
        # Even physical lines come from the field whose VSync sat on the line
        # boundary; odd ones from the field half a line later.
        self.assertEqual(result.surface[(4, 4)], (0x11, 0x22, 0x33))
        self.assertEqual(result.surface[(5, 4)], (0x44, 0x55, 0x66))
        self.assertEqual(result.metadata["vsync_phases"], [0, HALF_LINE])
        self.assertEqual(result.metadata["derived_field_parity"], [0, 1])
        self.assertEqual(result.metadata["field_disagreements"], 0)

    def test_a_progressive_stream_read_as_interlace_is_incomplete(self):
        # Both fields at the same phase: the physical placement collapses onto
        # the even lines and the profile's half of the picture is missing.
        words = field_words((0x11, 0x22, 0x33), odd=False)
        words += field_words((0x44, 0x55, 0x66), odd=False)
        region = one_window_region(words, cut=len(words))
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "incomplete")
        self.assertEqual(sorted({line for line, _ in result.surface}), [4, 6, 8, 10])
        self.assertTrue(any("active cells" in r for r in result.reasons), result.reasons)

    def test_a_core_field_bit_that_contradicts_the_measured_phase_is_reported(self):
        words = field_words((0x11, 0x22, 0x33), odd=False)
        odd = field_words((0x44, 0x55, 0x66), odd=True)
        # Keep the half-line VSync phase but flip the core's FIELD bit, which is
        # exactly the disagreement worth surfacing rather than resolving.
        odd = [w & ~(1 << 28) for w in odd]
        region = one_window_region(words + odd, cut=len(words) + len(odd))
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.metadata["field_disagreements"], 1)
        self.assertEqual(result.status, "incomplete")
        self.assertTrue(any("core FIELD" in r for r in result.reasons), result.reasons)

    def test_an_unsupported_vsync_phase_refuses_to_place_the_field(self):
        words = field_words((0x11, 0x22, 0x33), odd=False)
        odd = field_words((0x44, 0x55, 0x66), odd=True)
        # Move the second VSync one dot off both supported phases.
        odd[HALF_LINE] &= ~(1 << 27)
        odd[HALF_LINE + 2] |= 1 << 27
        region = one_window_region(words + odd, cut=len(words) + len(odd))
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "incomplete")
        self.assertTrue(any("phase this profile does not describe" in r
                            for r in result.reasons), result.reasons)


class TestWindowLifecycle(unittest.TestCase):
    def setUp(self):
        self.profile = PROFILES["test-tiny-progressive"]
        self.words = field_words((0x11, 0x22, 0x33), odd=False)

    def test_a_reused_window_is_a_capture_loss_not_a_mixed_image(self):
        region = one_window_region(self.words, cut=len(self.words),
                                   generation=9, record_generation=1)
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "lost")
        self.assertTrue(any("reused" in r for r in result.reasons), result.reasons)
        self.assertEqual(result.surface, {})

    def test_an_unsealed_window_is_pending_rather_than_decoded(self):
        region = one_window_region(self.words, cut=len(self.words),
                                   state=ssm_capture.STATE_INVALID)
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "pending")
        self.assertTrue(any("not sealed" in r for r in result.reasons), result.reasons)

    def test_a_window_that_lost_samples_cannot_produce_a_complete_image(self):
        region = one_window_region(self.words, cut=len(self.words),
                                   flags=ssm_capture.WIN_FLAG_LOST)
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "incomplete")
        self.assertTrue(any("lost samples" in r for r in result.reasons), result.reasons)
        # The trace is still there: a lossy window is evidence, not rubbish.
        self.assertEqual(len(result.raw), WINDOW_SAMPLES)

    def test_an_unpublished_capture_is_refused(self):
        region = one_window_region(self.words, cut=len(self.words),
                                   captures_published=0)
        with self.assertRaises(SsmCaptureError):
            ssm_capture.decode_capture_from_region(region, 0, self.profile)

    def test_a_slot_that_has_been_reused_by_a_later_capture_is_refused(self):
        region = build_region(
            descriptors={0: pack_descriptor(1, ssm_capture.STATE_SEALED, 0,
                                            WINDOW_SAMPLES)},
            records=[pack_record(0x0202, 10, (0, 1), capture_index=4)],
            payloads={0: self.words},
            captures_published=5,
            pin_prev=0,
        )
        with self.assertRaises(SsmCaptureError) as ctx:
            ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertIn("already been reused", str(ctx.exception))

    def test_history_spanning_two_windows_is_applied_in_time_order(self):
        first = field_words((0xAA, 0, 0), odd=False)
        second = field_words((0, 0xBB, 0), odd=False)
        region = build_region(
            descriptors={
                0: pack_descriptor(1, ssm_capture.STATE_SEALED, 0, len(first)),
                1: pack_descriptor(2, ssm_capture.STATE_SEALED, len(first), len(second)),
            },
            records=[pack_record(0x0303, len(first) + len(second), (1, 2),
                                 prev0=(0, 1))],
            payloads={0: first, 1: second},
            pin_prev=1,
        )
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "complete", result.reasons)
        # The later window's colour wins, which is only true if the two were
        # ordered by their first_logical_sample and not by pool index.
        self.assertEqual(result.surface[(2, 4)], (0, 0xBB, 0))
        self.assertEqual(result.metadata["fields_seen"], 2)


class TestHeaderAndTransport(unittest.TestCase):
    def test_a_region_that_was_never_written_is_reported_not_guessed(self):
        with self.assertRaises(ssm_capture.SsmCaptureNotInitializedError):
            ssm_capture.CaptureHeader(b"\x00" * 64).validate()

    def test_a_future_abi_is_refused(self):
        with self.assertRaises(ssm_capture.SsmCaptureFormatError):
            ssm_capture.CaptureHeader(pack_header(abi=7)).validate()

    def test_a_header_that_is_not_ready_is_not_decoded(self):
        with self.assertRaises(ssm_capture.SsmCaptureNotInitializedError):
            ssm_capture.CaptureHeader(pack_header(ready=False)).validate()

    def test_a_truncated_region_is_refused(self):
        with self.assertRaises(ssm_capture.SsmCaptureTruncatedError):
            ssm_capture.CaptureHeader(b"\x00" * 16)

    def test_an_empty_device_read_is_a_transport_error(self):
        with self.assertRaises(ssm_capture.SsmCaptureTransportError):
            ssm_capture.decode_payload("   \n")

    def test_a_misaligned_read_is_refused_rather_than_rounded(self):
        with self.assertRaises(SsmCaptureError):
            ssm_capture.read_command(0x3100_0000, 0x21, 64)

    def test_the_read_command_never_writes(self):
        command = ssm_capture.read_command(0x3100_0000, 0, 64)
        self.assertIn("if=/dev/mem", command)
        self.assertNotIn("of=", command)


class TestBracketedPayloadRead(unittest.TestCase):
    """A payload read is only valid between two agreeing descriptor reads."""

    class FakeResult:
        def __init__(self, stdout: str):
            self.exit_code = 0
            self.stdout = stdout
            self.stderr = ""

    class FakeTransport:
        def __init__(self, region: bytes, mutate=None, base: int = 0x3100_0000):
            self.region = bytearray(region)
            self.mutate = mutate
            self.base = base
            self.reads = 0

        def run_cmd(self, command: str, timeout: float = 30.0):
            import base64 as b64
            import re
            m_bs = re.search(r"bs=(\d+)", command)
            bs = int(m_bs.group(1)) if m_bs else 32
            skip = int(re.search(r"skip=(\d+)", command).group(1))
            count = int(re.search(r"count=(\d+)", command).group(1))
            addr = skip * bs
            start = addr - self.base if addr >= self.base else addr
            length = count * bs
            self.reads += 1
            if self.mutate:
                self.mutate(self, self.reads)
            chunk = bytes(self.region[start:start + length])
            return TestBracketedPayloadRead.FakeResult(b64.b64encode(chunk).decode())

    def region(self, generation: int = 1,
               state: int = ssm_capture.STATE_SEALED) -> bytes:
        words = field_words((0x11, 0x22, 0x33), odd=False)
        return one_window_region(words, cut=len(words), generation=generation,
                                 state=state)

    def poke_descriptor(self, transport, generation: int, state: int) -> None:
        off = ssm_capture.DESC_OFFSET
        struct.pack_into("<Q", transport.region, off, generation | (state << 32))

    def test_a_stable_window_reads_back(self):
        transport = self.FakeTransport(self.region())
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        header = reader.header()
        samples = reader.window_payload(header, 0, 1)
        self.assertEqual(len(samples), WINDOW_SAMPLES)
        self.assertEqual(samples[0][0], 0)

    def test_a_reuse_that_started_before_the_first_read_is_caught(self):
        transport = self.FakeTransport(self.region(generation=5,
                                                   state=ssm_capture.STATE_INVALID))
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        header = reader.header()
        with self.assertRaises(SsmCaptureError) as ctx:
            reader.window_payload(header, 0, 1)
        self.assertIn("generation", str(ctx.exception))

    def test_a_reuse_during_the_copy_is_caught(self):
        state = {"done": False}

        def mutate(transport, reads):
            # Third read is the payload; change the window right after it.
            if reads == 3 and not state["done"]:
                state["done"] = True
                self.poke_descriptor(transport, 7, ssm_capture.STATE_INVALID)

        transport = self.FakeTransport(self.region(), mutate=mutate)
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        header = reader.header()
        with self.assertRaises(SsmCaptureError) as ctx:
            reader.window_payload(header, 0, 1)
        self.assertIn("reused while it was being read", str(ctx.exception))

    def test_an_unsealed_window_is_not_copied_at_all(self):
        transport = self.FakeTransport(self.region(state=ssm_capture.STATE_INVALID))
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        header = reader.header()
        with self.assertRaises(SsmCaptureError) as ctx:
            reader.window_payload(header, 0, 1)
        self.assertIn("not sealed", str(ctx.exception))

    def test_descriptor_1_read_and_payload_at_production_address(self):
        # Index 1 starts at +0x60, which previously failed on 64-byte alignment.
        words = field_words((0x44, 0x55, 0x66), odd=False)
        region = build_region(
            descriptors={
                0: pack_descriptor(1, ssm_capture.STATE_SEALED, 0, WINDOW_SAMPLES),
                1: pack_descriptor(2, ssm_capture.STATE_SEALED, 0, WINDOW_SAMPLES),
            },
            records=[pack_record(0x0101, len(words), (1, 2))],
            payloads={1: words},
            pin_prev=0,
        )
        transport = self.FakeTransport(region, base=0x3100_0000)
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        header = reader.header()
        desc1 = reader.descriptor(header, 1)
        self.assertEqual(desc1.generation, 2)
        samples = reader.window_payload(header, 1, 2)
        self.assertEqual(len(samples), WINDOW_SAMPLES)

    def test_window_payload_epoch_mismatch_raises(self):
        words = field_words((0x11, 0x22, 0x33), odd=False)
        region = build_region(
            descriptors={0: pack_descriptor(1, ssm_capture.STATE_SEALED, 0, WINDOW_SAMPLES, epoch=99)},
            records=[pack_record(0x0101, len(words), (0, 1), epoch=1)],
            payloads={0: words},
            epoch=1,
            pin_prev=0,
        )
        transport = self.FakeTransport(region, base=0x3100_0000)
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        header = reader.header()
        with self.assertRaises(SsmCaptureError) as ctx:
            reader.window_payload(header, 0, 1)
        self.assertIn("epoch", str(ctx.exception))

    def test_read_live_capture_assembly(self):
        words = field_words((0x11, 0x22, 0x33), odd=False)
        region = one_window_region(words, cut=len(words))
        transport = self.FakeTransport(region, base=0x3100_0000)
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        h, rec, payloads = reader.read_live_capture(0)
        self.assertEqual(rec.capture_index, 0)
        self.assertIn(0, payloads)
        self.assertEqual(len(payloads[0]), WINDOW_SAMPLES)

    def test_record_commit_identity_invalid_publication_rejected(self):
        words = field_words((0x11, 0x22, 0x33), odd=False)
        region = bytearray(one_window_region(words, cut=len(words)))
        # Poke word 7 of slot 0 to 0xFFFFFFFF_FFFFFFFF (invalidation before write)
        off = ssm_capture.RECORD_OFFSET + 0x38
        struct.pack_into("<Q", region, off, 0xFFFF_FFFF_FFFF_FFFF)
        transport = self.FakeTransport(region, base=0x3100_0000)
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        with self.assertRaises(SsmCaptureError) as ctx:
            reader.read_live_capture(0)
        self.assertIn("invalid", str(ctx.exception))

    def test_record_commit_identity_mutation_across_reads_rejected(self):
        words = field_words((0x11, 0x22, 0x33), odd=False)
        region = one_window_region(words, cut=len(words))

        def mutate(transport, reads):
            # Mutate slot 0 word 7 during payload reading (reads == 5)
            if reads == 5:
                off = ssm_capture.RECORD_OFFSET + 0x38
                # Slot reused: capture_index becomes 4
                struct.pack_into("<Q", transport.region, off, 4 | (0x1000_0000 << 32))

        transport = self.FakeTransport(region, mutate=mutate, base=0x3100_0000)
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        with self.assertRaises(SsmCaptureError) as ctx:
            reader.read_live_capture(0)
        self.assertIn("commit identity changed", str(ctx.exception))

    def test_decode_live_capture_end_to_end_at_production_address(self):
        import tempfile, pathlib
        words = field_words((0x11, 0x22, 0x33), odd=False)
        region = one_window_region(words, cut=len(words))
        transport = self.FakeTransport(region, base=0x3100_0000)
        reader = ssm_capture.CaptureReader(transport, base=0x3100_0000)
        profile = PROFILES["test-tiny-progressive"]
        with tempfile.TemporaryDirectory() as tmpdir:
            res = ssm_capture.decode_live_capture(
                reader, 0, profile, out_dir=tmpdir, out_prefix="test_cap"
            )
            self.assertEqual(res.status, "complete", res.reasons)
            p = pathlib.Path(tmpdir)
            raw_p = p / "test_cap.raw"
            json_p = p / "test_cap.json"
            ppm_p = p / "test_cap.ppm"
            self.assertTrue(raw_p.exists())
            self.assertTrue(json_p.exists())
            self.assertTrue(ppm_p.exists())
            self.assertEqual(raw_p.stat().st_size, len(res.raw) * 4)
            with open(ppm_p, "rb") as f:
                header = f.readline()
                self.assertEqual(header, b"P6\n")


class TestRecordDecoding(unittest.TestCase):
    def test_the_legacy_event_prefix_decodes_like_a_ring_record(self):
        # Words 0 and 1 are byte-identical to the format-1 ring record pair, so
        # the same bit positions have to come back out.
        word_a = 0xFFFE | (0x123456 << 16) | (0x2A1 << 40) | (0xC3 << 50) | (1 << 58)
        word_b = 7 | (0xDEADBEEF << 16)
        blob = struct.pack("<QQQQQQQQ", word_a, word_b, 99, 0, 0, 0, 1, 0)
        record = CaptureRecord(blob, 0)
        self.assertEqual(record.code, 0xFFFE)
        self.assertEqual(record.frame, 0x123456)
        self.assertEqual(record.line, 0x2A1)
        self.assertEqual(record.hpos, 0xC3)
        self.assertEqual(record.field, 1)
        self.assertEqual(record.seq, 7)
        self.assertEqual(record.tick, 0xDEADBEEF)
        self.assertEqual(record.cut, 99)

    def test_referenced_windows_are_oldest_first(self):
        blob = pack_record(0x0001, 10, (2, 30), prev0=(1, 20), prev1=(0, 10))
        record = CaptureRecord(blob, 0)
        self.assertEqual(record.referenced_windows(), [(0, 10), (1, 20), (2, 30)])
        self.assertFalse(record.as_dict()["history_incomplete"])


class TestImageOutput(unittest.TestCase):
    def test_unknown_cells_are_not_dressed_up_as_picture(self):
        profile = PROFILES["test-tiny-progressive"]
        words = field_words((0x11, 0x22, 0x33), odd=False)
        # Blank one active dot so its cell is never written.
        words[2 * DOTS + 5] |= 1 << 24
        region = one_window_region(words, cut=len(words))
        result = ssm_capture.decode_capture_from_region(region, 0, profile)
        self.assertEqual(result.status, "incomplete")
        ppm = result.to_ppm()
        self.assertTrue(ppm.startswith(b"P6\n4 4\n255\n"))
        self.assertIn(b"\xff\x00\xff", ppm, "the unwritten cell was filled in")

    def test_profiles_do_not_claim_to_be_measured(self):
        for name, profile in PROFILES.items():
            with self.subTest(profile=name):
                self.assertFalse(profile.as_dict()["measured_on_hardware"])


class TestValidationAndMutations(unittest.TestCase):
    """Mutations testing Finding 7, 8, 13 rules that protect completeness."""

    def setUp(self):
        self.profile = PROFILES["test-tiny-progressive"]
        self.words = field_words((0x10, 0x20, 0x30), odd=False)

    def test_header_record_epoch_mismatch_fails(self):
        region = one_window_region(self.words, cut=len(self.words), epoch=2)
        # Header has epoch 2, pack_record by default has epoch 1
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "lost")
        self.assertIn("epoch", " ".join(result.reasons))

    def test_header_descriptor_epoch_mismatch_fails(self):
        region = build_region(
            descriptors={0: pack_descriptor(1, ssm_capture.STATE_SEALED, 0, WINDOW_SAMPLES, epoch=2)},
            records=[pack_record(0x0101, len(self.words), (0, 1), epoch=1)],
            payloads={0: self.words},
            epoch=1,
            pin_prev=0,
        )
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "lost")
        self.assertIn("epoch", " ".join(result.reasons))

    def test_status_history_incomplete_alone_prevents_complete(self):
        region = build_region(
            descriptors={0: pack_descriptor(1, ssm_capture.STATE_SEALED, 0, WINDOW_SAMPLES)},
            records=[pack_record(0x0101, len(self.words), (0, 1), history_incomplete=True)],
            payloads={0: self.words},
            pin_prev=0,
        )
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "incomplete")
        self.assertIn("ST_HISTORY_INCOMPLETE", " ".join(result.reasons))

    def test_status_lost_alone_reports_lost(self):
        region = build_region(
            descriptors={0: pack_descriptor(1, ssm_capture.STATE_SEALED, 0, WINDOW_SAMPLES)},
            records=[pack_record(0x0101, len(self.words), (0, 1), extra_status=ssm_capture.ST_LOST)],
            payloads={0: self.words},
            pin_prev=0,
        )
        result = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(result.status, "lost")
        self.assertIn("ST_LOST", " ".join(result.reasons))

    def test_validation_probe_missing_predecessors_incomplete(self):
        # Header promises PIN_PREV=2, but record omits both prev refs.
        region = build_region(
            descriptors={0: pack_descriptor(1, ssm_capture.STATE_SEALED, 0, WINDOW_SAMPLES)},
            records=[pack_record(0x0101, len(self.words), (0, 1), applied_config=0x01)],
            payloads={0: self.words},
            pin_prev=2,
        )
        r = ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertEqual(r.status, "incomplete")
        self.assertTrue(any("predecessor" in reason for reason in r.reasons))

    def test_validation_probe_missing_native_cadence_incomplete(self):
        rec = ssm_capture.CaptureRecord(
            pack_record(0x0101, len(self.words), (0, 1), applied_config=0x00), 0
        )
        r = ssm_capture.decode_samples(
            list(enumerate(self.words)), len(self.words), self.profile, rec
        )
        self.assertEqual(r.status, "incomplete")
        self.assertTrue(any("native cadence" in reason for reason in r.reasons))

    def test_contradictory_cadence_is_rejected_by_offline_and_live_decoders(self):
        # Keep colour/machine metadata valid; mutate only eligibility inputs.
        for bit in (0, ssm_capture.CONFIG_RAW_CRT, ssm_capture.CONFIG_PIXEL_RATE_SEL,
                    ssm_capture.CONFIG_HQ2X):
            with self.subTest(bit=bit):
                region = build_region(
                    descriptors={0: pack_descriptor(1, ssm_capture.STATE_SEALED, 0, WINDOW_SAMPLES)},
                    records=[pack_record(0x0101, len(self.words), (0, 1), applied_config=0x91 | bit)],
                    payloads={0: self.words}, pin_prev=0)
                offline = ssm_capture.decode_capture_from_region(region, 0, self.profile)
                reader = ssm_capture.CaptureReader(TestBracketedPayloadRead.FakeTransport(region))
                live = ssm_capture.decode_live_capture(reader, 0, self.profile)
                for result in (offline, live):
                    self.assertEqual(result.complete, bit == 0, result.reasons)

    def test_more_predecessors_than_the_abi_can_represent_is_rejected(self):
        header = ssm_capture.CaptureHeader(pack_header(pin_prev=2))
        header.validate()
        with self.assertRaises(ssm_capture.SsmCaptureFormatError):
            ssm_capture.CaptureHeader(pack_header(pin_prev=3)).validate()

    def test_zero_capture_slots_rejected(self):
        region = one_window_region(self.words, cut=len(self.words), capture_slots=0)
        with self.assertRaises(ssm_capture.SsmCaptureFormatError) as ctx:
            ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertIn("slot count", str(ctx.exception))

    def test_overlapping_offsets_rejected(self):
        region = one_window_region(self.words, cut=len(self.words), payload_offset=0x50)
        with self.assertRaises(ssm_capture.SsmCaptureFormatError) as ctx:
            ssm_capture.decode_capture_from_region(region, 0, self.profile)
        self.assertIn("overlaps", str(ctx.exception))

    def test_missing_rows_compensated_by_extra_columns_is_incomplete(self):
        # Two rows of 8 dots instead of four rows of 4 dots
        changed = []
        for n, w in enumerate(self.words):
            line = n // 8
            if line in (2, 3):
                w &= ~((1 << 24) | (1 << 25))
            else:
                w |= 1 << 25
            changed.append(w)
        r = CaptureRecord(pack_record(0x0101, len(changed), (0, 1)), 0)
        res = ssm_capture.decode_samples(list(enumerate(changed)), len(changed), self.profile, r)
        self.assertEqual(res.status, "incomplete")
        self.assertIn("active line(s) populated", " ".join(res.reasons))

    def test_missing_columns_compensated_by_extra_rows_is_incomplete(self):
        # Generate 8 lines of 2 active dots (same 16 pixels, but wrong geometry)
        words_8lines = []
        for line in range(8):
            for dot in range(8):
                hs = 1 if dot < 2 else 0
                hbl = 1 if dot < 6 else 0
                vbl = 0
                vs = 1 if (line == 0 and dot < 2) else 0
                words_8lines.append(sample(10, 20, 30, hbl, vbl, hs, vs, 0))
        r = CaptureRecord(pack_record(0x0101, len(words_8lines), (0, 1)), 0)
        res = ssm_capture.decode_samples(list(enumerate(words_8lines)), len(words_8lines), self.profile, r)
        self.assertEqual(res.status, "incomplete")

    def test_field_length_mismatch_is_incomplete(self):
        # 5 lines instead of 6 lines per field
        words_5lines = []
        for line in range(5):
            for dot in range(8):
                hs = 1 if dot < 2 else 0
                hbl = 1 if dot < 4 else 0
                vbl = 1 if line < 2 else 0
                vs = 1 if (line == 0 and dot < 2) else 0
                words_5lines.append(sample(10, 20, 30, hbl, vbl, hs, vs, 0))
        r = CaptureRecord(pack_record(0x0101, len(words_5lines), (0, 1)), 0)
        res = ssm_capture.decode_samples(list(enumerate(words_5lines)), len(words_5lines), self.profile, r)
        self.assertEqual(res.status, "incomplete")
        self.assertIn("field length", " ".join(res.reasons))

    def test_retained_samples_gap_immediately_before_cut_is_incomplete(self):
        samples = list(enumerate(self.words))[:-1]  # Omits the sample at cut - 1
        r = CaptureRecord(pack_record(0x0101, len(self.words), (0, 1)), 0)
        res = ssm_capture.decode_samples(samples, len(self.words), self.profile, r)
        self.assertEqual(res.status, "incomplete")
        self.assertIn("cut - 1", " ".join(res.reasons))

    def test_shifted_active_window_bounds_distinguishable(self):
        # Normal image
        reg1 = one_window_region(self.words, cut=len(self.words))
        res1 = ssm_capture.decode_capture_from_region(reg1, 0, self.profile)
        # Shifted image: active rows are 3..6 instead of 2..5
        words_shifted = []
        for line in range(7):
            for dot in range(DOTS):
                hs = 1 if dot < 2 else 0
                hbl = 1 if dot < 4 else 0
                vbl = 1 if line < 3 else 0
                vs = 1 if (line == 0 and dot < 2) else 0
                words_shifted.append(sample(0x10, 0x20, 0x30, hbl, vbl, hs, vs, 0))
        p_shifted = ssm_capture.RasterProfile(
            "test-shifted", line_dots=8, lines_per_field=7,
            active_dots=4, active_lines=4, interlaced=False, fields_required=1)
        reg2 = one_window_region(words_shifted, cut=len(words_shifted), window_samples=256)
        res2 = ssm_capture.decode_capture_from_region(reg2, 0, p_shifted)
        self.assertNotEqual(res1.bounds, res2.bounds)
        self.assertNotEqual(res1.crop_origin, res2.crop_origin)
        self.assertEqual(res1.crop_origin, (2, 4))
        self.assertEqual(res2.crop_origin, (3, 4))


if __name__ == "__main__":
    unittest.main()
