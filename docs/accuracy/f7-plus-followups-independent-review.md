# Independent review — F7/A1/A2 classic delta, Plus P1 follow-ups, CI synthesis policy

Reviewed 2026-08-23 by **ox-alpha** (same-model independent review under the 2026-08-22
locked decision in `docs/plans/2026-08-22-accc-review-plan.md`; cross-provider review
unavailable on this harness). This is the pass the two outstanding `docs/review-debt.md`
rows were waiting for.

- Repository branch: `accc-review-and-fixes`, clean at reviewed tip
  `dd3467bebe5a43c98efa422a86d49890a7ebc690`.
- Reviewed base: `df9e72f1f66d4c79dac8311a8365d4f5446bc28c`.
- Range: `git diff df9e72f..dd3467b` — commits `d97a4af` (type-1 F7 R5-trigger RFD),
  `cef4471` (A1 adjustment-ending VSYNC), `facbb7c` (A2 exact-C0=R0 caveat pair),
  `2509c7d` (tier Quartus synthesis by integration risk), integration merge `e70acc0`,
  rebased Plus follow-ups `efd8ea6`/`9198b84`/`0ffb05b`, merge `acd5cbd`, bookkeeping
  `dd3467b`.

## Overall verdict

**CLEAR WITH NON-BLOCKING FOLLOW-UPS** — all accepted and remediated on
`accc-review-and-fixes` in the same pass (see "Remediations landed" below). No blocking
finding. No assertion was weakened to go green: the `t08g` flip is an oracle correction
whose previous expectation encoded the spurious comparator A1 removes, landed in the same
commit as the fix with derivation comments.

## Findings (by severity)

### F-A · Low — stale status/handoff documents
`current-status.md` carried the pre-F7 canonical hash (`0xf5f8ae01ffdf928d`), a stale
93-pass count, and a "next independent work: F7 RFD" pointer; `implementation-roadmap.md`
repeated both stale values. Not covered by the four-file hash-consistency check named in
the review brief. Remediated by refreshing both files (canonical hash, 100-pass count,
completed-F7 wording).

### F-B · Low — internal inconsistency in `testbench-spec.md`
Line 110 said 100 passed while the Definition-of-done (line 146) still said 93.
Remediated.

### F-C · Low — Tier B auto-synthesis gap for GA40010 netlist sources
`scripts/ci/classify-synthesis-paths.sh` matched nested QIPs but no GA40010 *source*
file, so an edit to the frozen netlist recreation could evade every automatic Tier B run;
the u765 FDC controller had the same hole. Remediated by enumerating the five
qip-listed GA40010 sources (`ga40010.sv`, `video.sv`, `syncgen_sync.v`, `casgen_sync.v`,
`rslatch.v`) plus `rtl/u765/u765.sv` as required paths, with test cases for each true case
and for the sim-side siblings staying false (`ga40010_test.v`, `Makefile`), and by naming
both modules in `docs/ci-testing-policy.md`. Internal RTL remains simulation-only by
documented policy; default-branch/tag/dispatch builds always synthesize.

### F-D · Info — B6 bare-C9 disarm is level-triggered across the whole last line
`rtl/crtc_type1_engine.v`: `rfd_r1_gt_r0_disarm` is gated only by `line_last_w`, so the
VMA-source flag clears at the first CLOCK edge inside the last line rather than at the
C9==R9 match edge. Verified unobservable through MA: `frame_new_w` forces the R12/R13
reload at that same row boundary, and `row_addr_save` gating depends only on the parity
flag. Pinned by `t13c`. Rationale recorded as a code comment.

### F-E · Low — §28.1.1 arithmetic discrepancy lacked an author-question entry
ACCC §28.1.1 p.292 implies type-1 C4 reaches 39 for R4=36/R9=7/R5=16, but §§11.1-11.4
increment C4 once per 8 adjustment scanlines ⇒ max C4=38; the corrected oracle
(`t08g`/`t08h`) expects silence from R7=39 via §§16.1/16.4.2. The tension was documented
in the plan and vector comments but not in the designated author-questions channel.
Remediated as question 17 in `accc-author-questions.md`, which also records how open
question 4 (repeated-RFD sentence, p.88) interacts with the now-implemented F7 state
machine (arm wins same-edge; recipe triggers where C9!=R9, so nothing observable awaits
Q4).

## Per-area verdicts

