# SHAKER-first work order and remaining discriminators — 4 November 2026

Use SHAKER and original-machine photographs to choose deterministic failures,
then source inspection and focused replay to isolate their owner. Keep the
affected demos as milestone checks. Full compliance is an objective, not a
single reliable pass/fail gate: input history, field sampling, PPI variants
and capture geometry can change a screenshot independently of the intended
CRTC observable.

## Failure ownership

| Observable | Established boundary | Next useful discriminator |
|---|---|---|
| CRTC1 DSC4 static picture's last coloured row | All768 pen-class positions match an uncropped AmSpirit row excluded by its default crop. Full admits a larger vertical window. This is a viewport/acquisition question; no MiSTer-only pixel pattern is established. | Define the intended monitor viewport against physical-CPC output. Do not change source pixels to match a software crop. [Evidence](dsc4-bottom-row-crop-2026-11-04.md). |
| Type0 old-mode partial pixel | Non-aliased controls expose premature Mode1 and Mode2 decoding. The error is already present before display filtering. | Establish the native sampling edge at the mode mux and hold/shift controls. A publication delay has not been selected. [Evidence](classic-type0-mode-boundary-2026-11-04.md). |
| Plus DSC4 clipped credit labels | The device clips them; a matched fresh FR AmSpirit run shows complete labels. A controlled production-CPU byte probe isolates correct data/pen/palette being suppressed by raw-HSYNC blank. It does not yet prove the title has the same first error. | Attribute an intact title colon to the blank mask. The ACCC's absolute type3 blank timing is explicitly an assumption, so retain that qualification. |
| A4 lower VSYNC-bit words on Plus | PPI B bit0 reads raw CRTC VSYNC, independently of the sync filter. The native printed words differ; private full-caller result RAM does not reproduce them under either counter model. | Observe native RAM74F8..7507 to separate execution from formatting/display. Existing B18 refuses Plus saves. Do not infer a new admission rule from the saved-menu replay. [Evidence](asic-r0-terminal-latch-2026-11-04.md). |
| C1 E CPU-to-new-frame0060 versus0080 | This is a measured CPU/counter result, beyond merely restoring the E/P display. CPCSE and AmSpirit reach0080, but their evidence does not select a unique hardware C5 mechanism. | Resolve the positive-R5 engagement/restart question with a faithful internal observer or the ACCC author. [Evidence](shaker-e-0080-positive-r5-reentry-2026-11-04.md). |
| C1 S02D3=0640 versus4E40 | Full-history failure remains; direct entry gives identical passing PNGs on old and new RBFs. A computed numerical result is not explained by asynchronous screenshot sampling alone. | Capture the relevant entry/history state. No precise C5 cause is established. [Evidence](shaker-s-history-2026-11-04.md). |
| DT missing/repeated band history | Earlier attribution to initial filter acquisition was withdrawn: the saved classic checkpoint omits counter phase and the original loop has a one-time setup path. | Establish faithful raw CRTC/GA boundaries before judging the filter. Full/Raw CRT screenshots alone do not isolate it. [Evidence](shaker-dt-blank-boundary-2026-11-04.md). |

## Recommended order

1. Complete the bounded Plus credit attribution. It can identify a pixel-path
   defect cheaply without another long cold-boot search. Require a fail-first
   byte/blank vector and the normal review/build/device gates before any fix.
2. Retain the Type0 mixed-mode failure and non-aliased control while resolving
   its sampling-edge architecture. Do not force the XFAIL green by changing
   its source-derived expectation or palette.
3. Treat A4 lower rows and C1 E/S as state-observation work. Preserve the
   smaller accepted R0 mechanism; the broader queue has no discriminating
   advantage in the completed full-caller comparison. Keep author questions
   open where source inspection cannot distinguish mechanisms.
4. Resume monitor/filter policy only with raw boundary evidence and an explicit
   physical viewport contract. Raw pixel metadata currently precedes the new
   classic RGB blank path; it is not an independent alignment oracle.

Use fresh CPCSyntaxError v0.12.0 CSL/SSM runs where its result matches the
original-machine photograph, then read the matching source to propose a
mechanism. Use AmSpirit1.17.1 where CPCSE fails or disagrees, and retain the
hardware photograph as the authority. CPCSE's logical model is useful evidence
but does not supply the physical sampling edge missing above. See the
[tool guide](../../cpcsyntaxerror-testing.md) and
[source audit](cpcsyntaxerror-v012-pilot-2026-11-04.md).

Do not repeat the whole corpus after every probe. Package-only changes did not
invalidate the existing A/B/C captures; behavior changes require targeted
milestone reruns. The fresh80-page C1 walk is preserved, including its reopened
S failure. A full fresh compliance baseline remains worthwhile once the next
behavior milestone and script entry conditions are stable.
