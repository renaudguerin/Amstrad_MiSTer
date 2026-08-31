# ACCC v1.11 bilingual implementation audit queue

This queue contains only French/English v1.11 differences that could expose a model or test
premise defect. It is downstream of the adjudicated source ledger
`accc-1.11-fr-en-differences.md`; it is not evidence that the current RTL is wrong.

Technical information is sourced from *The Amstrad CPC CRTC Compendium* by Longshot
(CC BY-NC-ND 4.0).

For each item, read the French page and the current RTL before writing a vector. Derive the
expected result on paper and cite the section beside the assertion. A model pass is not
hardware confirmation. Classic CRTC and Plus/GX4000 work remain separate streams.

## IA-1 — Type-0 live R3 update at exact HSYNC end — COMPLETE (controlled source model)

- **Source:** BL-025; v1.11 French §15.3.2 p.150. English p.148 omits the sentence.
- **Prediction to test:** in the documented infinite/re-entry configuration, a type-0 R3
  change landing exactly at `C0=R2+R3` makes C3l overflow rather than taking the ordinary
  clean end route.
- **Implemented evidence (2026-08-31):** `t33a` proves unchanged RTL already preserved the
  paper-derived C3l sequence 10,11,...,15,0,1. `t33b` then fails unchanged RTL at its first
  post-write sample (character 21, tick 3/16: expected HSYNC low, actual high), isolating the
  missing pin gap. A type-0-only pending restart suppresses the still-live comparator and
  raises HSYNC after 14 master ticks for this controlled phase, using the existing F20
  four-ticks-per-Mode-2-pixel scale. The source's earliest/approximately qualifier is retained;
  14 ticks is not a universal bus-phase or hardware oracle. `t33c` pins reset, snapshot,
  live-type, and R3l=0 lifecycle controls.
- **Owner:** classic accuracy stream.
- **Gate:** the implemented French worked shape uses old R2=11/R3l=10, new R2=21, and
  exact-edge R3l=1 with a no-write control. It asserts the paper-derived C3l sequence and
  pin phase separately; unchanged RTL passed only the counter half and failed the pin phase.
- **Acceptance:** GPT-5.6 Sol xhigh authored the two-file delta; guarded Gemini 3.7 Flash high
  independently returned CLEAR. Full simulation reports 181 required classic passes, lint
  passes, and exact soak `0x87a9d80a91381c9b` passes. Real type-0 hardware confirmation
  remains open. Full record: `ia1-r3-terminal-hsync-independent-review.md`.

## IA-2 — Type-1 frame-origin ParityC9 realignment — COMPLETE (source model)

- **Source:** BL-038; v1.11 French §19.5.3 p.209. English p.208 omits the assignment.
- **Prediction to test:** first make ParityC9 differ from ParityFrame through a documented R8
  transition, then cross a frame origin. An aligned-start fixture cannot distinguish explicit
  `ParityC9=ParityFrame` realignment from toggling both states.
- **Implemented evidence (2026-08-31):** `t32a` uses the paper-derived even-R9 route below
  to reach `(ParityFrame,ParityC9)=(0,1)`. With the RTL correction temporarily absent it
  fails at the origin (`ParityC9` actual 0, expected 1), distinguishing the old toggle-both
  model from the French assignment. Type 1 now seeds both ParityC9 and the IVM C9 restart
  from the newly toggled ParityFrame at every frame origin, while retaining the documented
  R8 stage-A/entering-stage-B priority. Existing IVM and adjustment expectations were
  re-derived against the p.209 worked table; the complementary odd-to-even origin is pinned
  in `t28a`.
- **Owner:** classic accuracy stream.
- **Gate:** the discriminator must reach the origin with **even R9**, because current RTL only
  toggles ParityC9 there when R9 is even; odd R9 can accidentally produce the French result.
  One paper-derived route starts with R9=3/R4=1/R8=0, reaches C4=1/C9=0, writes R9=2, then
  enters R8=3 to create `(0,1)` before origin. Preserve all existing IVM transition fixtures,
  and explain any soak-hash movement. The affected comments in `rtl/CRTC.v` and at the
  actual update logic in `rtl/crtc_type1_engine.v` use the v1.11 French §19.5.3 p.209 anchor
  and point back to this discriminator.
- **Acceptance:** 177 required classic vectors, full repository simulation, lint, exact soak
  `0x654a244c2cce6e0b`, and whitespace checks pass. The soak moved because random IVM traffic
  reaches the corrected origin. This confirms the local source model, not physical CRTC-1
  hardware; SHAKER or pin-trace confirmation remains open. The load-bearing stage-B
  priority finding was fixed and the narrow remediation re-review returned CLEAR using
  Gemini 3.7 Flash high through the guarded bridge; Claude was quota-unavailable. Full
  record: `ia2-frame-origin-independent-review.md`.

## IA-3 — Type-0 R6 live condition at C0=R1 — COMPLETE (source model)

- **Source:** BL-036; v1.11 French §18.3.2 p.191 versus English p.190.
- **Prediction to test:** on the first frame line, enter the R6=0 alternation, change R6 to a
  nonzero value before C0 reaches R1, and observe that border does not become definitive at
  R1 solely because R6 was zero earlier.
