"""Offline tests for the SSM event ring reader and the runner's use of it.

The ring image fixtures here are built byte by byte from the layout comment in
``rtl/ssm_marker.v``, not captured from the simulator, so a field the RTL moves
breaks these tests instead of quietly agreeing with the move.
"""

from __future__ import annotations

import base64
import struct
import tempfile
import unittest
from pathlib import Path
from typing import Any, Dict, List

import ssm_ring
from csl_runner import CFG_BIT_SSM, CslError, RunOptions, run_csl
from driver import CommandResult
from test_csl_runner import MINIMAL, DeviceHarnessMixin, options, write


def make_record(code: int, seq: int, frame: int = 0, line: int = 0, hpos: int = 0,
                field: int = 0, tick: int = 0) -> bytes:
    word_a = (code & 0xFFFF) | ((frame & 0xFFFFFF) << 16) | ((line & 0x3FF) << 40) \
        | ((hpos & 0xFF) << 50) | ((field & 1) << 58)
    word_b = (seq & 0xFFFF) | ((tick & 0xFFFFFFFF) << 16)
    return struct.pack("<QQ", word_a, word_b)


def make_ring(records: List[bytes], written: int, dropped: int = 0,
              entries: int = 64, magic: int = ssm_ring.MAGIC,
              version: int = ssm_ring.SUPPORTED_FORMAT) -> bytes:
    word0 = (magic & 0xFFFFFFFF) | ((version & 0xFFFF) << 32) | ((entries & 0xFF) << 48)
    word1 = (written & 0xFFFFFFFF) | ((dropped & 0xFF) << 32)
    body = bytearray(entries * ssm_ring.RECORD_BYTES)
    for index, record in enumerate(records):
        body[index * 16:(index + 1) * 16] = record
    return struct.pack("<QQ", word0, word1) + bytes(body)


class TestRingParsing(unittest.TestCase):
    def test_record_fields_unpack_as_the_rtl_packs_them(self):
        raw = make_record(code=0xFFFE, seq=7, frame=0x123456, line=0x2A1,
                          hpos=0xC3, field=1, tick=0xDEADBEEF)
        _, records = ssm_ring.parse_ring(make_ring([raw], written=1))
        rec = records[0]
        self.assertEqual(rec.code, 0xFFFE)
        self.assertEqual(rec.frame, 0x123456)
        self.assertEqual(rec.line, 0x2A1)
        self.assertEqual(rec.hpos, 0xC3)
        self.assertEqual(rec.field, 1)
        self.assertEqual(rec.seq, 7)
        self.assertEqual(rec.tick, 0xDEADBEEF)

    def test_header_fields(self):
        header = ssm_ring.parse_header(make_ring([], written=9, dropped=2))
        self.assertEqual(header.magic, ssm_ring.MAGIC)
        self.assertEqual(header.entries, 64)
        self.assertEqual(header.written, 9)
        self.assertEqual(header.dropped, 2)

    def test_a_ring_that_was_never_written_is_reported_not_guessed(self):
        with self.assertRaises(ssm_ring.SsmRingError) as ctx:
            ssm_ring.parse_header(b"\x00" * 16)
        self.assertIn("SSM OSD option is off", str(ctx.exception))

    def test_a_future_format_version_is_refused(self):
        with self.assertRaises(ssm_ring.SsmRingError):
            ssm_ring.parse_header(make_ring([], written=0, version=99))

    def test_truncated_image_is_refused(self):
        with self.assertRaises(ssm_ring.SsmRingError):
            ssm_ring.parse_ring(make_ring([], written=0)[:200])


