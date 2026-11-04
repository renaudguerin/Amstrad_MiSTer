# CRTC0 R4 switch: VSYNC must follow the committed row — 4 November 2026

A focused counter/output proof exposes a definite defect independently of the
SHAKER caller's final number. An R4 write selects vertical adjustment and holds
C4, but the VSYNC engine compares a proposed next row that is never entered.
Fix `9a91920` uses the canonical row transition. The exact-source native E6
run repairs case 25 to 051B; eleven of 28 printed checks still fail.

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
exact-source RBF/device check now qualifies the targeted SHAKER repair.
The full [workflow 41900154684](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41900154684)
passes all jobs. Artifact 13359402650, `Amstrad-build-360-1-full`, contains
`Amstrad_20261104_9a91920.rbf` (3,145,108 bytes), SHA256
`eadc739cb8809236df65866ad36f99b9b6ba1dd04c22e71d507318e53d0c5576`.
Quartus 17.0.2.602 full compilation reports seven setup/hold groups, zero TNS,
minimum setup/hold 0.585/0.242 ns and no errors or critical warnings. The
installed MiSTer file has the same hash.

## Native E6 acceptance and corrected reading

The same direct CRTC0 E6 recipe completes 16 commands, four captures, zero
capture failures and nine SSM records on the installed author disk
6b9ff106…eb61, Full sync filter. Parent inspected every PNG and the enlarged
before/after numerical panel. Compared with 599:

| Case | 599 | 9a91920 | Printed expectation |
|---|---|---|---|
|19|0003|002B|00A3|
|25|0003|051B|051B|

Only these two numerical rows change. Pages 0407 and 040A are pixel-identical;
5/14/21/24 retain 00C0/0D97/0066/000E. Eleven of 28 unique printed checks fail:
2, 4, 6, 8, 9, 10, 16, 17, 19, 27 and 28. This is a program self-check tally,
not an original-CPC photo comparison or a full CRTC0 compliance percentage.

The earlier parent reading of case 25 as 0037 was wrong: the original 599
PNG says 0003. A one-off decoder now matches eight-pixel glyphs learned from
immutable case labels, fixed text and source-compiled EXP fields, independently
of the measured actual values. Its complete old/new ledger is retained as
`c0-row-vsync-build/e6-decoded-tally.json`. The initial source audit correctly
read 0003; its causal claim still failed to account for C4's row_new gating.
The case22/case24 naming confusion is separate. The accepted defect is the
stable discarded VSYNC proposal demonstrated by the counter/output proof,
not a trajectory inferred from one printed scalar.

The same artifact's bounded CRTC0 DSC4 control completes19commands/fourcaptures/
zero failures/zeroSSM records. Root inspected all four: credit colons/names
and sampled main/static/rotating/fire-band views remain visible. This does not
close the separate mixed-mode partial-pixel defect or prove full-demo fidelity.

Evidence: ignored `resumed-20261104/e6-r4-vsync-proof/` and
`resumed-20261104/e6-r4-vsync-fix/` and
`resumed-20261104/c0-row-vsync-build/`. No broad baseline follows from this unit.

Technical information sourced from the Amstrad CPC CRTC Compendium by Longshot
(CC BY-NC-ND 4.0).
