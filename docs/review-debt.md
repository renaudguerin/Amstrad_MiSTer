# Independent review debt

This file tracks work that was merged without the independent cross-provider review the
project normally requires. It exists because that review was unavailable at the time, not
because the work was judged low risk. Clear entries from this list only after a real
independent review has run; do not clear them because a later change touched the same file.

Cleared rows, the original 2026-08-22 per-commit review record (action items A1-A5), the
branch-level review register and the pass-3 triage are archived verbatim in
[archive/review-debt-cleared.md](archive/review-debt-cleared.md). Move a row there, unchanged,
once a real independent review clears it.

## Rule for future unreviewed work

New commits merged without independent review get a row under "Open debt" below in the same
commit that introduces them. Name what a reviewer should look at hardest. A change that is both
high risk and cheaply deferrable should wait rather than grow this list.

## What this does not excuse

- The Verilator suite and the GitHub Actions synthesis job still gate every commit. A red
  gate is a blocker regardless of review status.
- Hardware-test results are still the authority over simulation results.
- A finding must still have a deterministic regression test before its behaviour is changed.

## Open debt

Newest first. A row stays here until a real independent review clears it; source-review
clearance never closes a hardware gate, which is tracked in `backlog.md` and
`implementation-roadmap.md`.

**Classic GA serializer, 2026-11-05 — EXPERIMENTAL; stronger source review OPEN:**
Branch`codex/classic-ga-boundary-native-20261105` adopts the source-qualified
retained-byte/cursor/mode-epoch model and settled-SNA/prefetch normalization.
This is diagnostic adoption, not integrated or release acceptance. Gemini3.8's
supplementary broad review was insufficient for hard approval; the prepared
early Opus5.5high attempt failed401 expired OAuth. A new authenticated strong
review is now active after the user refreshed login. A fresh
narrow Gemini integration audit and root source review found no porting defect.
Look hardest at LOAD/MODE/PHI coincidence ordering, retained-byte replay across
adjacent phases, token/anchor epoch validity and16-bit wrap lifetimes, border/
palette timing, held-SNA apply/fetch freshness and no-HS restored-mode behavior.
Strong Opus5.5 review now identifies G1: the current colour refresh chooses
incoming DISPEN while rendering the outgoing byte's last pixel, moving border/ink
one refresh early. A failing DE-edge vector and correction are required before
this row can clear. Queue/anchor/replay, SNA freshness and wrap had no identified
counterexample; the optional identity simplification is deferred. The model is
abstract; source-derived RGB controls do not prove a unique silicon
pipeline or all phases. Retained64 production-T80 controls,72 public GA lifecycle
controls, shared B18/ASIC callers and46selected benches pass; no code edit followed
the final gate. Clean full synthesis and bounded C0/C1 DSC4/Amazing native
comparisons now pass ([evidence](investigations/hardware-runs/classic-ga-native-2026-11-05.md));
this does not clear the source-review debt or establish all-phase acceptance.
[Source and proof](investigations/hardware-runs/classic-type0-mode-boundary-2026-11-04.md),
[active handover](investigations/session-logs/overnight-resume-2026-11-04.md).
Keep this debt until an adequate fresh strong review actually runs; hardware
success or a weak review alone does not clear it.

**ACCC page-anchor migration to the v1.11 re-issue, 2026-09-21 — REVIEWED (Gemini 3.8 Flash
high, run `20260921T175020Z-80016-18b2`; Astra low, run `20260921T175020Z-79905-dd1b`), both
CHANGES REQUIRED; round-2 re-review (runs `20260921T180739Z-90193-140e`, `20260921T180739Z-90182-2e61`)
confirmed the round-1 fixes and found more stale citations, all fixed; round-3 Astra review
(run `20260921T182232Z-98421`) found nine more small items, fixed; round 4 (both reviewers)
confirmed every round-3 fix and found seven residual occurrences, fixed; Gemini round 5 (run
`20260921T215820Z-24363-180c`) confirmed those and found three more, fixed:** Opus shifted
about 1,000 English page anchors (+1 from first-print p.29) and renumbered first-print English
§14.4-14.8 to §14.5-14.9 across live docs, RTL and sim comments; only digits and chapter-14
section numbers changed, and the selected benches pass. Look hardest at: unlabelled anchors that
were really French (French pagination did not change, so a shifted French anchor is now wrong;
ten were caught and reverted, IA-series citations are the riskiest); anchors inside multi-page
sections, where the rule relied on the anchor predating the re-issue rather than on the section
map; and the follow-up that converted the remaining v1.10-labelled page citations to v1.11
re-issue pages and corrected the French §13.7.2.2 citations in `sim_main.cpp` from p.127 to
p.128, each checked against the PDF text.

