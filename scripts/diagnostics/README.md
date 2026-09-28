# Plus / GX4000 hardware diagnostics

Cartridges that ask original hardware a question the written sources answer differently,
or not at all. Each screen states a recorded prediction; its evidence note identifies the RTL
revision. Photographs decide. Record
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
(`NN/35`, test id, settings), prints the RTL prediction, and interrupt tests print the
counted test interrupts per frame (`IRQ/FRAME=nn`). Photograph each screen whole,
including the text rows; note the machine model.

Screen content uses pen 0 red, pen 1 green, border blue, sprites white. The "grid" is
the green RA2 raster of every character row (rows 0–19). Markers are either the green
RA2 plane shown on RA0 by an SSCR write (A), or pen 0 turned yellow for about 9 µs (B, C).

### Screens

"RTL" in the historical table records the pre-fix production simulations.
V4 screens26–30 were captured at `77ebf51`; PA1–PA4 have since changed.
The V5 PA7 follow-up below records current predictions separately.
"AmSpirit" is Lite 1.15.1, 6128Plus/CRTC3, on the same CPR. "Original Plus" is the
2026-09-27 photograph set (cartridge V1 screens 01–18, V2 screens 19–20, V3 screens 21–25) recorded in [the ledger](../../docs/plus/source-divergences.md#probe-photographs).
Screens 19–20 were added in V2 (photographed the same day), 21–25 in V3, and 26–30 in V4. Earlier numbering is unchanged. V4 photographs from a 6128 Plus are recorded in the [V4 hardware record](../../docs/plus/asic-audit-probes-v4.md#original-6128-plus-photographs-2026-09-28).

| # | Test | Question | RTL | AmSpirit | Original Plus |
|---|---|---|---|---|---|
| 01 | A1 PRI width 1 | Does a 1-character HSYNC still request? | marker, `01` | marker, `01` | `01` (as AmSpirit) |
| 02 | A2 PRI width 2 | Width 2 at the adopted phase | marker ends 139 dots in, `01` | same | `01` |
| 03 | A3 PRI width 3 | Framework cross-check with `pri-width-3.cpr` (hardware ~136) | 139, `01` | same | ~140, `01` |
| 04 | B PRI write phase | Does writing PRI := current line fire, by write C0 (45–63, HSYNC 49–59)? | marks for C0 45–60, the mark moving right from 49; none for 61–63; `16` | also fires for C0 60; `16` | 45–60, `16` (as AmSpirit) |
| 05 | C1 R2=49, R3=8 | Reference, no crossing | `01` | same | `01` |
| 06–08 | C2–C4 R2=57/58/62 | HSYNC entering the PRI line: line-entry request plus ordinary | `02` | same | `02` |
| 09 | C5 R2=63 | Ordinary +1 µs lands on the next line's entry | `02` | `02` | `02` (as AmSpirit) |
| 10–13 | D1–D4 SPLT=54–57 | Split near the 312-line wrap (SPLT=55 also matches line 311) | green from line SPLT+1; SPLT=55 also line 0 red, green from line 1 | SPLT=55: whole screen green | SPLT=55 as AmSpirit (line 0 red); others as RTL |
| 14 | E1 R9=11, vscroll 0 | Reference | rows 00–15 in order | same | as RTL |
| 15 | E2 R9=11, vscroll 5 | Low-three-bit vs wider RA addition | rows 00, 03, 06… | rows 00, 03, 06… (a third pattern) | as AmSpirit |
| 16 | F sprite mirrors | Which offsets write magnification (+3, +5, +6, +7)? | +5/+6/+7 big, +3 small | same | as RTL |
| 17 | G1 sprite left edge | X=-64/-63 at x4, X=-16/-15 at x1 | 1-dot column for -63 and -15 only | same | as RTL |
| 18 | G2 SSCR[7] over sprites | Does the extended border hide sprites? | X=0 hidden, X=8 right half, X=16 whole | same | as RTL |
| 19 | C6 R2=56, R3=8 | Raw HSYNC ending exactly at the PRI line start: line-entry request? | `02` | `02` | `02` |
| 20 | C7 R2=50, R3=14 | Same, at the CRTC3 demo's plasma/sphere/Wolverine timing | `02` | `02` | `02` |
| 21 | H1 SPLT=55, SSA rewritten on line 311 at C0≈52 | Is SSA sampled for the line-311 split before the C0≈52 write (RTL: C0=R1=40) or after it (line end or frame origin)? | `SSA ROW 05` (terminal line samples at C0=R0; `ROW 04` before the fix) | `SSA ROW 05` | `SSA ROW 05`, dash beside line 0 (as AmSpirit) |
| 22 | H2 as H1, rewrite at C0≈30 | Control: the rewrite precedes C0=R1 | `SSA ROW 05` | same | `SSA ROW 05`, dash above the display |
| 23 | D5 SPLT=55, vscroll 7 | Does the line-0 row capture (offset 7 makes raster 0 the row's last) replace the held line-311 split? | red to line 55, green from 56 | same | as RTL |
| 24 | E3 R9=3 rows 6–11, vscroll 0 | Reference for E4 | bars step once per 4-line row | same | as RTL |
| 25 | E4 R9=3 rows 6–11, vscroll 2 | R9<7 with an offset: does `ra_eff >= R9` capture on several lines of a row? | bars step on 3 lines of every 4-line row | same | as RTL |
| 26 | PA1 real `IN A/B/D/E/H/L,(C)` | Opcode, opcode OR1, or fixed model byte? | CPU `FF` each; GRB `066/666/006/066/666/0F6`; controls `066/F66` | pending | CPU78/40/50/58/60/68, repeat78; palette and controls as RTL |
| 27 | PA2 adjustment captures, R5=16 | Which adjustment indexes can capture SSA? | Lines296–311 red; timed writes verified | pending | Green297 and305–311: tested indexes0/1/8 capture |
| 28 | PA3 terminal split with R5=16 | Immediate, carried, or deferred terminal SSA? | Adjustment296–311 green; frame0–7 red | All red; early-arm variant has green adjustment/red frame0–7 | All red; phase dash not discernible; mechanism unresolved |
| 29 | PA4 unmapped page reads | Last operand/opcode, FF, or underlying RAM? | `FF FF FF FF`; controls `A5 5A 0B 5A` | pending | 50/68/7E/7E; all controls pass |
| 30 | PA7 compatible interrupt phase | Extra delay relative to raw HSYNC? | Marker right edges x68/x180, difference112 dots; count02 | pending | Approximately128-dot gap, count02; software-visible slot |

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
- **D5** is D2 with SSCR=&70: every row's raster 0 displays as raster 7, so the row capture
  fires on frame line 0 at C0=R1. Green from line 1, as on D2, would mean the held line-311
  split survives it.
- **H** puts SSA on bank-0 row 4 (labels `SSA ROW nn`, green bank) with SPLT=55, and a
  raster interrupt on line 255 enters a long calibrated delay (`SSA_COARSE`, `SSA_SLED`)
  that writes the row-5 SSA on line 311, restored on line 2. The first green text row
  shows which SSA the line-311 split captured. H1 and H2 bracket the sampling point between
  C0≈30 and C0≈52; they do not pin it to C0=R1 exactly. A cyan border dash starting ~3 µs after
  the write proves its position: H2 shows it on line 311 above the display's right half,
  H1 in the left border beside frame line 0 (its C0 60–63 follow HSYNC).
- **E3/E4** keep rows 0–5 as 8-line text rows; raster interrupts at PRI 48 and 96 (PRI
  numbers `{VC, RC2..0}`) set R9=3 for rows 6–11 and 7 again from row 12, keeping 312
  lines. Source row k (6–24) holds a green bar 3(k−3) bytes long on every raster, so bar
  length names each displayed line's source row. Equality capture would step once per
  row on E4 too.
- The NOP windows accept interrupts on the same 1 µs M1 boundary as `HALT`.


### V5 PA7 follow-up: screens 30–35

The [reviewed design, predictions and evidence](../../docs/plus/pa7-interrupt-phase-followup.md)
separates request timing from CPU acceptance. V5 adds31–35 while preserving
the existing experiments. Original-Plus results are pending.

```sh
python3 scripts/diagnostics/plus_hw_probes.py --output-dir output_files/plus-hw-probes/pa7-followup/final-sim --start 30
```

Hardware handoff: [V5 PA7 CPR](../../output_files/plus-hw-probes/pa7-followup/plus-hw-probes-v5-pa7.cpr).
Photograph30, then advance through31–35, waiting three seconds each time.
Include all four yellow markers, rulers and IRQ/FRAME digits;31 also has
four magenta references. Screen30 needs count02;31–35 need04.

| Screen | Control | Current RTL | AmSpirit | Original Plus |
|---|---|---|---|---|
| 30 | W8 / NOP reference | 112-dot gap | 128 | V4: approximately128 |
| 31 G | Requests pending under DI | Normalized G=0 | G=0 | pending |
| 32 W | HSYNC width12 / NOP | 176-dot mean gap | 192 | pending |
| 33 L | LD A,(HL), complementary passes | 112-dot mean gap | 128 | pending |
| 34 R | RET NC, carry set, never taken | 112-dot mean gap | 144 | pending |
| 35 I | INC HL, complementary passes | 112-dot mean gap | 128 | pending |

G subtracts each magenta right edge from its yellow right edge, then compares
compatible with reference. Other new screens average the A/B yellow-edge
gaps. The different RET NC result is conditional emulator evidence, not an
exact raw-IRQ delay or a hardware finding. No PA7 RTL change is included.

### V4 audit screens: historical photograph protocol

The [reviewed pre-build design](../../docs/plus/asic-audit-probes-v4.md) separates
hardware hypotheses from the production-model observations below. There is no
RTL change. The five original-6128-Plus photographs are recorded separately from these
predeclared predictions; discrepancies and unresolved mechanisms are listed there. Record the Plus model, loading method and capture device.

- **26 PA1:** photograph every CPU byte and three-digit **GRB** palette word.
  `IN A` at port7F54 repeats KT's original port low byte. OUT78/79 controls should
  read066/F66. Each IN starts from blue `00F`; that result means no observed
  palette write for that row. CPU return and the GA write are separate observations.
- **27 PA2:** photograph the 16-line adjustment strip just before the frame
  origin, with the cyan phase dash in the adjacent border. All red; green from297;
  green297 only; and green297 plus305–311 are the four observable classes.
  Green297 only also fits unrestricted eligibility without the unprobed RC3
  alias. A one-line green stripe must remain resolvable in the capture.
- **28 PA3:** use identical geometry to27. Adjustment green/frame1–7 red means
  restart reload; adjustment green/frame1–7 green means carry; adjustment
  red/frame1–7 green means deferred application. Frame0 is red in those cases.
  All red is ambiguous. The red or green strip must contrast with the blue border;
  green is not a prerequisite for visibility. The unusual R7 position deliberately
  brings adjustment and the frame origin onto the visible monitor area.
- **29 PA4:** photograph all four reads and four controls. RAM sentinels are
  written with the ASIC page unmapped, then read before remapping. The sprite
  and palette controls establish supported mapped readback.
- **30 PA7:** compare the right edges of the two yellow pen0 markers. The upper
  marker is the programmed PRI reference; the lower is the compatible interrupt.
  Both use the same ISR and A13=1 NOP window. Rulers immediately below the
  markers have8-dot ticks, with longer ticks every16 dots. Difference112 versus128 mode2 dots
  distinguishes the stated acceptance-slot hypotheses. This does not distinguish
  a common Plus HSYNC shift from no extra delay, nor measure raw IRQ time below
  the CPU acceptance resolution. `IRQ/FRAME=02` is the two-marker control.

PA2/PA3 show explanatory text again where video-address wrapping repeats it;
those text regions are outside the reserved flat sample spans. The top cyan dash
is at raw line295 after HSYNC; it appears near the monitor-wrapped left border.
The later diagnostic border write is inside active display and is not itself a
second visible mark. Judge visibility from the blue border, top dash and the
identified strip, not from that hidden write.

Generated hardware handoff: [title-first V4 CPR](../../output_files/plus-hw-probes/v4/plus-hw-probes.cpr)
and [start-at-26 V4 CPR](../../output_files/plus-hw-probes/v4/plus-hw-probes-start26.cpr).
The design document records their SHA256 hashes and the complete simulation evidence.

The archived V4 start-at-26 build advances26→27→28→29→30→title.
The current generator builds V5 and continues through35 instead; do not
regenerate it into the archived `v4/` directory. Allow three seconds after
each change for setup and monitor settling before photographing.

### Simulation predictions

```sh
python3 scripts/diagnostics/plus_hw_probes_sim.py          # all screens, ~2 min
python3 scripts/diagnostics/plus_hw_probes_sim.py --no-build --screens 4
```

Builds the D5 production-T80 fixture (`sim/plus/prepare_d5_boot.py`, GHDL T80 netlist)
with passive diagnostic probe ports, then runs one CPR per start screen (`--start N`). Each run
writes `output_files/plus-hw-probes/sim/NN.ppm` in C0 geometry (x = C0·16 + dot,
y = scanline). Images and `NN-events.txt` cover the same complete CRTC-origin
frame, identified in the event header. Events include raw HSYNC rises/falls,
actual motherboard INT and ACK, split captures, line-start MA/stored MA, ASIC
and legacy-GA writes, and monotonic master-clock timestamps. `NN-acquisition.txt`
records selected CPU read samples across startup, `NN-results.txt` dumps the
ordinary-RAM observation buffer, and `NN-navigation.txt` records screen changes.
The wrapper exits unsuccessfully if any screen run fails. Uses Homebrew LLVM by default
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