- **Implemented evidence (2026-08-31):** `t34a` uses reachable R0=7/R1=4, with a control
  that leaves R6=0 and a main arm that writes R6 0→2 at C0=3. On unchanged RTL the control
  failed at the first frame-line half (expected DE high, actual low): the ordinary
  `row_next==R6` assignment overrode the frame-origin display start. The corrected type-0
  priority preserves the p.191 high-then-low half-character alternation. A dedicated latch
  makes border definitive only when live R6 is still zero at C0=R1; the main arm proves the
  earlier zero does not persist after the live nonzero write. Reset, snapshot, type-switch,
  and row-transition lifecycle are explicit.
- **Owner:** classic accuracy stream.
- **Acceptance:** the failure-first vector and bounded RTL correction were separate Luna
  leaves. Guarded Gemini 3.7 Flash high independently returned CLEAR; its recommendation to
  add the new latch to the soak projection was accepted. Full simulation reports 182
  required classic passes, lint passes, exact soak `0x21bbf9c29ab08413` passes, and
  whitespace checks pass. Real type-0 hardware confirmation remains open. Full record:
  `ia3-r6-live-independent-review.md`.

## IA-4 — Type-0 R4-equality history during additional management — COMPLETE (source model)

- **Source:** BL-017; v1.11 French §13.2.1 p.106. English p.104 omits the requirement that R4
  remained equal to C4 throughout the line.
- **Prediction to test:** use identical line-end values with different mid-line history. In
  the transient arm, change R4 away from C4 after C0=2 and restore it before line end. French
  “remained equal” predicts a different next-line result from the no-write control.
- **Implemented evidence (2026-08-31):** `t16z` gives the control and transient histories
  identical line-end register values. On unchanged RTL the transient arm fails at C4
  (expected 2, actual 3), proving that the restoring write erased the required history.
  The corrected latch becomes sticky after any accepted unequal R4 write and clears only
  when the line is consumed or by reset/snapshot/type lifecycle events. The control reaches
  adjustment at C4=3/C9=0; the transient reaches C4=2/C9=4 from the p.106 history condition
  and section 11.2.2 comparator switch.
- **Owner:** classic accuracy stream.
- **Gate:** add the two-history discriminator before RTL. A minimal shape is R0=7, R4=2,
  R9=3, R5 written to 5 at final-line C0=2; transient arm writes R4 2→1→2 at C0=3/4.
  Bite-test any new “ever unequal” history latch against the control and existing t16 paths.
- **Acceptance:** Gemini 3.7 Flash high authored the bounded two-file delta; an independent
  GPT-5.6 Sol high review returned CLEAR. Full simulation reports 178 required classic
  passes, lint passes, and exact soak `0x654a244c2cce6e0b` remains unchanged because its
  fixed random schedule does not reach this history. Hardware confirmation remains open.
  Full record: `ia4-r4-history-independent-review.md`.

## IA-5 — U.S.-ROM GA interrupt/VSYNC scanline phase

- **Source:** BL-005; v1.11 §4.2 p.18. French says same scanline, before VSYNC; English says a
  different scanline.
- **Prediction to derive:** reproduce the documented U.S.-ROM two-line compensation and
  measure the GA interrupt-request edge relative to the CRTC VSYNC start.
- **Current evidence:** the 52-line GA counter exists, but no dedicated regression for this
  historical phase case was found. No hardware evidence was reviewed.
- **Owner:** classic accuracy stream for behavior; assign the shared GA/peripheral file owner
  explicitly before edits.
- **Gate:** first determine whether the production model exposes enough ROM/phase context for
  a meaningful deterministic test. If not, record a hardware discriminator rather than
  building a synthetic oracle.

## IA-6 — Type-0 adjustment and R0=1 premise reconciliation

- **Source:** BL-018–BL-020; v1.11 French §13.2.4 pp.107–108 and §13.7.2
  pp.126–127 versus English pp.105–106 and 124–125.
- **Premises to audit:** (a) C4/R4 inequality selects R5 as C9's target during additional
  management; (b) the normal next-line action is programmed at C0=0 from
  `C4=R4 && C9=R9` with no adjustment/interlace; (c) the initial R0=1 programmed increment
  includes C4=R4, while the later partially completed overflow case can persist after C4 has
  diverged.
- **Current evidence:** the digest has been reconstructed from French and existing R0=1/R5
  vectors cover much of the behavior, but this pass has not yet mapped each assertion back to
  the corrected premise.
- **Owner:** classic accuracy stream.
- **Gate:** produce a vector-to-premise table first. Add a new discriminator only for an
  uncovered condition; do not churn already-correct RTL or duplicate an existing test.

## Items deliberately not promoted to code work

BL-001–004, BL-006, BL-007, BL-010–013, BL-015, BL-016, BL-021, BL-023, BL-024, BL-026–032,
BL-034, BL-035, BL-040, BL-041, and BL-044 describe already-correct behavior,
documentation-only corrections, or an editorial clarification. BL-008, BL-009, BL-014,
BL-022, BL-033, BL-037, BL-039, BL-042, and BL-043 require author clarification,
out-of-scope CRTC2 work, or hardware evidence before an implementation premise exists.