class TestRingConsumption(unittest.TestCase):
    def test_only_new_records_are_returned(self):
        records = [make_record(0x1000 + i, i) for i in range(5)]
        header, parsed = ssm_ring.parse_ring(make_ring(records, written=5))
        self.assertEqual([r.code for r in ssm_ring.records_since(header, parsed, 0)],
                         [0x1000 + i for i in range(5)])
        self.assertEqual([r.code for r in ssm_ring.records_since(header, parsed, 3)],
                         [0x1003, 0x1004])
        self.assertEqual(ssm_ring.records_since(header, parsed, 5), [])

    def test_a_wrapped_ring_returns_what_survived_not_a_wrong_order(self):
        # 70 events into a 64-entry ring: slots hold events 6..69, and a
        # reader that consumed 0 can only be handed the 64 that survive.
        entries = 64
        slots = [make_record(0x2000 + (i % entries), i % entries) for i in range(entries)]
        header, parsed = ssm_ring.parse_ring(make_ring(slots, written=70, entries=entries))
        survived = ssm_ring.records_since(header, parsed, 0)
        self.assertEqual(len(survived), entries)
        # Oldest surviving event is number 6, which lives in slot 6.
        self.assertEqual(survived[0].slot, 6)
        self.assertEqual(survived[-1].slot, 69 % entries)

    def test_reader_counts_what_it_lost(self):
        entries = 8
        slots = [make_record(0x30 + i, i) for i in range(entries)]

        class Fake:
            def __init__(self):
                self.written = 0

            def run_cmd(self, cmd: str, timeout: float) -> CommandResult:
                image = make_ring(slots, written=self.written, entries=entries)
                if "count=1 " in cmd:
                    image = image[:16]
                return CommandResult(0, base64.b64encode(image).decode(), "")

        fake = Fake()
        reader = ssm_ring.SsmRingReader(fake, entries=entries)
        fake.written = 3
        _, new = reader.poll()
        self.assertEqual(len(new), 3)
        self.assertEqual(reader.lost, 0)
        fake.written = 3 + entries + 2   # two records overwritten before we looked
        _, new = reader.poll()
        self.assertEqual(len(new), entries)
        self.assertEqual(reader.lost, 2)


class TestReadCommand(unittest.TestCase):
    def test_block_size_divides_the_base_so_busybox_dd_can_seek(self):
        # BusyBox has no iflag=skip_bytes, so the skip has to be in blocks.
        cmd = ssm_ring.read_command(0x3000_0000, 64)
        self.assertIn("bs=16", cmd)
        self.assertIn(f"skip={0x3000_0000 // 16}", cmd)
        self.assertIn("count=65", cmd)
        self.assertIn("if=/dev/mem", cmd)

    def test_header_only_read_is_one_block(self):
        self.assertIn("count=1", ssm_ring.read_command(0x3000_0000, 64, header_only=True))

    def test_a_misaligned_base_is_refused_rather_than_rounded(self):
        with self.assertRaises(ssm_ring.SsmRingError):
            ssm_ring.read_command(0x3000_0001, 64)

    def test_suggested_name_follows_the_standard(self):
        # SSM v1.1: <Emulator name>_<CRTC number>_<HHLL code>.<extension>
        self.assertEqual(ssm_ring.suggested_name("MISTER", "1", 0xFFFE), "MISTER_1_FFFE.png")


