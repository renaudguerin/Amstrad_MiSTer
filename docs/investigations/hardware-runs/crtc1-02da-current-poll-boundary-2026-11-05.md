# CRTC1 O/02DA: current failure and first-poll qualification

The current experimental90f3b18 RBF still prints0050 instead of0060 in all
four R7=0 rows of original SHAKER C page02DA. The other eight rows retain
3C20/1E20. AmSpirit1.17.1/core2491682 running the same original normal-boot O
sequence gives the expected twelve values. The recent IVM adjustment repair
therefore did not close this residual. No new timing rule or RTL edit follows
from the scalar alone.

## Original caller and bounded native run

This is direct normal-boot O, traversing D4/D5/D6/D8/D9 before DA. It is not
direct PC entry, and equivalence to the full canonical C-module history is
unproved. Native Full-filter CRTC1 run completed19commands/6captures/zero
failures/10SSM records. Root inspected all six PNGs. The disk SHA256 is
6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61;
RBF SHA256 is a7c3b0ad177744a3c2e2120365c3febdbd8226f0f80782603ccb1e3e8008a102.
Private receipts are under
`local/overnight-20261103/resumed-20261104/ga-native-build/device-c1-direct-o-90f3b18/`.
The original O dispatch, literal table and poll formula are source-qualified
in the adjacent private`c1-02da-source/REPORT.md`; keyT selects a different test.

## AmSpirit first R7=0 row

A second normal boot used the same original sequence, with execution
breakpoints only. The first DA acquisition has IY57FC; the third has IY5800,
identifying the first original R7=0 row. ROM language is FR, model6128,
CRTC1,128KiB. The settled configuration after script reset reports
freq_screen1. No guest RAM, PC or hidden-counter writes were used. Original
5575..558D instructions and57FC..5813 expectation table were byte-guarded
against SHAKE27C body SHA256
3647e120e282412d5afc31bed0e4b6673beed850ba94ec7a4d5374ce04a4682f.

| Stop | Emulator tick (µs) | Observation |
|---|---:|---|
| Third acquired row,554F |42156766|IY5800; R8=0,R9=7,R4=38,R5=0,R7=30|
| R7 OUT before,5575 |42156818|R8=3,R9=7,R4=77,R5=0,R7=30|
| R7 OUT completed,5577 |42156822|R7=0|
| First IN completed,5581 |42156845|A=1E,PB0=0,HL=0004|
| Second IN completed,5581 |42156861|A=1F,PB0=1,HL=0005|
| Poll exit before doubling,5588 |42156871|HL=0006|

The second5581 stop is a distinct occurrence,16µs after the first; breakpoint
replacement did not silently reuse the prior stop. Actual HL4→5→6 proves
two polls without a modular-wrap ambiguity in this reference row. The caller
then prints6×16=0060. These are instruction-completion observations, not
measurements of the physical PPI sampling aperture or hidden CRTC phase.

Private evidence:`amspirit-c1-02da-poll-probe/run-001/` retains all write
before/after stops, operations, immutable source checks, original boot/tail
scripts and final cleanup. `amspirit-c1-direct-o-current/run-001/` retains the
complete twelve-row reference screenshot. Its initial post-run state dump
failed because the destination subdirectory was absent; the original script
had completed. Root created the directory and recovered the same paused state
without restarting or changing guest execution. The error and recovery remain
recorded. Script and breakpoint sets are cleared, emulator paused.

## Matching CPCSyntaxError and a concrete source omission

A fresh isolated CPCSyntaxErrorv0.12.0 original-O batch also completes all six
pages and gives all twelve expected02DA values. Binary/source identity is the
same pinned pair as the [B6 experiment](crtc1-b6-cpcse-ordering-2026-11-05.md);
this run uses packaged UK firmware, `--no-chain --no-errata`. Root viewed all
six pages and full DA. Private`cpcse-02da-current/` preserves script identity,
logs, original BMPs, PNG copies and hashes. Its first R7=0 trace is frame1931,
C0/C4/C9=2/0/2 at PC5577; this is its character-level dispatch, not measured
native hidden phase.

Matching source`src/core/crtc.cpp:759–817` applies active MID-VSYNC to a live
matching R7 write before the midpoint, and uses live frame parity for the
write rather than frame-origin parity. Inspection of our current wrapper
finds a concrete bypass: the live equal-R7 path sets logical VSYNC immediately,
independent of `vsync_mid`; the existing midpoint arm is populated only by the
line-seam path. French ACCC§19.7.1–2p219, re-read in the position-aware layer
and rendered page, says active MID-VSYNC triggers at C0=R0/2, with even parity
including R7=0. This gives an independently documented before-midpoint rule;
it does not select a new numeric delay or an after-midpoint policy.

A focused failing vector and smallest pending-consumption design are being
prepared before implementation. Preserve equal-write consumption, active-pulse
protection, parity/origin ordering and reset/SNA/type lifecycle. The separate
non-IVM B6 admission and optical9/5 questions are not explained by this rule.

## Current-T80 discriminator

Observe the same first R7=0 row using current production T80/CRTC sequencing:
R8/R9/R4/R7 writes, C0/C4/C9/C5 and adjustment, both parities, logical and
published VSYNC, and the actual first PPI sample. Preserve authentic entry and
source identity. The existing source audit already finds incoming parity at
R7=0 origin and parity-aware IVM row counting implemented; do not duplicate
those rules or move a VSYNC edge merely to make0060 appear. A native0050
print alone still does not prove the first poll count or an exact16µs early edge.
