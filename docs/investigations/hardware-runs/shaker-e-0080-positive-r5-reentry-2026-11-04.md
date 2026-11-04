# SHAKER E: positive-R5 re-entry and the `0080` residual — 4 November 2026

## Result and recommendation

The original-CPC E photograph requires0080; pre-fix MiSTer prints0060 and
599ca80 now gives0080 on native direct and canonical-history paths.
A bounded positive-R5 R4/R9 re-entry restart is now justified as an abstract
output rule, and its production implementation has passed the final gate and
code review, and is committed and pushed as
`599ca8098c655556457a6066b2e4fdeed2d8a9a7`. It is not a claim to identify a
physical C5 reset inside the chip.

The initial source comparison and private restart showed sufficiency only.
New original-caller AmSpirit/T80 timing excludes the extra acquisition frame
in their compared interval, a prior positive completion establishes a common
origin under the documented rule, and exact zero-R4-reset-only algebra predicts
0020 rather than 0080. French §11.3.2 p87 independently requires retained
management at R5=0. Fresh Opus review now says PROCEED with the narrow positive
restart while preserving the zero route, completion and added-line owners.
The final sections retain the exact evidence and inference bounds. Exact push
CI [41874429612](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41874429612)
passed all required jobs. Full build
[41878239893](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41878239893)
and targeted native E/P/DSC4 checks pass. The separate S history defect remains
open; exact results and bounds are in the native acceptance section below.

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

## Independent review and gap audit: discriminator still required

The frozen Opus review (`e-0080-proof/opus-review.log`) returns
**NEED-DISCRIMINATOR**. Production remains unchanged. The private restart
proves sufficiency within this replay; it does not select a hardware rule.
In particular, **CPCSyntaxError's matching0080 is not evidence that its
positive re-entry-while-already-active branch caused that result**: its source
releases adjustment at the earlier R5=0 R4/R9 match, so the later case can
start through ordinary first engagement. The mapped tick858558539 belongs
to the MiSTer replay; no timestamped internal CPCSyntaxError trace was captured.

The review identifies four classes consistent with the final number: the
private positive-only restart; an earlier adjustment release; C5 reset at
every R4/R9 edge while management stays active; or one extra frame between
cases while C5 preserves phase. The other seven numeric controls do not
separate those classes. Nor does an identical parity field prove that a
hardware re-entry would have no other side effects. Exit/restart priority,
interlace exclusions and entry side effects remain unestablished by this case.
The reviewer could not open the original photo and relied on this report's
provenance; no independent reviewer photo inspection is claimed.

### Why the inter-case frame-count explanation remains open

The original code gives part of the gap a fixed count:

- `6F3F` records the result, then calls cleanup `6E40`.
- `6E40` writes R5=0 and restores R7=30, then jumps to889D.
- `889D` calls `A4E4` three times. Each call waits for VSYNC to become
  inactive, then active. This is three subsequent VSYNC starts.
- After returning, the original character blitter prints the result line.
  The caller then reaches `70E5/7105`, whose `7108 CALL A4AD` reacquires C0vs.

But **A4AD is not a fixed-count frame wait**. It waits on/off/on, measures the
second pulse, then at `A4CF/A4D1` repeatedly samples PPI VSYNC. An unsuccessful
sample calls the delay with DE=`4DED` (=19949), then loops. The instruction
and call overhead is20 microseconds: the retry period is19969 microseconds,
one microsecond longer than a312-line frame. The final successful sample
then takes a separate near-frame delay using DE=`4DF5` (=19957).

French ACCC §7.2 p.39 explicitly describes this frame-by-frame one-microsecond
drift. Its timing table and wait routine on p.41 were rendered and read for
this audit (`gap-audit/fr-41.png`). They agree with the original assembly;
the cleanup's fixed three edges do not remove the adaptive acquisition.
Thus a one-microsecond difference in starting sample phase can change the
retry count by a frame, even when the preceding print finishes comfortably
before the first acquisition edge. This is a possible mechanism, not a claim
that such a discrepancy has been observed on hardware.

