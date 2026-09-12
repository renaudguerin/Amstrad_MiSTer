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
from csl_runner import CFG_BIT_SSM, CslError, DeviceBackend, RunOptions, run_csl
from driver import CommandResult, DriverError
from shaker_ssm_inventory import ssm_byte_allowed as inv_allowed
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
        # The conservative margin gives up one more slot than the naive
        # arithmetic: at the moment the second header says 13, the core may
        # already have written the record for index 13 into slot 13 % 8 = 5
        # without having committed the count. Indices 3, 4 and 5 are therefore
        # all unsafe, and saying so is the point.
        self.assertEqual([r.slot for r in new], [6, 7, 0, 1, 2, 3, 4])
        self.assertEqual(reader.lost, 3)

    def test_a_header_that_went_backwards_is_a_restart_not_a_loss(self):
        # Enabling the observer republishes written = 0. A reader that treated
        # that as an enormous loss would report nonsense for every later poll.
        entries = 8
        slots = [make_record(0x40 + i, i) for i in range(entries)]

        class Fake:
            def __init__(self):
                self.written = 5

            def run_cmd(self, cmd: str, timeout: float) -> CommandResult:
                image = make_ring(slots, written=self.written, entries=entries)
                if "count=1 " in cmd:
                    image = image[:16]
                return CommandResult(0, base64.b64encode(image).decode(), "")

        fake = Fake()
        reader = ssm_ring.SsmRingReader(fake, entries=entries)
        reader.poll()
        self.assertEqual(reader.consumed, 5)
        fake.written = 0
        _, new = reader.poll()
        self.assertEqual(new, [])
        self.assertEqual(reader.restarts, 1)
        self.assertEqual(reader.lost, 0)
        self.assertTrue(reader.startup_seen)
        fake.written = 2
        _, new = reader.poll()
        self.assertEqual([r.code for r in new], [0x40, 0x41])


