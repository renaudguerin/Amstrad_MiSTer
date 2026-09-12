#!/usr/bin/env python3
"""CSL script runner for the MiSTer hardware loop (backlog B4, phase 0).

Drives a Logon System CSL v1.4 script against a real MiSTer, reusing the
transport, MGL generation, hash pinning and screenshot retrieval already proven
by ``driver.py``.  The JSON case format stays for ad-hoc cases; CSL is the
normal way to drive a SHAKER walk.

What this target cannot honour is listed once, in ``REJECTED_COMMANDS``
below, and every deviation lands in the run manifest.  Nothing is silently
approximated: a command this target cannot perform stops the script with the
six fields CSL v1.4 requires (script, line, instruction, reason, script
version, supported version).

Two properties of the FPGA target shape the design:

* ``wait`` is specified in *emulated* microseconds.  The core runs 1:1 from the
  PLL, so a wait is a host sleep whose only error is host latency.  The SHAKER
  scripts pad their waits well above the screen times recorded in their own
  comments, so this is an approximation to record, not a blocker.
* Keyboard input goes through MBC, whose ``MBC_KEY_WAIT`` sleeps once before
  every press and every release.  That single knob is both the CSL key-press
  delay and the CSL inter-key delay, so ``key_delay 70000 70000`` maps exactly;
  a script asking for two different values cannot be honoured exactly and says
  so in the manifest.

Phase 0 has no SSM detector, so ``wait_ssm0000`` and marker-driven captures are
rejected here and arrive with the RTL work in phase 1.  See
``docs/csl-ssm-implementation-plan.md``.
"""

from __future__ import annotations

import argparse
import base64
import binascii
import hashlib
import json
import re
import shlex
import sys
import time
import uuid
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Sequence, Tuple

import cpc_keys
import ssm_ring
from cpc_keys import KeyGroup, KeyTranslationError, sequence_tokens, translate_text
from driver import (
    DriverError,
    SSHTransport,
    check_pillow_installed,
    check_rbf_directory,
    emergency_release_keys,
    fetch_sha256,
    generate_mgl_xml,
    validate_device_path,
    validate_raw_seq,
    validate_screenshot_name,
    verify_png,
)

SUPPORTED_CSL_VERSION = "1.4"
KNOWN_CSL_VERSIONS = ("1.0", "1.1", "1.2", "1.3", "1.4")

MAX_CSL_LOAD_DEPTH = 8
MAX_TOTAL_COMMANDS = 20000

DEFAULT_CFG_PATH = "/media/fat/config/Amstrad.CFG"
CFG_SIZE = 16

# Amstrad.sv CONF_STR status bits.  "d3P1O2,CRTC,Type 1,Type 0;" means bit 2
# clear selects Type 1, and Amstrad_motherboard is wired .crtc_type(~status[2]).
CFG_BIT_CRTC = 2
# "d3P2O[5:4],Model,CPC 6128,CPC 664,CPC 464;"
CFG_BITS_MODEL = (4, 5)
# "P2O[37],SSM markers,Off,On;" gates rtl/ssm_marker.v.
CFG_BIT_SSM = 37

# CSL cpc_model number -> (status[5:4] value, label).  Plus models have no
# classic-status encoding; they are rejected until Plus CSL is in scope.
CSL_MODEL_TO_STATUS = {0: (2, "CPC 464"), 1: (1, "CPC 664"), 2: (0, "CPC 6128")}

# MBC sleeps this many milliseconds once when it opens its uinput device and
# once before closing it (mbc.c sequence_wait).  It is fixed overhead per
# invocation, not a per-key delay.
MBC_SEQUENCE_WAIT_MS = 1000

# Commands that need the machine to be running.  A pending power-on load is
# performed before the first of these, and configuration/media commands seen
# between the reset and that point are folded into the load.
MACHINE_COMMANDS = frozenset(
    {"key_output", "key_from_file", "keyboard_write", "screenshot", "snapshot",
     "wait_ssm0000", "wait_vsyncoffon", "wait_driveonoff"}
)

# Commands folded into the power-on load rather than executed in place.
FOLDABLE_COMMANDS = frozenset({"crtc_select", "cpc_model", "disk_insert", "disk_dir"})

REJECTED_COMMANDS: Dict[str, str] = {
    "keyboard_write": "no keyboard matrix injection port exists in this core; input arrives through MBC as Linux keycodes",
    "gate_array": "the core has no selectable gate array model",
    "memory_exp": "the core has no selectable memory expansion",
    "rom_dir": "ROM selection is a Main/OSD setting this runner does not change",
    "rom_config": "ROM selection is a Main/OSD setting this runner does not change",
    "tape_insert": "tape media is out of scope for the SHAKER walk",
    "tape_dir": "tape media is out of scope for the SHAKER walk",
    "tape_play": "tape media is out of scope for the SHAKER walk",
    "tape_stop": "tape media is out of scope for the SHAKER walk",
    "tape_rewind": "tape media is out of scope for the SHAKER walk",
    "snapshot_load": "snapshot commands are out of scope for the SHAKER walk",
    "snapshot_dir": "snapshot commands are out of scope for the SHAKER walk",
    "snapshot_name": "snapshot commands are out of scope for the SHAKER walk",
    "snapshot": "snapshot commands are out of scope for the SHAKER walk",
    "snapshot_version": "snapshot commands are out of scope for the SHAKER walk",
    "wait_vsyncoffon": "no VSYNC observability reaches the host; no SHAKER script uses it",
    "wait_driveonoff": "no drive-motor observability reaches the host; no SHAKER script uses it",
}

# Rejected only when the SSM detector is not in use.
SSM_ONLY_COMMANDS: Dict[str, str] = {
    "wait_ssm0000": "needs the SSM detector; pass --ssm to enable it (OSD status bit 37)",
}

ALL_COMMANDS = frozenset(
    {"csl_version", "reset", "crtc_select", "cpc_model", "disk_insert", "disk_dir",
     "key_delay", "key_output", "key_from_file", "wait", "screenshot",
     "screenshot_name", "screenshot_dir", "csl_load"}
    | set(REJECTED_COMMANDS) | set(SSM_ONLY_COMMANDS)
)

_SAFE_NAME = re.compile(r"^[A-Za-z0-9_\-]+$")


class CslError(Exception):
    """Script-stopping error, carrying the six fields CSL v1.4 section
    "Some rules and implementation tips" requires an emulator to report."""

    def __init__(
        self,
        reason: str,
        script: str = "",
        line: int = 0,
        instruction: str = "",
        script_version: str = "",
        supported_version: str = SUPPORTED_CSL_VERSION,
    ):
        self.reason = reason
        self.script = script
        self.line = line
        self.instruction = instruction
        self.script_version = script_version
        self.supported_version = supported_version
        super().__init__(str(self))

    def as_dict(self) -> Dict[str, Any]:
        return {
            "script": self.script,
            "line": self.line,
            "instruction": self.instruction,
            "reason": self.reason,
            "script_version": self.script_version or "(not declared)",
            "supported_version": self.supported_version,
        }

    def __str__(self) -> str:
        return (
            f"{self.script}:{self.line}: {self.instruction!r} rejected: {self.reason} "
            f"(script version {self.script_version or 'not declared'}, "
            f"supported {self.supported_version})"
        )


