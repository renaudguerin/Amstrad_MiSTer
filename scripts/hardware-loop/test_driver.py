"""Integration boundary tests for MiSTer hardware test loop host driver.

Tests run entirely in-memory using a scripted transport and synthetic PNG fixtures.
Monotonic time and sleep are injected for deterministic timeout and polling tests.
"""

from __future__ import annotations

import io
import os
import subprocess
from unittest.mock import patch
import json
import shutil
import shlex
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional
from PIL import Image

from driver import (
    CommandResult,
    DriverError,
    SSHTransport,
    check_rbf_directory,
    emergency_release_keys,
    generate_mgl_xml,
    run_hardware_loop,
    validate_case_config,
    validate_device_path,
    validate_port,
    validate_raw_seq,
    validate_screenshot_name,
    validate_ssh_target,
    verify_png,
)


def create_minimal_png_bytes(width: int = 320, height: int = 240, color: tuple[int, int, int] = (0, 128, 255)) -> bytes:
    """Create valid PNG bytes in memory for tests."""
    img = Image.new("RGB", (width, height), color)
    buf = io.BytesIO()
    img.save(buf, format="PNG")
    return buf.getvalue()


class ScriptedTransport:
    """Scriptable in-memory transport for deterministic testing."""

    def __init__(self):
        self.files: Dict[str, bytes] = {}
        self.command_log: List[Dict[str, Any]] = []
        self.handlers: List[tuple[Callable[[str], bool], Callable[[str], CommandResult]]] = []
        self.default_exit_code: int = 0
        self.default_stdout: str = ""
        self.default_stderr: str = ""

    def register_handler(self, predicate: Callable[[str], bool], handler: Callable[[str], CommandResult]) -> None:
        self.handlers.append((predicate, handler))

    def run_cmd(self, cmd: str, timeout: float) -> CommandResult:
        res = None
        for pred, handler in self.handlers:
            if pred(cmd):
                res = handler(cmd)
                break

        if res is None:
            res = CommandResult(self.default_exit_code, self.default_stdout, self.default_stderr)

        self.command_log.append({
            "command": cmd,
            "timestamp": "2026-09-08T00:00:00Z",
            "duration_ms": 1.0,
            "exit_code": res.exit_code,
            "stdout": res.stdout,
            "stderr": res.stderr,
        })
        if res.exit_code == 255:
            raise DriverError(f"SSH connection failed (code 255): {res.stderr}")
        return res

    def upload_file(self, local_path: Path, remote_path: str, timeout: float) -> None:
        self.command_log.append({
            "command": f"scp {local_path} -> {remote_path}",
            "timestamp": "2026-09-08T00:00:00Z",
            "duration_ms": 1.0,
            "exit_code": 0,
            "stdout": "",
            "stderr": "",
        })
        self.files[remote_path] = local_path.read_bytes()

    def download_file(self, remote_path: str, local_path: Path, timeout: float) -> None:
        self.command_log.append({
            "command": f"scp {remote_path} -> {local_path}",
            "timestamp": "2026-09-08T00:00:00Z",
            "duration_ms": 1.0,
            "exit_code": 0 if remote_path in self.files else 1,
            "stdout": "",
            "stderr": "" if remote_path in self.files else "File not found",
        })
        if remote_path not in self.files:
            raise DriverError(f"Remote file {remote_path} not found")
        local_path.write_bytes(self.files[remote_path])


