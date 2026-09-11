# B6 boundary design review — 2026-09-11

Base: `20ed4d3c1ac2afcea5bd5ff536c13bdf7d01b8da`.
The parent owns [the architecture](b6-video-boundary.md). An independently
requested Opus 5 high proposal completed with exit 0 and clean process cleanup
(`20260911T045504Z-58891-3627`). Its original proposal and output are
retained under the ignored `docs/references/b6-boundary-2026-09-11/` directory.

## Parent review of the Opus proposal: changes required

1. **Raw VSYNC cannot be declared harmless to ASCAL.** The proposal correctly
   found the shared video stream and unused `i_hs`, but extended that result
   to changing both HSYNC and VSYNC. `sys/ascal.vhd:1204,1257-1258` consumes
   `i_vs` for frame state. Its claim of concurrent raw analog sync and unchanged
   scaler acquisition is unsupported. The user explicitly approved Raw CRT as
   the exception to Full timing after the shared-interface limit was explained.
2. **The proposed mode map does not satisfy the acquisition requirement.** It
   retains mode 1's Live HBLANK while later claiming Full blanking in modes
   0/1/3. Its description of current mode 1 VBLANK as filtered also disagrees
   with the selector's actual raw-VBLANK branch. The parent design replaces
   value 1 with Raw pixels and preserves all four Full timing members there.
3. **Full equivalence and SHIFT sampling conflict.** Latching SHIFT per
   character changes classic Full when SHIFT changes between fetches; the
   proposal both acknowledges that in S2 and claims classic Full unchanged.
   Disabling SHIFT for all Plus Full output also exceeds the selected output
   experiment. The parent retains Full's live per-fetch sampling and disables
   SHIFT only in explicitly selected native-pixel modes.
4. **A filtered-VSYNC-only mode latch can strand the user.** If no filtered
   VSYNC arrives, resetting to Full until that same edge cannot select raw CRT
   for the failure being diagnosed. The parent applies the requested mode on
   reset and otherwise commits during Full vertical blank at a safe byte phase.
5. **Raw pixel blanking needs a vertical owner.** Classic RGB includes GA
   vertical blank, while Plus RGB's final `blank` wire is HSYNC only. Replacing
   the output VBLANK with Full without an aligned raw vertical pixel mask loses
   that raw effect on Plus. The parent specifies an explicit dot-sampled tag
   through the colour boundary.

Useful retained findings: one shared framework stream, clipping outside DE,
existing HSYNC force blank, Plus word assembly after the shared byte mux,
separate sprite positioning, the per-extension Verilator build precedent, and
the distinction between GA-shaped H/V and framework-generated composite sync.
Claims about SHIFT's original intent and the visual effect of particular pulse
widths remain hypotheses; they do not authorize Full-mode repairs here.

The Opus proposal did not clear the implementation gate; the parent design
below is the reviewed implementation contract.

## Fable review of the parent architecture

The user explicitly requested this review. Fable 5.1 high completed
`20260911T051140Z-65685-ad71` with exit 0 and clean cleanup. Its exact verdict
was **CHANGES REQUIRED, narrowly**: the RTL contract checked out, with one
blocking fixture-specification correction, followed by permission to implement
after correcting the document. The original review is retained at
`docs/references/b6-boundary-2026-09-11/fable-review.log` (ignored).

The corrected specification explicitly uses the existing P10/B7 harness and
its real motherboard, CPU execution and SDRAM controller, seeds asymmetric
video data through the existing memory service, and makes sync selection a
runtime fixture input. This addresses the required correction without a new
hand-written VRAM-return substitute. The local mixed-dialect motherboard
elaboration also passes with real classic GA and ASIC; the historical
Verilog-only stub limitation no longer applies to that composition.

Fable verified the mode map, Full's per-fetch SHIFT contract, the CPU-phase
mode commit, reset escape, history priming, native-dot tag alignment, and
core-owned Raw CRT restrictions. Its nonblocking clarifications are also in
the design: explicit tag-register form after conversion, the reset-history
deviation from old Full, separation of horizontal and vertical force blank,
the unchanged Plus Full vertical-border follow-up, raw DE freezing under
absent HSYNC, and framework polarity correction as a hardware limitation.

**Parent acceptance:** the required document correction is complete; the
architecture gate permits implementation. This is fulfillment of Fable's
conditional verdict, not a claimed second review or an unconditional CLEAR
returned by the reviewer. RTL, simulation and hardware acceptance remain
separate stages.

## Implementation and code review

The implementation from `codex/general/b6-video-boundary` source `843cd5b`,
based on `20ed4d3`, is integrated into `master`. Opus 5 high implemented the motherboard mode/byte/tag logic,
selector, colour mask and Raw CRT restrictions. Its session could not run
builds or access `/tmp` because its approval layer denied those actions;
its report supplied no compiler or simulation evidence. The parent ran the
local gates below and retained the provider report separately.