| Area | Verdict | Key evidence |
|---|---|---|
| F7 RFD | Clear | Trigger/flags/disarm match digest §5 (`compendium-01-counters.md:212-254`): two independent flags (parity flag cleared only by reset/SNA/type-switch), parity-gated save, Case1/Case2 alternation via odd-R9 frame-boundary toggle, same-edge arming using old-R5+live-DI with the register-file decode duplicated exactly (`rtl/crtc_type1_engine.v:111` ≡ `rtl/CRTC.v:152`). `t13a` is the directed never-triggered proof backing the soak claim; `t13d` derives live-R5 adjustment entry from §11.4 p.86. RFD#10 and the §13.7.1.2 residual are explicitly out of scope. |
| A1 VSYNC | Clear | Substitution exclusion `!crtc1_adj_end` is correct: during non-ending adjustment lines `row_next==row+1` anyway, so the substitution only ever mattered on the ending line where C4 goes directly to 0 (§§16.1/16.4.2). `t08m` minimal paper-derived fixture. Source tension handled per F-E. |
| A2 reload caveat | Clear | Suppression touches only the origin marker; wrapper consumes the exported predicate. `t08n`/`t08o` assert the documented §11.2.4 p.84 pair with discriminating fixtures (VMA'=0x1238 vs R12/R13=0x2050). Widths/priorities correct (DI[6:0] R4, DI[4:0] R5; arm wins over save-clear). |
| Plus Q1 tooling | Clear | Generated-Verilator build flow replaces hardcoded paths; `VERILATOR_BIN ?=` override verified working via CLI and env; `unexport` isolates the wrapper variable; `-Wno-fatal` is the intended non-fatal warning policy; dependencies include the Makefile; clean target complete. No GA40010 netlist source touched. |
| Plus Q3 assumption/vector | Clear | `asic_video.v` delta is comments only; the collision wire pre-exists. `t04i` is named/commented as an explicitly unverified model assumption, consistent across RTL, vector, architecture doc, current-status, review-debt. |
| CI synthesis policy | Pass (F-C remediated) | Fail-closed SHA handling (zero/missing base → merge-base fallback → else required), `--no-renames` covers renames/deletes, PR three-dot merge-base, PR path-filter removed so the required-gate check always reports, single aggregating gate enforces skipped==expected, dispatch-clean invariant held for tags/default branch, manual milestones never cancelled. Docs consistent. |
| Integration/bookkeeping | Pass | Both merges byte-identical to clean automatic merges (`git merge-tree --write-tree` equality; zero conflict resolution); stream isolation proven by per-commit file lists; canonical hash coherent in AGENTS.md, plan, sim/README.md, split guide; handoff queue intact (F10 fixtures, F13 SHAKER Module A (O)/DE capture, manual real-.cpr checkpoint). |

## Gate and CI evidence (all rerun independently during the review)

| Check | Result |
|---|---|
| `git status --short --branch` / `git diff --check df9e72f..dd3467b` | Clean tree, in sync with origin; whitespace check exit 0 |
| `make -C sim` | Summary: 100 passed, 0 xfailed, 0 xpassed, 0 failed; all Plus suites green incl. 28 `asic_video` vectors (Verilator 5.050) |
| `make -C sim lint` | exit 0; known warnings only (UNUSEDSIGNAL, CASEX sdram.v) |
| `make -C sim soak SOAK_EXPECT=0x512eaae74a628dca` | hash matched (seed `0xaccc5eed20260822`, 2,845,088 CLKEN samples) |
| `make -C rtl/GA40010 clean && make -C rtl/GA40010` | builds, exit 0 (pre-existing MULTIDRIVEN warning) |
| `scripts/ci/test-classify-synthesis-paths.sh` | passed |
| GitHub Actions run `32652569271` @ e70acc09 | policy/simulation/synthesis/required-gate all success (full Quartus build correct: range contains build.yml) |
| GitHub Actions run `32653432699` @ dd3467be | simulation/policy/required-gate success, synthesis correctly skipped |

## Explicit confirmations

1. **Plus did not move classic behavior — confirmed.** File scoping per commit plus the
   reproduced canonical soak and 100/100 classic vectors.
2. **`0x512eaae74a628dca` is the coherent canonical hash — confirmed**, with the full
   re-mint chain (F7 → A1 → A2) each carrying a stated behavioral rationale and
   seed/schedule/projection invariance. (Stale copies elsewhere became finding F-A.)
3. **No production-relevant path can incorrectly evade synthesis — confirmed as
   documented policy after F-C**: internal HDL evasion is deliberate tiering backed by
   always-on default-branch/tag/dispatch builds and the manual-dispatch rule; the one real
   gap (GA40010 sources, u765) is closed.
4. **Both review-debt rows may be cleared — yes**, and they are cleared in the same pass
   that recorded this document, per the file's convention.
5. **No further implementation before the human picks the next queue item — confirmed.**
   Queues stand: classic → residual §13.7.1.2 R0-widening route or F10 fixtures; F13 →
   SHAKER Module A (O)/DE-pin capture (hardware-blocked); Plus → manual real-`.cpr`
   checkpoint then P1 remainder.

Standing caution: green gates verify the vectors, and the vectors verify against the
committed digests — the digests are the working oracle, not final authority. t13b/t13c/
t13d and t04i encode rule-derived readings of race-shaped hardware behavior that only a
SHAKER session (now unblocked for RFD by F7) can confirm.

## Remediations landed

All five findings were fixed immediately after acceptance, on `accc-review-and-fixes`:
engine comment (F-D), classifier/test/policy coverage (F-C), status-doc refresh
(F-A/F-B, including roadmap references found by sweep), author question 17 (F-E), and the
debt-row clearing recorded here. Gates were rerun after the changes: suite 100 passed,
lint clean, soak still `0x512eaae74a628dca`, classifier tests green.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
