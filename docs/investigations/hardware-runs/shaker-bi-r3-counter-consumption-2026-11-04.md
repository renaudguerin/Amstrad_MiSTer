# SHAKER Plus BI: qualifier misses the R3 count decision — 2026-11-04

## Position

An authentic SHAKER BI replay on production T80/motherboard/ASIC timing
reproduces all six hardware regressions of the rejected `bedb1c5` bus
qualifier. Baseline `8dde1e7` passes all eight pages. The qualifier-only
counterfactual passes the two wide-band controls and fails the six short-pulse
cases: raw HSYNC stays asserted for 14 characters instead of 1, 2 or 3.

The first divergence is the ASIC counter's consumption of R3 at a character
edge, independently of pixel mode latching. In the 1-character case, the
baseline has R3=1 before HSC advances 0→1 and ends HSYNC. The qualifier admits
the same write after that edge. Restoring R3=14 before the eventual wrap then
makes the pulse end at 14 characters. No opcode exception, synthetic display,
CPU redirection, RAM patch or invented raster phase was used.

This is a valid private failure-first regression candidate. No production RTL,
tracked simulation, gate, device, live emulator/API, VM, commit or push was
changed or run by this unit. Integration and retention remain with the parent.

## Sources and independent expectations

French ACCC v1.11 §14.5.3 p.139 was read from the saved pdf-inspector primary
Markdown and its chronograms rendered and inspected. It shows CRTC 3/4's
character-synchronous HSYNC counting, a live R3 width comparison, and the
full-counter-wrap consequence when a lower terminal value has already been
passed. §14.5.4 p.140 was likewise read/rendered: it explicitly excludes the
classic CRTC 0/1/2 sub-character R3.JIT technique on CRTC 3/4. The expected
short pulses here end on whole character edges; they are not classic JIT
quarter-character pulses.

