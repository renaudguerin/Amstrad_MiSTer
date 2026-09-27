# FF2 runtime PRI chain

## Scope and evidence

Investigation resumed from `7d80ddf` on `codex/plus/ff2-pri-investigation`.
Status: **integrated source candidate**, reviewed and gated (`d56ce1f`, identical
RTL/tests to reviewed `ff7e0a0`); exact-integration CI/synthesis and human
hardware acceptance are pending. The accepted nine-bit PRI comparison, ACK
provenance and CRTC3 repairs remain unchanged. The earlier emulator source survey is not repeated.

AmSpirit Lite 1.15.1/core 2491682, model 4/CRTC 3, runs the unchanged FF2 cartridge
(SHA256 `ce72fcf911b4b403a5012f8dedabd567c80a8af2e43e1b0646fd55468e48e794`).
After cold boot, a 700-frame wait target reaches the splash; a further
2200-frame wait target without input reaches attract gameplay. The captured image contains the cockpit, sky
gradient and road. This establishes the relevant program scene, not MiSTer
visual acceptance.

Private evidence in this checkout: `docs/specs/ff2-pri-2026-09-27/`:
`ff2_attract.sna`, `attract.png`, `attract-{cpu,raw}.bin`, `attract/` state,
`trace.json`, `trace-handlers.json` and their scripts/logs. The evidence and
replay sources are also preserved in the main checkout's ignored
`local/task-archives/ff2-pri-2026-09-27/evidence/`, excluding rebuildable object
files. Every emulator run
saves and restores the preceding machine snapshot, configuration, rendering
and pause state. Temporary PC breakpoints are cleared at completion. Final
configuration readback matches model 2/CRTC 1 and monitor Off.

## Observed write sequence

CPU-PC breakpoints stop before the direct `LD (&6800),A` instructions. The
preceding history includes the shadow write at `&9700`; the A register and
instruction bytes identify the intended PRI write without assuming register
readback support.

| Writer PC | New PRI | Role observed in the recurring sequence |
|---|---:|---|
| `8C30` | 46 | Arms the next frame's first palette handler |
| `8CAA` | 48 | Follows handler `8C72`, which copies sixteen palette bytes |
| `8BDF` | 50,52,…,112 | Advances the gradient chain by two each time |
| `8C51` | 200 | Ends the gradient chain and redirects the IM1 handler |

Ninety successive write stops show this chain recurring. A separate twenty-stop
pass identifies entries at `8C72` and `8B89` and their subsequent writes.
The game modifies the jump operand at `&0039`; a breakpoint at `&0038` itself
did not stop in this pass, so it supplies no interrupt-acknowledge observation.

At the four sampled `8C72` entries, the emulator beam coordinates are
(80,134), (96,134), (112,134), (96,134). Corresponding pre-write `8CAA` stops
are (960,135), (976,135), (992,135), (976,135). This repeatable instruction
interval is useful for program correlation. These are **emulator beam
coordinates**, not raw CRTC C0/C4/C9 or ASIC bus-write timestamps. The live
CRTC endpoint reports `rasterline=0` at these stops; do not use that field as
a current-line oracle.

Attract CRTC registers: R0=63, R1=32, R2=43, R3=142 (HSYNC width14), R4=38,
R5=0, R6=24, R7=31, R8=0, R9=7. The snapshot has 128 KiB RAM, a 2296-byte CPC+
chunk and an emulator-specific SPRT chunk. Saved mapping is all RAM, with
RMR=8C and no RAM expansion bank selected. ASIC paging is enabled temporarily
by `OUT (&7FB8),C` and disabled by `OUT (&7FA0),C` in the handlers.

## Handler and sound linkage

Read-only disassembly of the captured CPU image establishes the call chain.
The six direct PRI writers originate in cartridge bank cb01 and execute from
the copied RAM image at `8B08`, `8B2E`, `8BDF`, `8C30`, `8C51`, `8CAA`.
The boot entry contains explicit 16 KiB `LDIR` copies into `4000` and `8000`.

`8C72` saves registers, pages ASIC RAM, copies sixteen bytes from `8D0B` to
palette address `6400` with unrolled `LDI`, writes PRI 48, unpages ASIC RAM, and
redirects the IM1 jump to `8BC8`. That gradient handler increments the shadow
PRI at `9700` by two and writes two palette bytes per event. At shadow PRI 112 it
switches PRI to 200 and redirects the IM1 jump to `8B89`.

