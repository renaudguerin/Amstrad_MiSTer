# DSC4 credits: colon bytes at the short-HSYNC boundary — 2026-11-03

## Position

The four missing credit colons sit in VRAM byte-column 42, the even byte of
CRTC character 21, immediately before DSC4's programmed R2=22 split. The
saved caller repeatedly changes R2, R3, GA mode and palette within each
64 µs line. A generic R1=R2=4 white-pixel test does not reproduce this caller.

The first useful discriminator is this actual byte/sync/mode boundary,
including Full's byte-shift policy. No RTL defect or hardware rule was proved
in this bounded analysis, and no production code, tests, gates, device or live
emulator/API session was changed or run. A candidate fail-first experiment is
specified below; its RTL failure remains to be established before a fix.

The bottom-line symptom remains separate.

## Saved evidence

Task-local inputs under `local/overnight-20261103/`:

- Original MiSTer screenshots `20261103_205511-screen.png` (all four credit
  colons missing) and `20261103_205848-screen.png` (image plus bottom line).
- `classic/dsc4-crtc1-coldboot/credits-t23/` and `credits-t23_5/`: saved
  cold-boot AmSpirit Lite 1.17.1/core 2491682, classic 6128/CRTC 1, colons
  visible. `image-t42_5/` and `image-t43/` show the image without the line.
- `classic/dsc4-crtc1-coldboot/credits-trace/`: a restored-credit SNA plus
  20,000-entry `history.json`, `io-writes.txt`, state and mapping. Its register
  trace is a diagnostic continuation, distinct from the cold-boot screenshots.

The trace SNA is v2 with a 128 KiB RAM payload at offset 256 and additional
emulator data after that payload. CPU mapping is banks 0, 1, 2, 3 with both
ROMs disabled. All 20,000 history `current_hex` four-byte sequences match the
corresponding extracted RAM addresses. This checks the recovered code/mapping;
it does not supply missing CRTC counters or I/O-latch timestamps.

AmSpirit's screenshots establish emulator behaviour, not original CPC hardware
truth. The screenshots differ in scaling and acquisition geometry; no numerical
pixel-difference comparison between their image coordinates was used.

Analysis artifacts are in ignored `classic/credit-analysis/`: `ram128.bin`,
`io-events.json`, `executed-caller.tsv`, `line-loop-relative-timing.json`,
`mode1-label-plane.txt`, `colon-bytes.json`, and `source-evidence.json`.
The earlier main-checkout `classic-video-v2-2026-10-29-b157/` replay tools were
inspected, but their saved RAM/code is from another DSC4 phase and was not
substituted for this credits caller or executed.

## Authentic caller and relative phase

The observed code writes R1=40 at PC `191E`, and R12=48 (`#30`) at `1903`.
The saved state has R13=0. No trace evidence puts R1 at 22: the seam is an
HSYNC/mode operation inside the displayed 40-character area.

A line loop uses CRTC registers BC=`BC03`/`BD03`, DE=`1602`, HL=`062E`.
Its alternate register set uses GA BC=`7F02`, DE=`8C8D` and palette data
popped into HL. The following is an actually executed repetition, with time
zero at the start of instruction `1891`, not at any observed C0 value:

| PC | Instruction / purpose | Instruction start, µs |
|---|---|---:|
| `1891` | `OUT (C),E`: select CRTC R2 (`E=2`) | 0 |
| `1893` | `INC B`: BC→BD03 | 4 |
| `1894` | `OUT (C),D`: R2←22 | 5 |
| `1896` | `OUT (C),L`: R2←46 | 9 |
| `1898` | `DEC B`: BC→BC03 | 13 |
| `1899` | `OUT (C),C`: select R3 (`C=3`) | 14 |
| `189B` | `INC B`: BC→BD03 | 18 |
| `189C` | `OUT (C),H`: R3←6 | 19 |
| `189E` | `EXX`: GA register set | 23 |
| `189F` | `OUT (C),E`: GA RMR←`8D` (mode 1) | 24 |
| `18A1`–`18AC` | palette data/pen selection, decrement and branch | 28–47 |
| `18AE` | `OUT (C),D`: GA RMR←`8C` (mode 0) | 51 |
| `18B1` | `EXX`: CRTC register set | 56 |
| `18B2` | `OUT (C),E`: R3←2 | 57 |
| `18B4` | `DEC B`: BC→BC03 | 61 |
| `18B5` | `JP (IX)`, IX=`1891` | 62 |
| `1891` | next repetition | 64 |

A preceding loop at `1837` has the same CRTC operations at `183A`, `183C`,
`1842` and `1858`. The captured history contains 146 repeated writes each at
those four sites, and 64 each at `1894`, `1896`, `189C` (62 at `18B2` plus
entry variants). This is recurring scanline programming, not an isolated
initialization write.

