# Plus hardware-defect triage — 2026-09-01

This record separates the new hardware symptoms from mechanisms proven in the
current RTL.  Simulation evidence identifies a retest candidate; it is not a
hardware closure claim.

## B13 lifecycle: stale classic cartridge state before a CPR

Hardware sequence: after a cartridge had misbehaved, a subsequently loaded
known-good Navy Seals CPR showed a black screen while music continued.  CPR
apply reset did not recover it; FPGA reconfiguration did.

The original `rom_map` suspect is real state but not a Plus-mode cause in this
revision:

- `Amstrad.sv` initializes `rom_map` only at FPGA configuration and otherwise
  only sets bits after classic ROM downloads.  The map therefore persists
  across every runtime reset.
- `rtl/Amstrad_motherboard.v` forces the classic `Amstrad_MMU` ROM-enable input
  inactive in Plus mode.  CPR windows are instead owned by `plus_mmu`.
- The earlier B7 dark-silicon result independently established that corrupting
  the classic path does not move the Plus signature.  Clearing `rom_map` on a
  CPR or ordinary reset would therefore be a speculative hardware fix and
  would also eject classic expansion ROMs that should survive a soft reset.

The sibling state audit did find a production-visible lifecycle defect.
`dan_eeprom_loaded` is initialized only by FPGA configuration, becomes set
after a Dandanator image download, and survives ordinary reset and CPR apply.
Before this change, `dan_ena` could still override the main SDRAM address and
bank mux in Plus mode.  The real Dandanator controller is reset by CPR apply,
but its reset mapping can select the loaded EEPROM again as soon as the CPU
runs.  That gives configuration-initialized state a live route into a later CPR
even though stale `rom_map` itself is isolated.

The production `plus_legacy_cart_gate` owns the persistent loaded latch and its
focused lifecycle vector first failed with:

```text
FAIL legacy-cart lifecycle: persistent classic cartridge state poisoned Plus CPR memory ownership
```

The fix gates Dandanator SDRAM ownership with `!plus_mode`.  It deliberately
does not clear `dan_eeprom_loaded`: switching back to classic mode makes the
previously loaded expansion cartridge available again, matching physical
hardware across a soft reset.  Unloaded and chip-not-selected states remain
inactive.

### Reset-tier decision

| Boundary | `rom_map` | Dandanator image | Plus bus ownership |
|---|---|---|---|
| FPGA configuration | Cleared, then boot ROM pages are registered | Cleared | None until selected |
| Classic soft/menu/key reset | Preserved | Preserved | Classic device may resume |
| Classic ROM download | Adds the downloaded page | Preserved | Classic device may resume |
| SNA apply | Preserved; used only by the classic MMU | Preserved | Follows the currently selected machine; the SNA does not select it |
| CPR download/apply reset | Preserved but unreachable from Plus cartridge reads | Preserved | Classic Dandanator ownership suppressed |
| Plus/classic model switch | Preserved | Preserved | Combinatorially follows the selected machine |
| Explicit Dandanator detach (`status[32]`) | Preserved | Cleared | Released |

Plus/classic selection comes from the OSD model setting, not from the CPR
artifact itself.  With a Plus model selected, this closes the deterministic
state-leak mechanism in simulation.  Navy Seals must still be retested after
first loading a Dandanator image that makes its reset mapping active; without
that prerequisite, the original title symptom remains unassigned rather than
being attributed to this fix.

## BASIC/System CPR disk failure

The production-path control uses the tracked `rtl/u765/test.dsk` EDSK
(`SHA-256 591027f461e77cea900d3781e479afcf8a44c95a3131e5ea9ceb1d6bf6d005cc`).
That image is already the real-u765 leaf control; track 0/head 0 independently
describes sector `&41`, N=2, with 512 payload bytes beginning at file offset
`&200`.

`p10_boot_test_top` now connects its existing TV80/T80pa production
motherboard path to the real `rtl/u765/u765.sv` and exposes only the MiSTer SD
block transport to C++.  A cartridge-resident CPU program uses the Plus aliases
`&FADD` and `&FBDE/&FBDF` and issues:

```text
46 00 00 00 41 02 41 1E FF
```

The first-divergence trace is:

1. motor select and all nine command bytes reach the real decoder/controller;
2. the post-CPR-reset track-info reload completes;
3. u765 finds sector `&41` at physical byte position 30 and requests payload
   LBA 1;
4. with CPU and u765 enables derived from the exact production shared divider,
   the TV80 surrogate's first payload store is `&00` instead of the EDSK's
   `&21`; at that exact store edge u765 is still before data-ready (state 9,
   MSR `&50`).

Step 4 is the first divergence and is retained as `XFAIL fdc-payload-poll`; a
complete matching 512-byte payload is an XPASS so the marker cannot silently
hide a repair.  The real-u765 leaf suite independently keeps its EDSK parser,
reset/reload, and READ DATA controls green.  The production-path result does
not distinguish a TV80 instruction/polling limitation from behavior of the
production VHDL T80, which Verilator cannot compile, so it is not evidence for
changing u765 media, sector, or status RTL.

The FDC diagnostic selects a production-clock mode that derives both registered
enables from the same free-running three-bit counter.  The older P10a mode
remains isolated for its pre-existing cartridge trace.  An attempted sweep
with unrelated CPU/u765 dividers changed the payload result immediately and
was rejected as non-production evidence.  No speculative controller fix is
made.

This also prevents a no-media observation from being mislabeled: without an
image, there can be no LBA/payload trace, so a BASIC/AMSDOS prompt is not a
controller-read reproduction.  Hardware closure still requires the failing
System CPR with a known-good AMSDOS disk plus either a real-T80-capable trace or
hardware capture at the first MSR/data-read transition.

## Sprite X and screenshot defects

Pending source-derived discriminators.  Screenshots remain observations, not
coordinate, palette, split, or CRTC timing oracles by themselves.
