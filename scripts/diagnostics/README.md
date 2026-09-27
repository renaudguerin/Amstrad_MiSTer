# Original Plus / GX4000 PRI alias experiment

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

## Measurement protocol

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

## RAM inspection

These addresses are ordinary RAM, outside the paged ASIC area:

| Address | Meaning |
| --- | --- |
| `B000` | Current unsigned 16-bit IRQ count, little endian. |
| `B002` | Status: 0 during setup, 1–4 for the corresponding running case, `80` hex when all results have been drawn. |
| `B010` | PRI=100, R6=25 result, unsigned 16-bit little endian. |
| `B012` | PRI=10, R6=25 result. |
| `B014` | PRI=10, R6=34 result. |
| `B016` | PRI=255, R6=25 result. |

There is no hardware-error detection or result interpretation. An invalid
calibration or incomplete status should be reported alongside the counts.
