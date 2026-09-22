#!/usr/bin/env python3
"""Device acceptance test runner for B20-7 DMA terminal-PAUSE resurrection.

Executes Sonic controls (no-input and sustained-fire) and regression set
(Copter 271, Burnin' Rubber, Pang, Plotting, Navy Seals, CRTC3 demo)
against Build A (master ef8da61) and Build B (plus/resurrect-dma-pause).

Follows procedure in docs/backlog.md and docs/investigations/sonic/rearm-boundary-2026-09-22.md.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shlex
import struct
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional

DEVICE_TARGET = "root@mister"
EXPECTED_ORIGINAL_CFG_SHA = "2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4"
EXPECTED_PLUS_CFG_SHA = "13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747"

MEDIA_PATHS = {
    "sonic": {
        "path": "/media/fat/games/Amstrad/cpr/Sonic the Hedgehog (UK) (64K) (2025) [Original].cpr",
        "sha": "4cb31c7f1769989a029fe1bd7ffb04670d5d24570dc56b0417fac662fed887ae",
    },
    "copter271": {
        "path": "/media/fat/games/Amstrad/cpr/01_PlusGames/Copter 271.cpr",
        "sha": "4b75c62cbd660206ef30ff8cbf9c4b1281282a423b5eccc1ff8444589f2a9c1d",
    },
    "burnin_rubber": {
        "path": "/media/fat/games/Amstrad/cpr/01_PlusGames/Burnin Rubber.cpr",
        "sha": "08c81e4aeca95abaecc3c196519e2aeb63d2f805b6e18c055a7a3cf49e51a1df",
    },
    "pang": {
        "path": "/media/fat/games/Amstrad/cpr/01_PlusGames/Pang.cpr",
        "sha": "1bd132aa5871581db4fbc379d3ad9bfe162f0e098fb6f0e48f71bccfabc0912b",
    },
    "plotting": {
        "path": "/media/fat/games/Amstrad/cpr/01_PlusGames/Plotting.cpr",
        "sha": "d9e24dcdf199bdba270723cb835b006eba014576e010e8621d65a756a850d85e",
    },
    "navy_seals": {
        "path": "/media/fat/games/Amstrad/cpr/01_PlusGames/Navy Seals.cpr",
        "sha": "a933276f9f580ad81cb3d04af7de1a0ebdbb3944e4fa980137010bb4cd0a0695",
    },
    "crtc3": {
        "path": "/media/fat/games/Amstrad/cpr/crtc3_v2fix.cpr",
        "sha": "3d3f5e01c291e97c9284d10e27f12c0fda3e9530933f0919c607f354e38a7ad6",
    },
}

SSH_BASE = [
    "ssh",
    "-o", "ControlMaster=no",
    "-o", "ControlPath=none",
    "-o", "BatchMode=yes",
    "-o", "ConnectTimeout=10",
    DEVICE_TARGET,
]
SCP_BASE = [
    "scp",
    "-O",
    "-o", "ControlMaster=no",
    "-o", "ControlPath=none",
    "-o", "BatchMode=yes",
    "-o", "ConnectTimeout=10",
]


def run_remote(cmd: str, timeout: float = 30.0) -> str:
    res = subprocess.run(
        SSH_BASE + [cmd],
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    if res.returncode != 0:
        raise RuntimeError(f"Remote command failed ({res.returncode}): {cmd}\nstderr: {res.stderr}")
    return res.stdout.strip()


def upload_file(local_path: Path, remote_path: str, timeout: float = 30.0) -> None:
    subprocess.run(
        SCP_BASE + [str(local_path), f"{DEVICE_TARGET}:{remote_path}"],
        check=True,
        timeout=timeout,
    )


def download_file(remote_path: str, local_path: Path, timeout: float = 30.0) -> None:
    subprocess.run(
        SCP_BASE + [f"{DEVICE_TARGET}:{remote_path}", str(local_path)],
        check=True,
        timeout=timeout,
    )


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class DeviceSession:
    """Manages CFG backup/restore and MiSTer state cleanly."""

    def __init__(self, work_dir: Path):
        self.work_dir = work_dir
        self.work_dir.mkdir(parents=True, exist_ok=True)
        self.orig_cfg = self.work_dir / "Amstrad.CFG.orig"
        self.test_cfg = self.work_dir / "Amstrad.CFG.test"
        self.cfg_modified = False

    def enter(self) -> None:
        core = run_remote("cat /tmp/CORENAME")
        if core != "MENU":
            raise RuntimeError(f"MiSTer device not idle at MENU (running {core!r})")

        # Download original CFG
        download_file("/media/fat/config/Amstrad.CFG", self.orig_cfg)
        orig_sha = sha256_file(self.orig_cfg)
        if orig_sha != EXPECTED_ORIGINAL_CFG_SHA:
            raise RuntimeError(f"Original CFG SHA mismatch: expected {EXPECTED_ORIGINAL_CFG_SHA}, got {orig_sha}")
        print(f"[DeviceSession] Original CFG verified: {orig_sha}")

        # Prepare test CFG: bits [34:33]=2 (6128 Plus), [36:35]=0 (Sync Full)
        d = bytearray(self.orig_cfg.read_bytes())
        d[4] = (d[4] & ~0x1E) | 0x04
        self.test_cfg.write_bytes(d)
        test_sha = sha256_file(self.test_cfg)
        if test_sha != EXPECTED_PLUS_CFG_SHA:
            raise RuntimeError(f"Test CFG SHA mismatch: expected {EXPECTED_PLUS_CFG_SHA}, got {test_sha}")

        # Upload test CFG
        upload_file(self.test_cfg, "/media/fat/config/Amstrad.CFG")
        remote_sha = run_remote("sha256sum /media/fat/config/Amstrad.CFG").split()[0]
        if remote_sha != test_sha:
            raise RuntimeError(f"Remote test CFG SHA mismatch: {remote_sha} != {test_sha}")
        self.cfg_modified = True
        print(f"[DeviceSession] Test CFG applied: {test_sha}")

    def exit(self) -> None:
        print("[DeviceSession] Restoring device state...")
        errors = []
        if self.cfg_modified and self.orig_cfg.exists():
            try:
                upload_file(self.orig_cfg, "/media/fat/config/Amstrad.CFG")
                remote_sha = run_remote("sha256sum /media/fat/config/Amstrad.CFG").split()[0]
                if remote_sha != EXPECTED_ORIGINAL_CFG_SHA:
                    errors.append(f"Restored CFG SHA mismatch: {remote_sha}")
                else:
                    print(f"[DeviceSession] Original CFG restored: {remote_sha}")
            except Exception as e:
                errors.append(f"Failed to restore CFG: {e}")

        # Clean up any leftover temporary files on device
        try:
            run_remote(
                "rm -f /media/fat/config/inputs/Amstrad_input_0000_b017_v3.map "
                "/tmp/b20_7_input.py /tmp/b20_7_schedule.json /media/fat/autoloop_*.mgl"
            )
        except Exception as e:
            errors.append(f"Failed to remove temp files: {e}")

        # Return to MENU
        try:
            run_remote("printf 'load_core /media/fat/menu.rbf\\n' > /dev/MiSTer_cmd")
            time.sleep(2.0)
            core = run_remote("cat /tmp/CORENAME")
            if core != "MENU":
                errors.append(f"Core did not return to MENU: {core}")
            else:
                print("[DeviceSession] Returned to MENU.")
        except Exception as e:
            errors.append(f"Failed to reload MENU: {e}")

        if errors:
            raise RuntimeError("Cleanup errors:\n" + "\n".join(errors))


def run_case_via_driver(
    case_id: str,
    description: str,
    rbf_path: str,
    rbf_sha: str,
    media_name: str,
    boot_delay: float,
    settle_delay: float,
    capture_delay: float,
    captures_count: int,
    out_dir: Path,
) -> Dict[str, Any]:
    """Runs a standard test case using driver.py."""
    media_info = MEDIA_PATHS[media_name]
    case_def = {
        "case_id": case_id,
        "description": description,
        "rbf_path": rbf_path,
        "media": {
            "type": "cpr",
            "slot": "F8",
            "path": media_info["path"],
        },
        "declared_settings": {
            "model": "6128 Plus (status[34:33]=2)",
            "sync_filter": "Full (status[36:35]=0)",
        },
        "boot_delay": boot_delay,
        "settle_delay": settle_delay,
        "capture_delay": capture_delay,
        "captures_count": captures_count,
        "timeouts": {
            "cmd_timeout": 60.0,
            "capture_timeout": 20.0,
            "poll_interval": 0.5,
            "max_retries": 3,
        },
        "ack_main_cmd": True,
        "expected_sha256": {
            "rbf": rbf_sha,
            "media": media_info["sha"],
        },
    }

    out_dir.mkdir(parents=True, exist_ok=True)
    case_path = out_dir / f"{case_id}.json"
    case_path.write_text(json.dumps(case_def, indent=2))

    driver_path = Path("scripts/hardware-loop/driver.py").resolve()
    cmd = [
        sys.executable,
        str(driver_path),
        str(case_path),
        "--target", DEVICE_TARGET,
        "--ack-main-cmd",
        "--out-dir", str(out_dir),
    ]

    print(f"\n--- Running case: {case_id} ---")
    start = time.monotonic()
    res = subprocess.run(cmd, capture_output=True, text=True)
    dur = time.monotonic() - start
    print(f"[{case_id}] Completed in {dur:.1f}s, exit code {res.returncode}")
    if res.returncode != 0:
        print(f"[{case_id}] STDOUT:\n{res.stdout}")
        print(f"[{case_id}] STDERR:\n{res.stderr}")
        raise RuntimeError(f"Case {case_id} failed with exit code {res.returncode}")

    manifest_path = out_dir / "manifest.json"
    if not manifest_path.exists():
        raise RuntimeError(f"Manifest missing at {manifest_path}")

    manifest = json.loads(manifest_path.read_text())
    print(f"[{case_id}] Captured {len(manifest.get('captures', []))} screenshots.")
    for cap in manifest.get("captures", []):
        print(f"  Capture {cap['index']}: {cap['name']} ({cap['sha256'][:16]}...) {cap['dimensions']}")
    return manifest


def run_sonic_sustained_fire(
    case_id: str,
    rbf_path: str,
    rbf_sha: str,
    out_dir: Path,
) -> Dict[str, Any]:
    """Runs Sonic sustained-fire control with title checkpoint and timed captures."""
    out_dir.mkdir(parents=True, exist_ok=True)
    media_info = MEDIA_PATHS["sonic"]

    # 1. Boot cartridge and capture title checkpoint (18s) using driver.py
    title_dir = out_dir / "title_checkpoint"
    case_def = {
        "case_id": f"{case_id}_title",
        "description": "Sonic 18s boot title checkpoint for sustained fire",
        "rbf_path": rbf_path,
        "media": {"type": "cpr", "slot": "F8", "path": media_info["path"]},
        "declared_settings": {"model": "6128 Plus", "sync_filter": "Full"},
        "boot_delay": 18.0,
        "settle_delay": 0.0,
        "capture_delay": 1.0,
        "captures_count": 1,
        "timeouts": {"cmd_timeout": 60.0, "capture_timeout": 20.0, "poll_interval": 0.5, "max_retries": 3},
        "ack_main_cmd": True,
        "expected_sha256": {"rbf": rbf_sha, "media": media_info["sha"]},
    }
    case_path = out_dir / "title_case.json"
    case_path.write_text(json.dumps(case_def, indent=2))

    driver_path = Path("scripts/hardware-loop/driver.py").resolve()
    print(f"\n--- Running {case_id}: Title Checkpoint (18s) ---")
    res = subprocess.run([
        sys.executable, str(driver_path), str(case_path),
        "--target", DEVICE_TARGET, "--ack-main-cmd", "--out-dir", str(title_dir)
    ], capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Title checkpoint failed:\nSTDOUT: {res.stdout}\nSTDERR: {res.stderr}")

    title_manifest = json.loads((title_dir / "manifest.json").read_text())
    title_cap = title_manifest["captures"][0]
    print(f"Title captured: {title_cap['name']} ({title_cap['sha256'][:16]}...)")

    # 2. Upload temporary joystick map and input replay helper
    print("Installing temporary joystick map and input replay schedule...")
    map_bytes = struct.pack("<32I", 106, 105, 108, 103, 29, 56, 57, *([0] * 25))
    local_map = out_dir / "Amstrad_input_0000_b017_v3.map"
    local_map.write_bytes(map_bytes)
    remote_map = "/media/fat/config/inputs/Amstrad_input_0000_b017_v3.map"
    upload_file(local_map, remote_map)

    helper_path = "/tmp/b20_7_input.py"
    upload_file(Path("scripts/hardware-loop/input_replay.py"), helper_path)

    # Schedule: F18 (joystick 1 mode), hold left Ctrl (fire 1) for 2.0s, release, F20 (normal mode)
    # Total duration 3.2s
    schedule = {
        "version": 1,
        "duration": 3.2,
        "events": [
            [0.0, 188, 1],
            [0.1, 188, 0],
            [0.5, 29, 1],
            [2.5, 29, 0],
            [2.8, 190, 1],
            [2.9, 190, 0],
        ],
    }
    local_sched = out_dir / "fire_schedule.json"
    local_sched.write_text(json.dumps(schedule, indent=2))
    remote_sched = "/tmp/b20_7_schedule.json"
    upload_file(local_sched, remote_sched)

    # 3. Execute replay
    print("Executing sustained fire replay (hold left Ctrl for 2.0s)...")
    replay_out = run_remote(f"python3 {helper_path} {remote_sched}")
    (out_dir / "replay.log").write_text(replay_out)
    print(f"Replay output:\n{replay_out}")

    # Remove temporary map immediately after replay
    run_remote(f"rm -f {remote_map} {helper_path} {remote_sched}")

    # 4. Take captures at 2s, 8s, 16s after release
    # Release occurred at monotonic ~ (start + 2.5s)
    # We take captures relative to replay completion:
    # Captures: +2s, +8s, +16s after release
    print("Taking post-release captures at +2s, +8s, +16s...")
    post_captures = []
    # Origin is release point (replay duration - 0.7s)
    origin = time.monotonic()
    delays = [2.0, 8.0, 16.0]
    for delay in delays:
        target_time = origin + delay
        sleep_dur = target_time - time.monotonic()
        if sleep_dur > 0:
            time.sleep(sleep_dur)

        cap_name = f"{case_id}_post_{int(delay)}s.png"
        remote_cap = f"/media/fat/screenshots/{cap_name}"
        run_remote(f"printf 'screenshot {cap_name}\\n' > /dev/MiSTer_cmd")
        time.sleep(1.0)
        local_cap = out_dir / cap_name
        download_file(remote_cap, local_cap)
        run_remote(f"rm -f {remote_cap}")

        sha = sha256_file(local_cap)
        print(f"  Post-release {delay}s capture: {cap_name} ({sha[:16]}...)")
        post_captures.append({
            "delay_s": delay,
            "name": cap_name,
            "sha256": sha,
            "path": str(local_cap),
        })

    result = {
        "case_id": case_id,
        "title_capture": title_cap,
        "post_captures": post_captures,
    }
    (out_dir / "summary.json").write_text(json.dumps(result, indent=2))
    return result


def run_full_suite(rbf_name: str, rbf_sha: str, build_label: str, base_out_dir: Path) -> None:
    rbf_path = f"/media/fat/_Computer/{rbf_name}"
    print(f"\n========================================================")
    print(f"STARTING SUITE FOR BUILD {build_label}: {rbf_name}")
    print(f"RBF SHA256: {rbf_sha}")
    print(f"========================================================")

    session = DeviceSession(base_out_dir / f"session_{build_label}")
    try:
        session.enter()

        # 1. Sonic No-Input Control (18s boot, +8s, +16s)
        run_case_via_driver(
            case_id=f"sonic_noinput_{build_label}",
            description=f"Sonic no-input control on {build_label}",
            rbf_path=rbf_path,
            rbf_sha=rbf_sha,
            media_name="sonic",
            boot_delay=18.0,
            settle_delay=0.0,
            capture_delay=8.0,
            captures_count=3,
            out_dir=base_out_dir / build_label / "sonic_noinput",
        )

        # 2. Sonic Sustained-Fire Control (18s title, 2s hold, +2s, +8s, +16s)
        run_sonic_sustained_fire(
            case_id=f"sonic_fire_{build_label}",
            rbf_path=rbf_path,
            rbf_sha=rbf_sha,
            out_dir=base_out_dir / build_label / "sonic_sustained_fire",
        )

        # 3. Regression: Copter 271 (Title flash watch: 8s boot, 45s settle, 5 captures @ 2s)
        run_case_via_driver(
            case_id=f"copter271_{build_label}",
            description=f"Copter 271 title flash check on {build_label}",
            rbf_path=rbf_path,
            rbf_sha=rbf_sha,
            media_name="copter271",
            boot_delay=8.0,
            settle_delay=45.0,
            capture_delay=2.0,
            captures_count=5,
            out_dir=base_out_dir / build_label / "copter271",
        )

        # 4. Regression: Burnin' Rubber
        run_case_via_driver(
            case_id=f"burnin_rubber_{build_label}",
            description=f"Burnin Rubber on {build_label}",
            rbf_path=rbf_path,
            rbf_sha=rbf_sha,
            media_name="burnin_rubber",
            boot_delay=10.0,
            settle_delay=0.0,
            capture_delay=3.0,
            captures_count=3,
            out_dir=base_out_dir / build_label / "burnin_rubber",
        )

        # 5. Regression: Pang
        run_case_via_driver(
            case_id=f"pang_{build_label}",
            description=f"Pang on {build_label}",
            rbf_path=rbf_path,
            rbf_sha=rbf_sha,
            media_name="pang",
            boot_delay=12.0,
            settle_delay=0.0,
            capture_delay=3.0,
            captures_count=3,
            out_dir=base_out_dir / build_label / "pang",
        )

        # 6. Regression: Plotting
        run_case_via_driver(
            case_id=f"plotting_{build_label}",
            description=f"Plotting on {build_label}",
            rbf_path=rbf_path,
            rbf_sha=rbf_sha,
            media_name="plotting",
            boot_delay=10.0,
            settle_delay=0.0,
            capture_delay=3.0,
            captures_count=3,
            out_dir=base_out_dir / build_label / "plotting",
        )

        # 7. Regression: Navy Seals
        run_case_via_driver(
            case_id=f"navy_seals_{build_label}",
            description=f"Navy Seals on {build_label}",
            rbf_path=rbf_path,
            rbf_sha=rbf_sha,
            media_name="navy_seals",
            boot_delay=12.0,
            settle_delay=0.0,
            capture_delay=3.0,
            captures_count=3,
            out_dir=base_out_dir / build_label / "navy_seals",
        )

        # 8. Regression: CRTC3 demo
        run_case_via_driver(
            case_id=f"crtc3_{build_label}",
            description=f"CRTC3 demo on {build_label}",
            rbf_path=rbf_path,
            rbf_sha=rbf_sha,
            media_name="crtc3",
            boot_delay=10.0,
            settle_delay=0.0,
            capture_delay=3.0,
            captures_count=3,
            out_dir=base_out_dir / build_label / "crtc3",
        )

    finally:
        session.exit()


def main():
    parser = argparse.ArgumentParser(description="Run device acceptance for B20-7.")
    parser.add_argument("--build", choices=["a", "b", "both"], default="both", help="Which build to run")
    parser.add_argument("--rbf-b-name", type=str, default="", help="RBF filename for Build B")
    parser.add_argument("--rbf-b-sha", type=str, default="", help="SHA256 for Build B")
    parser.add_argument("--out-dir", type=Path, default=Path("docs/screenshots/device-acceptance-b20-7-2026-09-22"))
    args = parser.parse_args()

    rbf_a_name = "Amstrad_20260922_ef8da61.rbf"
    rbf_a_sha = "a982f5bd46911c76907cc22b44e133fd42bd5d325c28d57f71ef008a54288a2b"

    if args.build in ("a", "both"):
        run_full_suite(rbf_a_name, rbf_a_sha, "build_a", args.out_dir)

    if args.build in ("b", "both"):
        if not args.rbf_b_name or not args.rbf_b_sha:
            print("Error: Build B requires --rbf-b-name and --rbf-b-sha", file=sys.stderr)
            sys.exit(1)
        run_full_suite(args.rbf_b_name, args.rbf_b_sha, "build_b", args.out_dir)


if __name__ == "__main__":
    main()
