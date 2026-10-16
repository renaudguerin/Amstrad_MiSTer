#!/usr/bin/env python3
"""Diff MiSTer SHAKER captures against the Logon System reference pull.

Join key: (SSM hex, CRTC). MiSTer files are MISTER_<crtc>_<HHHH>.png;
reference files are indexed in index.json (see scripts/shaker-ref/README.md).

Geometry caveat: MiSTer native captures are 768x273 (async scaler buffer)
while AmSpirit references are 768x542. The reference is downscaled to MiSTer
size (LANCZOS) before diffing, which hides sub-line detail and can both hide
and invent small differences. Metrics here are triage signals for selecting
pairs a human (or photo-oracle) must judge, never pass/fail verdicts.

Output: <out>/results.json (per-pair metrics), <out>/diff_<...>.png
side-by-side composites for differing pairs (left MiSTer, middle reference
downscaled, right amplified abs-diff), and a printed summary table.

Requires Pillow.
"""
import argparse
import datetime
import json
import os
import re
import sys
from collections import Counter

try:
    from PIL import Image, ImageChops
except ImportError:
    sys.exit("Pillow is required (pip install Pillow)")

MISTER_RE = re.compile(r"MISTER_(\d+)_([0-9A-Fa-f]{4})\.png$")
MOD_RE = re.compile(r"(?:^|[/-])([A-E])(?=\d|-)")
DIFF_THRESH = 12  # per-channel sum threshold for a "differing" pixel


def load_rgb(path, size=None):
    im = Image.open(path).convert("RGB")
    if size is not None and im.size != size:
        im = im.resize(size, Image.LANCZOS)
    return im


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--ref-index", required=True,
                    help="index.json from scrape_logon.py")
    ap.add_argument("--mister", action="append", required=True,
                    help="MiSTer runs dir to scan (repeatable)")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--max-composites", type=int, default=300)
    args = ap.parse_args()

    index = json.load(open(args.ref_index))
    ref_by_key = {}
    for m in index["images"]:
        ref_by_key.setdefault((m["hex"].upper(), m["crtc"]), []).append(m)
    ref_root = os.path.dirname(os.path.abspath(args.ref_index))

    pairs = []  # (mister_path, hex, crtc, module, ref_rec|None)
    for root in args.mister:
        for dp, _, fns in os.walk(root):
            for fn in sorted(fns):
                m = MISTER_RE.search(fn)
                if not m:
                    continue
                crtc, hexcode = int(m.group(1)), m.group(2).upper()
                mod = MOD_RE.search(os.path.basename(dp.rstrip("/")))
                module = mod.group(1) if mod else "?"
                cands = ref_by_key.get((hexcode, crtc), [])
                pairs.append((os.path.join(dp, fn), hexcode, crtc, module,
                              cands[0] if len(cands) == 1 else None,
                              len(cands)))

    os.makedirs(args.out_dir, exist_ok=True)
    results = {"generated_utc": datetime.datetime.now(
        datetime.timezone.utc).isoformat(),
        "ref_index": os.path.abspath(args.ref_index),
        "mister_roots": [os.path.abspath(r) for r in args.mister],
        "geometry_note": "reference downscaled 768x542 -> 768x273 (LANCZOS); "
                         "metrics are triage signals, not verdicts.",
        "pairs": []}
    n_comp = 0
    for mpath, hexcode, crtc, module, ref, ncand in pairs:
        rec = {"mister": os.path.relpath(mpath),
               "hex": hexcode, "crtc": crtc, "module": module}
        if ref is None:
            rec["status"] = "no-reference" if ncand == 0 else "ambiguous"
            rec["candidates"] = ncand
            results["pairs"].append(rec)
            continue
        rec.update({"ref": ref["file"], "test": ref["test"],
                    "ref_test_name": ref.get("name"), "sub": ref.get("sub"),
                    "subfolder": ref.get("subfolder")})
        try:
            mi = load_rgb(mpath)
            rf = load_rgb(os.path.join(ref_root, ref["file"]), mi.size)
        except Exception as e:  # noqa: BLE001
            rec["status"] = f"load-error: {e}"
            results["pairs"].append(rec)
            continue
        diff = ImageChops.difference(mi, rf)
        extrema = diff.getextrema()
        identical = all(lo == hi == 0 for lo, hi in extrema)
        if identical:
            rec["status"] = "identical-after-downscale"
            results["pairs"].append(rec)
            continue
        h = diff.histogram()
        # summed abs-diff per pixel approximated from per-channel histograms
        px = mi.size[0] * mi.size[1]
        tot = sum(i * n for ch in range(3)
                  for i, n in enumerate(h[ch * 256:(ch + 1) * 256]))
        rec["status"] = "differs"
        rec["mean_abs_diff"] = round(tot / (3 * px), 3)
        # rows containing any differing pixel (thresholded)
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
            comp = Image.new("RGB", (mi.size[0] * 3 + 20, mi.size[1]), "white")
            comp.paste(mi, (0, 0))
            comp.paste(rf, (mi.size[0] + 10, 0))
            comp.paste(amp, (mi.size[0] * 2 + 20, 0))
            cfn = f"diff_{module}{crtc}_{hexcode}.png"
            comp.save(os.path.join(args.out_dir, cfn))
            rec["composite"] = cfn
            n_comp += 1
        results["pairs"].append(rec)

    with open(os.path.join(args.out_dir, "results.json"), "w") as f:
        json.dump(results, f, indent=1)

    st = Counter(r["status"] for r in results["pairs"])
    print(f"pairs={len(results['pairs'])} " +
          " ".join(f"{k}={v}" for k, v in sorted(st.items())))
    print(f"out={args.out_dir} composites={n_comp}")
    worst = sorted((r for r in results["pairs"] if r["status"] == "differs"),
                   key=lambda r: -r["differing_rows"])[:15]
    for r in worst:
        print(f"  {r['module']}{r['crtc']} {r['hex']} {r.get('test')}/{r.get('sub')} "
              f"rows={r['differing_rows']} first={r['first_row']} mean={r['mean_abs_diff']}")


if __name__ == "__main__":
    main()
