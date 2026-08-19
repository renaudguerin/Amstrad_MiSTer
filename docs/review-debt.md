# Independent review debt

This file tracks work that was merged without the independent cross-provider review the
project normally requires. It exists because that review is currently unavailable, not
because the work was judged low risk. Clear entries from this list only after a real
independent review has run; do not clear them because a later change touched the same file.

## Why the debt exists

The project's standing rule is that every non-trivial diff authored by the main model gets a
fresh independent review from a different provider, so no model is the sole reviewer of its
own work. That reviewer runs on GPT-5.6 Sol, and the Codex subscription quota is exhausted as
of 2026-08-19. The only alternative that satisfies the rule is a Fable second opinion, and Fable is
excluded for this project as of 2026-08-19 because it consumes paid credits. A same-provider
review does not satisfy the rule and is not a substitute. There is therefore no independent
review path available at all right now.

The accepted position for now: keep implementing, keep the deterministic Verilator gate and
CI synthesis as the mechanical safety net, and record here exactly what has not been read by
an independent reviewer. Review debt is repaid in a dedicated pass when quota returns, before
any of this work is treated as settled.

## What this does not excuse

- The Verilator suite and the GitHub Actions synthesis job still gate every commit. A red
  gate is a blocker regardless of review status.
- Hardware-test results are still the authority over simulation results.
- A finding must still have a deterministic regression test before its behaviour is changed.

## Outstanding items

Ordered oldest first. All are on `codex/exploratory-gx4000-plus-plan`. The last commit with a
genuine independent review is `1a1233f`, covering the F12 C0>=2 arbitration slice only.

| Commit | Scope | Why it needs a careful read |
|---|---|---|
| `da79915` | `rtl/UM6845R.v` (+49), `sim/sim_main.cpp` (+238), docs | Completes ACCC 1.10 type-0 last-line and vertical-adjustment entry arbitration. Touches the arbitration path that the reviewed `1a1233f` slice only partially covered, so the reviewed and unreviewed halves interlock. |
| `c4c3e0f` | `sim/sim_main.cpp` (+607) | Test-only F4 counter equality vectors. Risk is not to the DUT but to the gate itself: vectors that encode the implementation rather than the ACCC rule would make the suite agree with a wrong core. |
| `cd47d7d` | `rtl/plus/plus_cpr_parser.v` (new, 465 lines), tests (+676) | New RTL parsing untrusted external input (RIFF/CPR download stream). Fail-closed behaviour, bounds handling, and abort paths are exactly the class of code that deserves adversarial reading. Currently tied off at the top level, which limits blast radius until P0 integration. |
| `de71808` | `rtl/UM6845R.v` (+87), `sim/sim_main.cpp` (+503) | F4: removes the non-equality `!line_max` / `!R4` counter shortcuts for both CRTC types. The audit flagged this as the riskiest classic change, because equality-only rollover changes behaviour for every out-of-range register combination, not just the documented cases. |
| `90aed07` | `sim/sim_main.cpp` (+477) | Test-only `t20a`-`t20h` R12/R13 video-pointer reload vectors, written by the Gemini writer bridge rather than by the main model. Same risk class as `c4c3e0f`, and one specific point to check: `t20g` asserts MA is 0 at character 0 of an `R0=0` frame, where ACCC 20.3.1 taken literally would load R12/R13 because C4 and C0 are both 0. That may be a legitimate reset-timing artifact or a masked divergence. |
| (this commit) | `rtl/UM6845R.v` (+60/-11), `sim/sim_main.cpp` (+273/-20) | F8: the type-1 vertical adjustment gets its own C5 counter while C9 keeps cycling and C4 keeps incrementing. Drafted by the Gemini writer bridge, not by the main model. Three things to read hardest. First, the VSYNC comparator now uses `row + 1` instead of `row_next` during type-1 adjustment, because `row_next` folds to 0 on the adjustment-ending line; that is a change to the sync path, which was outside the brief's stated scope even though `t08f`/`t08g` motivate it. Second, the ACCC 11.2.4 VMA-from-R12/R13-while-C4==1 reload was newly implemented, and its only test (`t08l`) was authored by the same agent, so nothing independent confirms the reading. Third, that rule's documented caveat is not implemented: the reload must NOT happen if R4 was rewritten to a nonzero value exactly at C0==R0 entering adjustment. Untested corner, recorded rather than fixed. |

The two type-1 adjustment-identification cases in `t08` that were expected failures under F8
(`t08f`, `t08g`) are required passes as of this commit. The suite now has no expected failures.

## Repaying the debt

When independent review capacity returns:

1. Review `de71808` first. It has the widest behavioural reach and the audit already marked
   the shortcut removal as risky.
2. Then `da79915`, read together with the already-reviewed `1a1233f` so the arbitration path
   is assessed as one whole rather than two slices.
3. Then `cd47d7d`, reviewed as untrusted-input parsing, ideally before P0 wires it live.
4. Review `c4c3e0f` against the ACCC digests rather than against the RTL, to check the
   vectors assert the documented rule and not the current implementation.
5. Update this file as each item clears, and note the reviewing model and date.

## Rule for new work while the debt stands

New commits merged without independent review get a row in the table above in the same
commit that introduces them. A change that is both high risk and cheaply deferrable should
wait rather than grow this list.
