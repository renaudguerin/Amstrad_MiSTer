# V1: sync response feasibility and trace boundary

## Decision

Keep the three production output modes and the reserved-value fallback unchanged.
Do not repair CRTC rules or rewrite filter acquisition from these images. A
rising-edge-only horizontal phase model is insufficient as the complete proposed
presentation: it has no pulse-width response, pulse selection is unresolved,
and its finite-trace buffer demand is not a bound on malformed video.

This unit supplies a passive video trace exporter and a mapped comparison corpus.
It does **not** establish a CTM transfer function, close B22 acquisition, or fix
Amazing's upper effect. V2/DSC4 and V3/Amazing lower corruption retain separate
owners and acceptance. The [work plan](sync-response-and-demo-plan-2026-10-29.md)
remains the task contract.

Source base: `040f57df2d0a97b98c7103db1f0a035a108a01d0`; fetched local and
remote master agreed. Prepared brief `9b0b4bb` was cherry-picked as `2a9cb05`.
Branch: `codex/general/sync-monitor-response`, worktree `f4da`.
No device or shared AmSpirit operation was performed by Session B.

## Corpus identity and authentic navigation

Cross-checked the supplied `SHAKER_SCREENSHOT_CODE.xlsx`, sheet `SNAPSHOT REF`,
against the live [portal test inventory](https://shaker.logonsystem.eu/api/tests),
the supplied CSL and original-CPC photograph index. All identifiers below are
page codes, not a claim that this session executed them.

| Portal group | Module/menu key | Pages, in supplied CRTC1 CSL order | Workbook rows |
|---|---|---|---|
| AT: R2 UPD DURING & AFTER HSYNC | A / `T` | A–F: `00BD`–`00C2` | 118–123 |
| AY: R3 UPD DURING HSYNC | A / `Y` | A–G: `00C4`–`00CA`, then H: `03AC` | 125–131, 634 |
| D3: 2XCSYNC RELATIVE | D / `3` | A1–A11 `0314`–`031E`; B1–B11 `031F`–`0329`; C1–C11 `032A`–`0334`; D1–D11 `0335`–`033F` | 553–596 |
| D4: CSYNC MULTIPLES | D / `4` | A `037F`, B `0380` | 597–598 |

D3 is **44 pages**, not a single page or a CRTC3 selection. D4 is two pages.
The portal lists AY/H second; that is not the supplied script's execution order.
`CSL_27/MODULE A/SHAKER-A-1.CSL` boots `shaker27.dsk` / `SHAKE27A`.
`CSL_27/MODULE D/SHAKER-D-1.CSL` boots **`shaker26.dsk` / `SHAKE26D`** despite
its directory name. Preserve these scripts and record the version actually run.

Full script prehistory matters. A reaches `T` after keys `1`–`9`, `0`, `R`;
`Y` follows `T`. D reaches `3` after `I`, `R`, `T`, `H`, `1`, `2`, including the
long D1 space sequence; `4` follows the complete key-3 group. Direct menu entry
is a separate cold-entry experiment. The prior D1 input deficit is documented
in the integrated October 28 evidence; it must not silently become a shortened
history control. A static literal-marker scan cannot authenticate these page
codes because SHAKER patches its marker template at runtime; see the
[marker inventory](../ssm-csl/shaker-ssm-marker-inventory-2026-09-12.md).

Media SHA-256:

```text
shaker26.dsk  f7082f8eab521d632c343a288f54038af6df090c59b372e0d2866269c2cc4d08
shaker27.dsk  65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b
A-1 CSL      f18cd098158874d41aae82eff36f1ef0947051737f5385c30ec575c694383333
D-1 CSL      1ab3e5c66120c086a1d58e213e0161cf5d4664e97bea29b3fdf60e9628e9b526
```

## Evidence matrix

| Boundary / evidence | Established here | Limit / required discriminator |
|---|---|---|
| Original-CPC photographs | All 60 CRTC1 pages have indexed photographs; sampled AT/AY show changing blanked regions, D3 shows displaced/sheared central content, D4 shows multiple black regions | Stills do not identify transient response or distinguish source pixels from deflection; photograph perspective is not scan displacement |
| B22 type-1 production-module fixture | Existing startup/phase mismatch reproduced; raw timing and RGB can now be replayed together | TV80 substitute and synthetic program, not production-T80 demo history or a measured monitor |
| Full versus Raw pixels | Same acquisition tuple, different byte and raw-vertical-mask policy in source | A picture difference alone does not isolate SHIFT |
| Raw CRT | GA monitor sync and native bytes selected, core processing restrictions retained | Shared scaler/framework may still alter delivery; HDMI PNG is not an analog-pin or CTM observation |
| Amazing upper effect | A supplied two Full boots and one Raw-pixels boot, six frames each, on exact rev2 media; upper logo region identified | Runtime applied latch and matched animation state remain unobserved; comparison is inconclusive |
| Offline phase candidate | Finite causal-state experiment can reposition existing RGB samples with no CPC counter change | No CTM fitting, width response, frequency tracking, vertical response or held-out SHAKER acceptance |

Original-CPC reference examples: [AT/E](https://shaker.logonsystem.eu/images/cpc/CPC/AT_CRTC1_E.webp),
[AY/A](https://shaker.logonsystem.eu/images/cpc/CPC/AY_CRTC1_A.webp),
[D3/A1](https://shaker.logonsystem.eu/images/cpc/CPC/D3_CRTC1_A1.webp),
[D4/A](https://shaker.logonsystem.eu/images/cpc/CPC/D4_CRTC1_A.webp).
Photographs remain ignored and attributed to Logon System/contributors.
The local hardware index is the main checkout's
`docs/specs/shaker-ref-logon/index-cpc_CPC.json`, fetched October 27; API labels
were refreshed October 29. These are distinct evidence dates.

French ACCC v1.11 **§15.7 p.158**, read in its position-aware Markdown and
rendered French source page, describes progressive horizontal line displacement
while the monitor resynchronises, and relates it to short-HSYNC width changes.
It supplies a qualitative mechanism, not a fitted time constant. No new CRTC
rule is inferred from that discussion.

[CPCEC revision `c025aab`](https://github.com/cpcitor/cpcec/blob/c025aab961a796b918cc99bc3e16216ea65bb5d1/cpcec.c#L1142-L1166)
was pinned and read. Its renderer combines width-dependent line placement with
bounded corrections based on HSYNC phase, and contains explicit compatibility
heuristics. This is evidence of an emulator's display-side approximation;
its constants are not hardware measurements. No external code was copied.

## Passive video export

The new opt-in `--trace-video PREFIX` in `sim/plus/b6_dynamic_test.cpp` uses
existing fixture ports; no RTL, top, program, initialization or test oracle changes.
It is independent of the unchanged sparse `--trace-sync` writer.

```sh
mkdir -p sim/plus/obj_dir/b6_dynamic output_files/v1
make -C sim/plus obj_dir/b6_dynamic/b6_dynamic_tests
sim/plus/obj_dir/b6_dynamic/b6_dynamic_tests --machine 1 \
  --trace-sync output_files/v1/sync --trace-video output_files/v1/video
```

Files are `PREFIX-machineN-modeM.video.csv`. `N=0/1/2` means classic CRTC0,
classic CRTC1, Plus; `M=0/1/2` means Full, Raw pixels, Raw CRT. The fixture
runs all three modes from separate initializations. Settings are production
clock divider topology, colour `mix=0`, native pixel rate, no effects/crop or
forced scandoubler. This is not a device-configuration manifest.

Each record is a consistent **pre-edge** snapshot at an integer 64 MHz master
loop tick after fixture initialization. A value holds over `[tick,next_tick)`.
Rows are emitted on observed payload/stage changes; `stage_age` is the age at
that row, not a continuously held counter. The final `terminal=1` record bounds
the final interval; it is not an extra emitted pixel. No observation calls
`eval()` or changes stimulus. RGB changes between native dot enables remain
visible. Decimal columns:

- `rgb12`: native motherboard RGB. Classic uses the low two bits of each
  nibble as `{level, OE_N}` DAC pairs; Plus uses the full nibble. This is **not**
  packed RGB444 colour on classic. Only modes 1/2 disable SHIFT byte compensation.
- `crtc_hs`, `crtc_vs`: CRTC pins; `ga_raw_tuple`: monitor HS/VS, CRTC horizontal
  blank, GA vertical blank, with HS/VS/HB/VB at bits 3..0.
- `full_tuple`, `selected_tuple`: current filter and selected acquisition signals.
- `pixel_vblank`: current motherboard dot tag. The converter delays this tag
  by one `ce_pix` into its mask register; do not apply the same-row tag to
  `color_rgb24` a second time or treat it as that converted pixel's blank tag.
  `color_rgb24/color_tuple` and
  `mixer_rgb24/mixer_de/mixer_ce`: separately labelled downstream observations.
  Those pipeline values must **not** be silently paired with current raw sync.
- Requested/applied mode, stage and stage age retain the actual fixture state.

This export reconstructs the observed time stream, not lost internal CRTC
history, original-monitor scan position, ASCAL frames or physical output.
The sample has 1,281,605 rows per mode through terminal tick 2,563,207 (~40 ms),
roughly 70–71 MiB per mode including mixer cadence. Default runs write no files.
The finite synthetic trace is not enough to fit AT/AY/D3/D4 or Amazing.

## Offline prototype and held-out controls

Ignored `phase_probe.py` is a one-shot hypothesis probe. A fixed 4,096-master-
tick scanner applies 1/8 of each GA rising-edge phase error, capped at 16 ticks
per edge; it holds frequency during missing sync and samples source RGB against
that scan position. These are explicit experimental choices, not CTM constants.
Parameters were chosen on a synthetic phase step, held fixed for independent
steady, missing-sync and extra-pulse controls, then applied to B22 Raw pixels.
The raster illustration uses a simple linear DAC display, not the calibrated
production colour table, scaler geometry or a photograph metric.

Results with that one parameter set:

- Steady input: zero scan offset. A 256-tick step converges to 255.990 ticks.
  Missing ten pulses preserves zero phase in the zero-drift control; that does
  not establish realistic free-running oscillator drift.
- Extra pulses disturb phase by up to 104.249 ticks despite unchanged ordinary
  edges. Thus pulse admission is a material model choice, not a solved detail.
- B22 ordinary and short stages both yield zero model phase offset while
  complete GA monitor pulses change from **256 to 197 master ticks**. The
  rising-edge-only candidate has no width-response input. The raw CRTC short
  pulse remains 320 ticks; do not confuse those two boundaries.
- B22's restore stage approaches 512 ticks of positive source lookahead; the
  most negative observed offset is -28 ticks. The illustration changes 115,761
  sampled pixels. Neither resemblance nor pixel count is an accuracy score.
  This finite trace needs at least 512 ticks of presentation delay for positive
  source lookahead under this mapping, before any other pipeline/buffer margin.

No actual SHAKER pages were used to fit this candidate, so there is **no**
held-out SHAKER pass. Freeze the eventual split before fitting: AT/E–F, AY/A–B
and D3/A1,A11 can form the exploratory subset; retain D3's other three series,
D4 and ordinary steady content as controls. Failure there should reject the
hypothesis rather than introduce title/page constants. Width sensitivity and
multi-pulse admission must be in the candidate contract before that trial.

## Shared Amazing capture result

Session A retained 18 valid MiSTer PNGs: two independent Full boots and one
Raw-pixels boot, six images each near requested 30/45/60-second checkpoints
and adjacent samples. The upper ROI is native PNG **y=0..132**, above the
WELCOME row; lower-screen corruption belongs to V3. Whole frames are retained.
Invalid preliminary BASIC-command attempts are excluded.

The manifest pins `amazing2021_rev2.dsk` SHA-256
`4a631dabab9d8b9a916620a0e0b78f634daeae7736aee3c06b6c0e9979f1e8c9`
and `Amstrad_20261027_925b5bb.rbf` SHA-256
`9ba5e34be1bd6d0eba4d9dcf6a46b686648b6abacef1b093903252d72aed77eb`.
`925b5bb` is an ancestor of the selected source base. CRTC1 and the shared
video path are byte-identical across that span; type-0 uncertainty comments,
simulation observations/tests and documentation differ. Do not claim all files
or artifact names are identical.

Settings were applied through CFG/reset as classic 6128, Plus Off, CRTC1 and
French ROM, with Full or Raw pixels requested. The runtime filter latch was
not read. Stored INI records `direct_video=0`, `vga_scaler=0`,
`forced_scandoubler=0`, `composite_sync=0`; stored configuration is not runtime
connector verification. Scheduling starts after input-command completion;
final Return preceded that completion by approximately one second and was not
individually timestamped. Native scaler-buffer age is unknown. These are near
checkpoints, not exact frame- or final-Return-aligned observations.

Visual comparison identifies the logo wave/flip region but **does not establish
a consistent Full-versus-Raw-pixels improvement**. Logo orientation changes
between adjacent Full shots and between fresh Full boots. An unaligned
best-of-six pixel triage has large residuals even between the Full boots
(12–62/255 mean absolute RGB difference), overlapping Full-to-Raw-pixels
(16–83/255). These are phase-sensitive triage numbers, not accuracy scores.
The user's observation that Full works better remains valid reported evidence;
this asynchronous, requested-mode batch neither confirms nor overturns it.
No Raw CRT run was requested.

A also supplied continuous AmSpirit near-checkpoint images/state/SNA for the
same media and configured CRTC1. The intro scene is diagnostically comparable;
no exact hidden-state or original-CPC correctness claim follows. Session A
reports final original CFG restoration (SHA-256
`f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`),
MENU and no device operation in flight after the Raw-pixels batch; A retains
ownership. Its final shared bundle and cleanup record are under
`local/investigations/classic-video-v2-2026-10-29-b157/`; the originating
subdirectories are `amazing-valid-full`, `amazing-valid-full-boot2`,
`amazing-valid-rawpixels` and `amspirit-v2-amazing`.

## Next bounded slice and interface decision

Acquire a native-byte raw RGB/GA-sync stream for one AT R2 transition and one AY
R3-width case, retaining authentic module startup and navigation. Reuse existing
production-T80/motherboard replay infrastructure rather than make the B22 TV80
program a SHAKER oracle. Align emitted SSM page/scene identity, bus writes and
raw pixels; keep startup and earlier page history, not only a post-page SNA.
Use the identified Amazing upper logo scene for an explicitly phase-aligned
source trace independently of V3's lower-screen diagnosis. Source traces should include both rising and
falling monitor-sync edges, raw CRTC pins and RGB, so width and pulse admission
can be tested without changing the CPC source.

Fit and reject response candidates offline first. Only then specify maximum
horizontal displacement, frequency/holdover behavior, clipping/resampling,
latency and read/write bounds. A line-buffer count is not yet established.
Reserve no new menu encoding in this unit. A future model would use native
bytes once and its own stable presentation rectangle; applying Full SHIFT too
would double-count compensation. A compatible physical CRT must bypass the
model. Simultaneous raw analog and modelled HDMI needs an explicit framework
interface change: current `Amstrad.sv` supplies one core stream to `sys_top`,
whose analog/scaler/direct-video choices do not create a second independent
source. Agree that contract with the other sessions before production edits.

## Verification and retained artifacts

The prior fixture accepted `--trace-video` as an unknown argument but emitted no
video file. The one-shot missing-artifact check failed; after the change all
three CRTC1 modes produce strict monotonic ticks, one terminal row and all six
stages. An independent-writer check reconstructs the video records at every
sparse sync-writer timestamp and matches all eight shared fields plus stage.
Traced/untraced CRTC1 DUT output logs are byte-identical. These are one-shot
checks, not new mirrored tests in the permanent suite.

Final selected gate after the last C++ edit:

```text
python3 sim/select_tests.py --run --slow
select_tests: PASS 1 benches: b6-dynamic
```

All nine dynamic machine/mode cases pass. Canonical soak:
`make -C sim soak SOAK_EXPECT=0xde0a16d0ececc573` reports the same hash and
`soak hash matches expected`. No golden re-mint, synthesis or hardware retest is
needed for this passive simulation-only change; no hardware behavior changed.
The current host builds without the old September SDK include-path workaround.

Private artifacts live under
`local/investigations/sync-monitor-response-2026-10-29-f4da/`: raw CSVs, logs,
API inventories, original-reference contact sheet, rendered French source pages,
CPCEC source pin, workbook row extraction and one-shot analysis/proof scripts.
They remain ignored; source PDFs, media and photographs are not committed.
Fresh read-only DeepSeek review completed CLEAR, run
`20261029T130716Z-47840-66fd`; it checked terminal/age/column integrity and
sub-dot RGB changes against the retained artifacts without rerunning the gate.
Its raw-tag alignment warning is clarified above. Unknown-option handling
remains the pre-existing permissive parser: verify output existence and the
terminal row rather than treating process exit alone as trace delivery. The
review's identical-RGB observation applies only to this unacquired type-1
trace; it does not prove filter independence when Full SHIFT is active.

This bounded unit stops at committed READY with source/monitor evidence gaps
explicit. Shared Codex quota reached 89% five-hour / 47% weekly used during
final handover; no new investigation, delegation or device batch was started.
No integration, push, publication, paid overage, reset redemption or wakeup.
