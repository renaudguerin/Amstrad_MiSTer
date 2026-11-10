# Simulation tests

The repository has Verilator benches for the classic CRTC, Gate Array, Plus ASIC, motherboard and output path. Some slow production CPU and VHDL benches need GHDL or extra build time. Test names, covered paths and tiers are indexed in [TESTS.md](TESTS.md).

Requirements: Verilator 5+, GNU Make and a C++17 compiler. On macOS, `brew install verilator` provides Verilator.

```sh
python3 sim/select_tests.py --check  # validate the test index
python3 sim/select_tests.py --run    # run benches selected by changed files
make -C sim crtc-test               # classic CRTC pin suite
make -C sim                         # all fast benches
make -C sim lint                    # Verilator lint
make -C sim full                    # fast and slow benches, when needed
```

Behaviour changes require a deterministic test that fails before the fix and passes after it. Derive expected results from hardware or a cited primary source, not simulator output. The French Amstrad CPC CRTC Compendium is the written reference for classic CRTC rules; hardware observations outrank it.

The fixed seed randomized CRTC soak reports a rolling hash. The current expected hash is at the top of [soak-golden-history.md](soak-golden-history.md). A behaviour preserving refactor should reproduce it; an intentional behaviour change can update it with an explanation.
