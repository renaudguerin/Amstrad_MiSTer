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
| Type0 old-mode partial pixel | Non-aliased controls expose premature Mode1 and Mode2 decoding. Genuine R3.JIT mixed-mode control complete (OUT(C),E 14→2 at C022/hsc2/oldHS active); six fixed controls pass. 0→1 emits wrong 5B B pen 1 instead of 5B C pen 2 (causal old-SR ledger confirms 5B old-SR origin); 1→2 suffix 40..47 correct; blank release Type0 38 / Type1 39 vs 40. Universal old-pixel completion rejected. | Establish the native sampling edge and causal ownership at the mode mux and hold/shift controls. No candidate timing fix chosen. [Evidence](classic-type0-mode-boundary-2026-11-04.md). |
| Plus DSC4 credit labels | Accepted275af8a restores colons and names on MiSTer. Five targeted Plus runs retain sampled DSC4/Amazing effects, BI clipping/widths, AE/AR patterns and A4 deadlines. | Wider mixed-mode and full-demo acceptance remain bounded; the measured terminal mask is not a literal silicon-stage claim. [Proof and limits](asic-credit-blank-boundary-2026-11-04.md). |
| A4 lower VSYNC-bit words on Plus | PPI B bit0 reads raw CRTC VSYNC, independently of the sync filter. The native printed words differ; private full-caller result RAM does not reproduce them under either counter model. | Observe native RAM74F8..7507 to separate execution from formatting/display. Existing B18 refuses Plus saves. Do not infer a new admission rule from the saved-menu replay. [Evidence](asic-r0-terminal-latch-2026-11-04.md). |
| C1 E CPU-to-new-frame 0060 versus 0080 | Bounded positive-R5 C5 restart committed/pushed in 599ca80 after Gemini 4 Argon high review ACCEPT. Controlled replay returns 0080, 7 other metrics, 515 zero-route and 89 off-R4 controls (current CRTC/engines/GA, inherited T80 netlist bded028). Hardware acceptance pending on full build 41878239893. | Hardware verification of Amstrad_20261104_599ca80.rbf on MiSTer once build 41878239893 completes; direct C1 E/P/S and DSC4 checks. [Evidence](shaker-e-0080-positive-r5-reentry-2026-11-04.md). |
| CRTC1 ordinary R7 onset and 019D B6 | Source/reference gives ordinary Type0=5 / Type1=6 (ACCC §16.2.1 pp.161–162). Committed correction e574a46 adds abstract four-clock (+1 Pixel-M2) Type1 pin transport on both edges (reviewed ACCEPT), making ordinary NJIT 5/6 pass while keeping logical pulse width 16 lines and Type0 unchanged. Candidate E acceptance 0080 matches C5 baseline with +4 master ticks on all 1262 published edges. R7.JIT remains 8/4 vs source 9/5; B6 terminal-row admission remains 1F/1F vs 1E/1F. | Resolve terminal-row equality admission against current C4 rather than pin transport alone. The reference pair returned low for both controls; qualify its hidden phase before an admission candidate. No hardware acceptance claimed. [Evidence](crtc1-r7-onset-controls-2026-11-04.md). |
| C1 S02D3=0640 versus4E40 | Full-history failure remains; direct entry gives identical passing PNGs on old and new RBFs. A computed numerical result is not explained by asynchronous screenshot sampling alone. | Capture the relevant entry/history state. No precise C5 cause is established. [Evidence](shaker-s-history-2026-11-04.md). |
| DT early/late band history | The guest removes the second band about339ms after F8. Late same-run AmSpirit also has one band; native F8/F7 were already in one batch before its F8-labelled PNG. A separate earlier wrong-phase SNA inference is also withdrawn. | Qualify the early native window with the default-off passive trace. Current late images do not confirm a core/filter defect. [Evidence](shaker-dt-blank-boundary-2026-11-04.md). |

## Recommended order

1. The Plus credit fix 275af8a and Type0 E6 R9→R5 fix ee9c131 have targeted
   build/device acceptance. Type0 Last Line correction c3e896d, fixture repair
   28ba886, and Type1 positive-R5 C5 restart 599ca80 are committed/pushed in
   source 599ca80, with full build 41878239893 running. Case 16 remains a separate
   source question; do not infer its lifetime from a passing software model.
   [Last Line proof](crtc0-last-line-cancellation-2026-11-04.md).
2. Resolve the Type1 ordinary R7 onset with its Type0 positive control and
   separate original-caller JIT/B6 constraints. The ordinary NJIT publication
   candidate (+1 Pixel-M2 Type1 pin transport on both edges) passes 5/6 and
   candidate E acceptance, reviewed ACCEPT, but R7.JIT remains 8/4 vs 9/5 and
   B6 terminal-row admission remains 1F/1F vs 1E/1F. The reference pair returned low for both controls; its counter phase needs qualification. [R7 controls](crtc1-r7-onset-controls-2026-11-04.md).
   Genuine R3.JIT mixed-mode control is complete, showing 0→1 wrong 5B pen 1 from
   old SR and blank release 38/39 vs 40; universal old-pixel completion rejected,
   no timing candidate chosen. [Mode boundary](classic-type0-mode-boundary-2026-11-04.md).
3. Treat A4 lower rows and C1 S as state-observation work. C1 E positive-R5
   restart is verified in simulation and controlled replay (0080), awaiting
   device acceptance on build 41878239893. Preserve the smaller accepted R0
   mechanism; the broader queue has no discriminating advantage in the completed
   full-caller comparison. Keep author questions open where source inspection
   cannot distinguish mechanisms.
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
