# CRTC0 forced zero-R5 adjustment: source distinction — 4 November 2026

The French Compendium provides a stronger basis for the early-write E6
failures than the previously isolated C0=2 deactivation sentence. A focused
source review and parent inspection support failing counter vectors for the
R4/R9 distinction. Implementation is in progress; no new native fix is claimed.

## Source and scope

French ACCC1.11 §12.2 p95 and §11.2.2 p82 explicitly make the current line the
first additional line after an unequal R4 or R9 write at C0=1 of a true last
line. §13.2.4 p108 says this management survives C0=2 and ends when calculated
new C9 reaches R5. Its comparator explanation distinguishes C4=R4, where R9
still governs pending row completion, from C4≠R4, which selects R5. The
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