class TestHardwareLoopContracts(unittest.TestCase):
    """Test critical integration boundary contracts."""

    def setUp(self):
        self.temp_dir = Path(tempfile.mkdtemp())
        self.out_dir = self.temp_dir / "output"
        self.png_bytes = create_minimal_png_bytes(384, 272)
        self.valid_sha = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

        self.case_dict = {
            "case_id": "test_case",
            "rbf_path": "/media/fat/_Computer/Amstrad.rbf",
            "media": {
                "type": "dsk",
                "slot": "S0",
                "path": "/media/fat/games/Amstrad/shaker.dsk",
            },
            "declared_settings": {"model": "CPC 6128"},
            "boot_delay": 0.01,
            "settle_delay": 0.01,
            "capture_delay": 0.01,
            "captures_count": 3,
            "input": {"raw_seq": "{1C}1C"},
            "timeouts": {
                "cmd_timeout": 5.0,
                "capture_timeout": 5.0,
                "poll_interval": 0.01,
                "max_retries": 3,
            },
            "ack_main_cmd": True,
            "mbc_path": "mbc",
        }
        self.config = validate_case_config(self.case_dict)

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def _setup_working_device(self, transport: ScriptedTransport):
        """Set up standard successful responses for all preflight queries."""
        transport.files["/media/fat/MiSTer"] = b"MISTER"
        transport.files["/media/fat/_Computer/Amstrad.rbf"] = b"AMSTRAD_RBF"
        transport.files["/media/fat/games/Amstrad/shaker.dsk"] = b"SHAKER_DSK"

        def handler(cmd: str) -> CommandResult:
            if "test -p /dev/MiSTer_cmd" in cmd:
                return CommandResult(0, "", "")
            if "cat /media/fat/MiSTer.ini" in cmd:
                return CommandResult(0, "screenshot_image_format=png\n", "")
            if "mkdir -p /media/fat/screenshots" in cmd:
                return CommandResult(0, "", "")
            if "which mbc" in cmd:
                return CommandResult(0, "/bin/mbc\n", "")
            if "test -c /dev/uinput" in cmd:
                return CommandResult(0, "", "")
            if "ls -1 '/media/fat/_Computer'" in cmd or "ls -1 /media/fat/_Computer" in cmd:
                return CommandResult(0, "Amstrad.rbf\nOther.rbf\n", "")
            if "test -f" in cmd:
                return CommandResult(0, "", "")
            if "sha256sum" in cmd:
                target = cmd.split()[-1]
                return CommandResult(0, f"{self.valid_sha}  {target}\n", "")
            if "load_core" in cmd:
                return CommandResult(0, "", "")
            if "raw_seq" in cmd:
                return CommandResult(0, "success\n", "")
            if "screenshot " in cmd and "> /dev/MiSTer_cmd" in cmd:
                name = cmd.split("screenshot ")[1].split("'")[0].strip()
                transport.files[f"/media/fat/screenshots/{name}"] = self.png_bytes
                return CommandResult(0, "", "")
            if "rm -f" in cmd:
                return CommandResult(0, "", "")
            return CommandResult(0, "", "")

        transport.register_handler(lambda _: True, handler)

    def test_pinned_identity_mismatch_stops_before_upload(self):
        """A replaced file at a valid path must not boot as the pinned case."""
        for field in ("rbf", "media"):
            with self.subTest(field=field):
                transport = ScriptedTransport()
                self._setup_working_device(transport)
                case = dict(self.case_dict, expected_sha256={field: "a" * 64})
                config = validate_case_config(case)
                out = self.out_dir / field
                with self.assertRaisesRegex(DriverError, "SHA-256 mismatch"):
                    run_hardware_loop(transport, config, out, sleep_fn=lambda _: None)
                manifest = json.loads((out / "manifest.json").read_text())
                self.assertEqual(manifest["partial_identity"][field + "_sha256"], self.valid_sha)
                self.assertFalse(manifest["cleanup"]["attempted"])
                self.assertFalse(any("load_core" in r["command"] or "scp " in r["command"]
                                     for r in transport.command_log))

    def test_pinned_identity_match_allows_capture(self):
        transport = ScriptedTransport()
        self._setup_working_device(transport)
        case = dict(self.case_dict, expected_sha256={"rbf": self.valid_sha.upper(),
                                                   "media": self.valid_sha})
        result = run_hardware_loop(transport, validate_case_config(case), self.out_dir,
                                   sleep_fn=lambda _: None)
        self.assertEqual(result["status"], "success")
        self.assertEqual(len(result["captures"]), 3)

    def test_failed_load_stops_before_input_or_capture(self):
        """Failure on load_core must stop execution before input injection or screenshot capture."""
        transport = ScriptedTransport()
        self._setup_working_device(transport)

        # Force load_core to fail
        transport.handlers.insert(0, (
            lambda cmd: "load_core" in cmd,
            lambda _: CommandResult(1, "", "load_core: error writing to FIFO/character device"),
        ))

        with self.assertRaises(DriverError) as ctx:
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)

        self.assertIn("load_core", str(ctx.exception))

        # Verify no input or screenshot commands were executed
        cmds = [c["command"] for c in transport.command_log]
        self.assertFalse(any("raw_seq" in c for c in cmds))
        self.assertFalse(any("screenshot " in c and "> /dev/MiSTer_cmd" in c for c in cmds))

        # Verify failure manifest was written
        manifest_path = self.out_dir / "manifest.json"
        self.assertTrue(manifest_path.exists())
        manifest = json.loads(manifest_path.read_text())
        self.assertEqual(manifest["status"], "failed")
        self.assertIn("load_core", manifest["error"])
        self.assertEqual(manifest["target"], "mister")
        self.assertTrue(len(manifest["intended_sequence"]) > 0)
        self.assertTrue(manifest["partial_identity"].get("mister_cmd_device_writable"))

    def test_failed_screenshot_stops_loop(self):
        """Failure dispatching screenshot command must abort loop and record failure."""
        transport = ScriptedTransport()
        self._setup_working_device(transport)

        # Force screenshot to fail
        transport.handlers.insert(0, (
            lambda cmd: "screenshot " in cmd and "> /dev/MiSTer_cmd" in cmd,
            lambda _: CommandResult(1, "", "screenshot failed"),
        ))

        with self.assertRaises(DriverError) as ctx:
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)

        self.assertIn("screenshot", str(ctx.exception))
        manifest = json.loads((self.out_dir / "manifest.json").read_text())
        self.assertEqual(manifest["status"], "failed")

    def test_hash_failure_nonzero_or_malformed(self):
        """sha256sum failure or malformed output must abort before load, not masquerade as UNKNOWN."""
        # Nonzero exit
        transport = ScriptedTransport()
        self._setup_working_device(transport)
        transport.handlers.insert(0, (
            lambda cmd: "sha256sum" in cmd and "Amstrad.rbf" in cmd,
            lambda _: CommandResult(1, "", "I/O Error reading RBF"),
        ))

        with self.assertRaises(DriverError) as ctx:
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)
        self.assertIn("sha256sum failed", str(ctx.exception))

        # Malformed output (e.g. UNKNOWN)
        transport2 = ScriptedTransport()
        self._setup_working_device(transport2)
        transport2.handlers.insert(0, (
            lambda cmd: "sha256sum" in cmd and "Amstrad.rbf" in cmd,
            lambda _: CommandResult(0, "UNKNOWN  file.rbf\n", ""),
        ))

        with self.assertRaises(DriverError) as ctx:
            run_hardware_loop(transport2, self.config, self.temp_dir / "out2", target="mister", sleep_fn=lambda _: None)
        self.assertIn("Malformed sha256sum output", str(ctx.exception))

    def test_ambiguous_rbf_prefix_rejected(self):
        """Directory collisions matching Main's get_rbf prefix rule must be rejected before load."""
        transport = ScriptedTransport()
        self._setup_working_device(transport)

        # Simulate directory with both Amstrad.rbf and Amstrad_20240901.rbf
        transport.handlers.insert(0, (
            lambda cmd: "ls -1" in cmd,
            lambda _: CommandResult(0, "Amstrad.rbf\nAmstrad_20240901.rbf\n", ""),
        ))

        with self.assertRaises(DriverError) as ctx:
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)

        self.assertIn("Ambiguous RBF prefix collision", str(ctx.exception))

    def test_rbf_different_selected_rejected(self):
        """If directory contains only Amstrad_2024.rbf for stem Amstrad, reject exact mismatch."""
        transport = ScriptedTransport()
        self._setup_working_device(transport)

        transport.handlers.insert(0, (
            lambda cmd: "ls -1" in cmd,
            lambda _: CommandResult(0, "Amstrad_2024.rbf\n", ""),
        ))

        with self.assertRaises(DriverError) as ctx:
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)

        self.assertIn("requested RBF was 'Amstrad.rbf'", str(ctx.exception))

    def test_xml_quoting_special_characters(self):
        """MGL generator must properly escape XML special characters (&, <, >, ", ')."""
        xml_str = generate_mgl_xml(
            rbf_path="/media/fat/_Computer/Amstrad.rbf",
            media_type="dsk",
            media_slot="S0",
            media_path='/media/fat/games/Tom & Jerry "Special" <1>.dsk',
            delay=1,
        )

        self.assertIn("&amp;", xml_str)
        self.assertIn("&quot;", xml_str)
        self.assertIn("&lt;", xml_str)
        self.assertIn("&gt;", xml_str)

        # Must parse cleanly back with ElementTree
        root = ET.fromstring(xml_str)
        self.assertEqual(root.tag, "mistergamedescription")
        file_elem = root.find("file")
        self.assertIsNotNone(file_elem)
        self.assertEqual(file_elem.attrib["path"], '/media/fat/games/Tom & Jerry "Special" <1>.dsk')

    def test_case_accepts_quoted_media_filename(self):
        case = dict(self.case_dict, media=dict(self.case_dict["media"], path='/media/fat/Tom & Jerry "Special".dsk'))
        checked = validate_case_config(case)
        xml = generate_mgl_xml(checked["rbf_path"], "dsk", "S0", checked["media"]["path"])
        self.assertEqual(ET.fromstring(xml).find("file").get("path"), case["media"]["path"])

    def test_preflight_failure_manifest_saved(self):
        """Preflight check failure must write failure manifest with target, case, and intended sequence."""
        transport = ScriptedTransport()
        self._setup_working_device(transport)

        # MiSTer_cmd missing
        transport.handlers.insert(0, (
            lambda cmd: "test -p /dev/MiSTer_cmd" in cmd,
            lambda _: CommandResult(1, "", "/dev/MiSTer_cmd not found"),
        ))

        with self.assertRaises(DriverError):
            run_hardware_loop(transport, self.config, self.out_dir, target="mister.local", port=2222, sleep_fn=lambda _: None)

        manifest_path = self.out_dir / "manifest.json"
        self.assertTrue(manifest_path.exists())
        manifest = json.loads(manifest_path.read_text())
        self.assertEqual(manifest["status"], "failed")
        self.assertEqual(manifest["target"], "mister.local")
        self.assertEqual(manifest["port"], 2222)
        self.assertEqual(manifest["effective_case"]["case_id"], "test_case")
        self.assertIn("preflight_checks", manifest["intended_sequence"])

    def test_partial_file_retry_then_complete_decode(self):
        """Polling must allow transient missing files and retry partial/corrupt files without burning budget on missing files."""
        transport = ScriptedTransport()
        self._setup_working_device(transport)

        # Capture 1:
        # Poll 1: test -f returns 1 (missing file - transient)
        # Poll 2: test -f returns 0, download gives corrupt file
        # Poll 3: test -f returns 0, download gives valid PNG
        poll_count = 0

        def screenshot_poller(cmd: str) -> CommandResult:
            nonlocal poll_count
            poll_count += 1
            if poll_count == 1:
                return CommandResult(1, "", "File not found yet")
            return CommandResult(0, "", "")

        transport.handlers.insert(0, (lambda cmd: "test -f" in cmd and "capture_" in cmd, screenshot_poller))

        download_count = 0
        orig_download = transport.download_file

        def download_mock(remote: str, local: Path, timeout: float):
            nonlocal download_count
            download_count += 1
            if download_count == 1:
                transport.files[remote] = b"\x89PNG\r\n\x1a\nTRUNCATED_BYTES"
            else:
                transport.files[remote] = self.png_bytes
            orig_download(remote, local, timeout)

        transport.download_file = download_mock

        # Test single capture
        self.config["captures_count"] = 1
        manifest = run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)

        self.assertEqual(manifest["status"], "success")
        self.assertEqual(len(manifest["captures"]), 1)
        self.assertEqual(manifest["captures"][0]["dimensions"], [384, 272])
        self.assertEqual(download_count, 2)
        self.assertGreaterEqual(poll_count, 2)

    def test_absent_capture_deterministic_deadline(self):
        """Missing screenshot must deterministically time out using injected monotonic clock."""
        transport = ScriptedTransport()
        self._setup_working_device(transport)

        # File never appears
        transport.handlers.insert(0, (
            lambda cmd: "test -f" in cmd and "capture_" in cmd,
            lambda _: CommandResult(1, "", "Never appears"),
        ))

        curr_time = 1000.0

        def mock_time() -> float:
            return curr_time

        def mock_sleep(sec: float) -> None:
            nonlocal curr_time
            curr_time += sec

        self.config["timeouts"]["capture_timeout"] = 3.0
        self.config["timeouts"]["poll_interval"] = 0.5

        with self.assertRaises(DriverError) as ctx:
            run_hardware_loop(
                transport,
                self.config,
                self.out_dir,
                target="mister",
                time_fn=mock_time,
                sleep_fn=mock_sleep,
            )

        self.assertIn("timed out", str(ctx.exception).lower())
        manifest = json.loads((self.out_dir / "manifest.json").read_text())
        self.assertEqual(manifest["status"], "failed")

    def test_ssh_nonzero_and_timeout(self):
        transport = SSHTransport("mister.local", port=2222)
        with patch("driver.subprocess.run", return_value=subprocess.CompletedProcess([], 7, "", "denied")):
            self.assertEqual(transport.run_cmd("false", 2).exit_code, 7)
            with self.assertRaises(DriverError):
                transport.upload_file(self.temp_dir / "a", "/media/fat/a", 2)
            with self.assertRaises(DriverError):
                transport.download_file("/media/fat/a", self.temp_dir / "a", 2)
        with patch("driver.subprocess.run", side_effect=subprocess.TimeoutExpired("ssh", 2)):
            with self.assertRaises(DriverError):
                transport.run_cmd("sleep 60", 2)
        self.assertEqual([entry["exit_code"] for entry in transport.command_log], [7, 7, 7, -1])

    @unittest.skipUnless(shutil.which("timeout"), "local GNU/BusyBox timeout needed for transport contract")
    def test_remote_command_has_its_own_deadline(self):
        # Simulate a connected SSH client whose local watchdog is unavailable:
        # execute only its remote shell command. The remote timer must stop it.
        done = self.temp_dir / "must-not-complete"
        real_run = subprocess.run
        def remote_shell(argv, **kwargs):
            return real_run(["sh", "-c", argv[-1]], capture_output=True, text=True, timeout=3)
        transport = SSHTransport("fake-target")
        with patch("driver.subprocess.run", side_effect=remote_shell):
            result = transport.run_cmd("sleep 2; touch " + shlex.quote(str(done)), timeout=0.2)
        self.assertNotEqual(result.exit_code, 0)
        self.assertFalse(done.exists())

    def test_ssh_and_scp_preserve_arguments(self):
        transport = SSHTransport("owner@fake-target", port=2222)
        local = self.temp_dir / "capture: space.png"
        with patch("driver.subprocess.run", return_value=subprocess.CompletedProcess([], 0, "", "")) as run:
            transport.upload_file(local, "/media/fat/a 'quote'.mgl", 3)
            args = run.call_args.args[0]
            self.assertIn("-O", args)  # MiSTer SCP does not require an SFTP subsystem.
            self.assertEqual(args[-2], str(local.resolve()))
            self.assertEqual(shlex.split(args[-1].split(":", 1)[1]), ["/media/fat/a 'quote'.mgl"])

    def test_real_fifo_preflight(self):
        # Main input.cpp:5140-5143 creates a FIFO. Exercise the OS type test,
        # not a fake that grants every /dev/MiSTer_cmd check unconditionally.
        fifo = self.temp_dir / "command-fifo"
        os.mkfifo(fifo)
        transport = ScriptedTransport()
        self._setup_working_device(transport)
        def check_fifo(cmd):
            result = subprocess.run(["sh", "-c", cmd.replace("/dev/MiSTer_cmd", str(fifo))], capture_output=True, text=True)
            return CommandResult(result.returncode, result.stdout, result.stderr)
        transport.handlers.insert(0, (lambda c: c.startswith("test ") and "/dev/MiSTer_cmd" in c, check_fifo))
        self.assertEqual(run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)["status"], "success")

    def test_unicode_input_and_fractional_counts_rejected(self):
        for text in ("EéE", "²", "١"):
            with self.subTest(text=text), self.assertRaises(ValueError):
                validate_raw_seq(text)
        with self.assertRaises(ValueError):
            validate_case_config(dict(self.case_dict, captures_count=1.5))

    def test_poll_transport_timeout_retains_capture_record(self):
        transport = ScriptedTransport()
        self._setup_working_device(transport)
        now = [0.0]
        original = transport.run_cmd
        def budgeted_poll(cmd, timeout):
            if cmd.startswith("test -f") and "capture_" in cmd:
                now[0] += timeout
                raise DriverError("SSH command timed out during capture stat")
            return original(cmd, timeout)
        transport.run_cmd = budgeted_poll
        with self.assertRaises(DriverError):
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", time_fn=lambda: now[0], sleep_fn=lambda _: None)
        saved = json.loads((self.out_dir / "manifest.json").read_text())
        self.assertEqual(len(saved["captures"]), 1)
        self.assertEqual(saved["captures"][0]["status"], "failed")
        self.assertEqual(saved["captures"][0]["poll_attempts"], 1)
        self.assertIn("SSH command timed out", saved["captures"][0]["error"])

    def test_slow_stat_does_not_extend_capture_deadline(self):
        transport = ScriptedTransport()
        self._setup_working_device(transport)
        now = [0.0]
        def slow_stat(cmd):
            now[0] += 5.0
            return CommandResult(0, "", "")
        transport.handlers.insert(0, (lambda c: c.startswith("test -f") and "capture_" in c, slow_stat))
        with patch.object(transport, "download_file", wraps=transport.download_file) as download:
            with self.assertRaises(DriverError):
                run_hardware_loop(transport, self.config, self.out_dir, target="mister", time_fn=lambda: now[0], sleep_fn=lambda _: None)
            download.assert_not_called()

    def test_cleanup_failure_is_preserved_in_final_log(self):
        transport = ScriptedTransport()
        self._setup_working_device(transport)
        transport.handlers.insert(0, (lambda c: c.startswith("rm -f") and "autoloop_" in c, lambda _: CommandResult(1, "", "read-only filesystem")))
        run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)
        saved = json.loads((self.out_dir / "manifest.json").read_text())
        self.assertIn("cleanup", saved)
        self.assertEqual(saved["command_log"][-1]["exit_code"], 1)

    def test_input_diagnostics_and_validation(self):
        """Validate whitespace rejection, !m rejection, balanced keys, and diagnostic detection."""
        # Whitespace rejected
        with self.assertRaises(ValueError) as ctx:
            validate_raw_seq("E E M")
        self.assertIn("Whitespace is rejected", str(ctx.exception))

        # !m rejected
        with self.assertRaises(ValueError) as ctx:
            validate_raw_seq("!m")
        self.assertIn("Unbounded mount wait", str(ctx.exception))

        # Unbalanced {1C rejected
        with self.assertRaises(ValueError) as ctx:
            validate_raw_seq("{1C!s")
        self.assertIn("Unbalanced", str(ctx.exception))

        # Valid balanced sequence
        toks, held = validate_raw_seq(":01{1C!s}1C")
        self.assertEqual(toks, [":01", "{1C", "!s", "}1C"])
        self.assertEqual(held, {"1C"})

        # MBC exit code 0 with diagnostic error in stdout
        transport = ScriptedTransport()
        self._setup_working_device(transport)
        transport.handlers.insert(0, (
            lambda cmd: "raw_seq" in cmd,
            lambda _: CommandResult(0, "error - device busy\n", ""),
        ))

        with self.assertRaises(DriverError) as ctx:
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)
        self.assertIn("MBC raw_seq failed", str(ctx.exception))

    def test_emergency_key_release_uncertainty(self):
        """Interrupted sequence must attempt best-effort key release and record uncertainty."""
        transport = ScriptedTransport()
        self._setup_working_device(transport)

        # Interrupted during the input sequence
        transport.handlers.insert(0, (
            lambda cmd: "raw_seq" in cmd,
            lambda _: CommandResult(1, "", "input interrupted"),
        ))

        self.config["input"]["raw_seq"] = "{1C}1C"

        with self.assertRaises(DriverError):
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)

        manifest = json.loads((self.out_dir / "manifest.json").read_text())
        rel_info = manifest.get("emergency_key_release")
        self.assertIsNotNone(rel_info)
        self.assertTrue(rel_info["attempted"])
        self.assertEqual(rel_info["keys"], ["1C"])
        self.assertIn("not guaranteed", rel_info["uncertainty_note"])

    def test_keyboard_interrupt_keeps_failure_evidence(self):
        transport = ScriptedTransport()
        self._setup_working_device(transport)
        def interrupt(cmd):
            raise KeyboardInterrupt()
        transport.handlers.insert(0, (lambda c: "raw_seq" in c and not c.endswith("'}1C'"), interrupt))
        with self.assertRaises(KeyboardInterrupt):
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)
        saved = json.loads((self.out_dir / "manifest.json").read_text())
        self.assertEqual(saved["status"], "failed")
        self.assertIn("input_uncertainty", saved)

    def test_finite_numeric_limits_and_strict_booleans(self):
        """Finite bounds and strict boolean validation."""
        # String false for ack_main_cmd must NOT be acknowledged
        bad_case = dict(self.case_dict)
        bad_case["ack_main_cmd"] = "false"
        with self.assertRaises(ValueError) as ctx:
            validate_case_config(bad_case)
        self.assertIn("strict boolean", str(ctx.exception))

        # Negative delay
        bad_case = dict(self.case_dict)
        bad_case["boot_delay"] = -1.0
        with self.assertRaises(ValueError):
            validate_case_config(bad_case)

        # Non-finite delay
        bad_case = dict(self.case_dict)
        bad_case["boot_delay"] = float("inf")
        with self.assertRaises(ValueError):
            validate_case_config(bad_case)

        # Captures count > 10
        bad_case = dict(self.case_dict)
        bad_case["captures_count"] = 11
        with self.assertRaises(ValueError):
            validate_case_config(bad_case)

        # Unknown key
        bad_case = dict(self.case_dict)
        bad_case["unknown_key"] = 123
        with self.assertRaises(ValueError):
            validate_case_config(bad_case)

        # Path traversal
        bad_case = dict(self.case_dict)
        bad_case["rbf_path"] = "/media/fat/../Amstrad.rbf"
        with self.assertRaises(ValueError):
            validate_case_config(bad_case)

    def test_output_preservation_and_no_overwrite(self):
        """Preserve local MGL and refuse to overwrite existing output run directory."""
        transport = ScriptedTransport()
        self._setup_working_device(transport)

        manifest = run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)
        self.assertEqual(manifest["status"], "success")

        # Local MGL must be preserved
        mgl_files = list(self.out_dir.glob("*.mgl"))
        self.assertEqual(len(mgl_files), 1)

        # Re-running against the same out_dir must refuse to overwrite
        with self.assertRaises(ValueError) as ctx:
            run_hardware_loop(transport, self.config, self.out_dir, target="mister", sleep_fn=lambda _: None)
        self.assertIn("already contains manifest.json", str(ctx.exception))

    def test_dry_run_offline_no_pillow_no_transport(self):
        """Dry run must output complete planned command sequence without contacting device or Pillow."""
        manifest = run_hardware_loop(
            transport=None,
            config=self.config,
            out_dir=self.out_dir,
            target="root@192.168.1.50",
            port=22,
            dry_run=True,
        )

        self.assertTrue(manifest["dry_run"])
        self.assertFalse(manifest["hardware_contacted"])
        self.assertEqual(manifest["status"], "dry_run")
        self.assertTrue((self.out_dir / "manifest.json").exists())

        # Check planned commands
        cmds = [c["command"] for c in manifest["command_log"]]
        self.assertTrue(any("test -p /dev/MiSTer_cmd" in c for c in cmds))
        self.assertTrue(any("sha256sum" in c for c in cmds))
        self.assertTrue(any("load_core" in c for c in cmds))
        self.assertTrue(any("raw_seq" in c for c in cmds))
        self.assertTrue(any("screenshot" in c for c in cmds))


if __name__ == "__main__":
    unittest.main()
