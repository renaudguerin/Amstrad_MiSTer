# Plus/classic UI, media, and architecture research brief

This is a future-session brief. It records unresolved product and architecture
questions without baking provisional answers into RTL or the current OSD.

## Problem statement

The core currently exposes separate classic `Model` and `Plus model` controls,
always-visible media loaders, implicit ROM state, and several reset/detach
actions whose scope is not obvious. Real use has demonstrated that individually
tested subsystems can still form invalid or surprising combinations.

Observed friction:

- a saved or automatically discovered ROM configuration can reappear without a
  visible inventory;
- expansion ROMs can be loaded but there is no clear per-slot or all-slot unload;
- the current remediation deliberately masks the classic onboard/expansion-ROM
  service in Plus mode, so a loaded upper-ROM inventory can remain visible in
  saved state yet silently have no effect; the OSD does not explain that state;
- `Reset & Detach Cartridge` does not explain whether it affects expansion ROMs,
  Dandanator, MF2, disks, tapes, snapshots, or saved model state;
- DSK and CDT remain visible for models that lack the corresponding hardware;
- classic and Plus model selectors can describe two machines simultaneously;
- CPC464 has special ROM-loading and concatenation rules that are not expressed
  in ordinary Amstrad lower-ROM/upper-ROM terminology;
- `boot.rom`, `boot.eXX`, per-model concatenation, and filename-triggered loading
  are powerful but opaque to a user inspecting the OSD.

## Research questions

1. Compare how mature MiSTer Amiga and Atari ST cores present mutually exclusive
   machine models, capability-dependent media, reset scope, and saved settings.
2. Compare Arnold, WinAPE, CPCEC, MAME, RetroArch-capable CPC emulators, and other
   maintained CPC/Plus emulators for:
   - lower ROM, BASIC/system cartridge, AMSDOS, and upper/expansion ROM terms;
   - visible slot inventories and unload/eject behavior;
   - Plus cartridge versus classic ROM-box priority;
   - model changes, hard reset, media persistence, and invalid combinations.
3. Decide whether classic CPC and Plus/GX4000 should remain one core, become
   separate distributed cores, or share one binary with one mutually exclusive
   machine selector.
4. Decide whether capability-invalid media entries should be hidden, disabled,
   or accepted but retained for a later model switch.
5. Define saved-state scope explicitly: model, CRTC, ROM inventory, mounted
   media, cartridge, Dandanator, MF2, and transient ASIC unlock/register state.

## Required state model

Before changing the OSD, draw one authoritative state machine covering:

- selected machine: CPC464, CPC664, CPC6128, 464+, 6128+, GX4000;
- built-in/onboard ROMs versus cartridge pages versus expansion ROM slots;
- `/EXP`, ROMDIS/RAMDIS priority, Dandanator, and MF2 availability by model;
- whether Plus machines expose a Plus-compatible expansion-ROM service at all,
  and how it arbitrates against cartridges, Dandanator, MF2, and `/EXP`;
- DSK/FDC and CDT/tape capabilities;
- SNA/CPR/model compatibility and the model selected after load;
- soft reset, hard reset, detach/eject, model switch, and settings restore.

The implementation must expose enough of this state in the OSD to explain which
firmware and media can currently answer the CPU. Do not solve ambiguity with
silent fallback to a classic ROM, a default model, or a default locale-like
choice.

The present RTL state is intentionally conservative, not a final product
decision: `Amstrad_motherboard` prevents the classic `ROMbank`/`rom_map` path
from answering in Plus mode, while MF2 and Dandanator retain their separate
overrides. Research must decide whether expansion ROMs are unsupported,
temporarily retained for a later classic-model switch, or reintroduced through
a Plus-specific arbitration path. Whichever policy wins must be visible in the
OSD and covered by an ownership test.

## Candidate UX direction to evaluate

- One `Machine` selector rather than independent classic and Plus selectors.
- A `Media` page whose entries are capability-aware.
- A `ROMs` page using Amstrad terms: lower/system ROM, upper ROM slots, system
  cartridge, and expansion devices.
- A visible loaded-state label plus `Eject`/`Clear slot`/`Clear all expansion
  ROMs` actions.
- Reset actions named by scope, for example `Reset machine`, `Reset and eject
  cartridge`, and `Restore default firmware`, only if those scopes are real.
- Preserve power-user filename automation, but make it inspectable and document
  it as an import convention rather than the primary mental model.

## Exit artifacts

1. Source-linked comparison table for MiSTer peer cores and maintained emulators.
2. Current and proposed state diagrams.
3. Capability matrix by machine model.
4. OSD wireframe and migration behavior for saved settings.
5. An implementation plan split into UI-only, state-model, and RTL ownership
   changes, each with classic non-regression and real-hardware acceptance gates.
