# B18: save SNA snapshots from the running core

Design position for backlog B18. Scope for this branch is a classic SNA v3 writer (header
plus 64K/128K memory dump). The CPC+ chunk is a later Plus-scope slice that reuses the same
freeze and transport. Two Astra high design reviews on 2026-09-13 returned REWORK, the second
with no blocker. Their findings, checked against source, are folded in below.

## Status and limitation: a development tool, not a user feature

**Works on hardware (2026-09-14, RBF `0608653`):** a classic 6128 snapshot saved from the OSD
and pulled with `sna_pull.py` reloads correctly both in this core and in AmSpirit.

**Not shippable as a public feature in this form.** The core cannot put the file on the SD
card. After pressing Save snapshot, someone must run `scripts/hardware-loop/sna_pull.py` on
another computer, over SSH as root, to copy it out of DDR3. That suits handing a hardware state
to simulation during development. It is not a workflow to offer ordinary MiSTer users.

Making it a normal "save to SD" feature needs one of these, each still to be decided:

- **A Main_MiSTer change (upstream discussion).** Main already writes core files to SD for
  C64/C128 and arcade NVRAM through `ioctl_upload`, with per-core handlers. An Amstrad or
  generic handler would save a plain `.sna` wherever Main chooses. It also works in DSK-only
  sessions, and it is the cleanest result for users.
- **Main's existing save-state copy, no Main change.** Main copies DDR3 slots to
  `savestates/<core>/` when the core declares `SS` and a file was loaded through an `FS` menu
  entry. Three costs:
  - `FS` claims SD slot 0, which is drive A here, so the disk drives must move to slots 1/2.
  - The file is an `.ss`, not a `.sna`: an 8-byte header precedes the snapshot, and it is
    named after the `FS`-loaded file.
  - Our slot format is not Main's. Main reads 32-bit words: counter at byte 0, length at byte
    4, payload from byte 8 (`process_ss` in `user_io.cpp`, checked 2026-09-14). This design
    uses 64-bit words with the payload at byte 16. Main also writes as soon as the counter
    changes, so the all-ones "publication in progress" value would trigger a copy of an
    incomplete slot. The stream would have to write the payload first, then length and counter.

Until one of these lands, keep the OSD entry described as a development aid.

## Transport: core writes a DDR3 slot, host pulls it

The OSD "Save snapshot" action writes the finished file into a DDR3 region. A host script
over SSH (`scripts/hardware-loop`, alongside `driver.py`) reads it through `/dev/mem`, as
the SSM event ring at 0x30000000 already is. This covers B18's purpose, handing a hardware
state to simulation, with no Main change. It works during DSK sessions as long as the core
stays loaded.

- **Region.** 0x3E000000, the base MiSTer uses for save-state slots (the GBA core uses it
  too), clear of the SSM ring and the scaler buffer. The kernel's `memmap=513M$511M` keeps
  Linux out of it, and the 2026-09-14 device test read it successfully. Word 0 (64-bit) is a
  change counter, word 1 the payload length in 32-bit units, then the payload from byte 16.
  That is **not** Main's own save-state slot format, which uses 32-bit words; see "Status and
  limitation" above. A classic 128K v3 file is 0x20100 bytes; a CPC+ chunk adds 0x900.
- **Publication.** Word 0 carries an explicit invalid generation (all ones) while a save is in
  progress. The writer writes it first, then the payload, then the length, and last the new
  generation. The host pull rejects the invalid generation and any length that is not a legal SNA
  size. It copies only when it reads the same valid generation before and after the copy. Only one
  save runs at a time.
- **SSM coexistence.** `ssm_marker` shares the DDR3 master. While a save owns it, the marker
  sees `ddram_busy=1`. Its write handshake already holds a stalled transaction rather than
  dropping it (`ssm_marker.v:310-315, 359-360`), so no event is lost to the mux.
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
  for one instruction. `RETN` needs no exclusion: `I_RETN` is decoded in its own third M-cycle
  (`T80_MCode.vhd:2094`), so IFF1 is restored before the next fetch.
