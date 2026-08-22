# Session plan — ACCC v1.10 faithfulness review, doc corrections, review-debt repayment, then implementation

Created 2026-08-22 on branch `accc-review-and-fixes` (cut from
`codex/exploratory-gx4000-plus-plan`). This file is the durable session plan: if a session is
lost, resume from the checklists below. The user runs this project across multiple harnesses;
nothing here depends on conversation memory.

## Mission

1. Review Fable's distilled ACCC documentation (digests + F1–F12) for faithfulness against the
   original `docs/ACCC1.10-EN.pdf`; collect ambiguities into questions for the author (Longshot).
2. After user review: fix downstream docs; flag code implications.
3. Reconcile status docs with what is actually implemented.
4. Repay the independent-review debt in `docs/review-debt.md`.
5. Iterate both work streams (classic accuracy / Plus) implementing action items, one commit per item.

## Locked decisions (agreed with Renaud, 2026-08-22)

- **Branch topology**: all review/correction work lands on `accc-review-and-fixes`. Stream
  branches (`accuracy/*`, `plus/*`) cut from it only after common dependencies land there.
- **Review authority**: this model's review counts as the independent review that clears
  existing `review-debt.md` rows (other providers are excluded from this harness). Rows are
  cleared with reviewer name + date.
- **New work on this branch**: no per-commit review-debt rows. The whole branch is treated as
  implemented by this model and will be reviewed as one diff later. This supersedes the
  standing rule in AGENTS.md for work committed to this branch only.
- **SHAKER is not part of the build/test loop**: hardware sessions are manual, user-run, and
  only happen at significant milestones. Docs must not imply otherwise. Every milestone gets a
  suggested Shaker target list drawn from `accuracy/shaker/shaker-accc-crossref.md` (whose
  citations themselves need PDF confirmation before turning into RTL/vectors).
- **ACCC v1.9 is disregarded**: historical. `accc-1.10-differences.md` is subagent output kept
  as context; only its v1.10-side page anchors get verified. No v1.9 comparison work.
- **Source of truth**: local `docs/ACCC1.10-EN.pdf`, SHA-256
  `1bd6f0e3a06022d03fd40b51d4d622afef2675954a483780f0922cdf1e33a560` (verified against the hash
  recorded in `accc-1.10-differences.md`). 295 PDF pages.
- **Extraction persistence**: raw extractions stay untracked under `docs/accuracy/extract/`
  (gitignored; committing bulk text would effectively republish a CC BY-NC-ND book). A
  committed manifest documents exact regeneration. Curated per-page transcriptions of flagged
  pages may be committed inside review docs under attribution, like the existing digests.
- **Tooling**: poppler (`pdftotext -layout`) + uv-managed venv with `pdf-inspector`
  (primary Markdown/table extractor) and `pymupdf` (renders pages to PNG). Multimodal reading
  of rendered PNGs is the arbiter for tables/chronograms; "clean prose" requires both text
  extractors agreeing.
- **Subagents**: available and same-model only. Use for mechanical sweeps; keep rule-judgment
  verification in the main thread.

## Verification method (Phase 1)

Per digest section / finding:
1. Locate cited ACCC page(s) in extracted text.
2. Compare every rule claim. Verdicts: confirmed / inaccurate / omission /
   unverifiable-from-text.
3. Table- or diagram-dependent rules → render page(s) to PNG, read visually before judging.
4. Ambiguity in the source itself → numbered entry in `docs/accuracy/accc-author-questions.md`
   (page cite + why ambiguous). Never guess.
5. Record verdict + evidence per finding in `docs/accuracy/findings-review.md`.

Deliverables of Phase 1 (stop point — user reviews before Phase 2):
- `docs/accuracy/findings-review.md`
- `docs/accuracy/accc-author-questions.md`

## Phase checklists

### P0 — setup
- [x] Branch `accc-review-and-fixes` created off `codex/exploratory-gx4000-plus-plan`.
- [x] This plan committed.
- [x] `.venv` (uv) with pdf-inspector + pymupdf; gitignore covers `.venv/`, `docs/accuracy/extract/`.
- [x] Extraction manifest committed (`docs/accuracy/extract/README.md`); raw extraction regenerated locally.
- [x] Multimodal smoke test passed (read SHAKER screenshot).

### P1 — faithfulness review
- [x] Extract + skim ch. 2 (attribution/licence), 3–5 (context), 6–29 scope map.
- [x] Digest-01 (ch. 6–13) verified.
- [x] Digest-02 (ch. 14–16, 27) verified.
- [x] Digest-03 (ch. 17–22, 28–29) verified.
- [x] F1–F12 cross-checked against verified rules (not just digests).
- [x] testbench-spec / shaker-map citations spot-checked.
- [x] findings-review.md + accc-author-questions.md written and committed (`436d6b1`).

### P2 — corrections (ACCEPTED by user 2026-08-22: all of B1–B13; author questions being relayed)
- [x] Apply accepted fixes to affected docs. (`fa44862`)
- [x] Write code-implication list (RTL/tests), no RTL changes yet. (Part C pointers into
      audit-findings F7/F8/F9/F10 + testbench-spec planned additions, same commit.)

### P3 — status-vs-code audit
- [x] Run `make -C sim` and `make -C sim lint`; capture summary line.
      (85 passed, 0 xfailed, 0 xpassed, 0 failed; Verilator 5.050.)
