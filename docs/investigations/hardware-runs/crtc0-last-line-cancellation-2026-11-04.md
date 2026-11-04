# Type0 cancelled Last Line: preserve the row step — 4 November 2026

An early R9 write can cancel Last Line, yet the core still reset the frame from
C4=R4 alone. The final correction uses the combined Last Line latch for frame reset and
admits C0=1 validation into existing shared latches, using the current line's
IVM comparison. Independent review accepts it;239CRTC,51neighbors and44selected
benches pass. A full exact-source RBF and targeted E6 hardware check are next.
No new flip-flop or Type1 rule is introduced.

## Source-derived failure

French ACCC1.11 §12.2 pp94–95 allows R4/R9 changes atC0=0 to cancel Last Line.
A later equality restore afterC0<2 does not re-arm that decision. A C0=1 break
has the separate additional-line rule and is preserved. French §10.3.1.2 p77
explicitly requires ordinary counting when Last Line is false: if liveC9=R9
at the line end, C4 increments. Parent and worker read the rendered French
pages; neither expectation comes from a candidate simulator or printed number.

At C4=R4=38,C9=R9=7,C0=0, R0=63,R5=0,R8=0, writeR9=6 then restore7 atC0=5.
The seam holds line_last_r=0,row_last_r=1. Live equality returns, so RA resets0
but **C4 must increment39**, with adjustment inactive. Pre-fix ee9c131 gives
C4=0/RA0. An RA-only assertion would miss this defect.

The live rollover already used the combined decision. The separate
`crtc0_row_frame_last` used row_last_r alone, mistaking C4/R4 equality for
Last Line. Replacing that input with existing `type0_last_line_armed`
(line_last_r AND row_last_r) fixes the ownership mismatch. This frame-end consumer change adds no state. The subsequent C0=1
qualification below reuses the existing wrapper latches; Type1 is untouched. Adjustment
completion through in_adj and the accepted pending-R9 comparator route remain.
Genuinely true Last Line stays sticky through later R4/R9 writes, as p95 requires.

## SHAKER and CPCSyntaxError evidence bounds

Current SHAKE27E case5 routine5CC8 writes6 at5CE4, INC C then writes7 at5CE7;
the two OUT(C) acceptances are five CPC microseconds apart. It setsR7=45 and
polls5CFB fromHL0 with wrap timeout; expectedC0 is at5D0E. Seventy-five routine
bytes are checked against the pinned payload1a747864…bca1. Absolute first
acceptance atC0=0 is still the caller's label, not a traced production-CPU fact.
The counter proof specifies that bus recipe independently, without a release fit.

Fresh matching-version CPCSyntaxError0.12 directE6 prints00C0, while native
before-capture prints0000. Its source22de765 cancels lastFrameLine atC0=0,
keeps it false atC0=1 and ignores the restore at5; its frame-end consumer then
increments39. This path agrees with the French rule. Most E6 cases fail in
CPCSE too; this match does not validate its complete engine. Original-CPC E
photographs are unavailable; SHAKER's printed expected is corroboration, not
a substitute hardware oracle. The proof does not assert the full numeric00C0.

## Retained proof and verification

The existing `test_type0_c0_r9_write_immediate_clears_last_line` owner gains the
full38/7/63 transient-write follow-through. Frozen ee9 fails C4expected39/actual0
before production edits; the same retained extension passes on the candidate.
No duplicate owner is registered. Private red, green, VCD, exact author bytes,
source renders, final diff/hashes and handoff are under ignored
`resumed-20261104/e6-case5-proof/`.

- `make -C sim crtc-test`:239passed, zero failed.
- Focused neighbors:51passed, including opposite row/line decisions, late true
  Last Line writes, both C0=1 entries, R5/IVM/VMA/R0 and accepted ee9 cases.