The 24-instruction continuing path totals 64 µs using French ACCC v1.11 §26
pp.281–282, whose tables were rendered and inspected. `OUT (C),r8` lasts 4 µs
with I/O in its third µs. Thus the table gives relative instruction starts;
R2's write opportunities are four µs apart. The saved history contains no
per-instruction tick, C0, HSYNC, GA sequencer or bus-latch phase. In particular,
this evidence does **not** establish whether R2=22 is JIT or NJIT. Inferring
an absolute C0 solely from screenshot x or the frontend beam field would
silently mix different timing coordinates.

## Colon RAM anchor

With R12:R13=`3000`, the ordinary CPC screen-byte address is
`C000 + (y & 7)*0800 + (y >> 3)*80 + byte_column` for this R1=40 image.
Mode 1 extracts four pens from each byte, using `(b7,b3)`, `(b6,b2)`,
`(b5,b1)`, `(b4,b0)` with the high-numbered bit as pen bit 0; production
`rtl/GA40010/video.sv` implements this bit selection. Each mode-1 pixel is
two pixel-M2 (French §9.3.4.1 p.54, rendered).

All four colons use mode-1 pixel columns 170 and 171. Their byte-column is
`floor(170/4)=42`, hence character-column `floor(42/2)=21`.
The source byte is `12` → pens `[0,0,2,1]`, followed on the next raster row
by `31` → `[0,0,1,3]`. The adjacent odd byte, column 43, is zero.

| Label | First colon dot rows (screen RAM y) | Bytes and addresses |
|---|---|---|
| GFX | 91, 92 | `DB9A=12`, `E39A=31` |
| ORIGINAL GFX | 114, 115 | `D48A=12`, `DC8A=31` |
| MUSIC | 140, 141 | `E57A=12`, `ED7A=31` |
| CODE | 166, 167 | `F66A=12`, `FE6A=31` |

Each dot repeats four rows later; all sixteen nonzero source addresses and
their zero neighbours are recorded in `colon-bytes.json`. Their presence
rules out missing punctuation in this saved title RAM. Whether the correct
byte reaches the FPGA serializer at the decisive dot still requires a trace.

## First plausible boundaries

The common classic path, for either CRTC type, is:

1. `rtl/Amstrad_motherboard.v` selects CRTC MA/RA and fetches two bytes.
   In Full mode with `crtc_shift=1`, its `vram_d` policy saves the high byte
   and emits delayed-high/current-low; the two raw modes use native low/high.
2. `rtl/GA40010/ga40010.sv` buffers that byte and DISPEN at sequencer states
   `e0` and `03`, then feeds production `video_sync`.
3. `rtl/GA40010/video.sv` runs the byte loader, shift register, pen selection,
   palette mux and registered RGB. Graphic mode is latched by `mode_sync_en`
   from `syncgen_sync.v`, rather than changing at the RMR write itself.
4. The same renderer gets `FORCE_BLANK = HCNTLT28 | HSYNC_I`; raw CRTC HSYNC
   can asynchronously force RGB black independently of those buffered pixels.

This puts byte alignment, short-HSYNC blanking and the mode latch directly
at the colon boundary. A palette write can also make correctly fetched pen
1/2/3 black, so an RGB trace without the selected ink values is insufficient.
R1 is 40 while the punctuation is at character 21; a simple R1 truncation
story does not explain that location.

French ACCC §9.3.4.1 p.54 and §9.3.4.2 pp.55–57 explicitly distinguish the
HSYNC blank start, restoration and mode-switch instant, including the
three-pixel-M2 type-1 JIT/NJIT gap. The type-1 chronogram on p.57 was rendered
and inspected. These are separate signals to measure; they do not authorize
shifting raw HSYNC to preserve every source pixel. No hardware result supplied
here resolves which boundary is wrong.

Existing `rtl/GA40010/r2jit_tb.cpp` protects raw blank onset/JIT differences
and overflow restart, with a different R1/R2 fixture. The motherboard
`sim/plus/b6_video_boundary_test.cpp` protects byte policy and output transport
but programs R1=40/R2=50 and does not independently score these punctuation
bytes across this short-HSYNC/mixed-mode seam. Their green results therefore
do not dispose of this residual.

## Recommended next experiment

First compare the same settled credits under Full and Raw pixels, with model,
CRTC, media and image checkpoint fixed. These modes share the Full acquisition
tuple but differ in native versus shifted byte policy. This is a discriminator,
not an accuracy vote: restoration only in Raw pixels would point first at the
Full fetch/shift interaction. Persistence in both modes would direct attention
to the shared GA/raw-HSYNC/mode/palette path. A parent-operated hardware run
must record actual applied settings; no such comparison was done here.

