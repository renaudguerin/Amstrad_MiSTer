# SHAKER DT: black pixels and acquisition boundary — 4 November 2026

## Current finding — corrected 5 November

**The late native one-band image does not confirm a DT core or filter defect.**
The original guest removes the second band about339ms after F8, and late
same-run AmSpirit also shows one band. The native PNG associated with F8 was
requested after F7 had already been published. The [capture-epoch correction](#capture-epoch-correction--5-november)
below supersedes the earlier early-reference/late-native comparison. A passive
first-window native trace remains useful; no filter repair is selected.

## Source boundary finding

The sync filter can ignore an additional mid-frame CRTC VSYNC when regenerating
output timing. That makes it a plausible contributor to DT geometry, but does
not establish a filter defect. The classic GA produces vertical black pixels
before this filter. Suppressing a second acquisition VSYNC does not itself erase
a second GA black interval that falls within the acquired picture.

Missing raw CRTC VSYNC, a different GA blank chronology, and downstream placement
or clipping remain distinct explanations. A screenshot showing one black region
instead of two cannot distinguish them. Do not assign DT to the filter from
shape alone.

**The private replay below is rejected as a phase-faithful DT discriminator.**
It produces two upstream black intervals, one outside Full acquisition, but a
fresh CPCSyntaxError register trace proves its original caller has a different
vertical write phase. The loop bypasses its initial synchronizer on subsequent
iterations, so waiting for repetition cannot recover the missing classic SNA
counters. Those replay measurements describe the wrong-phase local execution;
they do not establish a device defect or narrow its cause to acquisition placement. The final
correction below records the concrete mismatch.

The initial source pass used no execution. A separately authorized follow-up
used an isolated headless CPCSE CLI and an ignored production-motherboard/T80
simulation. No production/retained-test edits, general gates, RBF builds,
devices, GUI/API access, commits, pushes or delegation were performed.

## Evidence and limits

The parent obtained CPCSyntaxError v0.12 CRTC1 DT captures at callers 02F8 and
02FA under `local/overnight-20261103/cpcse/dt1-v012/`. Their two horizontal black
regions resemble the original CPC photographs. The baseline MiSTer capture
`local/overnight-20261103/package/d1-device-5fba168/MISTER_1_02F8.png` has one.
The comparison inspected here is `package/dt-f8-compare.png`. Original references
are cached in the main checkout at
`local/investigations/shaker-sweep-2026-11-03/ref/cpc_CPC/CRTC1/DT_02F8_G2.webp`
and `DT_02FA_H2.webp`.

The baseline manifest's top-level `applied_b6_config` fields are null, but
its per-marker format-2 SSM records provide runtime evidence: both02F8 and
02FA have `requested_mode=0`, `applied_mode=0` (Full), line estimate256,
`shift=false`, `no_hsync=false`, `hsync_mask=true`, `arm=false`, and three
training syncs. Parent acceptance checked these actual records. Thus Full
is measured at the markers; the null summary must not be mistaken for an
absence of all runtime readback. Pixel-rate/mix/native cadence, the later
PNG's exact phase, and its vertical lock anchor remain unmeasured.

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

## Private original-caller replay

Artifacts are entirely under
`local/overnight-20261103/classic/dt-boundary-proof/`. `commands.txt` preserves
the commands; `capture-provenance.json`, `rom-hashes.json`,
`snapshot-header.json` and `replay-provenance.json` pin the inputs. No historical
capture or caller media was modified.

The known-working `dt1-through-fa.CSL` was copied and stopped immediately after
its `wait_ssm 0x02F8`, followed by the documented instructions:

```text
snapshot_version 3
snapshot_name 'dt-02f8-running'
snapshot
```

This invokes the original D module from disk, including its canonical prior T
pages. It does not start an initializer from a later snapshot. The entire app
was copied into task scratch so the GUI's settings were not shared. The pinned
v0.12 executable hash remains
`f4254653b11676055eedd89dcdae6c6aab381835344d69780f56fcbff218a577`;
disk hash is `6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
Machine was CPC6128/CRTC1, `--no-chain --no-errata`, with the copied packaged
firmware. The CLI completed with:

```text
[snapshot v3] -> .../capture/dt-02f8-running.sna  t=20.782s
done: 11 screenshot(s), 1 snapshot(s)
```

This is a running G caller captured after receipt of F8, not the instant its
image was rendered: F8 was saved at20.402s and received by the script at20.782s.
The module also emits F7 during continued execution; page markers are not a
guarantee that dynamic caller operands remain frozen forever.

The saved file is a131328-byte uncompressed standard-v3 SNA, magic `MV - SNA`,
SHA256 `bcbb2ea83508703b4c08bd6c1cf52221bcf7f471b0f0a6605a03dcf0e751ecfb`.
Its CPU state has PC=A2F6, SP=4061, GAconfig=8E, RAMconfig=0, CRTCtype1,
R0/R4/R7/R9=63/38/30/7, and C0/C4/C9=16/32/0. Both ROMs are disabled, so the
continued caller uses the saved RAM. The private CPCSE extension is present in
unused header space and is not interpreted by the FPGA loader.

### Restore and synchronization limits

The wrapper derives from `sim/b18_snapshot_top.sv`, uses the actual motherboard,
production classic GA/CRTC/filter and generated production T80, and supplies
byte RAM plus real saved VRAM words. It adds passive output taps. The copied
motherboard adapter only maps generated T80 port spelling and supplies its
OUT0/R800_mode constants; it changes no behavioral expression. Source copies
were pinned at HEAD `d50fee32dcb4447356034000c424817796eefa25`, with per-file
hashes. CPU state, PC and every RAM byte were consumed unchanged.

**The classic CRTC does not restore these standard-v3 counters.** Its production
counter block has reset and CLKEN branches, without a SNA counter-load branch;
the corresponding motherboard classic owner has no counter-load interface.
The observed restore was therefore PC=A2F6 as saved, but C0/C4/C9=0/0/0 rather
than16/32/0. GA V26 and filter lock history are also absent from the standard
snapshot. No private reseed, PC redirect, RAM patch or initializer restart was
used to conceal that gap.

Consequently the restored prefix is not evidence for the original capture's
phase. Evidence below is limited to the caller's repeated forward execution,
which reaches identical accepted-write/counter subcycles. It supports a settled
local boundary mechanism, not bit-identical continuation from the snapshot or
the historical device's lock choice.

The first12-frame run was preserved as `run-prefix12/`. Reading the original
caller bytes exposed a self-modifying countdown/intermediate R7 operand, so the
final run covered64million master clocks (1s), including both observed operand
states. No external input was injected. `run/markers.csv` is only a syntactic
ED/byte scan and includes real OUT/LDI pairs; only known DT marker candidates
are separately listed. It is not a general SSM decoder.

### Measured repeated chronology — phase-incompatible, see correction

`run/io.csv` records the production CRTC register-write level's first acceptance
edge; C0/C4/C9 here are the values at that edge, not CPU instruction-start
estimates. After warmup, these positions recur unchanged:

| Instruction PC | Register/payload | Accepted C0/C4/C9 |
|---|---|---|
| 69F1 | R7←10 | 61/6/7 |
| 69FC | R7←30, or the caller's other operand14 | 12/17/0 |
| 6A07 | R7←30 | 27/27/0 |

The14 payload is already behind C4=17 and did not create an additional raw
VSYNC in this replay. Both operand states still produce raw VSYNCs at
C0/C4/C9=0/10/0 and0/30/0. This was observed, not inferred from the register
values. Final execution completed:

```text
RESTORE PC=a2f6 expected=a2f6 C0=0 C4=0 C9=0 R7=30 applied_filter=0
DONE fetches=620329 writes=150 raw_vsync_rises=100 ga_black_rises=100 pc=a2f4 clocks=64000000
STEADY cycle_ticks 1277952 lines 312.0 rawVS 2 GAblack 2
```

`run/events.csv`, `run/lines.csv`, `cycle-summary.json` and `analysis.log`
preserve raw CRTC HS/VS, GA hcnt/black/monitor VS and DAC pairs, actual filter
vertical counters, and motherboard HBL/VBL. One late complete312-line cycle
starts at tick60391424; times below are relative to that raw row10 VSYNC:

| Relative tick | Observation | Acquisition state |
|---:|---|---|
| 0 | Raw VSYNC rises at0/10/0; GA black starts. | VBLANK already1; vSyncFlt311. |
| 7183 | Regenerated filter VSYNC rises. | vSyncFlt reset; vSyncCount1. |
| 7428 | GA monitor VSYNC rises at52/10/1. | Filter vborder resets to0. |
| 105732 | First GA black interval ends at52/13/1. | VBLANK remains1 throughout this interval. |
| 130879 | Full acquisition VBLANK clears. | First GA black interval has already ended. |
| 655360 | Second raw VSYNC and GA black start at0/30/0. | VBLANK0, HBLANK0 at this edge; vSyncFlt159. |
| 662788 | Second GA monitor VSYNC rises. | No regenerated filter VSYNC; VBLANK remains0. |
| 761092 | Second GA black interval ends at52/33/1. | VBLANK remained0 throughout this interval. |
| 1248063 | Full acquisition VBLANK reasserts. | Before the following row10 pulse. |

There are **two distinct upstream black intervals**, each105732 master clocks
from its raw VSYNC start to the26th raw HSYNC falling edge. The first starts and
ends entirely outside Full acquisition; the second is inside its vertical
window. Raw DAC pairs are zero at the recorded black events; the production
FORCE_BLANK path explains that color independently of output metadata.
HBL/VBL here are actual motherboard outputs, not downstream VGA_DE/HDMI traces.

This is an acquisition-boundary explanation for one acquired band despite two
upstream intervals in the local model. It is stronger than the screenshot-only
hypothesis, but not a confirmed filter defect: the fixed acquisition window,
chosen sync anchor and physical monitor placement still need comparison. No
complete scaler or monitor image was reconstructed. The Full vertical filter
source is unchanged between5fba168 and this copied HEAD, but the complete
replayed classic CRTC is current HEAD, not an exact5fba168 hardware replay.

### Source comparison and next action

CPCSE's matching source separately supplies GA black to its renderer while its
monitor consumes composite C-SYNC (`emulator.cpp::advanceClassicMonitorCharacter`).
`monitor_renderer.cpp::vSync` can refuse early pulses before its oscillator
window. Its black painting therefore need not disappear when its monitor
refuses a sync. The renderer and the MiSTer fixed VBLANK aperture are different
geometrical models; a matching CPCSE photograph does not validate a transplant
of its flywheel or a new FPGA counter rule.

Recommended next action is to correlate raw event/black counts with the vertical
Full acquisition tuple and anchor on the device at the same G caller, including
preceding lock history. Marker-time applied Full is already recorded; that does
not supply the missing vertical counters or screenshot-time alignment. If the device also has two raw black intervals, DT's
missing band should be pursued at acquisition placement before changing its
CRTC/GA. If it does not, compare accepted R7 edges before generalizing this
settled replay. No production change is proposed. All private processes exited;
the headless CLI ownership is returned to the parent.


## Fresh-reference phase correction and bounded device comparison

The parent ran the supplied unmodified v0.12 binary with its built-in register
trace, using the identical bounded T script and firmware:

```sh
CPCSE_TRACE_REG=7 CPCSE_TRACE_REG_MAX=8000 <cpcse> \
  --csl local/overnight-20261103/cpcse/dt1-through-fa.CSL \
  --out local/overnight-20261103/cpcse/dt1-reg7-v012 \
  --disk-dir local/test_media/shaker --roms <packaged-roms> \
  --model cpc6128 --crtc 1 --no-chain --no-errata
```

It completes13 captures, exit0. Target02F8 is byte-identical to the prior fresh
CPCSE capture (SHA256 `60d2e729ad929c5a36118f60e8e615f4d82e05b58e0800c3e2595590f6447313`).
In a representative original G iteration, the actual reference writes are:

| Opcode-start PC | Payload | Fresh CPCSE C0/C4/C9 | Rejected replay C0/C4/C9 |
|---|---:|---|---|
| 69F1 | R7=10 | 12/0/0 | 61/6/7 |
| 69FC | R7=14 | 27/10/0 | 12/17/0 |
| 6A07 | R7=30 | 42/20/0 | 27/27/0 |

CPCSE logs its CPU.pc after the two-byte OUT; its printed69F3/69FE/6A09
therefore correspond to the opcode-start PCs above. This is a large vertical
phase difference, not a logging-edge tolerance. The actual binary calls its
synchronizer at69E0, but the later loop's `JP69E3` at6A2D bypasses that call.
The earlier SNA resumes inside the delay, after this one-time synchronization.
Its missing counter restoration can therefore persist indefinitely. Repeated
312-line execution does not make this a valid reference-aligned caller.

In particular, the rejected replay writes14 after C4 has already reached17,
whereas fresh CPCSE writes it at10. The two traces do not even offer the same
opportunity for a later R7 match at14. **Withdraw the inference that the missing
hardware band is explained by one of these two replay intervals being outside
Full acquisition.** No new hardware rule or filter repair follows from it.
Exact evidence is `cpcse/dt1-reg7-v012.log` and
`classic/dt-boundary-proof/fresh-reference-phase-correction.json`.

The parent also ran the unchanged bounded script on3113323 in Full and Raw CRT:
each completes32commands/14captures/zero failures/14SSM records. Target02F8/02FA
marker observations confirm applied modes0 and2 respectively. Full reproduces
the single visible band; Raw CRT yields severely cropped native frames and
cannot provide a clean count of upstream bands. This is a negative discriminator,
not a filter pass/fail. Both manifests verify original CFG restoration. A final
four-command reset run restores runtime Full on classic CRTC1/BASIC; no hardware
job remains active. Device artifacts are `package/dt1-device-3113323-{full,raw-crt}/`
and `package/device-full-cleanup/run/`.

The useful next proof starts before the original one-time synchronizer, or uses
an independently verified complete classic counter restoration. Compare actual
accepted R7 phases and raw VSYNC/GA black before acquisition. Until then, DT
remains unassigned between functional timing and display acquisition; the source
boundary analysis above remains valid, while the wrong-phase causal replay does
not establish the device's mechanism.

## Authentic synchronizer and reported F8 variant — 5 November

The original pre-sync caller now reproduces the reference phases without any
guest patch or forced counter state. A normal-boot AmSpirit1.17.1/core2491682
run uses the same6b9 disk and65a9 D payload on6128/CRTC1/128K/French. At69E0,
the source/SMC/stack-return6943 guards pass and live registers are
`63,40,46,54,38,0,26,30,0,7,0,0,48,0`. This is before the original CALLA241.
The streamed classic snapshot is v2: hidden counters and its v3 CRTC-type
extension remain unavailable. Its original128K RAM/header prefix is preserved;
the trailing SPRT emulator-history chunk is excluded from the private loader
input without changing any header/RAM/PC byte.

Current coherent RTL and a current-source GHDL T80 translation replay that
caller from two naturally different starting phases. With the CPU held through
the existing SNA pause, fixed0/50,000-clock settling gives measured initial
C0/C4/C9=0/0/0 and13/1/4. A241 converges both to accepted R7 writes at
12/0/0,27/10/0,42/20/0. Instruction starts are C0=9/24/39; the earlier table's
12/27/42 values describe the accepted bus writes. Subtracting2 from CPCSE's
logged PC identifies the opcode address, not its instruction-start clock.

The first69E0 entry is REP0A and initially emits02F0, rather than the reported
02F8. Byte-checked tableB086 advances through F0,F1,F3,F6/F5,F8/F7 as the
original index advances0A..0E. A long first-REP run alone does not reach the
reported variant. The new target proof follows the canonical four Space
presses after F0,F1,F3,F5, using the existing PS2 input and70ms down/up pacing.
It guards the actual retired A385/A387 pair, following A389 RET fetch and
every marker/index. No host writes to guest memory, CPU PC or counters occur.

All six expected markers and indexes pass in one private run, ending two
frames after genuine F8 at index0E. Its accepted R7 sequence is10→14→30 at
the same12/0/0,27/10/0,42/20/0 phases. The source/input hashes remain frozen.
The correctly phased312-line cycle now has three raw VSYNC starts and three
GA black intervals:

|Raw C4 at start|GA black duration, physical-line units|Motherboard Full acquisition|
|---|---:|---|
|10|25.8125|Contains HBL=0/VBL=0 samples|
|14|25.8125|Contains HBL=0/VBL=0 samples|
|30|25.8125|Wholly inside acquisition VBL|

Thus both middle black bands reach the current simulated motherboard's
acquisition window. This does not establish the final native scaler image or
its acquired state, but supplies no basis for a pure filter diagnosis from
the earlier wrong-phase replay.

Root also continued the same genuine AmSpirit state through the canonical
inputs to F8. The original screen shows both bands;69C8/69FA are0E and the
next three R7 stops remain5,135µs apart. All four live images were inspected.
Beam coordinates are output coordinates, not measured hidden CRTC counters.

The last native one-band check is3113323, which predates the independently
reviewed CPU reset repair9de75d3 and later classic changes. Recheck the exact
current production build with the original bounded script before adding a
native R7-phase/acquisition observer. Neither a functional repair nor a filter
repair is selected from this result. No broad baseline was launched.

Evidence: `resumed-20261104/dt-authentic-prefix-prep/`, including genuine
`original-run-001/`, `original-f8-continuation-001/`, both natural-phase
controls, and `csl-to-f8-replay/target-f8-cycle-summary.json` with source,
retired-marker, key, raw-event and accepted-I/O ledgers. Earlier compile,
stream-format and observation-composition setup failures are preserved and
qualified separately; none is a functional hardware result.

## Fresh native run on current code tip e24cb50 — 5 November

The original bounded `dt1-through-fa.CSL` walk was executed on the exact production artifact
`Amstrad_20261104_e24cb50.rbf` under Full mode (`CRTC1`, French firmware, disk `6b9`). The run
completed all 32 commands, 14 captures, 0 failures, and 14 format-2 SSM records.

All 14 captures were root-viewed; targets 02F8 and 02FA were full-viewed. **Both 02F8 and 02FA
reproduce the single visible black band on native hardware**, matching the older 3113323
appearance:
- `MISTER_1_02F8.png` SHA256: `19a03408cb5b24613dfdc694a3058843838d1d0edef3a60b74a1da7b704dfa58`
- `MISTER_1_02FA.png` SHA256: `4df552a638871216346f9f400a9a56d9c3c0f931872da2b0fb685d389bce24c2`

Marker readback confirms requested mode 0 and applied mode 0 (Full), `shift=false`,
`no_hsync=false`, `hsync_mask=true`, `arm=false`, and 3 training syncs. The horizontal line
estimate is `line_estimate_ce4=256` CE4 ticks (4 MHz, hence a 64 µs horizontal scanline, not
256 vertical lines). Artifacts are preserved under `dt-authentic-prefix-prep/device-dt-full-e24cb50/`.

This confirms the late single-band appearance on the current artifact after the
CPU reset and recent CRTC repairs. It does not qualify the early F8 epoch: the
following capture-timing check withdraws the earlier comparison with an early
AmSpirit image. No functional or filter repair follows from this screenshot.

## Capture-epoch correction — 5 November

The complete existing original-T80 replay provides the missing time dimension.
Genuine F8 occurs at tick79,896,647. The guest's write at PC6A1C changes its
second-R7 operand69FA from0Eh to1Eh at tick101,617,919,339,394.875µs after F8.
The first17 iterations have raw VSYNC at C4=10/14/30; the last3 have10/30.
Genuine F7 occurs at105,455,687:399,360µs, or20×312-line frames, after F8.
The row14 black band therefore belongs to a short early interval rather than
a stable screen that persists until host capture.

Across that replay, row10/14 GA-black intervals each last105,728 master ticks
and overlap motherboard VBL=0 throughout; row30 black overlaps VBL=0 for zero
ticks. These later event-ledger overlaps are not per-clock HBL/RGB counts.
The first bracket separately measures actual active black samples; neither
measurement is a final HDMI/scaler trace.

Root continued the **same** paused normal-boot AmSpirit1.17.1/core2491682 caller
without reset, input or guest-state writes. Waiting for genuine F7 and pausing
404,568µs after the earlier stop leaves index0Eh and operand69FA=1Eh. The live
uncropped image has one band, with REP0F..14 visible, qualitatively matching the
late native layout. Its SHA256 is
`55791f0ccc546813da0f88c0bc854fe1539c90db6ca87e8414a6944f4914cea0`.
The earlier first-F8 image had two bands. AmSpirit was left paused in that late
state with script and breakpoints clear at this check; subsequent experiments
may change the active emulator state.

The native manifest independently exposes the acquisition delay:

| Observation | Native evidence |
|---|---|
| F8 / F7 core ticks |138,027,009 /163,586,049:399,360µs apart|
| Key transport |00:23:42.068810–00:23:45.631 UTC|
| First relevant ring poll |00:23:45.631819; both F8 and F7 returned in the same batch|
| F8-labelled screenshot command |00:23:50.445968 UTC, after F7 was already known|

The F8-labelled PNG is a marker association, not an image captured inside the
sub350ms early window. **Withdraw the claimed native-versus-AmSpirit DT
core/filter divergence from this comparison.** This correction is independent
of the earlier rejected wrong-phase SNA replay; both limitations must be retained.
Private evidence is `resumed-20261104/dt-temporal-window-qualification/`, including
`RESULT.json`, `amspirit-after-f7/` and the original native manifest above.

## Qualified next operation

A default-off native observer now captures the first qualified post-F8 bracket:
three M1 prefix-completion cuts, three actual raw-VSYNC ordinal cuts, independent
VS/GA-black counts and actual active-black master ticks at the motherboard
boundary. Its bounded window remains useful because SSH screenshots cannot
resolve that early epoch. Missing raw edges are reported by ordinal and valid
mask, never assigned an assumed row14. It reuses the seven-slot S storage and
publishes only after genuine F7 and the ordinary header acknowledgement.
The offline decoder rejects stale sequences and accepts honest invalid-tail
partials without falsely qualifying their elapsed endpoint.

The retained asset-free transport/host proof passes; final
`python3 sim/select_tests.py --run` reports46 selected benches, and
`PYTHONPATH=scripts/hardware-loop python3 -m unittest test_csl_runner test_ssm_ring`
reports149 tests OK,6 skipped. Source guards keep both-off, S-only and
DT-without-S behavior token-identical. The real-T80 enabled/disabled replay
preserves ordinary markers and machine/bus ledgers; its frozen source and the
subsequent partial-freeze projection are recorded in
`dt-passive-candidate/` and `dt-passive-decoder-followup/`.
The narrow foreign audit found two host decoder defects, both corrected with
actual producer-generated failing-first fixtures. Fresh final host review found
no additional defect; root accepted its narrow scope after reading the diff.
These are diagnostic/host changes, not a GA/filter repair.

Native synthesis, timing/fanout and early-packet acceptance remain separate
gates. An agreeing early packet would qualify that window, not prove final
HDMI behavior or a universal monitor model. The physical device remains on
`e24cb50` Full CRTC1 DT02FA; no device job is active. Broad baseline deferred.
