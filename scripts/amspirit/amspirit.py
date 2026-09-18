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
import shutil
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional

DEFAULT_URL = os.environ.get("AMSPIRIT_URL", "http://127.0.0.1:6128")

# CPC joystick 0 lives on keyboard matrix row 9, active low.
JOY_BITS = {"up": 0x01, "down": 0x02, "left": 0x04, "right": 0x08, "fire": 0x10, "fire2": 0x20}

STATE_ENDPOINTS = ["ping", "config", "render", "state", "memmap", "beam", "history", "keymatrix"]


class OracleError(Exception):
    pass


class AmSpirit:
    def __init__(self, url: str, deadline: float):
        self.url = url.rstrip("/")
        self.deadline = deadline

    # --- transport ---

    def _req(self, method: str, path: str, body: Optional[bytes] = None,
             ctype: Optional[str] = None, timeout: float = 30.0):
        req = urllib.request.Request(self.url + path, data=body, method=method)
        if ctype:
            req.add_header("Content-Type", ctype)
        try:
            return urllib.request.urlopen(req, timeout=timeout)
        except OSError as exc:
            raise OracleError(f"{method} {path}: {exc}") from exc

    def get_json(self, path: str) -> Any:
        with self._req("GET", path) as r:
            return json.load(r)

    def post_json(self, path: str, payload: Dict[str, Any]) -> Any:
        with self._req("POST", path, json.dumps(payload).encode(), "application/json") as r:
            out = json.load(r)
        if not out.get("ok", False):
            raise OracleError(f"POST {path} {payload}: {out}")
        return out

    # --- primitives ---

    def emu(self) -> Dict[str, Any]:
        return self.get_json("/api/ping")["emu"]

    def frames(self) -> int:
        return int(self.emu()["frames"])

    def wait_frames(self, n: int) -> int:
        """Wait until emu.frames advances by n. Fails on pause or host deadline,
        because the counter stops while paused, at a breakpoint or after a crash."""
        start = self.frames()
        target = start + n
        end = time.monotonic() + max(self.deadline, n / 25.0)
        while True:
            e = self.emu()
            if e["frames"] >= target:
                return int(e["frames"])
            if e["paused"]:
                raise OracleError(f"emulator paused at frame {e['frames']} while waiting for {target}")
            if time.monotonic() > end:
                raise OracleError(f"host deadline: frame {e['frames']} of {target} (started {start})")
            time.sleep(0.05)

    def eval_lua(self, chunk: str, timeout: Optional[float] = None) -> str:
        """Run one Lua chunk. A chunk that waits on frames cannot finish while
        paused; paused evals without waits (snapshot saves) finish at once, so the
        pause check only starts after a grace second. A timed-out chunk keeps
        running in AmSpirit and the engine refuses new evals until it ends."""
        with self._req("POST", "/api/eval", chunk.encode(), "text/plain") as r:
            seq = json.load(r)["seq"]
        start = time.monotonic()
        limit = timeout or self.deadline
        while time.monotonic() - start < limit:
            res = self.get_json(f"/api/eval?seq={seq}")
            if res.get("refused"):
                raise OracleError("eval refused: script engine busy")
            if res.get("done"):
                if res.get("error"):
                    raise OracleError(f"lua error: {res['error']}")
                return res.get("value", "")
            if time.monotonic() - start > 1.0 and self.emu()["paused"]:
                raise OracleError(f"eval seq {seq} blocked: emulator paused")
            time.sleep(0.05)
        raise OracleError(f"eval seq {seq} not done within {limit}s")

    def set_paused(self, paused: bool) -> None:
        self.post_json("/api/config", {"paused": paused})
        end = time.monotonic() + self.deadline
        while self.emu()["paused"] != paused:
            if time.monotonic() > end:
                raise OracleError(f"pause state did not become {paused}")
            time.sleep(0.05)

    def load_media(self, path: Path, hard_reset: bool) -> int:
        """Load media; return the frame counter read right after the (optional)
        hard reset. That read is the run's frame origin, accurate to a few frames."""
        name = urllib.parse.quote(path.name)
        with self._req("POST", f"/api/media?name={name}", path.read_bytes(), "application/octet-stream") as r:
            out = json.load(r)
        if not out.get("ok"):
            raise OracleError(f"media load refused: {out}")
        if hard_reset:
            self.post_json("/api/config", {"do_hard_reset": True})
        return self.frames()

    def screenshot(self, out: Path) -> Dict[str, Any]:
        # Settled plain frame: live=0 full=1. When the machine produces no VSYNC
        # (a crash) AmSpirit keeps returning the last settled frame, so compare
        # hashes across captures before trusting a "stable" screen.
        with self._req("GET", "/api/screenshot?crop=1&full=1&live=0") as r:
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
        """Save an SNA through Lua. snapshot() silently writes nothing when the
        target directory is missing, so create fs.root() first (fs.write makes it)."""
        root = self.eval_lua(
            'if not fs.exists(".oracle") then fs.write(".oracle", "") end '
            f'if fs.exists("{name}.sna") then fs.remove("{name}.sna") end '
            f'snapshot_dir(fs.root()) snapshot_name("{name}") snapshot() '
            f'if not fs.exists("{name}.sna") then error("snapshot not written") end '
            'return fs.root()')
        src = Path(root) / f"{name}.sna"
        if not src.exists():
            raise OracleError(f"{src} not visible from this host; AmSpirit must run locally")
        dst = out_dir / src.name
        shutil.copyfile(src, dst)
        self.eval_lua(f'fs.remove("{name}.sna") return "ok"')
        return {"file": dst.name, "sha256": sha256_file(dst), "sna": parse_sna(dst)}

    def hold_joystick(self, buttons: List[str], frames: int) -> None:
        mask = 0
        for b in buttons:
            if b not in JOY_BITS:
                raise OracleError(f"unknown joystick button {b!r}; known: {sorted(JOY_BITS)}")
            mask |= JOY_BITS[b]
        row9 = 0xFF & ~mask
        # pcall so the release runs even if the hold loop errors inside AmSpirit.
        self.eval_lua(
            "local m={} for i=1,16 do m[i]=255 end m[10]=" f"{row9} "
            f"local ok,err=pcall(function() for i=1,{frames} do keyboard_write(table.unpack(m)) wait_frames(1) end end) "
            "for i=1,16 do m[i]=255 end keyboard_write(table.unpack(m)) "
            "if not ok then error(err) end return 'ok'",
            timeout=max(self.deadline, frames / 25.0 + 5.0))

    def type_keys(self, text: str) -> None:
        self.post_json("/api/keytype", {"text": text})
        end = time.monotonic() + self.deadline
        while True:
            e = self.emu()
            if not e["autotyping"]:
                return
            if e["paused"]:
                raise OracleError("emulator paused while autotyping")
            if time.monotonic() > end:
                raise OracleError(f"autotype unfinished, {e['autotype_remaining']} chars left")
            time.sleep(0.1)


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


