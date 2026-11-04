# SHAKER DT: black pixels and acquisition boundary — 4 November 2026

## Finding

The sync filter can ignore an additional mid-frame CRTC VSYNC when regenerating
output timing. That makes it a plausible contributor to DT geometry, but does
not establish a filter defect. The classic GA produces vertical black pixels
before this filter. Suppressing a second acquisition VSYNC does not itself erase
a second GA black interval that falls within the acquired picture.

Missing raw CRTC VSYNC, a different GA blank chronology, and downstream placement
or clipping remain distinct explanations. A screenshot showing one black region
instead of two cannot distinguish them. Do not assign DT to the filter from
shape alone.

This is a read-only source investigation. No production/test edits, simulations,
gates, builds, devices, emulator/API calls, commits, pushes or delegation were
performed. The only new artifacts are this report and the French-page render
under `local/overnight-20261103/asic/dt-boundary/`.

## Evidence and limits

The parent obtained CPCSyntaxError v0.12 CRTC1 DT captures at callers 02F8 and
02FA under `local/overnight-20261103/cpcse/dt1-v012/`. Their two horizontal black
regions resemble the original CPC photographs. The baseline MiSTer capture
`local/overnight-20261103/package/d1-device-5fba168/MISTER_1_02F8.png` has one.
The comparison inspected here is `package/dt-f8-compare.png`. Original references
are cached in the main checkout at
`local/investigations/shaker-sweep-2026-11-03/ref/cpc_CPC/CRTC1/DT_02F8_G2.webp`
and `DT_02FA_H2.webp`.

The baseline manifest records **requested** Full sync filtering. Its applied
mode, pixel-rate, scale, mix and native-cadence fields are null: there was no
runtime readback. Do not treat applied Full as measured. This matters because
Raw CRT selects different acquisition geometry. The requested mode alone is
insufficient to prove which stream reached the scaler.

The inspected shared HEAD was `dd247d4e221f70bd3cd0ea6dae6e981bfed87238`.
`git diff 5fba168 HEAD` reported no changes in `rtl/crt_filter.v`,
`rtl/amstrad_video_color.sv`, `rtl/amstrad_video_output.sv` or
`rtl/GA40010/syncgen_sync.v`; those boundary facts also apply to the captured
baseline. This is not proof of identical complete title execution.

## Source authority

French ACCC v1.11 §§16.2.1 and 16.2.3, pp.161 and 167, distinguish GA vertical
black from the shorter monitor vertical-sync signal. A CRTC VSYNC rising edge
starts the vertical black sequence; its counter advances on CRTC HSYNC falling
edges. The algorithm gives monitor VSYNC at counter2..6 and black through
counter26. Repeated resets and actual input chronology therefore matter; counting
monitor pulses alone is not counting separate visible black intervals.

Navigation used `scripts/accc/lookup.py`; verification reused the existing
pdf-inspector French primary Markdown and inspection report. The relevant
algorithm was rendered and read visually as
`local/overnight-20261103/asic/dt-boundary/fr-167.png`. These sections support the
separation of black and monitor sync; they do not establish the exact DT caller
waveform without a trace.

## Actual production path

| Stage | Actual connection or behavior | Consequence for DT |
|---|---|---|
| Classic CRTC to GA | Motherboard connects `crtc_hs`/`crtc_vs` directly to GA `HSYNC_I`/`VSYNC_I`. | Raw vertical events remain a possible first divergence. |
| GA vertical counter | `syncgen_sync.v` resets upper encoded `hcnt` bits on `VSYNC_I & ~vsync_d`; `hcnt_cnt` is the raw HSYNC falling-edge detector. `VSYNC_O` and `HCNTLT28` are separate decodes. | There is no 260-line acquisition qualifier on this raw black path. |
| GA DAC blank | `ga40010.sv` connects `HCNTLT28 | HSYNC_I` to `video_sync.FORCE_BLANK`; `video.sv` clears DAC pairs on FORCE_BLANK. `VBLANK=HCNTLT28`. | Vertical black changes pixel values upstream of sync filtering, across border and display. |
| Filter input | Motherboard connects selected raw CRTC `hs_sel`/`vs_sel`, not GA-shaped monitor sync, to `crt_filter`. | Filter acquisition and GA monitor sync are different consumers of the same raw source. |
| Filter acquisition | In `crt_filter.v` syncgen, a sampled VSYNC rise resets `vSyncCount` only when `vSyncFlt > VFLT_SZ`, with `VFLT_SZ=260`. | An extra mid-frame pulse can be rejected for acquisition. Behavior depends on history and sampling; not every second pulse is universally collapsed. |
| Filter blanking | Blankgen resets `vborder` on regenerated VSYNC, clears VBLANK at row30/hborder49 and sets it at row302/hborder241. | Its fixed acquisition window can relocate or exclude raw black rows. It is not the GA26-line black mask. |
| Mode selection | Full and Raw pixels select the same filtered HS/VS/HBL/VBL tuple. Raw CRT uses GA monitor HS/VS, raw CRTC horizontal blank and GA vertical blank. | Raw CRT is a geometry experiment, not a clean pixel-only comparison. |
| Pixel conversion | `color_mix.sv` converts zero classic DAC pairs to calibrated near-black (`000201` or `020702` depending mix), while passing VBLANK separately. `pixel_vblank` enables an exact-zero post-conversion mask in non-Full modes. | A black threshold or raw-mask difference is not proof that a sync event is absent. |
| Acquisition/output | `video_mixer.sv` derives DE from HBL/VBL. `amstrad_video_output.sv` exposes the converted tuple before gamma/scandoubler/HQ2x/crop; final output then passes mixer and crop to the external scaler. | Both pixel values and acquisition placement must be observed before blaming the scaler or filter. |

