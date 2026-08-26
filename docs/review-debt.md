# Independent review debt

**Status: all rows cleared (the `plus/p4-sprites` row closed 2026-08-26
after a five-pass thread ended CLEAR; all earlier rows closed by
2026-08-25)** (the `accuracy/f11h-and-ivm-vsync-coverage` row
cleared by the same-day remediation after its NOT CLEAR verdict; the `accuracy/f10-fixtures`
row cleared by the Claude Opus 5 review + remediation; all others cleared 2026-08-24). The
pass-3 verification accepted the earlier remediations at reviewed tip `d64e449`, and the
same-day pass-4 review accepted the `accuracy/f7-rfd` classic delta, the `plus/p1-followups`
work, and the CI synthesis-policy commit
(`docs/accuracy/f7-plus-followups-independent-review.md`). The `accuracy/f7-rfd-r0-widening`
row opened 2026-08-23 took two cross-provider passes (Claude Opus 5 reviewing an
Ox-Alpha-authored delta): pass 1 returned NOT CLEAR on two blocking findings, and pass 2
cleared the remediation delta on 2026-08-24, after which the branch was merged into
`accc-review-and-fixes`. Full record:
`docs/accuracy/f7-r0-widening-independent-review.md`.

## Cleared rows

- `plus/p4-sprites` — CLOSED 2026-08-26. Five passes by Codex/GPT-5.6
  Sol high on the sprite-engine phase. Passes 1-2 (NOT CLEAR) fixed
  suppressed-completion sreq stranding, a walk skip that first starved
  odd sprites then trapped the walker in one bank half when sprite 15
  was disabled, prose-only exit-65 accounting, and a test-side mag
  misconfiguration that had manufactured the phantom post-flush refill
  residual (`69e5d91`, `9f77dc3`). Pass 3 (NOT CLEAR) exposed a real
  pre-existing seam race: same-edge issues captured pre-edge bank state,
  so delayed ACKs could mark row-N data delivered into a bank retagged
  N+1; fixed with completion-time row-tag validation (`bb6a3a7`). Pass 4
  (NOT CLEAR) confirmed that mechanism hazard-free but found a pixel-data
  WRITE during an in-flight same-sprite fetch returning the pre-write
  byte after ACC_EN dropped; fixed with whole-life request poisoning
  via fq_acc, plus an s11 Y2 source-row oracle fix and an s12 early-arm
  pin (`f9c93dd`). Pass 5 reviewed exactly that delta: **CLEAR**, no
  findings; merged to `accc-review-and-fixes` at `143a213`.

This file tracks work that was merged without the independent cross-provider review the
project normally requires. It exists because that review was unavailable at the time, not
because the work was judged low risk. Clear entries from this list only after a real
independent review has run; do not clear them because a later change touched the same file.

## Why the debt exists

The project's standing rule is that every non-trivial diff authored by the main model gets a
fresh independent review from a different provider, so no model is the sole reviewer of its
own work. When this debt was opened on 2026-08-19, the GPT-5.6 Sol reviewer route was quota-
blocked and Fable was excluded because it consumes paid credits. A same-provider review did
not satisfy the standing rule. This is the historical reason the rows were recorded; review
capacity later returned and the records below say exactly what cleared them.

