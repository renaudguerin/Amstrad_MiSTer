# Independent review debt

**Status: fully repaid 2026-08-22** — all rows reviewed and cleared (see the review record
below); action items A1-A5 carry the findings forward.

This file tracks work that was merged without the independent cross-provider review the
project normally requires. It exists because that review was unavailable at the time, not
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

**All rows cleared on 2026-08-22** — see the review record below. Ordered oldest first, as
they stood. All were on `codex/exploratory-gx4000-plus-plan`. The last commit with a
genuine independent review had been `1a1233f`, covering the F12 C0>=2 arbitration slice only.

| Commit | Scope | Why it needed a careful read | Verdict |
|---|---|---|---|
| `da79915` | `rtl/UM6845R.v` (+49), `sim/sim_main.cpp` (+238), docs | Completes ACCC 1.10 type-0 last-line and vertical-adjustment entry arbitration. Touches the arbitration path that the reviewed `1a1233f` slice only partially covered, so the reviewed and unreviewed halves interlock. | **Clear** (read with `1a1233f` as one whole) |
| `c4c3e0f` | `sim/sim_main.cpp` (+607) | Test-only F4 counter equality vectors. Risk is not to the DUT but to the gate itself: vectors that encode the implementation rather than the ACCC rule would make the suite agree with a wrong core. | **Clear** |
| `cd47d7d` | `rtl/plus/plus_cpr_parser.v` (new, 465 lines), tests (+676) | New RTL parsing untrusted external input (RIFF/CPR download stream). Fail-closed behaviour, bounds handling, and abort paths are exactly the class of code that deserves adversarial reading. Currently tied off at the top level, which limits blast radius until P0 integration. | **Clear** (observations recorded) |
| `de71808` | `rtl/UM6845R.v` (+87), `sim/sim_main.cpp` (+503) | F4: removes the non-equality `!line_max` / `!R4` counter shortcuts for both CRTC types. The audit flagged this as the riskiest classic change, because equality-only rollover changes behaviour for every out-of-range register combination, not just the documented cases. | **Clear** |
| `90aed07` | `sim/sim_main.cpp` (+477) | Test-only `t20a`-`t20h` R12/R13 video-pointer reload vectors, written by the Gemini writer bridge rather than by the main model. Same risk class as `c4c3e0f`, and one specific point to check: `t20g` asserts MA is 0 at character 0 of an `R0=0` frame, where ACCC 20.3.1 taken literally would load R12/R13 because C4 and C0 are both 0. That may be a legitimate reset-timing artifact or a masked divergence. | **Clear** (t20g concern resolved) |
| `c9f4a4e` | `rtl/UM6845R.v` (+60/-11), `sim/sim_main.cpp` (+273/-20) | F8: the type-1 vertical adjustment gets its own C5 counter while C9 keeps cycling and C4 keeps incrementing. Drafted by the Gemini writer bridge, not by the main model. Three things to read hardest. First, the VSYNC comparator now uses `row + 1` instead of `row_next` during type-1 adjustment, because `row_next` folds to 0 on the adjustment-ending line; that is a change to the sync path, which was outside the brief's stated scope even though `t08f`/`t08g` motivate it. Second, the ACCC 11.2.4 VMA-from-R12/R13-while-C4==1 reload was newly implemented, and its only test (`t08l`) was authored by the same agent, so nothing independent confirms the reading. Third, that rule's documented caveat is not implemented: the reload must NOT happen if R4 was rewritten to a nonzero value exactly at C0==R0 entering adjustment. Untested corner, recorded rather than fixed. | **Clear** (one corner finding → action item A1) |

The two type-1 adjustment-identification cases in `t08` that were expected failures under F8
(`t08f`, `t08g`) are required passes as of `c9f4a4e`. The suite now has no expected failures.

## Review record (2026-08-22)

Reviewer: **ox-alpha**, same-model independent review under the 2026-08-22 locked decision
(cross-provider review unavailable on this harness). Method: each diff read against the PDF
verified digests (`accuracy/compendium-01/02/03`, corrections B1-B13 applied) and the ACCC
rules they cite; vector arithmetic re-derived from the cited sections, not from simulator
output. Order: `de71808` → `da79915`+`1a1233f` → `cd47d7d` → `c4c3e0f` → `90aed07` →
`c9f4a4e`.

- **`de71808` — clear.** Equality-only comparisons match ACCC §10.3/§12; the live-compare
  term used for the type-0 VMA' capture matches §11.2.2 p.82-83; the R5 window (`hcc<=2`)
  matches the §11.3.1 p.85 caveat; the exact-R0 straddle keeps the latched C9/R9 result for
  C4 while switching C9 to R5 (§11.2.2 p.82). XFAIL→required flips landed in the same
  commit. Cosmetic only: the `expect_known_*` helpers behave as hard assertions in
  required-pass tests (flag-false), so their name is now misleading.
- **`da79915` + `1a1233f` — clear as one arbitration whole.** Write windows are disjoint in
  C0 exactly as documented (R4 `[2,R0]`, R9 `[2,R0−1]`, exact-R0 separate straddle path);
  the C0==1 equality-break route requires the last-line state armed at C0==0; the C0=0 seam
  evaluates same-edge effective R4/R9; the R0=0 hiccup increment is CLKEN-gated and consumed
  exactly once (§13.2.6 p.108). Latch lifetimes are bounded by line boundaries, which keeps
  the priority chain safe despite overlapping terms.
