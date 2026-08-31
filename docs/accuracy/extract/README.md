# ACCC v1.11 extraction manifest

Local, regenerable extraction of the project's accuracy reference for verification work
(Phase 1 of `docs/plans/2026-08-22-accc-review-plan.md`).

The v1.11 extraction snapshot listed below is intentionally versioned so a fresh session can
reproduce and audit the bilingual findings without regenerating the text and render layers.
The source PDFs remain local and ignored. Other regenerated or ad-hoc intermediate files in
this directory remain ignored unless they are deliberately added to the snapshot.

## Source

- `docs/references/ACCC1.11-EN.pdf`, 295 PDF pages
- SHA-256 `3e45eb7eea7dc8f0d7211f78bec4f8d00530ce3c00da2e76034fb24f7a751868`
- `docs/references/ACCC1.11-FR.pdf`, 295 PDF pages
- SHA-256 `4409e3a2e77cd54e499c6956446b01bce93f79a1c1ba366201d514cf6e3c0d47`
- (Legacy v1.10: `docs/references/ACCC1.10-EN.pdf`, SHA-256 `1bd6f0e3a06022d03fd40b51d4d622afef2675954a483780f0922cdf1e33a560`)

Verify before regenerating:

```sh
shasum -a 256 docs/references/ACCC1.11-EN.pdf docs/references/ACCC1.11-FR.pdf
```

## Regeneration

```sh
uv venv .venv && uv pip install --python .venv/bin/python pdf-inspector pymupdf
mkdir -p docs/accuracy/extract/pdf2md docs/accuracy/extract/pdftotext docs/accuracy/extract/pages

# Current first-pass classifier/extractor for both editions. Its reports decide which
# pages need rendered fallback; keep a writable uv cache outside the repository if needed.
env UV_CACHE_DIR=/tmp/accc-uv-cache uv run \
  /Users/renaudg/.claude/skills/pdf-inspector/scripts/inspect_pdf.py \
  docs/references/ACCC1.11-EN.pdf \
  --output-dir docs/accuracy/extract/inspector-v1.11-en
env UV_CACHE_DIR=/tmp/accc-uv-cache uv run \
  /Users/renaudg/.claude/skills/pdf-inspector/scripts/inspect_pdf.py \
  docs/references/ACCC1.11-FR.pdf \
  --output-dir docs/accuracy/extract/inspector-v1.11-fr

# Primary extractor: position-aware Markdown, one <!-- ======== PAGE N ======== --> marker per page
.venv/bin/python - <<'EOF'
import pdf_inspector as pi
res = pi.extract_pages_markdown("docs/references/ACCC1.11-EN.pdf")
out_md, out_txt = [], []
for i, p in enumerate(res.pages, 1):
    md = p.markdown or ""
    out_md.append(f"\n\n<!-- ======== PAGE {i} ======== -->\n\n{md}")
    out_txt.append(md)
open("docs/accuracy/extract/pdf2md/accc-v1.11-paged.md","w").write("".join(out_md))
open("docs/accuracy/extract/pdf2md/accc-v1.11-notags.txt","w").write("\n\f".join(out_txt))
print("pages:", len(res.pages))
EOF

# Independent second opinion (plain layout-preserving text)
pdftotext -layout docs/references/ACCC1.11-EN.pdf docs/accuracy/extract/pdftotext/accc-v1.11.txt

# Visual tier: render diagram/table-heavy pages at 200 dpi (1-indexed page numbers below)
.venv/bin/python - <<'EOF'
import pymupdf, os
d = pymupdf.open("docs/references/ACCC1.11-EN.pdf")
PAGES = [12, 34, 74, 75, 76, 78, 79, 81, 84, 86, 87, 88, 89, 90, 92, 93, 95, 96, 98,
         103, 104, 105, 106, 107, 108, 122, 123, 124, 127, 128, 129, 130, 133, 135, 136,
         137, 139, 140, 144, 146, 149, 150, 152, 157, 160, 166, 167, 177, 183, 185, 190,
         193, 196, 197, 198, 199, 205, 206, 207, 210, 211, 212, 219, 220, 221, 222, 223,
         224, 225, 242, 245, 246, 247, 248, 292, 293]
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
- The v1.11 first-pass reports classify both editions as `native_partial`, text-based, and
  complex-layout. English fallback pages are 39-40, 180-181, and 280; the corresponding
  French pages are 40-41, 182-183, and 280. Render these before comparing their contents.
- French and English pagination diverges because translated prose reflows. Align by section
  number and then record both page anchors; never assume equal PDF page numbers.

## Produced files

| Path | Contents |
|---|---|
| `pdf2md/accc-v1.11-paged.md` | English full document, page-anchored Markdown (~607 kB) |
| `pdf2md/accc-v1.11-notags.txt` | Same English content without markers, form-feed separated |
| `pdftotext/accc-v1.11.txt` | English poppler `-layout` text (~922 kB) |
| `pages/pNNN.png` | 200 dpi renders of the flagged English pages listed above |
| `inspector-v1.11-en/` | English first-pass report and full Markdown |
| `inspector-v1.11-fr/` | French first-pass report and full Markdown |

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
