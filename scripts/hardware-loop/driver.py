#!/usr/bin/env python3
"""MiSTer hardware test loop host driver (B2 slice).

Coordinates test core launching, media mounting, input sequence injection,
and repeatable native screenshot capture over SSH using existing MiSTer facilities
(/dev/MiSTer_cmd, MGL, MBC raw_seq).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import shlex
import subprocess
import sys
import time
import uuid
import xml.etree.ElementTree as ET
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Set, Tuple


class DriverError(Exception):
    """Base error for test loop failures."""


# --- Validation Helpers ---

_TARGET_REGEX = re.compile(r"^[a-zA-Z0-9._\-@]+$")
_SCREENSHOT_NAME_REGEX = re.compile(r"^[a-zA-Z0-9_\-]+\.png$")
_HEX_REGEX = re.compile(r"^[0-9A-Fa-f]{2}$")
_SHA256_REGEX = re.compile(r"^[0-9a-fA-F]{64}$")


def validate_ssh_target(target: str) -> None:
    """Validate SSH target hostname or user@host. Rejects option-like strings and injection."""
    if not target:
        raise ValueError("SSH target cannot be empty.")
    if target.startswith("-"):
        raise ValueError(f"SSH target cannot start with '-' (option-like argument rejected): {target!r}")
    if not _TARGET_REGEX.match(target):
        raise ValueError(f"SSH target contains invalid or unsafe characters: {target!r}")


def validate_port(port: int) -> None:
    """Validate TCP port number."""
    if isinstance(port, bool) or not isinstance(port, int) or not (1 <= port <= 65535):
        raise ValueError(f"Invalid port number: {port}. Must be an integer between 1 and 65535.")


def validate_device_path(path: Any, name: str) -> None:
    """Validate absolute /media/fat device paths without parent traversal."""
    if not isinstance(path, str) or not path:
        raise ValueError(f"{name} must be a non-empty string path.")
    if len(path) > 255:
        raise ValueError(f"{name} exceeds maximum length of 255 characters.")
    if not path.startswith("/media/fat/"):
        raise ValueError(f"{name} must be an absolute path starting with /media/fat/: {path!r}")
    norm = os.path.normpath(path)
    if norm != path or ".." in path.split("/"):
        raise ValueError(f"{name} contains parent directory traversal: {path!r}")
    if any(ord(c) < 32 for c in path):
        raise ValueError(f"{name} contains control characters: {path!r}")


def validate_screenshot_name(name: str) -> None:
    """Validate screenshot filename. Must end in .png and contain safe characters."""
    if not name or name.startswith("-") or "/" in name or "\\" in name:
        raise ValueError(f"Invalid screenshot name: {name!r}")
    if not _SCREENSHOT_NAME_REGEX.match(name):
        raise ValueError(f"Screenshot name must end with '.png' and be alphanumeric with '_' or '-': {name!r}")


def validate_raw_seq(seq: str) -> Tuple[List[str], Set[str]]:
    """Validate MBC raw_seq tokens and track all keys potentially held on interruption.

    Pinned MBC source (mbc.c:553-609) rejects whitespace. Unbounded wait (!m) is rejected.
    Presses {XX and releases }XX must be balanced within the sequence.
    """
    if not seq or len(seq) > 1024:
        raise ValueError("raw_seq must contain 1..1024 ASCII characters.")
    if any(c.isspace() for c in seq):
        raise ValueError("Whitespace is rejected by MBC raw_seq parser; omit whitespace or join tokens.")
    if "!m" in seq:
        raise ValueError("Unbounded mount wait '!m' is rejected in raw_seq.")

    tokens: List[str] = []
    currently_held: Set[str] = set()
    potentially_held: Set[str] = set()
    wait_count, idx = 0, 0

    while idx < len(seq):
        c = seq[idx]
        if c == ":":
            if idx + 2 >= len(seq) or not _HEX_REGEX.match(seq[idx + 1 : idx + 3]):
                raise ValueError(f"Invalid hex key at index {idx} in {seq!r}")
            tokens.append(f":{seq[idx+1:idx+3].upper()}")
            idx += 3
        elif c in ("{", "}"):
            if idx + 2 >= len(seq) or not _HEX_REGEX.match(seq[idx + 1 : idx + 3]):
                raise ValueError(f"Invalid key token at index {idx} in {seq!r}")
            k = seq[idx + 1 : idx + 3].upper()
            if c == "{":
                if k in currently_held:
                    raise ValueError(f"Key {k} pressed while already held at index {idx}")
                currently_held.add(k)
                potentially_held.add(k)
                tokens.append(f"{{{k}")
            else:
                if k not in currently_held:
                    raise ValueError(f"Key {k} released while not held at index {idx}")
                currently_held.remove(k)
                tokens.append(f"}}{k}")
            idx += 3
        elif c == "!":
            if idx + 1 >= len(seq) or seq[idx + 1] != "s":
                raise ValueError(f"Unsupported wait token at index {idx} in {seq!r}; only '!s' allowed")
            wait_count += 1
            if wait_count > 30:
                raise ValueError("Too many '!s' wait tokens (exceeds limit of 30)")
            tokens.append("!s")
            idx += 2
        elif c in "UDLROEHFMabcdefghijklmnopqrstuvwxyz0123456789":
            tokens.append(c)
            idx += 1
        else:
            raise ValueError(f"Unsupported MBC raw_seq character {c!r} at index {idx} in {seq!r}")

    if currently_held:
        raise ValueError(f"Unbalanced key sequence: keys {sorted(currently_held)} held without release.")

    return tokens, potentially_held


def _check_num(val: Any, name: str, default: float, min_val: float, max_val: float, is_int: bool = False) -> Any:
    if val is None:
        return default
    if isinstance(val, bool) or not isinstance(val, (int, float)) or not math.isfinite(val) or val < min_val or val > max_val:
        raise ValueError(f"{name} must be finite number between {min_val} and {max_val}, got {val!r}")
    if is_int and not isinstance(val, int):
        raise ValueError(f"{name} must be an integer")
    return int(val) if is_int else float(val)


def validate_case_config(data: Any) -> Dict[str, Any]:
    """Validate JSON case definition against schema and finite numeric bounds."""
    if not isinstance(data, dict):
        raise ValueError("Case configuration must be a JSON dictionary.")

    unknown = set(data.keys()) - {
        "case_id", "description", "rbf_path", "media", "declared_settings",
        "boot_delay", "settle_delay", "capture_delay", "captures_count",
        "input", "timeouts", "ack_main_cmd", "mbc_path", "expected_sha256"
    }
    if unknown:
        raise ValueError(f"Unknown top-level configuration keys: {sorted(unknown)}")

    case_id = data.get("case_id")
    if not isinstance(case_id, str) or not (1 <= len(case_id) <= 64) or not re.match(r"^[a-zA-Z0-9_\-]+$", case_id):
        raise ValueError(f"Invalid case_id: {case_id!r}. Must be alphanumeric with '_' or '-', 1-64 chars.")

    rbf_path = data.get("rbf_path")
    validate_device_path(rbf_path, "rbf_path")
    if not rbf_path.lower().endswith(".rbf"):
        raise ValueError(f"rbf_path must end with .rbf: {rbf_path!r}")

    media = data.get("media")
    if not isinstance(media, dict) or (set(media.keys()) - {"type", "slot", "path"}):
        raise ValueError("media must be a dictionary with keys: type, slot, path.")

    m_type, m_slot = media.get("type"), media.get("slot")
    if m_type == "dsk" and m_slot not in ("S0", "S1"):
        raise ValueError(f"DSK slot must be 'S0' or 'S1', got {m_slot!r}")
    elif m_type == "cpr" and m_slot != "F8":
        raise ValueError(f"CPR slot must be 'F8', got {m_slot!r}")
    elif m_type not in ("dsk", "cpr"):
        raise ValueError(f"Unsupported media type: {m_type!r}. Must be 'dsk' or 'cpr'.")
    validate_device_path(media.get("path"), "media path")

    declared = data.get("declared_settings", {})
    if not isinstance(declared, dict):
        raise ValueError("declared_settings must be an object.")

    expected = data.get("expected_sha256", {})
    if not isinstance(expected, dict) or set(expected) - {"rbf", "media"}:
        raise ValueError("expected_sha256 must be an object with optional rbf/media hashes.")
    if any(not isinstance(value, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", value)
           for value in expected.values()):
        raise ValueError("expected_sha256 values must be 64 hexadecimal characters.")
    expected = {key: value.lower() for key, value in expected.items()}

    boot_delay = _check_num(data.get("boot_delay"), "boot_delay", 5.0, 0.0, 300.0)
    settle_delay = _check_num(data.get("settle_delay"), "settle_delay", 2.0, 0.0, 300.0)
    capture_delay = _check_num(data.get("capture_delay"), "capture_delay", 1.0, 0.0, 300.0)
    cc = _check_num(data.get("captures_count"), "captures_count", 3, 1, 10, is_int=True)

    raw_seq = None
    input_obj = data.get("input")
    if input_obj is not None:
        if not isinstance(input_obj, dict) or (set(input_obj.keys()) - {"raw_seq"}):
            raise ValueError("input must be a dictionary with optional key 'raw_seq'.")
        if input_obj.get("raw_seq") is not None:
            if not isinstance(input_obj["raw_seq"], str):
                raise ValueError("input.raw_seq must be a string.")
            raw_seq = input_obj["raw_seq"]
            validate_raw_seq(raw_seq)

    timeouts = data.get("timeouts", {})
    if not isinstance(timeouts, dict) or (set(timeouts.keys()) - {"cmd_timeout", "capture_timeout", "poll_interval", "max_retries"}):
        raise ValueError("timeouts must be a dictionary with valid timeout keys.")

    cmd_timeout = _check_num(timeouts.get("cmd_timeout"), "cmd_timeout", 10.0, 0.1, 600.0)
    capture_timeout = _check_num(timeouts.get("capture_timeout"), "capture_timeout", 15.0, 0.1, 600.0)
    poll_interval = _check_num(timeouts.get("poll_interval"), "poll_interval", 0.5, 0.01, 60.0)
    if poll_interval > capture_timeout:
        raise ValueError("poll_interval cannot exceed capture_timeout")
    max_retries = _check_num(timeouts.get("max_retries"), "max_retries", 3, 1, 20, is_int=True)

    ack_val = data.get("ack_main_cmd", False)
    if not isinstance(ack_val, bool):
        raise ValueError(f"ack_main_cmd must be a strict boolean (true/false), got {type(ack_val).__name__} ({ack_val!r})")

    mbc_path = data.get("mbc_path", "mbc")
    if not isinstance(mbc_path, str) or not (1 <= len(mbc_path) <= 255) or mbc_path.startswith("-") or not re.fullmatch(r"[a-zA-Z0-9_./-]+", mbc_path):
        raise ValueError(f"Invalid mbc_path: {mbc_path!r}")

    return {
        "case_id": case_id,
        "description": str(data.get("description", "")),
        "rbf_path": rbf_path,
        "media": {"type": m_type, "slot": m_slot, "path": media["path"]},
        "declared_settings": declared,
        "expected_sha256": expected,
        "boot_delay": boot_delay,
        "settle_delay": settle_delay,
        "capture_delay": capture_delay,
        "captures_count": cc,
        "input": {"raw_seq": raw_seq} if raw_seq is not None else {},
        "timeouts": {
            "cmd_timeout": cmd_timeout,
            "capture_timeout": capture_timeout,
            "poll_interval": poll_interval,
            "max_retries": max_retries,
        },
        "ack_main_cmd": ack_val,
        "mbc_path": mbc_path,
    }


# --- MGL & PNG Helpers ---

def generate_mgl_xml(rbf_path: str, media_type: str, media_slot: str, media_path: str, delay: int = 1) -> str:
    """Generate official MiSTer Game Launcher (MGL) XML content using stdlib ElementTree."""
    clean_rbf = rbf_path
    if clean_rbf.startswith("/media/fat/"):
        clean_rbf = clean_rbf[len("/media/fat/") :]
    if clean_rbf.lower().endswith(".rbf"):
        clean_rbf = clean_rbf[:-4]

    mgl_type = "s" if media_type == "dsk" else "f"
    mgl_index = "0" if media_slot == "S0" else ("1" if media_slot == "S1" else "8")

    root = ET.Element("mistergamedescription")
    ET.SubElement(root, "rbf").text = clean_rbf
    ET.SubElement(root, "file", {"delay": str(delay), "type": mgl_type, "index": mgl_index, "path": media_path})
    return ET.tostring(root, encoding="utf-8").decode("utf-8") + "\n"


def check_pillow_installed() -> None:
    """Check that Pillow is installed before mutating device state."""
    try:
        import PIL.Image  # noqa: F401
    except ImportError as exc:
        raise RuntimeError("Pillow is required for PNG verification. Install with 'pip install Pillow'.") from exc


def verify_png(local_path: Path) -> Tuple[int, int]:
    """Verify PNG file integrity and load complete image data using Pillow."""
    from PIL import Image, UnidentifiedImageError

    try:
        with Image.open(local_path) as img:
            img.verify()
        with Image.open(local_path) as img:
            img.load()
            if img.format != "PNG":
                raise DriverError(f"Downloaded image format is {img.format!r}, expected 'PNG'.")
            if img.size[0] <= 0 or img.size[1] <= 0:
                raise DriverError(f"Degenerate image dimensions: {img.size}")
            return img.size
    except (UnidentifiedImageError, OSError, ValueError) as exc:
        raise DriverError(f"PNG verification failed on {local_path}: {exc}") from exc


# --- Transport Layer ---

class CommandResult:
    __slots__ = ("exit_code", "stdout", "stderr")

    def __init__(self, exit_code: int, stdout: str, stderr: str):
        self.exit_code = exit_code
        self.stdout = stdout
        self.stderr = stderr


class SSHTransport:
    """SSH and SCP communication transport."""

    def __init__(self, target: str, port: int = 22):
        validate_ssh_target(target)
        validate_port(port)
        self.target = target
        self.port = port
        self.command_log: List[Dict[str, Any]] = []

    def _log_record(self, cmd: str, ts: str, dur: float, code: int, out: str, err: str) -> None:
        self.command_log.append({
            "command": cmd, "timestamp": ts, "duration_ms": dur,
            "exit_code": code, "stdout": out, "stderr": err,
        })

    def run_cmd(self, cmd: str, timeout: float) -> CommandResult:
        # Bound the remote process as well as the local SSH client. A disconnected
        # client can leave its remote command alive; the timer stays on the device.
        remote = (
            "command -v timeout >/dev/null || { echo 'Missing device timeout utility' >&2; exit 127; }; "
            f"exec timeout -s KILL {max(1, math.ceil(timeout))} sh -c {shlex.quote(cmd)}"
        )
        ssh_cmd = ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=10", "-p", str(self.port), self.target, remote]
        start = time.monotonic()
        ts = datetime.now(timezone.utc).isoformat()
        try:
            res = subprocess.run(ssh_cmd, capture_output=True, text=True, timeout=timeout)
            dur = (time.monotonic() - start) * 1000.0
            self._log_record(cmd, ts, dur, res.returncode, res.stdout, res.stderr)
            if res.returncode == 255:
                raise DriverError(f"SSH connection failed to {self.target} (code 255): {res.stderr.strip()}")
            return CommandResult(res.returncode, res.stdout, res.stderr)
        except subprocess.TimeoutExpired as exc:
            dur = (time.monotonic() - start) * 1000.0
            self._log_record(cmd, ts, dur, -1, "", f"TimeoutExpired after {timeout}s")
            raise DriverError(f"SSH command timed out after {timeout}s: {cmd}") from exc

    def _run_scp(self, scp_args: List[str], log_cmd: str, timeout: float, is_upload: bool) -> None:
        start = time.monotonic()
        ts = datetime.now(timezone.utc).isoformat()
        try:
            res = subprocess.run(scp_args, capture_output=True, text=True, timeout=timeout)
            dur = (time.monotonic() - start) * 1000.0
            self._log_record(log_cmd, ts, dur, res.returncode, res.stdout, res.stderr)
            if res.returncode != 0:
                action = "upload" if is_upload else "download"
                raise DriverError(f"SCP {action} failed ({res.returncode}): {res.stderr.strip()}")
        except subprocess.TimeoutExpired as exc:
            dur = (time.monotonic() - start) * 1000.0
            self._log_record(log_cmd, ts, dur, -1, "", f"TimeoutExpired after {timeout}s")
            raise DriverError(f"SCP timed out after {timeout}s") from exc

    def upload_file(self, local_path: Path, remote_path: str, timeout: float) -> None:
        validate_device_path(remote_path, "upload remote_path")
        cmd = ["scp", "-O", "-P", str(self.port), "-o", "BatchMode=yes", str(local_path.resolve()), f"{self.target}:{shlex.quote(remote_path)}"]
        self._run_scp(cmd, f"scp {local_path} -> {self.target}:{remote_path}", timeout, is_upload=True)

    def download_file(self, remote_path: str, local_path: Path, timeout: float) -> None:
        validate_device_path(remote_path, "download remote_path")
        cmd = ["scp", "-O", "-P", str(self.port), "-o", "BatchMode=yes", f"{self.target}:{shlex.quote(remote_path)}", str(local_path.resolve())]
        self._run_scp(cmd, f"scp {self.target}:{remote_path} -> {local_path}", timeout, is_upload=False)


# --- Device Identity & Key Release ---

def check_rbf_directory(transport: Any, rbf_path: str, timeout: float) -> str:
    """Verify RBF prefix uniqueness matching Main's get_rbf selection behavior.

    Source: support/arcade/mra_loader.cpp:1224-1298 at Main f8dc68e (get_rbf in Main_MiSTer).
    """
    p = Path(rbf_path)
    dirname, stem, basename = str(p.parent), p.stem, p.name

    res = transport.run_cmd(f"ls -1 {shlex.quote(dirname)}", timeout=timeout)
    if res.exit_code != 0:
        raise DriverError(f"Cannot list RBF directory {dirname}: {res.stderr.strip()}")

    matches = [
        e.strip() for e in res.stdout.splitlines()
        if e.strip().lower().endswith(".rbf") and len(e.strip()) > len(stem)
        and e.strip()[: len(stem)].lower() == stem.lower() and e.strip()[len(stem)] in (".", "_")
    ]
    if not matches:
        raise DriverError(f"No RBF file matching stem {stem!r} found in {dirname}")

    matches.sort()
    selected = matches[-1]
    if len(matches) > 1:
        raise DriverError(f"Ambiguous RBF prefix collision in {dirname}: matches {matches}. Main selects {selected!r}.")
    if selected != basename:
        raise DriverError(f"Directory contains {selected!r} matching stem {stem!r}, but requested RBF was {basename!r}.")
    return selected


def fetch_sha256(transport: Any, file_path: str, timeout: float) -> str:
    """Hash remote file and validate sha256 output format strictly."""
    res = transport.run_cmd(f"sha256sum {shlex.quote(file_path)}", timeout=timeout)
    if res.exit_code != 0:
        raise DriverError(f"sha256sum failed for {file_path} ({res.exit_code}): {res.stderr.strip()}")
    parts = res.stdout.strip().split()
    if not parts or not _SHA256_REGEX.match(parts[0]):
        raise DriverError(f"Malformed sha256sum output for {file_path}: {res.stdout!r}")
    return parts[0].lower()


def emergency_release_keys(transport: Any, mbc_path: str, keys: Set[str], timeout: float) -> Dict[str, Any]:
    """Conservative best-effort release of all virtual keys potentially held on interruption."""
    if not keys:
        return {"attempted": False, "keys": []}
    rel_cmd = f"exec env MBC_KEY_WAIT=40 MBC_SEQUENCE_WAIT=1000 {shlex.quote(mbc_path)} raw_seq {shlex.quote(''.join(f'}}{k}' for k in sorted(keys)))}"
    try:
        res = transport.run_cmd(rel_cmd, timeout=timeout)
        return {
            "attempted": True, "keys": sorted(keys), "exit_code": res.exit_code,
            "uncertainty_note": "Best-effort release sent; new MBC process creates new uinput device, physical release not guaranteed.",
        }
    except Exception as exc:
        return {"attempted": True, "keys": sorted(keys), "error": str(exc), "uncertainty_note": "Emergency release dispatch failed."}


# --- Hardware Loop Orchestration ---

def run_hardware_loop(
    transport: Any,
    config: Dict[str, Any],
    out_dir: Path,
    target: str = "",
    port: int = 22,
    dry_run: bool = False,
    time_fn: Callable[[], float] = time.monotonic,
    sleep_fn: Callable[[float], None] = time.sleep,
) -> Dict[str, Any]:
    """Execute the hardware capture loop or generate a planned dry-run."""
    run_id = f"{int(time.time())}_{uuid.uuid4().hex[:6]}"
    ts = datetime.now(timezone.utc).isoformat()

    if (out_dir / "manifest.json").exists():
        raise ValueError(f"Output directory {out_dir} already contains manifest.json; refusing to overwrite previous run.")
    out_dir.mkdir(parents=True, exist_ok=True)

    local_mgl = out_dir / f"{config['case_id']}_{run_id}.mgl"
    local_mgl.write_text(generate_mgl_xml(config["rbf_path"], config["media"]["type"], config["media"]["slot"], config["media"]["path"]), encoding="utf-8")
    remote_mgl = f"/media/fat/autoloop_{config['case_id']}_{run_id}.mgl"

    raw_seq = config["input"].get("raw_seq")
    potentially_held = validate_raw_seq(raw_seq)[1] if raw_seq else set()
    cmd_timeout = config["timeouts"]["cmd_timeout"]

    seq = ["preflight_checks", f"upload_mgl {local_mgl.name} -> {remote_mgl}", f"load_core {remote_mgl} via /dev/MiSTer_cmd", f"boot_delay {config['boot_delay']}s"]
    if raw_seq:
        seq.extend([f"inject_input mbc raw_seq {raw_seq}", f"settle_delay {config['settle_delay']}s"])
    for i in range(1, config["captures_count"] + 1):
        s_name = f"capture_{config['case_id']}_{run_id}_c{i}.png"
        seq.append(f"capture_{i} {s_name} via /dev/MiSTer_cmd and poll")
        if i < config["captures_count"]:
            seq.append(f"capture_delay {config['capture_delay']}s")
    seq.append(f"cleanup_remote_mgl {remote_mgl}")

    if dry_run:
        plan = [
            {"command": "Remote shell commands use timeout -s KILL; host SSH/SCP calls also have deadlines"},
            {"command": "test -p /dev/MiSTer_cmd && test -w /dev/MiSTer_cmd"},
            {"command": "cat /media/fat/MiSTer.ini (optional on-disk snapshot)"},
            {"command": "mkdir -p /media/fat/screenshots && test -d /media/fat/screenshots && test -w /media/fat/screenshots"},
            {"command": f"ls -1 {shlex.quote(str(Path(config['rbf_path']).parent))}"},
            {"command": f"test -f {shlex.quote(config['media']['path'])}"},
            {"command": f"sha256sum {shlex.quote(config['rbf_path'])}"},
            {"command": f"sha256sum {shlex.quote(config['media']['path'])}"},
            {"command": "sha256sum /media/fat/MiSTer"},
            {"command": f"scp {local_mgl} -> target:{remote_mgl}"},
            {"command": f"printf '%s\\n' {shlex.quote(f'load_core {remote_mgl}')} > /dev/MiSTer_cmd"},
            {"command": f"SLEEP {config['boot_delay']}s (boot delay)"},
        ]
        if raw_seq:
            plan.extend([{"command": f"{shlex.quote(config['mbc_path'])} raw_seq {shlex.quote(raw_seq)}"}, {"command": f"SLEEP {config['settle_delay']}s (settle delay)"}])
        for i in range(1, config["captures_count"] + 1):
            sn = f"capture_{config['case_id']}_{run_id}_c{i}.png"
            plan.extend([
                {"command": f"test ! -e /media/fat/screenshots/{sn}"},
                {"command": f"printf '%s\\n' {shlex.quote(f'screenshot {sn}')} > /dev/MiSTer_cmd"},
                {"command": f"poll & scp target:/media/fat/screenshots/{sn} -> {out_dir / sn}"},
            ])
            if i < config["captures_count"]:
                plan.append({"command": f"SLEEP {config['capture_delay']}s (capture delay)"})
        plan.append({"command": f"rm -f {remote_mgl}"})

        manifest = {
            "case_id": config["case_id"], "run_id": run_id, "timestamp": ts, "dry_run": True,
            "hardware_contacted": False, "target": target, "port": port, "effective_case": config,
            "intended_sequence": seq, "partial_identity": {}, "captures": [], "command_log": plan,
            "status": "dry_run", "error": None,
        }
        (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
        return manifest

    partial_identity: Dict[str, Any] = {}
    captures: List[Dict[str, Any]] = []
    input_dispatched = input_completed = remote_mgl_attempted = hardware_contacted = False

    try:
        if not config["ack_main_cmd"]:
            raise DriverError("Main command support, active PNG format and /media/fat root must be acknowledged via --ack-main-cmd before contact.")
        check_pillow_installed()
        hardware_contacted = True
        # Preflight checks
        if transport.run_cmd("test -p /dev/MiSTer_cmd && test -w /dev/MiSTer_cmd", timeout=cmd_timeout).exit_code != 0:
            raise DriverError("/dev/MiSTer_cmd FIFO missing or not writable; Main software may not be running.")
        partial_identity["mister_cmd_device_writable"] = True

        ini_res = transport.run_cmd("cat /media/fat/MiSTer.ini", timeout=cmd_timeout)
        partial_identity["on_disk_mister_ini"] = {
            "available": ini_res.exit_code == 0, "text": ini_res.stdout,
            "note": "On-disk snapshot only; sections and alternate INI files can differ from active configuration.",
        }
        partial_identity["acknowledged_capture_contract"] = {"root": "/media/fat", "format": "png"}

        if transport.run_cmd("mkdir -p /media/fat/screenshots && test -d /media/fat/screenshots && test -w /media/fat/screenshots", timeout=cmd_timeout).exit_code != 0:
            raise DriverError("Screenshot directory /media/fat/screenshots missing or not writable.")
        partial_identity["screenshot_dir_writable"] = True

        if raw_seq:
            if transport.run_cmd(f"which {shlex.quote(config['mbc_path'])} || test -x {shlex.quote(config['mbc_path'])}", timeout=cmd_timeout).exit_code != 0:
                raise DriverError(f"MBC executable not found at '{config['mbc_path']}'.")
            if transport.run_cmd("test -c /dev/uinput && test -w /dev/uinput", timeout=cmd_timeout).exit_code != 0:
                raise DriverError("/dev/uinput character device missing or not writable; MBC cannot create virtual input.")
            partial_identity["input_prerequisites_writable"] = True

        check_rbf_directory(transport, config["rbf_path"], timeout=cmd_timeout)
        partial_identity["rbf_prefix_unambiguous"] = True
        partial_identity["load_confirmation"] = "Requested file selection only. Main does not acknowledge load completion; verify model/test footer in captures."

        if transport.run_cmd(f"test -f {shlex.quote(config['media']['path'])}", timeout=cmd_timeout).exit_code != 0:
            raise DriverError(f"Declared media file not found: '{config['media']['path']}'.")

        partial_identity["rbf_sha256"] = fetch_sha256(transport, config["rbf_path"], cmd_timeout)
        partial_identity["media_sha256"] = fetch_sha256(transport, config["media"]["path"], cmd_timeout)
        for kind, expected in config.get("expected_sha256", {}).items():
            actual = partial_identity[kind + "_sha256"]
            if actual != expected:
                raise DriverError(f"{kind} SHA-256 mismatch: expected {expected}, got {actual}")
        partial_identity["mister_binary_on_disk_sha256"] = fetch_sha256(transport, "/media/fat/MiSTer", cmd_timeout)
        partial_identity["mister_binary_note"] = "SHA256 of /media/fat/MiSTer on disk verifies stored file identity; does not prove running binary."

        remote_mgl_attempted = True
        transport.upload_file(local_mgl, remote_mgl, timeout=cmd_timeout)

        load_res = transport.run_cmd(f"printf '%s\\n' {shlex.quote(f'load_core {remote_mgl}')} > /dev/MiSTer_cmd", timeout=cmd_timeout)
        if load_res.exit_code != 0:
            raise DriverError(f"load_core command failed ({load_res.exit_code}): {load_res.stderr.strip()}")
        sleep_fn(config["boot_delay"])

        if raw_seq:
            input_dispatched = True
            mbc_res = transport.run_cmd(f"exec env MBC_KEY_WAIT=40 MBC_SEQUENCE_WAIT=1000 {shlex.quote(config['mbc_path'])} raw_seq {shlex.quote(raw_seq)}", timeout=cmd_timeout)
            if mbc_res.exit_code != 0 or "error - " in mbc_res.stdout.lower() or "error - " in mbc_res.stderr.lower():
                raise DriverError(f"MBC raw_seq failed ({mbc_res.exit_code}): stdout={mbc_res.stdout!r} stderr={mbc_res.stderr!r}")
            input_completed = True
            sleep_fn(config["settle_delay"])

        for i in range(1, config["captures_count"] + 1):
            shot_name = f"capture_{config['case_id']}_{run_id}_c{i}.png"
            validate_screenshot_name(shot_name)
            remote_shot, local_shot = f"/media/fat/screenshots/{shot_name}", out_dir / shot_name

            # Unique run names must not overwrite a pre-existing capture.
            if transport.run_cmd(f"test ! -e {shlex.quote(remote_shot)}", timeout=cmd_timeout).exit_code != 0:
                raise DriverError(f"Screenshot path already exists or cannot be checked: {remote_shot}")
            if transport.run_cmd(f"printf '%s\\n' {shlex.quote(f'screenshot {shot_name}')} > /dev/MiSTer_cmd", timeout=cmd_timeout).exit_code != 0:
                raise DriverError("screenshot command failed")

            deadline = time_fn() + config["timeouts"]["capture_timeout"]
            corruption_retries, poll_attempts = 0, 0
            verified_dims, last_err = None, None

            while time_fn() < deadline:
                poll_attempts += 1
                rem = deadline - time_fn()
                if rem <= 0:
                    break
                try:
                    stat = transport.run_cmd(f"test -f {shlex.quote(remote_shot)}", timeout=min(cmd_timeout, rem))
                    if stat.exit_code == 1:
                        sleep_fn(min(config["timeouts"]["poll_interval"], max(0.0, deadline - time_fn())))
                        continue
                    if stat.exit_code != 0:
                        raise DriverError(f"Capture file check failed ({stat.exit_code}): {stat.stderr}")
                    rem = deadline - time_fn()
                    if rem <= 0:
                        break
                    transport.download_file(remote_shot, local_shot, timeout=min(cmd_timeout, rem))
                    dims = verify_png(local_shot)
                    if time_fn() >= deadline:
                        last_err = "Capture completed after its deadline"
                        break
                    verified_dims = dims
                    break
                except Exception as exc:
                    corruption_retries += 1
                    last_err = str(exc)
                    if corruption_retries >= config["timeouts"]["max_retries"]:
                        break
                    sleep_fn(min(config["timeouts"]["poll_interval"], max(0.0, deadline - time_fn())))

            if verified_dims is None:
                err_msg = last_err or f"Capture {shot_name} timed out after {config['timeouts']['capture_timeout']}s"
                captures.append({"index": i, "name": shot_name, "remote_path": remote_shot, "local_path": str(local_shot), "sha256": "", "dimensions": [0, 0], "poll_attempts": poll_attempts, "status": "failed", "error": err_msg})
                raise DriverError(err_msg)

            sha = hashlib.sha256(local_shot.read_bytes()).hexdigest()
            captures.append({"index": i, "name": shot_name, "remote_path": remote_shot, "local_path": str(local_shot), "sha256": sha, "dimensions": list(verified_dims), "poll_attempts": poll_attempts, "status": "completed", "error": None})
            if i < config["captures_count"]:
                sleep_fn(config["capture_delay"])

        manifest = {
            "case_id": config["case_id"], "run_id": run_id, "timestamp": ts, "dry_run": False,
            "hardware_contacted": hardware_contacted, "target": target, "port": port, "effective_case": config,
            "intended_sequence": seq, "partial_identity": partial_identity, "captures": captures,
            "command_log": transport.command_log, "status": "success", "error": None,
        }
        (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
        return manifest

    except (Exception, KeyboardInterrupt) as exc:
        manifest = {
            "case_id": config["case_id"], "run_id": run_id, "timestamp": ts, "dry_run": False,
            "hardware_contacted": hardware_contacted, "target": target, "port": port, "effective_case": config,
            "intended_sequence": seq, "partial_identity": partial_identity, "captures": captures,
            "command_log": getattr(transport, "command_log", []), "status": "failed", "error": str(exc) or type(exc).__name__,
        }
        (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
        raise
    finally:
        # Finish the on-disk evidence after cleanup. A cleanup failure does not
        # replace the primary error or turn completed captures into a hardware pass.
        if input_dispatched and not input_completed:
            manifest["input_uncertainty"] = "Input was interrupted; inspect/reset the target before another case."
            manifest["emergency_key_release"] = emergency_release_keys(
                transport, config["mbc_path"], potentially_held, timeout=cmd_timeout)
        manifest["cleanup"] = {"attempted": remote_mgl_attempted}
        if remote_mgl_attempted:
            try:
                res = transport.run_cmd(f"rm -f {shlex.quote(remote_mgl)}", timeout=cmd_timeout)
                manifest["cleanup"].update(exit_code=res.exit_code, stderr=res.stderr)
            except Exception as exc:
                manifest["cleanup"]["error"] = str(exc)
        manifest["command_log"] = getattr(transport, "command_log", [])
        (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")


# --- CLI Interface ---

def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="MiSTer hardware test loop host driver (B2 slice).")
    parser.add_argument("case", type=Path, help="Path to JSON test case definition")
    parser.add_argument("--target", "-t", type=str, default="", help="SSH target (e.g. root@192.168.1.50)")
    parser.add_argument("--port", "-p", type=int, default=22, help="SSH port (default 22)")
    parser.add_argument("--out-dir", "-o", type=Path, default=None, help="Output directory for captures & manifest")
    parser.add_argument("--dry-run", action="store_true", help="Validate case and print execution plan without contacting device")
    parser.add_argument("--ack-main-cmd", action="store_true", help="Confirm installed Main commands, active PNG screenshots and /media/fat root")
    parser.add_argument("--mbc-path", type=str, default=None, help="Override MBC executable path on device")
    return parser.parse_args(argv)


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)
    if not args.case.exists():
        print(f"Error: Case file does not exist: {args.case}", file=sys.stderr)
        return 1

    try:
        raw_case = json.loads(args.case.read_text(encoding="utf-8"))
        if args.ack_main_cmd:
            raw_case["ack_main_cmd"] = True
        if args.mbc_path:
            raw_case["mbc_path"] = args.mbc_path
        config = validate_case_config(raw_case)
    except Exception as exc:
        print(f"Configuration error: {exc}", file=sys.stderr)
        return 1

    out_dir = (args.out_dir or Path(f"docs/references/hardware-loop/{config['case_id']}_{int(time.time())}")).resolve()

    if args.dry_run:
        manifest = run_hardware_loop(None, config, out_dir, target=args.target, port=args.port, dry_run=True)
        print(f"Dry-run plan written to: {out_dir / 'manifest.json'}")
        print(json.dumps(manifest, indent=2))
        return 0

    if not args.target:
        print("Error: --target is required for device execution (or use --dry-run).", file=sys.stderr)
        return 1

    try:
        transport = SSHTransport(args.target, port=args.port)
        manifest = run_hardware_loop(transport, config, out_dir, target=args.target, port=args.port, dry_run=False)
        print(f"Capture completed successfully. Manifest: {out_dir / 'manifest.json'}")
        for cap in manifest["captures"]:
            print(f"  - {cap['name']}: {cap['dimensions']} SHA256: {cap['sha256']}")
        return 0
    except Exception as exc:
        print(f"Execution failed: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