- A prefix (`Prefix /= "00"`) means the next M1 continues the same instruction. Indexed-CB
  reads its final opcode outside M1, so remembering the last M1 byte is not a substitute
  for T80's semantic `Prefix`.
- Interrupt and NMI acknowledge cycles (`IntCycle`, `NMICycle`) are not instruction starts.

The predicate is one new T80/T80pa output, true for `MCycle=1, TState=2` when the previous
instruction set none of those conditions. It is computed inside the core, not by
re-decoding opcodes outside the vendored source. A save request stays pending until a
qualifying fetch. Ordinary code reaches one within an instruction or two, but an unbroken
stream of `EI` or prefix bytes can postpone it indefinitely. A second OSD press or reset
therefore cancels a pending request. TV80 mirrors the output for Verilator
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
- **Mapping admission.** The dump covers physical pages 8-15, the base 128K. A save is admitted
  only when `RAMpage == 3`, meaning no expansion page is paged in (`Amstrad_MMU.v:61, 68`).
  Otherwise the save is refused, because the header would name memory the file does not contain.
- **Fairness.** Continuous cartridge requests outrank tape and VRAM (`sdram.v:164-199`). After
  each grant, the stream keeps its request low until the controller has passed through a real
  `q == STATE_IDLE` arbitration with other clients eligible, not merely for one clock. Video
  and tape keep being served, and refresh is protected either way.
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
| 41 | RAM config | `{5'b00000, RAMmap}` from `Amstrad_MMU.v`: admission guarantees zero page bits. 464/664 save 0 |
| 42-54 | CRTC select, R0-R17 | `CRTC.v` register fields reassembled into bytes; R16/R17 saved as 0 |
| 55 | ROM select | New shadow of the last byte written, because `ROMbank` is filtered by `rom_map` |
| 56 | PPI A | `ipa`, the input value regardless of direction (spec note 6; `Amstrad_motherboard.v:1073`). Not the direction-selected CPU read value |
| 57 | PPI B | Port B input value (`ipb`: tape, jumpers, VSYNC) (note 7) |
| 58 | PPI C | Port C outputs (note 8) |
| 59 | PPI control | Mode word with bit 7 forced to 1 (note 9) |
| 5A-6A | PSG | `YM2149.sv` `ymreg`; select is `addr[3:0]`. `addr` holds 8 bits, and a nonzero high nibble disables register access (`YM2149.sv:95`), which the 0-15 field cannot express. That state is normalized to its low nibble, and the loss is documented: after reload, access is enabled until the next select write. Exact round-trip claims exclude this case |
| 6B-6C | Memory size | 128 for the 6128 map, 64 otherwise |
| 6D | CPC type | `model` 0/1/2 (6128/664/464) maps to 2/1/0 |
| 9C | FDD motor | `motor` latch |
| 9D-A0 | FDD tracks A-D | `u765` `pcn[0]`, `pcn[1]` (new port) for drives A and B, whatever the media state, since the controller's cylinder is independent of a mounted image; 0 for the absent drives C and D |
| A1 | Printer | No printer port or latch exists today; needs a new passive decode of printer-port writes, 0 after reset |
| A4 | CRTC type | Menu bit `status[2]` is inverted: 0 means type 1, so save 1; 1 means type 0, so save 0 |
| A9-AD | CRTC counters | HCC, row, raster line direct. VTA count (AD): type 1 uses its adjust counter; type 0 has no separate one (`c5_next` is 0) and counts adjustment in `line`, so AD comes from `line` while in adjust |
| AE | HSYNC width | `hsc` counts up; direct |
| AF | VSYNC width | SNA counts up, but `vsc` counts down from a type-specific load (type 0 R3−1, type 1 15). Use an observation-only elapsed counter that resets on every accepted `vsc` load, not on a VSYNC edge. Adjacent pulses can reload `vsc` with no rising edge. It advances on exactly `vsc`'s qualified count ticks (`vsync_count_tick`), including holdoff suppression and the half-line phase (`CRTC.v:651-653, 724-743`), and is captured alongside `VSYNC_r` |
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