def main_checkout() -> Path:
    """Ignored media (cartridges, disks) lives only in the main checkout."""
    out = subprocess.run(["git", "rev-parse", "--path-format=absolute", "--git-common-dir"],
                         capture_output=True, text=True, check=True).stdout.strip()
    return Path(out).parent


def identity(ams: AmSpirit) -> Dict[str, Any]:
    ping = ams.get_json("/api/ping")
    return {"lite": ping.get("lite"), "core": ping.get("core"), "url": ams.url,
            "config": ams.get_json("/api/config"), "render": ams.get_json("/api/render")}


def apply_settings(ams: AmSpirit, case: Dict[str, Any]) -> Dict[str, Any]:
    """Request config/render settings and record requested versus applied."""
    req_cfg = case.get("config", {})
    req_render = case.get("render", {})
    if req_cfg:
        ams.post_json("/api/config", req_cfg)
    if req_render:
        ams.post_json("/api/render", req_render)
    time.sleep(0.2)
    cfg, render = ams.get_json("/api/config"), ams.get_json("/api/render")
    crt = render.get("crt", {})  # shader parameters are nested under "crt"
    applied = {**cfg, **{k: render.get(k, crt.get(k)) for k in req_render}}
    mismatches = {k: {"requested": v, "applied": applied.get(k)}
                  for k, v in {**req_cfg, **req_render}.items() if applied.get(k) != v}
    return {"requested": {"config": req_cfg, "render": req_render},
            "applied": {"config": cfg, "render": render}, "mismatches": mismatches}


def run_case(ams: AmSpirit, case_path: Path, out_dir: Path, media_root: Path) -> Dict[str, Any]:
    case = json.loads(case_path.read_text())
    out_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = out_dir / "manifest.json"
    if manifest_path.exists():
        raise OracleError(f"{manifest_path} exists; use a new --out-dir")

    media = media_root / case["media"]["path"]
    media_sha = sha256_file(media)
    expected = case["media"].get("sha256")
    if expected and expected != media_sha:
        raise OracleError(f"media hash {media_sha} != expected {expected}")

    manifest: Dict[str, Any] = {
        "case_id": case["case_id"], "description": case.get("description", ""),
        "started_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "side": "amspirit", "identity": identity(ams),
        "media": {"path": case["media"]["path"], "sha256": media_sha},
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
        hard_reset = case["media"].get("hard_reset", True)
        origin = ams.load_media(media, hard_reset)
        action = "media load and hard reset" if hard_reset else "media load (no reset)"
        manifest["frame_origin"] = {"frames": origin, "definition": f"emu.frames read right after {action}"}
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
                ams.post_json("/api/config", {"paused": True})
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
    s = sub.add_parser("load", help="load media (hard reset by default)")
    s.add_argument("media", type=Path)
    s.add_argument("--no-reset", action="store_true")
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
    s.add_argument("buttons", nargs="+", choices=sorted(JOY_BITS))
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
            out = {"frame_origin": ams.load_media(args.media, not args.no_reset),
                   "sha256": sha256_file(args.media)}
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
