# Amstrad simulation tests

This directory contains the aggregate Verilator regression suite. It currently
runs:

- Pin-level tests for the classic CRTC core: wrapper `rtl/CRTC.v` plus its two per-type
  rule engines (`rtl/crtc_type0_engine.v`, `rtl/crtc_type1_engine.v`). The harness drives
  the 16 MHz `CLOCK`,
  with `CLKEN` at character-clock phase 0 and `nCLKEN` at phase 8, and exercises
  the CRTC register bus directly. Counter-state assertions are used only for
  Compendium cases whose documented C4/C9 result is not distinguishable from pins alone.
- Production GA/CRTC write-phase cases in `crtc_cpu_phase_test.cpp`, using the
  real GA, production clock divider, and scripted post-enable bus launches.
  Run `make -C sim crtc-cpu-phase-test`; this also runs in the default gate.
  [B8-1's contract and evidence](../docs/accuracy/b8-1-cpu-write-timing-2026-09-08.md)
  distinguish this coverage from executed T80 and hardware evidence.
- Standalone Amstrad Plus suites in `sim/plus`: ASIC lock/unlock, model select,
  Dandanator bounds, cartridge memory service, real-SDRAM cartridge client, CPR
  parser, MMU windows/boot integration, and (since Plus P1) the `asic_video`
  CRTC3 counter/timing foundation bench (`rtl/plus/asic_video.v`).

- The B8-6 colour boundary test uses the production top-level colour/CE selector,
  `color_mix`, and `gamma_corr`; run it alone with `make -C sim video-color-test`.
  It checks Plus pixel/metadata alignment and classic equivalence across monitor
  options, native/frame-selected pixel enables and gamma bypass/enabled modes.
  It excludes the vendor scandoubler/HQ2x pipeline and freeze mode. See
  [the boundary evidence](../docs/plus/archive/b8-6-colour-boundary-2026-09-08.md).
- The B6 video boundary gate uses the executing P10/B7 TV80 fixture with real
  motherboard, GA/ASIC and SDRAM: `make -C sim/plus b6-video-boundary`.
  `make -C sim video-output-test` exercises the extracted production output
  chain, including native Raw CRT cadence and a retained `video_freak` crop
  after VSYNC stops. The motherboard B6 fixtures are slow benches, run on
  demand; `b6-video-boundary` boots 6 cases and `b6-video-boundary-strict` all
  18. The colour test also checks
  the aligned raw vertical-blank mask. The production output chain supplies
  final RGB/DE/CE. `make -C sim/plus b6-dynamic` adds CPU-written malformed
  raster regimes and a Full/Raw pixel discriminator; `b6-plus-layers` scores
  nonzero SDRAM data, scroll, opaque sprites and blank/border priority through
  final RGB. `make -C sim video-mixer-rgb-test` pins the mixer scope correction
  across gamma/colour-depth variants. These are simulation gates, not physical
  output or an exhaustive stuck-sync matrix. See the
  [contract and evidence limits](../docs/investigations/video-boundary/b6-video-boundary.md).

Requirements: Verilator 5 or later, GNU Make, and a C++20-capable compiler
(for the timed SystemVerilog colour fixture; C++ harnesses still use C++17).
CI pins 5.052, matching the local version. The strict Plus lint recipes load
five exact legacy `SIMILARNAME` waivers only on 5.052 or newer, since 5.050
rejects that rule name in control files even with `-Wfuture`.
On macOS:

```sh
brew install verilator
make -C sim
```

Normal runs choose benches per change: `python3 sim/select_tests.py` prints the
benches the change set needs and why, and `--run` runs them. The rules and the
index of every bench live in [TESTS.md](TESTS.md); CI uses the same selection.
`make -C sim` builds and runs every fast bench in parallel; `make -C sim full`
adds the slow motherboard-scale Plus fixtures (`SLOW_TESTS` in
`sim/plus/Makefile`) and `make -C sim/plus slow` runs only them. Output from
parallel jobs interleaves on GNU Make 3.81; pass `JOBS=1` for serial output. Unexpected failures
exit nonzero. CRTC failures report the test and character/tick timestamp and
retain a VCD at `sim/obj_dir/<test-name>.vcd`; passing CRTC traces are removed.
The Plus suite reports its failing test group directly and does not generate
traces.

Known CRTC RTL divergences are reported as `XFAIL` and do not fail the suite. An
`XPASS` does fail it: after an RTL fix, remove that test's expected-failure flag
so the newly correct behavior becomes a normal regression test. The Plus suite
has no expected-failure cases.

Other useful commands:

```sh
make -C sim lint
make -C sim clean
make -C sim soak    # randomized equivalence soak, see below
```

## Randomized equivalence soak

Alongside the directed vectors, the suite ships a deterministic randomized
"soak-diff" target. Rationale, for reviewers:

- Directed + randomized stimulus against a golden reference is standard
  verification practice; the 225 registered classic vectors are the directed
  leg and only visit
  states an author imagined. The soak explores unimagined ones.
- Its oracle is self-referential by design: hash every pin plus key internal
  state (`hcc`, `line`, `row`, `c5`, `in_adj`, arbitration latches) each CLKEN
  over millions of characters of pseudo-random register writes at random C0
  values and bus phases, both CRTC types, fixed seed. Before a
  behaviour-preserving refactor (the type-0/type-1 engine split), the golden
  hash is minted from the unsplit core; the refactor must reproduce it exactly.
  That turns "equivalent on this stimulus and projection" from a hope into a
  checkable claim.
- Scope of that claim, stated plainly: equal hashes prove equivalence for
  THIS fixed stimulus, as seen through this sampled projection of pins and
  internals, at CLKEN sample points only. They do not establish general RTL
  bit identity. Invisible to the hash: combinational states and bus phases
  between sample points, the values returned by the randomized reads (the
   harness restores the idle bus before each sample), and any state outside
   the sampled field set — which is exactly how the development-time holdoff
   latch bug escaped the soak before the 2026-08-23 field expansion (the
   latch is now sampled) and was caught by the differential comparator
   instead. Stronger split evidence lives in the lockstep differential tool
  (`tools/split-differential`, branch `docs/split-differential-evidence`),
  which samples a broader state set after every CLKEN edge.
- It is not split-only scaffolding: F7 (RFD) and F10's staged interlace work
  benefit from the same fixed randomized projection. There is no mechanism
  to exclude documented behaviour deltas from a hash window. F7's random
  traffic legitimately reaches the R5-at-R0 trigger, so its soak hash moved;
  the **bit-identical-when-unarmed** claim is protected instead by directed
  `t13a`, which writes R5 nonzero away from C0=R0 and proves no RFD state is
  armed. For a behaviour-changing commit, re-mint the golden hash and record
  why it moved.
- Cost is one self-contained target (~150 lines reusing existing TestBench
  helpers), seconds of runtime, zero entanglement with the default suite — so
  removing it later, if ever judged not worth keeping, is trivial.

Usage and golden-hash protocol:

```sh
make -C sim soak                          # prints the current hash
make -C sim soak SOAK_EXPECT=<16 hex>     # exits nonzero on mismatch
```

The stimulus is fixed-seed (seed value in `sim/sim_main.cpp`, `kSoakSeed`):
pseudo-random register writes at arbitrary C0 values and CLKEN/nCLKEN bus
phases, both CRTC types, with occasional reads, held writes, snapshot loads,
live type round-trips, and resets. The rolling hash is FNV-style mixing of
each sampled field as a whole — XOR the field into the accumulator, multiply
by the FNV-1a prime (1099511628211) — rather than byte-wise FNV-1a over a
serialized stream. Samples are taken after every CLKEN edge: every pin (MA,
RA, DE, HSYNC, VSYNC, CURSOR, FIELD, DO) plus `hcc`, `line`, `row`, `c5`,
`in_adj`, the type-0 arbitration latches (including the R5 retarget value),
the type-0 partial-VSYNC holdoff and C0=2 line-history latches, and the two
type-1 private status flops (`r6_border_condition`, `status_bit5_r`).

The current golden hash and the reason for every re-mint since the original
2026-08-22 mint from the unsplit core are in
[soak-golden-history.md](soak-golden-history.md). For a behaviour-changing
commit, re-mint and add a row there. The hash depends on the seed, the
sampled field set/order, the event schedule, and the DUT's observable
behaviour — any of the first three changing requires re-minting, recorded as
such. The soak accesses internals by their Verilator names, so a refactor
that renames them updates those accessors without touching the hashed
values.

For future behaviour changes the rule stays as elsewhere in this repo:
deterministic vector first, derived from the cited ACCC rule, expectations on
paper — the soak complements that; it never replaces it.

## D5 production-input BASIC boot regression

`make -C sim/plus d5-boot GHDL=/path/to/ghdl` checks the actual top-level `/EXP`
configuration using translated production T80 and both unchanged private BASIC
CPRs. It requires firmware `Ready` on 6128 Plus and 464 Plus, rejects disc-missing
output/timeouts, and runs ROM7/direct-page/GX4000 controls. It is opt-in because
the cartridges are user-owned and ignored. See the
[D5 report](../docs/plus/d5-basic-boot-input-2026-09-11.md) for input hashes,
fixture limits and results.

## Production T80 executed-instruction validation (B8-1)

Complementing the scripted CPU bus phase harness (`crtc_cpu_phase_test.cpp`),
the optional production-T80 suite executes real Z80 instructions through the
production `T80pa.vhd` core synthesized to Verilog, connected to the real
`ga40010` clock divider/enables/WAIT generator and `CRTC.v`.

Available make targets:

```sh
# 1. Synthesize production T80pa VHDL to Verilog via GHDL
make -C sim t80-netlist

# 2. Verify bounded defined-field trace equivalence between native VHDL and translated Verilog
make -C sim t80-trace-test

# 3. Build and run executed-instruction tests against current B8-1 CRTC engines
make -C sim crtc-t80-test

# 4. Build and run discriminator tests against old pre-B8 engines (verifying all 4 B8 failures)
make -C sim crtc-t80-old-test
```

Requirements: GHDL with LLVM or GCC backend for standalone executable targets (verified with GHDL 6.0.0 LLVM on macOS aarch64; mcode backend does not support executable generation; default `ghdl`, overridable via `GHDL=...`), Verilator 5+, C++17 compiler.

The executed-instruction suite asserts:
1. First system-edge storage capture: the CPU launches write cycles on `CEN_p`,
   and CRTC register values update on the immediate next system clock tick (`tick+1`),
   with positive margin verified before later character decisions (`CLKEN`).
2. Source-derived instruction timing windows: `OUT (C), C` critical I/O write executes
   in the 3rd microsecond ([128, 192) master ticks from opcode fetch start), whereas
   `OUTI` executes in the 5th microsecond ([256, 320) master ticks), strictly enforcing
   French ACCC v1.11 §13.7.1 p.126. Measured intervals (152 ticks = 2.375 µs for OUT;
   264 ticks = 4.125 µs for OUTI) are simulation observations within these documentary windows.
3. Exact seam behavior and counter progression: Type 1 R5 RFD (MA reload to `0x1234`),
   Type 0 R0 widening (C0 advances 1->2 without duplication, row increments to R4+1,
   C9 retained, enters adjustment), and Type 1 R0 widening (C0 advances past 15 to 16,
   line extended, `pending=1`). OUTI tests materially distinguish memory data from
   register C (`C=0x02` writing `RAM[0x40]=0x01` to port `0xBD02`).
4. Engine-isolated discrimination: all 4 cases independently fail when pre-B8 engines
   (`d46609d066aafb6b182fd6fa504a91719500cd91`) are paired with the current `CRTC.v`
   wrapper, because pre-B8 engines evaluated write qualification only at CLKEN, at which
   point the register was already overwritten with its new value.

See [B8 production T80 evidence](../docs/accuracy/b8-production-t80-2026-09-08.md) for full analysis.
