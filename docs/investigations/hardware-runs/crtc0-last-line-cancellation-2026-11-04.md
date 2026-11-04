# Type0 cancelled Last Line: preserve the row step — 4 November 2026

An early R9 write can cancel Last Line, yet the core still reset the frame from
C4=R4 alone. The minimal candidate uses the existing combined Last Line latch
for frame reset. Its retained counter recipe fails before and passes after;
239CRTC cases,51focused neighbors and44selected benches pass on the first
candidate, but independent review found a C0=1 establishing-write regression.
The candidate is not accepted; broader C0<2 latch qualification is next.

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
(line_last_r AND row_last_r) fixes the ownership mismatch. No new state,
C0-specific exception, wrapper, Type1 or parity rule is introduced. Adjustment
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
- `make -C sim soak`:0xc7dd6686aaa44162, seed0xaccc5eed20260822,
  2845088characters/samples, unchanged stimulus and projection. The history
  top row records this intended behavior change.
- Final `python3 sim/select_tests.py --run`:PASS44benches, once after final code
  edit. Exact final line and hashes are in FINAL-REPORT.json; parent verified
  all three. No later code edit or repeated parent gate.

Fresh Opus review run20261104T151222Z-4821-b186 blocks this revision: the
wrapper samples Last Line only atC0=0. An R9 equality-establishing write atC0=1
previously reset the frame accidentally through row_last_r; requiring combined
Last Line exposes its missing re-evaluation. R4 establishing writes and
positive-R5 entry are pre-existing related gaps. French §12.2 pp94–95 and
§13.2.1 p108 require the C0=1 opportunity too. These need independent red cases
and a source-faithful C0<2 decision before this frame-end change is accepted.
The original case5 red remains valid; no gate is weakened to hide the gap.

After the corrected candidate has fresh review, Then build a full exact-source artifact and
repeat the same direct E6 input. Changes in reported numeric results remain
bounded to that caller history; no broad fresh baseline is implied.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
