# SHAKER E: positive-R5 re-entry and the `0080` residual — 4 November 2026

## Result and recommendation

Fresh CPCSyntaxError v0.12.0 prints `#0080` for E's “R5=3 / ON 2ND ADD LINE,
R5=1 / CPU TO NEW FRAME” row, matching the original-CPC photograph. MiSTer
`8dde1e7` and `bedb1c5` print `#0060`. The matching source exposes a useful,
narrow hypothesis: **a positive R5 at a genuine C4=R4/C9=R9 boundary may restart
C5 even when additional management is already active**. CPCSyntaxError does
that; MiSTer preserves C5. Applying that restart to the saved MiSTer caller
trace accounts exactly for the numerical difference.

This is a source-derived discriminator, not an established silicon rule or an
RTL fix. The saved CPCSyntaxError run has no internal C5/write trace, and its
earlier R5=0 re-entry already differs from author-confirmed behavior. Keep Q20
and its R5=0 continuity vector intact. The next useful experiment records C5
around the **positive** R4 re-entry while replaying the original E history;
it does not copy CPCSyntaxError's global release rule.

This pass read saved artifacts and source only. No emulator/API/device operation,
source change, retained test, build or gate was performed. The clean AmSpirit
reference was left untouched.

## Evidence identities and authority

- Matching source: `/Users/renaudg/code/CPCSyntaxError`, clean revision
  `22de765ef44da46c55217e4dd7caa385d69a6045` (`v0.12.0`).
