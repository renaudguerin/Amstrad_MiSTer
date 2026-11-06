# Passive original-S observer — 4 November 2026

> Current correction,6November: this report preserves the named4November cohorts.
> Later fdf history fails0840/0440; the physical S word is reread4E40,
> superseding historical4E30 mentions below. Use the
> [current S report](shaker-s-current-2026-11-06.md) for the live position.

A default-off diagnostic captures the original CRTC1 S caller's missing live
FPGA state without changing functional counter rules. Two native passing fits
now exist: default production build `076eab1` (observer macro OFF) and parent-verified
diagnostic build `c16e7ec` (observer macro ON). Following the interlace adjustment
counting correction (`ccf2be7`), diagnostic execution on `c16e7ec` observes
normalized live hidden state: canonical history already arrives without inherited
adjustment (C5=0, adjustment OFF), and both direct and history paths compute 4E40,
matching the qualified baseline direct S image. Dated historical evidence from the
pre-fix diagnostic run on `ae9ccdc` (which observed history C5=31 at A4CF, C5=24 at
5BFB, and C4=2 with an added line active at 5B0A, yielding 0440) is preserved below.

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
`resumed-20261104/s-passive-fpga-trace/` (pre-fix) and
`resumed-20261104/s-passive-after-ivm/` (post-fix). The [S history report](shaker-s-history-2026-11-04.md)
owns the functional result and reference bounds.

## Pre-fix diagnostic build and direct native qualification (ae9ccdc)

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

## Pre-fix canonical-history qualification (ae9ccdc)

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

## Post-fix diagnostic qualification on c16e7ec

Parent-verified diagnostic branch `codex/s-passive-ivm-native-20261104` at commit
`c16e7eca6c574410d0f5d2b2cfee9bd30c19d898` is exact `076eab11a948e91e44279f5ef128c924a26b92ea`
plus ONE QSF enable `SSM_S_PASSIVE_TRACE=1`. Workflow 41933969717 passed all
required jobs (artifact 13375070713, build 372). Bitstream `Amstrad_20261104_c16e7ec.rbf`,
3,169,908 bytes, SHA256 `e6e6b848fff15d3e37c1bf5948aeb1c36654b856409955eb7d8ee01201f75f5e`.
Clean_full Quartus 17.0.2.602 Standard Fit / High Performance Effort: 7 setup and
7 hold clock groups, TNS 0, min slack +0.398 ns setup / +0.246 ns hold, zero errors,
zero critical warnings, installed hash matches on target device. Production observer
macro OFF remains unchanged.

Native execution confirms bounded resolution and observed hidden state normalization:
- Original direct S: 10 commands, 1 capture, 0 failures, 16 SSM records.
- Canonical history: 131 commands, 64 captures, 0 failures, 136 SSM records.
- BOTH direct S and canonical history S full RGB captures yield exact SHA256
  `b03c5e572b9d72e2a0cfb56b5c687f2e089b52d5cf38150f97fcee2dd50f4e2d`, printing
  the program expectation 4E40 identically to default `076eab1`.
- Preceding checkpoints E (`02C9`/`02CA`, 0080) and P (`02CB`, 26) full RGB are
  identical to default `076eab1` and baseline 9a.
- Two stable 1040-byte ring reads per path, 7 ordered valid cuts, error 0, dropped 0,
  all sequence numbers match header. Direct written 16 / history written 136.
- Timings: Acquisition direct 39,948 µs (2 failed retries); history 10 µs (0 failed retries);
  R8 pulse 8 µs for both; measurement 19,012 µs (1,188 polls) for both.

New hidden state is now actually observed on hardware:

| M1-completion cut | Direct C0, C4/C9/C5, adjustment | History C0, C4/C9/C5, adjustment |
|---|---|---|
| A4CF, before acquisition | C0=57, 29/7/0, off, IVM=0 | C0=59, 29/7/0, off, IVM=0 |
| A4DE, successful probe | C0=5, 30/0/0, off, IVM=0 | C0=5, 30/0/0, off, IVM=0 |
| A067, before R8=3 | C0=11, 0/0/0, off, IVM=0 | C0=11, 0/0/0, off, IVM=0 |
| A06B, after R8=0 | C0=19, 0/0/0, off, IVM=0 | C0=19, 0/0/0, off, IVM=0 |
| 5BFB, before R5 write | C0=58, 0/0/0, off, IVM=1 | C0=58, 0/0/0, off, IVM=1 |
| 5B0A, before R7 write | C0=13, 4/0/0, off, added line 0, IVM=1 | C0=13, 4/0/0, off, added line 0, IVM=1 |
| 5B17, measurement exit | C0=17, 0/1/0, off, parities=1, IVM=1 | C0=17, 0/1/0, off, parities=1, IVM=1 |

History now arrives at A4CF with C5=0 and adjustment OFF (before acquisition), and A067/A06B
are all 0 / off. Before R5 at 5BFB, C0=58, C4/C9/C5 = 0/0/0, adjustment is OFF, and IVM is 1.
At R7 fetch 5B0A, C0=13, C4/C9/C5 = 4/0/0, adjustment is OFF, added line is 0, and IVM is 1.
At exit 5B17, C0=17, C4/C9/C5 = 0/1/0, adjustment is OFF, and parities are 1. Direct exhibits
the same R5, R7, and exit states.

This replaces the old pre-fix history observation (A4CF C5=31 / adj ON, 5BFB C5=24 / adj ON,
and 5B0A C4=2 / adj ON / added line 1). Do NOT say new code clears C5 by R8 write or adds a
reset policy; it corrects IVM row counting during adjustment, and the full canonical prefix
now naturally arrives at S without inherited adjustment. The source fix remains independently
derived from French ACCC §11.8 p93; this is observed bounded state normalization, not scalar
coincidence or comprehensive C5 author adjudication.

Comparing the 64 captures of the diagnostic `c16e7ec` canonical history prefix against default
`076eab1`: 51 are RGB identical, and 13 raster pattern views changed (`MISTER_1_022B.png`,
`022C`, `0230`, `0231`, `0233`, `0236`, `0239`, `023A`, `023B`, `0287`, `0288`, `0298`,
`0299`), while S, E, and P are identical. Parent inspected all 64 contact views; each of the 13 changed RGB contents
exactly matches a previously inspected view in the 076/9a/ae9/599 corpus. No
novel content was found and no physical-photo verdict is refreshed. Physical photograph CS_02D3_A.webp (4E30)
versus program (4E40) revision binding remains open; there is no fresh global or photo baseline.

Concise proof files: `local/overnight-20261103/resumed-20261104/s-passive-after-ivm/{build-verification.json, qualified-native-pair.json, history-comparison/comparison.json, device-direct-c16e7ec/raw-observer/decoded.json, device-history-c16e7ec/raw-observer/decoded.json}`.
