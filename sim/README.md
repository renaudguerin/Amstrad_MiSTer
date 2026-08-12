# Amstrad simulation tests

This directory contains the aggregate Verilator regression suite. It currently
runs:

- Pin-level tests for `rtl/UM6845R.v`. The harness drives the 16 MHz `CLOCK`,
  with `CLKEN` at character-clock phase 0 and `nCLKEN` at phase 8, and exercises
  the CRTC register bus directly.
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
```