class TestRunnerSsmIntegration(DeviceHarnessMixin, unittest.TestCase):
    """The runner's SSM path over the scripted-transport device harness."""

    def setUp(self):
        super().setUp()
        self.ring_written = 0
        self.ring_slots: List[bytes] = []
        self.polls = 0
        self.publish_at: Dict[int, int] = {}
        self.transport.handlers.insert(0, (
            lambda c: "if=/dev/mem" in c,
            self._serve_ring,
        ))

    def _serve_ring(self, cmd: str) -> CommandResult:
        header_only = "count=1 " in cmd
        if not header_only:
            self.polls += 1
            code = self.publish_at.pop(self.polls, None)
            if code is not None:
                self.publish(code)
        image = make_ring(self.ring_slots, written=self.ring_written)
        if header_only:
            image = image[:16]
        return CommandResult(0, base64.b64encode(image).decode(), "")

    def publish(self, code: int) -> None:
        self.ring_slots.append(make_record(code, self.ring_written))
        self.ring_written += 1

    def test_enabling_ssm_sets_the_osd_bit_and_restores_it(self):
        manifest = self._run(MINIMAL, ssm=True)
        self.assertEqual(manifest["actions"][0]["cfg_bits_changed"][str(CFG_BIT_SSM)], 1)
        self.assertTrue(manifest["cleanup"]["cfg_restore"]["matches_original"])

    def test_a_screenshot_marker_produces_a_standard_named_capture(self):
        self.publish_at[2] = ssm_ring.CODE_SCREENSHOT
        manifest = self._run(MINIMAL, ssm=True)
        captures = manifest["captures"]
        self.assertTrue(captures, "the #FFFE marker produced no capture")
        # SSM v1.1 suggests <Emulator>_<CRTC>_<HHLL>; the sequence number is
        # appended because one script emits #FFFE many times.
        self.assertTrue(captures[0]["name"].startswith("MISTER_1_FFFE_"),
                        captures[0]["name"])
        self.assertEqual(manifest["ssm_records"][0]["code"], "FFFE")

    def test_screenshot_name_overrides_the_standard_name(self):
        self.publish_at[2] = ssm_ring.CODE_SCREENSHOT
        manifest = self._run("screenshot_name 'b9_crtc1'\n" + MINIMAL, ssm=True)
        self.assertEqual(manifest["captures"][0]["name"], "b9_crtc1.png")

    def test_the_marker_raster_position_travels_with_the_capture(self):
        # Main grabs the scaler output asynchronously, so the capture lands
        # at least a frame after the marker. Keeping the marker's own frame
        # and line is what makes that distance visible instead of assumed.
        self.publish_at[2] = ssm_ring.CODE_SCREENSHOT
        manifest = self._run(MINIMAL, ssm=True)
        record = manifest["ssm_records"][0]
        for field in ("frame", "line", "hpos", "field", "seq", "tick"):
            self.assertIn(field, record)

    def test_wait_ssm0000_is_refused_unless_the_detector_is_on(self):
        with self.assertRaises(CslError) as ctx:
            self._run(MINIMAL + "wait_ssm0000\n")
        self.assertIn("--ssm", ctx.exception.reason)

    def test_wait_ssm0000_releases_on_a_sync_marker_that_arrives_later(self):
        script = MINIMAL + "wait_ssm0000\nwait 1\n"
        # Publish well after the script's own waits, so the release can only
        # come from the marker and not from a record that was already there.
        self.publish_at[12] = ssm_ring.CODE_SYNC
        manifest = self._run(script, ssm=True, max_wait_seconds=60.0)
        released = [e for e in manifest["trace"] if e["outcome"] == "ssm_sync_released"]
        self.assertEqual(len(released), 1)
        self.assertGreater(released[0]["polls_seconds"], 0)

    def test_wait_ssm0000_gives_up_at_the_bound_instead_of_hanging(self):
        with self.assertRaises(CslError) as ctx:
            self._run(MINIMAL + "wait_ssm0000\n", ssm=True, max_wait_seconds=2.0)
        self.assertIn("no SSM #0000 arrived", ctx.exception.reason)

    def test_an_unwritten_ring_is_not_treated_as_a_failure(self):
        # Before the first marker the DDR3 window holds whatever was there.
        # That must not abort a run whose script never needs a marker.
        self.transport.handlers.insert(0, (
            lambda c: "if=/dev/mem" in c,
            lambda c: CommandResult(0, base64.b64encode(b"\x00" * 1040).decode(), ""),
        ))
        manifest = self._run(MINIMAL, ssm=True)
        self.assertEqual(manifest["status"], "success")
        self.assertEqual(manifest["ssm_records"], [])


if __name__ == "__main__":
    unittest.main()