The saved log locates cleanup R5=0 at858565970 and R7=30 at858566866.
The three following VSYNC starts are859541579,860819531,862097483. Its
previous-result `TXT` marker falls between the862097483 and863375435 edges,
but has no timestamp. Later raw VSYNC edges alone cannot identify which pulse
was caught by an `A4D1` sample and which followed the final near-frame delay.
The target R5 write at873889747 therefore does not reveal the acquisition
iteration count or margin retrospectively.

The exact missing observables are timestamps for `7105/A4AD` entry, the
accepted `A4D1` PPI reads and returned bit0, the successful acquisition branch,
and the print return. Corresponding real-machine timing or a deliberately
controlled extra-frame discriminator is also absent. This read-only unit did
not run another CPU replay or emulator. **Gap class C is not ruled out.**
The arithmetic remains `312 mod32=24`, so an extra frame changes C5=9 to1
modulo32 and can account for0080 under the preserved-phase model.

### Q20 raw-wording boundary and follow-up

The dated Q20 note identifies the original attachment as `message.txt`, SHA256
`278ad89000b9768c965b432157d83e6523259901fa7b3414504ce7d7d65d0677`.
Neither this unit nor the parent could locate that original attachment in the
supplied evidence. The available note and the current asked-question entry
are repository syntheses, not the raw sent/received exchange. They support
retaining the existing Q20 rule, but **the exact author's wording about C5
phase on the reset edge could not be reverified**. No claim is made that the
author explicitly selected uninterrupted C5 phase at that instant, or
contradicted a possible C5 reset while adjustment stays active.

Existing production rules and historical notes were not rewritten. The
[French follow-up draft](../../classic/accc-author-questions-c5-2026-11-04-fr.md),
**not sent**, asks separately about C5 at a zero-R5 R4 reset, positive-R5
re-engagement while management remains active, possible prior exit, and
invariance under0–3 deliberately added inter-case frames. It retains the
original E case history rather than treating the private C5 reset as a golden
rule. The compact audit and source slice are in
`local/overnight-20261103/classic/e-0080-proof/gap-audit/`.


## Actual CPCSyntaxError C5 trace

A private source build of clean v0.12.0 now observes the path directly rather
than inferring it from a final number. Instrumentation adds logging and PC/cycle
read callbacks only; the parent inspected its diff. The unchanged full-history
C-1 script completes80captures, exit0. Target02C9's BMP is byte-identical to the
supplied binary's earlier capture, SHA256
`7849374e8e0bcb5db0436f81f7ca0cabcbc9fca2398fabeecc8cacbdcaf128fe`.
Build/run commands, logs and the instrumentation diff are preserved in
`cpcse/e-c5-instrumented/`; no installed/source checkout or tracked emulator code
was changed. The guarded Gemini3.8-high run completed successfully.

The trace confirms **earlier zero-R5 release**, not positive re-entry while
already active:

| Event | Machine-cycle timestamp | Relevant state |
|---|---:|---|
| Earlier R4 match with R5=0 | 542098676 | C4=38, C5=0, adjustment active; commit clears `adjustStateEngaged`, then exits to `newFrame()` |
| Later target R5←3 | 542977008 | PC711A, C0/C4/C9=62/38/6; adjustment and engagement both inactive |
| Ordinary first engagement | 542977268 | R5=3, C5 reset0; adjustment and engagement become active |
| Target R5←1 | 542977536 | PC7125, C0/C4/C9=2/39/1, C5 already2 |
| Eventual adjustment exit | 542985716 | C5 reaches1 after modulo32 traversal; the target screenshot remains0080 |

These timestamps are CPCSE's machine-cycle counter, not FPGA master clocks.
At64µs per scanline,32scanlines are2048µs;512µs describes eight scanlines,
not32. The worker's final prose conflated these units; the parent correction is
saved separately, while the original trace and handoff remain intact.

