# B18: save SNA snapshots from the running core

Design position for backlog B18, written 2026-09-13. Scope for this branch is a classic
SNA v3 writer (header plus 64K/128K memory dump). The CPC+ chunk is a later Plus-scope
slice that reuses the same freeze and transport.

## Transport: MiSTer save-state slots in DDR3

The core writes the finished snapshot into a DDR3 save-state slot, and MiSTer Main copies
the slot to the SD card. Main needs no change. Facts checked in Main_MiSTer `master` on
2026-09-13:

- A second CONF_STR field `SS<base>:<size>` (hex) declares four consecutive slots of `size`
  bytes starting at physical `base`, which must lie in 0x20000000-0x3FFFFFFF
  (`user_io.cpp`, config parse). Candidate declaration: `Amstrad;SS3E000000:40000;`, which
  gives 256 KiB per slot. A classic 128K v3 file is 0x20100 bytes, and a CPC+ chunk adds
  0x900.
- Slot layout: word 0 is a change counter, word 1 is the payload length in 32-bit words, then
  the payload. Main polls once a second. When the counter changes it writes `(len+2)*4` bytes,
  header included, to `savestates/Amstrad/<loaded file>_<slot>.ss` (`process_ss`,
  `FileGenerateSavestatePath`). SNA sizes are multiples of 4, so dropping the first 8 bytes
  gives an exact `.sna`.
- Main enables slot polling only after a file is loaded through an `FS...` menu entry
  (the `opensave` flag in `user_io_file_tx`). Loads for SNA, CPR and tape should therefore use
  `FS`. **Mounting a DSK does not enable it**, so a disk-only session saves into DDR3 but
  Main never copies the slot to SD.
- Over SSH, a host script can read the slot through `/dev/mem` whether or not Main has enabled
  polling, as the SSM ring at 0x30000000 already does. This is the capture-loop path
  (`driver.py`), and it also covers DSK sessions.

**Rejected alternative: `ioctl_upload_req`.** Main polls UIO 0x3C only for C64/C128, and
that handler (`c64_save_cart`) is a bespoke EasyFlash writer. Main already knows the file
name and fixed size from the earlier cartridge load, and formats the file itself. The same
route for Amstrad would need new Amstrad-specific Main code for a name, a length protocol and
an SNA writer. Its only gain is saving straight to SD during a DSK-only session. If that
matters later, the smaller Main patch is to enable `process_ss` on a disk mount as well.

## Freeze point: M1 T2 of a new instruction

The save must sample a state the SNA format can express. It stops the CPU as `sna_hold` does,
by gating both T80pa clock enables, but only at a moment when every architectural register
is committed.

T80 finishes some register writes late: `Save_ALU_r` writes `A` and `F` on the first clock
enable of the *next* instruction's M1. Its T2 enable then increments `PC` and `R` and loads
`IR` from the fetched byte. The window after the M1 T1 enable and before the T2 enable
therefore has:

- all registers committed, `PC` equal to the fetch address, and `R` not yet incremented;
- the opcode fetch already requested (`MREQ_n`/`RD_n` low). Stopping here is the same as an
  extended hardware WAIT at T2, which the Gate Array already inserts.

The window counts as an instruction boundary only when the previous M1 byte was not a
prefix (`DD`, `FD`, `ED`, `CB`) and the cycle is neither an interrupt nor an NMI acknowledge.
T80 knows this internally (`Prefix`, `IntCycle`, `NMICycle`, `MCycle`, `TState`), but
`T80pa` exposes none of it. The minimal change is one new T80/T80pa output that is true in
that window, mirrored in the TV80 simulation stand-in. The production VHDL needs its own
GHDL trace test, because TV80 commits registers at different T-states. That test covers
deferred ALU writes, each prefix family (including `DD CB d op`), interrupt and NMI
acceptance, and HALT.

HALT: T80 holds `PC` after the `HALT` opcode while halted. Save `PC-1` when `HALT_n` is low, so
a reloaded snapshot waits in `HALT` again instead of falling through. This is a
convention choice; record it in the test.

## Memory and state capture

- **Header.** Latch every field on the freeze clock, then write from the latch. The Gate Array
  and CRTC keep running while the CPU is held, so values sampled later would drift.
- **RAM.** Stream the dump through the SDRAM controller's held `cart_*` request port, muxed
  away from `plus_cartridge_memory` during a save. Pages use the loader's
  `{8 + page, offset}` layout on the model bank. Cartridge reads land in `cart_dout` and
  leave `ram_dout` alone, so the CPU's outstanding fetch byte survives the stream. The main
  port is edge-triggered on `oe`, so it must not be reused.
- **DDR3.** The writer shares the DDR3 master with `ssm_marker`. SSM events are rare and
  small, so a simple mux that finishes the in-flight write and holds the other client is
  enough.
- **Resume timing.** Interrupts and video keep counting during the stream. The saved file is
  exact, but the resumed machine can slip timing by the stream's duration. That is
  acceptable because the file is the point of the feature.

## State owners to read back (classic)

| SNA field | Owner | Readback today |
|---|---|---|
| Z80 registers, IFF, IM | `T80pa.REG` | Yes |
| GA pen, palette, multi-config | `ga40010.sv` (`inksel`, `inkr`, mode/ROM enables) | Internal, needs ports |
| RAM config, ROM select | `Amstrad_MMU.v` (`RAMmap`, `RAMpage`, `ROMbank`) | Internal; ROM select needs a shadow of the last byte written, because `ROMbank` is filtered by `rom_map` |
| CRTC select, R0-R17, v3 counters and flags | `CRTC.v` | Internal, needs observation ports only (accuracy-stream file, no behaviour change) |
| PPI A/B/C, control | `i8255.v` | Internal, needs ports |
| PSG select, R0-R15 | `YM2149.sv` (`ymreg`, `addr`) | Internal, needs ports |
| FDD motor, track | `Amstrad.sv` `motor`, `u765` `pcn` | Motor visible; track internal |
| GA v3 B2-B4 | `GA40010/syncgen*.v` (`hcnt`, `intcnt`), `INT_N` | Internal, needs ports |

Classic `CRTC.v` restores registers only, not v3 counters, so a classic round trip cannot prove
counter fields through reload. Check those against the live counters at the freeze point.

## Slices and acceptance

1. **Freeze predicate.** Add the T80/T80pa boundary output, write a failing GHDL trace
   test first, then mirror the output in TV80. Needs GHDL installed.
2. **Header decode extraction.** Move the inline Z80/PPI/PSG/memory-size decode out of
   `Amstrad.sv`, which is neither linted nor simulated, into a module beside
   `plus_sna_header`, so a round trip goes through the production decoder.
3. **Readback ports** on the owners above, observation only.
4. **Writer.** Freeze controller, header latch, SDRAM stream through `cart_*`, DDR3 slot
   framing with the change counter, and the mux with `ssm_marker`.
5. **Integration.** CONF_STR `SS` declaration, OSD save action, `FS` on SNA/CPR/tape loads,
   a `driver.py` slot pull, then a hardware check.

Acceptance vector: run a program in the whole-motherboard fixture, save at a boundary, feed the
bytes back through the production decoder and loader, and compare CPU, owner and RAM state.
Every expected value comes from the SNA specification
(`docs/references/Snapshot (.SNA) file format.md`), never from the simulator.