- Parent's fresh headless canonical C-1 run: `--no-chain --no-errata
  --model cpc6128 --crtc 1`, 80 captures. Image
  `local/overnight-20261103/cpcse/c1-v012/png/CPCSE_1_02C9.png` was visually
  checked in this pass. It also prints the expected `0004`, `0B20`,
  `0073`, `0077`, `04A0`, `0B20`, `0223` rows.
- Hardware/photo comparison and executable/media identities are recorded in the
  [fresh CPCSyntaxError pilot](cpcsyntaxerror-v012-pilot-2026-11-04.md) and
  [E/P hardware report](shaker-ep-display-2026-11-03.md). The original E photo
  is also preserved in the existing private replay as `CE_02C9_A.png`.
- Inspected current RTL HEAD: `44ee14e71a5b5fa498e95327349fc2dcd6574f58`.
  Its relevant adjustment logic agrees with the saved `eng_new.v` logic used
  by the earlier production-T80 replay.
- Saved replay, untouched:
  `/Users/renaudg/code/Amstrad_MiSTer/local/investigations/shaker-sweep-2026-11-03/c02d3-replay/`.
  `new_EPS.log` contains actual accepted-write and VSYNC timings;
  `new_EPS3.log` explicitly records the `0060 (0080 expected)` text.
  This older private harness runs original E code on production T80/GA/CRTC,
  but starts it through a harness trampoline and seeded startup classification;
  it is not an original-boot SNA replay. No new execution was performed here.

French ACCC §11.3.2 p.87 was located with `scripts/accc/lookup.py`, read in the
saved PDF Inspector primary Markdown, and checked against the rendered French
page. The saved inspection report flags other pages, not p.87. It specifies
live `C5+1=R5` completion, overflow after lowering R5 below that target, and
retained additional management after R5 becomes zero. The
[author's Q20 answer](../../classic/accc-author-response-round2-2026-08-31.md#q20--n2--type-1-adjustment-after-r5-becomes-zero-1132)
confirms C4/R4 reset with adjustment and C5 counting preserved while R5=0.
Neither passage explicitly states what re-engagement does to C5 when R5 has
become **positive before the next R4 boundary**, but has not reached its C5
completion target. That is the remaining distinction; a matching emulator
row does not override the settled zero case.

## Actual caller and the prior state it inherits

E's original ordering calls `6F0A` before `70E5` (`6B6F`, then `6B72`). The
former includes R5=1 followed by R5=0 on the first additional line; it leaves
additional management active. Cleanup `6E40` writes R5=0, restores R7=30,
and returns through `889D`; it does not clear retained adjustment in MiSTer.
The `70E5` case then calls `7105` with HL=`0301`:

```text
7108 CALL A4AD       original PPI/VSYNC synchronization
710B LD DE,11A9
710E CALL A542       original calibrated delay
7111 LD BC,BC05
7114 OUT (C),C      select R5
7116 POP DE         D=3, E=1
7117 INC B
7118 OUT (C),D      R5 <- 3
711A LD HL,0
711D CALL A50C
7120 CALL A509      original NOP-delay tails
7123 OUT (C),E      R5 <- 1
7125 LD BC,BC07
7128 OUT (C),C      select R7
712A INC B
712B OUT (C),0      R7 <- 0
712D LD B,F5
712F INC HL
7130 CP (HL)
7131 CP (HL)
7132 IN A,(C)       PPI VSYNC bit 0
7134 RRA
7135 JR NC,712D
```

The loop takes 16 CPC microseconds per unsuccessful iteration. Thus the
printed difference `0x80-0x60=0x20` means 512 microseconds, **eight 64-microsecond
physical lines**, not one physical scanline. It is one differing text row.
CRTC status bit 5 and the display-origin repair do not directly enter this loop.

## Candidate first divergence from the saved phase

`new_EPS.log` supplies these events, in 64 MHz master-clock units:

| Event | Tick | C0 | C4 | C9 | Adjustment |
|---|---:|---:|---:|---:|---|
| Previous case: R5 <- 1 | 855637970 | 62 | 38 | 6 | inactive |
| Previous case: R5 <- 0 | 855642322 | 2 | 39 | 0 | active |
| Measured case: R5 <- 3 | 873889746 | 62 | 38 | 6 | active |
| Measured case: R5 <- 1 | 873898194 | 2 | 39 | 1 | active |
| Observed next-frame VSYNC | 873996363 | 0 | 0 | 0 | inactive |

The prior zero write is 135 clocks after its first additional line began at
855642187. Current RTL sets C5=0 on that entry and increments it on every
4096-clock physical line while adjustment remains active. No intervening
positive R5 write exists. Consequently the inferred C5 is 6 at the R5=3
write and 9 at the R5=1 write. The log does **not** expose C5 directly;
these values are derived from the logged entry, elapsed lines and inspected RTL.

The prospective positive re-entry is the C4=38/C9=7 row end at tick
873893963. Current C5 goes **7 -> 8** there; the current `frame_adj_CRTC1`
predicate excludes `in_adj`, so no fresh engagement resets it. One line later
R5=1 arrives with C5=9. Equality then needs 24 remaining line ends to reach
the exit after C5=0; the observed exit is exactly tick873996363.

CPCSyntaxError `src/core/crtc.cpp:1025–1103`, `updateVerticalType1()`, differs:

- At a genuine `r9Match && r4Match`, it enters the branch even when
  `verticalAdjustActive` is already true.
- Lines1045–1055 set the active flag, advance C4 when R5>0, set
  `adjustStateEngaged` from live R5, and reset `verticalAdjust=0`.
- Lines1066–1102 compare the live R5 limit and increment `verticalAdjust`.
  Its stored value during an additional line is one ahead of MiSTer's C5:
  the first additional line holds1, corresponding to C5=0. This representation
  difference alone is not a bug.

For the **same input trace**, that positive re-entry would normalize C5 to0
instead of8; the subsequent R5=1 write would land on normalized C5=1 instead
of9. Its completion needs32 remaining line ends instead of24: exactly eight
more lines, predicting tick874029131 and `#0080`. This is the minimal useful
counterfactual. It is not a claim that the fresh CPCSyntaxError execution
reached that exact tick or shared its preceding hidden state.

## Keep the forbidden global rule separate

The same CPCSyntaxError branch also overwrites `adjustStateEngaged=false`
when R5=0, then can call `newFrame()` immediately. That contradicts Q20's
retained-adjustment route. Its comment explicitly credits this re-entry with
its SHAKER CE result, but the comment cannot validate the whole branch.
MiSTer's existing `t08j` additionally pins a nonzero C5 phase across an R5=0
R4 reset. No change to that expected result is justified here.

The positive update is also not the still-open same-edge zero-release case:
the saved R5=3 write is on C9=6/C0=62, over one physical line before the
R4/R9 re-entry; R5=1 arrives at C0=2 of C9=1. It is not a 0->positive RFD
on C0=R0, and the 3->1 update is not an RFD trigger. CPCSyntaxError's generic
R5-write hook (`crtc_type_1.h:300–328`) adds only that RFD condition; the
candidate C5 restart occurs in vertical advancement, not an opcode exception.

