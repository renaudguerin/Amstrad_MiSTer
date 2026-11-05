# Experimental classic GA: full build and bounded native checks

Source90f3b18a1af4c01f616943eb55039cf3fef45c11 on
`codex/classic-ga-boundary-native-20261105` has a clean full build and bounded
DSC4/Amazing native observations on classic CRTC0/1. This supports continued
experimental use. Strong serializer source review remains
[open debt](../../review-debt.md); these observations do not establish all
phases, full-demo completion, master integration or release acceptance.
The source-derived retained proofs are in the
[mode-boundary report](classic-type0-mode-boundary-2026-11-04.md).

## Artifact acceptance

[Workflow41964150943](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41964150943),
artifact13386879496, build377: all jobs succeed. Clean full Quartus17.0.2.602
has seven setup and seven hold groups, zero TNS, minimum setup/hold
0.316/0.243ns, and no errors, critical warnings or implicit nets. Diagnostic
S/DT macros are off in the flow report. Resource use is24,269/41,910ALMs,
28,362registers,701,596RAMbits/102blocks and35DSPs.

`Amstrad_20261105_90f3b18.rbf` is3,139,964bytes, SHA256
a7c3b0ad177744a3c2e2120365c3febdbd8226f0f80782603ccb1e3e8008a102.
The installed MiSTer copy matches. Exact artifact/timing receipts remain in
`local/overnight-20261103/resumed-20261104/ga-native-build/BUILD-ACCEPTANCE.json`
and its downloaded artifact. Existing final46-bench gate and exact source
hashes remain valid; no GA code changed during this unit.

## Native original-caller observations

All runs request Full sync filtering, French keyboard, Plus off and the named
classic CRTC, with original disk/typed command. The manifest's requested mode
is not an OSD/applied-mode trace. Root inspected every PNG.

| Run | Commands / PNGs / failures | Bounded observation |
|---|---|---|
|C0 DSC4|19 /4 /0|Credit colons and names visible; static picture and two main-effect samples visible.|
|C1 DSC4|19 /4 /0|Same clear credits, pixel-identical to C0 credits; static picture and two main-effect samples visible. Later sample contains thin green horizontal lines.|
|C0 Amazing|13 /2 /0|Two main-effect/scrolling-title samples visible, including the lower text/QR region.|
|C1 Amazing|13 /2 /0|Same bounded observables; first frame is pixel-identical to C0 first frame.|

The first C1 Amazing attempt timed out during SSH preflight before core load.
The process stopped, a bounded reachability check succeeded, and a fresh-output
retry completed. No failed attempt is counted as a demo execution failure.
Disk hashes remain DSC4
ec9e0d51b10a7455848bd9211793c585c46456759486057b12a491039923b8e4
and Amazing
4a631dabab9d8b9a916620a0e0b78f634daeae7736aee3c06b6c0e9979f1e8c9.
Original CSL, all PNG hashes, per-command timestamps, RBF/media identities
and CFG receipts are in the four`ga-native-build/device-*90f3b18*` directories
and`DEMO-RECEIPTS.json`. Capture round-trip overhead shifts the actual guest
phase beyond the CSL wait; these are bounded snapshots, not exact timestamp
matching between builds.

## Horizontal-line control and limits

A subsequent six-frame C1 DSC4 burst on accepted reference e24cb50, using
the same original boot and a54-second wait followed by2-second inter-capture
waits, also shows transient long green and other coloured horizontal lines.
Root inspected all six original PNGs. Therefore this symptom is not unique
to the new GA implementation. Different animation phases do not establish
equal frequency or pixel-exact equivalence, and the physical/reference
status of the transient remains unqualified. Do not silently label it fixed
or newly introduced by90f3b18. Control receipts:
`ga-native-build/device-c1-dsc4-burst-e24cb50/`,25commands/6PNGs/zero failures.

The older extra bottom row remains a separately documented
[viewport question](dsc4-bottom-row-crop-2026-11-04.md). A visible title picture
does not verify the individual old-quarter/R3.JIT pixel constraints; the
retained production-T80 tests own those bounded deterministic proofs.

The coherent current-T80 R7 optical comparison separately remains8/4 versus
9/5, with all57 captured columns identical between e24-reference and new GA.
It is neither fixed nor regressed in that qualified window. B6 admission,
C0 E6 post-entry rematch,02DA polling and stronger serializer review remain
separate open work. No fresh full SHAKER or Plus baseline was launched.
