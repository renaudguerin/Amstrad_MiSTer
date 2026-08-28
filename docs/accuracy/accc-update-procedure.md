# ACCC Reference Update Procedure & Differential Audit Runbook

This document describes the standard, repeatable procedure for updating *The Amstrad CPC CRTC Compendium* (ACCC) PDF reference in this repository, conducting mechanical and multimodal differential analysis, updating project documentation, and tracking hardware accuracy impacts.

Technical information sourced from *The Amstrad CPC CRTC Compendium* by Longshot (CC BY-NC-ND 4.0).

---

## 1. Prerequisites and Setup

1. **Environment**:
   Ensure python environment has `pdf-inspector` and `pymupdf` installed:
   ```sh
   uv venv --clear .venv
   uv pip install --python .venv/bin/python pdf-inspector pymupdf
   ```
2. **File Location**:
   Place the new PDF version under `docs/references/` (e.g. `docs/references/ACCC1.11-EN.pdf`).
   Verify that `.gitignore` ignores `docs/**/ACCC*.pdf` (never commit the reference PDFs).

---

## 2. Step 1: Verification & Hash Recording

Compute and record SHA-256 hashes of both old and new PDF files:
```sh
shasum -a 256 docs/references/ACCC*.pdf
```

---

## 3. Step 2: Mechanical Text & Layout Differential Scan

Run a normalized word-stream and Markdown diff across all pages to detect all pages with text differences:

```python
import pymupdf, re, difflib, pdf_inspector as pi

old_pdf = "docs/references/ACCC1.10-EN.pdf"
new_pdf = "docs/references/ACCC1.11-EN.pdf"

d_old = pymupdf.open(old_pdf)
d_new = pymupdf.open(new_pdf)

print(f"Page counts: Old={len(d_old)}, New={len(d_new)}")

# 1. Word stream comparison (excluding footer version/date/page stamps)
footer_words = {"–", "Page", "295"}
changed_word_pages = []

for p in range(len(d_new)):
    w_old = [w[4] for w in d_old[p].get_text("words") 
             if not (w[4].startswith("V1.") or w[4] in footer_words or (w[4].isdigit() and int(w[4]) == p + 1))]
    w_new = [w[4] for w in d_new[p].get_text("words") 
             if not (w[4].startswith("V1.") or w[4] in footer_words or (w[4].isdigit() and int(w[4]) == p + 1))]
    if w_old != w_new:
        changed_word_pages.append((p + 1, len(w_old), len(w_new)))

print(f"Pages with substantive word changes ({len(changed_word_pages)}):")
for p, o_len, n_len in changed_word_pages:
    print(f"  Page {p}: {o_len} -> {n_len} words")
```

---

## 4. Step 3: Multimodal & Diagram Verification

To detect whether diagrams, chronograms, or non-text vectors have changed:
1. Render each page at 150 DPI with header/footer cropped.
2. Compare drawing streams and pixel masks.
3. For pages with significant pixel deltas, render side-by-side PNGs to `docs/accuracy/extract/pages/` for visual verification.

```python
import pymupdf

d_old = pymupdf.open(old_pdf)
d_new = pymupdf.open(new_pdf)

# Render diffs for flagged or changed pages
for p in [p for p, _, _ in changed_word_pages]:
    page_old = d_old[p - 1]
    page_new = d_new[p - 1]
    page_new.get_pixmap(dpi=200).save(f"docs/accuracy/extract/pages/p{p:03d}.png")
```

---

## 5. Step 4: Extraction Manifest Update

Regenerate the untracked extraction files under `docs/accuracy/extract/`:
- `pdf2md/accc-vX.XX-paged.md`
- `pdftotext/accc-vX.XX.txt`
- `pages/pNNN.png`

Update `docs/accuracy/extract/README.md` with new source paths and SHA-256 hashes.

---

## 6. Step 5: Documentation & Digest Sweep

1. **Author Feedback / Questions**: Update `docs/accuracy/accc-author-feedback.md` marking resolved questions, confirmed errata, and outstanding items.
2. **Version Differences Report**: Create `docs/accuracy/accc-X.XX-differences.md` recording all itemized changes.
3. **Accuracy Digests**: Update `compendium-01-counters.md`, `compendium-02-sync.md`, `compendium-03-display-regs.md`, and `audit-findings.md` with new chapter text and citations.
4. **Authority Rules**: Update `AGENTS.md` and `CLAUDE.md` to reference the new edition.

---

## 7. Step 6: RTL & Test Impact Assessment

1. Inspect RTL code (`rtl/CRTC.v`, `rtl/crtc_type0_engine.v`, `rtl/crtc_type1_engine.v`) for sections affected by modified or clarified rules.
2. Create dedicated tracking documents (e.g. `docs/accuracy/fXX-...-todos.md`) for any identified findings.
3. Run testbench suite:
   ```sh
   make -C sim
   make -C sim lint
   make -C sim soak
   ```
4. Perform independent cross-provider review.
