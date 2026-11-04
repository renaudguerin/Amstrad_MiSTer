# Passive original-S observer — 4 November 2026

A default-off diagnostic now captures the original CRTC1 S caller's missing
live FPGA state. It changes no functional counter rule. Native timing and
preservation of the direct/history S results remain required before its data
can diagnose that failure.

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
