# Plus / GX4000 hardware diagnostics

Cartridges that ask original hardware a question the written sources answer differently,
or not at all. Each screen states what the current RTL predicts; photographs decide. Record
results in [the divergence ledger](../../docs/plus/source-divergences.md).

## Multi-test probe cartridge (`plus_hw_probes.py`)

```sh
python3 scripts/diagnostics/plus_hw_probes.py        # needs sjasmplus on PATH
```

Writes `output_files/plus-hw-probes/plus-hw-probes.cpr` (plus `.lst`, `.sym`, `.json`;
all ignored). The program is `plus_hw_probes.asm`; the script generates its include file
(test table, screen text, phase-band table, font). Cold boot shows a title screen listing
every test. **Any key or joystick fire advances to the next screen**, wrapping back to the
title; every screen starts from a clean machine state. Each screen names itself
(`NN/20`, test id, settings), prints the RTL prediction, and interrupt tests print the
counted test interrupts per frame (`IRQ/FRAME=nn`). Photograph each screen whole,
including the text rows; note the machine model.

Screen content uses pen 0 red, pen 1 green, border blue, sprites white. The "grid" is
the green RA2 raster of every character row (rows 0–19). Markers are either the green
RA2 plane shown on RA0 by an SSCR write (A), or pen 0 turned yellow for about 9 µs (B, C).

### Screens