1. **Freeze predicate on production T80.** Extend the real-GA harness (`sim/crtc_t80_top.sv`,
   Verilator on the GHDL-translated T80 netlist, run by CI's `production-t80` job). It
   currently ties off hold, REG and interrupts (`crtc_t80_top.sv:115-136`); add all three,
   with controllable INT/NMI. The expected value at each boundary is derived from the
   instruction sequence on paper. `t80-trace-test` compares only VHDL against its translated
   netlist, so it cannot reject a wrong predicate. Cases: deferred ALU writes, each prefix
   family including `DD CB d op`, `DI; EI; NOP`, `EI; HALT` with a pending INT, `RETN`,
   `LD A,I`/`LD A,R` P/V, `LDIR`/`OTIR` repeats, `DJNZ`, interrupt and NMI acceptance, HALT.
   **Continuation:** a hold at a boundary is compared with an equivalent-duration hardware
   WAIT under identical external stimuli, not with an uninterrupted run. Interrupts and live
   inputs (PPI B carries VSYNC and tape) legitimately differ from an uninterrupted run. An
   uninterrupted comparison is valid only for programs with interrupts disabled and
   time-independent inputs.
2. **Header bytes.** The existing whole-motherboard fixtures are not sufficient. P10 links
   TV80, a GA stub and a PSG stub that holds no register state (`sim/plus/Makefile:267-269`,
   `motherboard_lint_stubs.v`), and fixes the RAM bank at 0 (`p10_boot_test_top.v:414`). The
   capture fixture needs a B7-style composition with the real GA, PSG and HID, the production
   clock divider, classic model and bank selection, and the production-T80 netlist. Check each
   saved byte against a value derived from the program's writes per the SNA specification;
   never read the expectation out of the simulator. Also exercise DDR3 stalls during a save
   and a host read that overlaps publication.
3. **Round trip.** Extract `Amstrad.sv`'s inline Z80/PPI/PSG/memory decode into a linted
   module beside `plus_sna_header`. Wire the production decoder and apply path into that
   fixture (P10 ties `sna_load`/`sna_hold` low). Save, reload, and compare loader-consumed state
   and RAM. PPI A compares as the input value (spec note 6), not as an output latch.

## Slices

1. Freeze predicate: T80/T80pa output and production-T80 test. **Done.** `T80.vhd` drives
   `InsnStart` (`MCycle=1`, `TState=2`, `Prefix="00"`, `SetEI='0'`, no interrupt or NMI
   acknowledge) and `T80pa` exports it as `INSN_START`. `sim/t80_freeze_top.sv` plus
   `t80_freeze_test.cpp` (`make -C sim t80-freeze-test`, part of `production-t80-test`) put the
   GHDL-translated T80 on the real GA enables and WAIT equation. They cover 13 cases: deferred
   ALU, EI, prefix families including DDCB, final opcodes equal to prefix bytes (`CB CB`,
   `CB DD`, `ED ED`), LD A,I and LD A,R P/V, LDIR/DJNZ, OTIR with port writes, IM 1, NMI/RETN,
   EI;HALT with INT already pending, HALT woken by INT, and hold versus an equal-length hardware
   WAIT. The hold case stalls inside LDIR and inside OTIR. It requires every REG bit (alternate
   banks loaded with distinct values) to match a free run at every boundary, the same memory and
   port writes, and a post-release bus trace (address, data, controls) identical to the WAIT run.
   Discrimination: six predicate mutants each fail. Dropping `SetEI`, `Prefix`, both acknowledge
   terms or only `NMICycle` fails; so does sampling at T1, and so does replacing semantic `Prefix`
   with a raw-opcode test (the prefix-valued opcodes case). The Astra high code review of the
   first commit was CLEAR WITH CHANGES; this suite closes its three test gaps.
   The TV80 mirror is deferred until a TV80-based fixture needs the port, because the capture
   fixture uses production T80. Harness note: T80pa never resets `IntCycleD_n`, so `IORQ_n` is
   low on the first fetch after reset; the acknowledge checks start after the first refresh.
2. Header decode extraction from `Amstrad.sv`.
3. Observation ports and shadows on the owners above, with the conversions. **Done.** Each owner
   exports its registered state on `SNAP_*`/`snap_*` outputs, gathered as `snap_*` outputs of
   `Amstrad_motherboard` (left unconnected in `Amstrad.sv` until the writer). New state is limited
   to three observers: `vsw_elapsed` in `CRTC.v`, assigned beside every `vsc` load and decrement;
   an unfiltered ROM-select shadow in `Amstrad_MMU.v`; and a printer-port (`&EFxx`, A12 low)
   shadow in the motherboard. The GA counters come from `syncgen_sync`, the production generator
   (`syncgen` is Verilator-only). `u765` copies `pcn` one clock late, because `pcn` is local to
   its named `fdc` block and Quartus does not accept a hierarchical read; a seek step landing on
   the capture clock may therefore save the previous cylinder. `rtl/sna_hw_header.v` is the pure
   combinational formatter for offsets 2E-B4, lint-clean at `-Wall` and not yet instantiated.
   It takes the CRTC type as fed to `CRTC_TYPE` (`~status[2]`), and derives 41 and 6B-6C from
   `model == 0` (the 6128 map, equivalent to `ram64k == 0` in classic mode). Soak hash unchanged
   (`0xb1cb70da95c2e44f`). Vector `t36a_b18_vsw_elapsed_counter` checks AF as C3h at mid-line
   sample points: a type 0 R7-triggered pulse with the partial line excluded, a type 1 pulse
   counting 0-15, a restart on a second R7-triggered pulse, and continuous adjacent pulses on
   both types. Three counter mutants fail it: no reset on the R7-write load, counting through
   the type 0 holdoff, and resetting on a VSYNC rising edge instead of on loads. The remaining
   byte conversions are checked by the capture fixture (acceptance 2), not by a mirror test.
4. Writer: freeze controller, header latch, SDRAM stream, DDR3 slot publication, SSM hold-off.
   **4a done (freeze controller and header latch).** `rtl/sna_save_capture.v` is an
   IDLE/ARMED/HELD controller. A request is refused unless admitted with `RAMpage == 3`. While
   armed, a second request or loss of admission cancels it. The freeze happens on the rising edge
   of `INSN_START`: `hold` is registered from that clock, which is the entry slice 1 proved
   equivalent to a hardware WAIT. The mapping is checked again at that boundary. The same clock
   latches the 256-byte header: signature, version 3, REG bytes 11-2D with the HALT PC/R
   adjustment, and `sna_hw_header` bytes 2E-B4. `hold` stays high until `release_req`. The
   motherboard now takes `save_hold` beside `sna_hold` and exports `cpu_reg`, `cpu_insn_start` and
   `cpu_halt_n`. `Amstrad.sv` ties `save_hold` low until 4b. Every Verilog T80pa stand-in ties the
   new outputs off; TV80 cannot provide the predicate. `t80-freeze-test` adds five cases on
   production T80:
   - controller hold versus hardware WAIT inside LDIR and OTIR, including a request made while
     `INSN_START` is already high;
   - header CPU bytes derived from the continuation program;
   - HALT with R bit 7 set;
   - postponement across EI and DD runs, and cancel;
   - refusal at request and at the boundary.

   Four mutants fail: level-triggered freeze, no HALT adjustment, hold one clock late, and no
   boundary mapping check. Still open for 4c: loss of admission while HELD (a download starting
   during the stream) is not handled inside the controller.

   **4b done (memory stream and DDR3 publication).** `rtl/sna_save_stream.v` starts on
   `captured`. It reads RAM pages 8-15, or 8-11 for 64K, from the running model's bank on the
   SDRAM `cart_*` port. After each acknowledge it keeps `cart_req` low through the next `ce_ref`
   rising edge and the following clock, so it never holds the arbitration edge. The stream writes
   the slot in publication order: word 0 all ones; header plus RAM bytes little-endian from
   word 2; word 1 = file bytes / 4; word 0 = generation. Then it pulses `done`, which drives
   `release_req`. It reads the header straight from `sna_save_capture`, which holds it until
   release. The generation starts at 1, wraps before all ones, and deliberately survives core
   reset, because the host detects a new save by a changed generation. A reset during a stalled
   DDR write waits for acceptance before releasing the bus. `rtl/sna_ddr_mux.v` gives the stream
   the DDR3 port only when the SSM marker holds no write. While the stream owns the bus the
   marker sees busy, so its pending write stays queued.

   `sim/plus` target `sna-save-stream` runs the production `sdram.v` against a physical SDRAM
   model, plus a DDR3 slave with scripted busy and an SSM stand-in. Its cases:
   - 128K and 64K publication content and order;
   - random DDR stalls with a per-clock stability check;
   - fairness: tape and VRAM keep being served and refresh continues, with at most 2 cartridge
     grants between a tape request and its grant;
   - mux ordering against the second master;
   - start while active, reset quiesce, and generation continuity after reset.

   Five mutants fail it: publication order, no idle-slot wait, dropping `we` while busy,
   granting over an outstanding SSM write, and resetting the generation.

   The Astra high review of slice 4 returned CHANGES REQUIRED; all three findings are fixed.
   - The mux no longer drops ownership on reset while the stream holds a write.
   - The generation is consumed when its final write is issued, so a reset that drains or accepts
     that write cannot publish the same value twice.
   - The 64K test now decodes pages correctly and checks the payload.

   Cases 6D and 6E cover the reset paths, and the fairness case checks that tape is served in
   every quarter of the stream.

   **4c done (top-level integration and host pull).** `Amstrad.sv` instantiates the formatter,
   capture, stream and DDR3 mux, and connects the motherboard and `u765` observation ports.
   - **OSD.** `T[38],Save snapshot` in the main menu. An `I,` popup reports saved, refused or
     cancelled.
   - **Admission.** Classic mode only, outside reset, downloads and snapshot apply, with no
     Dandanator, no Multiface II and no cartridge request outstanding, plus `RAMpage == 3`.
     Plus mode and both overlays are refused because the file cannot carry their memory or
     mapping state. Losing admission while armed cancels the request.
   - **Abort while held.** Only reset or a snapshot load (`sna_download`) aborts, since a load
     rewrites the RAM being dumped. Every other download either resets the core or leaves RAM
     alone.
   - **SDRAM port.** `rtl/sna_cart_mux.v` shares `sdram.v`'s `cart_*` port with
     `plus_cartridge_memory`, which owns it by default. When the owner's request is low while
     the other client waits, the mux stops forwarding the owner's requests. It hands over after
     two `clkref` rising edges, so an acknowledge still owed to an aborted read reaches its
     requester, not the new owner. Masking also bounds the wait: the stream re-requests one
     clock after each arbitration and would otherwise hold the port for the whole save.
   - **Test.** `sna-save-stream` case 7 runs the mux against production `sdram.v` with a
     scripted held-request client. It covers an abort on the clock the stream's read reaches
     the SDRAM, and a service request made mid-stream that must be served within 64 clocks with
     its own byte. Three mutants fail it: no drain, no forwarding mask, and an acknowledge that
     is not gated by owner.
   - **Host.** `scripts/hardware-loop/sna_pull.py` reads the slot through the same `/dev/mem`
     mmap pattern as `ssm_ring.py`. It accepts a file only when the slot header, a re-read of
     it, and the image's own first word agree, with a legal length, generation, signature,
     version and memory size. Torn reads are retried. `test_sna_pull.py` builds slot images from
     the layout comment in `sna_save_stream.v`.

   Usage: start
   `python3 scripts/hardware-loop/sna_pull.py --target root@mister --out game.sna --wait 30`,
   then press Save snapshot in the OSD within 30 seconds. `--wait` ignores the generation
   already present when it starts and pulls the next one; a save finishes in well under a
   second, so pressing first would make it wait for yet another save. To collect a save that
   has already happened, omit `--wait`.

   `Amstrad.sv` has no simulation; Quartus synthesis is its only compile check.
5. Device test. **Done 2026-09-14** on RBF `0608653` (branch only, without master's
   `88262b9` changes): a classic 6128 snapshot saved from the OSD, pulled with `sna_pull.py`,
   reloads correctly in this core and in AmSpirit. Still open: acceptance 2 (capture fixture)
   and 3 (round trip), a 464/664 64K save, and the user-facing SD route under "Status and
   limitation".
