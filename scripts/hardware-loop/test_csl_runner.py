"""Offline tests for the CSL runner (backlog B4, phase 0).

Deterministic and network-free.  The corpus tests read the bundled Logon System
scripts under ``docs/references/Shaker_CSL``; they skip when that directory is
absent, because the bundle is user-owned and untracked.

The cross-checks worth the most here are the ones that can fail for a reason we
do not already know:

* the keycode table is derived from ``rtl/hid.sv``, and the French
  ``RUN"SHAKE27B`` translation is compared against the sequence actually typed
  on the device in ``docs/b2-device-capture-2026-09-12.md``;
* the CFG bit arithmetic is compared against the 16 bytes read off that same
  device, where bit 22 is known to be the Right Shift option;
* every keystroke the runner would send for the whole 25-script corpus is fed
  back through ``driver.validate_raw_seq``, MBC's pinned parser contract.
"""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from typing import Any, Dict, List

import cpc_keys
from cpc_keys import KeyTranslationError, sequence_tokens, translate_text
from csl_runner import (
    CFG_BIT_CRTC,
    CslError,
    PlanBackend,
    RunOptions,
    _cfg_apply_bits,
    load_program,
    parse_script,
    run_csl,
)
from driver import CommandResult, DriverError, validate_raw_seq
from test_driver import ScriptedTransport, create_minimal_png_bytes

SHAKER_DIR = Path(__file__).resolve().parents[2] / "docs" / "references" / "Shaker_CSL"
RBF = "/media/fat/_Computer/Amstrad_test.rbf"
DISK_DIR = "/media/fat/games/Amstrad/dsk"


def options(tmp: Path, **kwargs: Any) -> RunOptions:
    params: Dict[str, Any] = dict(
        rbf_path=RBF, disk_dir=DISK_DIR, layout="fr", out_dir=tmp,
        max_wait_seconds=1e9, follow_loads=False,
    )
    params.update(kwargs)
    return RunOptions(**params)


def plan(script: Path, tmp: Path, **kwargs: Any) -> Dict[str, Any]:
    return run_csl(script, options(tmp, **kwargs), dry_run=True)


def write(tmp: Path, name: str, body: str) -> Path:
    path = tmp / name
    path.write_text(body, encoding="utf-8")
    return path


MINIMAL = """csl_version 1.4
crtc_select 1
reset
wait 1000000
disk_insert 'shaker26.dsk'
key_delay 70000 70000 400000
key_output 'RUN"SHAKE26B"\\(RET)'
wait 2000000
"""


class TestHardwareCrossChecks(unittest.TestCase):
    """Pin the derived tables against evidence from the real device."""

    def test_french_run_sequence_matches_b2_device_capture(self):
        # docs/b2-device-capture-2026-09-12.md, "French SHAKER 2.7 launch":
        # this exact sequence typed RUN"SHAKE27B on the device's French ROM.
        expected = ":13:16:31:04:1F:23:10:25:12{2A:03:08}2A:30"
        self.assertEqual(sequence_tokens(translate_text('RUN"SHAKE27B', "fr")), expected)

    def test_menu_digit_reaches_the_same_cpc_key_as_the_device_capture(self):
        # The same document selects SHAKER module B test 9 with bare :0A.
        # Under the French ROM the character '9' also needs SHIFT, so the
        # runner holds it; the CPC key underneath must still be 0A.
        tokens = sequence_tokens(translate_text("9", "fr"))
        self.assertIn(":0A", tokens)
        self.assertTrue(tokens.startswith("{2A") and tokens.endswith("}2A"))

    def test_cfg_bit_arithmetic_against_the_device_cfg(self):
        # The device's saved Amstrad.CFG is 00004000...; bit 22 (Right Shift as
        # Shift) is byte 2 bit 6.  Setting the CRTC bit must not disturb it.
        original = bytes.fromhex("00004000000000000000000000000000")
        self.assertEqual(original[2], 0x40)
        crtc0 = _cfg_apply_bits(original, {CFG_BIT_CRTC: 1})
        self.assertEqual(crtc0[0], 0x04)
        self.assertEqual(crtc0[2], 0x40)
        self.assertEqual(_cfg_apply_bits(crtc0, {CFG_BIT_CRTC: 0}), original)

    def test_cfg_rejects_wrong_size_and_out_of_range_bits(self):
        with self.assertRaises(DriverError):
            _cfg_apply_bits(b"\x00" * 8, {0: 1})
        with self.assertRaises(DriverError):
            _cfg_apply_bits(b"\x00" * 16, {128: 1})


