# Passive original-S observer — 4 November 2026

A default-off diagnostic now captures the original CRTC1 S caller's missing
live FPGA state. It changes no functional counter rule. Full timing passes and both native paths preserve their exact baseline S
results, qualifying the seven live-state cuts for further source investigation.

`SSM_S_PASSIVE_TRACE` adds seven ordered physical-M1 completion snapshots to
the existing SSM observer. The payload includes C0/C4/C9/C5, adjustment,
effective IVM/toggle/added-line state, parities, VSYNC permission, selected
register, R0/R4/R5/R7/R8/R9 and live PPI input. Live PB0 is not an asserted
CPU-IN return latch. Address/opcode/tail and base-RAM/model guards qualify the
cuts; host source/SMC guards still own whole-program identity.

Snapshots buffer on chip without DDR backpressure into the machine. Fourteen
reserved payload records and one status record export only after DDR accepts
the genuine S02D3 header. They create no SYNC, screenshot or marker event.
The controlled walk ends at S; do not send another guest input before reading
the committed status. Raw timestamps are modulo32-bit64MHz ticks.

The retained transport proof fails before implementation and passes afterward,
covering held fetches, source guards, DDR/header stalls, payload ordering and
ordinary-marker preservation. A production-T80/READY checkpoint-instruction
replay compares disabled/enabled instruction/I/O/CRTC/RGB hashes, seven owner
cuts and the genuine S record. It passes717M1/50,000clocks with explicit test
trampolines; it is not original normal-boot hidden-state evidence.

Final selected gate: `python3 sim/select_tests.py --run`, PASS45benches once
after the last code edit. Default-off soak matches57463f9d37d0251e across
2,845,088samples. All five production files retain identical default-off
noncomment tokens. Fresh Gemini4Argon high review20261104T204518Z-74150-8699
accepts the frozen12-file change with no blockers. Reviewer shell hash checking
was blocked; parent independently verifies all20 source/netlist hashes against
the author's freeze. No gate rerun or subsequent code edit.

Enable the macro only in a separate temporary Quartus branch/project. Full
synthesis/timing and exact-baseline direct/history computed-S retention are
separate native acceptance gates. Normal production synthesis leaves it off.
Private handoff, guards, decoder, proof/gate/review logs and frozen hashes:
`resumed-20261104/s-passive-fpga-trace/`. The [S history report](shaker-s-history-2026-11-04.md)
owns the unresolved functional result and reference bounds.

## Exact diagnostic build and direct native qualification

Full [workflow41910599920](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41910599920)
passes all required jobs on ae9ccdc02d6ac941eddc9b4aab771b15218ffa20.
Artifact13363847112/Amstrad-build-363-1-full contains
`Amstrad_20261104_ae9ccdc.rbf`,3,139,340bytes, SHA256
`ed83e351fc650633c7e6021cf3f7521b38550b3265a5a8132c7b8c5a7a250d54`.
Reports verify cleanfull/Quartus17.0.2.602/StandardFit/HighPerformanceEffort,
macroSSM_S_PASSIVE_TRACE=1, seven setup/holdgroups, zeroTNS and minima
0.548/0.178ns. The distinct installed MiSTer file matches that hash.

Direct S completes10commands/onecapture/zero failures/16SSM records, retaining
4E40 exactly. Two identical1040-byte ring reads qualify seven ordered cuts,
error0, version1, written16, dropped0 and every retained sequence number.
The acquisition delta is119,824µs (six failed retries), pulse8µs, measurement
19,012µs (1,188polls), matching the independently measured reference formulas.
At the R7-write fetch cut, native state is C0=13/C4=4/C9=0/C5=0, effectiveIVM1,
frame/C9parity0, raw/publishedVSYNC0. At exit it is C0=17/C4=0/C9=1/C5=0,
parities1 and raw/publishedVSYNC1. These are actual M1-completion cuts, not
instruction-start CPU registers. Parent viewed the final PNG and complete
raw decoded record. The canonical history qualification below completes the pair.

A same-baseline9a history repeat stopped after28captures on an SSH key-command
timeout, with a separate SCP cleanup timeout. It never reaches S and supplies
no second scalar. The raw failed manifest is retained; subsequent bounded
SSH reads succeed and show no orphan MBC client. No emulator or FPGA verdict
follows from this transport failure. The completed baseline remains direct4E40/
history0440. Its64images were compared with e574:53pixelidentical, ten changed
raster-pattern views plus S; E/P0080/26 remain. No photo verdict was refreshed.

## Canonical-history native qualification

The unchanged canonical prefix completes131commands/64captures/zero failures/
136SSM records. S remains0440 WRONG, exactly matching production9a. Two
identical1040-byte reads validate all retained sequence numbers, seven ordered
cuts, error0 and dropped0. Independent immutable-label glyph matching and the
raw timing formula both confirm0440: acquisition10µs/no failed retry,
R8 pulse8µs and measurement68µs/four polls. Direct remains4E40/1,188polls.

| M1-completion cut | Direct C4/C9/C5, adjustment | History C4/C9/C5, adjustment |
|---|---|---|
|A4CF, before acquisition|29/7/0, off|29/7/31, on|
|A067, before R8=3|0/0/0, off|0/0/0, on|
|A06B, after R8=0|0/0/0, off|0/0/0, on|
|5BFB, before R5 write|0/0/0, off|0/0/24, on|
|5B0A, before R7 write|4/0/0, off|2/0/0, on; added line active|
|5B17, measurement exit|0/1/0, off|0/1/0, off|

Effective IVM is1 and frame/C9 parities are0 at both R5/R7 cuts. The original
R5 write receives different inherited C5/adjustment state; the resulting added
line is already active before the history measurement begins. This is an
observed state difference, not proof that clearing C5 or adjustment is correct.
French interlace counting during adjustment is the next source check. A fix
must also retain E0080/P26 and the author-resolved zero-R5 lifetime rules.

All64 diagnostic history images were compared with the exact9a baseline:
53 are pixel-identical, including E/P and final S; eleven differ in raster
patterns. Parent inspected the complete contact sheet and all eleven paired
panels. No new physical-photo verdict is inferred from those phase views.
Private evidence: native-{direct,history}-ae9ccdc/raw-observer/,
native-pair-qualified-values.json and native-history-page-comparison.json
under s-passive-fpga-trace/. The device is idle at the completed history S;
no observer read or caller retry remains active.
