# V1 current original-caller inputs

Current production T80/READY, CRTC, GA and acquisition wiring now supply measured
T/A–F, Y/A–B and first D3/0314 inputs for the offline monitor model. These are
qualified source captures, not a monitor-response verdict. No FPGA behaviour or
bitstream changes. The [reviewed design](v1-qualitative-monitor-design-2026-11-06.md)
still requires causal collector, pixel-history and holdout acceptance.

## Execution boundary

Each sequence starts cold through the retained diagnostic reset ROM and ideal
RAM, runs the original module initializer and uses genuine HID input. It does
not reproduce firmware/FDC boot, SDRAM timing or a full canonical module walk.
The current GHDL-generated production CPU and 36 source inputs are frozen.
Original module bytes, the real French font, metadata copy, helper stub and
machine-type byte are guarded before interpreting the caller.

T and Y retain the earlier fixed, prefix-anchored Space schedule. Actual four
marker-byte fetches and the following instruction boundary independently prove
completion; cursor movement or an SSM prefix alone is insufficient. T completes
six callers in 226,741,240 clocks; separate Y completes two in 81,054,648.
Final T/F and Y/B field exports start before their marker completes. Those are
complete fields within the caller, without a wholly post-marker-field claim.

D3 uses a different original emitter contract. Its first `0314` marker executes
on departure after Space. A fixed elapsed input schedule captures stable
`35/06` before Space, then proves both `ED14 ED03` instructions and following
`A389`. The sole run ends at 53,005,000 clocks. Its last complete pre-Space field
is 50,830,223–52,108,175, before fixed Space at 52,925,815. Space is still held
at the first-marker stop; this is explicit transport variation, not canonical
70 ms input equivalence. The actual CPU ledger contains alternating R2 `2E/35`
and R3 `06`; the caption does not supply counter phase.

## Measured monitor input

Widths and intervals below are observations of actual GA HS at the 64 MHz
master clock. Incomplete field-boundary pulses are excluded. SYNC_N is also
recorded directly, rather than reconstructed from the filtered tuple.

| Caller field | Complete GA HS widths, clocks × count | Rise intervals, clocks × count |
|---|---|---|
| T/A–D, each | 256 × 399 | 1856 × 88, 2240 × 88, 4096 × 223 |
| T/E | 256 × 487 | 640 × 88, 1600 × 88, 1856 × 88, 4096 × 223 |
| T/F | 256 × 487 | 704 × 88, 1536 × 88, 1856 × 88, 4096 × 223 |
| Y/A | 142 × 88, 256 × 311 | 1856 × 88, 2240 × 88, 4096 × 223 |
| Y/B | 256 × 399 | 1856 × 88, 2240 × 88, 4096 × 223 |
| D3 `0314` | 256 × 375 | 448 × 64, 3648 × 64, 4096 × 247 |

Each selected field spans 1,277,952 clocks. Measured Full HBL-fall intervals are
4096 clocks, with 273 complete non-VBL windows of 768 native dots. Independent
every-enable accumulation establishes CE16 tick phase 2 modulo 4 and CE4 phase
14 modulo 16, with no gaps. Sparse point metadata must not be carried forward
as if it were a continuous signal. Per-row exports identify the actual first
native sample, which can follow HBL fall rather than coincide with it.

RawPixels source/public RGB agrees exactly. Both field sidebands remain zero.
Pair-valid toggles, but acquisition phase offsets and no_hsync remain zero:
these callers do **not** exercise nonzero servo correction, missing-HS recovery
or interlace. The offline engine still needs independent controls for those
paths. D3's composite trace retains its short VS-related event; this is not an
inferred physical sync-separator response.

## Ordinary-width source control

A separate unchanged public static guest supplies two local lines each for
CRTC1 Full widths 4/5/6, after three natural frame origins. One private build
and one three-case cohort measure actual GA HS/SYNC_N widths **133/197/256**
master clocks respectively; raw CRTC HS widths are 256/320/384. Each has two
complete equal pulses and rise interval 4096. The acquisition-period guard of
256 CE4 units is distinct from these pulse widths and supplies no expectation.

All three captures retain the same natural window 4,008,394–4,016,586 and
capture-local READY activity. Their independently measured native CE phase is
1 modulo 4, rather than the cold-module transport's phase 2. This reinforces
why a renderer must accept the actual phase contract per input. Full4's source
and final RGB planes differ; Full5/6 coincide in these windows. Those are
recorded planes, without a new physical-position verdict. The parent verifies
131 artifacts, 36 source bindings and unchanged public fixture originals under
`v1-current-ordinary-widths-20261106/`. No extra suite or DUT rerun follows.

## Model and evidence status

Both Gemini attempts and their logs are preserved. The second adds useful
receiver/buffer code but still resets history per rendered line and ignores field
inputs; parent acceptance remains withheld. A focused native Sol correction is
now active. The older T/Y traces have no recorded true CE16 points, so the
corrected pipeline uses current coherent inputs and their measured native phase
rather than guessing an old sampling phase.

The first Gemini offline implementation produces the intended T/A and T/E bend
directions, but parent review rejects its causal validation: several controls
repeat arithmetic outside the engine and its renderer indexes a completed
waveform without exercising the declared history. It remains geometry evidence,
not a passing implementation. Correction work keeps the preset frozen. Y is
already exposed and henceforth a regression check. Before exposing D3 to the
corrected engine, qualitative criteria are frozen against its original-CPC
photograph: registered header, leftward interior bend and recovery, without
fitting an amplitude or changing coefficients.

Private evidence under `local/overnight-20261103/resumed-20261104/`:

- `v1-current-callers-20261106/`: T/Y reports, inputs, sparse traces and exports;
  parent verifies 165 artifacts and 36 source bindings.
- `v1-current-d3-20261106/`: original first-caller report, pre-Space field,
  complete marker receipts and holdout registration; parent verifies 128
  artifacts and 36 source bindings, with unchanged qualified T/Y composition.
- `v1-offline-prototype-20261106/`: immutable attempt001, parent rejection,
  fixed preset and correction work. No production acceptance is implied.

Original hardware remains the oracle. These captures identify inputs and
presentation boundaries; they do not settle the outstanding silicon apertures
or confer SHAKER page grades.