class TestKeyTranslation(unittest.TestCase):
    def test_layouts_disagree_where_the_rom_does(self):
        # 'A' sits on the CPC Q position under a French ROM, and digits swap
        # shift state between the two layouts.
        self.assertEqual(sequence_tokens(translate_text("A", "fr")), ":10")
        self.assertEqual(sequence_tokens(translate_text("A", "uk")), ":1E")
        self.assertEqual(sequence_tokens(translate_text("2", "uk")), ":03")
        self.assertEqual(sequence_tokens(translate_text("2", "fr")), "{2A:03}2A")
        self.assertEqual(sequence_tokens(translate_text('"', "fr")), ":04")
        self.assertEqual(sequence_tokens(translate_text('"', "uk")), "{2A:03}2A")

    def test_chord_overlaps_keys_and_releases_in_reverse(self):
        tokens = sequence_tokens(translate_text(r"{\(SHI)1}", "uk"))
        self.assertEqual(tokens, "{2A{02}02}2A")
        validate_raw_seq(tokens)

    def test_shift_is_held_across_a_run_rather_than_retapped(self):
        self.assertEqual(sequence_tokens(translate_text("27", "fr")), "{2A:03:08}2A")

    def test_kof_marks_the_preceding_group_and_is_not_a_key(self):
        groups = translate_text(r"A\(KOF)B", "uk")
        self.assertEqual(len(groups), 2)
        self.assertTrue(groups[0].kof)
        self.assertFalse(groups[1].kof)

    def test_unmappable_input_is_refused_not_skipped(self):
        # CSL permits silently skipping unknown characters.  A skipped key
        # leaves SHAKER on the wrong screen, so this runner refuses instead.
        for text, layout in ((chr(0x263A), "uk"), ("a", "uk"), ("§", "uk"), (r"\(XYZ)", "uk")):
            with self.assertRaises(KeyTranslationError):
                translate_text(text, layout)

    def test_malformed_escapes_and_chords_are_refused(self):
        for text in (r"\A", r"\(RET", "{A", "A}", "{{A}", "{}"):
            with self.assertRaises(KeyTranslationError):
                translate_text(text, "uk")

    def test_every_cpc_key_position_has_a_distinct_keycode(self):
        codes = list(cpc_keys.CPC_KEY_TO_LINUX.values())
        self.assertEqual(len(codes), len(set(codes)))

    def test_every_csl_special_key_resolves_to_a_known_position(self):
        for name, position in cpc_keys.CSL_SPECIAL_KEYS.items():
            self.assertIn(position, cpc_keys.CPC_KEY_TO_LINUX, name)


class TestParser(unittest.TestCase):
    def test_comments_blank_lines_and_quoting(self):
        commands = parse_script(
            "; header\n\n  csl_version 1.4  ; trailing\n"
            "disk_insert B 'My Disk.dsk'\nkey_delay 70000 70000 400000\n",
            "x.csl",
        )
        self.assertEqual([c.name for c in commands], ["csl_version", "disk_insert", "key_delay"])
        self.assertEqual(commands[1].args, ["B", "My Disk.dsk"])
        self.assertEqual(commands[1].line, 4)
        self.assertEqual(commands[2].args, ["70000", "70000", "400000"])

    def test_unterminated_quote_reports_the_line(self):
        with self.assertRaises(CslError) as ctx:
            parse_script("reset\nkey_output 'oops\n", "x.csl")
        self.assertEqual(ctx.exception.line, 2)

    def test_unknown_instruction_parses_but_is_rejected_at_run_time(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "x.csl", "reset\nnot_a_csl_command 1\n")
            self.assertEqual(len(parse_script(script.read_text(), "x.csl")), 2)
            with self.assertRaises(CslError) as ctx:
                plan(script, tmp / "out")
            self.assertIn("unknown CSL instruction", ctx.exception.reason)


