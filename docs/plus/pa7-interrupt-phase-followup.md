# PA7 interrupt request versus CPU acceptance

Status: V5 original-6128-Plus results recorded; no IRQ RTL change. Original evidence is the V4 screen 30
photograph from a 6128 Plus through Retrotink 4K and the pixel-perfect AmSpirit
Lite 1.15.1/core 2491682 capture. Both show about 128 mode2 dots between marker
right edges; the production-T80 RTL shows 112. Hardware has precedence.

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

The next implementation prerequisite is to account for the RET NC pass-A
reference phase through CPU/WAIT/instruction alignment, then prove any proposed
compatible-IRQ timing change against all individual hardware marker positions
and counts. A compatible-only request delay cannot by itself explain the PRI
reference discrepancy. Preserve the existing classic lockstep test until the
specific Plus deviation is justified; do not weaken expectations or change
shared counter, ACK, PRI or snapshot policy merely to fit a mean. PA7 remains
open, and no RTL was edited for this evidence update. PA3's separate hardware
controls remain outstanding.

## Review and validation

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
