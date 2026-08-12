# UM6845R simulation tests

This directory contains pin-level regression tests for `rtl/UM6845R.v`. The
harness drives the 16 MHz `CLOCK`, with `CLKEN` at character-clock phase 0 and
`nCLKEN` at phase 8, and exercises the CRTC register bus directly.

Requirements: Verilator 5 or later, GNU Make, and a C++17 compiler. On macOS:

```sh
brew install verilator
make -C sim
```

`make -C sim` builds and runs all tests noninteractively. Unexpected failures
exit nonzero, report the test and character/tick timestamp, and retain a VCD at
`sim/obj_dir/<test-name>.vcd`. Passing traces are removed.

Known RTL divergences are reported as `XFAIL` and do not fail the suite. An
`XPASS` does fail it: after an RTL fix, remove that test's expected-failure flag
so the newly correct behavior becomes a normal regression test.

Other useful commands:

```sh
make -C sim lint
make -C sim clean
```