The accepted interim position was to keep implementing behind the deterministic Verilator
gate and CI synthesis, and record exactly what had not been read independently. The original
per-commit debt and the later branch-level generation have now both been repaid; open action
items remain implementation scope rather than uncleared review rows.

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
| `cd47d7d` | `rtl/plus/plus_cpr_parser.v` (new, 465 lines), tests (+676) | New RTL parsing untrusted external input (RIFF/CPR download stream). Fail-closed behaviour, bounds handling, and abort paths are exactly the class of code that deserves adversarial reading. It was tied off at this review point; P0 later wired it live. | **Clear** (observations recorded) |
| `de71808` | `rtl/UM6845R.v` (+87), `sim/sim_main.cpp` (+503) | F4: removes the non-equality `!line_max` / `!R4` counter shortcuts for both CRTC types. The audit flagged this as the riskiest classic change, because equality-only rollover changes behaviour for every out-of-range register combination, not just the documented cases. | **Clear** |
| `90aed07` | `sim/sim_main.cpp` (+477) | Test-only `t20a`-`t20h` R12/R13 video-pointer reload vectors, written by the Gemini writer bridge rather than by the main model. Same risk class as `c4c3e0f`, and one specific point to check: `t20g` asserts MA is 0 at character 0 of an `R0=0` frame, where ACCC 20.3.1 taken literally would load R12/R13 because C4 and C0 are both 0. That may be a legitimate reset-timing artifact or a masked divergence. | **Clear** (t20g concern resolved) |
| `c9f4a4e` | `rtl/UM6845R.v` (+60/-11), `sim/sim_main.cpp` (+273/-20) | F8: the type-1 vertical adjustment gets its own C5 counter while C9 keeps cycling and C4 keeps incrementing. Drafted by the Gemini writer bridge, not by the main model. Three things to read hardest. First, the VSYNC comparator now uses `row + 1` instead of `row_next` during type-1 adjustment, because `row_next` folds to 0 on the adjustment-ending line; that is a change to the sync path, which was outside the brief's stated scope even though `t08f`/`t08g` motivate it. Second, the ACCC 11.2.4 VMA-from-R12/R13-while-C4==1 reload was newly implemented, and its only test (`t08l`) was authored by the same agent, so nothing independent confirms the reading. Third, that rule's documented caveat is not implemented: the reload must NOT happen if R4 was rewritten to a nonzero value exactly at C0==R0 entering adjustment. Untested corner, recorded rather than fixed. | **Clear** (one corner finding → action item A1) |
| `b50e865`-`6ce976c` | `rtl/plus/asic_ga_timing.v` (new), `rtl/Amstrad_motherboard.v` (+120), benches, CI policy | P1 motherboard integration: behavioural GA timing path claimed cycle-exact via lockstep bench, plus full output muxing under `plus_mode`. An Opus-5-high review returned NOT CLEAR; both BLOCKINGs (uppercase implicit bus nets into asic_ga_timing — dead register decode, wedged ack latch, no Plus interrupts; inverted `plus_vidword` reset polarity) are fixed and CI re-greened. What a future reviewer should check hardest: (a) the queued findings 3-9 in `docs/current-status.md`, above all the missing Verilator lint/elaboration pass over `Amstrad_motherboard.v` that let finding 1 through, and the dead `plus_phi_en_*` wires leaving ga40010 owning CPU phase in Plus mode while ASIC READY gates WAIT; (b) the VIDBUF e0/even 03/odd byte-order mapping, explicitly assumed pending p1_video calibration; (c) classic-mode invariance is mux-inspection-only — no bench runs `plus_mode=0` at motherboard level. | **Clear-with-debt** (blockings fixed; follow-ups tracked as P1 review actions) **— follow-ups implemented 2026-08-25**: lint/elaboration pass live in the gate; phase-enable ownership muxed to the ASIC in Plus mode; U204-restart + randomised-fast lockstep added; INKR/inksel power-up pinned by RTL resets with bite test; plus_mode=1 motherboard bench m1-m4 in the gate; p1_video calibrated and gated (VIDBUF order now proven end-to-end by p1a). Remaining open item from this row's list: none — (c) still holds by design, classic-mode invariance remains soak+mux-inspection. |

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

- **A1 — F8 VSYNC comparator corner (`c9f4a4e`) — DONE 2026-08-23 on
  `accuracy/f7-rfd`.** On the type-1 adjustment-ending line,
  `((CRTC_TYPE && in_adj) ? row + 1 : row_next)` compares against `row+1` while the actual
  next row is 0 (new frame). If R7 equals (final adjustment row)+1 the core
  starts a VSYNC real hardware would not emit (C4 never takes that value; ACCC §16.1/§16.4.2).
  Unreachable with standard programming (R7<R4); reachable via dynamic-R7 tricks. Fix is to
  exclude the `crtc1_adj_end` case from the substitution; land with a focused deterministic
  vector derived from §16.4.2. Implemented with minimal `t08m`; the related `t08g` oracle
  now requires R7=39 silence instead of the same spurious final-row+1 pulse. The tension
  with the older §28.1.1 discriminator wording is named for the branch review.
