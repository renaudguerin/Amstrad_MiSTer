# ACCC v1.11 extraction manifest

Local, regenerable extraction of the project's accuracy reference for verification work
(Phase 1 of `docs/plans/2026-08-22-accc-review-plan.md`).

The v1.11 extraction snapshot listed below is intentionally versioned so a fresh session can
reproduce and audit the bilingual findings without regenerating the text and render layers.
The source PDFs remain local and ignored. Other regenerated or ad-hoc intermediate files in
this directory remain ignored unless they are deliberately added to the snapshot.

## Source

The only edition in use is the v1.11 re-issue that Longshot published on 2026-09-11 with our
round-1/round-2 feedback applied (same version number; formerly labelled "v1.11b"). Master
copies live in the main checkout's ignored `local/accc/`; each checkout works from ignored
copies in `docs/specs/`.

- `docs/specs/ACCC1.11-EN.pdf`, 296 PDF pages,
  SHA-256 `69d6a6a77de472937d41778ad48fc4fb427a937a24d3054f6d42c0b6ccfcc3e9`
- `docs/specs/ACCC1.11-FR.pdf`, 295 PDF pages,
  SHA-256 `28f25c73c1797578522f34ce9ff558210386972c9257b5b8081927483ee02c3b`

Verify before regenerating:

```sh
shasum -a 256 docs/specs/ACCC1.11-EN.pdf docs/specs/ACCC1.11-FR.pdf
```

## Regeneration

```sh
uv venv .venv && uv pip install --python .venv/bin/python pdf-inspector pymupdf
X=docs/classic/extract

# First-pass classifier/extractor for both editions. Its reports decide which pages need
# rendered fallback; keep a writable uv cache outside the repository if needed.
for L in EN FR; do
  env UV_CACHE_DIR=/tmp/accc-uv-cache uv run \
    /Users/renaudg/.claude/skills/pdf-inspector/scripts/inspect_pdf.py \
    docs/specs/ACCC1.11-$L.pdf --output-dir $X/inspector-v1.11-$(echo $L | tr A-Z a-z)
done

# Primary English text layer: position-aware Markdown, one <!-- ======== PAGE N ======== -->
# marker per page
.venv/bin/python - <<'PY'
import pdf_inspector as pi
res = pi.extract_pages_markdown("docs/specs/ACCC1.11-EN.pdf")
out_md, out_txt = [], []
for i, p in enumerate(res.pages, 1):
    md = p.markdown or ""
    out_md.append(f"\n\n<!-- ======== PAGE {i} ======== -->\n\n{md}")
    out_txt.append(md)
open("docs/classic/extract/pdf2md/accc-v1.11-paged.md","w").write("".join(out_md))
open("docs/classic/extract/pdf2md/accc-v1.11-notags.txt","w").write("\n\f".join(out_txt))
PY

# Independent second opinion (plain layout-preserving text)
pdftotext -layout docs/specs/ACCC1.11-EN.pdf $X/pdftotext/accc-v1.11.txt

# Visual tier: render diagram/table-heavy pages at 200 dpi (1-indexed PDF page numbers)
.venv/bin/python - <<'PY'
import pymupdf
X = "docs/classic/extract/pages"
EN = [12, 24, 35, 75, 76, 77, 79, 80, 82, 85, 87, 88, 89, 90, 91, 93, 94, 96, 97, 99, 104,
      105, 106, 107, 108, 109, 123, 124, 125, 128, 129, 130, 131, 134, 136, 137, 138, 140,
      141, 144, 145, 147, 150, 151, 153, 158, 161, 167, 168, 178, 184, 186, 191, 194, 197,
      198, 199, 200, 206, 207, 208, 211, 212, 213, 220, 221, 222, 223, 224, 225, 226, 243,
      246, 247, 248, 249, 283, 293, 294]
FR = [47, 73, 171, 282]
d = pymupdf.open("docs/specs/ACCC1.11-EN.pdf")
for n in EN: d[n-1].get_pixmap(dpi=200).save(f"{X}/p{n:03d}.png")
d = pymupdf.open("docs/specs/ACCC1.11-FR.pdf")
for n in FR: d[n-1].get_pixmap(dpi=200).save(f"{X}/fr_p{n:03d}.png")
PY
```

`extract/` is gitignored; add regenerated snapshot files with `git add -f`.

## Verification protocol

- pdf-inspector Markdown (`pdf2md/`) is the primary text layer and outranks pdftotext where
  they disagree (2026-08-24 decision: pdftotext `-layout` is the weaker extractor; keep it
  only as an optional second opinion when the Markdown itself looks ambiguous).
- Table/chronogram rules are judged from the rendered PNGs (multimodal), never from any
  text layer alone. The digests' ⚠ VERIFY flags mark the rendered pages; extend `EN`/`FR`
  when review surfaces more.
- Known extractor behaviour: pdf-inspector resolves Fable's "column smearing" complaints on
  prose pages (e.g. p.76 reads cleanly); genuine chronograms/pixel grids remain unusable in
  both text layers and require the visual tier.
- Logged extraction gap (2026-08-24): pdf2md dropped the entire first paragraph of §11.6.4
  (it is present in pdftotext and in the render). Do not treat a missing paragraph in
  the Markdown layer as proof the PDF lacks it.
- The first-pass reports classify both editions as `native_partial`, text-based, and
  complex-layout. English fallback pages are 40-41, 181-182, and 281; the corresponding
  French pages are 40-41, 182-183, and 280. Render these before comparing their contents.
- French and English pagination diverges because translated prose reflows. Align by section
  number and then record both page anchors; never assume equal PDF page numbers.
- Page anchors written before the 2026-09-11 re-issue: French anchors are unchanged. English
  anchors from p.29 onward are one lower than the current English edition (the re-issue adds
  a page before p.29 and restores §14.9 on pp.144-145). The section number is the durable key.

## Produced files

| Path | Contents |
|---|---|
| `pdf2md/accc-v1.11-paged.md` | English full document, page-anchored Markdown |
| `pdf2md/accc-v1.11-notags.txt` | Same English content without markers, form-feed separated |
| `pdftotext/accc-v1.11.txt` | English poppler `-layout` text |
| `pages/pNNN.png` | 200 dpi renders of the flagged English pages listed above |
| `pages/fr_pNNN.png` | 200 dpi renders of French pages whose tables or chronograms changed in the re-issue |
| `inspector-v1.11-en/` | English first-pass report and full Markdown (296 pages) |
| `inspector-v1.11-fr/` | French first-pass report and full Markdown (295 pages) |

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
