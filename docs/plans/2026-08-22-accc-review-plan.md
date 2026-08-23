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

### P4 addendum — verification scope convention (learned 2026-08-22, F6 premise miss)

The F6 "DE consumed at 1µs" premise survived P1-P4 because each phase owned a different
layer: P0/P1 = docs vs ACCC book; P2 = corrections downstream (still book side); P3 =
status docs vs code on the plan's named items only; P4 = six commits vs rules. Nobody owned
**engineering-assumption sentences inside audit-findings' Current/fix prompts about how the
core integrates**. Same-layer inventory from the 2026-08-22 sweep:

- ✅ F6 DE-consumption premise — wrong, corrected (`accuracy/f6-decision-gate.md`);
  consequence: GA40010 samples DISPEN at byte phase, option C likely needs no netlist change.
- ❌ testbench-spec non-goal "no GA co-simulation / netlist-based / slow" — wrong;
  ga40010_test.v co-simulates `rtl/CRTC.v` plus both engines with GA40010 and renders PNG
  frames. Corrected in
  testbench-spec; enables in-simulation seam measurement for F6 Stage 2.
- ⚠ Stale: audit-findings RTL line references throughout (drifted through commits).
- ⚠ Stale: F11h "Current" text — predates t20b/crtc1_row0_reload per-line row-0 reload.
- ✅ Spot-checked still true: F11f light-pen reads 0.

Convention going forward: within audit-findings, **Rule sections are ACCC-verified (P1);
Current/fix-prompt integration assumptions are unverified until confirmed against code.**
Remedy for the remainder: fold a sweep of F7/F10/F11a-i code-side claims + line refs into
the type-split refactor milestone (it forces a full read anyway).

### P5/P6 — implementation iterations (separate stream branches)
- [x] Common-dependency analysis; land shared items on base branch.
      (2026-08-22 conclusion: the only cross-stream shared surface is `sim/sim_main.cpp`
      harness helpers — classic vectors and Plus benches both extend it, so harness-only
      changes land on `accc-review-and-fixes` first. Classic queue touches only
      `rtl/CRTC.v` plus the per-type engines; Plus P0 touches top-level wiring + `files.qip`;
      no shared RTL items.
      Review action item A4 (harness tidy) landed on base for this reason. Nothing else
      blocks cutting `accuracy/*` and `plus/*`.)
- [x] Type-split prerequisite: land the soak-diff harness on base BEFORE the split
      (deterministic-seed randomized register writes at random C0/tick phases, both types,
      rolling hash over pins + key internals per CLKEN; mint the golden hash from unsplit
      code). Ship it as its own `make -C sim soak` target so the default suite stays lean
      and removal stays trivial; expected permanent tooling (F7 needs bit-identity-when-
      unarmed proof, F10 stages benefit), not split-only scaffolding.
      DONE: `make -C sim soak` on `accc-review-and-fixes`. **Golden hash
      `0x5b5004ff70148443`**, minted from the unsplit core at this commit (seed
      `0xaccc5eed20260822`, 2,845,088 characters / CLKEN samples, ~6 s, verified identical
      across two runs). The split must reproduce it exactly. Note: `accuracy/f9-t12-closure`
      (aea80b5) was merged into base first (merge commit before the soak commit) so the
      split branch inherits the full 87-vector suite and the exact core state the hash was
      minted against; re-minting is required only if the seed, sampled field set/order, or
      event schedule changes.