**Per-change simulation selection and one-run gate policy, 2026-09-15 — REVIEWED BY SPARK
(Muse Spark 1.3 xhigh, run `20260915T120117Z-43757-1bbc`), declined findings open:** Opus
wrote the parallel wrappers in `sim/Makefile` and `sim/plus/Makefile`, the bench index
`sim/TESTS.md`, the selector `sim/select_tests.py`, the CI simulation step that runs it, the
6-boot default for `b6_video_boundary_test.cpp` (`--strict` keeps all 18), and the gate rules
in `CLAUDE.md`, `AGENTS.md`, `docs/ci-testing-policy.md` and `stream-finish`. Look hardest at:
benches a change can break but the selector skips (Covers globs are hand-written; the safety
net only catches RTL no row covers); the CI base choice for pushes, pull requests, new
branches and force pushes; `make -n` dependency parsing across recursive makes; sub-makes never
sharing a directory under `-j`; and whether dropping classic type 1 and HSYNC width 14 from the
default B6 run hides a real output-chain difference. Spark found no Makefile, parser or CI-base
bugs and confirmed no B6 assertion lives only in the dropped cases. Accepted: same-area Covers
for the output chain, GA lockstep diff, SDRAM boot, PPI/PSG/DMA and motherboard rows; strict-B6
guidance. Declined by design, because selection follows what a bench asserts rather than what it
compiles: classic CRTC/GA changes do not select Plus integration benches (p1, p10, B6-B8); an
index or selector edit selects no benches, and a `sim/TESTS.md`-only push skips CI (`**/*.md`
ignore) until the next code push runs `--check`; pull requests select from the PR head while
testing the auto-merge tree (integration goes through local merges and pushes, not PRs).

**B4 physical integration residuals — OPEN:**
The original raw-fetch-provider source/execution debt is closed by fresh Opus
5.5 medium review and five production motherboard/T80 cases; its unchanged
entry is in the [review archive](archive/review-debt-cleared.md). See the
[provider evidence](investigations/ssm-csl/b4-production-fetch-2026-09-29.md).
This does not establish banked memory, cartridge/SDRAM service, repeated HALT
fetches, or physical timing. DDR allocation has the earlier device evidence;
The separate [device session](investigations/mister-format2-b25-2026-09-29.md)
confirms controlled format-2 startup/coherent reads and matching steady requested/applied
modes in three output modes on `cec641c`. Main mapping enumeration, live mode
transitions and independent device proof of the fetch-sampling cut remain outside
that bounded acceptance. No new synthesis or hardware result is claimed by the provider fixture.

**B4 phase 0, CSL runner — source review closed 2026-09-29; hardware residual OPEN:**
Fresh Opus 5.5 medium review (`20260929T004043Z-66217-cccf`, CLEAR) closes the
four runner findings: CFG restoration after uncertain writes, unknown live CRTC
selection, unsupported live disk insertion, and cleanup failure status/exit code.
The power-on fold and known same-type no-op remain intact; the CTRL comment now
matches both mappings in `rtl/hid.sv`. See the [failure-injection and review evidence](investigations/ssm-csl/csl-runner-safety-2026-09-29.md).

Hardware acceptance remains separate: only 15 key-map entries have B2 device
confirmation; the remainder are source-derived, and the absent user-owned SHAKER
corpus skipped six tests in this checkout. In particular, RIGHTCTRL's complete
Main/MBC input path is not newly device-confirmed.
[Real SSH interruption recovery](investigations/ssm-csl/csl-real-ssh-recovery-2026-09-29.md)
now passes for an acknowledged complete CFG write followed by loss of its owned
SSH client: the runner reports failure and restores the exact bytes through fresh
connections. Partial hardware writes, persistent outages and active OSD recovery
remain unverified. These limits do not reopen the four source findings or extend
existing Phase 1 acceptance.
