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

Pending production CPU/u765/known-good-media trace.  No RTL conclusion yet.

## Sprite X and screenshot defects

Pending source-derived discriminators.  Screenshots remain observations, not
coordinate, palette, split, or CRTC timing oracles by themselves.