## Minimal discriminator and exact unresolved question

The next private proof should preserve the original `6F0A -> 70E5/7105`
sequence and its original sync/delay helpers. Record accepted R5 writes,
C0/C4/C9/C5, adjustment, the R4/R9 event and raw/PPI VSYNC. Require the
hardware-backed text result `0080`; existing saved replay already demonstrates
the failing `0060`, but a fresh instrumented replay must first confirm this
specific C5 phase before testing a correction. Compare the first positive
R4 edge and the first later adjustment exit, not only final text.

A narrowly scoped experimental C5 restart could be evaluated there while
keeping the R5=0 Q20 control unchanged. Include an off-R4 positive-write
control so a general “positive write resets C5” shortcut cannot pass. Do not
turn the inferred internal C5=0 into a permanent hardware assertion until
trace evidence or clarification distinguishes this mechanism from a prior
history effect. No new retained test is proposed solely to mirror the source.

If the fresh traces cannot isolate this event, the precise author question is:

> On CRTC1 with R8=0, additional management remains active after R5=1 becomes0.
> C5 is still counting. Before a later C4=R4/C9=R9 row end, R5 becomes3 when
> current C5 is already6; then R5 becomes1 on C9=1 of C4=R4+1. Does the
> positive-R5 R4/R9 match restart C5 at0 while management remains active,
> or preserve the pre-existing C5 phase? How does that produce SHAKER CE's
> photographed `0080` rather than the preserved-phase `0060`?

The source comparison alone does not answer that narrowed question or justify
production RTL changes. A phase-aligned replay plus the already matching
hardware result may resolve it without contacting the author; the broader
same-edge R5=0-release question remains separate. The missing temporal input is CPCSyntaxError's actual C5/engagement trace
through the preceding zero case and this positive re-entry, aligned to the
accepted writes. The source comparison identifies where to measure it.

Private evidence is under `local/overnight-20261103/classic/e-0080-source/`:
`phase-arithmetic.json`, `evidence-log-slices.txt`, `caller-7105.asm`,
`source-identities.json`, and rendered French `fr-87.png`.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by
Longshot (CC BY-NC-ND 4.0).

## Private executable discriminator: actual C5 confirmed

A subsequent bounded private replay reproduced the unchanged `0060`, directly
observed the inferred C5 phase, and then obtained `0080` with a C5-only
positive-R5 re-entry counterfactual. This advances the evidence from phase
arithmetic to an executable mechanism; it does not uniquely identify silicon.
All files remain ignored under
`local/overnight-20261103/classic/e-0080-proof/`. Production RTL and retained
simulation sources were not edited; no selection gate, synthesis, emulator/API
or device operation was performed.

The replay uses a private `git archive` of
`3113323f24bb47dd18d51489d6c53a25793f158c`; relevant classic RTL is identical
to `8dde1e7`. The original `SHAKE27C.bin`, caller harness and observation top
were copied from the saved replay, without changing historical originals.
The production-T80 netlist is the preserved GHDL netlist from the BI unit.
The final observation additions expose C5 and record line-edge pre/post state,
bus-write onset, and actual R5 storage changes. They do not alter clocking,
WAIT, CPU instructions, RAM setup, PPI VSYNC input or production CRTC logic.
The inherited trampoline, seeded classification and direct PPI Port-B model
remain explicit limits: this is not a fresh-disk full-motherboard replay.

The unchanged replay first confirmed the target mismatch and C5=6/7/8/9
sequence. Only then was the private counterfactual evaluated. Its entire
behavioral difference (`private-counterfactual.diff`) is: while already in
adjustment, at a genuine positive-R5 C4=R4/C9=R9 line end, restart C5 at0
unless adjustment completion or the interlace additional-line state already
owns that edge. It does not clear adjustment or change C4/C9/parity logic.
In particular, R5=0 never enters this new branch. No PC, opcode, absolute tick
or special caller exception is present in the counterfactual.

### Measured transition and collateral state

The direct edge trace corrects the earlier VSYNC-derived boundary estimate
by one 64 MHz master clock: the actual counter update is tick873893962;
VSYNC observation follows at tick873996363 or874029131. R5 storage changes
occur one clock after the harness first observes bus-write onset.