class Command:
    """One parsed CSL line."""

    __slots__ = ("script", "line", "name", "args", "text")

    def __init__(self, script: str, line: int, name: str, args: List[str], text: str):
        self.script = script
        self.line = line
        self.name = name
        self.args = args
        self.text = text

    def error(self, reason: str, script_version: str = "") -> CslError:
        return CslError(reason, self.script, self.line, self.text, script_version)

    def __repr__(self) -> str:  # pragma: no cover - debugging aid
        return f"Command({self.script}:{self.line} {self.text!r})"


def _split_args(rest: str) -> List[str]:
    """Split a CSL argument tail into bare words and quoted strings.

    CSL quotes strings with a single quote and has no escape inside them, so a
    quoted run ends at the next quote.  Typographic quotes appear in the
    standard's own examples and are accepted.
    """
    args: List[str] = []
    idx = 0
    quotes = "'‘’"
    while idx < len(rest):
        char = rest[idx]
        if char.isspace() or char == ",":
            idx += 1
            continue
        if char in quotes:
            end = idx + 1
            while end < len(rest) and rest[end] not in quotes:
                end += 1
            if end >= len(rest):
                raise ValueError("unterminated quoted argument")
            args.append(rest[idx + 1 : end])
            idx = end + 1
        else:
            end = idx
            while end < len(rest) and not rest[end].isspace() and rest[end] != ",":
                end += 1
            args.append(rest[idx:end])
            idx = end
    return args


def parse_script(text: str, script: str) -> List[Command]:
    """Parse CSL source into commands.

    Syntax only: an unknown or unsupported command name parses fine and is
    rejected later, so that the whole corpus can be parsed offline without the
    parser deciding what this target supports.
    """
    commands: List[Command] = []
    for number, raw in enumerate(text.splitlines(), start=1):
        stripped = raw.split(";", 1)[0].strip()
        if not stripped:
            continue
        head, _, rest = stripped.partition(" ")
        name = head.lower()
        try:
            args = _split_args(rest)
        except ValueError as exc:
            raise CslError(str(exc), script, number, stripped) from exc
        commands.append(Command(script, number, name, args, stripped))
    return commands


def _resolve_csl_load(current: Path, name: str) -> Path:
    """Resolve a csl_load argument against the loading script's directory.

    Scripts reference siblings without an extension ("SHAKE26B-2"); the bundle
    stores them uppercase with a .CSL extension, and some hosts are
    case-sensitive.
    """
    if "/" in name or "\\" in name or name in ("", ".", ".."):
        raise ValueError(f"csl_load target {name!r} must be a plain file name")
    directory = current.parent
    candidates = [name, f"{name}.CSL", f"{name}.csl"]
    for candidate in candidates:
        path = directory / candidate
        if path.is_file():
            return path
    lowered = {p.name.lower(): p for p in directory.iterdir() if p.is_file()}
    for candidate in candidates:
        hit = lowered.get(candidate.lower())
        if hit is not None:
            return hit
    raise ValueError(f"csl_load target {name!r} not found beside {current.name}")


def load_program(entry: Path, follow_loads: bool = True) -> List[Command]:
    """Parse a script and, unless disabled, the chain it csl_loads.

    Depth-limited and cycle-checked: a script already on the load stack is a
    cycle and stops the program.
    """
    flattened: List[Command] = []

    def walk(path: Path, stack: List[Path], depth: int) -> None:
        resolved = path.resolve()
        if resolved in stack:
            chain = " -> ".join(p.name for p in stack + [resolved])
            raise CslError(f"csl_load cycle detected: {chain}", resolved.name, 0, "csl_load")
        if depth > MAX_CSL_LOAD_DEPTH:
            raise CslError(
                f"csl_load nesting exceeded {MAX_CSL_LOAD_DEPTH} levels",
                resolved.name, 0, "csl_load",
            )
        commands = parse_script(resolved.read_text(encoding="utf-8", errors="replace"), resolved.name)
        for command in commands:
            if len(flattened) >= MAX_TOTAL_COMMANDS:
                raise CslError(
                    f"program exceeded {MAX_TOTAL_COMMANDS} commands",
                    command.script, command.line, command.text,
                )
            if command.name == "csl_load":
                if not follow_loads:
                    flattened.append(command)
                    continue
                if len(command.args) != 1:
                    raise command.error("csl_load takes exactly one file name")
                try:
                    target = _resolve_csl_load(resolved, command.args[0])
                except ValueError as exc:
                    raise command.error(str(exc)) from exc
                flattened.append(command)
                walk(target, stack + [resolved], depth + 1)
            else:
                flattened.append(command)

    walk(entry, [], 0)
    return flattened


class RunOptions:
    """Everything the script does not and cannot say about this device."""

    def __init__(
        self,
        rbf_path: str,
        disk_dir: str,
        layout: str = "fr",
        cfg_path: str = DEFAULT_CFG_PATH,
        mbc_path: str = "mbc",
        out_dir: Optional[Path] = None,
        stop_at: Optional[Tuple[str, int]] = None,
        screenshot_at: Sequence[Tuple[str, int]] = (),
        expect_sha256: Optional[Dict[str, str]] = None,
        cmd_timeout: float = 30.0,
        capture_timeout: float = 20.0,
        poll_interval: float = 0.5,
        max_wait_seconds: float = 300.0,
        follow_loads: bool = True,
        ssm: bool = False,
        ssm_base: int = ssm_ring.DEFAULT_BASE,
        ssm_poll_interval: float = 0.5,
    ):
        validate_device_path(rbf_path, "rbf_path")
        validate_device_path(disk_dir, "disk_dir")
        validate_device_path(cfg_path, "cfg_path")
        if layout not in cpc_keys.SUPPORTED_LAYOUTS:
            raise ValueError(f"layout must be one of {cpc_keys.SUPPORTED_LAYOUTS}, got {layout!r}")
        self.rbf_path = rbf_path
        self.disk_dir = disk_dir.rstrip("/")
        self.layout = layout
        self.cfg_path = cfg_path
        self.mbc_path = mbc_path
        self.out_dir = out_dir
        self.stop_at = stop_at
        self.screenshot_at = list(screenshot_at)
        self.expect_sha256 = dict(expect_sha256 or {})
        self.cmd_timeout = cmd_timeout
        self.capture_timeout = capture_timeout
        self.poll_interval = poll_interval
        self.max_wait_seconds = max_wait_seconds
        self.follow_loads = follow_loads
        self.ssm = ssm
        self.ssm_base = ssm_base
        self.ssm_poll_interval = ssm_poll_interval


class Backend:
    """What the runner needs from the world.  PlanBackend records; DeviceBackend acts."""

    def load_core(self, load: Dict[str, Any]) -> Dict[str, Any]:
        raise NotImplementedError

    def sleep(self, seconds: float, reason: str) -> None:
        raise NotImplementedError

    def send_keys(self, raw_seq: str, key_wait_ms: int, description: str) -> Dict[str, Any]:
        raise NotImplementedError

    def screenshot(self, name: str) -> Dict[str, Any]:
        raise NotImplementedError

    def poll_ssm(self) -> List[Dict[str, Any]]:
        """Return the SSM records written since the previous poll."""
        return []


