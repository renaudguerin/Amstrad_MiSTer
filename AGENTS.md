# Repository guidance

This is an experimental MiSTer Amstrad CPC/Plus core. The FPGA project is `Amstrad.qpf`; RTL is in `rtl/` and simulation benches are in `sim/`. Keep classic CRTC and Plus ASIC behaviour changes separable for regression and bisection.

Real hardware is the final authority for behaviour. For classic CRTC rules, consult the latest French Amstrad CPC CRTC Compendium and cite its section and page in source derived tests or implementation comments. The English translation is a navigation aid. Keep the Compendium PDFs untracked; they are not licensed for redistribution here. CRTC modules derived from the Compendium need its attribution in source headers; see `rtl/CRTC.v` and `CREDITS.md`.

For behaviour changes, create a deterministic test that fails before the fix and passes after it. Index new benches in `sim/TESTS.md`. Run `python3 sim/select_tests.py --check` and `python3 sim/select_tests.py --run` before integrating code or test changes; pure documentation edits do not need simulation. Use `make -C sim crtc-test` for focused classic CRTC work. Review synthesis timing and test important changes on hardware.

If `local/dev/AGENTS.md` is installed, read it for the maintainer's additional private development context. The public checkout must remain usable without it.
