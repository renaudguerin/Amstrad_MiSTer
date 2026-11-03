#!/usr/bin/env python3
"""AmSpirit troubleshooting-oracle helper (Track G).

Thin client for a running AmSpirit lite instance's HTTP API: load media, pace
input on emu.frames, capture a settled screenshot, dump machine state and save
an SNA. Standalone by design: it never talks to the MiSTer. See
docs/investigations/hardware-runs/amspirit-oracle-design-2026-09-13.md for
what AmSpirit evidence can and cannot establish, and
scripts/amspirit/README.md for usage and API gotchas.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional

DEFAULT_URL = os.environ.get("AMSPIRIT_URL", "http://127.0.0.1:6128")

# Joystick button mapping for POST /api/joystick (port 0)
JOY_MAP = {
    "up": "up",
    "down": "down",
    "left": "left",
    "right": "right",
    "fire": "fire1",
    "fire1": "fire1",
    "fire2": "fire2",
    "fire3": "fire3",
}

MODEL_INT_TO_NAME = {0: "464", 1: "664", 2: "6128", 3: "464+", 4: "6128+", 5: "GX4000"}

STATE_ENDPOINTS = ["ping", "config", "render", "state", "memmap", "beam", "history", "keymatrix"]


class OracleError(Exception):
    pass


class AmSpirit:
    def __init__(self, url: str, deadline: float):
        self.url = url.rstrip("/")
        self.deadline = deadline
        self._api_ping: Optional[Dict[str, Any]] = None
        self.last_media_result: Dict[str, Any] = {}

    # --- transport ---

    def _req(self, method: str, path: str, body: Optional[bytes] = None,
             ctype: Optional[str] = None, timeout: float = 30.0):
        req = urllib.request.Request(self.url + path, data=body, method=method)
        if ctype:
            req.add_header("Content-Type", ctype)
        try:
            return urllib.request.urlopen(req, timeout=timeout)
        except urllib.error.HTTPError as exc:
            err_body = ""
            try:
                err_body = exc.read().decode("utf-8", errors="replace")
            except Exception:
                pass
            msg = f"{method} {path}: HTTP {exc.code} {exc.reason}"
            if err_body:
                msg += f" - {err_body}"
            raise OracleError(msg) from exc
        except OSError as exc:
            raise OracleError(f"{method} {path}: {exc}") from exc

    def get_json(self, path: str, timeout: Optional[float] = None) -> Any:
        with self._req("GET", path, timeout=30.0 if timeout is None else timeout) as r:
            return json.load(r)

    def post_json(self, path: str, payload: Dict[str, Any]) -> Any:
        self.require_command_sequences()
        with self._req("POST", path, json.dumps(payload).encode(), "application/json",
                       timeout=self.deadline) as r:
            out = json.load(r)
        if "ok" in out and not out["ok"]:
            raise OracleError(f"POST {path} {payload}: {out}")
        self.wait_applied_cmd(out.get("cmd_seq"), path)
        return out

    # --- primitives ---

    def emu(self, timeout: Optional[float] = None) -> Dict[str, Any]:
        return self.get_json("/api/ping", timeout=timeout)["emu"]

    def require_command_sequences(self) -> Dict[str, Any]:
        """Require the 1.16/1.17+ applied-command contract before sending mutations."""
        if self._api_ping is None:
            ping = self.get_json("/api/ping", timeout=self.deadline)
            applied = ping.get("emu", {}).get("applied_cmd_seq")
            if isinstance(applied, bool) or not isinstance(applied, int):
                raise OracleError(
                    "AmSpirit does not publish emu.applied_cmd_seq; command ordering is "
                    "unverifiable, so this helper requires Lite 1.16+ and will not assume "
                    "legacy commands have applied")
            self._api_ping = ping
        return self._api_ping

    def require_crtc_keep(self) -> Dict[str, Any]:
        """Fail before mutation unless the server advertises Lite 1.16 or newer."""
        ping = self.require_command_sequences()
        version = str(ping.get("version", ""))
        match = re.match(r"^(\d+)\.(\d+)\.(\d+)", version)
        version_tuple = tuple(map(int, match.groups())) if match else ()
        if ping.get("frontend") != "lite" or version_tuple < (1, 16, 0):
            raise OracleError(
                f"CRTC keep requires a Lite 1.16+ API capability; server reports "
                f"frontend={ping.get('frontend')!r}, version={version!r}")
        return ping

    def wait_applied_cmd(self, seq: Any, path: str = "command") -> None:
        """Wait for a command's applied sequence, bounding each HTTP poll too,
        and checking emu.failed_cmds for apply-time failure."""
        if isinstance(seq, bool) or not isinstance(seq, int):
            raise OracleError(
                f"{path} response has no integer cmd_seq; refusing to assume the "
                "accepted command has been applied")
        end = time.monotonic() + self.deadline
        while True:
            remaining = end - time.monotonic()
            if remaining <= 0:
                raise OracleError(f"cmd_seq {seq} was not applied before the host deadline")
            try:
                emu = self.emu(timeout=remaining)
            except OracleError as exc:
                if time.monotonic() >= end:
                    raise OracleError(f"cmd_seq {seq} poll hit the host deadline") from exc
                raise
            applied = emu.get("applied_cmd_seq")
            if isinstance(applied, int) and not isinstance(applied, bool) and applied >= seq:
                failed_cmds = emu.get("failed_cmds", [])
                if isinstance(failed_cmds, list):
                    for fail in failed_cmds:
                        if isinstance(fail, dict) and fail.get("cmd_seq") == seq:
                            err = fail.get("error", "command failed at apply time")
                            raise OracleError(f"{path} (cmd_seq {seq}) failed at apply time: {err}")
                return
            time.sleep(min(0.05, max(0.0, end - time.monotonic())))

    def frames(self, timeout: Optional[float] = None) -> int:
        return int(self.emu(timeout=timeout)["frames"])

    def wait_frames(self, n: int) -> int:
        """Wait until emu.frames advances by n. Fails on pause or host deadline,
        because the counter stops while paused, at a breakpoint or after a crash."""
        end = time.monotonic() + max(self.deadline, n / 25.0)
        remaining = end - time.monotonic()
        if remaining <= 0:
            raise OracleError("host deadline before frame wait started")
        start = self.frames(timeout=remaining)
        target = start + n
        last_frame = start
        while True:
            remaining = end - time.monotonic()
            if remaining <= 0:
                raise OracleError(f"host deadline: frame {last_frame} of {target} (started {start})")
            e = self.emu(timeout=remaining)
            last_frame = int(e["frames"])
            if e["frames"] >= target:
                return int(e["frames"])
            if e["paused"]:
                raise OracleError(f"emulator paused at frame {e['frames']} while waiting for {target}")
            time.sleep(min(0.05, max(0.0, end - time.monotonic())))

    def eval_lua(self, chunk: str, timeout: Optional[float] = None) -> str:
        """Run one Lua chunk. A chunk that waits on frames cannot finish while
        paused; paused evals without waits (snapshot saves) finish at once, so the
        pause check only starts after a grace second. A timed-out chunk keeps
        running in AmSpirit and the engine refuses new evals until it ends."""
        start = time.monotonic()
        limit = timeout or self.deadline
        end = start + limit
        with self._req("POST", "/api/eval", chunk.encode(), "text/plain", timeout=limit) as r:
            seq = json.load(r)["seq"]
        while True:
            remaining = end - time.monotonic()
            if remaining <= 0:
                raise OracleError(f"eval seq {seq} not done within {limit}s")
            res = self.get_json(f"/api/eval?seq={seq}", timeout=remaining)
            if res.get("refused"):
                raise OracleError("eval refused: script engine busy")
            if res.get("done"):
                if res.get("error"):
                    raise OracleError(f"lua error: {res['error']}")
                return res.get("value", "")
            if time.monotonic() - start > 1.0:
                remaining = end - time.monotonic()
                if remaining <= 0:
                    raise OracleError(f"eval seq {seq} not done within {limit}s")
                if self.emu(timeout=remaining)["paused"]:
                    raise OracleError(f"eval seq {seq} blocked: emulator paused")
            time.sleep(min(0.05, max(0.0, end - time.monotonic())))

    def set_paused(self, paused: bool) -> None:
        self.post_json("/api/run", {"action": "pause" if paused else "resume"})
        end = time.monotonic() + self.deadline
        observed = None
        while True:
            remaining = end - time.monotonic()
            if remaining <= 0:
                raise OracleError(f"pause state did not become {paused}; last state was {observed}")
            observed = self.emu(timeout=remaining)["paused"]
            if observed == paused:
                return
            time.sleep(min(0.05, max(0.0, end - time.monotonic())))

    def load_media(self, path: Path, hard_reset: Optional[bool] = None, crtc: str = "sna") -> int:
        """Load media; return the frame counter read right after the (optional)
        hard reset. That read is the run's frame origin, accurate to a few frames."""
        is_sna = media_is_sna(path)
        if hard_reset is None:
            hard_reset = not is_sna
        validate_media_policy(is_sna, hard_reset, crtc)
        self.require_command_sequences()
        crtc_before = None
        if crtc == "keep":
            self.require_crtc_keep()
            crtc_before = self.emu(timeout=self.deadline).get("crtc_type")
            if isinstance(crtc_before, bool) or not isinstance(crtc_before, int):
                raise OracleError("CRTC keep requires a live emu.crtc_type before loading")
        query = {"name": path.name}
        if is_sna:
            query["crtc"] = crtc
        url = "/api/media?" + urllib.parse.urlencode(query)
        with self._req("POST", url, path.read_bytes(), "application/octet-stream",
                       timeout=self.deadline) as r:
            out = json.load(r)
        if "ok" in out and not out["ok"]:
            raise OracleError(f"media load refused: {out}")
        self.wait_applied_cmd(out.get("cmd_seq"), "POST /api/media")
        if hard_reset:
            self.post_json("/api/run", {"action": "reset", "kind": "hard"})
        emu = self.emu(timeout=self.deadline)
        self.last_media_result = {
            "crtc_policy": crtc,
            "hard_reset": hard_reset,
            "crtc_before": crtc_before,
            "effective_crtc_type": emu.get("crtc_type"),
            "crtc_mismatch": crtc == "keep" and emu.get("crtc_type") != crtc_before,
        }
        return int(emu["frames"])

    def screenshot(self, out: Path) -> Dict[str, Any]:
        # Settled plain frame: live=false full=true. When the machine produces no VSYNC
        # (a crash) AmSpirit keeps returning the last settled frame, so compare
        # hashes across captures before trusting a "stable" screen.
        with self._req("GET", "/api/screenshot?crop=true&full=true&live=false") as r:
            data = r.read()
            headers = {k: v for k, v in r.headers.items() if k.lower().startswith("x-")}
        out.write_bytes(data)
        return {"file": out.name, "sha256": sha256_bytes(data), "headers": headers,
                "frames": self.frames()}

    def dump_state(self, out_dir: Path) -> List[str]:
        names = []
        for ep in STATE_ENDPOINTS:
            p = out_dir / f"{ep}.json"
            p.write_text(json.dumps(self.get_json(f"/api/{ep}"), indent=1) + "\n")
            names.append(p.name)
        return names

    def save_snapshot(self, name: str, out_dir: Path) -> Dict[str, Any]:
        """Save an SNA snapshot via GET /api/snapshot.
        In Lite 1.17+, the snapshot is streamed over HTTP directly as octet-stream,
        removing the requirement for filesystem co-location or Lua fs manipulation."""
        dst = out_dir / f"{name}.sna"
        with self._req("GET", "/api/snapshot", timeout=self.deadline) as r:
            data = r.read()
        dst.write_bytes(data)
        return {"file": dst.name, "sha256": sha256_file(dst), "sna": parse_sna(dst)}

    def hold_joystick(self, buttons: List[str], frames: int) -> None:
        payload: Dict[str, Any] = {"frames": frames}
        for b in buttons:
            if b not in JOY_MAP:
                raise OracleError(f"unknown joystick button {b!r}; known: {sorted(JOY_MAP)}")
            payload[JOY_MAP[b]] = True
        self.post_json("/api/joystick", payload)
        self.wait_frames(frames)

    def type_keys(self, text: str) -> None:
        self.post_json("/api/keytype", {"text": text})
        end = time.monotonic() + self.deadline
        e: Dict[str, Any] = {}
        while True:
            remaining = end - time.monotonic()
            if remaining <= 0:
                raise OracleError(f"autotype unfinished, {e.get('autotype_remaining', '?')} chars left")
            e = self.emu(timeout=remaining)
            if not e["autotyping"]:
                return
            if e["paused"]:
                raise OracleError("emulator paused while autotyping")
            time.sleep(min(0.1, max(0.0, end - time.monotonic())))


