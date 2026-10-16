#!/usr/bin/env python3
"""Scrape Logon System SHAKER reference screenshots for later comparison.

Source: https://shaker.logonsystem.eu (API: /api/archs, /api/tests;
images: /images/<arch-id>/<arch-version>/<test>_CRTC<c>[_<subfolder>]_<sub>.webp).
Reference screenshots (c) their authors; fetched for comparison only, see index.json.

Layout: <out>/<arch-key>/CRTC<c>/<test>_<hex>[_<subfolder>]_<sub>.webp
plus <out>/index.json joining on SSM hex code with our MiSTer captures
(MISTER_<n>_<hex>.png in the hardware-loop manifests).

Stdlib only. Resumable: existing files are skipped unless --overwrite.
Content-sniffed: the site returns HTTP 200 HTML for missing images, so only
RIFF/WEBP payloads are kept; missing ones are recorded in index.json.
"""
import argparse
import datetime
import hashlib
import json
import os
import sys
import time
import urllib.request

BASE = "https://shaker.logonsystem.eu"
UA = {"User-Agent": "Amstrad_MiSTer ref-scraper (comparison only)"}


def get_json(path):
    req = urllib.request.Request(BASE + path, headers=UA)
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def get_bytes(path):
    req = urllib.request.Request(BASE + path, headers=UA)
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read()


def is_webp(blob):
    return len(blob) > 12 and blob[:4] == b"RIFF" and blob[8:12] == b"WEBP"


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--arch", action="append", default=[],
                    help="arch key id_version, e.g. amspirit_lite-1.15 (repeatable)")
    ap.add_argument("--crtc", action="append", type=int, default=[],
                    help="CRTC number filter (repeatable)")
    ap.add_argument("--module", action="append", default=[],
                    help="module letter filter, e.g. A (repeatable)")
    ap.add_argument("--test", action="append", default=[],
                    help="test id filter, e.g. A1 (repeatable)")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--overwrite", action="store_true")
    ap.add_argument("--delay", type=float, default=0.1,
                    help="seconds between downloads")
    args = ap.parse_args()

    archs = get_json("/api/archs")
    tests = get_json("/api/tests")
    by_key = {a["id"] + "_" + a["version"]: a for a in archs}
    want_archs = args.arch or ["amspirit_lite-1.15"]
    for k in want_archs:
        if k not in by_key:
            sys.exit(f"unknown arch {k!r}; choices: {sorted(by_key)}")

    index = {"source": BASE,
             "attribution": "Reference screenshots (c) Logon System / contributors; "
                            "fetched for comparison only, not committed.",
             "fetched_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
             "archs": {k: by_key[k] for k in want_archs},
             "images": [], "missing": []}
    n_ok = n_skip = n_missing = 0
    for t in sorted(tests, key=lambda t: t["id"]):
        if args.module and t.get("module") not in args.module:
            continue
        if args.test and t["id"] not in args.test:
            continue
        for sub in t.get("subtests", []):
            for crtc in sub.get("crtcs", []):
                if args.crtc and crtc not in args.crtc:
                    continue
                for akey in want_archs:
                    arch_path = akey.replace("_", "/", 1)
                    parts = [t["id"], f"CRTC{crtc}"]
                    if sub.get("subfolder"):
                        parts.append(sub["subfolder"])
                    parts.append(sub["subTest"])
                    name = "_".join(parts)
                    url_path = f"/images/{arch_path}/{name}.webp"
                    hexcode = sub.get("hex", "?")
                    fn = f"{t['id']}_{hexcode}"
                    if sub.get("subfolder"):
                        fn += f"_{sub['subfolder']}"
                    fn += f"_{sub['subTest']}.webp"
                    dest = os.path.join(args.out_dir, akey, f"CRTC{crtc}", fn)
                    rec = {"test": t["id"], "name": t.get("name"),
                           "module": t.get("module"), "crtc": crtc,
                           "hex": hexcode, "subfolder": sub.get("subfolder"),
                           "sub": sub["subTest"], "arch": akey,
                           "url": BASE + url_path,
                           "file": os.path.relpath(dest, args.out_dir)}
                    if os.path.exists(dest) and not args.overwrite:
                        index["images"].append(rec)
                        n_skip += 1
                        continue
                    try:
                        blob = get_bytes(url_path)
                    except Exception as e:  # noqa: BLE001
                        rec["error"] = f"{type(e).__name__}: {e}"
                        index["missing"].append(rec)
                        n_missing += 1
                        continue
                    if not is_webp(blob):
                        rec["error"] = f"not WEBP ({len(blob)}B)"
                        index["missing"].append(rec)
                        n_missing += 1
                        continue
                    os.makedirs(os.path.dirname(dest), exist_ok=True)
                    with open(dest, "wb") as f:
                        f.write(blob)
                    rec["sha256"] = hashlib.sha256(blob).hexdigest()
                    rec["bytes"] = len(blob)
                    index["images"].append(rec)
                    n_ok += 1
                    time.sleep(args.delay)
    with open(os.path.join(args.out_dir, "index.json"), "w") as f:
        json.dump(index, f, indent=1)
    print(f"scrape: fetched={n_ok} skipped={n_skip} missing={n_missing} "
          f"tests={len(tests)} out={args.out_dir}")


if __name__ == "__main__":
    main()
