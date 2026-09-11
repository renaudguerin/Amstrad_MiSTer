# Hardware follow-up diagnosis — 2026-09-10

The September 9 retest has produced a new code-level lead: type-1 interlace
loses alternate VSYNC pulses when VSYNC begins at the frame origin. A focused
simulation reproduces the anomalous `#4E00` interval visible in SHAKER B (9).
This warrants a bounded repair. It does not establish the cause of DSC4's
garbled display or close any hardware result.

The broader display investigation also needs the small hardware feedback loop
already prepared in B2. Its first useful target can be Classic SHAKER; the
Plus BASIC boot blocker does not prevent that work. Repeating broad RTL audits
without a repeatable capture and a named hardware discriminator has diminishing
returns, even though specific code defects remain discoverable.

**Second pass (same day):** [hardware-diagnosis-2026-09-10-second-pass.md](hardware-diagnosis-2026-09-10-second-pass.md)
confirms D1–D5, extends D1 to CRTC 0 under French §19.7.2, and adds D6 (RFD
parity cannot be fixed by IVM ON/OFF), D7 (the SHAKER A (4) byte difference is PPI port B configuration, not CRTC) and
D8 (scaler-framebuffer reading of the "leftover" band).

## Source and retained evidence

- Checkout: `accc-review-and-fixes`, HEAD
  `6d1bfcdd79dbafdeced753bda3592d0ba529bcf3` (documentation only after the
  production source `ce1d2da67c2598c0dd06208b9fc14c52ada01712`).
- Hardware observations: [September 9 report](hardware-evidence-2026-09-09.md),
  RBF `Amstrad_20260908_ce1d2da.rbf`. This investigation did not run new hardware
  tests, deploy an RBF, change production RTL, or claim hardware closure.
- The user's right-edge sprite correction is a **nonregression** of the earlier
  fix, not a new improvement attributable to this retest.
- [Local diagnostic bundle](references/hardware-diagnosis-2026-09-10/README.md):
  sources, reproduction script, parent rerun logs, independent VSYNC review,
  and 18 selected reference images with URLs and SHA-256 values. These local
  assets remain ignored, as do the ACCC PDFs and user-owned media.
- Parent verification: `make -C sim` passes on the unchanged production source,
  including 192 classic vectors, with the existing `fdc-payload-poll` XFAIL.
  The three new failing diagnostics are deliberately separate from that suite;
  origin production / scratch control exit 1 / 0, sprite and PPI cases exit 1.
  The retained ASCAL reproduction was rebuilt and reproduces all three
  geometry measurements below.

## D1 — type-1 VSYNC uses outgoing parity at the frame origin

**Status: confirmed production RTL defect; repair pending.**

### Hardware clue and rule