class PlanBackend(Backend):
    """Offline backend: records the intended actions and touches nothing."""

    def __init__(self) -> None:
        self.actions: List[Dict[str, Any]] = []

    def load_core(self, load: Dict[str, Any]) -> Dict[str, Any]:
        self.actions.append({"action": "load_core", **load})
        return {"planned": True}

    def sleep(self, seconds: float, reason: str) -> None:
        self.actions.append({"action": "sleep", "seconds": round(seconds, 6), "reason": reason})

    def send_keys(self, raw_seq: str, key_wait_ms: int, description: str) -> Dict[str, Any]:
        self.actions.append({
            "action": "send_keys", "raw_seq": raw_seq,
            "mbc_key_wait_ms": key_wait_ms, "description": description,
        })
        return {"planned": True}

    def screenshot(self, name: str) -> Dict[str, Any]:
        self.actions.append({"action": "screenshot", "name": name})
        return {"planned": True, "name": name}

    def poll_ssm(self) -> List[Dict[str, Any]]:
        # Offline there is no device to read, so a plan shows where the
        # polls would happen and nothing more.
        self.actions.append({"action": "poll_ssm"})
        return []


def _cfg_apply_bits(data: bytes, bits: Dict[int, int]) -> bytes:
    """Return a copy of the CFG with exactly the named status bits set/cleared."""
    if len(data) != CFG_SIZE:
        raise DriverError(f"Amstrad.CFG must be {CFG_SIZE} bytes, got {len(data)}")
    out = bytearray(data)
    for bit, value in bits.items():
        if not 0 <= bit < CFG_SIZE * 8:
            raise DriverError(f"status bit {bit} outside the {CFG_SIZE}-byte CFG")
        index, mask = bit // 8, 1 << (bit % 8)
        if value:
            out[index] |= mask
        else:
            out[index] &= 0xFF ^ mask
    return bytes(out)


