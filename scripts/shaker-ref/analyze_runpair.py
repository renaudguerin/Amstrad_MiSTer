#!/usr/bin/env python3
"""Classify Full-vs-<mode> capture pairs: shift-only, content-change, flat.

For each capture pair of the same (cell, version, CRTC, hex) in two MiSTer
run trees (see compare_runs.py), find the integer (dx, dy) that best aligns
tree B onto tree A and report the residual difference after alignment.

Why: sync-filter modes change the acquisition window fed to the scaler (see
docs/investigations/hardware-runs/capture-reliability-2026-09-22.md), so the
raw pixel diff is dominated by a global offset. The aligned residual and the
best shift are the useful signals:

- flat: one side is a (near) uniform field -> scaler lost lock in that mode,
  not a content observation.
- shift-only: aligned residual below --shift-thresh -> same picture, moved.
- content-change: aligned residual above -> the visible content really
  differs between modes (phase, crop or rendering).

Thresholds are triage aids, not verdicts. Requires Pillow + numpy.
"""
import argparse
import json
import os
import sys
from collections import Counter, defaultdict

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from compare_runs import scan_tree  # noqa: E402

FLAT_CONTENT = 0.05   # <5% non-dominant pixels => flat field
SHIFT_THRESH = 6.0    # aligned mean abs diff below this => shift-only


def gray(im):
    return np.asarray(im.convert("L"), dtype=np.float32)


def content_fraction(arr):
    """Fraction of pixels not within tolerance of the modal value."""
    vals, counts = np.unique(arr.astype(np.uint8), return_counts=True)
    dom = vals[counts.argmax()]
    return float(np.count_nonzero(np.abs(arr - dom) > 24) / arr.size)


