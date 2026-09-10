# F19 independent cross-provider remediation review

> **Later evidence:** The author's direct 2026-08-31 answer to Q20 resolves N2 against the
> free-running C4 model reviewed here. The subsequent failure-first `t08j` correction retains
> adjustment/C5 while restoring the ordinary C4==R4 reset. This dated review remains an exact
> verdict on its original target; see `../accc-author-response-round2-2026-08-31.md`.

**Date:** 2026-08-28  
**Reviewed target:** branch `accuracy/f19-type0-c0-timing`, working-tree remediation over `7e7420d`  
**Reviewer:** OpenAI Codex (independent of the Gemini-authored implementation)  
**Reference:** `docs/references/ACCC1.11-EN.pdf`, SHA-256
`3e45eb7eea7dc8f0d7211f78bec4f8d00530ce3c00da2e76034fb24f7a751868`

## Verdict

**CLEAR.**

The behavioral finding B1 and documentation finding B2 are fully remediated. Type 0 retains
same-edge `R4`/`R9` evaluation per §12.2 pp.92–94, the discriminating vectors pin that rule,
and all four durable documents now identify the previous-`R9` ordering on p.95 as CRTC-2
specific under §12.4.1. N1 and N2 remain resolved at the documented confidence levels, and
every required gate passes. No blocking findings remain, so the
`accuracy/f19-type0-c0-timing` review-debt row is cleared.

The PDF was checked through `pdf-inspector` (`native_complete`, no OCR or encoding fallback),
then the relevant pages were rendered and inspected because section placement and the p.124
paragraph structure determine the rule scope.

## B2 remediation — CLEAR

The durable documentation now preserves the source's type boundary consistently:

- `docs/current-status.md:17,1042` identifies p.95 as CRTC-2-specific §12.4.1 and assigns
  Type-0 same-edge behavior to §12.2 pp.92–94.
- `docs/accuracy/compendium-01-counters.md:71-77` documents CRTC 0 under §12.2 and confines
  the previous-`R9` ordering to the explicit CRTC 2 note.
- `docs/accuracy/accc-1.11-differences.md:29-30,49-53` inventories p.95/p.96 under §12.4.1
  CRTC 2 and records Type-0 same-edge behavior under §12.2.
- `docs/accuracy/accc-author-feedback.md:32-38` identifies the v1.11 addition as CRTC 2 and
  states the contrasting CRTC-0 rule.

A targeted stale-pattern search over all four files is empty. A broader Markdown scan over
`docs/` finds only the corrected type boundary, and `git diff --check` passes.

## B1 remediation — CLEAR

The rendered ACCC v1.11 pages establish the type split directly:

- §12.2 pp.92–93 is headed **CRTC 0**. It says the Last Line equality is evaluated while
  `C0<2`, permits `R4` or `R9` writes in that window to validate the state, and says a write at
  `C0==0` can override a state that was true at the start of the line.
- The previous-`R9` ordering is on p.95 under **§12.4 CRTC 2 / §12.4.1 Last Line Concept**.
  It is not a Type-0 rule.

The implementation now matches the Type-0 source: `rtl/crtc_type0_engine.v:419-423` selects
same-edge `DI` for both `R4` and `R9` at `C0==0`. The three paper-derived vectors at
`sim/sim_main.cpp:1835-1923` cover both directions of the `R9` decision and the `R4` control:

- `t12c`: writing `R9` from 7 to 10 clears Last Line;
- `t12d`: writing `R9` from 10 to 7 validates Last Line;
- `t12e`: writing `R4` from 38 to 10 clears Last Line.

A focused mutation reintroduced the rejected CRTC-2 expression
`type0_c0_r9 = R9_v_max_line`. It failed exactly `t12c` and `t12d` (173/175), while the `R4`
control still passed. Restoring the reviewed expression returned the classic suite to 175/175.
The vectors therefore discriminate the repaired mechanism rather than merely documenting the
simulator's current output.

## N1 — CLEAR

ACCC §11.6.1 p.88 describes the general R5-trigger route: on the relevant `C9==R9` frame,
the RFD disables the state that sources VMA from `R12/R13`. Section 13.7.1.2 p.124 separately
defines the R0-widening paradox and says that, for its R4-cancel variant where `C9==R9` remains
true, `R12/R13` is considered on one frame out of two. Read together, p.124 is the specific
route behavior rather than an instruction to apply the general p.88 disarm unchanged.

The split at `rtl/crtc_type1_engine.v:417-427` reflects that distinction: the p.88 disarm applies
to `rfd_arm`, while `rfd_r0_arm` arms the VMA-source and parity states. `t13g` at
`sim/sim_main.cpp:4174-4212` reaches the p.124 R4-cancel case, asserts both flags, the same-edge
`R12/R13` reload, `C4` advancement, and `C9` reset. `t13n` remains the complementary p.88
R5-route case. The added RTL, vector, and finding commentary records why the apparently similar
conditions intentionally differ.

## N2 — CLEAR as an explicit model residual

ACCC §11.3.2 pp.85–86 says that, after `R5` becomes zero during Type-1 adjustment, adjustment
remains active, `C5` loops, and `C4` continues to be compared with `R4`; the following sentence
ties the actual reset and exit to programming a reachable positive `R5`. The first clause remains
ambiguous about whether an intermediate `C4==R4` match resets `C4` while adjustment stays active.

The remediation does not overstate that ambiguity as hardware fact. Question 20 records both
readings and labels the retained free-run behavior an adjudicated default; the RTL comment names
it as a residual. Extended `t08j` (`sim/sim_main.cpp:3646-3673`) now pins that model choice by
driving `C4` past `R4`, through 127 and back to 11 while adjustment remains active, then proving
that a reachable `R5=8` exits cleanly and resets `C4/C9/C5`. This closes the silent integration
assumption while preserving the need for author or hardware evidence if the residual is revisited.

## Independent verification

| Gate | Result |
|---|---|
| `make -C sim` | **PASS** — 175 CRTC tests passed; 0 xfailed, xpassed, or failed; every Plus suite passed |
| `make -C sim lint` | **PASS** — exit 0; existing non-fatal Verilator warnings only |
| `make -C sim soak SOAK_EXPECT=0x48146d2b681268ab` | **PASS** — hash matched exactly |
| B1 rejected-expression mutation | **PASS as a bite-test** — only `t12c` and `t12d` failed; restored tree returned 175/175 |
| `git diff --check` | **PASS** |

The preserved soak proves bit identity for its fixed seed and sampled projection. It is regression
evidence, not a replacement for the directed vectors or future hardware confirmation of N2.
