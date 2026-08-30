# Plus hardware test round 2 — 2026-08-30

This is the active handoff for the hardware observations supplied on 2026-08-30 and the
focused Plus follow-up. The observations were made on real MiSTer hardware, but the tested
RBF SHA, Plus model, mounted media, and exact cartridge hashes were not recorded. Treat them
as authoritative symptoms, not as proof that one source commit caused them.

The changes below are simulation-verified only until an exact-tip full-effort RBF is built
and the checklist is repeated. A passing fixture proves the checked RTL path; it is not a
claim about undocumented ASIC internals or a title's first divergence.

## Symptom disposition

| Hardware observation | Current disposition | Next discriminator |
|---|---|---|
| BASIC cartridges report `disc missing` or `read fail` | Not closed by the Plus-only delta. Integration `074c182` adds shared u765 reset/ACK quarantine, mount-retry retention, and CPC selected-write aliases; this Plus branch must include that integration before retesting. No bench yet boots BASIC against a mounted DSK. | 6128+ model, recorded known-good DSK, active FDC request, CPR reset/apply, delayed stale ACK, then a fresh real-controller access. |
| Switchblade remains black | Unexplained. No title-derived trace selected an MMU, CRTC3, PRI, sprite, or cartridge-timing rule. | Capture reset vector through the first stable loop or failed branch with PC, M1/MREQ/RD/WAIT, cartridge page, RMR2, ASIC lock, IRQ, and CRTC writes. |
| Sprite top-line flicker | Not claimed fixed. The new real-register/sprite fixture pins delayed read arbitration, stale completion rejection, re-demand, and the modeled per-access blanking tail. Those tests validate the current staging model, not the physical ASIC. | Record the first bad sprite row and the CPU pixel/register access that precedes it; compare against a same-cycle production trace. |
| Pang shows only the first `P`, reaches level select, then crashes | Unexplained as a title. CRTC3 interlace and sprite arbitration are stronger, but there is no Pang-derived first-divergence vector and no source basis to attribute either symptom. | Trace the logo transition and level-select crash separately; include IRQ/DMA state and cartridge WAIT cadence. |
| Plotting behaves as if Fire is held | The production-shaped PPI/YM2149/hid/joydb slice reads PS/2, SNAC, and USB Fire 1 with real ASIC CPU phases and READY stalls. It finds no constant-fire condition in that slice. This does not cover a title-specific joystick port, model selection, or DMA collision. | Capture Plotting's selected input row, AY R7/R14 state, both joystick sources, and model on hardware. |
| Arnold 5 keyboard is dead | Not closed. The same input fixture proves normal keyboard rows and Fire 1 through real PPI/YM/HID modules, but intentionally excludes DMA ownership and does not establish Arnold's control-write sequence. | Trace Arnold's PPI control words, AY register selection, keyboard row, and concurrent DMA PSG activity on 6128+ or 464+. |
| Copter 271 logo has wrong-colour top rows | Not claimed fixed. The symptom is compatible with a sprite row/palette seam, but no cartridge-derived vector identifies one. | Record whether the logo is sprites, the affected sprite/Y rows, palette writes, and the first wrong pixel phase. |
| CRTC3 demo detects an emulator, has wrong DMA sample pitch, fixed-picture corruption, then crashes | Two source-backed mechanisms changed: R8=3 interlace/IVM counting and VSYNC timing are implemented, and inactive DMA slots no longer consume fetch or execute cycles. These are direct retest candidates for detection and pitch. R8=1 sync-only interlace, odd-frame R5 recurrence, the picture corruption, and the eventual crash remain open. | Retest each phase separately. Record the first discriminator result, DMA channel mask/sample, and the first corrupted line before the crash. |
| Reset or CPR reload sometimes cannot recover the core | Only partial mechanisms are addressed. The Plus SNA FIFO now reserves the checked two-byte HPS tail, while the shared FDC stream is hardening reset during an active SD request. Neither proves the reported whole-core wedge. | When wedged, record whether CPU, cartridge service, u765 SD request/ACK, snapshot busy, and video counters still advance before reloading the core. |
| Burnin' Rubber initially reveals columns of a sprite that should be off-screen | Not claimed fixed. The sprite fixtures cover register mapping, fetch arbitration, access blanking, and live updates, but no sourced coordinate change was made. | Capture sprite X/Y/magnification and the compositor pixel position before the word enters from the right. Keep the unresolved coordinate formula as hardware debt. |

## Implemented and focused evidence

