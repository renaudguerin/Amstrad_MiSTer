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
- [ ] Apply accepted fixes to affected docs.
- [ ] Write code-implication list (RTL/tests), no RTL changes yet.

### P3 — status-vs-code audit
- [ ] Run `make -C sim` and `make -C sim lint`; capture summary line.
- [ ] Reconcile current-status.md / implementation-roadmap.md / shaker-module-a-map.md /
      testbench-spec.md claims vs code. Known stale already: shaker-map "no R12/R13 vectors"
      (t20a–t20h exist since `90aed07`); testbench-spec xfail statement (none remain);
      roadmap "46 required passes" (now 85).
- [ ] Add SHAKER-not-CI clarification to roadmap Gate C, current-status, shaker-map.

### P4 — review debt (order per review-debt.md)
- [ ] `de71808` F4 equality-only rollover (+vectors) — widest reach.
- [ ] `da79915` last-line arbitration completion, read with reviewed `1a1233f` as one whole.
- [x] `cd47d7d` CPR parser as untrusted-input review.
- [x] `c4c3e0f` F4 vectors vs ACCC digests (gate-integrity risk).
- [x] `90aed07` t20 R12/R13 vectors incl. the t20g C0=0 concern.
- [x] F8 commit (`c9f4a4e`) incl. VSYNC comparator change + untested R4-rewrite corner.
- [ ] Update review-debt.md rows with reviewer/date; problems → action items.

### P5/P6 — implementation iterations (separate stream branches)
- [ ] Common-dependency analysis; land shared items on base branch.
- [ ] Classic queue: F9 closure (t12 vectors) → F6 decision gate → F7 RFD → F10 fixtures+impl.
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
the resume point in one line here: **resume point: P1 complete (`436d6b1`); user ACCEPTED B1-B13 and is relaying Q1-Q14. Next: P2, then P3, P4 (user review of findings no longer blocks). Previously blocked on user
review of findings-review.md + acccc-author-questions.md before P2. P3/P4 can start in
parallel if the user prefers.**