Relevant consumers were checked in `rtl/Amstrad_motherboard.v`,
`rtl/GA40010/{syncgen_sync.v,ga40010.sv,video.sv}`, `rtl/crt_filter.v`,
`rtl/{color_mix.sv,amstrad_video_color.sv,amstrad_video_output.sv,video_interlace.v}`,
`sys/video_mixer.sv` and the top-level video-output wiring in `Amstrad.sv`.

## Matching CPCSyntaxError distinction

The local source at `/Users/renaudg/code/CPCSyntaxError` was clean at
`22de765ef44da46c55217e4dd7caa385d69a6045`, also `v0.12.0^{commit}`. Its
`GateArray::onVsyncStart()` starts `cblackVsync`; raw HSYNC counting clears it at
26. `blanking()` exposes that black state. `emulator.cpp` records it as
`RasterLine::gateArrayBlank`.

`video.cpp::paintBorderScanline()` explicitly paints an entire black line when
that flag is set, and the character renderer skips over it. The beam renderer
also reads GA blanking independently. Monitor/flywheel handling separately
ignores spurious mid-frame VSYNCs (`beamVsync()` requires at least260 rows before
presenting). The legacy monitor path has its own composite integration; this
source inspection does not assert which renderer alone produced the saved BMPs.

Thus software can ignore an extra monitor retrace while still drawing two raw
GA black regions. Its matching pictures do not by themselves prove either the
FPGA's raw event chronology or its filter geometry is wrong.

## Minimal first-divergence discriminator

Record one authentic DT caller cycle with enough preceding lines to establish
filter lock and GA counter state. Preserve the caller/page identity and actual
applied filter/scale/pixel-rate mode. No further generic mode sweep is needed.

1. At the source boundary, record `crtc_vs`, `crtc_hs`, relevant R7 writes and
   C4/C9. Determine whether both expected raw VSYNC events exist, rather than
   inferring them from filtered output.
2. At the GA boundary, record `hcnt_res1`, `hcnt_cnt`, encoded `hcnt_reg/hcnt`,
   `HCNTLT28` (`ga_vblank_o`), `ga_vsync_o`, and raw DAC pairs. Count **separate
   black intervals**, since a retrigger before completion can extend one interval.
3. At acquisition, record `vsync_filtered`, `vblank_filtered`,
   `vSyncFlt`, `vSyncCount` and `vborder`, plus HSYNC qualification state. The
   existing `ssm_sync_observation` exposes useful horizontal state and applied
   mode, but not these vertical counters; it cannot settle this alone.
4. Correlate the pre-postprocessing `obs_*` RGB/HS/VS/HBL/VBL/CE tuple and native
   cadence with final VGA/HDMI DE and visible REP rows. Preserve RGB values as
   well as a black classification to avoid confusing calibrated black with a
   missing interval.

| First observed divergence | Boundary to investigate |
|---|---|
| Second raw CRTC VSYNC absent | CRTC/caller/R7 behavior, before filter. |
| Two raw VSYNCs, but only one continuous or absent second GA black interval | GA/input chronology; inspect resets and raw HSYNC count before calling it a defect. |
| Two raw black intervals, second excluded or relocated by acquisition DE | Filter/crop/scaler/monitor geometry candidate. |
| Both black intervals inside active pre-postprocessing tuple, one missing at final output | Downstream processing/scaler/capture boundary. |

Recommended next action is that single boundary trace, not a filter correction.
The source establishes a credible acquisition hypothesis and an equally live raw
event hypothesis; the current screenshots do not choose between them.
