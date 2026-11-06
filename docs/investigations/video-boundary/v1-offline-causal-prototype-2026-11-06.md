# V1 offline causal monitor prototype

A fixed qualitative response now bends the qualified original T/Y callers and
an unseen D3 caller in the photographed direction, while preserving header
registration. This is an offline candidate for the separate CRT-response
profile in the [reviewed design](v1-qualitative-monitor-design-2026-11-06.md).
Full remains the production default; this work changes no FPGA behaviour.

## Current evidence

| Input | Observed candidate response | Acceptance |
|---|---|---|
| T/A | Left bend; settled8native pixels | Fixed training direction and causal transport pass |
| T/E | Right bend; settled8pixels, with boundary transients | Fixed training direction and actual pulse ownership pass |
| Y/A | Weaker left bend; settled4pixels | Exposed regression, not an independent gain holdout |
| Y/B | Left bend; settled8pixels | Exposed near-repeat characterization |
| D3/0314 | Left bend0→8pixels and visible recovery; header unchanged | First held-out response partially accepted; final blank tail has no measurable visual edge |

The earlier entrance-to-interior figures7pixels for T/A and3for Y/A subtract
an entrance that has already started moving; they understate the settled
8/4pixel offsets. These values characterize the candidate, not a calibrated
CTM monitor. No coefficient was selected to fit a photograph's amplitude.

D3 criteria and row ranges were frozen before its first model response.
Header rows35–60 remain RGB-identical with zero offset. The fixed entrance
and interior ranges show a multi-line left bend; the visible portion of the
fixed recovery range moves back toward baseline. Rows257–272 are uniform
border with no geometric edge, so their visual criterion remains unknown.
The saved engine offset is zero there, a separate state observation.

The initial evaluator incorrectly required an edge on every row, including
uniform border, and stopped after saving the complete first response. Its
runner/log are preserved. Assessment uses that same response, unchanged row
ranges and unchanged engine/preset; there was no model rerun or retuning.

## Causal and source qualification

One chronological feed path serves controls and real replay. Every native dot,
including blank, advances continuous RGB history; timing and both field tags
have a fixed64-dot delay. Each live HS captures a line offset which commits
with its own delayed HS in blank. Owner epochs prevent stale reads, and an
explicit missing-source flag re-primes the receiver. No title, row or pattern
branch selects a response inside the engine.

The ten private control groups cover cross-line tagged transport, source loss,
owner refill, persistent and cumulative ruler changes, rise-owned late pulses,
VS overlap, and strict timestamp ordering. Independent review supplied three
common-motion counterexamples which failed before the cumulative-coordinate
repair and pass afterward. After the final edit, controls plus continuous T
pass, all implementation/preset/input identities are frozen, then Y passes.
All four T/Y RGB12/offset arrays and twelve PNGs remain byte-identical to the
preserved predecessor. Independent real-input within-row transport checks
cover836181T/Y samples and209152D3 samples with zero mismatches; active-boundary
samples outside those checks are explicitly counted separately.

D3 independently matches all312rise-owned input windows. Its entire processed
stream has zero cumulative raster-coordinate displacement and no invalidation;
there are no target-bound windows. Engine SHA256 is
`b01eb92f295beab672bd1f28f86b9f3f1e18a4be82ae29fd63d024f6329f6fe0`;
preset SHA256 remains
`c343be8623db7bd154de9f6ca087996c97842240861277a101795cf88fd79ac0`.

The inputs are the [qualified original callers](v1-current-original-callers-2026-11-06.md)
under production T80/GA/CRTC composition, ideal RAM and the documented cold
module scaffold. Actual native-enable phase2 and acquisition windows are used.
This does not establish firmware/FDC boot equivalence or physical-display
fidelity. Current real traces have field tags0 and no source-loss assertion;
nonzero tags and missing-source transitions are synthetic controls.

## Bounds before implementation

Fresh Opus5.5 review accepts this frozen engine for the zero-coordinate
holdout domain. At nonzero coordinate displacement, primary selection,
early/late classification and rise ownership can still change under a common
translation of an extra-tip waveform. That residual needs its independent
counterexample and repair before a general dynamic monitor implementation.
It is not exercised by these accepted real streams.

Reset-assertion and committed mode-change paths, ordinary width4/5/6 model
response and a CPCSE-rendering comparison remain owed. General short-period
admission, synchronous RAM scheduling, packing/resources, RTL timing and live
sprite/owner transport also remain implementation gates. T's direction check
alone does not prove recovery; D3 uses its separately preregistered checks.
The signed fixed-point relaxation has a harmless sub-dot floor bias.

Private manifests, frozen gates, images and parent receipts are under
`local/overnight-20261103/resumed-20261104/`:
`v1-causal-sol-20261106/`, `v1-causal-pre-raster-review-20261106/`,
`v1-causal-opus-review-20261106/`,
`v1-raster-correction-opus-review-20261106/`, and
`v1-d3-holdout-replay-20261106/`. No public RTL or filter mode is added.