- Corrected C0<2 candidate `make -C sim soak`:0xfe95fe98619b6cfa, seed0xaccc5eed20260822,
  2845088characters/samples, unchanged stimulus and projection. The history
  top row records this intended behavior change.
- Final `python3 sim/select_tests.py --run`:PASS44benches, once after final code
  edit. Exact final line and hashes are in FINAL-REPORT.json; parent verified
  the four frozen files. This gate belongs to the revision before the IVM
  regression below; it did not validate the final IVM correction.

Fresh Opus review run20261104T151222Z-4821-b186 blocks this revision: the
wrapper samples Last Line only atC0=0. An R9 equality-establishing write atC0=1
previously reset the frame accidentally through row_last_r; requiring combined
Last Line exposes its missing re-evaluation. R4 establishing writes and
positive-R5 entry are pre-existing related gaps. French §12.2 pp94–95 and
§13.2.1 p108 require the C0=1 opportunity too. The review required independent red cases and a source-faithful C0<2
decision; the corrected proof follows below.
The original case5 red remains valid; no gate is weakened to hide the gap.

The C0<2 correction adds nine independent fail-before cases: R4/R9
establishment at C0=1 with R5=0/2, nonzero case13 counters, R0=1 same-edge
validation/default adjustment, and cancellation restored at1 versus5. It
passes239CRTC/51neighbors/two live-type priority controls and the selected44.
Fresh Opus review20261104T160353Z-14286-8d95 accepts that correction but names
an IVM residual. The parent verifies its four source hashes independently;
the review's hash command was denied. Restoring atC0=1 is an interpretation
of Frenchp94, not a measured hardware rule.

## Source-backed interlace regression found after review

French§19.8.1p220 requires the line where R8 becomes3 to compare rawC9;
doubling begins on the next line. With R0=63,R4=0,R9=6,R5=0,R6=25, even
frame/raster parity and rawC9=3, writeR8=3 duringC0=1 before its CLKEN. The
line must advance rawC9 to4; the next doubled display givesRA8. A private
bite-test givesRA8 on acceptedee9 andRA0 on the frozen C0<2 candidate.
The latter reuses the next-seam expression, reads liveR8 too early, arms
Last Line and incorrectly resets the frame.

This is a definite regression, rather than the review's separate unpinned
odd-R9 first-doubled-line question. The final correction uses the existing **current-line** IVM/frozen-exit
interpretation plus effective incoming R4/R9. Four existing owners fail before
and pass after: entry, exit, steady incoming odd-R9 target and same-edge R8
frame-parity forwarding. Two steady-IVM establishing controls remain green.
Parent visually read the primary French207/220/221 renders, including the
R9=7 even-row0,2,4,6,8 example and switch/exit rules. No RBF yet contains it. Primary renders, red/green proofs and review deltas are
under `e6-case5-proof/review3/`; the earlier gate/review remains evidence for
its own revision only.

Final review20261104T163812Z-22111-e809 accepts the complete four-file diff;
parent verifies its hashes against review3/FINAL-REPORT.json. Final
`python3 sim/select_tests.py --run` passes44benches once after the last code
edit. CRTC239/51neighbors pass; soak recomputesfe95fe98619b6cfa unchanged with
the same2845088samples. The directed IVM reds establish the distinction the
random stimulus does not exercise. No parent gate repeat or later code edit.

Nonblocking source residual: on the first doubled line after R8 entry with
oddR9, C0=0 may use prior toggle parity while C0=1 uses current seeded parity.
The latter can now admit positiveR5 adjustment where HEAD reset the frame.
Smallest unpinned recipe: enterR8=3 on the preceding line, R9=7, even frame,
C4=R4, rawC9=4/VMA8,R5=2. It is recorded without a new expected value. Frozen
IVM-exit re-entry at the exact C0=1 clock and late restore-at1 retain their
stated inference bounds. E6case16 is unchanged.

Build an exact-source artifact and repeat direct E6 plus named regression controls. No broad fresh
baseline is implied.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
