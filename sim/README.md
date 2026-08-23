# Amstrad simulation tests

This directory contains the aggregate Verilator regression suite. It currently
runs:

- Pin-level tests for the classic CRTC core: wrapper `rtl/CRTC.v` plus its two per-type
  rule engines (`rtl/crtc_type0_engine.v`, `rtl/crtc_type1_engine.v`). The harness drives
  the 16 MHz `CLOCK`,
  with `CLKEN` at character-clock phase 0 and `nCLKEN` at phase 8, and exercises
  the CRTC register bus directly. Counter-state assertions are used only for
  Compendium cases whose documented C4/C9 result is not distinguishable from pins alone.
- Standalone Amstrad Plus ASIC lock/unlock sequence tests in `sim/plus`.

Requirements: Verilator 5 or later, GNU Make, and a C++17 compiler. On macOS:

```sh
brew install verilator
make -C sim
```

`make -C sim` builds and runs both suites noninteractively. Unexpected failures
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
  verification practice; the 87 vectors are the directed leg and only visit
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
- It is not split-only scaffolding: F7 (RFD) requires proving never-armed
  equivalence under this projection, and F10's staged interlace work benefits
  between stages. There is no mechanism to exclude documented behaviour
  deltas from a hash window. For a behaviour-changing commit, change the
  stimulus/sampled fields deliberately if needed and re-mint the golden
  hash, recording why it moved.
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
the type-0 partial-VSYNC holdoff latch, and the two type-1 private status
flops (`r6_border_condition`, `status_bit5_r`).

The golden hash for the type-0/type-1 engine split was minted from the
unsplit core; the minting commit and hash value are recorded in the session
plan (`docs/plans/2026-08-22-accc-review-plan.md`). It has been re-minted
twice on 2026-08-23: first for the intended F6 Stage 1 behaviour change (the
type-0 spurious border byte when R1>R0, protected by the t10 vectors), then
for the sampled-field expansion (holdoff latch + type-1 status flops added,
review issue 4 remediation — no RTL change). All hash values and reasons are
recorded there; the current golden value is **`0xf5f8ae01ffdf928d`**. The
hash depends on the seed, the
sampled field set/order, the event schedule, and the DUT's observable
behaviour — any of the first three changing requires re-minting, recorded as
such. The soak accesses internals by their Verilator names, so a refactor
that renames them updates those accessors without touching the hashed
values.

For future behaviour changes the rule stays as elsewhere in this repo:
deterministic vector first, derived from the cited ACCC rule, expectations on
paper — the soak complements that; it never replaces it.