"RTL" is the production simulation at the commit that built the CPR (see below);
"AmSpirit" is Lite 1.15.1, 6128Plus/CRTC3, on the same CPR. "Original Plus" is the
2026-09-27 photograph set (cartridge V1, screens 01–18) recorded in [the ledger](../../docs/plus/source-divergences.md#probe-photographs).
Screens 19–20 were added in V2; the V1 numbering is unchanged.

| # | Test | Question | RTL | AmSpirit | Original Plus |
|---|---|---|---|---|---|
| 01 | A1 PRI width 1 | Does a 1-character HSYNC still request? | no marker, `00` | marker, `01` | `01` (as AmSpirit) |
| 02 | A2 PRI width 2 | Width 2 at the adopted phase | marker ends 139 dots in, `01` | same | `01` |
| 03 | A3 PRI width 3 | Framework cross-check with `pri-width-3.cpr` (hardware ~136) | 139, `01` | same | ~140, `01` |
| 04 | B PRI write phase | Does writing PRI := current line fire, by write C0 (45–63, HSYNC 49–59)? | marks for C0 45–59, the mark moving right from 49; none for 60–63; `15` | also fires for C0 60; `16` | 45–60, `16` (as AmSpirit) |
| 05 | C1 R2=49, R3=8 | Reference, no crossing | `01` | same | `01` |
| 06–08 | C2–C4 R2=57/58/62 | HSYNC entering the PRI line: line-entry request plus ordinary | `02` | same | `02` |
| 09 | C5 R2=63 | Ordinary +1 µs lands on the next line's entry | `01` | `02` | `02` (as AmSpirit) |
| 10–13 | D1–D4 SPLT=54–57 | Split near the 312-line wrap (SPLT=55 also matches line 311) | green from line SPLT+1 in all four | SPLT=55: whole screen green | SPLT=55 as AmSpirit (line 0 red); others as RTL |
| 14 | E1 R9=11, vscroll 0 | Reference | rows 00–15 in order | same | as RTL |
| 15 | E2 R9=11, vscroll 5 | Low-three-bit vs wider RA addition | every row shows ROW 00 | rows 00, 03, 06… (a third pattern) | as AmSpirit |
| 16 | F sprite mirrors | Which offsets write magnification (+3, +5, +6, +7)? | +5/+6/+7 big, +3 small | same | as RTL |
| 17 | G1 sprite left edge | X=-64/-63 at x4, X=-16/-15 at x1 | 1-dot column for -63 and -15 only | same | as RTL |
| 18 | G2 SSCR[7] over sprites | Does the extended border hide sprites? | X=0 hidden, X=8 right half, X=16 whole | same | as RTL |
| 19 | C6 R2=56, R3=8 | Raw HSYNC ending exactly at the PRI line start: line-entry request? | `02` | `02` | not yet photographed |
| 20 | C7 R2=50, R3=14 | Same, at the CRTC3 demo's plasma/sphere/Wolverine timing | `02` | `02` | not yet photographed |

Why each matters and what each outcome would change: [source-divergences.md](../../docs/plus/source-divergences.md).

### Mechanism notes

- **A** keeps the 2026-09-26 flat-plane handler byte for byte at `0038` (EXX, 4 NOPs,
  SSCR←AC, 8 NOPs, SSCR←8C), so A3 is comparable with the photographed
  `pri-width-3.cpr`. An arming interrupt on line 2 (R3=11) sets PRI=7, switches R3 to the
  tested width for lines 3–10 only, and waits in a NOP window; a width that never
  requests must not also silence the arming interrupt. While arming, the first three
  bytes at `0038` are overlaid with `JP` to the arming handler.
- **B** arms on line T-2 of each band, enters a NOP sled calibrated in simulation so the
  `LD (&6800),A` write lands at C0 = 45 + band on line T, then opens a 150 µs window.
  Calibration constant `SLED_BASE`; re-derive it if handler code before the sled changes.
- **C** changes R2 for the whole frame. With R2≥57 the HSYNC blanks the first C0 of each
  line, so text starts at byte column 16. The monitor may shift the picture; the count is
  the primary result.
- **D** fills RAM bank `0000` green and points SSA there (`&6802/3` = 0); the red `C000` bank
  holds the R12/R13 display. Labels are drawn in both banks.
- The NOP windows accept interrupts on the same 1 µs M1 boundary as `HALT`.

### Simulation predictions

```sh
python3 scripts/diagnostics/plus_hw_probes_sim.py          # all screens, ~2 min
python3 scripts/diagnostics/plus_hw_probes_sim.py --no-build --screens 4
```

Builds the D5 production-T80 fixture (`sim/plus/prepare_d5_boot.py`, GHDL T80 netlist)
with four extra probe ports, then runs one CPR per start screen (`--start N`). Each run
writes `output_files/plus-hw-probes/sim/NN.ppm` in C0 geometry (x = C0·16 + dot,
y = scanline) and `NN-events.txt` with PRI/SPLT/SSCR/pen-0 writes, raster requests and
acknowledges of the last frame, each with line and C0. Uses Homebrew LLVM by default
(`--cxx`), as the rest of the Verilator suite does on macOS.

## PRI alias experiment (`pri_alias_probe.py`)

Build with Python 3 (standard library only):

```sh
python3 scripts/diagnostics/pri_alias_probe.py
```

Load `output_files/pri-alias-probe/pri-alias-probe.cpr` as a cartridge on a real
Amstrad Plus or GX4000 and cold boot/reset. No keyboard or controller is needed.
After about three seconds the screen says `DONE - RESULTS READY`. Photograph the
whole screen and record the machine model and cartridge/loading method. The
`.bin`, annotated address/byte `.lst`, and label/address `.json` are generated
beside the CPR for inspection; all generated artifacts are ignored by Git.

The four displayed values are **16-bit hexadecimal interrupt counts**, each over
32 complete VSYNC intervals. PRI and R6 labels are decimal.

| PRI | R6 | Purpose |
| --- | --- | --- |
| 100 | 25 | Calibration: one IRQ per frame would give `0020` (32). |
| 10 | 25 | Does an alias at line 266 fire outside displayed rows? |
| 10 | 34 | Line 266 is now inside displayed rows. |
| 255 | 25 | Lower-half, non-displayed control: does PRI fire during vertical blank? |

For either PRI=10 case, one match per frame gives `0020`; an additional match at
266 gives `0040`. Comparing the two R6 settings tests whether any alias depends
on vertical display enable. PRI=255 would give `0020` if it operates outside
displayed rows, or `0000` if suppressed there. These are hypotheses, not claims
about the correct hardware result. Unexpected counts are useful evidence; the
probe does not translate them into a verdict. An emulator run only validates
that the cartridge executes under that emulator's model.

### Measurement protocol

The cold-boot entry copies the 16 KiB ROM bank into RAM at `8000`, jumps there,
disables both ROM mappings, uses a stack below `BFF0`, and installs an IM1 handler
at RAM `0038`. The handler preserves AF/HL, increments a 16-bit RAM count, and
returns with EI/RETI. Interrupt acknowledge itself clears the raster request;
the ISR does not write PRI or any ASIC register. DMA is disabled and its pending
flags cleared. Sprite magnifications are cleared with direct writes, without
relying on ASIC register readback; SPLT, SSA and SSCR are cleared. IVR is 1.
The PPI is explicitly initialized with port B as input for VSYNC polling.

CRTC registers are R0=63, R1=40, R2=49, R3=0E, R4=38, R5=0, R7=30,
R8=0, R9=7, R12=30h, R13=0. This requests 312 lines and VSYNC beginning at
line 240, with a 16-line VSYNC width and no HSYNC overlap across line boundaries.
Each case writes PRI and R6 with interrupts disabled, waits for two VSYNC rising
edges, briefly executes EI/NOP/DI to acknowledge any pending PRI through IM1,
then zeros the count and resets classic interrupt state through MRER. The PRI
drain does not depend on whether MRER clears a pending PRI on real hardware.
PRI is nonzero after ASIC setup and throughout measurement, suppressing classic
periodic interrupt delivery. After enabling interrupts, the code waits for 32
further rising edges, then disables interrupts and stores the count. No PRI
write occurs inside the count window. Polling and setup add a small fixed phase
offset from VSYNC rise; all candidate IRQs are separated from that boundary
by many scanlines, including any monitor-VSYNC shaping delay. In particular, PRI=255 is deliberately not measured against VSYNC's
falling edge at line 256.

R6=34 is used only for its test; the next case and the final screen use R6=25.
The results are drawn after all measurements with interrupts disabled. A normal
run takes approximately 2.8 seconds plus initialization. There is no timeout:
missing VSYNC leaves the `RUNNING` screen and current status byte in place.

### RAM inspection

These addresses are ordinary RAM, outside the paged ASIC area:

| Address | Meaning |
| --- | --- |
| `B000` | Current unsigned 16-bit IRQ count, little endian. |
| `B002` | Status: 0 during setup, 1–4 for the corresponding running case, `80` hex when all results have been drawn. |
| `B010` | PRI=100, R6=25 result, unsigned 16-bit little endian. |
| `B012` | PRI=10, R6=25 result. |
| `B014` | PRI=10, R6=34 result. |
| `B016` | PRI=255, R6=25 result. |

**Original-Plus result (2026-09-27):** all four cases read `0020`, matching the nine-bit
no-alias comparison, AmSpirit and CPCEC. Photograph:
`local/task-archives/crtc3-2026-09-27/output_files/pri-alias-probe/real-pri-alias-probe.jpeg`
(main checkout, ignored).

There is no hardware-error detection or result interpretation. An invalid
calibration or incomplete status should be reported alongside the counts.
