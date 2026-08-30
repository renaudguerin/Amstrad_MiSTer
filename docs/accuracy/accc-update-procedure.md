# ACCC reference update and bilingual audit runbook

This is the repeatable procedure for adopting a new edition of *The Amstrad CPC CRTC
Compendium* (ACCC), checking the French authority against its English translation, and
cascading consequential findings through documentation, tests, RTL, and hardware work.

Technical information is sourced from *The Amstrad CPC CRTC Compendium* by Longshot
(CC BY-NC-ND 4.0).

## Authority and scope

Real hardware and the Logon System reference photographs remain the highest authority. The
latest French ACCC edition is the primary documentary oracle. The matching English edition
is a working translation and accessibility aid. When they differ, record both readings and
use the French reading unless hardware or an author clarification supersedes it.

The full French/English sweep is a one-time baseline for v1.11. For a later release:

1. compare the new French edition with the previous French authority;
2. inspect the English translation in changed technical sections;
3. run a small structural visual sample to detect changed diagrams; and
4. cascade only accepted consequential changes.

Do not build exhaustive comparison matrices for superseded language editions unless a
current ambiguity requires historical evidence.

Use ACCC section numbers as durable keys. New or materially revised rule claims cite the
French page and may add the English page. Preserve historical provenance: do not bulk-rewrite
old reports, quotations, or unaffected code comments solely to replace English page numbers.

## 1. Source and workspace verification

Place both current PDFs under `docs/references/` and verify that `.gitignore` excludes
`docs/**/ACCC*.pdf` and `docs/accuracy/extract/`. Never commit source PDFs or bulk extracts.

Record page counts, edition dates, and hashes:

```sh
shasum -a 256 docs/references/ACCC*.pdf
pdfinfo docs/references/ACCC1.11-FR.pdf
pdfinfo docs/references/ACCC1.11-EN.pdf
git check-ignore -v docs/references/ACCC1.11-FR.pdf
git check-ignore -v docs/accuracy/extract/example.txt
```

The tracked extraction manifest is `docs/accuracy/extract/README.md`; generated siblings in
that directory remain ignored.

## 2. First-pass extraction

Use the `pdf-inspector` skill first for each PDF. It classifies the document, produces
position-aware Markdown, and lists fallback pages. Use a fresh ignored output directory per
edition:

```sh
env UV_CACHE_DIR=/tmp/accc-uv-cache uv run \
  /Users/renaudg/.claude/skills/pdf-inspector/scripts/inspect_pdf.py \
  docs/references/ACCC1.11-FR.pdf \
  --output-dir docs/accuracy/extract/inspector-v1.11-fr
env UV_CACHE_DIR=/tmp/accc-uv-cache uv run \
  /Users/renaudg/.claude/skills/pdf-inspector/scripts/inspect_pdf.py \
  docs/references/ACCC1.11-EN.pdf \
  --output-dir docs/accuracy/extract/inspector-v1.11-en
```

Read each inspection report before its Markdown. Preserve usable native extraction, but
render every reported fallback page. `pdftotext -layout` is an optional second opinion; it
does not outrank position-aware Markdown or the rendered page.

## 3. Same-language edition diff

For a new French release, normalized text and word-stream diffs against the previous French
edition identify changed pages cheaply. Strip only known version footers and page stamps.
Do not normalize technical punctuation, comparison operators, subscripts, units, or numeric
formatting: those may be the change.

Use the section number, not the PDF page, to align changed material after translation reflow.
Classify every candidate as technical text, table/diagram, metadata, formatting-only, or
unresolved.

## 4. French/English semantic comparison

A direct cross-language word or pixel diff is not useful: translated prose and pagination
reflow make nearly every page different. Build a section map from both tables of contents and
compare bounded section packets instead.

Each packet returns only candidate records and a coverage result. A candidate contains:

- stable finding ID;
- section plus French and English page anchors;
- concise paraphrase of each edition;
- difference class and narrow technical consequence;
- visual verification status;
- affected documentation, test, RTL, or hardware area;
- confidence, reviewer, adjudication state, and disposition.

Consequential candidates include changes in conditions, temporal quantifiers, old/new value
sampling, event ordering, inequalities, values, units, bit numbers, exceptions, CRTC type
scope, counter rules, readback, synchronization, display, pointers, interrupts, tables,
pseudocode, or chronograms. Stylistic translation differences do not enter the finding ledger.

Reader agents perform candidate discovery only. They do not edit or delegate. The parent
adjudicates every candidate against the original PDFs and keeps source text, deduction,
current RTL/tests, and hardware evidence separate. Model agreement is not source evidence.

## 5. Visual calibration and routing

Before rendering hundreds of pages, determine whether non-text content is reused:

1. compare hashes and occurrence counts of embedded raster images across both PDFs;
2. compare vector drawing fingerprints normalized to ignore page translation;
3. sample prose, counters, sync diagrams, interlace tables/chronograms, display registers,
   and diagnostic chapters; and
4. record section-aligned page pairs because French and English page numbers may differ.

If embedded assets and normalized drawings match in the representative sample, limit full
visual work to:

- unmatched image or drawing fingerprints;
- tables/chronograms whose labels contain translated technical text;
- `pdf-inspector` fallback pages;
- semantic candidates where layout changes meaning; and
- pages whose extraction layers disagree.

Asset equality does not prove label equality or semantic equivalence. Inspect rendered pages
when labels, callouts, legends, table headers, or spatial ordering carry meaning.

## 6. Adjudication and author feedback

Use these verdicts:

- English omission or contradiction;
- French clarification;
- value, unit, type, or condition mismatch;
- table or diagram mismatch;
- source-internal ambiguity;
- no consequential difference; or
- unresolved extraction/visual question.

High-impact findings receive targeted independent review. Ambiguity becomes a numbered author
question or a hardware discriminator, never an invented rule. Add remaining translation
differences to `docs/accuracy/accc-author-feedback.md` as courteous, narrowly worded notes.
Do not imply that a documentation correction proves current RTL or hardware.

## 7. Documentation cascade

Freeze and review the curated differences report before changing downstream claims. Then
update, in order:

1. author feedback and the current-edition differences report;
2. `compendium-01-counters.md`, `compendium-02-sync.md`, and
   `compendium-03-display-regs.md`;
3. `audit-findings.md`, testbench specifications, and SHAKER mappings;
4. finding-specific review or implementation notes; and
5. current status, roadmap, reference inventory, and authority instructions.

Keep documentation-only milestones separate from tests and RTL. Documentation changes do not
require simulation, but run `git diff --check`, verify local links and cited sections, and
search for stale contradictory claims.

## 8. RTL, test, and hardware impact

For each accepted behavioral finding:

1. read the French rule, the English translation, and the current RTL before writing a test;
2. derive the expected result on paper and cite the ACCC section beside the assertion;
3. add a focused failing vector where the documented rule predicts a mismatch;
4. keep classic and Plus work in their separate streams and commits;
5. run `make -C sim`, `make -C sim lint`, and soak where applicable;
6. stop and explain any behavior-preserving commit that changes the soak hash; and
7. obtain fresh independent review for non-trivial code.

A passing model test proves only the selected model. Hardware-blocked conclusions stay open
with a focused discriminator and named evidence requirement.