class TestReadFailuresAreDistinguished(unittest.TestCase):
    """A failed read must never come back as "no marker"."""

    def _reader(self, exit_code: int = 0, stdout: str = "", stderr: str = ""):
        class Fake:
            def run_cmd(self, cmd: str, timeout: float) -> CommandResult:
                return CommandResult(exit_code, stdout, stderr)

        return ssm_ring.SsmRingReader(Fake())

    def test_a_failing_dd_is_a_transport_error(self):
        reader = self._reader(exit_code=1, stderr="Permission denied")
        with self.assertRaises(ssm_ring.SsmTransportError) as ctx:
            reader.poll()
        self.assertIn("Permission denied", str(ctx.exception))

    def test_an_empty_read_is_a_transport_error_not_an_empty_ring(self):
        reader = self._reader(stdout="")
        with self.assertRaises(ssm_ring.SsmTransportError):
            reader.poll()

    def test_garbage_that_is_not_base64_is_a_transport_error(self):
        reader = self._reader(stdout="not base64 !!")
        with self.assertRaises(ssm_ring.SsmTransportError):
            reader.poll()

    def test_a_short_read_is_truncation_not_an_unwritten_ring(self):
        image = make_ring([make_record(0x1234, 0)], written=1)[:512]
        reader = self._reader(stdout=base64.b64encode(image).decode())
        with self.assertRaises(ssm_ring.SsmTruncatedError):
            reader.poll()

    def test_an_unknown_format_version_is_a_format_error(self):
        image = make_ring([], written=0, version=42)
        reader = self._reader(stdout=base64.b64encode(image).decode())
        with self.assertRaises(ssm_ring.SsmFormatError) as ctx:
            reader.poll()
        self.assertNotIsInstance(ctx.exception, ssm_ring.SsmNotInitializedError)

    def test_an_absent_header_is_its_own_startup_case(self):
        reader = self._reader(stdout=base64.b64encode(b"\x00" * 1040).decode())
        with self.assertRaises(ssm_ring.SsmNotInitializedError):
            reader.poll()


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
        # SSM v1.1: <Emulator name>_<CRTC number>_<HHLL code>.<extension>.
        # Its own example is CRTC 2, ED E3 ED 02 -> AMSPIRIT_2_02E3.bmp.
        self.assertEqual(ssm_ring.suggested_name("AMSPIRIT", "2", 0x02E3, "bmp"),
                         "AMSPIRIT_2_02E3.bmp")
        self.assertEqual(ssm_ring.suggested_name("SUGARBOX", "0", 0x01DA, "jpg"),
                         "SUGARBOX_0_01DA.jpg")

    def test_reserved_set_is_0000_plus_every_ffxx(self):
        # The standard says the reserved set is 178 values: #0000 plus the 177
        # legal LL values with HH = #FF. That arithmetic is the independent
        # check on the allowed byte set.
        self.assertTrue(ssm_ring.is_reserved(0x0000))
        self.assertTrue(ssm_ring.is_reserved(0xFF00))
        self.assertTrue(ssm_ring.is_reserved(0xFFFF))
        self.assertFalse(ssm_ring.is_reserved(0x0001))
        self.assertFalse(ssm_ring.is_reserved(0x040C))
        self.assertFalse(ssm_ring.is_reserved(0xFE00))
        legal = [b for b in range(256) if inv_allowed(b)]
        self.assertEqual(len(legal), 177)
        self.assertEqual(1 + len(legal), 178)

    def test_screenshot_requests_are_every_non_reserved_code_plus_fffe(self):
        self.assertTrue(ssm_ring.is_screenshot_request(0x0001))
        self.assertTrue(ssm_ring.is_screenshot_request(0xFFFE))
        self.assertFalse(ssm_ring.is_screenshot_request(0x0000))
        self.assertFalse(ssm_ring.is_screenshot_request(0xFFFF))
        self.assertFalse(ssm_ring.is_screenshot_request(0xFFFD))


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
            entry = self.publish_at.pop(self.polls, None)
            if entry is not None:
                code, frame = entry if isinstance(entry, tuple) else (entry, 0)
                self.publish(code, frame)
        image = make_ring(self.ring_slots, written=self.ring_written)
        if header_only:
            image = image[:16]
        return CommandResult(0, base64.b64encode(image).decode(), "")

    def publish(self, code: int, frame: int = 0) -> None:
        self.ring_slots.append(make_record(code, self.ring_written, frame=frame))
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
        # SSM v1.1 suggests <Emulator>_<CRTC>_<HHLL>.
        self.assertEqual(captures[0]["name"], "MISTER_1_FFFE.png")
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

    def test_an_ordinary_code_is_a_screenshot_request(self):
        # This is how SHAKER actually works: it assigns a code per test screen
        # and the standard says any non-reserved code triggers a capture named
        # from the code. #FFFE is only the "name it from screenshot_name"
        # variant, which the bundled scripts never use.
        self.publish_at[2] = 0x0001
        manifest = self._run(MINIMAL, ssm=True)
        self.assertEqual([c["name"] for c in manifest["captures"]], ["MISTER_1_0001.png"])

    def test_the_crtc_in_the_name_follows_the_applied_type(self):
        self.publish_at[2] = 0x040C
        manifest = self._run(MINIMAL.replace("crtc_select 1", "crtc_select 0"), ssm=True)
        self.assertEqual(manifest["captures"][0]["name"], "MISTER_0_040C.png")

    def test_reserved_codes_other_than_fffe_do_not_capture(self):
        for code in (ssm_ring.CODE_SIKOVIEW_START, ssm_ring.CODE_SIKOVIEW_BREAK, 0xFF42):
            with self.subTest(code=code):
                self.setUp()
                self.publish_at[2] = code
                manifest = self._run(MINIMAL, ssm=True)
                self.assertEqual(manifest["captures"], [])
                self.assertEqual(manifest["ssm_records"][0]["code"], f"{code:04X}")

    def test_a_repeated_code_does_not_overwrite_the_earlier_capture(self):
        self.publish_at[2] = (0x0007, 10)
        self.publish_at[3] = (0x0007, 60)
        manifest = self._run(MINIMAL, ssm=True)
        self.assertEqual([c["name"] for c in manifest["captures"]],
                         ["MISTER_1_0007.png", "MISTER_1_0007_2.png"])

    def test_paired_markers_flag_both_captures_not_only_the_later_one(self):
        # SHAKER emits two markers close together on tests that flash between
        # two graphics. Main cannot serve two grabs a few frames apart, so
        # either capture may show the wrong phase; flagging only the second
        # would present the first as trustworthy.
        self.publish_at[2] = 0x0101
        self.publish_at[3] = 0x0102
        manifest = self._run(MINIMAL, ssm=True)
        captures = manifest["captures"]
        self.assertEqual(len(captures), 2)
        for capture in captures:
            self.assertIn("state_uncertain", capture)
            self.assertIn("absence of this warning is not a guarantee",
                          capture["state_uncertain"])
        self.assertTrue(any(a["kind"] == "ssm" for a in manifest["approximations"]))

    def test_every_ssm_capture_is_labelled_approximate(self):
        self.publish_at[2] = 0x0101
        manifest = self._run(MINIMAL, ssm=True)
        self.assertEqual(manifest["captures"][0]["capture_alignment"], "approximate")

    def test_markers_many_frames_apart_are_not_flagged(self):
        # Two markers a second apart are each servable, so neither capture
        # carries the paired-marker caveat.
        self.publish_at[2] = (0x0201, 10)
        self.publish_at[3] = (0x0202, 60)
        manifest = self._run(MINIMAL, ssm=True)
        self.assertEqual(len(manifest["captures"]), 2)
        for capture in manifest["captures"]:
            self.assertNotIn("state_uncertain", capture)

    def test_every_ssm_capture_states_what_it_actually_is(self):
        self.publish_at[2] = ssm_ring.CODE_SCREENSHOT
        manifest = self._run(MINIMAL, ssm=True)
        self.assertIn("not the image the standard specifies",
                      manifest["captures"][0]["capture_semantics"])

    def test_wait_ssm0000_is_refused_unless_the_detector_is_on(self):
        with self.assertRaises(CslError) as ctx:
            self._run(MINIMAL + "wait_ssm0000\n")
        self.assertIn("--ssm", ctx.exception.reason)

    # --- wait_ssm0000: one-shot consumption, ordered by the ring -------------
    #
    # This script has no `wait` before the sync wait, so the only ring polls
    # that can happen before the wait is entered are the command-boundary
    # drains. A publish scheduled well past those is unambiguously "after
    # entry", and one scheduled on the first poll is unambiguously "before".

    SYNC_SCRIPT = (
        "csl_version 1.4\ncrtc_select 1\nreset\n"
        "disk_insert 'shaker26.dsk'\nwait_ssm0000\n"
    )

    def test_wait_ssm0000_consumes_a_sync_that_arrived_before_it_was_entered(self):
        # CSL does not settle this case. The runner selects one-shot
        # consumption because the host is asynchronous and routinely arrives
        # after the core has already executed the sequence.
        self.publish_at[1] = ssm_ring.CODE_SYNC
        manifest = self._run(self.SYNC_SCRIPT, ssm=True, max_wait_seconds=30.0)
        released = [e for e in manifest["trace"] if e["outcome"] == "ssm_sync_released"]
        self.assertEqual(len(released), 1)
        self.assertIn("before entry", released[0]["released_by"])
        self.assertEqual(released[0]["polls_seconds"], 0.0)

    def test_wait_ssm0000_waits_for_a_sync_that_arrives_after_entry(self):
        self.publish_at[10] = ssm_ring.CODE_SYNC
        manifest = self._run(self.SYNC_SCRIPT, ssm=True, max_wait_seconds=30.0)
        released = [e for e in manifest["trace"] if e["outcome"] == "ssm_sync_released"]
        self.assertEqual(len(released), 1)
        self.assertIn("after entry", released[0]["released_by"])
        self.assertGreater(released[0]["polls_seconds"], 0)

    def test_each_sync_releases_exactly_one_wait(self):
        # One marker cannot pace two waits: the second has nothing to consume
        # and must time out rather than sail through on the same event.
        self.publish_at[1] = ssm_ring.CODE_SYNC
        with self.assertRaises(CslError) as ctx:
            self._run(self.SYNC_SCRIPT + "wait_ssm0000\n", ssm=True,
                      max_wait_seconds=2.0)
        self.assertIn("no SSM #0000 arrived", ctx.exception.reason)

    def test_wait_ssm0000_gives_up_at_the_bound_instead_of_hanging(self):
        with self.assertRaises(CslError) as ctx:
            self._run(self.SYNC_SCRIPT, ssm=True, max_wait_seconds=2.0)
        self.assertIn("no SSM #0000 arrived", ctx.exception.reason)

    def test_wait_ssm0000_fails_boundedly_if_marker_observed_after_monotonic_deadline(self):
        # A poll that completes past the monotonic deadline must fail boundedly
        # rather than reporting success.
        clock = {"t": 0.0}

        def advance_time():
            clock["t"] += 0.01
            return clock["t"]

        def slow_serve(cmd: str) -> CommandResult:
            res = self._serve_ring(cmd)
            # Advance clock past deadline during the poll
            clock["t"] += 10.0
            return res

        self.publish_at[2] = ssm_ring.CODE_SYNC
        self.transport.handlers[0] = (
            lambda c: "if=/dev/mem" in c,
            slow_serve,
        )
        script = write(self.tmp, "m.csl", self.SYNC_SCRIPT)
        with self.assertRaises(CslError) as ctx:
            run_csl(script, options(self.tmp / "out", ssm=True, max_wait_seconds=2.0),
                    transport=self.transport, dry_run=False, sleep_fn=lambda s: None,
                    time_fn=advance_time)
        self.assertIn("deadline", ctx.exception.reason)

    # --- startup and read failures -------------------------------------------

    def test_the_startup_header_is_observed_before_the_script_sends_input(self):
        manifest = self._run(MINIMAL, ssm=True)
        startup = manifest["ssm_startup"]
        self.assertTrue(startup["attempted"])
        self.assertEqual(startup["observed"], "zero header")
        self.assertEqual(startup["written_at_start"], 0)
        self.assertIn("not a session identity", startup["guarantee"])
        # The observation happens before the first key, which is the last
        # moment the state is unambiguous.
        outcomes = [e["outcome"] for e in manifest["trace"]]
        self.assertLess(outcomes.index("ssm_startup_observed"),
                        outcomes.index("keys_sent"))

    def test_zero_header_returned_after_startup_deadline_is_rejected(self):
        clock = {"t": 0.0}

        def slow_header(cmd):
            reply = self._serve_ring(cmd)
            clock["t"] += 2.0
            return reply

        self.transport.handlers[0] = (lambda c: "if=/dev/mem" in c, slow_header)
        backend = DeviceBackend(
            self.transport, options(self.tmp / "out", ssm=True, ssm_startup_seconds=1.0),
            self.tmp / "out", time_fn=lambda: clock["t"], sleep_fn=lambda _: None)
        with self.assertRaises(DriverError):
            backend.ensure_ssm_started()
        self.assertFalse(backend.ssm_ready)

    def test_a_ring_that_is_already_counting_at_startup_is_flagged(self):
        self.publish(0x0900)          # a record exists before the host looks
        with self.assertRaises(DriverError) as ctx:
            self._run(MINIMAL, ssm=True)
        self.assertIn("non-zero", str(ctx.exception))

    def test_an_unwritten_ring_is_reported_rather_than_read_as_no_marker(self):
        # A window with no header used to look exactly like "no marker yet".
        # It is now a bounded startup case that becomes a real error.
        self.transport.handlers.insert(0, (
            lambda c: "if=/dev/mem" in c,
            lambda c: CommandResult(0, base64.b64encode(b"\x00" * 1040).decode(), ""),
        ))
        with self.assertRaises(DriverError) as ctx:
            self._run(MINIMAL, ssm=True, ssm_startup_seconds=0.5)
        self.assertIn("header", str(ctx.exception))

    def test_a_failing_device_read_stops_the_run(self):
        self.transport.handlers.insert(0, (
            lambda c: "if=/dev/mem" in c,
            lambda c: CommandResult(1, "", "dd: /dev/mem: Permission denied"),
        ))
        with self.assertRaises(DriverError) as ctx:
            self._run(MINIMAL, ssm=True, ssm_startup_seconds=0.5)
        self.assertIn("Permission denied", str(ctx.exception))

    # --- naming ---------------------------------------------------------------

    def test_an_ordinary_code_does_not_consume_a_pending_csl_name(self):
        # SSM v1.1 gives screenshot_name to #FFFE only. Letting an ordinary
        # code eat it renames the wrong capture and leaves the #FFFE unnamed.
        self.publish_at[2] = 0x0123
        self.publish_at[3] = ssm_ring.CODE_SCREENSHOT
        manifest = self._run("screenshot_name 'flash_state'\n" + MINIMAL, ssm=True)
        self.assertEqual([c["name"] for c in manifest["captures"]],
                         ["MISTER_1_0123.png", "flash_state.png"])

    # --- draining --------------------------------------------------------------

    def test_a_marker_published_at_the_very_end_is_still_collected(self):
        # The last command's marker belongs to this run. Without a final drain
        # the walk would silently end one screen early.
        # With polling suppressed until startup is established at the first
        # machine touch, poll 1 is startup/key_output, poll 2 is post-key_output,
        # and poll 3 is the final drain.
        self.publish_at[3] = 0x0777
        manifest = self._run(
            "csl_version 1.4\ncrtc_select 1\nreset\ndisk_insert 'shaker26.dsk'\n"
            "key_delay 70000 70000\nkey_output 'A'\n",
            ssm=True)
        self.assertEqual([c["name"] for c in manifest["captures"]], ["MISTER_1_0777.png"])


if __name__ == "__main__":
    unittest.main()
