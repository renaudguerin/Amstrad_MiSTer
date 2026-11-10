# Building the MiSTer core

The project targets the DE10-Nano with Quartus Prime Lite 17.0.2 and Cyclone V device support. Use `Amstrad.qpf` as the project file.

## GitHub Actions

The [build workflow](../.github/workflows/build.yml) runs simulation checks and produces a downloadable RBF, fitter summary and timing report when synthesis is selected. Pull requests run the build checks. A release's attached RBF is the easiest installation path.

## Local build

On a machine with Quartus 17.0.2 and Cyclone V support:

```sh
quartus_sh --flow compile Amstrad.qpf
```

The RBF appears at `output_files/Amstrad.rbf`. Check the fitter and TimeQuest reports before deploying a locally built bitstream. Copy it to `/media/fat/_Computer/` and ensure `boot.rom` is installed as described in the [README](../README.md).

On a host with Verilator 5+, GNU Make and a C++17 compiler, run `python3 sim/select_tests.py --run` for benches selected by a change, or `make -C sim` for all fast benches. See the [test guide](../sim/README.md).
