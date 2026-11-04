# CRTC0 forced zero-R5 adjustment: source distinction — 4 November 2026

The French Compendium provides a stronger basis for the early-write E6
failures than the previously isolated C0=2 deactivation sentence. A focused
source review and parent inspection support failing counter vectors for the
R4/R9 distinction. A candidate now passes its focused and selected gates and has fresh
independent review. No new native fix is claimed.

## Source and scope

French ACCC1.11 §12.2 p95 and §11.2.2 p82 explicitly make the current line the
first additional line after an unequal R4 or R9 write at C0=1 of a true last
line. §13.2.4 p108 says this management survives C0=2 and ends when calculated
new C9 reaches R5 under the selected comparator. Its comparator explanation
distinguishes C4=R4, where R9 still governs pending row completion, from
C4≠R4, which selects R5. The
worked R9/R4 examples in §11.2.2 p83 corroborate that ownership distinction.

The one-line zero-R5 example in §13.2.1 p106 explicitly uses R0=1. It does
not establish a universal one-extra-line exit for a forced C0=1 break with
R0=63. §13.7.2.2 p128 separately demonstrates zero-R5 counting through31
in a widening-R0 case; it corroborates the wrap mechanism without being the
same stimulus. Parent rendered/read French pp108/128 and reread pp82/83/95/106.

The standalone proposed vector starts from R0=63,R8=0,R5=0,R4=C4=8,
R9=C9=0 at C0=0. At C0=1:

| Write | Required counter progression |
|---|---|
|R9←2|C4 stays8 through C9=0,1,2; calculated C9=0 reaches R5 and produces frame origin on line3|
|R4←10|C4 stays8 through C9=0..31; zero-R5 wrap produces frame origin on line32|

These are counter expectations from the combined French rules, not values
read out of a candidate simulator. Current9a instead exits after two inclusive
lines in both paths. The current source ties the forced-entry path to the
short-R0 zero-adjustment flag and suppresses its pending R9 comparator.
The retained red proof must establish the actual mismatch before an RTL edit.

## Reference and numerical bounds

Matched original E6 in AmSpirit1.17.1 passes all28 printed checks. Native9a
still gives16=002B versus002F,17=0013 versus008B and19=002B versus00A3.
A full32-line adjustment rather than the current two inclusive lines explains
the30-line/120-poll difference for17/19 without retaining the old R4 value.
This is a consistent source-based explanation; final scalars alone do not
observe the hidden trajectory or establish the caller's absolute bus phase.

The provisional Gemini3 claim that008B/00A3 are author calculation errors is
rejected. Its baseR4−C4 arithmetic is not evidence of the author's formula,
and its two proposed trajectories do not exhaust the possibilities. Its useful
recipe/media retrieval remains qualified separately. CPCSyntaxError's E6
fails17=0017,19=0003 and20=0027; it supplies no validated alternative here.

Fresh Gemini4Argon high source review20261104T204402Z-73922-7226 and the
parent's independent primary-page reading support this narrowly defined work.
Its exact full-caller phase table is conditional, not a measured native phase.
The post-entry R4=C4 restoration question remains separate: these rules do not
settle whether that later re-match changes an already selected comparator.
Do not broaden this fix to case28 by analogy.

Private source review/renders: `resumed-20261104/c0-forced-zero-adjust-read/`.
Original-program reference and numeric ledgers are linked from the
[canonical-row VSYNC report](crtc0-r4-switch-vsync-2026-11-04.md).

Technical information sourced from the Amstrad CPC CRTC Compendium by Longshot
(CC BY-NC-ND4.0).

## Completion is conditional on comparator ownership

The specific rule in §11.2.2 p83 and §13.2.4 p108 also governs completion.
While pending C4=R4, a natural C9 overflow does not independently activate an
R5 endpoint. With incoming C9=R9=7 and a C0=1 write R9←6, C9 traverses7..31,
then0..6, with C4 held; origin occurs on line32. Likewise a positive R5 target
between the current C9 and pending R9 does not end that R9 phase early.
After the R9 match advances C4, calculated C9=0 may complete R5=0 before an
extra row is displayed. Read the general calculated-C9 wording under this
specific comparator rule, rather than as a second always-active endpoint.

The provisional line25 natural-wrap expectation and generic-end attempt are
withdrawn. Their red logs are preserved as a rejected reading, not an RTL
finding. Fresh Opus source adjudication20261104T212148Z-81631-1f7e supports
restoring the reviewed candidate and keeping the positive-R5 owner unchanged.
The correct line32 counter/VSYNC control is retained in existing t16n; it
protects the distinction against the erroneous generic refactor just tried.

## Terminal R4 selection and frozen candidate

A forced R4 break has already selected R5 on the current additional line.
It can therefore be terminal immediately: natural C4=R4=8, R0=63, R8=0,
R9=C9=31 and R5=0, followed by R4←10 at C0=1, must reach origin on the first
boundary. The positive case R9=C9=3/R5=4 has the same endpoint. Private
source-derived proofs fail both cases on the initial candidate, before this
terminal fix. Retained t16m then fails with expected C4=0 versus actual8.
These are selected-R5 endpoints; they do not broaden pending-R9 completion.

The final candidate separates forced entry from the unmodified short-R0
flag, retains pending R9 ownership, and uses existing R5 override storage for
the forced current line's selected target. Accepted C0≤2 R5 writes can replace
that target; a C0=3 write cannot replace it for this line. Effective ownership
includes the accepting write when R0=1 coincides with rollover. Terminal
completion feeds the canonical counter/frame decisions and retains added-line
priority. No wrapper, new flop or post-entry R4-rematch rule is introduced.

Extended existing counter owners cover zero/positive terminal entry, accepted
and late R5 targets, real versus absent VSYNC, short-R0 follow-through and
coincident R9 ownership. An obsolete assertion of the chosen private override
storage was removed; it was not changed to mirror the new storage value.
Independent public target/origin/window and lifecycle assertions remain.

Frozen final evidence: CRTC240/240, selected45 benches PASS once after the
last source edit, intentional soakf9b5f86e16277c55 with unchanged stimulus and
2,845,088 samples. Fresh Gemini4Argon high review20261104T215343Z-88790-cc7b accepts the frozen
terminal delta with no findings. Parent rechecked all three final hashes; no
source edit or gate rerun followed acceptance. Prior Opus acceptance covered
the initial candidate only. Native caller phase,
new printed E6 results and synthesis/timing are separate remaining gates.
Private `c0-forced-zero-adjust-fix/r4-terminal-fix/` retains the red, full and
incremental diffs, hashes, review brief, gate log and handoff.