class DeviceBackend(Backend):
    """Live backend over the existing SSH transport."""

    def __init__(
        self,
        transport: Any,
        options: RunOptions,
        out_dir: Path,
        sleep_fn: Callable[[float], None] = time.sleep,
        time_fn: Callable[[], float] = time.monotonic,
    ):
        self.transport = transport
        self.options = options
        self.out_dir = out_dir
        self.sleep_fn = sleep_fn
        self.time_fn = time_fn
        self.actions: List[Dict[str, Any]] = []
        self.captures: List[Dict[str, Any]] = []
        self.ssm_status: str = "not enabled"
        self.ssm_header: Dict[str, Any] = {}
        self.cfg_original: Optional[bytes] = None
        self.cfg_original_sha: str = ""
        self.cfg_written = False
        self.keys_in_flight: Optional[set] = None
        self.remote_temp: List[str] = []
        self.run_id = f"{int(time.time())}_{uuid.uuid4().hex[:6]}"
        self.ring = (
            ssm_ring.SsmRingReader(transport, base=options.ssm_base,
                                   timeout=options.cmd_timeout)
            if options.ssm else None
        )

    # --- helpers -------------------------------------------------------

    def _run(self, cmd: str, timeout: Optional[float] = None) -> Any:
        return self.transport.run_cmd(cmd, timeout=timeout or self.options.cmd_timeout)

    def read_cfg(self) -> bytes:
        path = self.options.cfg_path
        if self._run(f"test -f {shlex.quote(path)}").exit_code != 0:
            raise DriverError(
                f"{path} is absent. Set the base configuration once through the OSD; "
                "this runner changes only the bits a script names and never creates a CFG."
            )
        res = self._run(f"base64 < {shlex.quote(path)} | tr -d '\\n'")
        if res.exit_code != 0:
            raise DriverError(f"Cannot read {path} ({res.exit_code}): {res.stderr.strip()}")
        try:
            data = base64.b64decode(res.stdout.strip(), validate=True)
        except (binascii.Error, ValueError) as exc:
            raise DriverError(f"Malformed base64 for {path}: {exc}") from exc
        if len(data) != CFG_SIZE:
            raise DriverError(f"{path} is {len(data)} bytes, expected {CFG_SIZE}")
        return data

    def write_cfg(self, data: bytes) -> str:
        """Upload a CFG and confirm by hash that the device holds exactly it."""
        path = self.options.cfg_path
        expected = hashlib.sha256(data).hexdigest()
        local = self.out_dir / f"Amstrad_{self.run_id}.CFG"
        local.write_bytes(data)
        staging = f"/media/fat/csl_cfg_{self.run_id}.bin"
        self.transport.upload_file(local, staging, timeout=self.options.cmd_timeout)
        self.remote_temp.append(staging)
        res = self._run(f"cat {shlex.quote(staging)} > {shlex.quote(path)}")
        if res.exit_code != 0:
            raise DriverError(f"Cannot write {path} ({res.exit_code}): {res.stderr.strip()}")
        actual = fetch_sha256(self.transport, path, self.options.cmd_timeout)
        if actual != expected:
            raise DriverError(f"{path} SHA-256 mismatch after write: expected {expected}, got {actual}")
        self.cfg_written = True
        return actual

    def restore_cfg(self) -> Dict[str, Any]:
        if self.cfg_original is None or not self.cfg_written:
            return {"attempted": False}
        try:
            actual = self.write_cfg(self.cfg_original)
            return {
                "attempted": True,
                "restored_sha256": actual,
                "matches_original": actual == self.cfg_original_sha,
            }
        except Exception as exc:
            return {"attempted": True, "error": str(exc),
                    "original_sha256": self.cfg_original_sha,
                    "local_copy": str(self.out_dir / f"Amstrad_{self.run_id}.CFG")}

    # --- backend interface ---------------------------------------------

    def load_core(self, load: Dict[str, Any]) -> Dict[str, Any]:
        record: Dict[str, Any] = {"action": "load_core", **load}

        if self.cfg_original is None:
            self.cfg_original = self.read_cfg()
            self.cfg_original_sha = hashlib.sha256(self.cfg_original).hexdigest()
            record["cfg_original_sha256"] = self.cfg_original_sha

        bits: Dict[int, int] = {}
        if load.get("crtc_status_bit") is not None:
            bits[CFG_BIT_CRTC] = load["crtc_status_bit"]
        if load.get("model_status") is not None:
            value = load["model_status"]
            bits[CFG_BITS_MODEL[0]] = value & 1
            bits[CFG_BITS_MODEL[1]] = (value >> 1) & 1
        # The detector is off by default, so turning it on is part of the
        # configuration this run applies and restores.
        bits[CFG_BIT_SSM] = 1 if self.options.ssm else 0
        if bits:
            record["cfg_sha256"] = self.write_cfg(_cfg_apply_bits(self.cfg_original, bits))
            record["cfg_bits_changed"] = {str(k): v for k, v in sorted(bits.items())}
            record["configuration_evidence"] = (
                "applied by CFG; the native PNG omits the OSD, so this is not visually confirmed"
            )

        check_rbf_directory(self.transport, self.options.rbf_path, self.options.cmd_timeout)
        media = load.get("media_path")
        if media:
            if self._run(f"test -f {shlex.quote(media)}").exit_code != 0:
                raise DriverError(f"Declared media file not found: {media}")
            record["media_sha256"] = fetch_sha256(self.transport, media, self.options.cmd_timeout)
        record["rbf_sha256"] = fetch_sha256(self.transport, self.options.rbf_path, self.options.cmd_timeout)

        for kind, expected in self.options.expect_sha256.items():
            key = f"{kind}_sha256"
            if key in record and record[key] != expected:
                raise DriverError(f"{kind} SHA-256 mismatch: expected {expected}, got {record[key]}")

        # MGL exists to mount media as part of the load.  With no disk there is
        # nothing to mount, so load the RBF directly and leave no temp file.
        if media:
            mgl = generate_mgl_xml(self.options.rbf_path, "dsk",
                                   load.get("media_slot", "S0"), media, delay=1)
            local_mgl = self.out_dir / f"csl_{self.run_id}_{len(self.actions)}.mgl"
            local_mgl.write_text(mgl, encoding="utf-8")
            target = f"/media/fat/csl_{self.run_id}_{len(self.actions)}.mgl"
            self.transport.upload_file(local_mgl, target, timeout=self.options.cmd_timeout)
            self.remote_temp.append(target)
            record["mgl"] = target
        else:
            target = self.options.rbf_path

        res = self._run(f"printf '%s\\n' {shlex.quote(f'load_core {target}')} > /dev/MiSTer_cmd")
        if res.exit_code != 0:
            raise DriverError(f"load_core failed ({res.exit_code}): {res.stderr.strip()}")
        record["load_confirmation"] = (
            "file selection requested only; Main does not acknowledge load completion"
        )
        if self.ring is not None:
            # The ring lives in DDR3 and a core load does not clear it, so the
            # reader starts from whatever the previous run left and counts
            # forward from there.
            self.ring.reset()
        self.actions.append(record)
        return record

    def sleep(self, seconds: float, reason: str) -> None:
        start = self.time_fn()
        self.sleep_fn(seconds)
        self.actions.append({
            "action": "sleep", "requested_seconds": round(seconds, 6),
            "elapsed_seconds": round(self.time_fn() - start, 6), "reason": reason,
        })

    def send_keys(self, raw_seq: str, key_wait_ms: int, description: str) -> Dict[str, Any]:
        _, potentially_held = validate_raw_seq(raw_seq)
        self.keys_in_flight = potentially_held
        cmd = (
            f"exec env MBC_KEY_WAIT={key_wait_ms} MBC_SEQUENCE_WAIT={MBC_SEQUENCE_WAIT_MS} "
            f"{shlex.quote(self.options.mbc_path)} raw_seq {shlex.quote(raw_seq)}"
        )
        res = self._run(cmd)
        self.keys_in_flight = None
        if res.exit_code != 0 or "error - " in (res.stdout + res.stderr).lower():
            raise DriverError(
                f"MBC raw_seq failed ({res.exit_code}): stdout={res.stdout!r} stderr={res.stderr!r}"
            )
        record = {
            "action": "send_keys", "raw_seq": raw_seq, "description": description,
            "mbc_key_wait_ms": key_wait_ms, "mbc_sequence_wait_ms": MBC_SEQUENCE_WAIT_MS,
        }
        self.actions.append(record)
        return record

    def screenshot(self, name: str) -> Dict[str, Any]:
        validate_screenshot_name(name)
        remote = f"/media/fat/screenshots/{name}"
        local = self.out_dir / name
        if self._run(f"test ! -e {shlex.quote(remote)}").exit_code != 0:
            raise DriverError(f"Screenshot path already exists or cannot be checked: {remote}")
        if self._run(f"printf '%s\\n' {shlex.quote(f'screenshot {name}')} > /dev/MiSTer_cmd").exit_code != 0:
            raise DriverError(f"screenshot command failed for {name}")

        deadline = self.time_fn() + self.options.capture_timeout
        dims: Optional[Tuple[int, int]] = None
        last_error: Optional[str] = None
        attempts = 0
        while self.time_fn() < deadline:
            attempts += 1
            remaining = deadline - self.time_fn()
            if remaining <= 0:
                break
            try:
                stat = self._run(f"test -f {shlex.quote(remote)}", timeout=min(self.options.cmd_timeout, remaining))
                if stat.exit_code == 1:
                    self.sleep_fn(min(self.options.poll_interval, max(0.0, deadline - self.time_fn())))
                    continue
                if stat.exit_code != 0:
                    raise DriverError(f"Capture file check failed ({stat.exit_code}): {stat.stderr}")
                remaining = deadline - self.time_fn()
                if remaining <= 0:
                    break
                self.transport.download_file(remote, local, timeout=min(self.options.cmd_timeout, remaining))
                candidate = verify_png(local)
                if self.time_fn() >= deadline:
                    last_error = "Capture completed after its deadline"
                    break
                dims = candidate
                break
            except Exception as exc:  # retry a partially written file
                last_error = str(exc)
                self.sleep_fn(min(self.options.poll_interval, max(0.0, deadline - self.time_fn())))

        if dims is None:
            message = last_error or f"Capture {name} timed out after {self.options.capture_timeout}s"
            self.captures.append({"name": name, "status": "failed", "error": message})
            raise DriverError(message)

        record = {
            "action": "screenshot", "name": name, "remote_path": remote,
            "local_path": str(local), "dimensions": list(dims),
            "sha256": hashlib.sha256(local.read_bytes()).hexdigest(),
            "poll_attempts": attempts, "status": "completed",
        }
        self.captures.append(record)
        self.actions.append(record)
        return record

    def poll_ssm(self) -> List[Dict[str, Any]]:
        if self.ring is None:
            return []
        try:
            header, records = self.ring.poll()
        except ssm_ring.SsmRingError as exc:
            # Before the first marker the ring holds whatever was in DDR3, so
            # an unrecognised header is expected early and is not an error.
            self.ssm_status = str(exc)
            return []
        self.ssm_status = "ok"
        self.ssm_header = header.as_dict()
        out = [r.as_dict() for r in records]
        if out:
            self.actions.append({"action": "ssm_records", "records": out,
                                 "header": self.ssm_header})
        return out

    # --- teardown -------------------------------------------------------

    def cleanup(self) -> Dict[str, Any]:
        result: Dict[str, Any] = {"remote_temp": []}
        if self.keys_in_flight:
            result["emergency_key_release"] = emergency_release_keys(
                self.transport, self.options.mbc_path, self.keys_in_flight, self.options.cmd_timeout
            )
        result["cfg_restore"] = self.restore_cfg()
        for path in self.remote_temp:
            try:
                res = self._run(f"rm -f {shlex.quote(path)}")
                result["remote_temp"].append({"path": path, "exit_code": res.exit_code})
            except Exception as exc:
                result["remote_temp"].append({"path": path, "error": str(exc)})
        return result


