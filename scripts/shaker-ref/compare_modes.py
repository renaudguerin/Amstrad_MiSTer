#!/usr/bin/env python3
"""Paired mode comparison against the AmSpirit reference (compare.py output).

Joins two compare.py results.json on (cell, CRTC, hex), keeps per side the
best (minimum) mean_abs_diff across attempts, and reports per-cell
distributions of (B - A). Negative deltas mean B is closer to the reference.

Usage:
  compare_modes.py --a docs/specs/shaker-compare-2026-10-15/results.json \
                   --b /tmp/raw-vs-ref/results.json \
                   --a-tag runs2/ --b-tag runs-raw-crt/ --out /tmp/modes.json

This is triage arithmetic over existing metrics; the reference itself remains
a visual oracle (see scripts/shaker-ref/README.md).
"""
import argparse
import json
import re
import statistics as st
from collections import defaultdict

CELL_RE = re.compile(r"((?:[A-E]\d)-\d+)-(?:full|raw-crt|raw-pixels)(-a\d+)?$")
SHOT_RE = re.compile(r"MISTER_(\d+)_([0-9A-F]+)\.png")


def norm(mister_rel):
    parts = mister_rel.split("/")
    m = CELL_RE.match(parts[-2])
    s = SHOT_RE.match(parts[-1])
    return (m.group(1), int(s.group(1)), s.group(2).upper())


def build(pairs_path, tag):
    best = {}
    for p in json.load(open(pairs_path))["pairs"]:
        if p.get("status") != "differs" or tag not in p["mister"]:
            continue
        key = norm(p["mister"])
        v = p["mean_abs_diff"]
        if key not in best or v < best[key]:
            best[key] = v
    return best


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--a", required=True)
    ap.add_argument("--b", required=True)
    ap.add_argument("--a-tag", default="runs2/")
    ap.add_argument("--b-tag", default="runs-raw-crt/")
    ap.add_argument("--out", default=None)
    args = ap.parse_args()

    ba, bb = build(args.a, args.a_tag), build(args.b, args.b_tag)
    common = sorted(set(ba) & set(bb))
    rows = [{"cell": k[0], "crtc": k[1], "hex": k[2],
             "a": ba[k], "b": bb[k], "delta": round(bb[k] - ba[k], 3)}
            for k in common]
    print(f"paired keys: {len(rows)} (a-only {len(set(ba)-set(bb))}, "
          f"b-only {len(set(bb)-set(ba))})")
    by_cell = defaultdict(list)
    for r in rows:
        by_cell[r["cell"]].append(r["delta"])
    print("per-cell (B - A); negative = B closer to reference:")
    for cell in sorted(by_cell):
        v = by_cell[cell]
        print(f"  {cell}: n={len(v)} median={st.median(v):+6.1f} "
              f"mean={st.mean(v):+6.1f} better={sum(1 for x in v if x < -3):3d} "
              f"worse={sum(1 for x in v if x > 3):3d} same={sum(1 for x in v if -3 <= x <= 3):3d}")
    for label, key in (("improvements (B closer)", lambda r: r["delta"]),
                       ("regressions (B farther)", lambda r: -r["delta"])):
        print(f"top 8 {label}:")
        for r in sorted(rows, key=key)[:8]:
            print(f"  {r['cell']} {r['hex']} crtc={r['crtc']} "
                  f"a={r['a']} b={r['b']} delta={r['delta']:+.1f}")
    if args.out:
        with open(args.out, "w") as f:
            json.dump({"a": args.a, "b": args.b, "a_tag": args.a_tag,
                       "b_tag": args.b_tag, "pairs": rows}, f, indent=1)


if __name__ == "__main__":
    main()
