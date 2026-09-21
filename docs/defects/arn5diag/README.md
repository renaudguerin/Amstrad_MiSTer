# Defect: Arnold 5 Diagnostic (arn5diag)

- **Stream**: plus
- **Status**: keyboard defect fixed and hardware-confirmed on build `a0778b6` (2026-09-13); the cartridge remains a Plus diagnostic reference
- **Cartridge**: `arn5diag.cpr` (`local/test_media/cartridges/05_Other/arn5diag.cpr`)
- **Snapshot**: `local/test_media/defects/arn5diag/snapshot_20260913_171245_OS_PLUS_FR.sna`

---

## 1. Summary
Arnold 5 Diagnostic test ROM for Amstrad Plus / GX4000 hardware verification.
Used to validate ASIC register readback, PRI behavior, split-screen registers, and raster timing on real hardware vs FPGA.

## 2. Keyboard defect (fixed)
Keyboard navigation failed from cold boot. `arn5diag` scans the keyboard through PSG R14
before any sound code programs R7. The core reset R7 to `0xFF`, which set Port A to output, so
R14 read `0x00`; the matrix is active-low, so every key read as held. The AY-3-8912 resets all
registers to `0x00` (GI datasheet), and `rtl/YM2149.sv` now does the same. The same fix
cleared the held fire button in `Pang` and `Plotting`. Evidence:
[hardware retest 2026-09-13](../../investigations/hardware-runs/hardware-evidence-2026-09-13.md);
regression vector: reset-default R14 read in `sim/plus/p10_input_test`.