The CRTC1 B (9) capture `20260909_022251-shaker.png` reports the first
`R7=0, R9=7` block as `#4E00`, `#4E40`, `#4E40`, `#4E80`, `#4E80`.
The corresponding [real-CPC photograph](https://shaker.logonsystem.eu/images/cpc/CPC/B9_CRTC1_A.webp)
and [Amspirit 2.0 reference](https://shaker.logonsystem.eu/images/amspirit/2.0/B9_CRTC1_A.webp)
show `#2740`, `#2760`, `#2780`, `#27A0`, `#27C0`. The six measurements below
`R7=#18, BEFORE R6` match (`#1820` through `#18A0`, then `#25C0`), as does the
line-43 cutoff value `#43C0`. This separates a frame-origin problem from a
general inability to display or read the result screen.

French ACCC v1.11 **§19.5.3 p.209** specifies type-1 IVM frame parity and the
VSYNC table: at C4=R7 the even frame uses MID-VSYNC, offset by 32 microseconds
under normal 64-microsecond line timing; the odd frame uses the line seam.
The table includes C4=0 on both parities. The French page was visually checked
after consulting its position-aware extraction; English §19.5.3 p.208 was used
for navigation. Both original PDFs remain private local references.

### Production mechanism

In `rtl/CRTC.v`, `vsync_ivm_mid` is derived from the registered
`!parity_frame`; it selects both `vsync_count_tick` and `vsync_fire`.
With R7=0, the type-1 engine's row comparison fires at the same origin that
updates frame parity. The decision therefore uses the outgoing frame:

- Entering an odd frame, the old even parity selects a midpoint tick at the
  seam, so VSYNC does not start. After parity toggles, the midpoint no longer
  qualifies either: the entire pulse is lost.
- Entering an even frame, the old odd parity selects a seam start, which fires
  too early instead of arming MID-VSYNC.
- With R7 away from the origin, parity is already current when the comparison
  happens; the selected controls produce one pulse per frame.

The motherboard PPI reads **raw selected VSYNC** (`vs_sel` on PPI port B),
upstream of `crt_filter`, `video_mixer` and ASCAL. A scaler image defect cannot
by itself account for these changed program-readable VSYNC intervals.

### Reproduction and limits

The diagnostic uses the existing `sim/crtc_cpu_phase_top.sv` production GA and
divider fixture with unchanged CRTC/rule engines. It snapshot-initialises a
register set, then ignores the first three frames and observes six complete
steady-state frames. It does **not** execute SHAKER or model the program's
timed R8 writes.

| R7 / R9 | Production source | Scratch parity control |
|---|---|---|
| 0 / 7 | Three of six frames miss VSYNC; surviving rises are at C0=0; interval 19,968 us = `#4E00` | One rise per frame; odd seam / even midpoint |
| 24 / 7 | One rise per frame | One rise per frame |
| 0 / 8 | Three of six frames miss VSYNC | One rise per frame |
| 1 / 8 | One rise per frame | One rise per frame |

The scratch control substitutes the incoming parity at `frame_new`. It removes
the missing-pulse failure; it does not reproduce every numerical SHAKER result.
Gemini independently rebuilt production and control and agreed with the defect.
The review correctly rejected treating this one-line experiment as a complete
repair: type-1 `pf_value` also has R8-transition ownership, and an in-flight
VSYNC can cross a frame origin.

**Next repair:** use the engine's canonical parity transition, with a failing
regression covering origin pulse count and phase. Check active-pulse width,
R7 writes near origin, R8 transitions (including mode 1), snapshot entry and live
type changes as relevant to the implementation. Run the normal simulation,
lint and soak gates plus fresh cross-provider review. Then compare the same
hardware B (9) case and unchanged nonzero-R7 controls. Treat DSC4 as a separate
holdout, not a promised beneficiary.

## D2 — Live blanking changes ASCAL's acquisition geometry

**Status: known B6 design problem, now reproduced in the actual ASCAL input
logic; not a newly discovered CRTC rule.**

`video_mixer` derives display enable from horizontal/vertical blanking. In the
current `sys/ascal.vhd`, the input horizontal counter starts on DE rising edges,
width is measured from DE, and a delayed DE falling edge advances the input line
counter. The `i_hs` port has no consumer in this implementation. Regenerating
stable HSYNC while passing live HBLANK therefore does not keep acquisition
geometry stable.

A GHDL fixture instantiates unchanged `sys/ascal.vhd`. Its 300 active lines give:

| Input experiment | Measured geometry |
|---|---|
| One 768-sample DE window per line, fixed HSYNC | 768 × 300 |
| Two 256-sample DE windows per line, same HSYNC | 256 × 600 |
| One 768-sample DE window, deliberately shifted HSYNC | 768 × 300 |

The parent reran and checked the reported maxima (767/299, 255/599, 767/299).
This is an **input-geometry diagnostic**, with output clock disabled. It does
not model framebuffer delivery, HDMI, interlace buffering, a complete mixed
language motherboard, or a particular DSC4 waveform. Output clock suppression
avoids an existing GHDL startup range error on `o_pshift=-1`; it is an explicit
scope boundary, not evidence that full scaler simulation passes.

The [B6 decision](b6-architecture-decision.md) already prescribes the next
direction: keep the Full acquisition tuple stable, and preserve the intended
raw blanking effect in pixel colour/timing at a separately defined boundary.
`crt_filter.SHIFT` also changes VRAM byte assembly, so an apparent pre-filter
RGB signal is not yet a filter-independent image. Further HBLANK-width tweaks
alone do not address this contract. Full-mode DSC4 failure remains unexplained
by this diagnostic.

## D3 — sprite first-row refill can miss early pixels

**Status: bounded first-row repair implemented on the D3/D4 Plus task;
relationship to reported flicker unproved.**

The parent reviewed and reran a diagnostic of unchanged
`rtl/plus/asic_sprites.v` using the production 4-master-clock/dot ratio and
`H_ORIGIN_DOTS=16`. Attributes are static before Y=10. Sprite pixel data is
returned with an uncontended registered request/ACK model matching the
`asic_regs` service; the fixture does not instantiate the full motherboard or
CPU/ASIC RAM arbitration.

All sprites are unmagnified. Only the target sprite has nonzero pixels, so
higher-priority sprites cannot hide it. Arnold V issue 1.5 **§2.1** defines
the 16×16 image, zero-colour transparency and fixed priority.

- One enabled sprite, either sprite 0 or 15: all 16 pixels appear on the first
  and second lines at X=0,16,32,64,256.
- All 16 enabled, opaque target 0: the same controls pass.
- All 16 enabled, opaque target 15: **all 16 first-line pixels are missing**
  at X=0,16,32,64; the second line is complete.
- Moving that target case to X=256 restores the complete first line too.

In the investigated source, `c_predok` required the current line to be inside
the vertical window, so the first visible source row was not speculatively
prepared while the sprite was still above Y. The walker scans whole eight-byte
sprite blocks, leaving later sprite IDs dependent on urgent refill after the seam. This is a stronger lead
than the rejected claim that even a lone sprite at X=0 must fail.

The original source header's claim that static-Y sprites never show an unfetched
pixel was too broad. The repair below defines a first-visible-row/prefetch
contract and the real service cadence. This diagnostic establishes first-row
loss under its stated conditions, not the temporal leftmost-1/16-screen flicker in games, nor the
DMA pitch/crash defect. Keep those hardware observations separate.

### D3 repair contract

The predictor now also permits speculation on the compare line immediately
before Y: the ten-bit vertical difference advances from 1023 to zero, and
the existing four-bit row-tag increment stages source row zero. The existing
bank promotion and stale-response checks remain in use.

The guarantee requires attributes and image data to be stable by the preceding
line seam, sequential compare-line advance into Y, and an uncontended normal
64-character line at four master clocks per dot. The required `d3-sprites`
target uses production `H_ORIGIN_DOTS=16`, all 16 sprites enabled and only
sprite 15 opaque at X=0/16/32/64/256. It checks 16 visible pixels on both the
first and second rows, with distinct row colours to reject wrong-row delivery.
The original RTL fails the first-row assertion at X=0; the repair passes.

Reset-at-Y, discontinuous CRTC taps, late attribute changes, CPU access
effects and insufficient service time remain outside the guarantee.
This is an implementation service contract, not a claim about the ASIC's
undocumented internal sprite-fetch schedule or a hardware flicker fix.

## D4 — Plus PPI control readback contradicts the hardware reference

**Status: mode-word readback repair implemented on the D3/D4 Plus task;
title causality unproved.**

Kevin Thacker's [Extra CPC Plus Hardware Information](references/Extra%20CPC%20Plus%20Hardware%20Information.md),
**PPI / PPI Control port**, reports `00` after control writes in 80–8F and
`FF` after writes in 90–9F. The investigated `rtl/i8255.v` instead returned its
stored `mode` value for every control-register read, including Plus mode.

The focused diagnostic obtains `82` after writing `82` (expected `00`) and
`9B` after writing `9B` (expected `FF`). Independent port-A controls pass:
output data is retained, input data is read, and input mode presents `FF` to
the PSG. This narrows the defect rather than blaming the entire input path.

The original `sim/plus/p10_dma_ppi_test.cpp` expected an external F700 read of
`9B` to prove direction retention. That assertion encoded the implementation's
readback, not the measured Plus rule. The repair verifies retained direction
through port behavior while pinning actual external readback separately.

There is no evidence yet that Pang, Plotting, Arnold 5 or the System cartridge
executes the affected control-read sequence. Do not claim this explains stuck
fire or failed boot merely because the symptoms involve input.

### D4 repair boundary

Plus control reads now decode the retained mode word's bit 4 to `00` or `FF`,
matching Thacker's mode-word table (`80–FF`). Classic control reads still return
the stored word. Mode storage and port-direction behavior are unchanged.
The P8 PPI test pins the external readback, and the DMA/PPI regression checks
input-direction retention through the Port-A pins before any setup rewrites
the direction; its subsequent AY/keyboard read still checks the input path.

BSR writes continue to update Port C without replacing the retained mode word.
Thacker's separate `00–7F → FF` control-read result after BSR is not implemented
by this change when the retained mode has bit 4 clear. That separate behavior
does not weaken the required `80–9F` mode-word checks. No title-level or new
hardware validation was performed.

### D3/D4 validation — 2026-09-11

`make -C sim` and `make -C sim lint` pass, including the required D3 target,
P8 PPI checks, DMA/PPI concurrency, sprite/register service and CPR regressions.
The classic suite has 192 passes; the existing `fdc-payload-poll` XFAIL remains.
The new P8 assertion fails against original RTL at `80 → 80` instead of `00`;
the new D3 assertion fails at X=0 with zero first-row target pixels. The original
20-case diagnostic has exactly four failures at X=0/16/32/64, with X=256 passing.

Fresh Opus 5 high cross-provider review is **CLEAR on correctness**: it checked
the ten-bit prediction wrap, four-bit row-tag promotion, unchanged stale-ACK
rules, PPI decoding and required-pass wiring. The parent checked its requested
failure evidence against retained logs and corrected stale access-invalidation
wording. Review was source-only; simulation and lint were run by the implementer
and focused gates rerun by the parent. The Port-A behavior check intentionally
does not assert every retained control-word bit through a private internal tap.
Logs and the complete review are retained locally under ignored
`docs/references/d3-d4-validation-2026-09-11/`.

## Classic / Plus ownership and reset

Both CRTC/GA paths remain clocked, and both CRTC instances receive I/O traffic.
Runtime output selection owns bus readback, MA/RA, raw HS/VS/DE, FIELD,
interrupts, memory/CPU timing and RGB. The earlier FIELD leak is fixed in this
source. CPU, PPI/AY/HID, main memory and the later video pipeline are deliberately
shared. Thus they are runtime-selected paths, not two completely isolated
machines. This pass did not establish another inactive-engine output leak.

Do not gate engines directly from raw menu `plus_mode`: selection changes
before the explicit reset/apply action, so transition state and ownership need
a contract first. The B6 rationale remains applicable.

Normal machine reset does not scrub main SDRAM: `Amstrad.sv` routes writes during
reset from the download/boot writer; there is no ordinary-reset erase loop.
The SDRAM controller's PLL-init sequence configures the memory rather than
zero-filling it. **An RBF reload therefore is not a promise of cleared RAM.**
Zilog's [Z80 CPU manual](https://www.zilog.com/docs/z80/UM0080.pdf), RESET pin
description, specifies CPU-state reset, not erasure of external memory. Firmware
and loaded software determine which RAM locations are subsequently written.

The user's DSC4 remnants are preserved as an observation. They can reflect
retained emulated RAM, stale displayed buffer regions, or another rendering
error; the captures do not distinguish these. A useful control explicitly
writes a known pattern to the relevant video RAM and checks whether the visible
remnants follow the new data. A normal reset should not acquire a blanket RAM
clear merely to make this symptom disappear. Deterministic RAM initialisation
may be a separate test setup choice, recorded alongside the load sequence.

## Plus boot/input blocker and next work

The user reports that System/BASIC cartridges fail to finish boot with a
"disc missing" message before any apparent disk access. This blocks DSK-based
SHAKER/DSC4 testing in **Plus/CRTC3 mode**; only CPR software currently works
there. Classic disk-based investigation remains available. Pang and Plotting
also appear to see fire continuously pressed.

Treat boot selection/input, cartridge firmware mapping and actual FDC traffic
as separate candidate boundaries. The older reduced-TV80 payload XFAIL does
not establish the cause of this boot failure. Its first useful observation is
the actual cartridge's boot branch plus PPI/AY input bytes and whether the CPU
issues FDC commands; a missing-disk screen alone is not that trace.

### D5 — ROM 0 selects the cartridge's disc-boot path

**Status: strong boot hypothesis from verified ROM bytes and mapping; correct
hardware `/EXP` rule still needs resolution before a production change.**

The local `06_System/Plus_EN.cpr` is 131,148 bytes, SHA-256
`3ce35dfccf79ee6bf8f990124aa4e0af1ce9753cbca03af8545abef21cf081ae`.
The parent checked its chunk map and bytes: `cb01` contains BASIC; `cb03`
contains AMSDOS, the Plus menu, and the `missing`/`Retry`/`Drive` strings.
The user's device cartridge has not been hashed against this local copy.

In `cb03`, the entry at C1B3 tests carry, calls B912 (current ROM selection),
then takes `JR Z,C1D3` when selection is zero. The C1D3 path calls the boot
sector routine at C666; its failure branch at C21A loads error 0F, calls CB1E,
and retries. The menu's F1 branch reaches `LD HL,0; JP BD16` at CE1B.
These bytes give a concrete discriminator: observe which physical cartridge
page is active when firmware launches ROM 0, and whether execution reaches
C1D3/C21A. They do not establish that the user's CPU followed that branch.

Production `plus_mmu.v` selects physical page 3 for ROM 0 with `exp_n=1`,
and `Amstrad.sv` ties `plus_exp_n` high. Selecting page 1 instead would avoid
executing this AMSDOS entry as ROM 0. That is a useful controlled experiment,
not yet a source-verified replacement rule for every `/EXP` state.

The two Gemini passes called the mapping polarity inverted. Their **functional
inference is not accepted as silicon proof**: Arnold V §2.8 distinguishes disc
codes 0/7 but does not by itself establish this claimed polarity. The project's
revised-spec digest currently says the opposite. The classic firmware manual's
grounded `/EXP` link cannot simply be carried over to Plus. The original Plus
[service manual](https://retronik.silicium.org/DOCUMENTS/Info/Amstrad_CPC/Amstrad-464plus-6128plus-GX4000-MM12-MM14-Service-Manual.pdf)
and its rendered 6128+ schematic p.18 are retained for continued checking.
The parent's image check did not substantiate Gemini's specific claim that
`R116` pulls up an ASIC pin 130 named `/NEXP`; do not propagate those pin/resistor
claims into the implementation. Resolve the revised hardware rule or measure
the ROM-select behavior, then test ROM 0, ROM 7, direct page selection and
GX4000 independently.

The attempted existing P10 capture did not complete a firmware frame. Its
header identifies the **reduced TV80 surrogate**, with conditional branches and
interrupt software outside its contract; this is not a System CPR boot trace.
The separate production T80 netlist path now exists under `sim/Makefile`, but
was not integrated with this Plus firmware capture here. Retained material is
under `references/hardware-diagnosis-2026-09-10/plus-boot/`.

## Recommended sequence

1. Repair D1 in the Classic accuracy stream, starting from the retained failing
   discriminator. Its result has a specific SHAKER hardware comparator.
2. Use the existing [B2 driver](mister-hardware-loop-driver.md) to establish one
   repeatable Classic SHAKER screen, with three captures, exact RBF/media and
   declared model/filter/scaler settings. The user supplied `root@mister`,
   `/media/fat/_Computer/Amstrad` and `/media/fat/games/Amstrad`; access is no
   longer an unknown prerequisite, though device functionality remains untested
   in this session. The example case still needs real boot/menu input, and
   declared settings are recorded rather than applied by the driver.
3. Use D2 to guide a bounded video-acquisition experiment, with stable acquisition
   and raw pixel effects observed separately. Keep DSC4 and Amazing Demo as
   named holdouts; use repeated captures or video for flicker.
4. In the separate Plus stream, unblock System/BASIC boot and trace the no-input
   state. D3 and D4 are retained scoped repair leads, not substitutes for a
   real-cartridge trace. Preserve existing CPR regression cases.

Amspirit is useful for clean comparisons but is not an infallible oracle. For
A (4), the real photograph reports `01` at the `C0vs=#3c` OUTI case and `5E5F`
in the lower block; Amspirit reports `00` and `1E1F`, while this MiSTer capture
reports `00` and `FEFF`. The existing SHAKER 2.6 references and supplied 2.7
capture need a proper routine/version comparison before assigning that timing
difference to a new RTL rule. The downloaded-version analysis did not provide
sufficient evidence to establish binary equivalence. A (U) and C (1) do not
receive whole-test pass verdicts from this spot-check.