Then replay one authentic line, carrying the `1891` loop and saved palette
state on production T80/CRTC/GA, rather than a new generic white screen. Capture
actual accepted I/O edges with C0, MA/RA, fetched byte/half, `crtc_shift`, GA
`DISPEN_BUF`, load/shift state, `mode_sync_en`, effective mode, pen/ink, raw
HSYNC and raw RGB. Score source bytes `12`/`31` at byte-column 42 and the
zero at 43. Use the French p.57 phase chart and observed JIT/NJIT write phase
to derive which colon pixels precede blanking. Stop at the **first** divergence:
wrong fetched half, wrong effective mode/ink, or premature force blank.

That is the concrete candidate fail-first discriminator. Its expected pixels
can be tied to authentic bytes and an independently documented phase; no
meaningful failing RTL vector was established by this saved-data-only pass.
Do not alter CRTC or GA timing merely to make an R1×16 pixel count pass.

## Bottom line

The supplied image captures establish a separate emulator/MiSTer difference
at the bottom edge. This trace is from the credits, not the image's final
raster. It cannot determine whether the line comes from R6/frame termination,
VMA restoration, blank tagging, acquisition crop or output transport. Keep
that symptom open until an image-stage final-line trace or applied-mode
comparison isolates it; the colon diagnosis does not imply a common fix.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by
Longshot (CC BY-NC-ND 4.0).

## Parent follow-up: instruction steps and snapshot limitation

A restored-credit breakpoint at B00:1891 and24single-instruction steps
reached the next1891 exactly64ticks later (14670→14734). This independently
checks the caller's relative period on the reference emulator. Artifacts:
`classic/credit-line-checkpoints/records.json` and its per-step states.
The streamed snapshots are **version2**, with zero-filled v3 CRTC-counter
offsets. Those bytes do not establish C0/C4/C9; the attempted absolute
phase checkpoint is therefore inconclusive. `/api/crtc` still publishes
only registers and selection. No expectation was manufactured from the
zero fields.

The first private resume observer timed out because the breakpoint was
hit before it observed a running state; the corrected command waits on
command acknowledgment and the intended PC stop. A separate control also
showed that clearing breakpoints resumes this build; final pause was then
explicitly reapplied and verified. These are tooling observations, not
CRTC rules.

## Private replay and restart controls — 2026-11-04

The parent subsequently captured the same credits on diagnostic RBF `bedb1c5`
in Full and Raw pixels. Both still miss the colons; the later image captures
also retain the coloured bottom line. Each hardware case completed 16 commands,
three captures and zero driver failures. Evidence:
`classic/dsc4-device-bedb1c5-{full,rawpixels}/` and
`classic/credits-full-raw-crop.png`. These runs record the applied CFG request
and reset sequence, not the live `crtc_shift` bit. In particular SHIFT might
be zero in both cases: the result does not prove that changing byte policy
was exercised or exclude that path.

### What the private unit actually executed

Ignored `classic/credit-replay/` contains an adaptation of the existing B18
snapshot motherboard fixture, the production GHDL T80 netlist, external CPU
and video RAM, production CRTC/GA, and observation ports for accepted writes,
MA/RA, byte/half, SHIFT, GA byte buffer, serializer, mode, ink, blank and RGB.
The motherboard adapter only changes VHDL port-name case as the existing
`sim/b18_snapshot.mk` does. Production RTL and tracked simulation files were
not edited. Build and execution are private diagnostic commands:

```sh
python3 local/overnight-20261103/classic/credit-replay/prepare.py
bash local/overnight-20261103/classic/credit-replay/build.sh
# Run from that private directory:
./obj/replay 0
./obj/replay 0 sync
./obj/replay 0 init
```

The replay loads the saved v2 CPU registers and 128 KiB RAM, supplies the
actual video word from external memory, and uses production PPI reads.
A diagnostic FFF0 trampoline avoids the known first-fetch bus overlap;
its overwritten bytes are restored after reaching the intended entry.
It does not interpret zero-filled v3 counter offsets in the streamed v2
snapshot as measurements or seed the CRTC from those fields.

The continuing title path is **not** self-resynchronizing every frame.
PC `190D` jumps back to `180C`; the repeated frame path uses fixed delays.
The original PPI/VSYNC synchronizer is at `2030`, called at `1809` during
stage initialization, before that frame loop begins.

| Private attempt | Observed result | Acceptance |
|---|---|---|
| Original saved PC `2084` continuation | Six VSYNC rises, 160 line-loop entries; PC `1891` at C0=44; R2=22/46 writes at C0=51/55 | Wrong phase; reject as colon evidence |
| Original `2030` synchronizer, then `180C` | Synchronizer returns after six VSYNC rises; 192 line loops at 4096 master clocks = 64 µs; PC `1891` at C0=18; R2 writes at C0=25/29 | Writes have already missed C0=22; reject as faithful split replay |
| Original initializer `1803`, including `1D00` and `2030` | Bounded 64,000,000-clock run reaches 50 VSYNC rises but no `1891` line-loop entry | Incomplete stage restart; not a pixel oracle |