# --- helpers ---

def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def parse_sna(path: Path) -> Dict[str, Any]:
    """Header fields and chunk list of an SNA v3. Our core applies MEM0/MEM1 and
    CPC+ but not SPRT, so the chunk list is what a MiSTer handoff must record."""
    d = path.read_bytes()
    if len(d) < 0x100 or d[:8] != b"MV - SNA":
        raise OracleError(f"{path.name}: not an SNA")
    dump_kb = int.from_bytes(d[0x6B:0x6D], "little")
    info: Dict[str, Any] = {"version": d[0x10], "model": d[0x6D], "crtc_type": d[0xA4],
                            "dump_kb": dump_kb, "chunks": []}
    i = 0x100 + dump_kb * 1024
    while i + 8 <= len(d):
        length = int.from_bytes(d[i + 4:i + 8], "little")
        info["chunks"].append({"name": d[i:i + 4].decode("latin-1"), "len": length})
        i += 8 + length
    return info


def media_is_sna(path: Path) -> bool:
    with path.open("rb") as media:
        has_sna_magic = media.read(8) == b"MV - SNA"
    if path.suffix.lower() == ".sna" and not has_sna_magic:
        raise OracleError(f"{path.name}: .sna file has invalid snapshot magic")
    return has_sna_magic


def validate_media_policy(is_sna: bool, hard_reset: bool, crtc: str) -> None:
    if crtc not in ("sna", "keep"):
        raise OracleError(f"invalid SNA CRTC policy {crtc!r}; expected 'sna' or 'keep'")
    if crtc == "keep" and not is_sna:
        raise OracleError("crtc=keep is only supported when loading an SNA")
    if is_sna and hard_reset:
        raise OracleError("SNA media must not be hard reset; its loaded state is the reset")