On the sampled zero-0750 path, `8B89` calls `6E1D` once and `6F51` three times.
The latter is an indexed sequencer whose downstream calls reach the PSG write
routine at `9128`/`912E` through PPI ports `F4`/`F6`. The nonzero-0750 path calls `8B73`.
Both join `8BFD`, which copies sixteen palette bytes from `8D1B`, writes PRI 46,
and redirects the next interrupt to `8C72`. This connects the palette chain and
sound service; it does not yet prove the cause of the measured music slowdown.

The disassembly used `/opt/homebrew/bin/unidasm -arch z80` against the private
CPU dump. Opcode T-state totals alone exclude CPC READY/WAIT and interrupt
acceptance, so they are not substituted for production bus timing.

## Production-T80 reproduction and candidate

The replay adapts the existing P10/D5 motherboard fixture, generates T80 from
this checkout's VHDL, restores the 128 KiB RAM and standard CPU/peripheral/CPC+
fields, and uses production 64 MHz clocking with READY enabled. RMR2 is restored
to 0, matching the saved page state; the older CRTC3 replay's provisional 18 is
not inherited. The SNA's CRTC beam counters are incomplete, so the trace spans
six VSYNC frames. There is no SPRT restoration or claim of exact emulator phase.

The first PRI 46→48 transition is identical before and after the candidate:

| Event on CRTC line 48 | Master tick | C0 |
|---|---:|---:|
| Raw HSYNC rises |199302|43|
| Monitor HSYNC falls; ordinary compare still sees PRI 46 |199686|49|
| CPU memory-write onset at 6800, data 48 |199878|52|
| ASIC PRI register changes 46→48 |199879|52|
| Raw HSYNC falls |200198|57|

All values in the table are decimal. One master clock is 15.625 ns. The PRI write
begins 3 µs after the ordinary event, with 5 µs of raw HSYNC still remaining.
The unchanged RTL produces no request at that write; its next PRI 48 event is
at tick 1477638 in the following frame. The failure recurs, rather than being
confined to initial snapshot synchronization.

The candidate remembers the previous stored PRI value and adds a current-line
write event while raw HSYNC is high. Existing nonzero/nine-bit comparison,
vertical-adjust suppression, reset/SNA history guards and ACK deferral apply
to the new event too. Held or same-value writes do not retrigger. Other PRI
writes do not clear pending requests. Ordinary monitor-HSYNC timing and ACK
provenance are unchanged.

At tick 199879 the candidate produces the missing raster event; INT asserts at
199880 through the existing request latch. Over the same six-frame replay:

| Observation | Unchanged RTL | Candidate |
|---|---:|---:|
| PRI updates |105|210|
| Entry to sound handler 8B89 |3|6|
| Frames containing sound entry |1,3,5|0,1,2,3,4,5|

This isolates the frame-long gap in the palette/music chain and removes it in
simulation. AmSpirit runs the same recurring chain without this gap. It does
not certify audible cadence, rendered pixels or original-Plus behavior.

Private replay sources: `replay/{Makefile,prepare.py,replay.cpp}`; unchanged
trace: `replay/baseline/{events.csv,replay.log}`; candidate trace:
`replay/output/{events.csv,replay.log}`. Exact command, from the checkout root:

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' CXX=/opt/homebrew/opt/llvm/bin/clang++ make -C sim/plus -f "$PWD/docs/specs/ff2-pri-2026-09-27/replay/Makefile" replay-run
```

Baseline: `REPLAY frames=6 ticks=7405510 pc=0x7426 pri_changes=105 hsync_edges=30/30 handler_opcodes=210`.
Candidate: `REPLAY frames=6 ticks=7405510 pc=0x730d pri_changes=210 hsync_edges=30/30 handler_opcodes=420`.

## Why the old alias comparator hid the missing event

An isolated diagnostic uses the pre-fix GA from `7d80ddf`, changing only
`{0,PRI}` to `{current_line[8],PRI}`. The current production RTL and candidate
logs are untouched. The same snapshot then yields 175 PRI updates and five
sound entries in successive frames 1–5, after its initial missed event.

The old comparator first rescues pending PRI 48 at line 304: event tick 1248262,
request 1248263, ASIC ACK 1248342. The handler subsequently writes PRI 50 on
line 305, then alias events on 306, 308, 310 advance the chain through PRI 54.
On later passes PRI 46 itself aliases at line 302. These are the extra events
that the hardware-bisected comparison repair removed. Their presence masks
the missing current-line write event; restoring them would also restore the
known Copter regression.

This is a causal simulation check of the bisect interaction, not an original-
hardware rule. The candidate requests the event on the actual matching line 48
and retains the full nine-bit comparison.

Private script `replay/prepare_alias.py` pins the pre-fix commit; its generated
`asic_ga_timing_alias.v` differs only at the comparator. `replay/alias-output/`
contains the independent trace. Reproduction after generating that source:

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' CXX=/opt/homebrew/opt/llvm/bin/clang++ make -C sim/plus -f "$PWD/docs/specs/ff2-pri-2026-09-27/replay/Makefile" REPLAY_OBJ="$PWD/docs/specs/ff2-pri-2026-09-27/replay/alias-build" REPLAY_GA="$PWD/docs/specs/ff2-pri-2026-09-27/replay/asic_ga_timing_alias.v" replay-build
"$PWD/docs/specs/ff2-pri-2026-09-27/replay/alias-build/ff2_pri_replay" "$PWD/local/test_media/cartridges/01_PlusGames/Fire And Forget II.cpr" "$PWD/docs/specs/ff2-pri-2026-09-27/replay/alias-output" 6
```