class CslRunner:
    """Walks a parsed program once, driving a backend."""

    def __init__(self, program: List[Command], options: RunOptions, backend: Backend,
                 entry_script: str):
        self.program = program
        self.options = options
        self.backend = backend
        self.entry_script = entry_script

        self.script_version = ""
        self.version_warning: Optional[str] = None
        self.key_press_us = 19968.0
        self.key_between_us = 19968.0
        self.key_after_cr_us: Optional[float] = None
        self.crtc_requested: Optional[str] = None
        self.crtc_status_bit: Optional[int] = None
        self.model_status: Optional[int] = None
        self.media_path: Optional[str] = None
        self.media_slot = "S0"
        self.disk_dir = options.disk_dir
        self.screenshot_name: Optional[str] = None
        self.machine_running = False

        self.trace: List[Dict[str, Any]] = []
        self.approximations: List[Dict[str, Any]] = []
        self.folded: Dict[int, int] = {}
        self._fold_order: Dict[int, List[int]] = {}
        self.ssm_records: List[Dict[str, Any]] = []
        self.ssm_sync_seen = 0
        self.capture_count = 0
        self.entry_script_stem = Path(entry_script).stem
        self._media_line = 0

    # --- planning helpers -----------------------------------------------

    def _fold_power_on(self) -> None:
        """Bind configuration and media commands to the power-on they belong to.

        A core load applies the CFG and mounts the media in one step, so both
        sides of a ``reset`` feed it: scripts put ``crtc_select`` before the
        reset and ``disk_insert`` after it, separated only by a boot ``wait``.
        The window stops at the nearest command that needs a running machine,
        and at any other reset, so a ``crtc_select`` that genuinely arrives
        mid-session is left to be judged as a live change.
        """
        owned: Dict[int, List[int]] = {}
        for index, command in enumerate(self.program):
            if command.name != "reset":
                continue
            window: List[int] = []
            for back in range(index - 1, -1, -1):
                previous = self.program[back]
                if previous.name in MACHINE_COMMANDS or previous.name == "reset":
                    break
                if previous.name in FOLDABLE_COMMANDS and back not in self.folded:
                    window.append(back)
            for ahead in range(index + 1, len(self.program)):
                nxt = self.program[ahead]
                if nxt.name in MACHINE_COMMANDS or nxt.name == "reset":
                    break
                if nxt.name in FOLDABLE_COMMANDS:
                    window.append(ahead)
            for position in window:
                self.folded[position] = index
            owned[index] = sorted(window)
        self._fold_order = owned

    def _note(self, command: Command, kind: str, detail: str) -> None:
        self.approximations.append({
            "script": command.script, "line": command.line,
            "instruction": command.text, "kind": kind, "detail": detail,
        })

    def _record(self, command: Command, outcome: str, **extra: Any) -> None:
        self.trace.append({
            "script": command.script, "line": command.line, "instruction": command.text,
            "outcome": outcome, "timestamp": datetime.now(timezone.utc).isoformat(), **extra,
        })

    def _key_wait_ms(self, command: Command) -> int:
        """Pick MBC_KEY_WAIT and record any loss against what CSL asked for.

        mbc.c sleeps inter_key_wait before every press and every release, so
        one value is simultaneously the press duration and the inter-key gap.
        """
        press, between = self.key_press_us, self.key_between_us
        chosen_us = max(press, between)
        ms = max(1, min(60000, round(chosen_us / 1000.0)))
        if press != between:
            self._note(command, "key_delay",
                       f"MBC has one delay knob for both; asked press={press:.0f}us "
                       f"between={between:.0f}us, used {ms}ms for both")
        elif abs(ms * 1000.0 - chosen_us) > 1.0:
            self._note(command, "key_delay",
                       f"MBC_KEY_WAIT has millisecond resolution; asked {chosen_us:.0f}us, used {ms}ms")
        return ms

    def _sleep(self, seconds: float, reason: str, command: Optional[Command] = None) -> None:
        """Sleep, polling the SSM ring in slices when the detector is on.

        A marker is only useful if the capture follows it closely, so with
        SSM enabled the runner never sleeps past one poll interval without
        looking. With SSM off this is a single sleep and costs nothing.
        """
        if not self.options.ssm or seconds <= 0:
            self.backend.sleep(seconds, reason)
            return
        slice_s = max(0.05, self.options.ssm_poll_interval)
        remaining = seconds
        while remaining > 0:
            step = min(slice_s, remaining)
            self.backend.sleep(step, reason)
            remaining -= step
            self._consume_ssm(command)

    def _consume_ssm(self, command: Optional[Command]) -> List[Dict[str, Any]]:
        """Act on markers the core has published since the last look."""
        records = self.backend.poll_ssm()
        for record in records:
            self.ssm_records.append(record)
            code = int(record["code"], 16)
            if code == ssm_ring.CODE_SYNC:
                self.ssm_sync_seen += 1
            elif code == ssm_ring.CODE_SCREENSHOT:
                self._ssm_capture(record, command)
            elif code == ssm_ring.CODE_SNAPSHOT:
                if command is not None:
                    self._note(command, "ssm",
                               "marker #FFFF asks for a snapshot; this runner makes none")
        return records

    def _ssm_capture(self, record: Dict[str, Any], command: Optional[Command]) -> None:
        """Capture for an #FFFE marker.

        The capture lands at least a frame after the marker, because Main
        grabs the scaler output asynchronously. The manifest keeps the
        marker's own frame and raster position so the distance is visible
        rather than assumed away.
        """
        self.capture_count += 1
        if self.screenshot_name:
            name = f"{self.screenshot_name}.png"
            self.screenshot_name = None
        else:
            crtc = "0" if self.crtc_status_bit else "1"
            name = ssm_ring.suggested_name("MISTER", crtc, ssm_ring.CODE_SCREENSHOT)
            # A bare #FFFE would name every capture the same file, so the
            # marker's own sequence number disambiguates them.
            name = name.replace(".png", f"_{record['seq']:04d}.png")
        result = self.backend.screenshot(name)
        result["ssm_record"] = record
        if command is not None:
            self._record(command, "ssm_capture", name=name, ssm=record)

    def _pending_load(self) -> Dict[str, Any]:
        return {
            "crtc_requested": self.crtc_requested,
            "crtc_status_bit": self.crtc_status_bit,
            "model_status": self.model_status,
            "media_path": self.media_path,
            "media_slot": self.media_slot,
        }

    def _ensure_machine(self, command: Command) -> None:
        if self.machine_running:
            return
        raise command.error(
            "the machine has not been powered on; the script must reset before sending input"
        )

    # --- command implementations ----------------------------------------

    def _do_reset(self, command: Command, index: int) -> None:
        mode = command.args[0].lower() if command.args else "hard"
        if mode not in ("hard", "soft"):
            raise command.error(f"unknown reset mode {mode!r}", self.script_version)
        if mode == "soft":
            raise command.error(
                "no soft reset path exists from the host; only a power-on core load is available",
                self.script_version,
            )
        # The machine is about to be replaced, so configuration bound to this
        # power-on is not a live change even when a previous reset already ran
        # (a csl_load chain resets once per script).
        self.machine_running = False
        # Apply everything folded into this power-on, in script order, before
        # the load happens.
        for position in self._fold_order.get(index, []):
            self._apply_config(self.program[position], folded_into=command.line)
        load = self._pending_load()
        if load["media_path"]:
            self._note(command, "ordering",
                       f"disk_insert from line {self._media_line} hoisted into this power-on load; "
                       "MGL mounts media as part of the core load")
        self.backend.load_core(load)
        self.machine_running = True
        self._record(command, "load_core", **load)

    def _apply_config(self, command: Command, folded_into: Optional[int] = None) -> None:
        name = command.name
        if name == "crtc_select":
            if len(command.args) != 1:
                raise command.error("crtc_select takes exactly one CRTC identifier", self.script_version)
            requested = command.args[0].upper()
            if requested in ("1", "1A", "1B"):
                bit = 0
            elif requested == "0":
                bit = 1
            elif requested in ("2", "3", "4"):
                raise command.error(
                    f"CRTC type {requested} is not implemented in this core", self.script_version
                )
            else:
                raise command.error(f"unknown CRTC identifier {requested!r}", self.script_version)
            if self.machine_running and self.crtc_status_bit is not None and bit != self.crtc_status_bit:
                raise command.error(
                    "a live CRTC change needs the OSD; the effective type is fixed at core load",
                    self.script_version,
                )
            if self.machine_running:
                self._note(command, "crtc_select",
                           f"accepted as a no-op: {requested} maps to the CRTC type already loaded")
            if requested in ("1A", "1B"):
                self._note(command, "crtc_select",
                           f"script asked for {requested}; this core has one UM6845R model and "
                           "does not distinguish 1A from 1B")
            self.crtc_requested = requested
            self.crtc_status_bit = bit

        elif name == "cpc_model":
            if len(command.args) != 1 or not command.args[0].isdigit():
                raise command.error("cpc_model takes one model number", self.script_version)
            number = int(command.args[0])
            if number not in CSL_MODEL_TO_STATUS:
                raise command.error(
                    f"cpc_model {number} selects a Plus machine; Plus CSL is out of scope for this runner",
                    self.script_version,
                )
            if self.machine_running:
                raise command.error(
                    "the model is fixed at core load; cpc_model after the first key needs the OSD",
                    self.script_version,
                )
            self.model_status = CSL_MODEL_TO_STATUS[number][0]

        elif name == "disk_dir":
            raise command.error(
                "disk_dir names a host directory this runner cannot honour; pass --disk-dir instead",
                self.script_version,
            )

        elif name == "disk_insert":
            args = list(command.args)
            slot = "S0"
            if len(args) == 2:
                drive = args.pop(0).upper()
                if drive == "A":
                    slot = "S0"
                elif drive == "B":
                    slot = "S1"
                else:
                    raise command.error(f"unknown drive {drive!r}", self.script_version)
            if len(args) != 1:
                raise command.error("disk_insert takes [drive] 'file'", self.script_version)
            name_only = args[0].replace("\\", "/").rsplit("/", 1)[-1]
            if name_only != args[0]:
                self._note(command, "media",
                           f"script path {args[0]!r} reduced to {name_only!r} under --disk-dir")
            path = f"{self.disk_dir}/{name_only}"
            try:
                validate_device_path(path, "disk_insert path")
            except ValueError as exc:
                raise command.error(str(exc), self.script_version) from exc
            self.media_path = path
            self.media_slot = slot
            self._media_line = command.line

        if folded_into is not None:
            self._record(command, "folded_into_power_on", power_on_line=folded_into)

    def _do_key_output(self, command: Command, text: str) -> None:
        self._ensure_machine(command)
        try:
            groups = translate_text(text, self.options.layout)
        except KeyTranslationError as exc:
            raise command.error(f"{exc} (layout {self.options.layout})", self.script_version) from exc
        if not groups:
            self._record(command, "no_keys")
            return

        key_wait_ms = self._key_wait_ms(command)
        if any(group.shift for group in groups):
            # SHAKER reads its menu keys straight off the CPC matrix, so a
            # SHIFT the layout needs to print the character is visible to it
            # too.  Under the French ROM every digit is shifted, which covers
            # every SHAKER menu selection.
            self._note(command, "shift",
                       f"the {self.options.layout} layout needs SHIFT for this character; "
                       "SHIFT is held in the CPC matrix while the key is down")
        if any(group.kof for group in groups):
            self._note(command, "key_delay",
                       r"\(KOF) asks for no delay before the next key; MBC always sleeps "
                       "MBC_KEY_WAIT before each event, so the delay is applied anyway")

        # CSL allows a distinct delay after a carriage return, which MBC cannot
        # express inside one sequence.  Split there and sleep on the host.
        segments: List[Tuple[List[KeyGroup], bool]] = []
        current: List[KeyGroup] = []
        for group in groups:
            current.append(group)
            if self.key_after_cr_us is not None and group.positions == ["RETURN"]:
                segments.append((current, True))
                current = []
        if current:
            segments.append((current, False))

        sent = []
        for segment, after_cr in segments:
            raw_seq = sequence_tokens(segment)
            description = "".join(group.source for group in segment)
            self.backend.send_keys(raw_seq, key_wait_ms, description)
            sent.append(raw_seq)
            self._note(command, "mbc_overhead",
                       f"MBC adds {2 * MBC_SEQUENCE_WAIT_MS}ms of uinput settling per invocation, "
                       "outside the script's timing model")
            if after_cr:
                self._sleep(self.key_after_cr_us / 1e6, "key_delay after CR", command)

        self._record(command, "keys_sent", raw_seq=sent, mbc_key_wait_ms=key_wait_ms)

    def _do_wait(self, command: Command) -> None:
        if len(command.args) != 1:
            raise command.error("wait takes one delay in microseconds", self.script_version)
        try:
            micros = float(command.args[0])
        except ValueError as exc:
            raise command.error(f"wait delay {command.args[0]!r} is not a number", self.script_version) from exc
        if micros < 0:
            raise command.error("wait delay must not be negative", self.script_version)
        seconds = micros / 1e6
        if seconds > self.options.max_wait_seconds:
            raise command.error(
                f"wait of {seconds:.1f}s exceeds the {self.options.max_wait_seconds:.0f}s bound; "
                "raise --max-wait to accept it",
                self.script_version,
            )
        self._sleep(seconds, f"csl wait {micros:.0f}us", command)
        self._record(command, "waited", seconds=seconds)

    def _do_wait_ssm0000(self, command: Command) -> None:
        """Block until the core reports an SSM #0000 newer than the last one.

        CSL uses this to let an emulated program pace the script instead of
        the script guessing. The bound is --max-wait: without one, a SHAKER
        build that never reaches the marker would hang the run.
        """
        self._ensure_machine(command)
        target = self.ssm_sync_seen + 1
        waited = 0.0
        step = max(0.05, self.options.ssm_poll_interval)
        while self.ssm_sync_seen < target:
            if waited >= self.options.max_wait_seconds:
                raise command.error(
                    f"no SSM #0000 arrived within {self.options.max_wait_seconds:.0f}s",
                    self.script_version,
                )
            self.backend.sleep(step, "wait_ssm0000 poll")
            waited += step
            if not self._consume_ssm(command) and isinstance(self.backend, PlanBackend):
                # Offline there is no core to answer, so the plan records the
                # wait rather than spinning to the bound.
                self._note(command, "ssm",
                           "wait_ssm0000 cannot be planned offline; the plan shows one poll")
                break
        self._record(command, "ssm_sync_released", polls_seconds=round(waited, 3))

    def _do_screenshot(self, command: Command) -> None:
        self._ensure_machine(command)
        if command.args and command.args[0].lower() == "vsync":
            raise command.error(
                "screenshot vsync cannot be honoured: Main captures the scaler output "
                "asynchronously and offers no VSYNC-aligned trigger",
                self.script_version,
            )
        if command.args:
            raise command.error(f"unknown screenshot option {command.args[0]!r}", self.script_version)
        self.capture_count += 1
        name = self.screenshot_name or f"csl_{self.entry_script_stem}_{self.capture_count:03d}"
        self.backend.screenshot(f"{name}.png")
        self.screenshot_name = None
        self._record(command, "captured", name=f"{name}.png")

    def _host_capture(self, command: Command) -> None:
        """Operator-requested capture at a chosen script line.

        The SHAKER CSL scripts contain no screenshot instructions at all: their
        captures come from SSM #FFFE, which needs the phase 1 detector.  This
        keeps phase 0 useful without editing the author's scripts.
        """
        self.capture_count += 1
        crtc = self.crtc_requested or "unknown"
        name = f"MISTER_{crtc}_{Path(command.script).stem}_{command.line:04d}_{self.capture_count:03d}.png"
        self.backend.screenshot(name)
        self._record(command, "host_capture", name=name)

    # --- main walk --------------------------------------------------------

    def run(self) -> Dict[str, Any]:
        self._fold_power_on()

        stop_script, stop_line = self.options.stop_at or ("", 0)
        capture_points = {(s or self.entry_script, line) for s, line in self.options.screenshot_at}

        for index, command in enumerate(self.program):
            name = command.name

            if name not in ALL_COMMANDS:
                raise command.error(f"unknown CSL instruction {name!r}", self.script_version)

            if name in REJECTED_COMMANDS:
                raise command.error(REJECTED_COMMANDS[name], self.script_version)

            if name in SSM_ONLY_COMMANDS and not self.options.ssm:
                raise command.error(SSM_ONLY_COMMANDS[name], self.script_version)

            if index in self.folded:
                # Already applied by the power-on load that owns it.
                pass
            elif name == "csl_version":
                self.script_version = command.args[0] if command.args else ""
                if self.script_version not in KNOWN_CSL_VERSIONS:
                    self.version_warning = (
                        f"script declares CSL version {self.script_version!r}; "
                        f"this runner implements {SUPPORTED_CSL_VERSION}"
                    )
                    self._note(command, "version", self.version_warning)
                self._record(command, "version_recorded", version=self.script_version)

            elif name == "reset":
                self._do_reset(command, index)

            elif name in FOLDABLE_COMMANDS:
                self._apply_config(command)
                self._record(command, "configuration_applied")

            elif name == "key_delay":
                if not 2 <= len(command.args) <= 3:
                    raise command.error("key_delay takes 2 or 3 microsecond values", self.script_version)
                try:
                    values = [float(a) for a in command.args]
                except ValueError as exc:
                    raise command.error("key_delay values must be numbers", self.script_version) from exc
                if any(v < 0 for v in values):
                    raise command.error("key_delay values must not be negative", self.script_version)
                self.key_press_us, self.key_between_us = values[0], values[1]
                self.key_after_cr_us = values[2] if len(values) == 3 else None
                self._record(command, "key_delay_set", press_us=values[0], between_us=values[1],
                             after_cr_us=self.key_after_cr_us)

            elif name == "key_output":
                if len(command.args) != 1:
                    raise command.error("key_output takes exactly one quoted string", self.script_version)
                self._do_key_output(command, command.args[0])

            elif name == "key_from_file":
                raise command.error(
                    "key_from_file names a host file this runner does not resolve; "
                    "inline the text with key_output",
                    self.script_version,
                )

            elif name == "wait":
                self._do_wait(command)

            elif name == "wait_ssm0000":
                self._do_wait_ssm0000(command)

            elif name == "screenshot_name":
                if len(command.args) != 1 or not _SAFE_NAME.match(command.args[0]):
                    raise command.error(
                        "screenshot_name takes one name of letters, digits, '_' or '-'",
                        self.script_version,
                    )
                self.screenshot_name = command.args[0]
                self._record(command, "screenshot_name_set", name=self.screenshot_name)

            elif name == "screenshot_dir":
                raise command.error(
                    "screenshot_dir names a host directory; captures land under --out-dir",
                    self.script_version,
                )

            elif name == "screenshot":
                self._do_screenshot(command)

            elif name == "csl_load":
                self._record(command, "chained")

            if (command.script, command.line) in capture_points:
                self._ensure_machine(command)
                self._host_capture(command)

            if stop_line and command.script == (stop_script or self.entry_script) and command.line >= stop_line:
                self._record(command, "stopped_at_requested_line")
                break

        return {
            "trace": self.trace,
            "approximations": self.approximations,
            "ssm_records": self.ssm_records,
            "effective_settings": {
                "layout": self.options.layout,
                "ssm_enabled": self.options.ssm,
                "ssm_base": f"0x{self.options.ssm_base:08X}" if self.options.ssm else None,
                "crtc_requested": self.crtc_requested,
                "crtc_status_bit": self.crtc_status_bit,
                "model_status": self.model_status,
                "media_path": self.media_path,
                "media_slot": self.media_slot,
                "key_press_us": self.key_press_us,
                "key_between_us": self.key_between_us,
                "key_after_cr_us": self.key_after_cr_us,
                "script_version": self.script_version,
                "supported_version": SUPPORTED_CSL_VERSION,
            },
            "version_warning": self.version_warning,
        }


