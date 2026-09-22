# Plus System Cartridge on GX4000 (2026-09-22)

## Conclusion

The core's GX4000 behaviour with the Plus System Cartridge (v4 banner, then BASIC `Ready`,
no f1/f2 menu) is what the cartridge code produces under the documented GX4000 ROM mapping.
No RTL input is wrong on this path, so no vector or fix follows. The previous checklist
expectation of an "insert-cartridge splash" on GX4000 had no source and has been replaced.

## Evidence

Cartridge: `local/test_media/cartridges/06_System/Plus_EN.cpr`, SHA-256
`3ce35dfccf79ee6bf8f990124aa4e0af1ce9753cbca03af8545abef21cf081ae`. Pages extracted from the
RIFF `cbNN` chunks and disassembled with `z80dasm` 1.2.0 (Homebrew). Device observation:
`docs/investigations/hardware-runs/device-acceptance-cdcb3c3-2026-09-22.md` on branch
`general/device-acceptance-cdcb3c3`, section "Plus System Cartridge on three models".

Start-up path, cartridge page 0 (firmware v4, runs at `&0000`):

- `&0000`: `LD BC,&7F89 / OUT (C),C / JP &0591`.
- `&0591`: standard 6128 hardware init. The only input read is PPI port B bit 4 (`&F5xx`,
  50/60 Hz link) to choose the CRTC table. No ASIC ADC (`&6808-&680F`) read, no keyboard scan.
- `&05E5`: `LD DE,&0679` (banner routine), `LD HL,&C072`, `LD C,7`, then the firmware reset
  path at `&0621`, which ends at `&0077` and far-calls ROM 7 address `&C072`. The banner
  routine at `&0679` reads PPI port B bits 1-3 only to pick the vendor name string at `&0720`.

So the firmware makes no model decision. It always enters logical upper ROM 7 at `&C072`.

Cartridge page 3 (AMSDOS, ROM 7 on 464+/6128+):

- `&C072` is an unnamed jump-table slot (after the RSX name table ends): `JP &CDD7`.
- `&CDD7` prints the menu ("f1 Amstrad BASIC", "f2 Burnin' Rubber") and polls `KM READ KEY`.
  f1 starts BASIC via `MC START PROGRAM`; f2, or a timeout of `&2328` = 9000 KL ticks
  (30 s at 300 Hz, `KL TIME PLEASE` compared at `&CDFB`), far-calls Burnin' Rubber at cartridge
  page 4 (`&CE5D`, ROM `&84`).

Cartridge page 1 (BASIC) at `&C072` is the middle of `LD A,(&AD90)`: the CPU executes
`SUB B / XOR L / SUB 2 / JR NZ,&C081`, and `&C081` prints the string at `&C0D7` ("Ready")
and enters the BASIC input loop without BASIC's normal `&C006` initialisation.

ROM mapping rule. Arnold V Specs Revised (CPCWiki, archived 2025-01-02,
`web.archive.org/web/20250102091704/https://www.cpcwiki.eu/index.php/Arnold_V_Specs_Revised`),
section 2.8: "On an unmodified GX4000, selecting logical page 7 using DFxx doesn't select
physical page 3, instead it selects physical page 1." The original Arnold V issue 1.5 (local
copy in `docs/specs/plus/`) has no GX4000 clause; the revised text is the more specific
source. `rtl/plus/plus_mmu.v` implements exactly this (`gx4000` forces page 1 for values
below 128), as does `docs/plus/references/asic-reference.md` sections 11 and 12.

## Predicted real-hardware screen

On an unmodified GX4000 the far call reaches BASIC page 1 instead of the menu: v4 banner,
then `Ready` and a BASIC cursor. The menu, and its 30 s auto-start of Burnin' Rubber, are
unreachable. This is an inference from the ROM code plus the revised-spec mapping; no
photograph of a real GX4000 running this cartridge has been found.

The extra `14592` line is not determined by ROM code alone. BASIC is entered uninitialised,
so what it prints next depends on entry registers (`B`, `L` choose the `JR NZ` at `&C076`) and
on the contents of the BASIC workspace around `&AC00`, which the firmware reset does not
clear (it zeroes only `&B100-&B8F9`). Treat that line as state-dependent, not as a defect.

Even `Ready` itself is state-dependent. BASIC's character output (`&C3C1`) routes by the
current-stream byte at `&AC06`: 0-7 go to a screen window, 8 to the printer, higher values
elsewhere. Uninitialised BASIC leaves that byte at whatever RAM held at power-on. A core
whose RAM starts at zero gets stream 0 and shows `Ready`; a real machine with random power-on
RAM may send it to the printer or nowhere, and keyboard input uses the same kind of state.

A MiSTer forum recollection of real GX4000s matches this: a Locomotive banner, usually no
cursor or `Ready`, no f1/f2 menu, sometimes a few random numbers, and firmware calls that
"get a bit random". The same thread attributes the difference to configuration resistors
(recalled as R125-R127) that the GX4000 board lacks. That fits the revised spec's "the
appropriate hardware is not activated" for ROM 7, and the original spec's statement that RAM
size and disc presence are separately configurable. A suggestion in that thread that the menu
code probes the FDC and hides itself is not supported: nothing on the path above touches
the FDC, and on GX4000 the menu page is never mapped at all.

## Residuals

- AmSpirit with `cpc_model 5` showed only the banner. That model number is not confirmed as
  GX4000, and AmSpirit's state was not inspected, so this is not evidence either way.
- `asic-reference.md` section 10 says firmware senses the configuration through ADC5/ADC7.
  The start-up path above does not do that; the 464+ ADC defaults returned for every model by
  `rtl/plus/asic_regs.v` are therefore irrelevant to this symptom. Whether any other firmware
  routine reads them is unexamined.
- A real GX4000 photograph or capture would upgrade the prediction to hardware evidence.