The first two controls recorded accepted CRTC-write phase and colon bytes
through GA, but they do not establish the phase used by the original cold
boot. Their target-window samples had mode 1, no short raw HSYNC and SHIFT=0,
so scoring those RGB samples as the title's expected colon pixels would be
misleading. No failing, meaningful RTL colon discriminator was obtained.

Partial sources and CSV/logs are preserved in `full-initial/`, `full-sync/`,
`sync-warmup/`, plus the final `replay-full-init.log`, `io.csv`, `pixels.csv`
and `instructions.csv`. The parent's original AmSpirit instruction stepping
in `classic/credit-line-checkpoints/records.json` independently measures
`1891→1891` as exactly 64 µs. Agreement on line duration does not establish
agreement on absolute raster phase.

### Why the user-visible restart images were invalid references

Two explicit AmSpirit controls used the saved credits SNA, with classic
6128/CRTC 1 retained, and a CPU/RAM trampoline at FFF0. They were experiments,
not original disk boots:

- `amspirit_sync_control.py` invoked `2030` then jumped to `180C`. At the
  `1891` breakpoint its frontend beam x was 211, versus 451 in the parent's
  original saved-line checkpoint. These are frontend beam coordinates, not
  measured C0 values, but they demonstrate that the restart did not reproduce
  that original checkpoint's phase.
- `amspirit_init_control.py` jumped to `1803`, rerunning initialization from
  an already initialized/mutable RAM image. Its `1891` beam x was 223. The
  resulting screenshot had clear handwritten names and flattened/corrupt
  left labels. This is **not merely a possible missing-counter effect**:
  the initializer had changed the displayed source RAM.

The latter state was frozen and saved as
`visible-control/restart-experiment/`, including screenshot, SNA, state and
`operations.json`. Its `ram-comparison.json` proves that all ten sampled
colon bytes (`DB9A`, `E39A`, `FB9A`, `C3EA`, `D48A`, `DC8A`, `E57A`, `ED7A`,
`F66A`, `FE6A`) changed from the original `12`/`31` values to **00**. Across
200 screen rows, columns 0–43 had 303 changed bytes out of 8,800, versus nine
out of 6,400 in name columns 48–79 (the diagnostic trampoline also lies in
that latter RAM range). Thus the first demonstrated divergence for this
opposite corruption is already in title RAM, before fetching or rendering.
A one-frame stability claim for that frozen experimental image was not made.
Missing raster-state restoration in v2 remains a separate replay limitation;
it is not established as the cause of this RAM corruption.

### Clean user-visible reference restored

After recording the experiment, the original disk was hard-booted with
classic 6128/CRTC 1, French ROM and monitor Off. While the user-visible
results were being aligned with the evidence, this boot was paused in its
loader and recorded as `visible-control/urgent-current/`: PC `C6E7`, tick
19,439,965. It was then **continued without another reset or reload** from
the recorded launch origin to 1,155 frames, approximately 23.1 seconds.

The final screenshot was visually checked: all four colons and all handwritten
names are clear and match the original clean credits reference. The ten sampled
colon RAM bytes also match the original `12`/`31` values. Final state:
PC `2084`, tick 29,164,371, frame 210,070, paused, classic 6128/CRTC 1.
Breakpoints are empty; pause was explicitly reapplied after clearing them.

Evidence is `visible-control/fresh-credits-t23/`: `screen.png`, `state.json`,
`ram128.bin`, `fresh-credits-t23.sna`, and `final-control.json` with identity,
media hash, requested settings and actual frame offset. API ownership was
returned to the parent at that point; no further API experiment was performed.

### Specific missing piece before an RTL repair

A faithful private replay needs the original cold-boot stage-entry CPU/RAM
state **before** its one-time initialization mutates the label/font inputs,
so `1803/1D00/2030` can run once in their genuine context and establish phase.
An independently measured, complete raster state at a later checkpoint would
be an alternative, but the current streamed v2 header does not provide it.
Restarting the already initialized credits snapshot is not a substitute.

The next capture should therefore stop an original cold boot before `1800`
initialization, or preserve its exact pre-`1809` synchronization context, then
continue that same caller on the production replay. Verify unchanged colon
RAM and correct mixed-mode rendering before asserting a pixel failure. Until
then, the accepted-write observations above are failed replay controls, not
permission to retime CRTC or GA. The image-stage bottom line remains separate.