def main_checkout() -> Path:
    """Ignored media (cartridges, disks) lives only in the main checkout."""
    out = subprocess.run(["git", "rev-parse", "--path-format=absolute", "--git-common-dir"],
                         capture_output=True, text=True, check=True).stdout.strip()
    return Path(out).parent


def identity(ams: AmSpirit) -> Dict[str, Any]:
    ping = ams.get_json("/api/ping")
    return {"lite": ping.get("lite"), "frontend": ping.get("frontend"),
            "version": ping.get("version"), "core": ping.get("core"), "url": ams.url,
            "config": ams.get_json("/api/config"), "render": ams.get_json("/api/render")}


def normalize_model(val: Any) -> Any:
    if isinstance(val, int) and val in MODEL_INT_TO_NAME:
        return MODEL_INT_TO_NAME[val]
    return str(val) if val is not None else None


def read_effective_settings(ams: AmSpirit, requested: Dict[str, Any]) -> Dict[str, Any]:
    cfg, render = ams.get_json("/api/config"), ams.get_json("/api/render")
    emu = ams.emu(timeout=ams.deadline)
    req_cfg = requested.get("config", {})
    req_render = requested.get("render", {})
    crt = render.get("crt", {})
    applied = {**cfg, **{k: render.get(k, crt.get(k)) for k in req_render}}
    if "crtc_type" in req_cfg and "crtc_type" in emu:
        # An SNA can switch the live CRTC without changing the saved preference.
        applied["crtc_type"] = emu["crtc_type"]

    def setting_mismatch(k: str, req_v: Any, app_v: Any) -> bool:
        if k == "cpc_model":
            return normalize_model(req_v) != normalize_model(app_v)
        return app_v != req_v

    mismatches = {k: {"requested": v, "applied": applied.get(k)}
                  for k, v in {**req_cfg, **req_render}.items() if setting_mismatch(k, v, applied.get(k))}
    return {"config": cfg, "render": render,
            "emu": {k: emu.get(k) for k in ("cpc_model", "crtc_type", "frames", "ticks")},
            "mismatches": mismatches}


