# Sync response and classic demo work plan

Requested by Renaud, 2026-10-29. This extends backlog B1/B22 and the existing
DSC4/Amazing residuals; it does not reopen the completed Module-D investigation
as an established CRTC defect. No new device captures or RTL changes were made
while preparing this plan. The three work packages below are READY for investigation.

## Symptom separation and priorities

Renaud's 2026-10-29 clarification defines three independent investigation paths:

| Path | Reported observation | Working hypothesis and ownership |
| --- | --- | --- |
| V2: DSC4 | Display problems do not vary with sync-filter setting | Investigate CRTC/GA execution, addressing and pixels first; CRTC/GA repair is the user's hypothesis, not an established cause |
| V1: Amazing top / SHAKER sync effects | Amazing's top effect changes with the filter and currently works better in Full on HDMI | Sync/output presentation investigation, alongside SHAKER AT/AY/D3/D4 |
| V3: Amazing bottom | Flashing corruption in the lower part | Investigate a possible CRTC/GA defect separately from the top effect; shared cause with DSC4 is unproven |

DSC4 repair does not depend on completing V1. Amazing's top and bottom need
separate captures, diagnoses and acceptance verdicts; improving the top effect
does not close lower-screen corruption. Shared capture infrastructure is useful,
but neither title is a prerequisite for the sync-response design. Independent
source/simulation work can proceed concurrently with explicit file ownership;
only MiSTer device access must be serialized.

## Analysis: signal accuracy and monitor response

The recommended direction is an accurate CRTC/GA signal source, with an explicit
choice of output presentation. A single menu default could provide useful
compatibility, but should not promise faithful original-monitor behaviour on
both a physical CRT and scaled HDMI for malformed sync.

There are three separate contracts:

1. CRTC and GA generate counters, addresses, RGB, blanking and monitor sync.
2. A physical monitor converts that time stream into a spatial picture, with
   its own lock, phase, flyback and deflection response.
3. MiSTer's scaler captures pixels into a digital raster and presents HDMI.
   Acquisition geometry and display response are different operations.

**Engineering inference:** when source sync moves but a CRT's horizontal scan
does not follow instantaneously, successive lines can be displaced differently.
Drawing the RGB time stream against a simulated scan position can reproduce a
slant on a stable digital raster. This need not invent new CPC pixel data: it
maps existing samples to different display coordinates, with explicit blanking,
clipping and resampling. Missing-sync holdover also requires state. This does
not establish which mechanism produces each requested SHAKER effect; AT, AY,
D3 and D4 must be mapped to exact test pages and raw traces before classifying
them. Horizontal phase lag alone may be insufficient for some pages.

Different CRTs, including a generic RGB television and an Amstrad CTM, can have
different responses. Analog connector availability does not establish a CTM
oracle. A native MiSTer screenshot reads the scaler buffer and cannot validate
analog pins, the connected CRT, or their transient response.