- [x] Reconcile current-status.md / implementation-roadmap.md / shaker-module-a-map.md /
      testbench-spec.md claims vs code. Fixed: shaker-map "no R12/R13 vectors"
      (t20a-t20h exist since `90aed07`) + drifted RTL line refs; testbench-spec xfail
      statements (none remain) and definition-of-done; roadmap "46 required passes" (85)
      and stale F4-next baseline.
- [x] Add SHAKER-not-CI clarification to roadmap Gate C, current-status, shaker-map.
      (`2506687`)

### P4 — review debt (order per review-debt.md)
Note: the four `[x]` ticks this checklist carried before P4 ran were stale (present since
the plan was created, with no cleared rows in review-debt.md); all six commits were
reviewed fresh on 2026-08-22.
- [x] `de71808` F4 equality-only rollover (+vectors) — widest reach. Clear.
- [x] `da79915` last-line arbitration completion, read with reviewed `1a1233f` as one whole. Clear.
- [x] `cd47d7d` CPR parser as untrusted-input review. Clear; observations A5.
- [x] `c4c3e0f` F4 vectors vs ACCC digests (gate-integrity risk). Clear.
- [x] `90aed07` t20 R12/R13 vectors incl. the t20g C0=0 concern. Clear; concern resolved
      (cold-reset event semantics), companion-vector suggestion A3.
- [x] F8 commit (`c9f4a4e`) incl. VSYNC comparator change + untested R4-rewrite corner.
      Clear; corner finding A1 (spurious VSYNC if R7==R4+R5+1 on the adjustment-ending
      line), caveat pair A2.
- [x] Update review-debt.md rows with reviewer/date; problems → action items A1-A5.
      (Commit with the plan update.)

### P5/P6 — implementation iterations (separate stream branches)
- [x] Common-dependency analysis; land shared items on base branch.
      (2026-08-22 conclusion: the only cross-stream shared surface is `sim/sim_main.cpp`
      harness helpers — classic vectors and Plus benches both extend it, so harness-only
      changes land on `accc-review-and-fixes` first. Classic queue touches only
      `rtl/UM6845R.v`; Plus P0 touches top-level wiring + `files.qip`; no shared RTL items.
      Review action item A4 (harness tidy) landed on base for this reason. Nothing else
      blocks cutting `accuracy/*` and `plus/*`.)
- [ ] Classic queue: F9 closure done (`t12a`/`t12b` on `accuracy/f9-t12-closure`, `aea80b5`,
      87 required passes) → next: F6 decision gate — options analysis + corrected GA
      interface evidence recorded in `accuracy/f6-decision-gate.md`; user leans option C
      (full fidelity, staged, no GA netlist change expected). Longshot's per-CRTC-separation
      advice + status recorded in `accuracy/crtc-per-type-separation.md` (recommendation:
      split before F7/F10 RTL, option S2 there) → F7 RFD (incl. B6 disarm path; A1 VSYNC
      corner fix first if accepted) → F10 fixtures+impl (gated on B10/B11 + Q10-Q12 answers).
- [ ] Plus queue: P0 parser→service/MMU/boot wiring → P1 CRTC3 foundation.
- [ ] Milestone→Shaker suggestions recorded per milestone (e.g. F9→Module E "(3)";
      F8 build `4c78603`→Module A adjustment entries + Module E "(2)"; F7→RFD entries;
      F10→interlace suite 22C/3; Plus P1/P5→Shaker on CRTC3 setting).
- [ ] Discipline: vector-first behaviour changes; XFAIL removal in fix commit; attribution
      lines; files.qip same-commit; CI synthesis for top-level/files.qip changes; update
      current-status.md at hardware-testable checkpoints.

## Tooling notes

```sh
# Regenerate extractions (untracked output dir)
uv venv .venv 2>/dev/null; .venv/bin/uv pip install pdf-inspector pymupdf
mkdir -p docs/accuracy/extract/pdf2md docs/accuracy/extract/pdftotext
.venv/bin/pdf2md docs/ACCC1.10-EN.pdf --pages --json \
  > docs/accuracy/extract/pdf2md/accc-v1.10.json     # primary: tables + page anchors
pdftotext -layout docs/ACCC1.10-EN.pdf docs/accuracy/extract/pdftotext/accc-v1.10.txt
# Render specific pages to PNG for visual reading:
.venv/bin/python -c 'import fitz; d=fitz.open("docs/ACCC1.10-EN.pdf"); p=d[74];
p.get_pixmap(dpi=200).save("docs/accuracy/extract/pages/p075.png")'   # 0-indexed
```

Verilator 5.050 installed locally; `make -C sim` is the gate for every commit.

## Handoff convention

At end of any phase (or if the session must end), tick the checklist above, commit, and note
the resume point in one line here: **resume point: P2-P4 complete on `accc-review-and-fixes`
(fa44862, 2506687, bf5c1fb, 1b4eb0b); common-dependency check done (only shared surface =
sim harness; A4 landed there). P5 started: F9 t12 closure committed on stream branch
`accuracy/f9-t12-closure` (aea80b5, 87 required passes, no RTL change needed). Next: user
decision on the F6 gate; then F7 RFD design (fold in B6 disarm path + A1 VSYNC corner +
A2 caveat vectors) on a fresh accuracy branch; Plus P0 wiring on a plus/* branch. Review
action items A1-A5 live in docs/review-debt.md. Author answers to Q1-Q14 may refine docs but
block nothing except F10 fixtures.**