The parent extracted the existing top output chain into
`rtl/amstrad_video_output.sv` to test the production crop/cadence path, added
its source manifest entry and focused fixture, and completed the persistent
mode-transition coverage through the native fixture writer. The top still
exports `en270p` to the existing menu-capability decoder. No `sys/`, CRTC rule
engine, ASIC renderer or shared fetch/address-ownership change was made.

Fresh **Sol high** review of the Opus-authored production changes and current
integration returned **CLEAR**, with no actionable findings. Fresh
**Gemini 3.8 Flash high** cross-provider review of the OpenAI fixture/output
extraction and the final integration also returned **CLEAR**
(`20260911T060600Z-89824-edae`). Both reviews apply to the source diff after the
output extraction and completed transition tests. Only documentation changed
after those reviews.

The parent narrows the Gemini report's coverage table: changing a programmed
HSYNC width is not a CPU-driven mid-line phase-change proof, leaf filter tests
are not a complete renderer-to-mixer malformed-raster proof, and the fixture
does not independently score an opaque sprite with scroll. Those distinctions
do not identify a code regression; they keep the broader diagnostic matrix
open rather than claiming it was fully executed.

## Local evidence

| Gate | Result |
|---|---|
| Pre-change production fixture | Full and native-byte controls pass; legacy mode 1 fails tuple, native-byte and DE equivalence assertions on classic types 0/1 and Plus |
| Pre-change state/mask cases | premature request and retained reset history each fail 1/1; raw vertical mask fails on classic and Plus |
| `make -C sim/plus b6-video-boundary` | pass: 18 machine/mode/width combinations; complete safe DE windows contain 768 enabled dots and Raw pixels matches Full's DE trace |
| Completed live transitions | pass on classic type 0 and Plus, width 5: four commits, two returned-low/qualified-prime checks and two reset-policy checks per owner |
| `make -C sim video-output-test` | pass: a real Full crop acquires 270 lines; after VSYNC stops, Raw CRT passes 32 uncropped 768-dot windows with 131,072 native-cadence checks despite conflicting retained settings |
| `make -C sim video-color-test` | pass: 182,272 checks, zero errors; mask alignment and metadata/Full controls across machine colours, fixed/adaptive enables and gamma modes |
| `make -C sim` | pass: aggregate suite, including the new B6 gates; 225 classic vectors; existing FDC XFAIL retained |
| `make -C sim lint` | pass; existing warnings and the documented mixer RGB limit remain |
| `make -C sim soak SOAK_EXPECT=0xb1cb70da95c2e44f` | pass: 2,845,088 sampled characters; canonical hash unchanged |

The old width-5 classic-type-0 mode-1 trace reports 101,519 tuple errors,
332,848 byte errors and 20,416 DE mismatches; its Full control has zero errors
and 301 complete 768-dot windows. Both use nonzero colour and 350,000
asymmetric fetch samples. The corresponding type-1 and Plus traces also fail
before the repair. These are fail-first evidence, not values mined from the
new implementation to define its expected bytes.

Temporary negative controls leave production source untouched. Removing the
final crop bypass, retaining adaptive cadence in Raw CRT, and retaining
scanlines each cause the output fixture's corresponding assertion to fail.
Swapping the Plus word-assembly halves causes 43,750 assembly failures in
each mode while the byte/tuple/DE checks remain clean. The original red fixture
already rejects the combined old Live tuple and Raw-pixels SHIFT routing.

The retained logs, exact provider reports, negative-control scripts and
pre-change fixture are under ignored
`docs/references/b6-boundary-2026-09-11/`. No PDF or generated build output is
part of the source change.

## Acceptance limits

The complete motherboard fixture uses real GA/ASIC/SDRAM and executing TV80,
not production-T80 edge timing. It observes converter RGB and real mixer
DE/CE. The untouched mixer's Verilator generate-scope issue prevents an
end-of-mixer RGB claim. The output-policy fixture drives a controlled tuple
into the actual extracted output chain; it does not replace the complete
motherboard fixture or establish physical connector timing.

The return-to-Full test checks the first shifted low byte and later nonzero
DE-qualified priming. Ordinary vertical blank may already have zero history;
the reset case is the deliberately primed, nonzero stale-history failure.
Dynamic malformed-raster rendering and the combined scroll/opaque-sprite
matrix row remain diagnostic follow-ups. No complete diagnostic-matrix,
ASCAL delivery, HDMI lock, synthesis or CRT acceptance is claimed. The
September 9 hardware defects remain open pending a new artifact and retest.


## Destination integration verification

The exact reviewed source `843cd5bb45eb7c3a70a5d2918d73a6f24a77757d`
merged without conflict into destination `20ed4d3`. Only shared status and
review prose changed during integration; the reviewed production and fixture
sources are unchanged. Destination `make -C sim`, lint and soak all exit 0;
the soak retains `0xb1cb70da95c2e44f`. The full suite includes all 18 B6
cases, production output/cadence/crop and colour checks, 225 classic vectors
and the existing FDC XFAIL. Logs are retained as `integration-*.log` in the
ignored B6 evidence directory. Exact-SHA CI and full-fit RBF delivery follow
publication; hardware and broader diagnostic acceptance remain open.