class TestCslLoad(unittest.TestCase):
    def test_chain_is_flattened_in_order(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            write(tmp, "B.CSL", "wait 1\n")
            entry = write(tmp, "A.CSL", "wait 2\ncsl_load 'B'\n")
            names = [c.script for c in load_program(entry)]
            self.assertEqual(names, ["A.CSL", "A.CSL", "B.CSL"])

    def test_cycle_is_detected(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            write(tmp, "A.CSL", "csl_load 'B'\n")
            write(tmp, "B.CSL", "csl_load 'A'\n")
            with self.assertRaises(CslError) as ctx:
                load_program(tmp / "A.CSL")
            self.assertIn("cycle", ctx.exception.reason)

    def test_self_reference_is_a_cycle(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            entry = write(tmp, "A.CSL", "csl_load 'A'\n")
            with self.assertRaises(CslError):
                load_program(entry)

    def test_missing_target_and_path_arguments_are_refused(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            write(tmp, "A.CSL", "csl_load 'Nope'\n")
            with self.assertRaises(CslError):
                load_program(tmp / "A.CSL")
            write(tmp, "C.CSL", "csl_load '../escape'\n")
            with self.assertRaises(CslError):
                load_program(tmp / "C.CSL")


class TestRejectionReporting(unittest.TestCase):
    def test_rejection_carries_all_six_standard_fields(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "r.csl", "csl_version 1.4\ncrtc_select 2\nreset\n")
            with self.assertRaises(CslError) as ctx:
                plan(script, tmp / "out")
            fields = ctx.exception.as_dict()
            self.assertEqual(fields["script"], "r.csl")
            self.assertEqual(fields["line"], 2)
            self.assertEqual(fields["instruction"], "crtc_select 2")
            self.assertIn("not implemented", fields["reason"])
            self.assertEqual(fields["script_version"], "1.4")
            self.assertEqual(fields["supported_version"], "1.4")

    def test_undeclared_version_is_reported_as_such(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "r.csl", "crtc_select 3\n")
            with self.assertRaises(CslError) as ctx:
                plan(script, tmp / "out")
            self.assertEqual(ctx.exception.as_dict()["script_version"], "(not declared)")

    def test_every_unsupported_command_stops_the_script(self):
        for command in ("reset soft", "keyboard_write 255,255", "wait_ssm0000",
                        "wait_vsyncoffon", "tape_play", "snapshot", "gate_array 40010",
                        "key_from_file 'x.txt'", "screenshot_dir 'c:\\out\\'",
                        "disk_dir 'c:\\dsk\\'", "cpc_model 6"):
            with tempfile.TemporaryDirectory() as td:
                tmp = Path(td)
                script = write(tmp, "r.csl", f"reset\nwait 1\n{command}\n")
                with self.assertRaises(CslError, msg=command) as ctx:
                    plan(script, tmp / "out")
                self.assertTrue(ctx.exception.reason, command)

    def test_screenshot_vsync_is_refused_with_the_asynchronous_capture_reason(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "r.csl", "reset\nwait 1\nkey_output ' '\nscreenshot vsync\n")
            with self.assertRaises(CslError) as ctx:
                plan(script, tmp / "out")
            self.assertIn("asynchronously", ctx.exception.reason)

    def test_failure_still_writes_the_manifest_and_last_run_log(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            out = tmp / "out"
            script = write(tmp, "r.csl", "csl_version 1.4\nreset\nwait 1\ncrtc_select 2\n")
            with self.assertRaises(CslError):
                plan(script, out)
            manifest = json.loads((out / "manifest.json").read_text())
            self.assertEqual(manifest["status"], "failed")
            self.assertEqual(manifest["error"]["line"], 4)
            # The partial trace before the rejection is the run's evidence.
            self.assertEqual(manifest["trace"][0]["outcome"], "version_recorded")
            # crtc_select binds to the reset it configures, so an unsupported
            # CRTC stops the script before the core is ever loaded.
            self.assertFalse(any(e["outcome"] == "load_core" for e in manifest["trace"]))
            self.assertIn("crtc_select 2", (out / "last-run.log").read_text())


class TestPowerOnFold(unittest.TestCase):
    def test_disk_insert_after_reset_is_hoisted_into_the_load(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", MINIMAL)
            manifest = plan(script, tmp / "out")
            loads = [a for a in manifest["actions"] if a["action"] == "load_core"]
            self.assertEqual(len(loads), 1)
            self.assertEqual(loads[0]["media_path"], f"{DISK_DIR}/shaker26.dsk")
            self.assertEqual(loads[0]["crtc_status_bit"], 0)
            self.assertTrue(any(a["kind"] == "ordering" for a in manifest["approximations"]))
            # The load happens before the script's post-reset wait.
            kinds = [a["action"] for a in manifest["actions"]]
            self.assertLess(kinds.index("load_core"), kinds.index("sleep"))

    def test_drive_b_selects_the_second_slot(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", "reset\ndisk_insert B 'x.dsk'\nwait 1\n")
            manifest = plan(script, tmp / "out")
            self.assertEqual(manifest["actions"][0]["media_slot"], "S1")

    def test_live_crtc_change_to_the_same_type_is_a_no_op(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", MINIMAL + "crtc_select 1B\nwait 1\n")
            manifest = plan(script, tmp / "out")
            self.assertEqual(len([a for a in manifest["actions"] if a["action"] == "load_core"]), 1)
            self.assertTrue(any("no-op" in a["detail"] for a in manifest["approximations"]))

    def test_live_crtc_change_to_a_different_type_is_refused(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", MINIMAL + "crtc_select 0\nwait 1\n")
            with self.assertRaises(CslError) as ctx:
                plan(script, tmp / "out")
            self.assertIn("OSD", ctx.exception.reason)

    def test_input_before_a_reset_is_refused(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", "key_output ' '\n")
            with self.assertRaises(CslError) as ctx:
                plan(script, tmp / "out")
            self.assertIn("powered on", ctx.exception.reason)


class TestTiming(unittest.TestCase):
    def test_key_delay_maps_onto_the_single_mbc_knob(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", MINIMAL)
            manifest = plan(script, tmp / "out")
            keys = [a for a in manifest["actions"] if a["action"] == "send_keys"]
            self.assertEqual(keys[0]["mbc_key_wait_ms"], 70)
            # 70000/70000 is expressible exactly, so no key_delay approximation.
            self.assertFalse([a for a in manifest["approximations"] if a["kind"] == "key_delay"])

    def test_split_press_and_gap_delays_are_recorded_as_an_approximation(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", "reset\nkey_delay 10000 90000\nkey_output 'A'\n")
            manifest = plan(script, tmp / "out")
            keys = [a for a in manifest["actions"] if a["action"] == "send_keys"]
            self.assertEqual(keys[0]["mbc_key_wait_ms"], 90)
            notes = [a for a in manifest["approximations"] if a["kind"] == "key_delay"]
            self.assertTrue(notes and "one delay knob" in notes[0]["detail"])

    def test_after_cr_delay_splits_the_sequence_and_sleeps_on_the_host(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl",
                           "reset\nkey_delay 70000 70000 400000\nkey_output 'A\\(RET)B'\n")
            actions = plan(script, tmp / "out")["actions"]
            sends = [a for a in actions if a["action"] == "send_keys"]
            self.assertEqual([a["raw_seq"] for a in sends], [":10:1C", ":30"])
            after = [a for a in actions if a.get("reason") == "key_delay after CR"]
            self.assertEqual(after[0]["seconds"], 0.4)

    def test_wait_is_a_host_sleep_of_the_emulated_microseconds(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", "reset\nwait 1300455\n")
            sleeps = [a for a in plan(script, tmp / "out")["actions"] if a["action"] == "sleep"]
            self.assertAlmostEqual(sleeps[0]["seconds"], 1.300455)

    def test_an_unbounded_wait_is_refused_rather_than_hanging_the_run(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", "reset\nwait 100000000\n")
            with self.assertRaises(CslError) as ctx:
                run_csl(script, options(tmp / "out", max_wait_seconds=30.0), dry_run=True)
            self.assertIn("--max-wait", ctx.exception.reason)


class TestOperatorControls(unittest.TestCase):
    def test_stop_at_line_truncates_the_walk(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", MINIMAL)
            manifest = run_csl(script, options(tmp / "out", stop_at=("m.csl", 4)), dry_run=True)
            self.assertEqual(manifest["trace"][-1]["outcome"], "stopped_at_requested_line")
            self.assertEqual(manifest["trace"][-1]["line"], 4)

    def test_screenshot_at_line_captures_without_editing_the_script(self):
        # No SHAKER script contains a screenshot instruction; their captures
        # come from SSM #FFFE, which arrives with the phase 1 detector.
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            script = write(tmp, "m.csl", MINIMAL)
            manifest = run_csl(script, options(tmp / "out", screenshot_at=[("m.csl", 8)]),
                               dry_run=True)
            shots = [a for a in manifest["actions"] if a["action"] == "screenshot"]
            self.assertEqual(len(shots), 1)
            self.assertTrue(shots[0]["name"].startswith("MISTER_1_m_0008_"))

    def test_dry_run_touches_no_transport_and_still_writes_evidence(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            out = tmp / "out"
            script = write(tmp, "m.csl", MINIMAL)
            manifest = run_csl(script, options(out), transport=None, dry_run=True)
            self.assertFalse(manifest["hardware_contacted"])
            self.assertEqual(manifest["status"], "planned")
            self.assertTrue((out / "manifest.json").is_file())
            self.assertTrue((out / "last-run.log").is_file())

    def test_existing_manifest_is_never_overwritten(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            out = tmp / "out"
            script = write(tmp, "m.csl", MINIMAL)
            plan(script, out)
            with self.assertRaises(ValueError):
                plan(script, out)


class DeviceHarnessMixin:
    """Scripted-transport stand-in for the device: no network, no hardware."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.cfg = bytes.fromhex("00004000000000000000000000000000")
        self.transport = ScriptedTransport()
        self.written: List[bytes] = []
        t = self.transport

        t.register_handler(lambda c: c.startswith("base64 <"),
                           lambda c: CommandResult(0, __import__("base64").b64encode(self.cfg).decode(), ""))
        t.register_handler(lambda c: c.startswith("cat /media/fat/csl_cfg_"),
                           self._capture_cfg_write)
        t.register_handler(lambda c: c.startswith("sha256sum"), self._sha)
        t.register_handler(lambda c: c.startswith("ls -1"),
                           lambda c: CommandResult(0, "Amstrad_test.rbf\n", ""))
        t.register_handler(lambda c: c.startswith("test ! -e"), lambda c: CommandResult(0, "", ""))
        t.register_handler(lambda c: c.startswith("test -f /media/fat/screenshots/"),
                           self._screenshot_ready)
        self.screenshot_polls = 0

    def _capture_cfg_write(self, cmd: str) -> CommandResult:
        source = cmd.split()[1]
        self.written.append(self.transport.files[source])
        self.cfg = self.transport.files[source]
        return CommandResult(0, "", "")

    def _sha(self, cmd: str) -> CommandResult:
        import hashlib
        path = cmd.split()[-1]
        if path == "/media/fat/config/Amstrad.CFG":
            data = self.cfg
        else:
            data = path.encode()
        return CommandResult(0, f"{hashlib.sha256(data).hexdigest()}  {path}\n", "")

    def _screenshot_ready(self, cmd: str) -> CommandResult:
        remote = cmd.split()[-1]
        self.transport.files[remote] = create_minimal_png_bytes(384, 272)
        return CommandResult(0, "", "")

    def _run(self, body: str, **kwargs: Any) -> Dict[str, Any]:
        script = write(self.tmp, "m.csl", body)
        return run_csl(script, options(self.tmp / "out", **kwargs), transport=self.transport,
                       dry_run=False, sleep_fn=lambda s: None, time_fn=self._clock())

    def _clock(self):
        state = {"t": 0.0}

        def now() -> float:
            state["t"] += 0.01
            return state["t"]

        return now


class TestDeviceBackend(DeviceHarnessMixin, unittest.TestCase):
    """Live path against a scripted transport: no network, no real device."""

    def test_cfg_is_changed_by_bit_and_restored_afterwards(self):
        manifest = self._run(MINIMAL)
        self.assertEqual(manifest["status"], "success")
        load = manifest["actions"][0]
        # Bit 37 is written explicitly even with SSM off, so a detector left
        # on by an earlier run cannot leak into this one.
        self.assertEqual(load["cfg_bits_changed"], {"2": 0, "37": 0})
        self.assertIn("not visually confirmed", load["configuration_evidence"])
        # CRTC 1 means bit 2 clear, which this CFG already had: the written
        # bytes must equal the original, and the restore must put them back.
        self.assertEqual(self.written[0], bytes.fromhex("00004000000000000000000000000000"))
        self.assertTrue(manifest["cleanup"]["cfg_restore"]["matches_original"])
        self.assertEqual(self.cfg, bytes.fromhex("00004000000000000000000000000000"))

    def test_crtc_0_sets_the_bit_and_leaves_other_options_alone(self):
        self._run(MINIMAL.replace("crtc_select 1", "crtc_select 0"))
        self.assertEqual(self.written[0][0], 0x04)
        self.assertEqual(self.written[0][2], 0x40)

    def test_absent_cfg_stops_before_anything_is_changed(self):
        self.transport.register_handler(
            lambda c: c == "test -f /media/fat/config/Amstrad.CFG",
            lambda c: CommandResult(1, "", ""))
        self.transport.handlers.insert(0, self.transport.handlers.pop())
        with self.assertRaises(DriverError) as ctx:
            self._run(MINIMAL)
        self.assertIn("never creates a CFG", str(ctx.exception))
        self.assertEqual(self.written, [])

    def test_mbc_is_invoked_with_the_script_derived_key_wait(self):
        self._run(MINIMAL)
        mbc = [e["command"] for e in self.transport.command_log if "raw_seq" in e["command"]]
        self.assertTrue(mbc)
        self.assertIn("MBC_KEY_WAIT=70", mbc[0])
        self.assertIn("MBC_SEQUENCE_WAIT=1000", mbc[0])

    def test_pinned_rbf_hash_mismatch_stops_before_the_core_loads(self):
        with self.assertRaises(DriverError) as ctx:
            self._run(MINIMAL, expect_sha256={"rbf": "0" * 64})
        self.assertIn("SHA-256 mismatch", str(ctx.exception))
        loaded = [e for e in self.transport.command_log if "load_core" in e["command"]]
        self.assertEqual(loaded, [])

    def test_capture_is_downloaded_verified_and_hashed(self):
        manifest = self._run(MINIMAL, screenshot_at=[("m.csl", 8)])
        capture = manifest["captures"][0]
        self.assertEqual(capture["status"], "completed")
        self.assertEqual(capture["dimensions"], [384, 272])
        self.assertEqual(len(capture["sha256"]), 64)
        self.assertTrue(Path(capture["local_path"]).is_file())

    def test_cleanup_removes_every_temporary_device_file(self):
        manifest = self._run(MINIMAL)
        removed = {e["path"] for e in manifest["cleanup"]["remote_temp"]}
        uploaded = {k for k in self.transport.files if k.startswith("/media/fat/csl_")}
        self.assertTrue(uploaded)
        self.assertTrue(uploaded <= removed)


@unittest.skipUnless(SHAKER_DIR.is_dir(), "Shaker_CSL bundle is user-owned and untracked")
class TestBundledCorpus(unittest.TestCase):
    """The whole author-supplied corpus, in both supported ROM layouts."""

    @classmethod
    def setUpClass(cls):
        cls.scripts = sorted(SHAKER_DIR.glob("MODULE_*/*.CSL"))

    def test_the_bundle_is_the_expected_shape(self):
        self.assertEqual(len(self.scripts), 25)

    def test_every_script_parses(self):
        for script in self.scripts:
            parse_script(script.read_text(encoding="utf-8"), script.name)

    def test_only_the_crtc_2_3_4_scripts_are_rejected_and_only_for_the_crtc(self):
        rejected, planned = {}, []
        for layout in cpc_keys.SUPPORTED_LAYOUTS:
            for script in self.scripts:
                with tempfile.TemporaryDirectory() as td:
                    try:
                        plan(script, Path(td), layout=layout)
                    except CslError as exc:
                        rejected[(layout, script.name)] = exc
                    else:
                        planned.append((layout, script.name))
        for (layout, name), exc in rejected.items():
            self.assertTrue(name.endswith(("-2.CSL", "-3.CSL", "-4.CSL")), f"{layout}/{name}")
            self.assertEqual(exc.instruction.split()[0], "crtc_select", f"{layout}/{name}")
            self.assertIn("not implemented in this core", exc.reason)
        self.assertEqual(len(rejected), 30)
        self.assertEqual(len(planned), 20)

    def test_every_keystroke_the_corpus_needs_is_mappable_in_both_layouts(self):
        texts = set()
        for script in self.scripts:
            for command in parse_script(script.read_text(encoding="utf-8"), script.name):
                if command.name == "key_output" and command.args:
                    texts.add(command.args[0])
        self.assertTrue(texts)
        for layout in cpc_keys.SUPPORTED_LAYOUTS:
            report = cpc_keys.coverage_report(sorted(texts), layout)
            self.assertEqual(report["unmappable"], [], layout)

    def test_every_generated_sequence_satisfies_the_pinned_mbc_parser(self):
        for layout in cpc_keys.SUPPORTED_LAYOUTS:
            for script in self.scripts:
                if script.name.endswith(("-2.CSL", "-3.CSL", "-4.CSL")):
                    continue
                with tempfile.TemporaryDirectory() as td:
                    manifest = plan(script, Path(td), layout=layout)
                for action in manifest["actions"]:
                    if action["action"] == "send_keys":
                        validate_raw_seq(action["raw_seq"])

    def test_the_chain_of_a_module_resolves_and_stops_at_the_unsupported_crtc(self):
        entry = SHAKER_DIR / "MODULE_B" / "SHAKE26B-0.CSL"
        program = load_program(entry)
        self.assertEqual(sorted({c.script for c in program}),
                         [f"SHAKE26B-{n}.CSL" for n in range(5)])
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(CslError) as ctx:
                run_csl(entry, options(Path(td), follow_loads=True), dry_run=True)
        self.assertEqual(ctx.exception.script, "SHAKE26B-2.CSL")


if __name__ == "__main__":
    unittest.main()
