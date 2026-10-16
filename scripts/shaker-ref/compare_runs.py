#!/usr/bin/env python3
"""Diff two MiSTer SHAKER capture trees against each other.

Purpose: phase-2 sync-filter comparison (e.g. Full vs raw-crt). Same cells,
same versions, joined on (cell, suite version, CRTC, SSM hex); picks one
attempt dir per (cell, version) preferring a success manifest, else the
attempt with the most captures.

Geometry: both sides come from the same runner/capture path, so sizes should
match; if a pair's sizes differ the smaller is upscaled (LANCZOS) with a note.
Metrics are triage signals for filter-sensitivity, never verdicts.

Output: <out>/results.json, runpair_<cell>-<ver>_<crtc>_<hex>.png composites
(left A, middle B, right amplified abs-diff), and a printed summary.

Requires Pillow.
"""
import argparse
import datetime
import json
import os
import re
import sys
from collections import Counter, defaultdict

try:
    from PIL import Image, ImageChops
except ImportError:
    sys.exit("Pillow is required (pip install Pillow)")

MISTER_RE = re.compile(r"MISTER_(\d+)_([0-9A-Fa-f]{4})\.png$")
DIR_RE = re.compile(r"^([A-E])(\d)-(\d+)-(.+?)-a(\d+)$")


def scan_tree(root):
    """Return {(cell, ver, crtc, hex): {path, dir, attempt, status}}."""
    by_dir = defaultdict(lambda: {"pngs": [], "status": None})
    for dp, _, fns in os.walk(root):
        base = os.path.basename(dp.rstrip("/"))
        m = DIR_RE.match(base)
        if not m:
            continue
        key = m.groups()
        for fn in fns:
            if fn == "manifest.json":
                try:
                    by_dir[key]["status"] = json.load(
                        open(os.path.join(dp, fn))).get("status")
                except Exception:  # noqa: BLE001
                    pass
            pm = MISTER_RE.search(fn)
            if pm:
                by_dir[key]["pngs"].append(
                    (int(pm.group(1)), pm.group(2).upper(),
                     os.path.join(dp, fn)))

    chosen = {}
    for (mod, num, ver, filt, attempt), rec in by_dir.items():
        if not rec["pngs"]:
            continue
        cell, ver_i, att = f"{mod}{num}", int(ver), int(attempt)
        cur = chosen.get((cell, ver_i))
        rank = (rec["status"] == "success", len(rec["pngs"]), -att)
        if cur is None or rank > cur["rank"]:
            chosen[(cell, ver_i)] = {"rank": rank, "dir": None, "pngs": []}
            chosen[(cell, ver_i)]["pngs"] = rec["pngs"]
            chosen[(cell, ver_i)]["dir"] = os.path.dirname(rec["pngs"][0][2])
            chosen[(cell, ver_i)]["status"] = rec["status"]
            chosen[(cell, ver_i)]["filter"] = filt
    out = {}
    for (cell, ver), rec in chosen.items():
        for crtc, hexcode, path in rec["pngs"]:
            out[(cell, ver, crtc, hexcode)] = {
                "path": path, "dir": rec["dir"], "status": rec["status"],
                "filter": rec["filter"], "rel": os.path.relpath(path, root)}
    return out