def apply_settings(ams: AmSpirit, case: Dict[str, Any]) -> Dict[str, Any]:
    """Request config/render settings and record requested versus applied."""
    req_cfg = case.get("config", {})
    req_render = case.get("render", {})
    if req_cfg:
        ams.post_json("/api/config", req_cfg)
    if req_render:
        ams.post_json("/api/render", req_render)
    requested = {"config": req_cfg, "render": req_render}
    effective = read_effective_settings(ams, requested)
    return {"requested": requested,
            "applied": {"config": effective["config"], "render": effective["render"]},
            "mismatches": effective["mismatches"]}


def run_case(ams: AmSpirit, case_path: Path, out_dir: Path, media_root: Path) -> Dict[str, Any]:
    case = json.loads(case_path.read_text())
    out_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = out_dir / "manifest.json"
    if manifest_path.exists():
        raise OracleError(f"{manifest_path} exists; use a new --out-dir")

    media = media_root / case["media"]["path"]
    is_sna = media_is_sna(media)
    crtc_policy = case["media"].get("crtc", "sna")
    hard_reset_value = case["media"].get("hard_reset")
    if hard_reset_value is not None and not isinstance(hard_reset_value, bool):
        raise OracleError("media.hard_reset must be a boolean")
    hard_reset = (not is_sna) if hard_reset_value is None else hard_reset_value
    validate_media_policy(is_sna, hard_reset, crtc_policy)
    if crtc_policy == "keep":
        # Capability checks are read-only and must happen before settings or pause change.
        ams.require_crtc_keep()
    media_sha = sha256_file(media)
    expected = case["media"].get("sha256")
    if expected and expected != media_sha:
        raise OracleError(f"media hash {media_sha} != expected {expected}")

    manifest: Dict[str, Any] = {
        "case_id": case["case_id"], "description": case.get("description", ""),
        "started_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "side": "amspirit", "identity": identity(ams),
        "media": {"path": case["media"]["path"], "sha256": media_sha,
                  "crtc": crtc_policy, "hard_reset": hard_reset},
        "case_sha256": sha256_file(case_path), "steps": case.get("steps", []),
        "pipeline_position": "AmSpirit pre-shader raw frame (monitor Off), settled, plain",
        "events": [],
    }
    status = "failed"
    was_paused = False
    try:
        was_paused = ams.emu()["paused"]
        if was_paused:
            ams.set_paused(False)
        manifest["settings"] = apply_settings(ams, case)
        origin = ams.load_media(media, hard_reset, crtc_policy)
        action = "media load and hard reset" if hard_reset else "media load (no reset)"
        manifest["frame_origin"] = {"frames": origin, "definition": f"emu.frames read right after {action}"}
        manifest["media_effect"] = ams.last_media_result
        manifest["settings"]["effective_after_media"] = read_effective_settings(
            ams, manifest["settings"]["requested"])
        shot_no = 0
        # "at" is the offset when a step starts; "done_at" when it returned.
        for step in case.get("steps", []):
            at = ams.frames() - origin
            if "wait_frames" in step:
                f = ams.wait_frames(int(step["wait_frames"]))
                manifest["events"].append({"wait_frames": step["wait_frames"], "at": at, "done_at": f - origin})
            elif "joystick" in step:
                frames = int(step.get("frames", 10))
                ams.hold_joystick(step["joystick"], frames)
                manifest["events"].append({"joystick": step["joystick"], "frames": frames, "at": at,
                                           "done_at": ams.frames() - origin})
            elif "keys" in step:
                ams.type_keys(step["keys"])
                manifest["events"].append({"keys": step["keys"], "at": at, "done_at": ams.frames() - origin})
            elif "screenshot" in step:
                shot_no += 1
                s = ams.screenshot(out_dir / f"step{shot_no:02d}_{step['screenshot']}.png")
                s["at"] = s["frames"] - origin
                manifest["events"].append({"screenshot": s})
            else:
                raise OracleError(f"unknown step {step}")

        # Checkpoint: pause so screenshot, state and SNA describe one instant.
        ams.set_paused(True)
        cp: Dict[str, Any] = {"at": ams.frames() - origin}
        cp["screenshot"] = ams.screenshot(out_dir / "checkpoint.png")
        cp["state_files"] = ams.dump_state(out_dir)
        cp["snapshot"] = ams.save_snapshot(case["case_id"], out_dir)
        manifest["checkpoint"] = cp
        status = "ok"
    except Exception as exc:  # the manifest must say why, whatever failed
        manifest["error"] = f"{type(exc).__name__}: {exc}"
    finally:
        manifest["status"] = status
        manifest["finished_utc"] = datetime.now(timezone.utc).isoformat(timespec="seconds")
        # A failure stays paused for post-mortem; success restores the pause state
        # found at start unless the case asks to stay on the checkpoint.
        if status == "ok" and not case.get("leave_paused", False) and not was_paused:
            try:
                ams.set_paused(False)
            except OracleError as exc:
                manifest["resume_error"] = str(exc)
        elif status != "ok":
            try:
                ams.post_json("/api/run", {"action": "pause"})
            except OracleError:
                pass
        manifest_path.write_text(json.dumps(manifest, indent=1) + "\n")
    return manifest


