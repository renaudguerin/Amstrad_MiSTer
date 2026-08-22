# Amstrad simulation tests

This directory contains the aggregate Verilator regression suite. It currently
runs:

- Pin-level tests for `rtl/UM6845R.v`. The harness drives the 16 MHz `CLOCK`,
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
make -C sim soak        # (planned: see "Randomized equivalence soak" below)
```

## Randomized equivalence soak (planned for the type-split refactor)

Alongside the directed vectors, the suite gains a deterministic randomized
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
  That turns "bit-identical" from a hope into a checkable claim.
- It is not split-only scaffolding: F7 (RFD) requires proving bit-identity when
  never armed, and F10's staged interlace work benefits between stages. For
  behaviour-changing commits the same harness runs with documented deltas
  excluded from the hash window.
- Cost is one self-contained target (~150 lines reusing existing TestBench
  helpers), seconds of runtime, zero entanglement with the default suite — so
  removing it later, if ever judged not worth keeping, is trivial.

For future behaviour changes the rule stays as elsewhere in this repo:
deterministic vector first, derived from the cited ACCC rule, expectations on
paper — the soak complements that; it never replaces it.
