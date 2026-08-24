# ACCC v1.10 extraction manifest

Local, regenerable extraction of the project's accuracy reference for verification work
(Phase 1 of `docs/plans/2026-08-22-accc-review-plan.md`).

**Everything in this directory is untracked** (see `.gitignore`). Bulk full-text extraction of
*The Amstrad CPC CRTC Compendium* (CC BY-NC-ND, Longshot / Logon System) must not be
committed; this file documents how to rebuild it in under a minute. Only curated, cited
transcriptions inside review docs get committed, matching the existing digests' practice.

## Source

- `docs/ACCC1.10-EN.pdf`, 295 PDF pages
- SHA-256 `1bd6f0e3a06022d03fd40b51d4d622afef2675954a483780f0922cdf1e33a560`

Verify before regenerating:

```sh
shasum -a 256 docs/ACCC1.10-EN.pdf
```

## Regeneration

```sh
uv venv .venv && uv pip install --python .venv/bin/python pdf-inspector pymupdf
mkdir -p docs/accuracy/extract/pdf2md docs/accuracy/extract/pdftotext docs/accuracy/extract/pages

# Primary extractor: position-aware Markdown, one <!-- ======== PAGE N ======== --> marker per page
.venv/bin/python - <<'EOF'
import pdf_inspector as pi
res = pi.extract_pages_markdown("docs/ACCC1.10-EN.pdf")
out_md, out_txt = [], []
for i, p in enumerate(res.pages, 1):
    md = p.markdown or ""
    out_md.append(f"\n\n<!-- ======== PAGE {i} ======== -->\n\n{md}")
    out_txt.append(md)
open("docs/accuracy/extract/pdf2md/accc-v1.10-paged.md","w").write("".join(out_md))
open("docs/accuracy/extract/pdf2md/accc-v1.10-notags.txt","w").write("\n\f".join(out_txt))
print("pages:", len(res.pages))
EOF

# Independent second opinion (plain layout-preserving text)
pdftotext -layout docs/ACCC1.10-EN.pdf docs/accuracy/extract/pdftotext/accc-v1.10.txt

# Visual tier: render diagram/table-heavy pages at 200 dpi (0-indexed page numbers below)
.venv/bin/python - <<'EOF'
import pymupdf, os
d = pymupdf.open("docs/ACCC1.10-EN.pdf")
PAGES = [34, 74, 75, 76, 78, 79, 81, 90, 103, 104, 105, 106, 107, 108, 122, 123, 127, 128,
         129, 130, 133, 135, 136, 137, 139, 140, 144, 149, 150, 152, 157, 160, 166, 183,
         185, 196, 197, 205, 206, 207, 210, 211, 212, 219, 220, 221, 222, 223, 224, 225,
         242, 247, 248]
os.makedirs("docs/accuracy/extract/pages", exist_ok=True)
for n in PAGES:
    d[n-1].get_pixmap(dpi=200).save(f"docs/accuracy/extract/pages/p{n:03d}.png")
EOF
```

## Verification protocol

- pdf-inspector Markdown (`pdf2md/`) is the primary text layer and outranks pdftotext where
  they disagree (2026-08-24 decision: pdftotext `-layout` is the weaker extractor; keep it
  only as an optional second opinion when the Markdown itself looks ambiguous).
- Table/chronogram rules are judged from the rendered PNGs (multimodal), never from any
  text layer alone. The digests' ⚠ VERIFY flags mark exactly the pages above; extend `PAGES`
  when review surfaces more.
- Known extractor behaviour: pdf-inspector resolves Fable's "column smearing" complaints on
  prose pages (e.g. p.75 reads cleanly); genuine chronograms/pixel grids remain unusable in
  both text layers and require the visual tier.
- Logged extraction gap (2026-08-24): pdf2md dropped the entire first paragraph of §11.6.4 on
  p.90 (it is present in pdftotext and in the render). Do not treat a missing paragraph in
  the Markdown layer as proof the PDF lacks it.

## Produced files

| Path | Contents |
|---|---|
| `pdf2md/accc-v1.10-paged.md` | Full document, page-anchored Markdown (~607 kB) |
| `pdf2md/accc-v1.10-notags.txt` | Same content without markers, form-feed separated |
| `pdftotext/accc-v1.10.txt` | poppler `-layout` text (~922 kB) |
| `pages/pNNN.png` | 200 dpi renders of the flagged pages listed above |

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