def load_rgb(path, size=None):
    im = Image.open(path).convert("RGB")
    if size is not None and im.size != size:
        im = im.resize(size, Image.LANCZOS)
    return im


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--a", required=True, help="run tree A (e.g. Full)")
    ap.add_argument("--b", required=True, help="run tree B (e.g. raw-crt)")
    ap.add_argument("--a-label", default="full")
    ap.add_argument("--b-label", default="raw-crt")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--max-composites", type=int, default=300)
    args = ap.parse_args()

    ta, tb = scan_tree(args.a), scan_tree(args.b)
    os.makedirs(args.out_dir, exist_ok=True)
    results = {
        "generated_utc": datetime.datetime.now(
            datetime.timezone.utc).isoformat(),
        "a": {"root": os.path.abspath(args.a), "label": args.a_label},
        "b": {"root": os.path.abspath(args.b), "label": args.b_label},
        "note": "same-runner capture pair diff; triage signal only.",
        "pairs": [], "a_only": [], "b_only": [],
    }
    n_comp = 0
    for key in sorted(set(ta) | set(tb)):
        cell, ver, crtc, hexcode = key
        if key not in tb:
            results["a_only"].append({"cell": cell, "ver": ver, "crtc": crtc,
                                      "hex": hexcode, "a": ta[key]["rel"]})
            continue
        if key not in ta:
            results["b_only"].append({"cell": cell, "ver": ver, "crtc": crtc,
                                      "hex": hexcode, "b": tb[key]["rel"]})
            continue
        ra, rb = ta[key], tb[key]
        rec = {"cell": cell, "ver": ver, "crtc": crtc, "hex": hexcode,
               "a": ra["rel"], "b": rb["rel"],
               "a_dir_status": ra["status"], "b_dir_status": rb["status"],
               "a_attempt_dir": os.path.basename(ra["dir"]),
               "b_attempt_dir": os.path.basename(rb["dir"])}
        try:
            ia = Image.open(ra["path"]).convert("RGB")
            ib = Image.open(rb["path"]).convert("RGB")
            if ia.size != ib.size:
                rec["size_mismatch"] = [list(ia.size), list(ib.size)]
                big = max(ia.size, ib.size)
                ia = ia.resize(big, Image.LANCZOS)
                ib = ib.resize(big, Image.LANCZOS)
            diff = ImageChops.difference(ia, ib)
            if all(lo == hi == 0 for lo, hi in diff.getextrema()):
                rec["status"] = "identical"
                results["pairs"].append(rec)
                continue
            h = diff.histogram()
            px = ia.size[0] * ia.size[1]
            tot = sum(i * n for ch in range(3)
                      for i, n in enumerate(h[ch * 256:(ch + 1) * 256]))
            rec["status"] = "differs"
            rec["mean_abs_diff"] = round(tot / (3 * px), 3)
            gr = diff.convert("L").point(lambda v: 255 if v > 4 else 0)
            cols, rows = gr.size
            rowpx = list(gr.getdata())
            drows = [y for y in range(rows)
                     if any(rowpx[y * cols:(y + 1) * cols])]
            rec["differing_rows"] = len(drows)
            rec["first_row"] = drows[0] if drows else None
            rec["last_row"] = drows[-1] if drows else None
            if n_comp < args.max_composites:
                amp = diff.point(lambda v: min(255, v * 4))
                comp = Image.new("RGB", (ia.size[0] * 3 + 20, ia.size[1]),
                                 "white")
                comp.paste(ia, (0, 0))
                comp.paste(ib, (ia.size[0] + 10, 0))
                comp.paste(amp, (ia.size[0] * 2 + 20, 0))
                cfn = f"runpair_{cell}-{ver}_{crtc}_{hexcode}.png"
                comp.save(os.path.join(args.out_dir, cfn))
                rec["composite"] = cfn
                n_comp += 1
        except Exception as e:  # noqa: BLE001
            rec["status"] = f"load-error: {e}"
        results["pairs"].append(rec)

    with open(os.path.join(args.out_dir, "results.json"), "w") as f:
        json.dump(results, f, indent=1)

    st = Counter(r["status"] for r in results["pairs"])
    print(f"pairs={len(results['pairs'])} " +
          " ".join(f"{k}={v}" for k, v in sorted(st.items())) +
          f" a_only={len(results['a_only'])} b_only={len(results['b_only'])}")
    print(f"out={args.out_dir} composites={n_comp}")
    diffs = [r for r in results["pairs"] if r.get("status") == "differs"]
    cells = defaultdict(list)
    for r in diffs:
        cells[(r["cell"], r["ver"])].append(r["mean_abs_diff"])
    print("per-cell changed-capture stats (differs only), worst ten by mean:")
    for (cell, ver), means in sorted(
            cells.items(), key=lambda kv: -max(kv[1]))[:10]:
        print(f"  {cell}-{ver}: n={len(means)} max={max(means)} "
              f"mean_of_means={round(sum(means)/len(means), 2)}")
    print("top 15 pairs by mean_abs_diff:")
    for r in sorted(diffs, key=lambda r: -r["mean_abs_diff"])[:15]:
        print(f"  {r['cell']}-{r['ver']} {r['hex']} crtc={r['crtc']} "
              f"mean={r['mean_abs_diff']} rows={r['differing_rows']}")


if __name__ == "__main__":
    main()