def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--url", default=DEFAULT_URL, help="AmSpirit web server (env AMSPIRIT_URL)")
    p.add_argument("--deadline", type=float, default=30.0, help="host deadline per wait, seconds")
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("identity", help="version, config and render settings")
    s = sub.add_parser("load", help="load media (CPR resets by default; SNA never resets)")
    s.add_argument("media", type=Path)
    reset = s.add_mutually_exclusive_group()
    reset.add_argument("--reset", dest="reset", action="store_true",
                       help="hard reset after loading (SNA files cannot be reset)")
    reset.add_argument("--no-reset", dest="reset", action="store_false",
                       help="skip the hard reset after loading")
    s.set_defaults(reset=None)
    s.add_argument("--crtc", choices=("sna", "keep"), default="sna",
                   help="SNA CRTC policy (default: use the snapshot's CRTC)")
    s = sub.add_parser("wait", help="wait N emulated frames")
    s.add_argument("frames", type=int)
    s = sub.add_parser("shot", help="settled screenshot to a PNG")
    s.add_argument("out", type=Path)
    s = sub.add_parser("state", help="dump state endpoints into a directory")
    s.add_argument("out_dir", type=Path)
    s = sub.add_parser("snapshot", help="save an SNA into a directory")
    s.add_argument("name")
    s.add_argument("out_dir", type=Path)
    s = sub.add_parser("joy", help="hold joystick 0 buttons for N frames")
    s.add_argument("buttons", nargs="+", choices=sorted(JOY_MAP))
    s.add_argument("--frames", type=int, default=10)
    s = sub.add_parser("keys", help="autotype text and wait until it lands")
    s.add_argument("text")
    s = sub.add_parser("pause")
    s = sub.add_parser("resume")
    s = sub.add_parser("eval", help="run one Lua chunk, print its value")
    s.add_argument("chunk")
    s = sub.add_parser("run", help="run a case JSON to a checkpoint with a manifest")
    s.add_argument("case", type=Path)
    s.add_argument("--out-dir", type=Path, required=True)
    s.add_argument("--media-root", type=Path, help="base for media.path (default: main checkout)")
    return p.parse_args(argv)


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)
    ams = AmSpirit(args.url, args.deadline)
    try:
        if args.cmd == "identity":
            out: Any = identity(ams)
        elif args.cmd == "load":
            frame_origin = ams.load_media(args.media, args.reset, args.crtc)
            out = {"frame_origin": frame_origin, "sha256": sha256_file(args.media),
                   "media_effect": ams.last_media_result}
        elif args.cmd == "wait":
            out = {"frames": ams.wait_frames(args.frames)}
        elif args.cmd == "shot":
            out = ams.screenshot(args.out)
        elif args.cmd == "state":
            args.out_dir.mkdir(parents=True, exist_ok=True)
            out = ams.dump_state(args.out_dir)
        elif args.cmd == "snapshot":
            args.out_dir.mkdir(parents=True, exist_ok=True)
            out = ams.save_snapshot(args.name, args.out_dir)
        elif args.cmd == "joy":
            ams.hold_joystick(args.buttons, args.frames)
            out = {"frames": ams.frames()}
        elif args.cmd == "keys":
            ams.type_keys(args.text)
            out = {"frames": ams.frames()}
        elif args.cmd in ("pause", "resume"):
            ams.set_paused(args.cmd == "pause")
            out = ams.emu()
        elif args.cmd == "eval":
            out = ams.eval_lua(args.chunk)
        else:
            out = run_case(ams, args.case, args.out_dir, args.media_root or main_checkout())
            print(json.dumps({"status": out["status"], "error": out.get("error"),
                              "manifest": str(args.out_dir / "manifest.json")}, indent=1))
            return 0 if out["status"] == "ok" else 1
    except (OracleError, OSError, KeyError, json.JSONDecodeError) as exc:
        print(f"error: {exc!r}", file=sys.stderr)
        return 1
    print(out if isinstance(out, str) else json.dumps(out, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