Result: `REPLAY frames=6 ticks=7405510 pc=0x730f pri_changes=175 hsync_edges=30/30 handler_opcodes=350`.

## Focused test and selected gate

`pr09_live_pri_write` fails on the unchanged RTL with
`FAIL: pr09: late matching write lost its request`, after the eight existing PRI
vectors pass. It passes after the repair, including DMA-ACK deferral/provenance,
raw-HSYNC qualification, the ninth compared bit, PRI zero, vertical adjustment,
pending-request retention and held-value controls. The CRTC-connected restore
test now distinguishes a legitimate live PRI-write event from a manufactured
line-entry event; its reset and snapshot suppression assertions remain intact.

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' make -C sim/plus run/asic_pri_tests
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' make -C sim/plus run/p1_video_tests
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' CXX=/opt/homebrew/opt/llvm/bin/clang++ GHDL=/tmp/eerie-ghdl/ghdl-llvm-6.0.0-macos15-aarch64/bin/ghdl python3 sim/select_tests.py --run
```

Final selection result:

```text
select_tests: PASS 7 benches: run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_pri_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests
```

Logs are under `gates/` in the private evidence directory. No full synthesis or
MiSTer/original-Plus test has been performed for this candidate.

## Independent review

Opus5.5-medium review `20260927T025413Z-4675-a24b` found no blocking issues.
It checked reset/SNA history, held values, shared guards, request retention,
ACK deferral/provenance, the counter-bit5 side effect and the revised P1 guard.
It ran no tests and made no edits. Its direct read of the separate CPCEC
checkout was permission-blocked; the parent directly read that source, while
the reviewer used the brief and checked-in survey for that comparison.
The completed wrapper log and exit status are preserved in private `review/`.

## Remaining acceptance

The write rule is supported by CPCEC's explicit FF2 handling and the connected
reproduction. It remains an emulator-supported model awaiting hardware
acceptance; the ordinary-PRI source conflict is not resolved by this repair.
Exact simultaneous PRI-write/raw-HSYNC-fall ordering is not established.
The review also notes that a matching write before the ordinary monitor edge
can now request early and, if acknowledged before that edge, receive a second
request there. This follows the adopted raw-HSYNC write rule but has no new
original-hardware measurement; it remains an acceptance boundary.

After building this candidate, use 6128Plus and Full sync:

1. FF2 no-input attract: stable sky gradient and normal music; then gameplay.
2. Copter 271 logo/title and World of Sports BMX: preserve palette/music fixes.
3. CRTC3: preserve boot detection and the repaired scenes/progression.
4. Eerie Forest: check progression and the known left-edge residual separately.

Eerie's existing residual is screen-plane data surviving a live SSCR change
through the horizontal delay. This FF2 result supplies no basis for a flush,
a wider mask, or CPCEC's separate clear-on-PRI-write policy. Those remain
separate investigations, as does Prehistorik II's bottom-HUD line.


## Eerie follow-up: this trigger does not match the recorded writes

A read-only check of the preserved production trace
`local/task-archives/eerie-graphics-2026-09-23/evidence/diagnostic/phase.log`
finds 1065 PRI-write onsets, 1064 in the documented steady window from master
tick 385900000. None writes a PRI equal to the logged full CRTC line. Of the
steady writes, only three begin during the recorded raw-HSYNC interval
(R2=49, width 11): C0=56 on line 265, old PRI 80h→01h, at log lines 33884, 43047, 52210.
The other 1061 begin at C0=11–29.

`diagnostic/phase.cpp` records the line from VC/RC and the write-onset phase;
`diagnostic/PHASE.md` records the setup. These are archived simulation bus
onsets, not original-hardware samples or a new replay of every possible demo
state. They provide no qualifying current-line write for this FF2 fix to act
on and therefore no demonstrated mechanism for it to repair Eerie's known
left-edge sliver. Preserve the existing SSCR/pixel-delay investigation and the
ordinary-PRI phase discriminator; do not add a speculative clear, flush or mask.