This validates the source-divergence diagnosis. It **does not** establish that
original hardware takes the same release path, overturn Q20, or distinguish
positive C5 restart from the extra-frame explanation in the FPGA replay. Keep
the precise French follow-up unsent and production C5 unchanged. A matching
photograph plus a known software mechanism is useful evidence to prepare a
hardware question, not authority to copy that mechanism.


### AmSpirit original-history result

AmSpirit Lite1.17.1/core2491682 independently completes the canonical C1 history
through02C9 with the new pinned disk, classic6128/CRTC1/128KiB/French firmware,
PAL refresh and monitor Off. The only script adaptations are HTTP preloading
of that same disk (the redundant `disk_insert` is omitted), explicit output
directories and a final two-frame settle. The E page visibly prints0080 and
all seven other numeric controls match the photograph/CPCSE page. The machine
is left paused on this valid page, not an initializer restart.

Evidence is `classic/e-amspirit-full-history-v2/`: canonical/script/disk hashes,
identity/config, script completion sentinel, PNG, SNA and state. An initial
helper-method mismatch stopped before any emulation change; a final output-
directory error occurred after the successful script. Both are preserved.
Creating the missing directory and capturing the still-paused page recovered
that output without rerunning or changing the original history.

The published `/api/crtc` returns only registers and selected register on this
classic machine; its streamed SNA is version2. No classic C5 phase was observed.
Consequently this corroborates the final0080, **not** CPCSE's early-release
mechanism or any private FPGA C5 repair. Keep those conclusions distinct.

## Original reference acquisition: six attempts on both paths

A new unchanged production-T80 observer reproduces0060 and all2967 prior
observable log lines exactly after removing only its new acquisition records.
It measures the original previous-result print return, A4AD entry and six
A4D1 attempts: five retry branches followed by success,19969µs apart. The
target C5 phase remains6/7/8/9; all515 zero-route and81 off-R4 controls pass.

Parent then executed one guarded original-disk SHAKE27C/keyE run in
AmSpirit1.17.1/core2491682,6128/CRTC1/FR. Original code, stack/order, mapping
and media guards pass; the normal key coroutine finishes. Breakpoints freeze
the whole machine; no RAM/PC/counter/instruction patch or extra inter-case
frame is present. Both6F3F subcases, their cleanups,6B72,7105/A4AD, every
A4D1 and retry/success branch, original R5 instruction boundaries and the
original formatter were captured. It independently returns HL0080 and formats
0080; parent viewed the complete page and all seven other numeric controls.
The script does not assert0080 or an expected attempt count.

The reference also makes **six attempts/five retries**. Relative to6B72, both
paths reach7105 at117µs, A4AD at134µs, and A4D1 at48422,68391,88360,108329,
128298,148267µs; post-acquisition710B is168238µs on both. These align by the
original instruction identity, without a fitted offset. This excludes an
extra adaptive-acquisition frame between the previous print and target for
this observed pair. It does not reveal AmSpirit's hidden C5 or choose between
a zero-boundary C5 reset, earlier adjustment release and positive re-entry.
Earlier history and replay startup qualifications still need judgment.

AmSpirit ticks have1µs resolution. Local accepted-write times are compared
against the reference's original4µs OUT instruction windows, not assigned a
fabricated bus timestamp. Classic version2 SNA counter fields remain null.
The local dense PPI logger missed the second ED-opcode M1 identity; the
original retry/success branches establish returned bits and attempt count,
not the exact PPI sample aperture.

Evidence is under ignored `resumed-20261104/c1-positive-r5-probe/`; the
root-only reference run is `amspirit-request/runs/original-e-acq-001/`.
Production C5 remains unchanged. This narrows the prior gap audit rather
than validating the private positive-restart implementation.

## Narrowed model and implementation proof

