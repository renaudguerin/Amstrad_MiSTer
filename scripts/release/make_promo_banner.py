#!/usr/bin/env python3
"""Generate a promotional screenshot collage banner for Amstrad MiSTer releases.

Arranges screenshots from the captures directory into a balanced 4x3 grid,
trims black borders and Copter271's blue/black borders, and preserves a uniform
aspect ratio across tiles (leaving pillarbox black bars for narrow playfields like
Puzzle Bobble and Ghosts 'n Goblins).
"""

import argparse
import os
import sys
from pathlib import Path
from PIL import Image

DEFAULT_LAYOUT = [
    'burninrubber.png', 'robocop2.png', 'sonic.png', 'dicktracy.png',
    'copter271.png', 'puzzle.png', 'eerie.png', 'pangplus.png',
    'plotting.png', 'ghosts.png', 'wos.png', 'navyseals.png'
]

COPTER_BORDER_COLORS = [
    (0, 0, 102), (0, 0, 119), (0, 0, 85), (0, 0, 68),
    (17, 17, 0)
]


def detect_active_bounds(im, filename):
    """Find the bounding box of active game content."""
    w, h = im.size
    min_x, max_x = w, 0
    min_y, max_y = h, 0
    pix = im.load()

    for y in range(h):
        for x in range(w):
            p = pix[x, y]
            is_border = False
            if filename == 'copter271.png':
                if p == (0, 0, 0) or p in COPTER_BORDER_COLORS:
                    is_border = True
            else:
                if p == (0, 0, 0):
                    is_border = True
            if not is_border:
                if x < min_x:
                    min_x = x
                if x > max_x:
                    max_x = x
                if y < min_y:
                    min_y = y
                if y > max_y:
                    max_y = y

    return min_x, min_y, max_x, max_y


def get_clean_tile(im, filename, target_ar):
    """Crop image to target aspect ratio, removing borders while keeping full content."""
    w, h = im.size
    min_x, min_y, max_x, max_y = detect_active_bounds(im, filename)

    cw = max_x - min_x + 1
    ch = max_y - min_y + 1
    cx = (min_x + max_x) / 2
    cy = (min_y + max_y) / 2

    # Clean copter271 blue border pixels to black
    work_im = im.copy()
    if filename == 'copter271.png':
        pix = work_im.load()
        for y in range(h):
            for x in range(w):
                if pix[x, y] in COPTER_BORDER_COLORS:
                    pix[x, y] = (0, 0, 0)

    # Determine crop dimensions that contain the active area and match target_ar
    content_ar = cw / ch
    if content_ar > target_ar:
        crop_w = cw
        crop_h = cw / target_ar
    else:
        crop_h = ch
        crop_w = ch * target_ar

    x1 = cx - crop_w / 2
    x2 = cx + crop_w / 2
    y1 = cy - crop_h / 2
    y2 = cy + crop_h / 2

    out_w = int(round(crop_w))
    out_h = int(round(crop_h))

    if x1 >= 0 and x2 <= w and y1 >= 0 and y2 <= h:
        tile = work_im.crop((int(round(x1)), int(round(y1)), int(round(x2)), int(round(y2))))
    else:
        canvas = Image.new('RGB', (out_w, out_h), (0, 0, 0))
        paste_x = int(round((out_w - cw) / 2))
        paste_y = int(round((out_h - ch) / 2))
        content_crop = work_im.crop((min_x, min_y, max_x + 1, max_y + 1))
        canvas.paste(content_crop, (paste_x, paste_y))
        tile = canvas

    return tile


def generate_banner(input_dir, output_path, target_ar=887/600, cell_w=887, gap=8, border=10, bg=(10, 12, 16)):
    """Generate the 4x3 collage banner."""
    cell_h = int(round(cell_w / target_ar))
    cols, rows = 4, 3
    bw = cols * cell_w + (cols - 1) * gap + 2 * border
    bh = rows * cell_h + (rows - 1) * gap + 2 * border

    banner = Image.new('RGB', (bw, bh), bg)

    for idx, fname in enumerate(DEFAULT_LAYOUT):
        r = idx // cols
        c = idx % cols
        fpath = Path(input_dir) / fname
        if not fpath.exists():
            print(f"Warning: {fpath} not found, skipping", file=sys.stderr)
            continue
        im = Image.open(fpath)
        tile = get_clean_tile(im, fname, target_ar)
        resized = tile.resize((cell_w, cell_h), Image.Resampling.LANCZOS)
        x = border + c * (cell_w + gap)
        y = border + r * (cell_h + gap)
        banner.paste(resized, (x, y))

    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    banner.save(output_path, quality=95)
    print(f"Banner saved to {output_path} ({banner.size[0]}x{banner.size[1]})")
    return output_path


def main():
    parser = argparse.ArgumentParser(description="Create promotional collage banner from captures.")
    parser.add_argument("--input-dir", default="local/captures/working", help="Path to captures directory")
    parser.add_argument("--output", default="local/captures/promo_banner.png", help="Output banner path")
    parser.add_argument("--mode", choices=["cpc", "16x9-framed", "16x9-seamless"], default="cpc",
                        help="Banner preset mode (cpc = native active area ratio ~1.48; 16x9-framed = 4:3 cells with sleek gap; 16x9-seamless = 4:3 cells with 0 gap)")
    parser.add_argument("--cell-width", type=int, default=None, help="Custom cell width")
    args = parser.parse_args()

    if args.mode == "cpc":
        cw = args.cell_width or 887
        generate_banner(args.input_dir, args.output, target_ar=887/600, cell_w=cw, gap=8, border=10)
    elif args.mode == "16x9-framed":
        cw = args.cell_width or 800
        generate_banner(args.input_dir, args.output, target_ar=4/3, cell_w=cw, gap=8, border=10)
    elif args.mode == "16x9-seamless":
        cw = args.cell_width or 800
        generate_banner(args.input_dir, args.output, target_ar=4/3, cell_w=cw, gap=0, border=0, bg=(0, 0, 0))


if __name__ == "__main__":
    main()