def best_shift(a, b, max_dx, max_dy, margin=8):
    """Coarse-to-fine integer shift of b onto a minimizing mean |diff|."""
    def mean_diff(aa, bb, dx, dy, m):
        h, w = aa.shape
        y0, y1 = m, h - m
        x0, x1 = m, w - m
        by0, by1 = y0 + dy, y1 + dy
        bx0, bx1 = x0 + dx, x1 + dx
        if by0 < 0 or bx0 < 0 or by1 > h or bx1 > w:
            return None
        return float(np.mean(np.abs(aa[y0:y1, x0:x1] -
                                    bb[by0:by1, bx0:bx1])))

    # coarse on /4 scale
    a4, b4 = a[::4, ::4], b[::4, ::4]
    m4 = max(1, margin // 4)
    coarse = None
    for dy in range(-max_dy // 4, max_dy // 4 + 1, 2):
        for dx in range(-max_dx // 4, max_dx // 4 + 1, 2):
            v = mean_diff(a4, b4, dx, dy, m4)
            if v is not None and (coarse is None or v < coarse[0]):
                coarse = (v, dx, dy)
    if coarse is None:
        return None
    _, cdx, cdy = coarse
    best = None
    for dy in range(cdy * 4 - 6, cdy * 4 + 7):
        for dx in range(cdx * 4 - 6, cdx * 4 + 7):
            if abs(dx) > max_dx or abs(dy) > max_dy:
                continue
            v = mean_diff(a, b, dx, dy, margin)
            if v is not None and (best is None or v < best[0]):
                best = (v, dx, dy)
    return best


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--a", required=True)
    ap.add_argument("--b", required=True)
    ap.add_argument("--out", required=True, help="JSON output path")
    ap.add_argument("--max-dx", type=int, default=64)
    ap.add_argument("--max-dy", type=int, default=32)
    ap.add_argument("--a-label", default="full")
    ap.add_argument("--b-label", default="raw-crt")
    ap.add_argument("--limit", type=int, default=0, help="debug: cap pairs")
    args = ap.parse_args()

    ta, tb = scan_tree(args.a), scan_tree(args.b)
    keys = sorted(set(ta) & set(tb))
    if args.limit:
        keys = keys[:args.limit]
    rows = []
    for key in keys:
        cell, ver, crtc, hexcode = key
        rec = {"cell": cell, "ver": ver, "crtc": crtc, "hex": hexcode}
        try:
            ia = Image.open(ta[key]["path"]).convert("RGB")
            ib = Image.open(tb[key]["path"]).convert("RGB")
            if ia.size != ib.size:
                rec["size_mismatch"] = [list(ia.size), list(ib.size)]
                ib = ib.resize(ia.size, Image.LANCZOS)
            ga, gb = gray(ia), gray(ib)
            rec["content_frac_a"] = round(content_fraction(ga), 4)
            rec["content_frac_b"] = round(content_fraction(gb), 4)
            if (rec["content_frac_a"] < FLAT_CONTENT or
                    rec["content_frac_b"] < FLAT_CONTENT):
                rec["class"] = "flat"
                aa = np.asarray(ia, dtype=np.float32)
                bb = np.asarray(ib, dtype=np.float32)
                rec["unaligned_mean"] = round(float(np.mean(np.abs(aa - bb))), 3)
                rows.append(rec)
                continue
            sh = best_shift(ga, gb, args.max_dx, args.max_dy)
            if sh is None:
                rec["class"] = "shift-search-failed"
                rows.append(rec)
                continue
            aligned, dx, dy = sh
            aa = np.asarray(ia, dtype=np.float32)
            bb = np.asarray(ib, dtype=np.float32)
            h, w = ga.shape
            m = 8
            y0, y1 = m, h - m
            x0, x1 = m, w - m
            over_a = aa[y0:y1, x0:x1]
            over_b = bb[y0 + dy:y1 + dy, x0 + dx:x1 + dx]
            rec["unaligned_mean"] = round(float(np.mean(np.abs(aa - bb))), 3)
            rec["dx"], rec["dy"] = dx, dy
            rec["aligned_mean"] = round(float(np.mean(np.abs(over_a - over_b))), 3)
            rec["class"] = ("shift-only" if rec["aligned_mean"] < SHIFT_THRESH
                            else "content-change")
        except Exception as e:  # noqa: BLE001
            rec["class"] = f"load-error: {e}"
        rows.append(rec)

    out = {"a": args.a, "b": args.b, "a_label": args.a_label,
           "b_label": args.b_label, "pairs": rows}
    with open(args.out, "w") as f:
        json.dump(out, f, indent=1)

    st = Counter(r["class"] for r in rows)
    print(f"pairs={len(rows)} " + " ".join(f"{k}={v}" for k, v in sorted(st.items())))
    shifts = Counter((r["dx"], r["dy"]) for r in rows if "dx" in r)
    print("top shifts (dx,dy):", shifts.most_common(8))
    cc = [r for r in rows if r["class"] == "content-change"]
    print(f"content-change: {len(cc)}; by cell:",
          dict(sorted(Counter(r["cell"] for r in cc).items())))
    by_cell = defaultdict(list)
    for r in rows:
        if r.get("aligned_mean") is not None:
            by_cell[r["cell"]].append(r["aligned_mean"])
    print("aligned-mean per cell (median / p90 / max):")
    for cell in sorted(by_cell):
        v = np.array(by_cell[cell])
        print(f"  {cell}: n={len(v)} med={np.median(v):.1f} "
              f"p90={np.percentile(v, 90):.1f} max={v.max():.1f}")
    print("top 15 by aligned_mean:")
    for r in sorted(cc, key=lambda r: -r["aligned_mean"])[:15]:
        print(f"  {r['cell']}-{r['ver']} {r['hex']} crtc={r['crtc']} "
              f"aligned={r['aligned_mean']} unaligned={r['unaligned_mean']} "
              f"shift=({r['dx']},{r['dy']})")
    flat = [r for r in rows if r["class"] == "flat"]
    print(f"flat pairs ({len(flat)}):")
    for r in flat[:20]:
        side = 'a' if r["content_frac_a"] < FLAT_CONTENT and r["content_frac_b"] >= FLAT_CONTENT else (
            'b' if r["content_frac_b"] < FLAT_CONTENT and r["content_frac_a"] >= FLAT_CONTENT else 'both')
        print(f"  {r['cell']}-{r['ver']} {r['hex']} crtc={r['crtc']} "
              f"flat_side={side} frac_a={r['content_frac_a']} frac_b={r['content_frac_b']}")


if __name__ == "__main__":
    main()
