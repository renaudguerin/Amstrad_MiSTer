# B18: save SNA snapshots from the running core

Design position for backlog B18. Scope for this branch is a classic SNA v3 writer (header
plus 64K/128K memory dump). The CPC+ chunk is a later Plus-scope slice that reuses the same
freeze and transport. The Astra high design review of 2026-09-13 returned REWORK. Its
findings, checked against source, are folded in below.

## Transport: core writes a DDR3 slot, host pulls it

The OSD "Save snapshot" action writes the finished file into a DDR3 region. A host script
over SSH (`scripts/hardware-loop`, alongside `driver.py`) reads it through `/dev/mem`, as
the SSM event ring at 0x30000000 already is. This covers B18's purpose, handing a hardware
state to simulation, with no Main change. It works during DSK sessions as long as the core
stays loaded.

- **Region.** Use MiSTer's save-state slot layout at 0x3E000000, 256 KiB per slot. That
  sits clear of the SSM ring and the scaler buffer; the GBA core uses the same base. Word 0
  is a change counter, word 1 the payload length in 32-bit words, then the payload. A
  classic 128K v3 file is 0x20100 bytes; a CPC+ chunk adds 0x900. The live HPS kernel
  reservation of this range is unverified; check `/proc/iomem` on the device before the
  hardware test.
- **Publication.** The writer invalidates the header first, writes the payload, and only then
  publishes length and counter. The host pull reads the counter before and after copying
  and retries if it changed. Only one save runs at a time. `ssm_marker` shares the DDR3 master
  and is held off while a save owns it; it already counts dropped events.
- **No `SS` CONF_STR declaration yet.** Without an `FS` load, Main only records the
  parameters (`process_ss` never enables), so declaring `SS` buys nothing today.

**Why not SD directly.** Facts checked in Main_MiSTer `master`, 2026-09-13:

- `ioctl_upload_req` (UIO 0x3C) is polled only for C64/C128 (`user_io_poll`) and for arcade
  NVRAM (`MENU_SAVE_CHECK`). Both handlers are bespoke: Main knows the file name and a fixed
  size from an earlier load, and formats the file itself. The same route for Amstrad means
  new Amstrad-specific Main code.
- Main copies save-state slots to `savestates/<core>/<file>_<n>.ss` only after a file load
  through an `FS` menu entry. That same flag mounts a writable `saves/<core>/<name>.sav` on SD
  slot 0 (`user_io_file_mount(buf, 0, 1)`), and slot 0 is this core's drive A.

Deferred routes to automatic SD copy: move drives A/B to SD slots 1/2 and leave slot 0 as the
`.sav` sink, then add `SS` and `FS` entries (this changes the disk-slot layout); or patch Main,
which also covers DSK-only sessions.

## Freeze point

The save stops the CPU the way `sna_hold` does, by gating both T80pa enables, but only at a
state the SNA format can represent.

**Where in the cycle.** T80 finishes some register writes late: `Save_ALU_r` commits `A`/`F`
on the first core enable of the next instruction's M1 (the T1 tick). The T2 tick then
increments `PC` and `R` and loads `IR`. The capture point is therefore after the T1 tick
and before the T2 tick. At that point `PC` is the fetch address, `R` has not been
incremented, and the fetch is outstanding on the bus.

**Which instructions qualify.** `IR` still holds the previous opcode during that window, and
some opcodes act on the T2 tick of the next fetch:

- `EI` (`SetEI`) sets IFF1/IFF2 there (`T80.vhd:1253`), and suppresses interrupt acceptance
  for one instruction.
- `RETN` (`I_RETN`) copies IFF2 into IFF1 there.
- A prefix (`Prefix /= "00"`) means the next M1 continues the same instruction. Indexed-CB
  reads its final opcode outside M1, so remembering the last M1 byte is not a substitute
  for T80's semantic `Prefix`.
- Interrupt and NMI acknowledge cycles (`IntCycle`, `NMICycle`) are not instruction starts.

The predicate is one new T80/T80pa output, true for `MCycle=1, TState=2` when the previous
instruction set none of those conditions. It is computed inside the core, not by
re-decoding opcodes outside the vendored source. The save waits for the next qualifying
fetch, which arrives within a few instructions. TV80 mirrors the output for Verilator
fixtures, but TV80 commits at different T-states, so it cannot validate the production
predicate.

**Entry and release against WAIT.** T80pa samples `WAIT_n` into `CEN_pol` on the
negative enable during T2 (`T80pa.vhd:170`). The hold must be asserted after the positive
T1 tick and before that negative enable, so the stop lands before the WAIT decision. On
release, the first negative enable samples WAIT fresh against the Gate Array's
current READY. The Gate Array and CAS sequencing keep running during the hold. The test
must show this entry/release point is equivalent to a hardware WAIT. REG is sampled after
the T1 commit, while held.

**HALT.** While halted, T80 holds `PC` after the `HALT` opcode and increments `R` on every M1.
Save `PC-1` and `R-1` (low seven bits) so a reloaded snapshot refetches `HALT` and ends with
the same `R`. This is re-execution, not a stored halt flag, which SNA does not have.

## Memory capture

- **Admission.** A save starts only outside reset, download and snapshot apply, and only when
  no cartridge SDRAM request is outstanding. `plus_cartridge_memory`'s `busy` means
  "loading", not "idle", so wait for `cart_req` to drop.
- **Stream.** Read RAM through the SDRAM controller's held `cart_*` port, muxed away from
  `plus_cartridge_memory`, with ACK routed only to the current owner. Pages use the loader's
  `{8 + page, offset}` layout on the model bank (`Amstrad.sv:449`). Cartridge reads land in
  `cart_dout` and leave `ram_dout` untouched (`sdram.v:295-305`), so the CPU's outstanding
  fetch byte survives. The main port is edge-triggered and is not reused.