| Observable | Unchanged | Private C5 restart |
|---|---|---|
| R5=3 stored at tick873889747 | C4=38, C9=6, C5=6, adjustment=1 | identical |
| R4/R9 edge at tick873893962 | C5=7 -> 8 | C5=7 -> 0 |
| Other state after that edge | C4=39, C9=0, adjustment=1, ParityFrame=1, ParityC9=1 | identical |
| R5=1 stored at tick873898195 | C4=39, C9=1, C5=9, adjustment=1 | C5=1; other fields identical |
| Actual adjustment-exit edge | tick873996362; pre C4=42/C9=0 | tick874029130; pre C4=43/C9=0 |
| State after exit | C4=C9=C5=0, adjustment=0, both parity states0 | identical |
| Raw VSYNC observed | tick873996363 | tick874029131 |
| Printed measurement | `0060 (0080 expected)` | `0080 (0080 expected)` |

The VSYNC shift is exactly32768 master clocks:512 microseconds/eight physical
lines. The changed pre-exit C4 is the expected consequence of counting eight
more lines, not collateral equality being silently assumed.

The **earliest divergence in the whole E execution** occurs earlier, at
tick36143178, during a positive-R5=1 scanner case: both start from C5=7;
unchanged goes to8 and the private variant to0. Both retain C4=39, C9=0,
adjustment=1, ParityFrame=1 and ParityC9=1. Thus the experiment is a general
positive-re-entry hypothesis, not an edit activated solely at the target row.
The target measurement later reaches identical pre-write state and accepted
write timings in both executions, as shown above. `comparison.json` preserves
both the earliest divergence and the target windows.

### Focused results and commands

```sh
bash local/overnight-20261103/classic/e-0080-proof/build.sh old
local/overnight-20261103/classic/e-0080-proof/obj-old/replay local/overnight-20261103/classic/e-0080-proof/SHAKE27C.bin 6b54 1 0 quiet
python3 local/overnight-20261103/classic/e-0080-proof/score.py old

bash local/overnight-20261103/classic/e-0080-proof/build.sh restart
local/overnight-20261103/classic/e-0080-proof/obj-restart/replay local/overnight-20261103/classic/e-0080-proof/SHAKE27C.bin 6b54 1 0 quiet
python3 local/overnight-20261103/classic/e-0080-proof/score.py restart
```

Both private Verilator compilations and replay processes exit0. Replay stdout
is retained as `old.log` and `restart.log`. The focused scorer independently
requires the photographed `0080`, not the expected-value text supplied by the
title. Exact scorer results:

- Unchanged: **`e-0080-proof: FAIL`, exit1**, observed96 versus expected128.
- Private restart: **`e-0080-proof: PASS`, exit0**, observed128.
- Both retain all seven other E measurements: `0004`, `0B20`, `0073`, `0077`,
  `04A0`, `0B20`, `0223`.
- Both pass515 actual R5=0 R4-reset events: C4 resets0, adjustment stays
  active and C5 advances modulo32, including nonzero phases.
- All off-R4 positive-R5 line edges preserve ordinary C5 increment:
  81 baseline checks and89 private-restart checks, zero failures. The extra
  eight checks correspond to the intentionally extended measurement.

`score-old.log`, `score-restart.log`, their JSON results, build logs,
`private-counterfactual.diff`, `comparison.json` and `provenance.json` retain
commands, exits, source SHA and file/log hashes. The earlier `old-bus-only.log`
is an initial observation pass; the final `old.log` additionally records actual
register-storage edges and the complete positive-line control set.

### What this now resolves

The preserved-phase explanation and the proposed positive-re-entry C5 restart
are now measured, not merely arithmetic guesses. A C5-only restart is
sufficient to match the hardware number while retaining the tested Q20 and
other E controls. This supplies a concrete failure-first candidate for a
future retained cross-module test and production review.

It still does not prove that original hardware uses this exact hidden-state
mechanism, nor validate CPCSyntaxError's global R5=0 release. Its internal
execution trace remains absent, and the replay's inherited setup remains
synthetic. A future narrowly reviewed implementation can be evaluated against
this discriminator and the original E hardware result without waiting on a
broad author question; a new hardware result or a different authentic history
that distinguishes mechanisms would carry more weight than final-number
agreement alone. No production repair is included in this unit.
