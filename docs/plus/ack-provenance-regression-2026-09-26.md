# CRTC3 repair regressions: acknowledge provenance

## Implementation checkpoint (2026-09-27)

The narrow production repair now qualifies `last_raster` capture once per CPU
M1 acknowledge window. Reset, SNA load and M1 high re-arm capture. Both shaped
ASIC pulses still reach vector selection and DMA auto-clear; no PRI timing or
DMA retirement rule changes. The existing distinct-M1 diagnostic confirms a
later CPU acknowledge replaces the previous raster provenance.

The two B20 diagnostics now assert preserved raster status separately from
vector06→04. Their existing stimuli already distinguish two pulses in one M1
from two CPU acknowledgements, so no driver changes or new benches were needed.
Focused B20 ACK and production-T80 bus diagnostics pass. The selected gate
passes all seven benches; the additional existing P8 fixture passes snapshot
provenance cases. Hardware confirmation in Copter/BMX and CRTC3 remains pending.
Opus5.5-medium review `20260926T231805Z-23858-6dee` found no blockers and
confirmed raw M1 scope, first-pulse sampling, reset/SNA ordering and test
stimulus. It identified an unmeasured corner: DMA first, raster arriving in the
split-ACK gap, then raster retired on the second pulse. The candidate preserves
DMA provenance there. OR-ing later raster provenance could change that outcome,
but would introduce another unmeasured contract. Defer it rather than broadening
this first hardware candidate; revisit if residual mixed-source failures remain.
DMA automatic retirement on a second pulse is also intentionally unchanged.