- **A2 — F8 §11.2.4 caveat pair — DONE 2026-08-23 on `accuracy/f7-rfd`.**
  `t08n` requires an R4(>0) rewrite at exact C0==R0 adjustment entry to suppress the
  VMA-from-R12/R13 reload; `t08o` requires an R9 write on that edge not to suppress it (B5).
- **A3 — t20 companion vector — DONE 2026-08-23** (`t20i` on
  `accuracy/a3-f6-stage1`). Pins the live-entry R0=0 case (write R0=0 mid-frame landing on
  a wrap edge, freeze conditions armed): the wrap-edge §20.3.1 VMA reload from R12/R13,
  the single armed C4 increment consumed on the first repeated C0==0 (§13.2.6 p.108), and
  R12/R13 writes staying ignored while frozen against a non-zero latched pointer
  (§13.8.3 p.129).
- **A4 — cosmetic — DONE 2026-08-25** (this branch): the `expect_known_*` helpers are
  renamed `expect_xfail_*` with a comment stating both behaviors explicitly (XFAIL in
  known-divergence-registered tests, ordinary failure anywhere else) and the
  use-only-in-fixture-commits house rule. They currently have no callers (the F10 behavior
  commits converted their sites to plain `expect_*`).
- **A5 — parser observations** (`cd47d7d`): RESOLVED 2026-08-23 on `plus/p0-parser-wiring`,
  before the parser was wired live. Oversized `cbNN` chunks abort (truncate was not
  required — the §11 "ignored" wording is tool tolerance, and excess bytes are
  unreachable); zero-`cbNN`-container abort kept; reset-time cleanup ownership documented.
  Decisions + rationale: `docs/plus/architecture.md` "CPR parser policy (P0)". The abort
  behavior landed with its own focused vector in the same commit series.

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

## Branch-level review register (locked decision 2026-08-22)

No per-commit rows are added for work on the branches below: the 2026-08-22 locked
decision treats them as authored by a single model (ox-alpha) to be reviewed **as one whole
diff** before any of their content is treated as settled or upstreamed. The reviewer's guide
with per-commit rationale, evidence commands, and a prioritized reading list is
`docs/accuracy/type-split-review-guide.md` (the `accuracy/a3-f6-stage1` row names its own
hardest-reading guidance inline).

