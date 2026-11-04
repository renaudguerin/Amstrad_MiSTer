# CRTC0 R4 switch: VSYNC must follow the committed row — 4 November 2026

A focused counter/output proof exposes a definite defect independently of the
SHAKER caller's final number. An R4 write selects vertical adjustment and holds
C4, but the VSYNC engine compares a proposed next row that is never entered.
The candidate uses the canonical row transition. Native acceptance is pending.

## Source and failing proof

French ACCC v1.11 §11.2.2 p83, rendered example 2, holds C4 after an unequal
windowed R4 write until adjustment completes and C4/C9 reset to zero.
§16.4.1 p170 starts Type0 VSYNC when C4 reaches R7. No equal R7 write occurs
in this proof; R0=7 reaches C0=2 normally, and interlace is off.

At R4=C4=2, R9=C9=3, R5=5 and C0=2, write R4=4. Program R7=3 beforehand,
so no earlier row matches it. At the first boundary, C4 must remain 2 and C9
become 4, with adjustment active and VSYNC low. The unchanged engine gets the
counters right but asserts VSYNC. Root's private sibling fails at character
96, tick 2/16. The retained t16a extension independently reproduces that failure
before the RTL edit, while preserving its original R4=1/R7=1 fixture.

The red VCD establishes a stable discarded proposal: before the seam,
row=2, line=3, row_next=3, row_new=0 and vsync_fire_seam_raw=1. At the seam,
row stays 2, line becomes 4 and adjustment becomes active; VSYNC is published
high afterward. This is not an inferred combinational glitch.

## Correction and verification

The old predicate combined prospective row_next with the live C9/R9 comparison.
R4's adjustment route can suppress the row commit while that live comparison
still matches. The candidate instead requires row_new and row_next=R7, using
the same transition that updates C4. Existing origin, midpoint, delayed parity
and direct R7-write paths retain their separate ownership. Type1 is unchanged.

The existing t16a owner now also checks that held C4 and the adjustment reset
remain quiet, then verifies a genuine next-frame C4=R7=3 still starts VSYNC.
The expected values follow the French counter/output rules, not a candidate
trace or the printed SHAKER expectation.

- Retained red: expected VSYNC low, actual 1, with counter assertions passing.
- `make -C sim crtc-test`: 240 passed, zero failures or unexpected passes.
- Unchanged soak stimulus/projection: 2,845,088 character samples; intended
  output change mints `0x57463f9d37d0251e` in the golden history.
- Final `python3 sim/select_tests.py --run`: PASS 44 benches, once after the
  final code edit. Source hashes and complete logs remain in the private proof.

Fresh Gemini4Argon high review20261104T193542Z-59624-bee9 accepts the frozen
three-file change. Parent independently verifies its SHA256 hashes against the
author freeze and current files; no later code edit or repeated gate. An
exact-source RBF/device check is still required before accepting this as a
SHAKER repair. Current 599 case25 reads 0037 versus 051B;
this focused proof does not assert that full caller number or establish its
absolute write phase. Earlier source audits confused case22 with passing
case24 and reused a historical 0003 result; those proposed causes are rejected.

Evidence: ignored `resumed-20261104/e6-r4-vsync-proof/` and
`resumed-20261104/e6-r4-vsync-fix/`. No broad baseline follows from this unit.

Technical information sourced from the Amstrad CPC CRTC Compendium by Longshot
(CC BY-NC-ND 4.0).
