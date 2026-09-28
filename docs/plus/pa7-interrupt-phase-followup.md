# PA7 interrupt request versus CPU acceptance

## Current position in plain English

**The V5 timing mismatch is fixed, reviewed, integrated and built.**
A separate original-Plus DMA overlap probe exposed a
[DMA/compatible cancellation mismatch](pa7-dma-overlap-original-plus-2026-09-28.md),
now repaired in a [local candidate with its own acceptance record](pa7-dma-overlap-repair.md).
Your original 6128 Plus photos showed software reacting at different positions
from our simulation. Two changes now make every measured marker and interrupt
count on screens30–35 match those photos:

- T80 now samples interrupts at the clock edge specified by Zilog. This is a
  source-derived CPU correction, supported by independent review and tests
  that fail before it and pass afterwards. It affects classic CPC too.
- The Plus-compatible interrupt is delivered about one character later.
  This reproduces the photographs, but the photographs cannot prove that
  the real ASIC uses this exact internal mechanism.

The important uncertainty is what happens in cases the screens do not cover.
In RBF1346f39, a DMA acknowledge during the new delay also clears
an undelivered compatible interrupt. The original-Plus overlap photographs
show that both interrupts survive at the disputed positions on all four pages;
the local repair now preserves both sources and all six V5 images in production
simulation. Its review, gate and device acceptance are recorded separately.
The precise electrical delay, VSYNC interaction and literal timing
comparison with an original CPC are also unmeasured.