Verification commands (no code edits followed these passes):

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' GHDL=/tmp/eerie-ghdl/ghdl-llvm-6.0.0-macos15-aarch64/bin/ghdl make -C sim/plus b20-ack-diag b20-bus-diag
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' CXX=/opt/homebrew/opt/llvm/bin/clang++ GHDL=/tmp/eerie-ghdl/ghdl-llvm-6.0.0-macos15-aarch64/bin/ghdl python3 sim/select_tests.py --run
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' make -C sim/plus run/plus_p8_tests
```

The initial focused compile used the system compiler because environment-only
CXX did not override nested make; `MAKEFLAGS` fixed compiler selection. No
behavioral failure occurred. Logs and review are preserved in ignored
`docs/references/crtc3-2026-09-25/regression-fix/`. Exact selected result:

```
select_tests: PASS 7 benches: run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_pri_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests
```

## Integrated hardware candidate

Implementation `0c2cebb` is integrated and pushed as
`cf62d5f63de0f103b6e3715df9345b9b2cc79034`. Exact-SHA
[CI run36279150334](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/36279150334)
passed simulation, production-T80, full hosted synthesis and required gate.
The downloaded artifact is `Amstrad-build-275-1-full` (ID10917987698), with
`build_mode=clean_full`, Quartus17.0.2 and target5CSEBA6U23I7.

Delivered RBF:
`/Users/renaudg/code/Amstrad_MiSTer/output_files/Amstrad_20260926_cf62d5f.rbf`.
SHA-256, verified after copying:
`ae4d5b7881f3a6740d40a02fcea78762cef3ae49c943329fc800c9714d3f27a7`.
The filename uses the build's UTC date. Reports and another RBF copy remain
under the task checkout's ignored `output_files/provenance-cf62d5f/`.

Setup minimum +0.331ns, hold minimum +0.178ns, zero setup/hold TNS on all seven
reported clocks. Resources:24,427/41,910 ALMs (58%),28,244 registers. TimeQuest
still reports the existing incomplete setup/hold constraints; passing timing
applies to the constrained paths. Full compile elapsed20m36s.

No device loading, capture or emulator replay was performed. This is the
reviewed hardware candidate, not confirmed closure of Copter/BMX. Check:

1. Copter271: Loriciel moving logo, title sprite motion and palette boundary,
   then gameplay/HUD split.
2. World of Sports BMX: sustained music/timer cadence, then warm reset or
   cartridge reload and title-to-menu behavior. Watch for progressive drift.
3. CRTC3: FlowLIB without the real-machine warning, repaired scenes still clean,
   and playback past the former2:58 freeze.
4. PrehistorikII gameplay after those checks; use `edaa15b` if a baseline
   comparison is still needed. Eerie Forest's left strip remains open.

## User evidence and narrowed range

The user confirms all listed CRTC3 defects fixed on full-effort `e4445a1`.
Eerie Forest's left green strip remains. Copter271 and World of Sports BMX have
major regressions also present in `1eec469` and `bbd7348`. Prehistorik II is
broken but has no confirmed recent baseline and requires more interaction;
defer it initially. Captures: `local/captures/regression_26092026/`.

Known-good RBF is `edaa15b`. Comparing it to investigation base `18c2dce` shows
no changes to production RTL or synthesis inputs: intervening changes are docs,
agent instructions and release tooling. Thus the first suspect pair is
`bd65578` (board-shaped IORQ/acknowledge) and `1eec469` (cross-line PRI).
Later sprite-cache and DMA-operation repairs are not required to trigger these
regressions. Settings are 6128Plus, Full sync, unchanged cartridges except the
new Prehistorik II file.

Copter: Loriciel boot animation flashes/stutters; title sprites stutter and the
palette boundary between upper logo and lower background fails; gameplay has
an intact-looking HUD above badly corrupted playfield. A mode1/mode0 split is
user-suspected, not yet established from the game code. Earlier no-input title
captures were insufficient temporal/gameplay acceptance and must not override
this user result.

World of Sports: boot title unexpectedly exits to menu without fire; select
default Play All Events with fire, wait5–10s for BMX. Music and displayed timer
are slow and become slower; pitch stays correct. Reset or reload the same CPR
from BMX yields a slow title that no longer immediately exits. After seconds
or fire, the menu returns to normal music speed. This suggests lost/misclassified
software service work; it does not prove the CPU or PSG clock slowed.

## Concrete source finding

`bd65578` changes GA and register-page `intack` from raw CPU acknowledge to
ASIC-side shaped pulses. A susceptible memory instruction at A13=0 can produce
two ASIC pulses within one CPU M1 acknowledge; vector06 then04 is the observed
hardware bug that FlowLIB detects. Low-A13 HALT is a measured nonsplitting
control, so PC region alone must not predict a split.

The changed input also reaches consumers that were written for one pulse:

- `asic_ga_timing.v` resamples `last_raster` at every `intack` rising edge. The
  first pulse records raster and clears the request; the second empty pulse
  replaces the provenance with zero. DCSR bit7 then says not-raster.
- `asic_regs.v` can auto-clear another pending DMA source on the second pulse
  when IVR bit0 is zero, because the raster request is already cleared. The
  hardware bug explicitly also affects automatic DMA mode; its precise flag
  retirement remains unmeasured and should not be silently redefined.

The existing Copter investigation in `docs/backlog.md` B19 establishes why this
matters: its IM1 handler at0038 reads6C0F about14µs after ACK and branches on bit7.
Raster dispatch drives the title palette chain (PRI FF/37/A7); an erroneous
DMA dispatch loses the raster operation. This is a direct integration risk of
ACK shaping, independent of additional PRI events.

The FlowLIB proofs mistakenly made the changed provenance an expectation:
`b20_bus_diag_test.cpp` requires DCSR0 in the split case, and
`b20_ack_diag_test.cpp` requires second-pulse raster provenance to disappear.
The physical vector trace does not establish those status-register effects.
Those checks therefore codified an integration assumption rather than proving
it. Preserving vector06→04 and preserving software-visible raster provenance
are separate requirements.

## World of Sports static evidence

`local/test_media/cartridges/01_PlusGames/World Of Sports.cpr` is 131,148 bytes,
SHA-256 `1a99e91af21751bfeede749e1233a9ec3c5fd50c79b01b93e7c6858d456e7130`;
the corresponding `test_media` copy has the same identity. A bounded static
scan of chunk `cb00` finds IM1 (`ED56`) at offset33BA, and at33D6:
`LD A,(6C0F); BIT 7,A; JR NZ,+8`. The set-bit branch skips
`AND 87; LD (6C0F),A; JP 3485`. These are offsets in that cartridge chunk,
not a verified runtime mapping.

This independently establishes that the cartridge contains status-based IRQ
classification matching the suspected failure mechanism. It does not establish
that this routine is active during BMX or the slow title. No saved World of
Sports snapshot, identified timer variable or pre-existing disassembly was
found. The supplied regression capture folder contains Copter/Prehistorik II
images, not a World of Sports capture; the BMX timing evidence is the user's
report.

## Evidence and uncertainty

CPCWiki *Plus Vectored Interrupt Bug*, revision115513 (S29), describes the first
ASIC pulse clearing raster, recording DCSR and presenting its vector; the
second presents DMA0 because no request remains. Its software workaround uses
manual clearing and common/IM1-style dispatch. Existing complete text extraction
is `/tmp/crtc3-flowlib-s29/Plus-Vectored-Interrupt-Bug---CPCWiki.md`; original
sources and physical trace are preserved under ignored
`docs/references/crtc3-2026-09-25/flowlib/`.

Opus5.5-high read-only assessment `20260926T223608Z-13182-f640` independently
ranks repeated bookkeeping in `bd65578` first and recommends first-pulse
qualification while keeping vector shaping. Its suggested low-A13 HALT test
would not produce the measured split; use the existing LD(DE),A case instead.
The review did not inspect the new images or run tests. Its claim that standard
HSYNC excludes PRI is conditional on the games' actual register values, which
have not yet been established for BMX/gameplay.

Reset/reload does reset CPU, CRTC registers, ASIC control/SAR/PPR/DCSR and DMA
counters/ownership. It does not clear main SDRAM. Consequently the user's slow
second title can reflect retained game RAM or recurring address/instruction
phase dependence; it does not establish retained DCSR or DMA prescaler state.
Some GA sequencer/sync/counter state is not explicitly reset, but that alone is
not an explanation for prolonged slowdown. No immediate combinational feedback
loop from shaped IORQ to READY was found. Ordinary GA writes require M1 high;
CRTC/PPI/MMU accesses require RD/WR, which are inactive on an ACK.

## Bounded production-CPU diagnostic

Frozen baseline: `c48d84144d6f8dc092c238407fe324fae78e4677`. The existing
production-T80 bus diagnostic was copied into ignored scratch space, with the
handler's DCSR expectation changed to preserve first-ack raster provenance.
This is a candidate contract, not a newly measured hardware rule.

| CPU case | Baseline DCSR | Candidate DCSR | ASIC vectors, both versions |
|---|---|---|---|
| Low-A13 LD(DE),A | 00 (fails candidate contract) | 80 | 06 then 04 |
| High-A13 LD(DE),A | 80 | 80 | 06 |
| Low-A13 HALT | 80 | 80 | 06 |

The candidate adds `raster_ack_seen` in `asic_ga_timing.v`, clears it on reset,
SNA load or M1 high, and qualifies `last_raster` capture with its old value.
It changes neither board IORQ shaping nor DMA automatic clearing. All 206
TRACE/SAMPLE lines are identical after removing the baseline's interleaved
failure message. In particular the susceptible CPU still consumes vector04
at tick107410, with unchanged READY, WAIT, address, M1, T-state, clock-enable
and IRQ traces. This isolates the software-visible status change from bus
phase and vector presentation.

Commands (already run; do not repeat as a baseline):

```
python3 sim/plus/obj_dir/dcsr_provenance_diag/run.py baseline
python3 sim/plus/obj_dir/dcsr_provenance_diag/run.py candidate
```

Baseline build/test: 3.078/8.332 seconds, test exit1. Candidate: 2.617/8.274
seconds, exit0. Builds use the existing production-T80 recipe and Homebrew LLVM
C++ because the system compiler cannot find its standard headers. Logs include
exact expanded commands. Sources, candidate, identity manifest, commands, logs
and comparison are preserved under ignored
`docs/references/crtc3-2026-09-25/regression-provenance/`; executables remain in
`sim/plus/obj_dir/dcsr_provenance_diag/`. The archived runner assumes its original
scratch location; restore it there before rerunning. Candidate SHA-256:
`bdd6dc05c8cbbae8dd50f5b59efbb5ee38e075db938b082c467fe4b2a9441b22`.

**Limits:** synthetic handler, IVR=0 with no DMA pending; no Copter/BMX replay,
no new hardware observation, no production RTL edit. The result proves the
present status overwrite and demonstrates an isolated remedy. It does not
establish the real ASIC's second-pulse DCSR semantics or close either title.

## CPCEC source comparison (2026-09-27)

Read-only reference: `/Users/renaudg/code/cpcec`, commit
`c025aab961a796b918cc99bc3e16216ea65bb5d1` (2026-03-10). The inspected
`cpcec.c` and `cpcec-z8.h` have no local modifications. The user's successful
CRTC3, Eerie Forest and Copter runs are compatibility evidence; this inspection
does not add a new emulator run or a hardware measurement.

CPCEC supports separating vector delivery from acknowledge bookkeeping:

- `cpcec-z8.h:211–223` chooses the IM2 vector (or fixed IM1 entry) and then
  calls `Z80_IRQ_ACK` exactly once for that CPU interrupt.
- `cpcec.c:1264–1289`, `z80_irq_ack()`, sets DCSR bit7 when raster is pending,
  clears that pending raster and retains pending DMA requests. Otherwise it
  clears bit7 and services the highest-priority DMA source. It does not call
  this routine twice to reproduce the vector bug. Consequently an IM1 raster
  handler still reads raster provenance, matching the narrow repair's intent.
- Its vector bug model is explicitly approximate: `plus_8k_bug` starts at0
  (`cpcec.c:245`), a raster ACK changes it to6 (`1267`), and `Z80_IRQ_BUS`
  (`1261`) uses it when raster is pending and PC A13 is low. Thus the first
  such vector after reset can use offset0 instead of6; it does not model our
  measured two-pulse06→04 sequence, READY phase or interrupted instruction.
  After a raster ACK, that shim returns6 even at low A13. Passing FlowLIB in
  this emulator therefore cannot validate the core's electrical implementation.

This strengthens the case for preserving first-ACK raster provenance without
changing our physical vector sequence. It does **not** settle silicon DCSR
semantics after the second shaped pulse. CPCEC also retires at most one DMA
source per CPU interrupt, but its simplified model is insufficient grounds to
bundle a change to the core's DMA auto-clear behavior.

The source exposes separate PRI differences relevant to Eerie Forest:
`cpcec.c:874–877` requests PRI on raw HSYNC assertion or line entry while HSYNC
is already active. Ordinary PRI therefore uses a different event from our
retained monitor-HSYNC trailing edge. At `2106–2114`, a changed PRI write sets
the raster request if the new value matches the current line during HSYNC;
otherwise a nonzero new value clears it, explicitly annotated for Eerie Forest.
That is not a ready-made fix for the remaining edge: our earlier
[pending-CPC discriminator](../investigations/hardware-runs/eerie-forest-pending-classic-2026-09-23.md#pending-request-discriminator-and-source-boundary)
found that AmSpirit masks and retains an old classic request across PRI=0→1→0
without ACK. CPCEC's shared raster bit and nonmatching-write clear do not
preserve that distinction. The core already separates classic and programmed
pending state. Neither CPCEC behavior should be copied on emulator authority
alone. See the
[ordinary-PRI source comparison](references/amspirit-pri-phase-2026-09-26.md)
for the unresolved phase disagreement and the distinction from ACCC.

No production RTL, tests, synthesis or hardware state changed during this
comparison. The recommended first repair remains the isolated DCSR change.

## Recommended repair and acceptance

The implemented candidate uses once-per-CPU-acknowledge raster provenance, retaining both shaped
pulses and their vector06→04 behavior. This is the smallest candidate supported
by the prior Copter handler analysis and the first-ack software workaround.
Do not bundle PRI, reset, DMA auto-clear or ordinary I/O changes. If hardware
shows this provenance candidate is wrong, revisit the contract instead of
forcing the title to agree with a self-derived assertion.

The existing cross-module handler-read and split-ACK tests now check the
preserved provenance; their physical vector assertions remain unchanged.
Existing distinct-M1, B19 simultaneous raster/DMA and snapshot-provenance cases
pass. Independent review and the full-effort, timing-checked RBF above complete
software/build acceptance; visual acceptance belongs to the user.

User acceptance should begin with Copter's moving logo, title palette boundary
and gameplay; then BMX music/timer for long enough to expose progressive drift,
including reset/reload from slow BMX and the subsequent return to menu. Recheck
FlowLIB's warning and the repaired CRTC3 scenes/freeze on the same build.
Prehistorik II follows those two simpler discriminators. Eerie's left strip
remains a separate open issue.

## Remaining information and continuation brief

- Execution of the candidate BMX IRQ handler and its interrupt cadence are not
  yet established. Correct pitch with slow event timing favors delayed software
  servicing but does not by itself exclude CPU stalls or excess interrupts.
- Actual CRTC R0/R2/R3 and PRI values during the affected game stages would
  determine whether the cross-line PRI addition can fire. Standard geometry
  must not be assumed from a normal-looking frame.
- To distinguish retained game RAM from ASIC timing state, a cold power-on
  versus warm reset/reload comparison is useful only if the narrow repair
  leaves the symptom. No such additional human check is needed before the
  first candidate build.
- Prehistorik II still needs a known-good-build gameplay check before attributing
  its corruption to this change set.
- A real Plus trace/readback of DCSR after the split acknowledge, especially
  with simultaneous DMA pending and both IVR auto-clear settings, would settle
  the remaining silicon semantics. Do not silently broaden the repair to DMA
  flag retirement without that evidence or an independently grounded failure.

Continue from `codex/plus/crtc3-demo`. Read this note, the B19 entry in
`docs/backlog.md`, and the archived Opus assessment under
`docs/references/crtc3-2026-09-25/regression-opus/`. User confirms CRTC3 fixed on
`e4445a1` but rejects that build for Copter/BMX regressions; `edaa15b` is the
known-good baseline. The first suspect change is `bd65578`, not the fifth DMA
repair. The isolated failing/passing proof above is available and the narrow production
repair is implemented. Keep visual testing with the user and avoid another
broad capture or emulator session. The reviewed, timing-checked RBF
and acceptance list are delivered above; do not call the game regressions
closed before the user checks them.