- **Fairness.** Continuous cartridge requests outrank tape and VRAM (`sdram.v:164-199`). The
  stream drops its request for one idle slot between grants, so video and tape keep being
  served. Refresh is protected either way.
- **Timing slip.** Interrupts and video keep counting during the stream, so the resumed
  machine may slip by the stream's duration. The file itself is not affected.

## Header fields and their sources (classic)

All fields are sampled on one capture clock from the same pipeline stage. Mixing a
registered internal flag with a delayed public output is not coherent.

| Offset | Field | Source and conversion |
|---|---|---|
| 11-2D | Z80 | `T80pa.REG` (already normalizes alternate banks), with the HALT adjustment above |
| 2E-3F | GA pen, palette | `ga40010.sv` `inksel`, `inkr`, `border` (new ports) |
| 40 | GA multi-config | `0x80 \| {hromen, lromen, mode}`. Bit 4 (interrupt-counter reset) is a write strobe with no storage and is saved as 0 |
| 41 | RAM config | `{2'b00, page bits, RAMmap}` from `Amstrad_MMU.v`. Configurations whose `RAMpage` selects beyond the 128K dump are refused rather than saved with dangling state; 464/664 save 0 |
| 42-54 | CRTC select, R0-R17 | `CRTC.v` register fields reassembled into bytes; R16/R17 saved as 0 |
| 55 | ROM select | New shadow of the last byte written, because `ROMbank` is filtered by `rom_map` |
| 56 | PPI A | Value on port A's pins: output latch in output mode, PSG readback in input mode (spec note 6) |
| 57 | PPI B | Port B input value (`ipb`: tape, jumpers, VSYNC) (note 7) |
| 58 | PPI C | Port C outputs (note 8) |
| 59 | PPI control | Mode word with bit 7 forced to 1 (note 9) |
| 5A-6A | PSG | `YM2149.sv` `addr`, `ymreg` |
| 6B-6C | Memory size | 128 for the 6128 map, 64 otherwise |
| 6D | CPC type | `model` 0/1/2 (6128/664/464) maps to 2/1/0 |
| 9C-A0 | FDD motor, track | `motor` latch; `u765` `pcn[0]` (new port), 0 when no drive is mounted |
| A1 | Printer | New shadow of the last printer-port write, 0 after reset |
| A4 | CRTC type | Menu bit `status[2]` is inverted: 0 means type 1, so save 1; 1 means type 0, so save 0 |
| A9-AD | CRTC counters | HCC, row, raster line direct. VTA count (AD): type 1 uses its adjust counter; type 0 has no separate one (`c5_next` is 0) and counts adjustment in `line`, so AD comes from `line` while in adjust |
| AE | HSYNC width | `hsc` counts up; direct |
| AF | VSYNC width | SNA counts up, but `vsc` counts down from a type-specific load (type 0 R3−1, type 1 15). An observation-only elapsed-lines counter, started on the internal VSYNC edge, avoids depending on an R3 that may change mid-pulse |
| B0 | CRTC flags | Internal `VSYNC_r`, internal HSYNC, in-adjust flag. Not the delayed public `VSYNC` (`CRTC.v:690`) |
| B2 | GA VSYNC delay | Decode `syncgen_sync.v` `hcnt` from its encoded sequence (00, 01, 06, ...) to 2/1/0, the inverse of the B8-5 mapping |
| B3 | GA interrupt count | `intcnt` direct |
| B4 | Interrupt pending | GA `INT_N` low |

All new ports are observation-only. `CRTC.v`, `ga40010.sv` and `syncgen_sync.v` belong to the
accuracy work stream; these changes must leave their behavior and the soak hash untouched.

## Acceptance

Capture correctness and restore correctness are separate claims. The existing loaders do not
restore classic CRTC counters, GA interrupt phase, FDC state, or anything B8-5 lists as
unrepresented. A round trip therefore proves only the fields the loaders consume. The
remaining fields need direct checks of the capture.

1. **Freeze predicate on production T80.** Use the real-GA harness (`sim/crtc_t80_top.sv`,
   Verilator on the GHDL-translated T80 netlist; needs GHDL). The expected value at each
   boundary is derived from the instruction sequence on paper. The Makefile's `t80-trace-test`
   compares only VHDL against its translated netlist, so it cannot reject a wrong predicate.
   Cases: deferred ALU writes, each prefix family including `DD CB d op`, `DI; EI; NOP`,
   `EI; HALT` with a pending INT, `RETN`, `LD A,I`/`LD A,R` P/V, `LDIR`/`OTIR` repeats, `DJNZ`,
   interrupt and NMI acceptance, HALT. Add a continuation check: the same program held at a
   boundary for many clocks and run uninterrupted must reach the same architectural trace.
2. **Header bytes.** In the whole-motherboard fixture, check each saved byte against a value
   derived independently from the program's writes, per the SNA specification. Never read the
   expectation out of the simulator.
3. **Round trip.** Extract `Amstrad.sv`'s inline Z80/PPI/PSG/memory decode into a linted
   module beside `plus_sna_header`. Wire the production decoder and apply path into the
   fixture (P10 currently ties `sna_load`/`sna_hold` low). Save, reload, and compare the
   loader-consumed state and RAM. PPI A/B compare as the spec's pin values, not as output
   latches.

## Slices

1. Freeze predicate: T80/T80pa output, then the failing production-T80 test, then TV80 mirror.
2. Header decode extraction from `Amstrad.sv`.
3. Observation ports and shadows on the owners above, with the conversions.
4. Writer: freeze controller, header latch, SDRAM stream, DDR3 slot publication, SSM hold-off.
5. Integration: OSD action, host pull script, device test.
