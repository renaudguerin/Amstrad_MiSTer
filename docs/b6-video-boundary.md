# B6 video boundary: acquisition, pixels and CRT output

**Status:** implemented and validated locally, 2026-09-11; source review is
CLEAR. Fable's required fixture correction is incorporated. The broader
diagnostic matrix and hardware acceptance remain open; see the
[review and validation record](b6-video-boundary-review-2026-09-11.md).
**Source base:** `20ed4d3c1ac2afcea5bd5ff536c13bdf7d01b8da`.

This completes the video-boundary part of the [B6 decision](b6-architecture-decision.md).
The governing inputs are [D2](hardware-diagnosis-2026-09-10.md#d2--live-blanking-changes-ascals-acquisition-geometry)
and the [second-pass brief](hardware-diagnosis-2026-09-10-second-pass.md#b6-boundary-brief-for-the-next-session).
It changes output policy, not CRTC or ASIC timing rules. The September 11 slice
left `sys/` unchanged. The September 12 rendering follow-up permits only the
minimal `video_mixer` RGB generate-scope correction, backed by a failing
production-module test. No monitor PLL model, duplicate renderer or engine
gating belongs to this work.

## Decision and physical limit

Keep one pixel pipeline. Separate its byte-phase policy from the timing tuple
used to acquire its pixels. Full and Raw pixels use the complete Full tuple;
their raw sync effects exist in pixel values, never as holes in acquisition DE.
Keep Full as the default compatibility baseline.

Provide a separate, explicit Raw CRT selection. **Raw CRT is the user-approved
exception to Full acquisition timing.** There is only one core video output
interface in this checkout: `sys/emu_ports.vh` exposes `VGA_*`, and
`sys/sys_top.v` assigns the ASCAL input from the same `VGA_scanlines` stream
used by analog output. A simultaneous Full scaler stream and raw analog stream
would require changing that interface/framework or a separate framebuffer path.
Neither is part of this design. Merely adding a second motherboard tuple would
not create a second physical route.

D2's observation that ASCAL does not consume `i_hs` does not extend to VSYNC:
`sys/ascal.vhd` consumes `i_vs` in its input-frame logic (`i_pvs`,
`i_endframe0` and `i_endframe1`). Keeping Full DE while substituting raw HS/VS
therefore cannot establish an unchanged complete scaler acquisition contract.

Raw CRT consequently also sends raw geometry to ASCAL. HDMI acquisition can
become unusable in this mode; normal/scaled HDMI is not its acceptance target.
This is a deliberate diagnostic/output mode, not automatic fallback on bad sync.

"Stable Full" means the existing Full acquisition tuple, independent of the
Full/Raw-pixels selection for an identical input trace. It does not mean a new
fixed-frequency generator: `crt_filter` still learns cadence, responds to VSYNC,
and has its existing acquisition and missing-sync limitations. Its constants
and watchdog algorithm are unchanged by B6.

## Signal ownership

| Signal or state | Production owner | Use at the boundary |
|---|---|---|
| Raw CRTC HSYNC/VSYNC | selected classic CRTC or `asic_video`, `hs_sel`/`vs_sel` | input to `crt_filter`; preserve as diagnostic phase references |
| Monitor HSYNC/VSYNC | selected `ga40010` or `asic_ga_timing`, `hsync_ga`/`vsync_ga` | physical Raw CRT sync; these are GA-shaped signals, not the CRTC pins |
| Horizontal force blank | classic `HSYNC_I`; Plus `asic_video.HSYNC` | already rendered by the real pixel engines; do not synthesize a fixed-width horizontal black bar |
| Raw vertical blank | selected GA `vblank_ga` | Raw CRT vertical geometry; an aligned pixel mask in Raw-pixels mode |
| Full acquisition tuple | `crt_filter` | all four timing signals for Full and Raw pixels |
| Fetch phase | selected GA CAS/RAS/CPU strobes | byte serializer and the safe point for committing a mode |
| Filter compensation | `crt_filter.SHIFT` | Full compatibility byte policy only |
| FIELD | selected machine's existing FIELD mux | unchanged; never substitute the inactive classic owner |

Classic `HCNTLT28` supplies the vertical component of its combined
`FORCE_BLANK(HCNTLT28 | HSYNC_I)` input.

The classic RGB interface carries netlist DAC pairs, whereas Plus carries
native nibbles. In particular, classic force-blank pair `00` passes through a
calibrated near-black entry in `color_mix`; it is not numerically equivalent to
24-bit zero. Preserve that horizontal-blank rendering and its existing native
latency. Do not detect blank by comparing RGB against a colour, or gate RGB with
CRTC DE: DE also selects coloured border and Plus sprite priority.

## Mode and transition contract

Retain `status[36:35]` and three menu values:

| Value | Menu | Timing tuple | Byte policy | Additional pixel blank |
|---|---|---|---|---|
| 0 | Full | Full | existing live `SHIFT` compensation | none |
| 1 | Raw pixels | Full | native byte order; ignore `SHIFT` | selected raw vertical blank |
| 2 | Raw CRT | GA monitor sync, raw CRTC horizontal blank, GA vertical blank | native byte order; ignore `SHIFT` | raw vertical blank, same pixel path |
| 3 | reserved | Full | Full | none |

Value 1 deliberately replaces the failed Live-blanking acquisition experiment;
it must not select `HBLANK_LIVE`. Value 2 gives the existing Off intent an
explicit CRT contract and core post-processing restrictions. Persisted values
are not renumbered. The legacy live extension can remain an internal diagnostic
during the first slice, but is no longer an output policy.

Normalize reserved value 3 to Full. Introduce one applied-mode register in the
motherboard. During reset it takes the normalized requested mode. Outside
reset, commit a changed request only while Full VBLANK is asserted and the
selected GA is in its CPU-owned phase (`!cpu_n`). The output tuple, byte policy,
pixel mask and top-level Raw CRT restrictions use this same applied mode.
Do not independently latch these policies in several modules.

The CPU-owned phase precedes the two video fetch halves. Committing there
prevents one word from being assembled under two mode policies. The Full
vertical blank condition also allows the pixel pipeline to drain before the
next active image. It is a level condition, not a one-clock coincidence of two
edges. If malformed source timing never produces Full VBLANK, the pending mode
waits; the existing Reset action applies it without that dependency. No new
timeout or hidden automatic mode switch is needed.

Full-to-Raw-pixels changes never change the acquisition tuple. Entering or
leaving Raw CRT may splice sync and disturb one acquisition interval; wait for
reacquisition before comparing frames. An arbitrary live machine-model switch
remains outside this contract: use the existing reset/apply-model workflow.

## Explicit byte-phase rule

The clock is the motherboard's 64 MHz master clock. `ce_16` and selected GA
strobes retain their production phases. The following table describes values
sampled **before** a rising master edge; nonblocking updates become visible
after that edge. `W[n]` is the returned 16-bit VRAM word for the current pair,
and `H` is the retained, DE-qualified high byte.

| Accepted fetch (`!ras_n && !cas_n`) | Native policy | Full with sampled `SHIFT=0` | Full with sampled `SHIFT=1` |
|---|---|---|---|
| `vram_bs=0` | `W[n][7:0]` | `W[n][7:0]` | previous `H` |
| `vram_bs=1` | `W[n][15:8]` | `W[n][15:8]` | `W[n][7:0]`; retain `H = de_sel ? W[n][15:8] : 0` |

`vram_bs` clears in the CPU-owned phase and becomes one on the observed rising
CAS edge under active RAS. Consumers on the same clock edge see the previous
`vram_d`. The Plus word assembler keeps its existing mapping: low word byte on
`plus_cclk_en_p`, high word byte on `plus_cclk_en_n`. Neither the address mux nor
the Plus pixel/scroll/sprite engine changes.

Full deliberately continues sampling live `SHIFT` on each accepted fetch.
Latching `SHIFT` once per word would be a different Full behaviour and is not a
permitted cleanup in this work. Raw pixels and Raw CRT ignore it entirely,
including when it changes between the two fetch halves.

Retained shifted history must not reappear from an earlier mode visit. Clear
the history on reset and on a committed mode change; the first shifted high
half subsequently primes it through the normal production rule. A first
shifted low half before priming emits zero. Do not continuously prime history
from unshifted fetches: that would also change steady Full behaviour when
`SHIFT` subsequently becomes one. The startup/mode-change rule requires its
own deterministic regression before implementation. Steady Full after startup
retains its existing byte sequence, including live `SHIFT` changes.
Clearing history on reset deliberately changes the first shifted low half
from a retained pre-reset byte to zero; this is the small reset-boundary
deviation from legacy Full, not a claim of reset-trace equivalence.

If extraction improves the test seam, move this serializer and the Plus word
assembly into a small production module used by the motherboard. Do not create
a test-only copy of these always blocks. Keep VRAM address/DMA ownership in the
motherboard; the new seam concerns returned-byte assembly only.

## Pixel and output pipeline

Raw HSYNC blanking already comes from the selected production renderer. Raw
pixels preserves that pixel stream with native fetch order, while the Full
tuple supplies the sampling rectangle. Pixels outside that rectangle are
cropped; this mode does not expose the whole analog raster or reconstruct CRT
horizontal displacement.

The additional raw vertical-blank mask is explicit pixel metadata. Sample
`vblank_ga` on `ce_16` alongside the native renderer's pixel production, then
consume that tag alongside the corresponding selected RGB at
`amstrad_video_color`. Register the masked converted colour on the same
`ce_pix` edge as its Full timing tuple. This is especially necessary for Plus:
its renderer already registers RGB on `PIXEN`, and the B8-6 converter adds a
pixel register. A combinational mask after conversion would tag a previous
pixel with the current blank state.

Only the vertical-blank tag adds numeric black. Full conversion is unchanged;
horizontal blank retains the source renderer/DAC behaviour described above.
Apply the vertical mask after colour conversion, so a zeroed classic netlist
pair is not mistaken for exact output black. Gamma remains the user's colour
transfer function; pixel black is defined at its input, and a nonzero gamma
entry for zero is not a geometry failure.

Use a `ce_pix`-enabled tag register beside the existing converter output
registers, followed by a combinational output mask. This leaves the classic
DAC table unchanged and gives the tag and converted RGB identical latency.
Plus Full's existing vertical-border output remains a separate Plus follow-up;
the additional tag is enabled only in the two raw modes.

The fixture must establish the alignment of this tag at both machine paths
before accepting the mask implementation. There is no new pipeline stage on
Full RGB or on the timing tuple. The source timestamp, enabled-edge latency,
and held value between enables are part of the assertion, not inferred from a
visually black screenshot.

The explicit tag follows a dot-sampled contract. Classic FORCE_BLANK can assert
asynchronously before that tag is sampled; its native near-black may therefore
appear before the additional exact-zero mask. Do not change the GA's
asynchronous behaviour to make these two representations identical. The tag
and Plus's registered pixels are sampled from pre-edge inputs on the same
`ce_16`; colour conversion uses the previous registered pixel/tag together.

For Raw CRT, force the core's post-processing policy to native `ce_16`, no
HQ2x, no scandoubler, no scanline effect, and no vertical crop. Feed the
applied-mode flag to these decisions in `Amstrad.sv`; do not rewrite retained
OSD settings. Disable the forced-scandoubler input at this boundary too, so it
cannot silently invalidate a selected CRT mode. Returning to a safe mode
restores the user's existing settings after ordinary pipeline reacquisition.

`rtl/amstrad_video_output.sv` now contains the existing production colour,
interlace, mixer and crop chain extracted from `Amstrad.sv`. The top supplies
the motherboard's applied mode and retained settings to that one instance.
The output fixture instantiates the same chain, including production
`video_mixer` and `video_freak`; it does not copy their policy into a test-only
model. The existing `en270p` result still feeds the top's menu capability
decoder. This extraction adds no pixel register or independent mode state.

The supported physical setup is unscaled analog output to a compatible CRT,
or an appropriate Direct Video analog adapter. Framework `vga_scaler`/framebuffer
selection, sync polarity correction, composite-sync generation and OSD remain
framework-owned. They are configuration/hardware acceptance conditions, not
core-controlled guarantees. "Raw CRT" means bypassing the core filter and
line-resampling policy; it does not claim pin-for-pin connector timing from
local simulation. No simultaneous stable HDMI guarantee is made.

In particular, the unchanged `video_mixer` updates DE only at horizontal-DE
edges. With absent raw HSYNC, Raw CRT DE can hold its previous value; it is not
a missing-sync recovery mode. Framework `sync_fix` can also reverse polarity
when the pulse duty exceeds half a line. Record that setting/waveform in any
CRT lock failure rather than inferring a new CRTC defect from the image alone.

## Implementation slices and gates

1. **Prove the old boundary fails.** Add a focused production-seam fixture
   with asymmetric VRAM words and a controlled mid-line sync disturbance.
   Before changing timing RTL, demonstrate that legacy mode 1 changes DE and/or
   accepts filter-dependent byte compensation under the new Raw-pixels
   contract. Pin the returned-byte sequence independently of simulator output.
2. **Separate timing and byte policy.** Implement the shared applied mode,
   serializer/history contract and Full tuple for both safe modes. Preserve
   Full generator behaviour and inactive-machine ownership. Add the aligned
   raw vertical-blank pixel tag. Extend the fixture through real colour and
   mixer consumers, including Plus word assembly.
3. **Wire Raw CRT policy and menu.** Keep the encoding, rename the choices,
   and connect the applied mode to native cadence, effects and crop policy.
   Exercise the production policy with conflicting retained settings and with
   raw sync loss. Check all production instantiations/manifests after any
   module/port extraction.
4. **Review and deliver locally.** Run `make -C sim`, `make -C sim lint` and
   the canonical classic soak (`0xb1cb70da95c2e44f`). Require fresh
   cross-provider code review and resolve findings. Update current B6 status
   without rewriting dated hardware reports. Integration, push, synthesis and
   CRT acceptance are separate later stages.

Every timing/state change starts with its failing case, including mode-change
history handling. A green old test is useful regression protection but is not
evidence that the new boundary was exercised. Do not change a golden CRTC soak
hash for this output-boundary work.

## Seam fixture specification

Base the fixture on **`sim/plus/p10_boot_test_top.v` and its B7 build**. This
already instantiates the production motherboard, an executing CPU and real `sdram.v`
behind a testbench memory model. Here the executing CPU is the harness's
**TV80 substitute**, not the production T80 VHDL/netlist; no production-CPU
write-edge timing claim follows from this fixture. B7 supplies real classic GA/video and exposes
runtime classic CRTC selection. Reuse its program generator and memory service
from `b7_dark_silicon_audit.cpp`, plus the existing
`dbg_video_vram_addr/word/byte` taps. Make the harness's `SYNC_FILTER` setting a
runtime input for B6's mode-change cases while retaining the existing parameter
default for other callers. Seed asymmetric video words through the existing
SDRAM model; do not introduce a hand-written substitute for the production
address registration and byte-return window.

Use the existing TV80 build for the portable B6 gate. It executes actual I/O
instructions to configure the production devices, while the new assertions
score the observed production fetch strobes and returns. Importing D5's
generated T80 adapter would add a GHDL dependency without strengthening this
output-policy contract; exact production-T80 timing remains its own existing
diagnostic tier. Use `production_clocking=1` for the shared divider topology.

The older `p1_video_test_top.v` copies assembly logic and acknowledges a
different address-return timing; the ordinary Plus motherboard fixture ties
VRAM to zero and stubs classic GA. Neither is the B6 base. The old stub-header
claim that `.do(` prevents combining the Verilog motherboard and SystemVerilog
children is superseded by the per-extension flags already used in the Makefile:
`+1364-2001ext+.v +1800-2017ext+.sv`. The parent also elaborated the production
motherboard with real classic GA and ASIC successfully on this host (zero
errors; existing warnings retained). Do not label a constant-GA run classic
end-to-end verification.

Drive real bus writes or controlled production input pins; seed deterministic
asymmetric words, e.g. successive pairs with distinct low/high bytes, and
non-black inks/border. The testbench memory service seeds the real SDRAM
controller's returns, including its video-cache behaviour. Keep unrelated
audio, keyboard and storage assertions outside the fixture. Do not substitute
fake sync or RGB for the very renderer whose interaction is being claimed.

Observe pre-edge byte inputs, fetch phase, `SHIFT`, applied mode, `vram_d`,
Plus assembled word, selected RGB, raw sync/blank, Full tuple, converted tuple,
and final production output RGB/DE/CE. Measure enabled samples per DE interval and
intervals per frame; HSYNC edge counting is not an acquisition-width oracle.

| Case | Required observation |
|---|---|
| Settled Full, CRTC 0/1 and Plus | existing byte/pixel sequence and acquisition baseline retained |
| Raw pixels under the same trace/settings | Full tuple and DE windows unchanged, native low/high byte order |
| Mid-line HSYNC change inside the Full mask/reconstruction interval | raw black region follows real renderer timing; no extra DE interval |
| Short and multiple raw pulses | pixels may change; safe-mode tuple equals Full for the same trace |
| Missing/stuck raw sync after acquisition | inherited Full behaviour remains mode-invariant; Raw CRT follows actual raw source without fallback |
| Raw vertical blank changes inside Full active area | matching pixel becomes black at converter output; metadata and DE remain Full |
| `SHIFT` changes on either byte half | raw policy unaffected; Full samples the documented pre-edge value |
| Requested mode changes on every fetch phase | applied policy changes only at the stated safe point; no mixed pair |
| Raw-to-Full after long absence with `SHIFT=1` | no stale high byte from the previous visit; prime/zero rule visible |
| Plus nonzero word, scroll and an opaque sprite | byte assembly preserved; blank wins through the selected final colour path |
| Raw CRT with retained HQ2x/crop/forced-scandoubler | actual core output policy remains native and unresampled |

The [validation record](b6-video-boundary-review-2026-09-11.md#acceptance-limits)
states which observations are established by the complete motherboard,
colour boundary or output-policy fixtures. The September 12 follow-up adds
CPU-written short/missing/multiple sync, changing blanking and restoration
through final RGB, plus a combined scroll/opaque-sprite fixture. A sustained
stuck-high raw-sync CPU recipe is not established; zero-width missing HS and
stuck vertical blank are distinct observations. No exhaustive matrix or
physical connector acceptance is claimed. A fixed short sync width or a leaf
filter test alone must not be described as complete production evidence.

Use a few deliberate negative controls: restore the old Live tuple connection,
enable `SHIFT` in Raw pixels, and swap the Plus byte-half mapping. Each must
trip an independently calculated assertion. Keep these controls local and
bounded; do not build a general mutation framework.

The retained D2 GHDL experiment used unchanged ASCAL with output clock disabled;
it proved input geometry only. GHDL is not installed on this task's host at
design time. A current real-mixer DE fixture is therefore the mandatory local
gate; an ASCAL rerun is optional additional evidence when that runtime is
available. Neither establishes framebuffer delivery, HDMI lock or CRT output.

**Implementation preflight limit:** in this Verilator build, the unchanged
`sys/video_mixer.sv` declares `R_in/G_in/B_in` inside generate branches and
resolves its later unqualified references as undriven implicit nets. Mixer RGB
is consequently black in simulation, despite nonzero real motherboard and
converter pixels. B6 keeps `sys/` unchanged: assert pixel identity/masking at
the production colour-converter output and acquisition at actual mixer DE/CE.
Do not claim downstream mixer RGB was validated, or silently repair a copied
framework file in the fixture. Resolving that framework elaboration difference
is a separate follow-up before a complete rendered-pipeline simulation claim.

## Acceptance still requiring hardware

**September 12 retest:** the user confirms build `5c16b17` and reports no
visible difference so far between Full and Raw pixels in Amazing Demo, DSC4
and SHAKER A (T). No Raw CRT result or exact output configuration was supplied.
See the [hardware report](hardware-evidence-2026-09-12.md). B6 has not shown a
visible improvement in those cases; this observation does not establish that
the applied modes or internal pixel signals are identical.

After reviewed source and a separately authorized RBF build, compare Full and
Raw pixels on the same SHAKER/DSC4/Amazing Demo sequence and fixed output
settings. Then test Raw CRT at 15 kHz, recording configuration and photographs
against the Logon reference. Distinguish a moving black region from a CRT's
PLL-induced displacement. DSC4 and the remaining Amazing Demo corruption stay
open until that evidence exists; B6 supplies the controlled boundary for the
experiment, not a promised visual cure.
