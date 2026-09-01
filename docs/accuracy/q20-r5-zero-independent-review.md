# Q20 type-1 R5=0 adjustment correction — independent review

**Date:** 2026-09-01
**Branch:** `accuracy/hybrid-live-blanking`
**Reviewed baseline:** `2e1eb72672ea097536b6710f8737d6e65b01a988`

## Evidence boundary

The behavioral premise comes from the CRTC Compendium author's direct 2026-08-31 response to
round-2 question 20, recorded in `accc-author-response-round2-2026-08-31.md`. It is dated
correspondence evidence, not a new ACCC edition and not a hardware observation. Published ACCC
v1.11 and its hashes are unchanged. ParityC9 behavior on this non-frame C4 reset remains a
separate source/hardware evidence gap.

## Failure-first evidence

The first `t08j` amendment changed only the expected C4 result at the next reachable C4=R4
row end while R5 remained zero. Unchanged RTL produced C4=`0x0b` instead of `0x00`; the other
182 classic vectors passed. The first row-only RTL correction then passed that counter check.

A fresh native Sol review found three downstream seams that the initial discriminator did not
protect:

- the new C4=0 transition did not trigger the type-1 R12/R13 row-0 reload;
- VSYNC still compared the hypothetical adjustment `row+1` instead of actual `row_next=0`;
- C5 happened to be zero at the original boundary, so an accidental C5 reset could pass.

The revised test-only pass moved the reset to C5=8, changed the address base, and added positive
R7=0 plus negative R7=R4+1 cases. Before the downstream RTL correction, the three independent
failures were: saved VMA' `0x2209` instead of the R12/R13-derived scan address, missing R7=0
VSYNC, and spurious R7=R4+1 VSYNC. No assertion was weakened to obtain the final pass.

## Final implementation boundary

`crtc1_stuck_r5_row_reset` resets only `row_next`; it does not assert `row_frame_last`,
`frame_new`, or adjustment end. The rollover-effective R5 suppresses this route on a same-edge
zero-to-positive RFD write. A line-boundary event from the same condition drives the type-1
row-0 R12/R13 reload, and the VSYNC comparator selects actual `row_next` at that boundary.
Positive-R5 exit, C5 continuity, F14 additional-line behavior, RFD, VMA save, and live
`CRTC_TYPE` behavior retain their existing paths and regression coverage.

## Independent review and verification

Native Sol re-review found the downstream issues above; they were remediated failure-first.
The final uncommitted diff then received guarded Gemini scan review:

- run `20260901T034537Z-644-15153`, model `gemini-3.7-flash-medium`;
- verdict: RTL and testbench **CLEAR**, with only shared-document handoff updates;
- checkout audit: branch and HEAD unchanged, no changed files, no scope violations;
- provider cleanup: clean.

Final local gates:

| Gate | Result |
|---|---|
| `make -C sim test` | PASS — 185 classic vectors plus every integrated Plus, GA, and u765 suite |
| `make -C sim lint` | PASS — existing non-fatal warnings only |
| `make -C sim soak SOAK_EXPECT=0x9d8cd95357d1d752` | PASS twice — fixed seed, 2,845,088 CLKEN samples |
| `git diff --check` | PASS |

## Integration handoff

Shared files remain coordinator-owned. Stream finish must update the current classic-vector
count from 183 to 185 and replace the previous soak hash `0xd6bc1649ff2058a1` with
`0x9d8cd95357d1d752`, carrying the Q20 row reset, R12/R13 reload, and actual-row VSYNC rationale.

**Verdict: CLEAR for commit and READY handoff. Hardware confirmation remains open.**