- [x] Classic queue: F9 closure done (`t12a`/`t12b` on `accuracy/f9-t12-closure`, `aea80b5`,
      87 required passes) → next: type-split prerequisite (soak harness on base), then the
      split per `accuracy/crtc-per-type-separation.md` → F6 option C Stage 1 (user lean
      recorded in `accuracy/f6-decision-gate.md`) → F13 R1>R0 byte-phase hardware gate
      (BLOCKED-PENDING-HARDWARE-EVIDENCE; does not block independent work) → F7 RFD
      (incl. B6 disarm path; A1 VSYNC corner fix) → F10 fixtures+impl. F10 gating fallback:
      author answers Q10-Q12 may
      never arrive — if so, derive fixtures from the §19.5-§19.8 pseudocode (P1-verified)
      plus the SHAKER 22C/3 tables via the visual tier (render pp.210-212 PNGs), record
      each ambiguity as a resolved-by-default-reading note citing the page, and proceed;
      SHAKER hardware comparison remains the arbiter.
      TYPE SPLIT DONE (`accuracy/crtc-type-split`, 27efc2d, pushed, CI simulation +
      synthesis green): wrapper keeps ports/regfile/shared-counter sequencing; two engines
      hold all type rules + provably-private flops; singular shared state because live
      CRTC_TYPE round-trips are a pinned contract (t02j/t06d/t09f/t16l). Soak reproduces
      the golden hash; 87 vectors pass; lint clean; files.qip in-commit; audit-findings
      prose sweep folded in (line refs refreshed to split layout, implementation stamps on
      F1/F2/F3/F4/F8, F7 absence + F10 minimal-IVM re-verified, F11h Current rewritten).
      Dev-time catch worth remembering: the soak alone passed nothing until a lockstep
      differential harness vs the pre-split core isolated a holdoff-latch set-path bug the
      directed vectors missed — the differential harness was throwaway (opencode temp dir),
      since preserved reproducibly as tools/split-differential (branch
      docs/split-differential-evidence, c68459b).
      FOLLOW-UP (same branch, user-raised): wrapper renamed `rtl/UM6845R.v` → `rtl/CRTC.v`,
      module `CRTC` (63f4c01) — UM6845R is the type-1 part number and misdescribed a
      two-variant component. Mechanical; gates re-run green, golden hash unchanged.
      Reviewer logistics: whole-branch review pending per locked decision; guide written at
      `docs/accuracy/type-split-review-guide.md`; `docs/review-debt.md` now lists both
      branches as outstanding whole-diff reviews instead of per-commit rows.
      FOLLOW-UP 2026-08-23 (branch `accuracy/a3-f6-stage1`): A3 companion vector `t20i`
      (live-entry R0=0 VMA reload; behaviour-preserving, soak reproduced
      `0x5b5004ff70148443`) and **F6 option C Stage 1** per
      `accuracy/f6-decision-gate.md` (vectors t10a-t10e red first, then the type-0
      substituted border-start term in `crtc_type0_engine.v` injected ahead of the
      wrapper's SKEW-DISPTMG delay line). This INTENDED a behaviour change: golden hash
      re-minted to **`0x326ea81358e7d88f`** (same seed/sampling; delta is exactly the
      type-0 R1>R0 DE byte, protected by t10a-t10e). Suite 93 passed / lint clean.
- [x] A3 companion vector (`t20i`) — DONE 2026-08-23 on `accuracy/a3-f6-stage1`,
      rebased onto the post-review base (A3-point soak verified against the base-recorded
      hash) and merged into `accc-review-and-fixes`. Closes review-debt action item A3.
- [x] F6 option C Stage 1 — DONE 2026-08-23, same branch, merged. At landing, this was
      believed to match a char-aligned DE premise with GA byte-phase sampling; Stage 2b
      below refutes that premise and reclassifies Stage 1 as the full-character
      approximation. Golden hash re-minted to `0x326ea81358e7d88f` for the
      intended delta; recorded in plan/README/guide/AGENTS.md. Next stage: Stage 2
      seam-width measurement via the GA40010 co-simulation render.
- [x] F6 option C Stage 2 (seam-width measurement) — DONE 2026-08-23 on
      `accuracy/f6stage2-soak-expand`, per `accuracy/f6-decision-gate.md`
      (dated section) + `accuracy/evidence/f6-stage2-seam-measurement-2026-08-23.log`.
      GA40010 co-sim render, two geometries: type 0 seam = 16 mode-2 px (1 µs)
      on every display row, type 1 = none. The recreation renders the
      char-aligned DE gap one-for-one (byte-phase DISPEN latch exists but does
      not halve/anticipate). Stage 2 stopped before changing any candidate owner;
      its initial downstream-owner inference is superseded by Stage 2b below.
      No RTL change; suite/gates untouched.
- [x] F6 Stage 2b disambiguation — DONE 2026-08-23 on
      `accuracy/f6stage2-soak-expand`. Visual ACCC pp.185-187/195: R1>R0 is DISP ON for
      the first byte of C0=R0, BORDER for the second; p.195 explicitly places the
      0.5 µs pulse at the CRTC signal. Test and production both clock CRTC CLKEN from
      GA S=03; adding the test top's omitted production `nCLKEN` connection leaves the
      16 px result unchanged; original async and synchronous GA paths agree. Formal
      finding promoted as F13, BLOCKED-PENDING-HARDWARE-EVIDENCE; no production RTL.
      Q15/Q16 resolved by default visual readings. NEXT independent work: F7 RFD →
      F10 fixtures; next F13 action is SHAKER Module A (O) + DE-pin capture if possible.
- [x] Branch review note (clarification): no branch stacking is needed. Stream branches
      (`accuracy/*`, `plus/*`) cut from `accc-review-and-fixes`; the base branch itself
      carries no new review-debt rows by decision, which is only safe because its whole
      diff gets one review pass before its content is treated as settled/upstreamed.
      The pass-1 and pass-2 records now exist; the pass-2 remediation commits await the
      user's next independent confirmation pass before the rows clear.
- [x] Whole-branch independent review ran 2026-08-23
      (`accuracy/accc-review-and-fixes-independent-review.md`): per-type split accepted as
      sound (mux seams, latch cluster, holdoff, hcc==0 capture all confirmed; ~45.5M-sample
      differential reproduced). Its six issues fixed on `accc-review-and-fixes`: 90f0cda
      GA40010 co-sim manifest (+ lint/render proof), 72d7cf4 handoff/roadmap tip-grounding,
      4140ebb F6 premise stamped SUPERSEDED, d66ec23 soak claim bounds + re-mint protocol,
      c7558ae rename sweep + Q15/Q16 numbering. Guide wording fixes it refuted landed in
      6cfd4dd beforehand. At that pass-1 tip, gates were 87 required passes and soak hash
      `0x5b5004ff70148443`. Those resolutions awaited pass 2.
- [x] Independent pass 2 preserved verbatim at
      `accuracy/accc-review-and-fixes-independent-review-pass2.md` (reviewed tip
      `0773ad47369f983094201c142122f6e2a1425d29`). All 11 findings were confirmed and
      remediated in focused commits: P0 cancellation/late-response safety (`c889142`) and
      load-time MMU waiting with a production-sized integrated vector (`a3dc85a`); exact
      range whitespace (`68b8aef`); complete GA40010 co-sim manifest (`42e8fd2`); live-R2
      HSYNC collision from ACCC §15.3.5 p.151 (`d5d8416`, docs `bfb2057`); canonical
      handoff (`7c40f93`); differential `r6_border_condition` coverage and preserved rerun
      (`b75330b`); wrapper-path sweep (`cdfc7ff`); honest t01e oracle classification
      (`de6e14e`); F6 approximation wording (`b077821`); and residual counts/status
      (`f6f09f5`, with the duplicate review-debt header removed in the final handoff).
      No GA40010 netlist RTL changed. The canonical soak hash stayed
      `0xf5f8ae01ffdf928d`. GitHub Actions run `32645547100` is green at `f6f09f5`
      (simulation plus Quartus synthesis/package/upload). Fixes await reviewer confirmation.
- [x] Independent pass-2 fixes verification (report title: pass 3) preserved verbatim at
      `accuracy/accc-review-and-fixes-independent-review-pass2-fixes-verification.md`.
      It reviewed `0773ad4..d64e449`, reran all five relevant gates, accepted all 11 fixes,
      and cleared every branch-level review row on 2026-08-23. No blocking issue was raised.
      Q1 (standalone GA40010 Makefile portability/warning policy) and Q3 (the R3=0 collision
      guard is an unverified model assumption) are queued with Plus P1 follow-ups; Q2's t04a
      isolation and Q4's fail-closed load-time WAIT policy were accepted without code change.
      No GA40010 netlist change was made or authorized. A1/A2 remain open and were outside
      this review.
- [x] Expand the soak sampled field set (partial-VSYNC holdoff latch, type-1 status flops)
      and re-mint the golden hash at the next natural boundary (review Issue 4: the current
      projection is exactly how the dev-time holdoff bug escaped it). The F6 Stage 1
      behaviour change took the earlier re-mint slot (hash now `0x326ea81358e7d88f`,
      rationale recorded); this field-expansion re-mint is still queued and must land with
      its own minting note.
      DONE 2026-08-23 (`accuracy/f6stage2-soak-expand`): three fields added after the
      existing ones in `soak_mix_sample` (`type0_vsync_wait_line_start`,
      `r6_border_condition`, `status_bit5_r`); no RTL change; suite stayed 93 passed /
      lint clean throughout; golden hash re-minted to **`0xf5f8ae01ffdf928d`**
      (same seed/sampling/event schedule — delta is exactly the added fields), verified
      identical across two runs and via `SOAK_EXPECT`; rationale: review issue 4
      remediation. Recorded in this plan, sim/README.md,
      accuracy/type-split-review-guide.md, and AGENTS.md.
- [x] F7 RFD primary R5 trigger — DONE 2026-08-23 on `accuracy/f7-rfd`:
      type-1-only same-edge `R5 0->nonzero` arming at `CLKEN && hcc==R0`, RFD VMA-source
      reload, parity-gated VMA' save, odd-R9 frame-parity alternation, successful-save
      disarm, and the R1>R0 bare-C9 disarm route (B6). `t13a`-`t13d` are required passes;
      `t13a` is the directed never-triggered proof (R5 written away from C0=R0). Random
      soak traffic legitimately arms the new behaviour, so with the seed, schedule, and
      sampled fields unchanged the hash moved from `0xf5f8ae01ffdf928d` to
      **`0xae27f2c3c758ed87`**, reproduced twice and checked with `SOAK_EXPECT`. The soak
      does not prove bit identity when unarmed. RFD#10's optional "1-B" chip variant is
      explicitly not modeled.
- [x] A1 adjustment-ending VSYNC corner — DONE 2026-08-23 on `accuracy/f7-rfd`:
      the type-1 `row+1` VSYNC comparator substitution now excludes `crtc1_adj_end`, because
      the actual transition is final adjustment row → C4=0. New `t08m` derives a minimal
      R4=1/R5=2/R9=0 case from ACCC §§16.1/16.4.2; existing `t08g` was the same stale
      final-row+1 oracle and is corrected to require R7=39 silence. This exposes a source
      tension with the older §28.1.1 discriminator wording; the later independent review
      must check that reconciliation explicitly. With seed/schedule/projection unchanged,
      the intended VSYNC delta re-minted the soak from `0xae27f2c3c758ed87` to
      **`0x6439f9805b20acaa`**, reproduced twice and checked with `SOAK_EXPECT`.
- [x] A2 §11.2.4 exact-edge caveat pair — DONE 2026-08-23 on `accuracy/f7-rfd`:
      `t08n` proves an R4(>0) rewrite at C0==R0 adjustment entry suppresses the type-1
      C4=1 R12/R13 reload; `t08o` proves an R9 write on the same edge does not. The narrow
      type-1 engine predicate also controls the wrapper's existing adjustment-origin marker.
      With seed/schedule/projection unchanged, this intended behavior delta re-minted the
      soak from `0x6439f9805b20acaa` to **`0x512eaae74a628dca`**, reproduced twice and
      checked with `SOAK_EXPECT`.
- [x] Plus queue: P0 parser→service/MMU/boot wiring DONE on `plus/p0-parser-wiring`
      (4556665: A5 decisions + oversized-cbNN abort; 7eaddf2: `plus_mmu` cartridge
      windows, production service→SDRAM hookup, CPU cart read bridge with WAIT
      insertion; 5a69ebe: CPR ioctl index 8 live, P0 boot integration bench).
      P1 CRTC3 counter/timing foundation DONE 2026-08-23 on
      `plus/p1-crtc3-foundation` (4 commits: register file + HCC, vertical chain,
      pointer/DE/skew, syncs; vectors t01a-t04h, ACCC cites at point of
      implementation). Integration inspection added an ACCC-derived `t03c` MA assertion
      and fixed the simultaneous C0=R1=R0 VMA'/VMA save/reload seam; the original code
      had incorrectly restored the stale row base on the first scanline of the next row.
      Pass-2 follow-up `t04h` pins the live R2 end/start collision and C3 continuation;
      t01e remains an explicitly unverified model assumption rather than a sourced rule.
      P1 remainder: pixel path ([KT] colour table to source) +
      CPU/WAIT contract decision at first motherboard instantiation.
- [x] Independent pass 4 (F7/A1/A2 + Plus follow-ups + CI policy delta,
      `df9e72f..dd3467b`) — DONE 2026-08-23, verdict CLEAR WITH NON-BLOCKING
      FOLLOW-UPS; record preserved at
      `accuracy/f7-plus-followups-independent-review.md`. Both outstanding
      review-debt rows are cleared. All five findings remediated in the same
      pass: B6 early-clear comment, GA40010/u765 Tier-B classifier coverage
      (+tests+policy doc), status-doc hash/count refresh (current-status,
      roadmap, testbench-spec DoD), author question 17 (§28.1.1 type-1
      boundary). Gates rerun green after the changes; soak still
      `0x512eaae74a628dca`.
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

At end of any phase (or if the session must end), tick the checklist above, commit, and
note the resume point in one line here: **resume point: pass-4 review complete and both
review-debt rows cleared; the five findings are remediated on `accc-review-and-fixes`
(engine comment, CI classifier coverage, status-doc refresh, author question 17, debt
clearing + record at `accuracy/f7-plus-followups-independent-review.md`). The branch is
NOT pushed: pushing will itself trigger full Tier B synthesis because the classifier edit
is a self-triggering policy path — that run doubles as live validation of F-C. NEXT:
push, require green CI (simulation + synthesis + required-gate), then stop for the user
to pick the next queue item: classic residual §13.7.1.2 R0-widening route or F10
fixtures; Plus manual real-.cpr checkpoint then P1 remainder; F13 waits for SHAKER
Module A (O)/DE-pin evidence. Keep the classic and Plus streams separate.**