**Emulator source check:** the inspected [CPCEC video loop](https://github.com/cpcitor/cpcec/blob/master/cpcec.c#L1115-L1151)
places scanlines using horizontal timing state, then applies bounded corrections
to that state based on sync phase error. This confirms an explicit display-side
timing approximation in that source, rather than a CRTC-only raster. It does
not establish AmSpirit's implementation or validate CPCEC's approximation
against the requested SHAKER pages. The link follows master; pin its revision
before using it as an implementation reference. No external code was copied.

### Current source boundary, inspected in this checkout

- `rtl/CRTC.v` and the type engines own CRTC rules. The GA shapes monitor
  sync separately from CRTC pins; raw analog should preserve the intended
  CPC monitor-facing signals rather than blindly expose CRTC pins.
- `rtl/Amstrad_motherboard.v` feeds `crt_filter` from `hs_sel`/`vs_sel`.
  The output selector uses GA-shaped sync for Raw CRT, raw CRTC HSYNC for
  horizontal blank and GA vertical blank. Full/Raw pixels share filtered
  HS/VS/HBLANK/VBLANK; only their pixel/byte policy differs.
- Full's `crt_filter.SHIFT` also affects VRAM byte assembly. Thus today's
  filter is not purely a final sync conditioner, and an arbitrary tap before
  the selector does not guarantee uncompensated source pixels.
- `rtl/crt_filter.v` suppresses closely spaced HS pulses, learns a line
  period after VSYNC, regenerates pulses and derives fixed acquisition blanking.
  Its `SHIFT`/`hs4` compensation is not a general scan-position response model.
- `rtl/amstrad_video_output.sv` carries the selected stream through colour,
  mixer and crop. Raw CRT disables core post-processing. `sys/sys_top.v`
  derives both native analog and scaler input from the shared core stream;
  `vga_scaler`, direct video and other framework settings alter the final route.
  There is no independent raw-analog plus monitor-model-HDMI stream today.

Existing [B6 design](b6-video-boundary.md) deliberately excluded a monitor model
from that implementation slice. This new request authorizes investigating one,
not silently treating the old slice as incomplete or implementing a second
renderer without a design decision.

### Recommended architecture to investigate

- Preserve accurate CRTC/GA timing independently of presentation; never change
  counters or CPU-visible sync to make a filtered picture resemble a photograph.
- Retain a raw monitor-signal path for a compatible CRT. Feeding a digitally
  simulated CRT response to a physical CRT risks applying the response twice.
- Investigate a separate digital presentation profile: stable capture timing
  with RGB positions driven by a bounded monitor-response model. Model phase,
  frequency/holdover and lock explicitly only to the extent the corpus needs.
  Preserve raw blanking in pixel values without punching holes in scaler DE.
- Prototype offline first from timestamped RGB/GA-sync traces. Measure whether
  a few line buffers and bounded displacement suffice; do not assume they can
  represent arbitrary lost sync, frame slips, vertical effects or bidirectional
  resampling. Record latency, clipping, buffer bounds and FPGA cost before RTL.
- Fit one parameter set on a subset of identified SHAKER pages, then test on
  held-out pages and normal steady-sync content. Failure on those controls is
  a reason to revisit the hypothesis, not add per-title tuning. Use transitions
  or video where available: a still photograph cannot uniquely identify response
  dynamics, so a fitted match is an approximation rather than a physical model.
- Start with selectable output profiles through the existing single interface.
  Simultaneous faithful raw analog and modelled HDMI is a separate framework/
  interface project, not another motherboard mux. Do not auto-detect the physical
  monitor or assume HDMI always means scaled output (direct video exists).

This is a proposed direction, not a measured CTM transfer function or an
acceptance claim. Do not tune a response constant independently for every title.

## V1 — Sync acquisition and monitor-response feasibility (B1/B22)

**Scope:** AT/AY/D3/D4 visual effects, Amazing's filter-sensitive top effect
(currently better in Full on HDMI, user observation), and the B22 acquisition case.
Own filter/output observations and the response prototype. Avoid changes to
CRTC rules or demo-specific timing hacks. Coordinate with V2 before touching
shared production video files; coordinate with V3 for Amazing captures.
Only one session operates MiSTer at a time. Neither V2 nor V3 must finish first.
Use the explicit coordinator and handoff protocol below for parallel sessions.

1. Read B1/B22, B6 and the acquisition trace. Verify the current source/output
   route, deployed RBF, framework settings and available observation taps.
   Map the user's AT/AY/D3/D4 labels to SHAKER version, module, menu key,
   SSM page and matching Logon reference photograph; do not assume D3/D4 are
   individual pages rather than script groups. Preserve authentic prehistory.
2. Use Full for the ordinary baseline. Record applied mode via available
   observation, raw CRTC sync, GA sync, selected sync/DE, line estimate and
   pulse-classification state. MiSTer screenshot geometry alone is insufficient.
   Use the existing B22 CSV fixture for training/phase history and extend
   observation only when a missing measurement prevents a bounded conclusion.
3. Run the small visual corpus across Full/Raw pixels/Raw CRT where informative,
   using fresh boots and repeatable navigation. Record failure to lock or stale
   frames as a result. Inspect several frames for slants/flicker; a still is
   insufficient for transient behaviour. Raw CRT HDMI failure is not proof of
   a CRTC defect or faithful analog behaviour.
4. Compare raw waveform/pixel evidence, scaler output and existing original-CPC
   photographs. Inspect an emulator's actual monitor-renderer source before
   claiming how it implements the effect. AmSpirit is a diagnostic comparator.
   Read the French ACCC through the prescribed lookup/PDF protocol for any new
   hardware-rule claim; do not promote an emulator's constants into hardware rules.
5. Prototype the smallest trace-driven response model offline if the evidence
   supports it. Compare against unchanged raw and current Full renderings.
   Write the mode/output contract and one measurable next implementation slice.

**Deliverable:** evidence matrix and design decision: what current outputs can
represent, where acquisition is demonstrably wrong, which effects require
presentation modelling, and what remains unknowable without CTM/original CPC.
A reproducible fixture bug or proven implementation-contract defect can be fixed
without an original CPC; an unsupported monitor-faithfulness claim cannot.

**Gate:** fail-first focused deterministic proof for any timing/state change;
source-grounded expectations; relevant selection gate, soak policy and fresh
cross-provider review. Synthesize only a justified candidate, then run the same
bounded device corpus. Do not begin a broad filter rewrite as the first unit.

## Shared demo reproduction settings

These settings support separate V2 and V3 investigations. Do not presume a
shared filter defect or make DSC4 repair wait for sync-response improvements.

| Case | Device media (under `/media/fat/games/Amstrad/`) | BASIC command | Checkpoint after launching |
| --- | --- | --- | --- |
| DSC4 | `dsk/dsc4-alt.dsk` | `RUN"-DSC4` then Return | Wait **at least 50 seconds** before first capture; retain nearby repeated frames |
| Amazing2021 | `dsk/amazing2021_rev2.dsk` | `RUN"AMAZING` then Return | Sample **30, 45 and 60 seconds**; refine once the faulty scene is identified |

Use **classic CPC 6128, Plus off, CRTC1** for both initial cases. DSC4 settings
are supplied by Renaud; reusing them for Amazing is the explicit initial
assumption. Apply/reset the model, rather than just recording a declaration.
Use Full as the baseline; the user did not identify their latest manual
screenshot's filter or complete output configuration. Recover those from
available artifacts where possible rather than inventing them.

1. Use `mister-capture` and the hardware-loop driver, serially via direct SSH to `root@mister`.
   Hash the exact DSKs and deployed RBF, establish its relation to current master,
   and find/preserve the latest relevant manual screenshot without modifying it.
   The user's latest DSC4 image is reported to be around 50 seconds, not yet
   identified by filename. Record t=0 as final launch Return, actual capture
   timestamps and boot/load delays. Do not treat a delay after MGL load as 50
   seconds of demo execution.
2. For V2, reproduce DSC4 from two independent boots with repeated captures after
   50 seconds. Retain movement/flicker evidence rather than a convenient still.
   Use scene identity as well as elapsed time for cross-emulator comparisons.
3. For V1/V3, run the Amazing checkpoints with the exact revision above; retain
   top-effect and bottom-corruption evidence separately. Older reports
   mention Amazing Demo, but do not establish identity with this revision.
4. Compare Full/Raw pixels first only when a discriminating symptom warrants it;
   use Raw CRT as a bounded output-path probe. Measure machine progress/raw timing
   independently of screenshots, since a stale scaler frame can mask execution.
5. Use `amspirit-test` to run the same media/model and scene. A snapshot near the
   fault can help inspect code, RAM, registers and fetch addresses. Preserve a
   continuous boot replay too: SNA does not preserve every hidden CRTC/filter state,
   and snapshot agreement or emulator agreement does not establish CPC correctness.
6. Isolate one failing interaction at a time: CRTC/addressing, GA blanking/pixels,
   filter compensation/acquisition, or scaler output. Produce a source-derived
   fail-first vector before a timing/state repair. If the cause is presentation
   response, hand the exact trace to V1 instead of changing CRTC rules.

## V2 — DSC4 CRTC/GA investigation and repair

Use the DSC4 reproduction above. Renaud reports the display defect does not
vary with filter setting and suspects CRTC/GA fixes are needed. Prioritize
continuous execution, register-write timing, video addresses, fetched data and
GA RGB/blanking before output presentation. Lack of visible mode dependence
does not prove a CRTC/GA cause, but is a reason not to funnel this task into a
filter redesign. Do not require another broad mode matrix before tracing it.

**Deliverable:** an isolated failing interaction and justified minimal repair,
or a precise diagnosis of the remaining evidence gap. Use AmSpirit state and
snapshots where useful and apply the shared fail-first/gate/review requirements.
Proceed independently of V1; hand off an output trace only if evidence actually
locates the problem there. Do not assume Amazing's lower defect shares a cause.

## V3 — Amazing2021 lower-screen flashing corruption

Use the Amazing reproduction above, preserving both screen regions in the
evidence. The lower flashing corruption is a separate reported symptom from
the upper filter-sensitive effect. Renaud suspects a CRTC/GA defect here;
investigate that hypothesis through continuous replay, RAM/buffer selection,
addresses, register timing and pre-filter pixels. Retain repeated frames to
distinguish alternating content, execution problems and stale scaler capture.

**Deliverable:** a separate lower-screen diagnosis and justified minimal repair
with fail-first proof, gates, review and device retest where supported. V1 owns
the upper effect. Reuse a capture batch where convenient, but neither a V1
improvement nor a DSC4 repair closes V3 without its own matched retest.

## Device ownership and acceptance

One designated device operator, direct SSH to `root@mister`, one writer per checkout. V1, V2 and V3
have no mandatory execution order. DSC4 can be tackled first as an independent
repair task. Share Amazing capture batches between V1 and V3 where useful.
Do not launch two unattended device sessions concurrently; independent research
and simulation may proceed with nonoverlapping ownership.
The combined prompt is an alternative to the separate-session prompts, not
another concurrent worker. For separate sessions, use separate worktrees and
make V2/session A the initial coordinator and sole MiSTer operator. V1/V3 send
capture requests and receive artifacts through task-relevant chat messages,
authorized by the continuation prompts. Establish peer chat IDs at startup;
transfer ownership only by explicit request/acknowledgment after input release,
CFG restoration and MENU. Silence or a missing peer never grants ownership.
Apply the same rule to a shared AmSpirit instance. If coordination is unavailable,
continue offline work and defer device operations. Agree on shared CRTC/GA
semantics or video-interface changes before implementation and integrate serially.

Direct read-only SSH from this Mac to `root@mister` was verified on 2026-10-29
and reported `Amstrad`. No device state was changed. The earlier Mac mini route
was a run-specific choice; it is not required by this plan. Discover helpers,
media and artifact paths on the chosen host instead of assuming the mini's
scratch paths exist locally. Only use the mini when an actual connection/tool
limitation warrants it; preserve sole-operator ownership across hosts.

For these continuation tasks, save and restore the original MiSTer CFG and
return to MENU after each batch, including on failure. This explicit task policy
overrides the capture skill's default of leaving test settings applied.
Keep media, snapshots, images and full logs ignored under `local/investigations/`;
commit only source, justified tests and concise evidence notes. Check quota at
bounded work units, preserve reserves, and do not redeem resets or buy overage.
Do not send author questions or publish/push without explicit authorization.

## Independent consultation and accepted conclusions

**Completed:** Claude Opus 5.5, high effort, read-only via guarded `ask-claude`,
run `20261029T040532Z-17786-a899`, session
`88c8ebbf-8151-48b0-ba80-ea51f9e99d34`; exit 0. This was architecture advice,
not a code review, hardware test or independent examination of the photographs.
The initial call `20261029T040254Z-17006-e612` failed with API 401. Renaud
checked CLI authentication and requested the successful retry. Full reply and
lifecycle logs are retained under ignored `.roster-scratch/opus-sync-20261029/`.

Opus independently supports the separation of raw CRT output and digital
monitor-response presentation, a model after CRTC/GA with native byte order,
stable scaler timing, no simultaneous SHIFT compensation, offline feasibility
before RTL, and one parameter set tested against held-out SHAKER pages. Those
recommendations are accepted as the direction of investigation. A new mode
could use reserved encoding 3, but this is an option, not an allocated interface
or authority to redefine the reserved-value fallback before design acceptance.

The parent retains these qualifications to the advice:

- A Full/Raw-pixels picture difference is not uniquely evidence of SHIFT:
  raw vertical-blank pixel policy differs too, and scene/capture timing matters.
- Demos do not automatically emit SSM markers. Obtain filter state through an
  existing suitable observation method or a justified minimal diagnostic build;
  do not promise marker-time telemetry for unmodified DSC4/Amazing.
- Opus's approximate 1024-by-12-bit buffer estimate is not an implementation
  resource result. Buffer multiplicity, RGB width, read/write causality,
  clocking, overflow and pipeline latency remain to be worked out.
- The first DSC4 capture remains at or after the user's 50-second requirement;
  nearby samples can follow it. AmSpirit pixels may be compared diagnostically,
  with scene/state alignment; they are not an original-monitor oracle.
- Filter lock failure and pulse suppression identify a presentation boundary
  worth measuring, but do not by themselves exclude a bad upstream CRTC stream.
- A future monitor model must be bypassed for a raw CRT target. Present modes
  do not contain that model; the double-response risk concerns the proposed one.

The actual AT/AY/D3/D4 effects, monitor transfer constants, complete framework
output behaviour and trace-export capacity still need verification. No FPGA
implementation or measured analog-faithfulness conclusion follows from this advice.

## Sources and existing evidence

- Current production files named above; [B1/B22](../../backlog.md) and
  [B6 boundary](b6-video-boundary.md).
- [B22 acquisition trace](b22-acquisition-trace-2026-09-29.md).
- [September 9 captures](../hardware-runs/hardware-evidence-2026-09-09.md)
  and [September 12 results](../hardware-runs/hardware-evidence-2026-09-12.md):
  existing DSC4/Amazing defects, with incomplete output/settings provenance.
- [SHAKER author portal](https://shaker.logonsystem.eu/welcome): original-machine
  reference photographs; the author warns that emulator synchronization can
  differ from the machine. These are visual references, not raw signal traces.