- **`cd47d7d` — clear.** Adversarial pass found no escape: sequence validation
  (`ioctl_addr != expected_addr` aborts), 33-bit extent arithmetic cannot wrap (length ≤
  0xFFFFFFFF + pad ≤ 1 + position < 2^25 < 2^33), `riff_limit` capped below 2^25 so the +8
  cannot overflow, the pad byte is included in the header-time extent check, DONE-state
  trailing bytes abort, backpressure holds outputs stable and stalls the stream, and a
  download drop mid-`load_valid` fails closed (no commit). Observations, recorded not fixed:
  (a) `cbNN` chunks longer than 16 KiB truncate silently instead of aborting — writes stay
  in bounds, but an error might be more honest; (b) a well-formed container with zero
  `cbNN` chunks ends in ABORT, not COMMIT — conservative, keep; (c) a raw `reset` does not
  pulse `load_abort`; the memory service owns reset-time cleanup, which must stay true when
  P0 wires it live.
- **`c4c3e0f` — clear.** Every assertion traces to a cited section (§10.3, §12.2/§12.3,
  §28.1.1) and the counts re-derive correctly (overflow-to-limit, wrap-then-match
  sequencing, R7 boundaries 37/39). Zero-limit cases were honestly registered as xfails
  pre-fix and promoted, not weakened, by `de71808`.
- **`90aed07` — clear; t20g concern resolved.** The MA==0 expectation at character 0 is a
  cold-reset artifact and is defensible: no counter edge has occurred, so the event-driven
  §20.3.1/§13.2.6 reload has not fired before the hiccup pins C4=1. A level-literal reading
  of §20.3.1 would disagree for exactly one character; the live-entry scenario (R0 written
  to 0 mid-frame, the actual §13.2.6 setup) does produce the documented wrap-edge reload in
  the RTL. Action item A3 pins the live-entry case explicitly.
- **`c9f4a4e` — clear, with corner finding A1.** The C5 counter, the six-bit `C5+1==R5`
  comparison (no aliasing at C5=31), deliberate reproduction of the §11.3.2 R5=0 bug, the
  RA cycling, and the §11.2.4 reload (independently confirmed against the prose here,
  discharging the same-author concern about `t08l`) all match the verified rules. But the
  VSYNC comparator substitution is equivalent to plain `row_next` everywhere except the
  type-1 adjustment-ending line, and there it can fire spuriously — see A1. The
  unimplemented §11.2.4 caveat (R4-rewrite-at-C0==R0 cancels) stays recorded; B5 adds its
  complementary half (an R9 write at C0==R0 must NOT cancel).

### Action items arising (never silently fixed)

- **A1 — F8 VSYNC comparator corner (`c9f4a4e`).** On the type-1 adjustment-ending line,
  `((CRTC_TYPE && in_adj) ? row + 1 : row_next)` compares against `row+1` while the actual
  next row is 0 (new frame). If R7 equals (final adjustment row)+1 (= R4+R5+1) the core
  starts a VSYNC real hardware would not emit (C4 never takes that value; ACCC §16.1/§16.4.2).
  Unreachable with standard programming (R7<R4); reachable via dynamic-R7 tricks. Fix is to
  exclude the `crtc1_adj_end` case from the substitution; land with a focused deterministic
  vector derived from §16.4.2.
- **A2 — F8 §11.2.4 caveat pair.** Implement/test both halves: R4(>0)-rewrite at C0==R0
  entering adjustment suppresses the VMA-from-R12/R13 reload; an R9 write at C0==R0 does
  not (B5). Already noted in `audit-findings.md` F8 and `testbench-spec.md` planned additions.
- **A3 — t20 companion vector.** Pin the live-entry R0=0 case (write R0=0 mid-frame with
  the freeze conditions armed) so the documented "1st C0==0 reloads VMA" worked-example
  behaviour is protected, not just the cold-reset path.
- **A4 — cosmetic:** rename or annotate the `expect_known_*` helpers in `sim/sim_main.cpp`
  (they are hard assertions in required-pass tests).
- **A5 — parser observations** (`cd47d7d`): decide truncate-vs-abort for oversized `cbNN`
  chunks before P0 wires the parser live; document the service-side reset-cleanup ownership;
  keep the no-`cbNN`-container aborts behaviour.

## Repaying the debt

~~When independent review capacity returns:~~ Done 2026-08-22 (see the review record above).
The original plan, for the record:

1. Review `de71808` first. It has the widest behavioural reach and the audit already marked
   the shortcut removal as risky.
2. Then `da79915`, read together with the already-reviewed `1a1233f` so the arbitration path
   is assessed as one whole rather than two slices.
3. Then `cd47d7d`, reviewed as untrusted-input parsing, ideally before P0 wires it live.
4. Review `c4c3e0f` against the ACCC digests rather than against the RTL, to check the
   vectors assert the documented rule and not the current implementation.
5. Update this file as each item clears, and note the reviewing model and date.

All five steps were followed (with `90aed07` and `c9f4a4e` added per the same order);
findings became action items A1-A5 above.

## Rule for new work while the debt stands

New commits merged without independent review get a row in the table above in the same
commit that introduces them. A change that is both high risk and cheaply deferrable should
wait rather than grow this list.