| Branch | Scope of the whole-branch review | Status |
|---|---|---|
| `accc-review-and-fixes` | Canonical docs, GA40010 co-sim manifest, exact-range whitespace gate, and pass-2 remediation integration. | **CLEARED — GPT-5.6 Sol, 2026-08-23.** Pass 3 accepted all 11 remediations at reviewed tip `d64e449`; full record: `accuracy/accc-review-and-fixes-independent-review-pass2-fixes-verification.md`. |
| `accuracy/crtc-type-split` | Per-type engine split (`27efc2d`), wrapper rename to `rtl/CRTC.v` (`63f4c01`), session docs. | **CLEARED — GPT-5.6 Sol, 2026-08-23.** Pass 3 independently reconfirmed the wrapper mux seams, type-0 latch/holdoff ordering, shared captures, and expanded differential evidence. |
| `accuracy/a3-f6-stage1` | A3 companion vector `t20i`; F6 Stage 1 full-character approximation and its behavior-driven hash re-mint. | **CLEARED — GPT-5.6 Sol, 2026-08-23.** Accepted within the declared full-character approximation. F13 remains separately hardware-blocked. |
| `plus/p0-parser-wiring` | Production parser → cartridge service → SDRAM → MMU/CPU-WAIT path. Review hardest: cancellation, late acknowledgements, load-time MMU waiting, and classic-mode isolation. | **CLEARED — GPT-5.6 Sol, 2026-08-23.** Pass 3 traced cancellation/rearm and load replay, adversarially checked fail-closed parsing, and accepted the production-sized integration proof. |
| `docs/split-differential-evidence` | Frozen `418aa68` pre-split vs `2d4f880` split comparator and preserved 45,498,863-sample run. | **CLEARED — GPT-5.6 Sol, 2026-08-23.** Pass 3 reproduced the committed run including `r6_border_condition`; no divergence. |
| `plus/p1-crtc3-foundation` | Plus P1 CRTC3 counter/timing foundation, now 27 vectors (`t01a`-`t04h`). Review hardest: live R2 HSYNC collision, save/reload priority, vertical equality/overflow rules, and the explicitly unverified t01e R0-shrink model assumption. | **CLEARED — GPT-5.6 Sol, 2026-08-23.** Pass 3 verified the live-R2 fix directly against ACCC p.151 and accepted t04a's test isolation. The t01e and R3=0 collision cases remain labelled or queued as model assumptions, not review debt. |
| `accuracy/f7-rfd` | Type-1 F7 R5-trigger RFD, A1 adjustment-ending VSYNC correction, and A2 §11.2.4 caveat pair. Review hardest: same-edge R5/rollover ordering; parity/save disarm; B6 bare-C9 timing; the §§16.1/16.4.2 vs §28.1.1 t08g source tension; and A2 exact-R0 write discrimination. | **CLEARED — ox-alpha, 2026-08-23.** Pass 4 verified the vector expectations as rule-derived (not implementation-derived), accepted the A1 §§16.1/16.4.2 reconciliation with its §28.1.1 tension routed to an author question, and recorded the B6 early-clear interpretation as unobservable. Full record: `accuracy/f7-plus-followups-independent-review.md`. |
| `plus/p1-followups` | Pass-3 Q1 standalone GA40010 target tooling and Q3 R3l=0 collision-assumption labelling/vector. Review hardest: `VERILATOR_BIN` override isolation, non-fatal warning policy, and whether t04i pins the current assumption without implying an ACCC oracle. | **CLEARED — ox-alpha, 2026-08-23.** Pass 4 empirically verified the override in both CLI and env form, confirmed no GA40010 netlist source changed, and confirmed t04i labels the assumption without claiming an ACCC oracle. Full record: `accuracy/f7-plus-followups-independent-review.md`. |
| `accuracy/f7-rfd-r0-widening` | §13.7.1.2 p.124 R0-widening RFD trigger: wrapper `hcc_end` line-end strobe (defers C0 wrap, MA increment, roll-into-R1 display end, odd-field count tick), engine pending-window/arm state, vectors `t13e`-`t13k`. Review hardest: completeness of the `hcc_end` consumer switch (any line-event consumer still on raw `hcc_last` fires at the suppressed edge); same-edge ordering of window-set vs -clear vs arm against the R5-route flag updates; the end-state reading of the documented "(C9 != R9 / C4 != R4 by line end)" variant definitions behind `t13h`; and bit-identical type-0/ordinary-type-1 behavior outside the recipe (directed guards plus an unchanged soak hash). | **NOT CLEARED — Claude Opus 5, 2026-08-23** (cross-provider: the delta was authored by Ox-Alpha). Record: `accuracy/f7-r0-widening-independent-review.md`. Two blocking findings, both inside the new feature; nothing pre-existing regresses and all three gates pass as reported. **F-1**: the `~(CRTC_TYPE & e1_rfd_r0_extend)` guard on the `hcc_next == R1_h_displayed` term in `rtl/CRTC.v:385` is vestigial — `hcc_next` already carries the correct continuation value `R0_old+1` at the suppressed edge — and its only effect is to block the legitimate DISPEN-off at `C0==R1` when `R1 == R0_old+1` (ACCC §6.1.3 p.33), a configuration family §11.6 p.87 ties directly to RFD; measured DE stays high through the widened remainder, and removing the guard fixes it with all 107 passes and the soak hash unchanged. **F-2**: `t13j`'s fixture lands the write at `C0=6, C4=1, C9=0`, not at `C0==R0` off the last line as documented, so the `rfd_r0_widen_at_last_line` last-line gate — the guard the "mid-frame widening deliberately unmodeled" scope claim rests on — is exercised by no vector. Non-blocking follow-ups F-3 to F-9 are in the record: dead `field_count_tick` guard, raw-`hcc_last` arm invariant left uncommented, stale window surviving a C0 overflow line, the unmodelled §8.6 second-frame stuck-C4 consequence, the §8.5-vs-§8.6 write-event/end-state tension behind `t13h`, the unarmed-parity interpretation, and the overstated "pins bit-identity" soak claim (the soak is measurably insensitive in this region). **Remediated in-branch 2026-08-23** (post-review commits): F-1/F-3 guards removed from `rtl/CRTC.v` and `rtl/crtc_type1_engine.v`; F-4 invariant comment added at `rfd_r0_arm`; F-2 fixed by retiming `t13j` onto a genuine `C0==R0` edge and adding `t13l` (line-half gate) and `t13m` (DE blanking at `C0==R1`, ACCC §6.1.3); each new vector bite-tested against its reverted mechanism; F-5/F-6 scope notes added to `audit-findings.md`, F-7 routed to author question 18, F-8 marked ⚠ for hardware, F-9 erratum recorded in `current-status.md`. Suite 109/109, lint clean, soak `0x512eaae74a628dca` unchanged. **CLEARED — Claude Opus 5, 2026-08-24.** Pass 2 re-reviewed the remediation delta (`729ba02..ab98c6b`): both blocking findings are genuinely fixed, `t13m`'s DE expectations re-derive from ACCC §6.1.3 p.33 rather than from the simulator, `t13j`/`t13l` now sit on genuine `C0==R0` edges with counter asserts at the write point, and the three bite-tests were reproduced independently (reinstating the F-1 guard fails only `t13m`; dropping the row gate fails only `t13j`; dropping the line gate fails only `t13l`). Five non-blocking follow-ups N-1 to N-5 recorded in the pass-2 section of `accuracy/f7-r0-widening-independent-review.md`, including a backwards clause in the new `rfd_r0_arm` invariant comment and an unverified emergence claim in the F-6 scope note. Merged into `accc-review-and-fixes` on 2026-08-24. **Pass 3 — GPT-5.6 Sol high, 2026-08-24:** fresh-eyes cross-check of the pass-2 verdict, requested because passes 1 and 2 shared a reviewer. AGREE with CLEAR, no blocking findings, gates independently reproduced. Sol found one thing both earlier passes missed — `t13e`/`t13j`/`t13l` cited §13.5 p.121, which is the CRTC 2/3/4 contrast (digest §8.7), for a type-1 rule that belongs to §13.3 p.113 — and argued that the N-2/N-3 wording defects should be fixed rather than deferred. All corrected in the follow-up commit; details as N-6/N-7 in the pass-3 section of the record. |
| `accuracy/f10-fixtures` | F10 interlace parity machinery, three commits `20eb6d5`/`657ccde`/`3a2293a`: type-1 two-stage R8-toggle parity update + §19.8.2 counting; type-0 split C9/C9.VMA with seam-latched value-doubled and line-scoped target-parity bits; §19.5.2/§19.5.3 parity rules; 31 required vectors (`t21a`-`t21p`, `t22a`-`t22o`); soak re-mints to `0x83e80134f7705b46`. Review hardest: the type-1 stage machine against the pp.210-211 panels directly (leaving-stage-A C9.0 hold; odd-ParityFrame stage-B reduction); the type-0 seam/toggle lifecycle against p.219's "next C0=0" rule incl. the same-edge write race; the three-way type-1 row-end split as bit-identical outside IVM (t08i/j/l, t13a-d sensitive); the t22 exit-fixture C4=1 shape against pp.223-224; and the t09g RA re-derivation against §19.5.2. Reviewer guidance in `accuracy/f10-implementation-notes.md`. | **CLEARED — Claude Opus 5 (claude CLI, fresh session), 2026-08-25.** Verdict NOT CLEAR on two blockings, both inside the new F10 behavior; full record `accuracy/f10-independent-review.md`. B-1: the type-1 leaving stage A must write C9.0 (the four X=1 pp.210-211 panels draw the change in the 3rd-µs column; the p.209 prose states the 3rd-µs rule once for both directions). B-2: the §19.8.2 match-branch ParityC9 toggle and the C9 restart were split at frame boundaries. Five bite-tests reproduced; gates verified. All blockings and accepted non-blockings (N-1..N-3, N-5..N-11, N-13) remediated in the follow-up commit; N-4/N-12 recorded as scope notes/errata. Post-remediation gates: 147 required passes, 0 failed, soak `0xa9e5026de83d287c`. |
| `accuracy/f11h-and-ivm-vsync-coverage` | F11h closure (same-edge R12/R13 write caught by the type-1 row-0 VMA reload, §20.3.2 p.242; vectors `t20j`/`t20k`), t24 type-1 IVM VSYNC positions from the p.208 table with the §19.8.2 p.225 alternation (vectors `t24a`-`t24c`), and a CI-only checkout bump. Review hardest: the p.242 shaded-cell semantics derived from the CRTC-0 contrast pair; the harness same-edge write phase in `t20j`/`t20k`; the p.208 box grid and the odd-C4-count alternation arithmetic behind the R4=6 fixture geometry; and the VSYNC fire/count-tick mux changes against the p.208 prose (MID-VSYNC on the ParityFrame-even frame). | **CLEARED — Claude Opus 5 xhigh (ask-claude bridge, fresh session), 2026-08-25.** Record: `accuracy/f11h-t24-independent-review.md`. Verdict was NOT CLEAR on one blocking finding: **B-1** — the t24 fix removed the type-1 IVM MID-VSYNC (§19.5.3 p.208 schedules it on the ParityFrame-even frame) and no vector sampled the half-line phase, while `f10-implementation-notes.md` claimed the residual narrowed. All four rule claims had been verified PDF-first and confirmed; no vector weakened. Remediated at `0304afa`: the wrapper keys the type-1 IVM VSYNC on ParityFrame directly (even-parity frames start/end at the half-line tick via a seam-latched fire decision, odd-parity frames keep the seam), `t24c` pins the phase, the notes bullets corrected. The reviewer sandbox could not execute gates or bite-tests; the parent reproduced the gates (152 required passes, soak `0x63d9de100ac9f6f2`) and all three specified bite-tests — (a) legacy-mux revert fails exactly t24b+t24c, (b) plain-C9==R9 row-end revert fails exactly t24a+t24b+t24c, (c) F11h override removal fails exactly t20j — each restoring to green. Non-blockings: N-3/N-4/N-5 fixed; N-1 recorded as a residual comment; N-2 folded into A4; N-6's upload-artifact bump queued as the next CI-only item; N-7 covered by synthesis-on-merge. |