- DMA cadence issue: inactive channel slots are skipped in both phases, so each active channel
  receives one fetch and one channel-ordered execute slot per DMA scan. All eight channel
  masks are pinned; LOAD retains its fixed eight-cycle execute sequence. This is the only
  change in this round that directly targets the reported sample-pitch symptom.
- CRTC3 R8=3: the counter now implements IVM `+2` counting, parity seeding/toggling,
  adjustment behavior, even-frame added line, MID-VSYNC, odd-frame delay, R7=0 priority,
  live R0/R3/R8 handling, reset, exit, DE, ADJ, and VMA consumers. ACCC v1.11 sections
  19.3-19.8 are the rule source. R8=1 and odd-frame R5 recurrence remain explicit residuals.
- CPC+ SNA: the FIFO wait watermark moved from four to three entries. Checked production
  timing permits at most two already-admitted payload bytes after wait becomes visible, so
  three queued bytes plus two payload bytes, each expanding to at most two writes, cannot
  overrun the eight-entry queue. The local test injects that maximum; it does not elaborate
  `hps_io` and `Amstrad.sv` together.
- Input path: the fixture elaborates the real `i8255`, `YM2149`, `hid`, `joydb`, and
  `asic_ga_timing` modules. It executes explicit `0x82`, AY R7/R14, `0x9B`, Port C row
  selection, PS/2 A, SNAC Fire 1, and USB Fire 1 operations. It mirrors both production
  joystick fallbacks and is included in the default test and lint gates. It is a minimal
  exact slice because Verilator cannot parse the production motherboard's Verilog-only
  `.do` pin and the SystemVerilog YM/HID sources in one language mode.
- Sprite path: focused fixtures exercise all-16 live programming cadence and the real
  `asic_regs` to `asic_sprites` arbitration seam. CPU access holds grants off, preserves
  request/address, rejects a stale delayed completion, re-demands the row, blanks the
  staged visible sprite through the access tail, and restores it afterward. Physical ASIC
  bandwidth and the access-hole duration remain hardware assumptions.
- Cartridge timing: the production CPU/SDRAM harness now runs a sustained cartridge-resident
  NOP/JP chain. Its deterministic 4,096-tick window completes 38 M1 fetch phases and 73
  physical cartridge reads, with 803 CPU WAIT-low ticks and a maximum 11-tick stall run.
  The harness's ordinary-RAM side hardwires `no_wait` and does not provide a valid like-for-like
  SDRAM execution comparison, so this is a pinned baseline rather than evidence for a cache or
  normal-slot redesign. A title or hardware trace must still prove the legal WAIT pacing harmful.

## Review and confidence boundary

A fresh Sol review found no High production RTL defect in the DMA or CRTC3 changes. It
identified three integration gaps: the input fixture was absent from lint, the all-16 sprite
test did not actually overlap all sprites at one deadline, and durable status was stale. The
lint gap and exact second-joystick fallback are fixed. The revised `s17` now overlaps all 16
sprites in one early window and observes a unique emitted marker from each. This document is
the durable symptom/status reconciliation; the shared handoff documents are reconciled after
rebasing onto the accuracy/FDC integration.

A fresh exact-tip Sol remediation review returned CLEAR after independently reproducing the
focused fixtures. A guarded Claude review found one Medium DMA scheduler defect: the first
change skipped inactive fetch states but inactive lower-numbered channels still delayed the
execute phase. The new `d13` discriminator failed unchanged RTL (channel 1 alone began at CCLK
3 instead of 2); direct active-channel routing now passes every enable mask while preserving
LOAD's eight-cycle sequence. Claude's narrow remediation review accepted the production
routing and found one Low test gap: partial masks did not pin channel identity. `d13` now also
requires each distinct PSG register/data pair in ascending active-channel order, and the final
narrow review returned CLEAR at `d17a1bc`. This is review evidence, not synthesis or hardware
confirmation.

## Required hardware rerun

Use one exact full-effort RBF and record its integration SHA, RBF SHA-256, Quartus timing,
Plus model, mounted media, cartridge filename/hash, and cold-versus-reload start for every
row. Prioritize:

1. CRTC3 demo detection, DMA samples, fixed picture, and soak-to-crash.
2. BASIC/System cartridge with a known-good DSK after the shared FDC stream is integrated.
3. Plotting and Arnold 5 with input/DMA traces.
4. Pang and Switchblade with first-divergence CPU/cartridge traces.
5. Copter 271, the generic sprite top-line flicker, and Burnin' Rubber with sprite register,
   fetch, palette, and compositor traces.
