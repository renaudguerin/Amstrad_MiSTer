# CRTC0 E6: qualify the original rematch stimulus — 5 November 2026

AmSpirit's original cases27/28 return0065/000E. A two-operand case27 control,
which preserves instruction timing while leaving R4/R9 unchanged, returns0055
and removes exactly256µs (four64µs lines) from the measured polling delay.
This supports a real software-reference difference at the coupled rematch,
not an expected-number transcription. It does **not** establish a physical
CRTC rule or authorize a new RTL policy. The unsent
[author question](../../classic/accc-author-questions-crtc0-c0-1-2026-11-04-fr.md)
remains necessary.

## Source question and method

The existing [case27/28 source judgment](crtc0-forced-zero-adjustment-2026-11-04.md)
separates post-entry R4 restoration from the already-fixed forced-entry/first-line
R9 cases. A focused Astra medium rereading of French §§11.2.2,12.2,13.2.1 and
13.2.4 found no unambiguous rule for restoring C4=R4 after adjustment already
owns C9. The wrapper's `frame_adj_r & ~in_adj` and engine's R5-owned comparison
jointly implement the present one-shot behavior; changing only a register-write
gate would not settle that policy.

The new experiment qualifies public instruction-relative timing without using
classic snapshot counter bytes or video beam coordinates as C0. Both original
cases run from a normal boot,6128/128K/French/CRTC0, AmSpirit Lite1.17.1,
core2491682, monitor Off, effective `freq_screen=1` in both captures. The
pre-boot request was0; the published value after CSL reset is1 and owns this
comparison. No runtime result was changed to match that setting. Disk SHA256:
`6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
SHAKE27E body SHA256:
`1a74786431ffedee46e52df1a16f069f7a3ae36f37260c0dfeea645ffb34bca1`.
Original E6 page inputs are reused; source ranges, stack return and public
registers are checked at ordered physical-RAM breakpoints. No PC, counter or
original-run guest writes occur.

Per case the ordered chain is entry6588/65FF, synchronizer8768, first877B,
first8780, return658B/6602, before/after R5/R4/R9/R7 OUTs, then65C9/6643
before cleanup. Breakpoint replacement's implicit resume is acknowledged before
checking the target; there is no unconditional second resume. First8780 is
essential because that address later repeats in the width-counting loop.

At first877B, carry0 records the preceding8776 PPI sample low. At first8780,
carry1 records the first successful877B sample high; A.bit0 has already been
rotated and is not the VSYNC bit. In both originals, `T−B=18,943=8×2367+7µs`:
negative iterations take8µs and the positive exit7µs. Without an independently
measured IN sampling offset, the rising edge lies conservatively in
`(T−15,T−3]`, a12µs interval, not an exact8µs phase measurement.

## Original pair

The public initial tuple is R0=63,R4=38,R5=0,R7=30,R8=0,R9=7. Actual OUTs
change R5→22,R4→39,R9→3, then R7→0/40. Whole instruction intervals bound
accepted I/O; instruction start is never substituted for its acceptance edge.
After accounting for4/6 complete19,968µs frames after the sampled VSYNC:

| Observable | Case27,µs relative to bounded VS origin | Case28,µs |
|---|---:|---:|
| R5 OUT acceptance bound |4534..4550|4536..4552|
| R4 OUT acceptance bound |4612..4628|4614..4630|
| R9 OUT acceptance bound |4626..4642|4628..4644|
| R7 OUT acceptance bound |4640..4656|4642..4658|
| Poll exit before cleanup |6280..6292,HL0065|4874..4886,HL000E|

With the unchanged ordinary tuple, the preceding last-line origin is4544µs
and first additional-line origin4608µs after R7=30's origin
(French §11.2.2 pp82–83,72lines×64µs). R4 is therefore inside the first
**would-be** additional line. These conservative bounds alone do not qualify
R5's narrow C0≤2 admission aperture, so the original timing argument remains
conditional on entry. No hidden(C4,C9) values were read.

## Timing-preserving case27 control

A fresh normal boot applies only65A4:27h→26h and65AE:03h→07h before E6 runs.
These immediate operands keep R4=38 and R9=7. Every instruction and delay,
R5=22 and R7=0 are preserved. Only these two byte differences are allowed by
the runtime source guard. No PC/counter/CPU redirect or phase forcing occurs.

Both first polling anchors and all eight before/after OUT timestamps differ
from the original case27 by **exactly one19,968µs frame**. Thus the tested
write schedule has the same phase relative to its public synchronization.
The control's polling exit has HL0055 and occurs256µs earlier after removing
that whole-frame shift. Its timing is consistent with ordinary22-line
adjustment; it is far from the no-adjustment hypothesis's next-origin timing.
This qualifies that control path in AmSpirit and makes the coupled R4/R9
rematch effect concrete. It does not identify a unique internal comparator
latch, independently resolve case28's R5 bus aperture, or establish which
hardware rule to implement.

## Evidence and disposition

Private evidence:
`local/overnight-20261103/resumed-20261104/c0-rematch-original-qualification/`:
SOURCE-PLAN.md, original-001/, guard-001/, RESULT.json and source/artifact hashes.
Both captures complete; scripts/breakpoints are cleared and AmSpirit is paused
at the control's65C9. A private-driver syntax error occurred before any API
execution and was corrected before the first capture; it is not a machine result.
No production RTL, simulation vector or global gate changed for this experiment.
The existing26/28 native tally and remaining0055/02D6 are unchanged.

The next useful external input is the precise post-entry transition from the
author or matched original-hardware observation. Emulator agreement alone
cannot choose between the one-shot and re-arm policies.


## One-operand controls: isolate the R4/R9 interaction

Two fresh normal boots use the same guarded driver and one immediate change each: R4-only restoresR9=7 while keepingR4=39; R9-only restoresR4=38 while keepingR9=3. Every instruction and both OUT strobes remain. Both complete once on the same1.17.1/core2491682/configuration and effectivefreq_screen1; published post-OUT CRTC registers confirm R5/R4/R9/R7. No hidden state or PC/CPU redirect is used.

|R4 after OUT|R9 after OUT|HL result|Poll exit relative to original|
|---:|---:|---|---:|
|38|7|0055 (existing guard)|−256µs|
|38|3|0055|−256µs|
|39|7|0075|+256µs|
|39|3|0065 (original)|0|

Both new runs have all ten sync/OUT anchors at exactly the original timestamps. The old guard’s anchors differ by one19968µs frame. After that normalization, R4=39 adds512µs whenR9=7 or256µs whenR9=3, compared with the R4=38 corner. The observed R9 sensitivity is conditional on the restored R4 value in this four-corner stimulus; this is consistent with eight/four64µs lines, without proving which internal counter transition produces them. It is more specific than the original two-operand comparison, and does not independently qualify the R5 admission aperture or a physical rule.

Private `c0-rematch-factorial/` preserves the pre-execution plan, two single-operand drivers,56 artifact hashes, all ordered states/operations and RESULT.json with phase and published-value checks. Scripts/breakpoints are cleared and AmSpirit is paused after R9-only at65C9. The unsent French question now includes the matrix; no RTL or native26/28 tally changes.

## French rule audit and remaining authority gap

A focused independent Opus5.5 audit reads the French position-aware text and renders of pp76,81–83,105–108. Root checks the comparator passages visually on pp82,83,108. §13.2.4 p108 attributes the R9→R5 switch to the C4/R4 difference and says C4 increments once whatever R5; it does not explicitly describe restoring R4=C4 after that increment. §11.2.2 p83 example3 also retains R9 ownership for several already-active adjustment lines until C4 changes. The text therefore does not establish the current lifetime restriction under later R4 writes, but it also does not explicitly establish a reverse R5→R9 transition.

The source owner for that choice is the type0 engine’s comparator selection. The wrapper’s no-re-entry mask is separate: adjustment cancels Last Line, so a live-comparator hypothesis need not remove that mask. The current whole-adjustment restriction remains an integration assumption rather than a proven French rule. No RTL change follows from this audit. The four-corner software timing fits both a live selector and a later re-detection model in the observed cases; original-CRTC0 case28 or an author answer is still needed.

The unsent question now retains the “whatever R5” qualifier and asks directly about the comparator after rematching R4. Private `c0-rematch-rule-audit/` preserves REPORT.md, rendered French pages, provider logs, hashes and the parent disposition. Native E6 remains26/28.