Classic integration tests pass. Opus found no evidence that the old T80 error
was compensating for the classic gate array: the suspected errors act in the
same direction. Nevertheless, original classic hardware confirmation is still
missing. The separate classic gate-array phase question is tracked as
[B25](../backlog.md#b25-classic-ga-interrupt-phase-against-cpu-edges).

**MiSTer V5 verification is complete:** screens30–35 on RBF1346f39 match
the simulation pixel-for-pixel in the active test area, including every marker
and count compared with the original Plus photographs. **Next:** check familiar
classic and Plus software, and complete the local overlap candidate's acceptance;
clock/INT/HSYNC traces on original machines would settle the precise timing.
PA3's extra original-hardware controls remain separate B24 debt.

Build `1346f39` passed exact-commit simulation, production-T80 tests and full
Quartus synthesis. [Artifact details](#integrated-build-and-artifact) are below.

## Pre-fix investigation record

The following traces and intermediate acceptance decisions describe the
investigation before the final repair; statements that a fix is pending or
RTL is unchanged are historical. The current result is above and in
[the implementation acceptance record](#implemented-cpu-and-compatible-delivery-repair-2026-09-28).

Original evidence was the V4 screen30 photograph from a 6128 Plus through
Retrotink4K and the pixel-perfect AmSpirit Lite1.15.1/core2491682 capture.
Both showed about128 mode2 dots between marker right edges, against112 in
the pre-fix production-T80 RTL. Hardware has precedence.

## Existing RTL trace

Source: `output_files/plus-hw-probes/v4/phase4-sim/30-events.txt`, complete
CRTC origin 38, generated from the committed PA1–PA4 fixes. Master clock 64 MHz.

| Event | PRI reference | Compatible |
|---|---:|---:|
| Raw HSYNC rise | 58180362 (line7/C49) | 58430218 (line68/C49) |
| Raw HSYNC fall | 58180874 (line7/C57) | 58430730 (line68/C57) |
| INT asserts | 58180427 | 58430731 |
| CPU acknowledge | 58180522 | 58430826 |
| Palette yellow starts | 58180955 | 58431259 |
| Palette yellow ends | 58181595 (line8/C4) | 58431899 (line69/C11) |

The programmed request follows raw HSYNC rise by 65 master ticks; compatible
INT follows raw HSYNC fall by 1 tick. Both request-to-ACK intervals are 95 ticks,
and both ACK-to-palette-start intervals are 433 ticks. Removing the 61-line
vertical displacement leaves 448 ticks = 112 mode2 dots.

In `asic_ga_timing.v`, raw HSYNC fall drives `hcnt_cnt`; the 52 comparator
clears the counter, and its bit 5 transition sets `classic_int_n`. The PRI
path instead uses character-delayed HSYNC. The existing NOP image therefore
exposes an acceptance-slot difference, without proving an exact 1µs hardware
request delay. Counter reset, ACK clears, masked pending requests and VSYNC
resynchronization are shared state and must not be changed to fit one image.

AmSpirit HTTP documentation was checked on 2026-09-28: `/api/z80` exposes
registers, `/api/beam` exposes beam position and `/api/ga` exposes palette and
blanking state. These endpoints do not expose raw INT. Screenshots therefore
measure the CPU-visible result; they do not directly timestamp IRQ assertion.

The production T80 samples INT at an eligible instruction boundary (`T_Res`,
last machine cycle, no prefix and no EI inhibition in `T80.vhd`). Its bus
wrapper then delays IORQ during interrupt acknowledge. An ACK timestamp is
therefore downstream of the sampling decision; it is not the IRQ assertion
time. The follow-up must retain a controlled instruction stream through both
requests and check actual fetch/ACK phases, rather than assume equal delays.

The passive follow-up logger additionally exposes `intcnt52`, the separate
classic/programmed pending latches and T80 `intcycle_n`. The fresh screen 30
trace reproduces the timestamps above. INT-to-interrupt-cycle-entry is 55
master ticks for both requests; entry-to-ACK is 40 ticks. The compatible
C52 pulse coincides with raw HSYNC fall, with count 51; next tick count is zero
and the classic pending latch asserts. No programmed fire occurs there.
These are implementation observations, not hardware expectations.

## Follow-up design

The controls separate pending-request acceptance, the HSYNC-width anchor,
and instruction sampling phase:

| Control | Intended distinction |
|---|---|
| G: both requests pending under DI, CPU-timed magenta mark before EI | Source-dependent acceptance/ACK delay versus request timing |
| W: HSYNC width 12 instead of 8, same NOP stream | Falling-edge anchor versus width-independent anchor |
| L: repeated LD A,(HL) | Memory-read instruction sampling phase |
| R: repeated RET NC, carry set (never taken) | Five-T-state instruction sampling phase |
| I: repeated INC HL | Six-T-state instruction sampling phase |

Use the same yellow ISR for every request. L/R/I need two complementary
passes, separated by 78 lines plus one CPU microsecond, to cover the parity
of their nominal two-microsecond instruction stream. Both passes must finish
before VSYNC. IRQ/FRAME must be 04. Keep PRI0-before-MRER reset ordering.
Runtime sleds must remain at A13=1, without overlapping variables or display.

### Predictions recorded before building or running

Let D be compatible minus reference yellow right-edge position, measured
against each marker's own ruler. Use the mean of the two passes for L/R/I.
G instead subtracts each band's CPU-timed magenta edge from its yellow edge,
then compares compatible with reference. All quantities below are mode2 dots.

| Operational reading | Existing W8/NOP | G | W12/NOP | L/R/I means |
|---|---:|---:|---:|---|
| Raw falling edge, equal acceptance path (current RTL) | 112 | 0 | 176 | 112 each |
| Raw falling edge, compatible acceptance path adds 1µs | 128 | 16 | 192 | 128 each |
| Request delayed one character, equal sub-character phase | 128 | 0 | 192 | 128 each |
| Width-independent request anchor fitting original photo | 128 | 0 | 128 | Depends on request phase |
| Fractional request delay crossing the original NOP sample | 128 | 0 | 192 | May differ by ±16 from 128 at other phases |

The nominal phase offsets relative to NOP are −¼µs (L), +¼µs (R),
+½µs (I), assuming CPC WAIT alignment. Validate those assumptions in the
production trace. Do not adjust a hypothesis to match simulation: a mismatch
invalidates its simplifying timing assumptions and requires investigation.
A pass-pair must actually cover opposite parity, not merely execute the same
code at different scanlines. Record individual edges as well as their means.

This is a conditional discriminator, not a measurement of an exact analogue
IRQ edge. No finite screen set distinguishes delays within an unsampled gap.
A source-specific latency present only when the request arrives during a
particular instruction may disappear in the DI control; G=0 alone does not
prove the entire CPU path correct. Common HSYNC shifts remain invisible.
Changing HSYNC width might change other consumers; agreement cannot choose
between delaying the IRQ alone and delaying a shared internal HSYNC.

The review supplies an assembler sketch and calibration targets, not verified
code. Calibration may position the experiment (requests pending before EI,
markers visible, reset away from the counter clock edge); it must not redefine expected results.
Original 6128 Plus results are recorded below; these predictions remain unchanged.

## Oracle observations (compare with hardware results below)

AmSpirit Lite 1.15.1/core 2491682, 6128 Plus model, CRTC3, unfiltered render.
Each cartridge settled for 100 frames. The prior snapshot and pause state were
restored. Inputs are preserved under `pa7-followup/oracle-inputs/`; captures
and cartridge SHA-256 values are in `pa7-followup/amspirit/measurements.json`.

| Screen | Yellow right edges (reference A, compatible A, reference B, compatible B) | A/B separations | Mean |
|---|---|---|---:|
| 30 W8/NOP | 67, 195 | 128 | 128 |
| 32 W12/NOP | 67, 259, 67, 259 | 192 / 192 | 192 |
| 33 LD A,(HL) | 67, 211, 83, 195 | 144 / 112 | 128 |
| 34 RET NC, not taken | 51, 211, 67, 195 | 160 / 128 | 144 |
| 35 INC HL | 51, 179, 67, 195 | 128 / 128 | 128 |

Coordinates are screenshot pixels in the 768×542 unfiltered capture; horizontal
pixels are mode2 dots. Vertical pixels are doubled. Marker rows are 96–97,
218–219, 252–253 and 374–375. All five new screens show IRQ/FRAME=04; their count regions are identical
to the visually checked screen 34.

The untaken RET NC mean differs from the simple uniform-shift prediction.
With complementary passes and the same sampling rule in both windows, this
distinguishes different effective request phases from an exact-character
separation. A CPU delay common to both sources cancels; a source-dependent
path to INT sampling or emulator scheduling could still explain the result.
It does not uniquely identify raw ASIC assertion time. This is a question
for the original Plus, not a new asserted hardware rule. The pending-request
control G measures zero, as detailed below.

## Final V5 cartridge and production predictions

The final build contains 16,100 program bytes, below BF00. Screens 1–30
retain their experiment code; screen 30 pixels through raw line 191 are
identical to the prior committed simulation. Footer version/total now read
V5/35. The original cartridge artifacts remain available under `v4/`.

| Screen | RTL yellow right edges: reference A, compatible A, reference B, compatible B | A/B separation | Mean |
|---|---|---|---:|
| 30 NOP W8 | 68, 180 | 112 | 112 |
| 31 G | 468, 484, 484, 500 | See normalized control below | 0 |
| 32 NOP W12 | 68, 244, 68, 244 | 176 / 176 | 176 |
| 33 LD A,(HL) | 68, 180, 84, 196 | 112 / 112 | 112 |
| 34 RET NC not taken | 52, 180, 68, 164 | 128 / 96 | 112 |
| 35 INC HL | 52, 180, 68, 164 | 128 / 96 | 112 |

G's RTL magenta right edges are 112,128,128,144. Each yellow-minus-magenta
distance is 356 dots, so both normalized comparisons are zero. AmSpirit
places each edge one screenshot pixel earlier (yellow 467,483,483,499;
magenta 111,127,127,143), retaining the same 356-dot distance and G=0. All
four magenta spans are visible; B is +16 dots relative to A for each source.

The original design sketch placed EI near C4 and hid the magenta markers in
left blanking. The final delay constants (BC1=35, BC2=40, wait=499) move acceptance
to C11–13 without
changing the G prediction. G deliberately holds requests pending under DI:
PRI at 7/C50 and 85/C50; compatible at 60/C57 and 138/C57. MRER reset starts
at 9/C47 and 87/C48, before the raw HSYNC falls at C57 and far from VSYNC.
The compatible request is already pending during the coarse wait, as intended
for this control. It is not the free-running phase measurement of screens 32–35.

Production T80 interrupt-cycle entries confirm four phases relative to raw
HSYNC rise: NOP dot 14, LD A,(HL) dot 10, RET NC dot 2, INC HL dot 6, modulo
one character. Each tick record retains the master-clock timestamp, CPU
state, pending source and counter event. Both A/B passes complete in the same
frame; the alternate pass starts 78 lines and 1µs later. Four test ACKs plus
two arming ACKs are present, and the display reads IRQ/FRAME=04. The RET NC
stream never returns early. These are fixture validation, not hardware facts.

Final production run:

```sh
python3 scripts/diagnostics/plus_hw_probes_sim.py --out output_files/plus-hw-probes/pa7-followup/final-sim --screens 30 31 32 33 34 35 --frames 40
```

All six runs completed 40 frames, origin 38. Final AmSpirit captures reproduce
the oracle table for screens 30/32–35 with the V5 cartridge; screen 31 adds G=0.
AmSpirit state was restored after both capture sessions.

## Original-Plus handoff

Load [V5 PA7 cartridge](../../output_files/plus-hw-probes/pa7-followup/plus-hw-probes-v5-pa7.cpr).
It starts at screen 30. After settling, photograph 30, then advance with a key
or joystick fire through 31–35. Include the full screen, rulers, title and
IRQ/FRAME digits. Allow three seconds after each change. Reuse the same
6128 Plus / Retrotink 4K path so the new controls can be compared with V4.

Screen 30 must show count 02; screens 31–35 must show 04. Each new screen has
four yellow marks: reference A, compatible A, reference B, compatible B.
Screen 31 also needs four visible magenta marks. Missing marks or a different
count invalidate the intended comparison and should be reported as observed.
The main new question is whether screen 34 also averages 144 dots on hardware.

No PA7 RTL fix is made. The original Plus run is recorded below. The separate
PA3 early-arm and last-adjustment-line hardware controls are still outstanding;
this cartridge does not replace them.

Cartridge SHA-256: `70efbbbcbf6bcc994461c4052ca9cbd4f06545f6d12f429fb858161a424bf352`.

## Original-6128-Plus results (2026-09-28)

The user supplied `IMG_3973.HEIC` through `IMG_3978.HEIC` under
`output_files/plus-hw-probes/pa7-followup/`. Their visible titles identify
screens 30 through 35 respectively. Capture provenance follows the user's
6128 Plus → Retrotink 4K → TV → phone setup. Every expected marker is visible,
including all four magenta markers on screen 31. Screen 30 reads IRQ/FRAME=02;
screens 31–35 all read 04. The printed RTL predictions are labels, not results.

Measurements use each band's adjacent ruler, whose ticks are eight mode2 dots
apart. The table records visually resolved positions in ruler intervals from
that band's zero tick; camera coordinates from different bands are not
subtracted. Half-tick positions are approximate (allow about two dots per
edge), not subpixel timing measurements. These readings were made from the
photographs, independently of the emulator's expected values.

| Photo / screen | Yellow right edges in 8-dot ruler intervals: ref A, compatible A, ref B, compatible B | A/B gaps in dots | Mean |
|---|---|---|---:|
| IMG_3973 / 30 NOP W8 | 6.5, 22.5 | 128 | 128 |
| IMG_3974 / 31 G | 56.5, 58.5, 58.5, 60.5 | Normalized below | 0 |
| IMG_3975 / 32 NOP W12 | 6.5, 30.5, 6.5, 30.5 | 192 / 192 | 192 |
| IMG_3976 / 33 LD A,(HL) | 6.5, 24.5, 8.5, 22.5 | 144 / 112 | 128 |
| IMG_3977 / 34 RET NC not taken | 8.5, 24.5, 6.5, 22.5 | 128 / 128 | 128 |
| IMG_3978 / 35 INC HL | 4.5, 20.5, 6.5, 22.5 | 128 / 128 | 128 |

Screen 31's magenta edges lie at ruler intervals 12,14,14,16. Each
magenta-to-yellow distance is approximately 44.5 intervals =356 dots;
subtracting reference from compatible gives G≈0 in both passes.

The hardware supports the falling-edge-width dependence (W12 minus W8 is
64 dots) and does not show a persistent source-dependent acceptance penalty
in the pending-under-DI control. All L/R/I means are approximately128 dots.
Within the predeclared operational readings, these results fit a one-character
request shift with equal sub-character phase. They do not uniquely establish
that internal implementation: a fractional delay in an unsampled interval,
source-dependent sampling effects, and a shared internal HSYNC shift are still
not distinguished. A common Plus-versus-CPC shift remains unmeasured.

**Hardware rejects AmSpirit's RET NC mean144 as the oracle for this case.**
The disagreement is specifically the first PRI reference marker: hardware is
at ruler dot68, while AmSpirit is at dot36 (capture x51 minus ruler origin15).
The other three RET edges agree at dots196,52,180. Current RTL puts the first
reference at dot36 too (raw x52 minus ruler origin16). Hardware's first PRI
reference therefore differs by32 dots even though that is not the compatible
IRQ source. Matching the mean alone would hide this discrepancy.

The reference discrepancy is now explained by a shared CPU sampling defect
in the diagnostic counterfactual below. A compatible-only request delay cannot
by itself explain it. PA7 remains open pending the shared-CPU scope decision
and the separate ASIC timing implementation. PA3 hardware controls remain
outstanding.

## Sampling-edge diagnosis (2026-09-28)

The original RET trace has request-to-interrupt-entry margins of7/71 ticks for
reference A/B and71/7 for compatible A/B. The shared ISR takes417 ticks from
ACK to its first yellow write and holds yellow for640 ticks, identically in
all four windows. In an instrumented copy,698 sled fetch records have carry
set (flags01/41/45), and sequential RET fetches are128 ticks apart. RET NC is
untaken throughout; the alternate pass has the intended opposite parity.
Muse Spark independently checked the assembly and original trace
(`20260928T045156Z-57786-2bea`); the parent checked the added flags/clock taps.

Two software controls were predicted before assembly/run:

| Screen34 variant | Predicted and observed RTL local edges | Predicted and observed AmSpirit local edges |
|---|---|---|
| B padding3NOP→2NOP: same entry parity as A | 36,164,36,164 | 36,196,36,196 |
| RET count400→401: one full iteration longer per window | 36,164,52,148 | 36,196,52,180 |

Both retain IRQ/FRAME=04. AmSpirit snapshot and pause state were restored.
The controls confirm the model's phase/length isolation; they do not establish
physical hardware request timing or choose an ASIC-versus-CPU explanation.

### Disposable timing counterfactuals

Generated motherboard and fixture copies add two configurable delays **only to
the CPU input**, leaving production files, internal IRQ latches, counter and
ACK policy unchanged. `C` delays both sources; `K` adds a compatible-only
delay while PRI=0. Units are64MHz ticks (16ticks=0.25µs;64ticks=1µs).
Predictions were recorded first in `phase-diagnosis/predictions.md`.

| C / K | Screen34 local edges | Result |
|---|---|---|
| 0 / 0 | 36,164,52,148 | All six images byte-identical to committed-model predictions |
| 0 / 64 | 36,164,52,180 | All hardware means fit, but the two pass-A RET edges remain32 dots early |
| 16 / 0 | 68,164,52,180 | Corrects reference A; compatible timing remains wrong |
| 16 / 64 | 68,196,52,180 | Every individual measured marker on screens30–35 matches hardware |

All24 runs complete40 frames, origin38, with count pixels unchanged (02 for30,
04 for31–35). G remains normalized0. The C16/K64 local edges are:
30:52,180;31:452,468,468,484;32:52,244,52,244;
33:52,196,68,180;34:68,196,52,180;35:36,164,52,180.

The common shift must cross RET-A's7-tick margin but not INC-A's23-tick
margin; the compatible total must cross71 but not87. These delimit sampling
bins, not exact analogue timing: later ASIC assertion and earlier CPU sampling
can produce the same pictures. A sixteen-tick common term and an additional
sixty-four-tick compatible term are sufficient, not uniquely measured delays.

The initial exploratory build exposed the delay tap but failed to replace the
generated CPU's uppercase `.INT_n` connection. Its `c*` outputs are invalid as
delay experiments. The corrected build asserts the connection replacement and
observes the actual CPU pin; accepted outputs are exclusively `wired-c*k*`.

### Independent CPU rule check

[Zilog UM008011-0816](https://www.zilog.com/docs/z80/um0080.pdf), printed p12 and
Figure9 on p13, places INT sampling on the rising edge **beginning the final
T-state**, one clock before the next M1. The text was extracted with
pdf-inspector and the diagram checked visually. This is independent of the
Plus measurements and emulator behavior.

T80pa clocks the CPU on `CEN_p` (`T80pa.vhd:109`). Its bus strobes establish the
physical phase: M1 begins on the edge entering T1; MREQ/RD begin at the falling
edge inside T1. In `T80.vhd:1326`, live INT is tested under `T_Res` on the edge
leaving the last T-state and entering T1. The sample is therefore one CPU clock
late. On the Plus production divider that is16 master ticks. Opus medium
independently checked this physical phase mapping and the disposable delay
wiring (`20260928T050206Z-62360-c85b`), and advised against a Plus-only delay
on the CPU pin as a production fix. The parent verified the cited Zilog diagram;
the reviewer relied on that citation and did not independently open the PDF.

A standalone production-T80 check removes the ASIC and WAIT entirely. After
DI/LD SP/IM1/SCF/EI/NOP it executes untaken RET NC. The documented final-T rising
edge is tick640; next M1 begins at656. The three INT stimuli are:

| INT low interval (master ticks) | Expected ACK at656 | Current T80 |
|---|---:|---:|
| 632–660: spans both edges | yes | yes |
| 632–647: spans documented sample only | yes | **no** |
| 648–660: arrives after documented sample | no | **yes** |

The early pulse provides eight master ticks of setup and hold around the
sample. Expectations come from Zilog Figure9, not from fitting a photograph.
Command: `output_files/plus-hw-probes/pa7-followup/phase-diagnosis/cpu-obj/cpu_irq`.
Result: **exit1, `cpu_irq_sample: FAIL 2 mismatches`**. This is an intentional
fail-before reproduction, not a regression gate failure. No production fix
or golden-hash change has been made.

A candidate repair at the shared T80 sampling boundary needs EI/prefix, WAIT,
interrupt modes and snapshot continuation checked. The user authorized this
shared-CPU scope, but acceptance is blocked on classic-CPC impact evidence.
The Plus photos alone do not identify the CPU as their unique cause. If the
CPU repair is accepted, PA7 still needs its separate compatible-request timing
fix and source-derived checks for pending clear, reset, PRI masking and VSYNC
resynchronization.
Do not delay a live aggregate IRQ pin: that could deliver a request already
withdrawn by a register write or restore.

Artifacts and reproducible diagnostic scripts are under the ignored
`output_files/plus-hw-probes/pa7-followup/phase-diagnosis/`: `build.py`, `run.py`,
`software_controls.py`, `oracle_controls.py`, `cpu_irq_top.sv`, `cpu_irq.cpp`,
measurements, traces and both provider reports. Only evidence documentation
is committed at this point; the last production gate/soak below remain valid.

### Shared-CPU evidence and acceptance boundary (2026-09-28)

A fresh Opus medium review (`20260928T052306Z-71403-637e`) independently
confirmed the committed T80 edge mapping, motherboard clock polarity and
isolated harness arithmetic. Its worker permissions blocked both the Zilog
image and web access, so it did **not** independently verify the primary
source or upstream history. It identified the missing acceptance check:
a shared CPU change may move classic CPC interrupt acceptance, and the current
gate-array timing could compensate for the CPU discrepancy.

Gemini Flash high (`20260928T052709Z-73621-8f7e`) independently inspected the
rendered Zilog Figure9 and corroborated it with the HALT discussion and
Figure11 (printed pp14–15). The manual distinguishes the final-T sampling
edge from the next rising edge that starts the acknowledge cycle. Gemini
confirmed the one-clock discrepancy in committed T80. Its report's stimulus
table incorrectly starts the early pulse at640; the actual reproducer starts
it at632, providing eight master ticks of setup before640. The source and
recorded run above are authoritative for that interval. The review supports
the documented Z80 rule, not an independently measured original-Z80 pin trace
or a unique explanation of the Plus photographs.

Upstream history narrows the provenance. In
[mist-devel/T80 commit670437e (2018-07-14)](https://github.com/mist-devel/T80/commit/670437ea23f09f993df9cf85021b0e2d20e01b9f),
the enabled-clock `INT_s` register was removed and the boundary decision
changed to live `INT_n`, together with interrupt-acknowledge and WAIT changes.
The commit gives no separate sampling-edge rationale. Current MiST T80 retains
this choice; it is not a local Amstrad change. The related
[TV80 implementation](https://github.com/hutch31/tv80/blob/master/rtl/core/tv80_core.v)
retains an enabled-clock INT register consumed at the instruction boundary.
Thus the discrepancy should not be attributed to every T80-derived core.

**Acceptance remains pending.** Before a shared CPU fix is committed:

- Measure classic GA INT assertion against physical CPU edges and compare
  before/after instruction acceptance at the sensitive phases.
- Validate the resulting classic timing against original-hardware evidence or
  the applicable French ACCC timing diagrams; a green existing suite alone
  does not exclude compensating errors. Record missing hardware coverage.
- Only then complete the focused fail-before/passes-after CPU test, boundary
  preservation checks and fresh cross-provider code review, followed by the
  selection gate and soak required for the final code change.

Production RTL remains unchanged. The implementation worker was stopped;
its unfinished regression draft also had harness-startup failures, so it was
removed from the active test registration and preserved under ignored
`phase-diagnosis/unfinished-test-draft/`. It is not accepted regression
coverage. No new gate or soak was run for this documentation-only update.

## V5 probe tooling review and validation (before the timing repair)

Opus medium reviewed the design before any cartridge build
(`20260928T025142Z-11868-4ef0`) and independently reviewed the final code
(`20260928T034042Z-31854-8397`): accept with limits, no blockers. The parent
checked the requested individual edges, visible G bands, interrupt counts
and reset margin. Sled MRER writes start at C32/33 or C33/34, leaving at
least23µs to the C57 falling edge (W12 falls at C61). G is a separate
pending-request control with the timing recorded above.

Muse Spark implemented the passive taps and probe code; the parent reviewed
the changes and completed G visibility calibration, labels and final runs.
The screen worker was stopped after silent retry/backoff notices; no provider
remained running. Its provisional results were checked against the final
cartridge rather than accepted as a completed handoff.

Final gate (run once after the last code edit; exit0):

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/select_tests.py --run
```

```text
select_tests: PASS 41 benches: crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

```sh
make -C sim CXX=/opt/homebrew/opt/llvm/bin/clang++ soak SOAK_EXPECT=0xe99ab434a5e1cdb3
```

Soak exit0: `soak hash: 0xe99ab434a5e1cdb3`, `soak hash matches expected`
(2,845,088 CLKEN samples). No golden hash was re-minted. No production RTL
changed in this follow-up. Logs are preserved with the ignored cartridge
artifacts under `pa7-followup/validation/`.


## Implemented CPU and compatible-delivery repair (2026-09-28)

The shared CPU discrepancy is corrected at the T80 instruction decision:
Z80 modes consume INT sampled on the preceding enabled rising edge. It is
independently source-derived, not a Plus-only delay fitted to these images.
[The CPU evidence record](../t80-int-sampling.md) includes upstream provenance,
the isolated fail-before/passes-after proof and the actual classic CRTC/GA
regression. Original classic hardware confirmation remains outstanding.

The paragraph below records the original V5 model. Its DMA-ACK cancellation
assumption and unrestricted raw-IRQ differential domain are superseded by the
[hardware-derived overlap repair](pa7-dma-overlap-repair.md).

The ASIC retains its GA-derived raw counter and pending latch. A separate
compatible-request delivery latch samples that raw request at the next
CCLK edge, approximately one character later. PRI delivery is unchanged.
Raw AND delivered eligibility prevents a cancelled request from reappearing;
MRER/ACK cancellation, PRI masking and immediate delivery of restored pending
are checked by pr14. Raw IRQ continues to qualify counter ACK clearing;
the differential bench still checks raw GA lockstep. Reset retention follows
the existing classic latch policy. This is a chosen model mechanism:
next-CCLK phase, VSYNC-originated request delay and clearing an undelivered
compatible request during a DMA ACK are not independently hardware-proven.
The conditional pr14 bounds are relative to the retained raw-counter phase;
they must not be presented as measured INT-pin bounds.

Production probe simulation was rebuilt after both RTL edits and run for
40 frames per screen. Yellow endpoints relative to each local ruler are:

| Screen | Corrected production RTL | Original 6128 Plus photograph |
|---|---|---|
| 30 | 52, 180 | 52, 180 |
| 31 | 452, 468, 468, 484 | 452, 468, 468, 484 |
| 32 | 52, 244, 52, 244 | 52, 244, 52, 244 |
| 33 | 52, 196, 68, 180 | 52, 196, 68, 180 |
| 34 | 68, 196, 52, 180 | 68, 196, 52, 180 |
| 35 | 36, 164, 52, 180 | 36, 164, 52, 180 |

Hardware endpoints have approximately ±2-dot photographic uncertainty.
Counts remain 02 on screen30 and 04 on screens31–35, pixel-identical to the
previous simulation count fields. These were comparisons against already
recorded photographs, not newly derived expectations. Reproduction:

```sh
python3 scripts/diagnostics/plus_hw_probes_sim.py --out output_files/plus-hw-probes/pa7-followup/production-corrected --screens 30 31 32 33 34 35 --frames 40 --cxx /opt/homebrew/opt/llvm/bin/clang++
```

No same-setup original classic CPC versus Plus measurement exists. The audit's
literal cross-machine §7.22 claim therefore remains untested even though the
software-visible Plus mismatch is repaired. Further hardware work should
measure raw INT against clock/HSYNC, exercise classic RET NC parity pairs,
and repeat the existing Plus software checks on the resulting MiSTer RBF.

Opus5.5 medium reviewed the actual CPU/ASIC diff and both new CPU benches
(`20260928T061710Z-98533-b40b`): GO, no blockers. It confirmed that the model's
early GA request and old late CPU sampling reinforce each other rather than
compensating. Follow-ups tighten pr14 to conditional 55/72 bounds, label PRI
mask maturation as model policy, and track the classic phase question as
[B25](../backlog.md#b25-classic-ga-interrupt-phase-against-cpu-edges).
A DMA ACK inside the new undelivered window is classified as DMA and can clear
its flag **and lose the raw CPC request**; previously that ACK was raster and
left the DMA flag pending. This concrete consequence is accepted as model
policy pending a mixed DMA/PRI=0 original-hardware probe.

The first final gate exposed b8_5_b3's eight-tick CPU-delivery assumption.
The revised snapshot test preserves that deadline on the raw counter event,
then separately checks the V5-compatible delivery window. It does not extend
the deadline for the restored 52-line counter. No restore policy or RTL was
changed to resolve the test failure.

The follow-up caught a harness-unit mistake before acceptance: `plus_p8`
ties `cen_16` high, so its steps are 16MHz, whereas pr14/production use
64MHz steps. The snapshot delivery bounds are rounded outward to 13/18
fixture steps; the raw eight-step deadline stays unchanged. The failed
55/72-step attempt and corrected result are preserved in validation logs.

Opus medium follow-up `20260928T062807Z-5073-e6da` independently checked
fixture clock units, outward rounding, the preserved raw deadline and the
final focused log: GO. The earlier follow-up missed the unit mismatch; its
GO alone was not used as acceptance. No production RTL changed after the
main code review. Final focused `plus_p8_tests` and `asic_pri_tests` pass.

### Final repair gate

After the last code edit, the selected gate passed (the preceding failed gate
and focused unit-correction attempts remain in the ignored evidence directory):

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/select_tests.py --run
```

```text
select_tests: PASS 41 benches: crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

```sh
make -C sim CXX=/opt/homebrew/opt/llvm/bin/clang++ soak SOAK_EXPECT=0xe99ab434a5e1cdb3
```

Soak: `soak hash: 0xe99ab434a5e1cdb3`, `soak hash matches expected`,
2,845,088 CLKEN samples. This is the classic CRTC projection; it does not
claim CPU/ASIC behaviour was unchanged. The intended changes are demonstrated
by the focused CPU/ASIC failures before repair. No golden hash was re-minted.

## Integrated build and artifact

Source `a68dca89c71d2bf0f74dd95c487fc53a9458d6f4` was merged without conflicts
or post-review code changes as `1346f399639a2b7f5850eb07d74b2fa7757bf6d9`.
[CI run36425467292](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/36425467292)
passed selected simulation (41 benches), lint, production-T80, synthesis policy,
routing, full hosted synthesis and the required gate on that exact commit.
The local synthesis leg was correctly skipped. The production-T80 log includes
`classic_irq_phase: 8 passed, 0 failed`, isolated sampling controls and B18
snapshot preservation.

Artifact `Amstrad-build-292-1-full` records `build_mode=clean_full`, Quartus
17.0.2. Worst setup/hold slack is +0.628/+0.247ns, all TNS zero; utilization
is24,569/41,910 ALMs (59%). Delivered main-checkout file:
`output_files/Amstrad_20260928_1346f39.rbf`.

SHA-256 (CI download and delivered copy agree):
`680940ac1d03d6195bccfe33fc15178a7a83213308655070bb88ff943b61d523`.

This is a simulation- and timing-verified hardware-test artifact, not a claim
that the remaining original-hardware checks have been run. This documentation
handoff does not alter the bitstream's source commit.

## MiSTer V5 verification (2026-09-28)

The hardware-loop driver loaded the exact delivered RBF1346f39 on `root@mister`
and six independently auto-starting V5 cartridges (screens30–35). Three native
768×273 PNGs were captured per load; all three are byte-identical for each
screen. Each screen was visually inspected. The 640×200 active test area
(x16,y40 in MiSTer capture; x16,y0 in the simulation PPM) is pixel-identical
to the corrected production simulation for **all six screens**. This includes
yellow markers, G magenta reference markers, labels and interrupt counts.

| Screen | MiSTer yellow endpoints, relative to ruler zero | IRQ/frame |
|---|---|---|
| 30 | 52,180 | 02 |
| 31 | 452,468,468,484 | 04 |
| 32 | 52,244,52,244 | 04 |
| 33 | 52,196,68,180 | 04 |
| 34 | 68,196,52,180 | 04 |
| 35 | 36,164,52,180 | 04 |

Screen31 magenta endpoints are96,112,112,128; G remains0. These agree with
all previously recorded original-6128-Plus photographic measurements. The
native screenshots validate the synthesized FPGA implementation; they do not
measure original ASIC pins or independently capture the HDMI/analogue output.

CFG: 6128 Plus (bits34:33=2), Full sync filter (bits36:35=0), other fields
preserved. Applied CFG SHA256:
`13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`.
Original CFG SHA256:
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
The user explicitly authorized replacing the active Amstrad session and asked
not to restore it. Screen35 and the test configuration were left running.
The remote RBF hash matches the integrated-build record above.

Captures, case JSONs, media/Main hashes and cleanup manifests are preserved
in the task checkout's ignored `docs/references/pa7-mister-20260928-1346f39/`
and `output_files/plus-hw-probes/pa7-followup/mister-1346f39/`.
No RTL changed; no simulation gate rerun is required for this evidence update.

## Remaining mechanisms and how to distinguish them

The V5 prediction table explicitly compared observable timing readings. It did
not completely distinguish internal circuit mechanisms. The remaining choices
should be kept separate:

| Hypothesis | Meaning | Current evidence / discriminator |
|---|---|---|
| Early pending latch, later CPU delivery | A request exists internally before the CPU sees it | Current implementation; a DMA ACK in the gap can erase it. Test this on original Plus |
| Later creation of the pending request | The counter event or its input is later, so no request exists during some or all of the gap | Can fit the visible timing; cancellation interactions may differ. Needs a reviewed overlap probe |
| Shifted internal HSYNC timing | Timing moves at an input shared with other ASIC functions, rather than only at the IRQ output | Width tests constrain it but do not isolate all consumers. Probe another HSYNC-driven function alongside IRQ |
| A different delay within the same CPU sampling interval | The electrical edge differs but software accepts on the same clock | V5 cannot distinguish this; finer phase controls or physical signal capture are needed |

A persistent source-specific acceptance penalty after the request is already
pending is disfavoured by the G control. The shared T80 correction has its own
Zilog-based evidence; a Plus-specific CPU input delay is not the chosen fix.
A common shift between original CPC and original Plus remains unmeasured.

For DMA overlap, first design a software CPR probe for original6128Plus: sweep
DMA interrupt/acknowledge timing across the compatible boundary, record vector
order/count and DMA flags, and include no-DMA and early/late controls. Exact
predictions require design review before cartridge construction. This needs no
physical circuit probe. By contrast, an electrical clock/INT/HSYNC trace means
connecting suitable logic-analyser or oscilloscope probes to accessible signals
on an original CPC or Plus. That is optional deeper evidence, not what the
MiSTer screenshot capture did.

The classic gate-array logic is derived from Gerald's reverse-engineered40010
schematics. This strongly supports the underlying logic, but does not prove
all FPGA clock adaptation and inter-chip timing. Production instantiates
`syncgen_sync.v`; the schematic-style `syncgen.v` is a VERILATOR comparison
shadow (`ga40010.sv`). B25 therefore concerns the **combined CRTC→GA→CPU phase
and its interpretation**, not a demonstrated error in the original gate-array
logic. Check CRTC HSYNC phase, enable/clock mapping, synchronous adaptation,
bench anchoring and the Compendium reading before proposing a GA change.
Original classic validation means the same program on a real non-Plus CPC
and MiSTer; no T80 code can be installed into the original machine's Z80.