def _dedupe_approximations(entries: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    """Collapse repeats of one approximation into a single counted row.

    A 200-line SHAKER walk repeats the same MBC overhead note 200 times; the
    manifest should say it once with a count, so the rows that differ stay
    visible.
    """
    seen: Dict[Tuple[str, str], Dict[str, Any]] = {}
    order: List[Tuple[str, str]] = []
    for entry in entries:
        key = (entry["kind"], entry["detail"])
        if key not in seen:
            seen[key] = {"kind": entry["kind"], "detail": entry["detail"], "count": 0,
                         "first_at": f"{entry['script']}:{entry['line']}"}
            order.append(key)
        seen[key]["count"] += 1
    return [seen[key] for key in order]


def write_outputs(out_dir: Path, manifest: Dict[str, Any]) -> None:
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    lines = [
        f"CSL run {manifest['run_id']} ({manifest['timestamp']})",
        f"script: {manifest['entry_script']}",
        f"status: {manifest['status']}",
        "",
    ]
    for entry in manifest.get("trace", []):
        lines.append(f"{entry['script']}:{entry['line']:>5}  {entry['outcome']:<24} {entry['instruction']}")
    if manifest.get("error"):
        lines.extend(["", "ERROR", json.dumps(manifest["error"], indent=2)])
    (out_dir / "last-run.log").write_text("\n".join(lines) + "\n", encoding="utf-8")


def run_csl(
    entry: Path,
    options: RunOptions,
    transport: Any = None,
    dry_run: bool = False,
    sleep_fn: Callable[[float], None] = time.sleep,
    time_fn: Callable[[], float] = time.monotonic,
) -> Dict[str, Any]:
    """Plan or execute one CSL script.  Always leaves a manifest behind."""
    out_dir = options.out_dir or Path(f"docs/references/csl-runs/{entry.stem}_{int(time.time())}")
    out_dir = Path(out_dir)
    if (out_dir / "manifest.json").exists():
        raise ValueError(f"{out_dir} already contains manifest.json; refusing to overwrite a previous run")
    out_dir.mkdir(parents=True, exist_ok=True)

    run_id = f"{int(time.time())}_{uuid.uuid4().hex[:6]}"
    manifest: Dict[str, Any] = {
        "run_id": run_id,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "entry_script": str(entry),
        "dry_run": dry_run,
        "hardware_contacted": False,
        "supported_csl_version": SUPPORTED_CSL_VERSION,
        "status": "failed",
        "error": None,
    }
    backend: Optional[Backend] = None
    runner: Optional[CslRunner] = None
    try:
        program = load_program(entry, follow_loads=options.follow_loads)
        manifest["command_count"] = len(program)
        manifest["scripts"] = sorted({c.script for c in program})

        if dry_run:
            backend = PlanBackend()
        else:
            check_pillow_installed()
            manifest["hardware_contacted"] = True
            backend = DeviceBackend(transport, options, out_dir, sleep_fn=sleep_fn, time_fn=time_fn)

        runner = CslRunner(program, options, backend, entry.name)
        manifest.update(runner.run())
        manifest["status"] = "planned" if dry_run else "success"
        return manifest
    except CslError as exc:
        manifest["error"] = exc.as_dict()
        raise
    except Exception as exc:
        manifest["error"] = {"reason": str(exc) or type(exc).__name__,
                             "supported_version": SUPPORTED_CSL_VERSION}
        raise
    finally:
        # A rejected command stops the script, so the partial trace and the
        # approximations recorded before it are the evidence for the run.
        if runner is not None:
            manifest.setdefault("trace", runner.trace)
            manifest["approximations"] = _dedupe_approximations(runner.approximations)
        if backend is not None:
            manifest["actions"] = getattr(backend, "actions", [])
            manifest["captures"] = getattr(backend, "captures", [])
        if isinstance(backend, DeviceBackend):
            try:
                manifest["cleanup"] = backend.cleanup()
            except Exception as exc:  # cleanup never replaces the primary error
                manifest["cleanup"] = {"error": str(exc)}
        if transport is not None:
            manifest["command_log"] = getattr(transport, "command_log", [])
        write_outputs(out_dir, manifest)


def _parse_point(value: str, default_script: str) -> Tuple[str, int]:
    script, _, line = value.rpartition(":")
    if not line.isdigit():
        raise argparse.ArgumentTypeError(f"expected LINE or SCRIPT:LINE, got {value!r}")
    return (script or default_script, int(line))


def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run a Logon System CSL script on a MiSTer.")
    parser.add_argument("script", type=Path, help="Entry .CSL file")
    parser.add_argument("--rbf-path", required=True, help="Core RBF under /media/fat")
    parser.add_argument("--disk-dir", required=True, help="Device directory holding the DSK files")
    parser.add_argument("--layout", default="fr", choices=list(cpc_keys.SUPPORTED_LAYOUTS),
                        help="Keyboard layout of the ROM in the machine (default fr)")
    parser.add_argument("--target", "-t", default="", help="SSH target, e.g. root@192.168.1.50")
    parser.add_argument("--port", "-p", type=int, default=22)
    parser.add_argument("--out-dir", "-o", type=Path, default=None)
    parser.add_argument("--dry-run", action="store_true",
                        help="Parse, validate and plan offline; contact nothing")
    parser.add_argument("--ack-main-cmd", action="store_true",
                        help="Confirm Main commands, PNG screenshots and /media/fat root before contact")
    parser.add_argument("--mbc-path", default="mbc")
    parser.add_argument("--cfg-path", default=DEFAULT_CFG_PATH)
    parser.add_argument("--stop-at", default=None, metavar="[SCRIPT:]LINE",
                        help="Stop after this script line")
    parser.add_argument("--screenshot-at", default=[], action="append", metavar="[SCRIPT:]LINE",
                        help="Capture after this script line (repeatable). The SHAKER scripts "
                             "carry no screenshot instructions of their own.")
    parser.add_argument("--expect-rbf-sha256", default=None)
    parser.add_argument("--expect-media-sha256", default=None)
    parser.add_argument("--max-wait", type=float, default=300.0,
                        help="Reject any single wait longer than this many seconds")
    parser.add_argument("--no-follow-loads", action="store_true",
                        help="Do not descend into csl_load targets")
    parser.add_argument("--ssm", action="store_true",
                        help="Turn on the core's SSM marker detector (OSD status bit 37), "
                             "poll its DDR3 event ring, honour wait_ssm0000 and capture on "
                             "marker #FFFE")
    parser.add_argument("--ssm-base", type=lambda v: int(v, 0), default=ssm_ring.DEFAULT_BASE,
                        help="Physical byte address of the event ring (default 0x30000000)")
    parser.add_argument("--ssm-poll", type=float, default=0.5,
                        help="Seconds between ring polls while SSM is enabled")
    return parser.parse_args(argv)


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)
    if not args.script.is_file():
        print(f"Error: script not found: {args.script}", file=sys.stderr)
        return 1

    expect = {}
    if args.expect_rbf_sha256:
        expect["rbf"] = args.expect_rbf_sha256.lower()
    if args.expect_media_sha256:
        expect["media"] = args.expect_media_sha256.lower()

    try:
        options = RunOptions(
            rbf_path=args.rbf_path,
            disk_dir=args.disk_dir,
            layout=args.layout,
            cfg_path=args.cfg_path,
            mbc_path=args.mbc_path,
            out_dir=args.out_dir,
            stop_at=_parse_point(args.stop_at, args.script.name) if args.stop_at else None,
            screenshot_at=[_parse_point(v, args.script.name) for v in args.screenshot_at],
            expect_sha256=expect,
            max_wait_seconds=args.max_wait,
            follow_loads=not args.no_follow_loads,
            ssm=args.ssm,
            ssm_base=args.ssm_base,
            ssm_poll_interval=args.ssm_poll,
        )
    except (ValueError, argparse.ArgumentTypeError) as exc:
        print(f"Configuration error: {exc}", file=sys.stderr)
        return 1

    transport = None
    if not args.dry_run:
        if not args.ack_main_cmd:
            print("Error: --ack-main-cmd is required before contacting the device.", file=sys.stderr)
            return 1
        if not args.target:
            print("Error: --target is required unless --dry-run is used.", file=sys.stderr)
            return 1
        transport = SSHTransport(args.target, args.port)

    try:
        manifest = run_csl(args.script, options, transport=transport, dry_run=args.dry_run)
    except CslError as exc:
        print("CSL script stopped:", file=sys.stderr)
        print(json.dumps(exc.as_dict(), indent=2), file=sys.stderr)
        return 2
    except Exception as exc:
        print(f"Run failed: {exc}", file=sys.stderr)
        return 1

    print(f"status={manifest['status']} commands={manifest.get('command_count')} "
          f"captures={len(manifest.get('captures', []))} "
          f"ssm_records={len(manifest.get('ssm_records', []))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