Frozen shared-history algebra excludes a blanket C5 reset at every zero-R5
R4/R9 edge as the sole explanation. The last such edge before the target,
tick872616010, has C5=15→16. Resetting it to0 instead makes the R5=3/1 writes
inherit22/25; completion then takes8lines rather than24 and predicts0020,
not0080. This calculation changes only C5 on the frozen zero-R5 trajectory;
no simulator run or candidate output supplied that expectation. Exact
derivation: `offline-comparison/ZERO-R4-DERIVATION.md` and its algebra script.

French§11.3.2 p87 explicitly retains adjustment when R5 becomes0; releasing
it at a later zero-R4 edge contradicts that rule. The raw Q20 attachment is
still unavailable, so no new exact-author-wording claim is made. A reset at
every R4 edge, including positive re-entry, could still match0080, but its
positive restart is shared with the proposed minimal repair; E adds no
evidence to change the protected zero-route continuity.

Fresh Opus review20261104T163544Z-21690-32e5 says PROCEED with this bounded
abstract rule. A real-CPC extra retry remains unmeasured and is documented
as a residual, rather than asserted impossible. Completion/re-entry collision,
interlace added-line and row0 wrapper side effects are not inferred from E.

The private minimal C5-only candidate passes a new public-origin test that
fails before: retain the original R4=38/R9=7 phase, write3 then1 across the
positive R4/R9 edge, and require the photographed32line delay instead of24.
Removing eight312line frames from the long caller history removes78×32lines
and preserves the relevant phase without injecting counters. Off-R4 positive
writes retain their29line delay, and existing completion priority remains.
All240CRTC, CPU-phase and GA neighbors pass privately; the code and retained
test are being applied before the final gate, fresh code review and one
original-caller production acceptance replay. No hardware closure is claimed.

## Final production verification

The tracked C5-only correction passes240CRTC cases and the selected
CRTC/CPU-phase/GA gate: `python3 sim/select_tests.py --base HEAD --run`,
PASS3benches once after its last code edit. Intended soak becomes
5ee108925f8a58e6 with unchanged seed/stimulus/projection and2845088samples.
A subsequent historical-fixture Makefile repair runs its own final selected
44benches on the same frozen C5 source; no C5 code or test edits follow.

Exactly one original E controlled replay rebuilt against CURRENT CRTC wrapper,
both engines, and GA (with inherited production-family T80 netlist `bded028`
pre-`9de` startup fix, NOT latest CPU `435`) returns `0080`, all seven other
numeric controls, 515 zero-route and 89 off-R4 controls, zero failures. Its
complete output is byte-identical to the prior private restart result. The
inherited trampoline, seeded startup classification, and direct Port B
qualifications remain; this is a controlled replay, not a full original-disk
motherboard boot. Artifacts: `positive-candidate/production-replay/` and
`TRACKED-HANDOFF.md`.

Fresh Gemini 4 Argon high code review `20261104T165734Z-27885-54dd` ACCEPT reads
the French p87 render and original `CE_02C9_A` photograph directly. It checks
effective-R5 forwarding, completion/added-line priority and the public-origin
test without rerunning gates. Parent verified the three frozen source hashes.
The correction is committed and pushed as
`599ca8098c655556457a6066b2e4fdeed2d8a9a7` (exact push CI
[41874429612](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41874429612)
all required jobs SUCCESS). The full exact-source build
[41878239893](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41878239893)
and targeted native checks complete as recorded below; this does not close S.

## Targeted native acceptance

Full599build41878239893passes all required jobs/timing. Native E02C9 now gives
0080 with all seven other measurements intact, E02CA stays readable, and P
remains26. Canonical history through S retains E0080; S itself still0640
versus direct4E40, so that separate history defect is not closed by this fix.
The later publication build keeps E/P pixel-identical but changes S to5060.
[Exact artifacts and limits](classic-targeted-milestones-2026-11-04.md).