The original real-Plus reference photographs are indexed under ignored
`asic/b3-r3jit-real-plus-ref/index.json`, with hashes and direct Logon URLs.
For example, [BI CRTC3 B / 01DF](https://shaker.logonsystem.eu/images/cpc/CPC/BI_CRTC3_B.webp)
shows the thin band, between the wide and thicker-band cases. The photographs
provide independent hardware shape evidence; their perspective/scaled x
coordinates were not treated as internal C0 counts.

Parent-operated hardware on old RBF `5fba168` gave dark-band widths
`224,16,32,48,224,16,32,48` pixel-M2 for `01DE`–`01E5`; rejected `bedb1c5`
gave 224 for every page. Fresh AmSpirit Plus captures show the same old
pattern with the parent's dark threshold 8. These pixel observations
corroborate the raw counter expectations derived below; the replay directly
measures HSYNC rather than inferring it from colour or acquisition geometry.

## Authentic input and pinned implementation

All inputs are the unmodified fresh Plus BI snapshots under
`asic/b3-r3jit-amspirit-plus/{01DE..01E5}/`. They were captured from the
original B3 menu I disk flow, advancing pages with Space. Each snapshot is
version 3 with 128 KiB standard RAM and valid CRTC counters. Every replay
starts with actual saved PC `9FFD`, C0=8, C4=30, C9=2, R3=14. The runner
checks/reports the restored PC and continues the actual instructions.

Unlike the earlier classic v2 credits experiment, these are measured counter
headers. No field is manufactured to align a write. The original BI loop
continues and repeats normally; scoring excludes the first restored frame.
CPU timing, READY/WAIT, PPI, bus decode, ASIC register storage and HSYNC logic
are production modules.

The ignored unit is `local/overnight-20261103/asic/bi-replay/`:

- `base/` was extracted with `git archive 8dde1e7 rtl sim/b18_snapshot_top.sv`.
- `motherboard-old.v` is baseline `8dde1e7` with only the existing GHDL CPU
  port-case adapter, as in `sim/b18_snapshot.mk`.
- `motherboard-candidate.v` uses that same adapter on the `bedb1c5` motherboard.
  The only behavioural difference is the rejected ASIC CRTC `ENABLE` qualifier.
  All other RTL, including ASIC timing/mode logic, remains pinned to `8dde1e7`.
- `T80pa.v` copies the existing production GHDL netlist. Its tracked VHDL
  sources have no diff from the pinned baseline.
- `replay_top.sv` adapts the B18 fixture to Plus and external CPU/video RAM,
  adding observation ports only. `replay.cpp` streams the genuine standard
  header/RAM and records instruction, accepted-write, character-edge and
  raw-HSYNC traces.

The runner does not consume the optional extended Plus snapshot chunks;
this raw-timing test restores the standard CPU, RAM, CRTC counter and GA
header state. It makes no claim about restored Plus palette/sprite pixels.
Those extended graphics states do not drive the measured R3 counter decision.
Snapshot hashes and full source SHAs are in `source-provenance.json`.

## Caller and decision-edge proof

The first BI loop's critical instructions are:

| Address | Original bytes | Effect |
|---|---|---|
| `5AC1` | `ED 49` | select R2 |
| `5AC5` | `ED 69` | R2←46 for ordinary monitor pulse |
| `5AD2` | `ED 61` | R2←14 for the diagnostic pulse |
| `5AD5` | `ED 49` | select R3 |
| `5AD9` | `ED 51` | R3←target 0/1/2/3 |
| `5ADB` | `ED 61` | restore R3←14 |

The second four-page group uses the corresponding target write at `5AFA`.
These bytes are read from the genuine snapshots and executed by T80; no
replacement program is installed.

Baseline first-admission states are consistent across all repeated lines:

| Target R3 | Admission C0 | HSC at admission | Expected pulse |
|---:|---:|---:|---:|
| 0 | 13 | inactive/stale 14 | 14 characters: R3=14 is restored before its end |
| 1 | 14 | 0 | 1 character |
| 2 | 15 | 1 | 2 characters |
| 3 | 16 | 2 | 3 characters |

The zero case is a caller control, not a general claim that R3=0 means a
14-character pulse. In this sequence the later R3=14 restore occurs while
the long pulse is active and selects its eventual end.

An exact `01DF` example from frame 1, C4=6/C9=2 (the scored later frames
repeat the same states):

| Master tick | Baseline | Qualifier candidate |
|---:|---|---|
| 755593 | C0 13→14 starts HSYNC, HSC=0 | same |
| 755602 | admits/stores R3=1 at C0=14 | qualifier still excludes transfer |
| 755657 | HSC 0→1 sees R3=1; HSYNC falls | sees old R3=14; HSYNC stays high |
| 755665 | pulse already ended | admits/stores R3=1 at C0=15/HSC=1 |
| 755721 | inactive | HSC 1→2 misses R3=1 |
| 755858 / 755921 | R3=14 restore, pulse remains ended | delayed R3=14 restore while pulse remains active |

The first admitted transfer moves by 63 master clocks, across the decisive
64-MHz character edge. This is stronger than checking that a storage write
landed somewhere in a broad CPU microsecond window. Instruction traces are
unchanged between variants; the qualifier changes ASIC consumption, not the
executed caller's control flow.

The raw pulse starts at C0=14 and baseline ends at C0=15: 64 clocks = 1 µs =
16 pixel-M2. The candidate ends at the 14-character point: 896 clocks =
14 µs = 224 pixel-M2. Normal pulses starting at C0=46 stay 896 clocks in both.

## Deterministic results and commands

Build both pinned variants:

```sh
bash local/overnight-20261103/asic/bi-replay/build.sh old
bash local/overnight-20261103/asic/bi-replay/build.sh candidate
```

An individual authentic execution, from its output directory:

```sh
../obj-old/replay ../../b3-r3jit-amspirit-plus/01DF/01DF.sna
../obj-candidate/replay ../../b3-r3jit-amspirit-plus/01DF/01DF.sna
```

Each execution reaches five VSYNC checkpoints, approximately 5.1 million
master clocks, and over 45,000 real instruction boundaries. Per-page outputs
are `io.csv`, `hs.csv`, `seams.csv`, `instructions.csv` and `run.log`.

`score.py` uses the raw-HSYNC durations, not rendered pixel data. It requires
at least ten diagnostic and ten normal pulses, and scores only frames 2 onward.
Each final page result contains 297 diagnostic pulses and 936 normal controls.

| Page | Expected characters | Baseline clocks | Candidate clocks | Candidate |
|---|---:|---:|---:|---|
| `01DE` | 14 | 896 | 896 | pass |
| `01DF` | 1 | 64 | 896 | **fail** |
| `01E0` | 2 | 128 | 896 | **fail** |
| `01E1` | 3 | 192 | 896 | **fail** |
| `01E2` | 14 | 896 | 896 | pass |
| `01E3` | 1 | 64 | 896 | **fail** |
| `01E4` | 2 | 128 | 896 | **fail** |
| `01E5` | 3 | 192 | 896 | **fail** |

The failure-first scorer is independent of the qualifier implementation:

```sh
python3 local/overnight-20261103/asic/bi-replay/score_all.py candidate
# bi-replay: candidate 2 passed, 6 failed; exit 1
python3 local/overnight-20261103/asic/bi-replay/score_all.py old
# bi-replay: old 8 passed, 0 failed; exit 0
```

Exact outputs are `score-candidate-final.log`, `score-old-final.log`, individual
`score.log`/`score.json`, and aggregate `scores-{candidate,old}.json`.
The baseline is the restored-old counterfactual, not a new RTL patch: the
rejected qualifier is already absent from the parent's working tree.

## Minimal retention recommendation

Keep one cross-module production-T80/ASIC regression for this synchronous R3
termination boundary. Its primary acceptance must be the raw end decision and
normal-pulse control, not register storage latency or screenshot similarity.
The present eight-page private replay is the proven candidate and can remain
an ignored hardware-reference fixture while the parent chooses a CI payload.

For a compact retained test, preserve the actual BI caller instructions and
its original synchronization path, with target 1/2/3 plus the zero/wide control.
Do not commit complete user snapshots or photo assets merely to run it. Reduce
that authentic payload only against the full replay's accepted-write/counter
trace, and retain the source-derived expectations above. The duplicate second
four-page loop need not become a second test unless it protects a distinct
interaction. This is shared motherboard/ASIC counter state under active change,
so a focused end-decision regression satisfies the repository retention rule.

Do not resurrect the bus qualifier from a green register-window test. These
results prove the missing boundary in that candidate; they do not propose an
opcode exception, new bus delay or production fix. Concurrent pixel-mode work
was excluded by the pinned source and is outside this raw-HSYNC finding.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by
Longshot (CC BY-NC-ND 4.0).