The same rule applies here as everywhere else in this file: clear an entry only after a real
independent review has run — not because later work touched the same files. The pass-3 record
reviewed `0773ad4..d64e449`, reran the 93-vector suite, lint, canonical soak, exact-range
whitespace check, and split differential, and accepted every row. Integrated run
`32645547100` remains green at `f6f09f5`; all commits after it through the reviewed tip are
documentation-only.

### Pass-3 non-blocking issue triage (2026-08-23)

- **Q1 — GA40010 standalone target: implemented and independently reviewed.** The target
  honours `VERILATOR_BIN`, uses Verilator's generated build instead of hardcoded
  include/compiler paths, and makes the existing warning policy non-fatal. No GA40010
  netlist pin was wired or otherwise changed; pass 4 confirmed all three properties.
- **Q2 — t04a isolation: accepted, no follow-up.** The reviewer confirmed the assertion loop
  is unchanged and the bounded drain fails rather than hiding an infinite HSYNC.
- **Q3 — R3=0 collision guard: implemented and independently reviewed.** RTL and handoff
  docs label the guard as an unverified model assumption; focused vector `t04i` pins the
  current bounded choice without presenting it as an ACCC-derived oracle (confirmed by
  pass 4). Any future behaviour change remains vector-first.
- **Q4 — load-time unbounded WAIT: accepted fail-closed policy.** The HPS-wedge consequence
  is documented and does not reopen P0. Real `.cpr` hardware boot remains a separate
  milestone gap.

Pass 3 raised no blocking issues. A1 and A2 were subsequently implemented on the later
`accuracy/f7-rfd` branch; both were outside pass 3's scope and were reviewed in pass 4
(`accuracy/f7-plus-followups-independent-review.md`), which cleared the row.

## Rule for future unreviewed work

New commits merged without independent review get a row in the table above in the same
commit that introduces them. A change that is both high risk and cheaply deferrable should
wait rather than grow this list.
