# Hardware retest reported 2026-09-13 — PSG R7 keyboard & joystick fix

This records the user's direct hardware observations on build `a0778b6`. It supersedes
earlier verdicts for the symptoms named below; source review, simulation, and other
title/subsystem acceptance remain separate.

## Build and configuration

- Commit: `a0778b6b67f2a76a351f2d50566611e71fbb851f` ("general: reset PSG R7 to 0x00 so bare-metal keyboard scans work").
- Tested hardware: MiSTer FPGA, 6128 Plus mode.
- Titles tested: `arn5diag`, `Pang`, `Plotting`.

## Root cause and fix mechanism

The GI AY-3-8910/8912/8913 datasheet specifies that `/RESET` clears all registers to 0.
Register 7 (I/O port direction and noise/tone enable) resetting to `0x00` configures
PSG Port A as an input port.

In the previous RTL (`rtl/YM2149.sv`), `RESET` had initialized `ymreg[7] <= '1` (`0xFF`),
configuring Port A as an output port. When software read register 14 (Port A) without first
writing register 7:
- The core returned `ymreg[14]` (initialized to `0x00`) ANDed with the keyboard matrix lines.
- This wedged all bits of every row read to `0x00`.
- Because CPC keyboard matrix and joystick inputs are active-low (0 = pressed), every key
  and joystick direction/fire button was read as permanently pressed.

Standard Locomotive BASIC firmware and SNA snapshot restoration routines explicitly write
R7 early in their boot sequence, which masked the bug during standard OS operation. However:
1. Bare-metal diagnostic cartridges such as `arn5diag` read keyboard matrix rows via R14
   before their sound tests ever program R7, causing keyboard navigation to fail completely
   from cold boot (all keys appeared pressed/conflicting).
2. Cartridge games such as `Pang` and `Plotting` read joystick/fire button status before or
   without programming R7 to input mode, causing the Fire button to be read as permanently held down.

Fixing `ymreg` reset to `{default:0}` in `rtl/YM2149.sv` ensures Port A defaults to input mode
on reset, matching hardware. Simulation regression `sim/plus/p10_input_test` was extended with
a reset-default R14 read vector, and the canonical soak hash `0xb1cb70da95c2e44f` remained identical.

## Reported hardware results

| Scope / symptom | User observation | Current verdict |
|---|---|---|
| `arn5diag` keyboard | Keyboard navigation works from cold boot | **Confirmed fixed on hardware.** Bare-metal R14 scans read the matrix correctly. |
| `Pang` input | Fire button is no longer permanently pressed; game responds to controls | **Confirmed fixed on hardware.** Closes the persistent Pang input defect. |
| `Plotting` input | Fire button is no longer permanently pressed; game responds to controls | **Confirmed fixed on hardware.** Closes the persistent Plotting input defect. |

## Impact on open tracking

- Closes the long-standing "Fire always pressed in Pang/Plotting" defect tracked across
  `backlog.md`, `current-status.md`, and `implementation-roadmap.md`.
- Closes the "Arnold 5 keyboard inoperable" defect.
- P10e / P10i input acceptance criteria are satisfied for these titles.
