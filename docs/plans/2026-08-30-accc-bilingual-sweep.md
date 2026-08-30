# ACCC v1.11 French/English consequential-difference sweep

This is the durable execution plan and resume point for the one-time v1.11 bilingual
baseline. It compares the latest French and English editions for differences that can affect
the repository, updates project documentation, and then maps accepted findings to tests,
RTL, or hardware discriminators. It does not build an exhaustive history of superseded
language editions.

Technical information is sourced from *The Amstrad CPC CRTC Compendium* by Longshot
(CC BY-NC-ND 4.0).

## Scope and source register

- Primary source: `docs/references/ACCC1.11-FR.pdf`, 295 pages,
  SHA-256 `4409e3a2e77cd54e499c6956446b01bce93f79a1c1ba366201d514cf6e3c0d47`.
- Working translation: `docs/references/ACCC1.11-EN.pdf`, 295 pages,
  SHA-256 `3e45eb7eea7dc8f0d7211f78bec4f8d00530ce3c00da2e76034fb24f7a751868`.
- English v1.10 is consulted only when it helps explain a current v1.11 difference or author
  correction. French v1.10 is not required for this sweep.
- Source PDFs and bulk extracts remain untracked. Commit only compact manifests, paraphrased
  findings, short attributed excerpts where necessary, and downstream project corrections.

## Durable authority and citation policy

Real hardware remains the highest authority. For documentary interpretation, the latest
French ACCC edition is primary and the matching English edition is a working translation and
reader aid. A language disagreement is recorded explicitly; it is not resolved by model
voting. French controls unless hardware or an author clarification supersedes it.

Use the section number as the stable key. New or materially revised rule claims cite the
French page and may add the English page. Existing English-only citations and historical
reports are not rewritten wholesale. Migrate them when a bilingual finding affects the claim
or the surrounding material is substantively revised. Code comments should prefer section
anchors and concise paraphrase over edition-specific quotations.

For a future ACCC release:

1. Diff the new French edition against the previous French authority.
2. Review the matching English translation only in changed technical sections, plus a small
   structural visual sample.
3. Cascade accepted rule changes through the same finding ledger and verification gates.

This v1.11 sweep is therefore the expensive one-time bilingual baseline, not a promise to
repeat a full 295-page cross-language comparison for every release.

## Evidence separation

Every consequential record keeps these claims separate:

1. French source text or rendered figure.
2. English source text or rendered figure.
3. Translation difference and narrow semantic consequence.
4. Deduction needed to apply the rule.
5. Current RTL and test-model behavior.
6. Hardware or SHAKER evidence.

A passing model test is not hardware confirmation. Source ambiguity becomes an author
question or hardware discriminator, not an assumed RTL oracle.

## Finding record

Each finding receives a stable `BL-nnn` identifier and records:

- ACCC section and both page anchors;
- classification: English omission, contradiction, French clarification, value/unit/type
  mismatch, table/diagram mismatch, source ambiguity, or no consequential difference;
- concise paraphrase of each edition;
- repository consequence and exact affected files;
- visual evidence status;
- confidence, reviewer, and adjudication status;
- disposition: docs only, test expectation, RTL candidate, author courtesy note, hardware
  discriminator, or no action.

Coverage is section-complete: every technical section must be marked reviewed, deferred with
a reason, or represented by a finding. The main thread owns adjudication and integration.
Reader agents return candidate records only and do not edit or delegate.

## Work packages and gates

### P0 — sources, extraction, and visual calibration

- [x] Verify both v1.11 hashes and 295-page counts.
- [x] Confirm `.gitignore` excludes ACCC PDFs and all generated extraction artifacts.
- [x] Generate fresh `pdf-inspector` reports for both editions under ignored extraction
      directories.
- [x] Compare embedded images and normalized vector drawings across both PDFs. All 18
      embedded EN/FR image streams are byte-identical; representative vector signatures
      also match apart from a minor video-pointer line-segmentation difference.
- [x] Visually inspect a representative sample spanning counters, sync, interlace,
      chronograms, display registers, and diagnostic chapters.
- [x] Decide from evidence whether visual review can be limited to unmatched drawings,
      translated labels, extraction fallback pages, and semantic candidates.

### P1 — bilingual calibration

- [x] Check the known §19.5.2 qualifier difference without giving the reader its answer.
- [x] Check §12.2 as a negative control. It was not clean: the French source exposed the
      consequential BL-002 write-window omission in English.
- [ ] Fix the output schema or packet size if the calibration invents differences, loses
      qualifiers, or confuses deduction with source text.

### P2 — full consequential sweep

- [ ] Build a section-to-page map for both editions.
- [ ] Review all technical sections in bounded packets.
- [ ] Maintain the section coverage ledger and deduplicate candidates by section/rule.
- [ ] Render any table, chronogram, diagram, or extraction disagreement before adjudication.

### P3 — adjudication and author courtesy report

- [ ] Adjudicate each candidate against the original pages.
- [ ] Obtain targeted independent review for high-impact findings.
- [ ] Write the curated French/English v1.11 difference report.
- [ ] Add unresolved English translation differences and source ambiguities to the author
      feedback document as courtesy notes, without overstating implementation or hardware
      consequences.

### P4 — documentation cascade

- [ ] Update affected compendium digests and `audit-findings.md`.
- [ ] Update testbench and SHAKER mappings only where a finding changes their rule premise.
- [ ] Reconcile current status, roadmap, references, and authority prose.
- [ ] Preserve historical provenance instead of bulk-replacing old English citations.

### P5 — implementation map

- [ ] For each accepted behavioral finding, read the cited source and current RTL before
      proposing a vector.
- [ ] Record paper-derived expected behavior and the owning classic or Plus stream.
- [ ] Keep classic and Plus work in separate commits and stream branches.
- [ ] Add a focused failing vector before RTL where a documented mismatch is predicted.
- [ ] Run `make -C sim`, lint, and soak as applicable; explain any golden-hash change.
- [ ] Keep model-only and hardware-blocked conclusions explicitly open.

No merge, push, synthesis dispatch, or stream finish is authorized by this plan.
