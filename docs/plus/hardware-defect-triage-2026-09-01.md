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

Arnold V issue 1.5 §2.1 supplies the coordinate discriminator that the earlier
model lacked.  The 10-bit X field uses raw `0..767` for positions `0..767` and
raw `768..1023` for positions `-256..-1`; on a standard screen the on-screen
range at maximum magnification is `-64..639`, and a sprite wholly outside the
on-screen range is not displayed.  [KT] independently records the raw
`&000..&3FF` / sign-extended `&FF00..&02FF` range and the separate fact that an
on-screen sprite may repeat when CRTC R0 is greater than 64.

The prior engine instead compared every raw X value against the modular
horizontal counter.  The source-derived `s09` first failed unchanged RTL with:

```text
FAIL asic_sprites_test.cpp: s09: X=-256 leaked at hp=768
```

The smallest correction treats raw `768..1023` as negative, begins a
partially-left-clipped sprite at X=0 with the corresponding source-pixel
offset, and suppresses a sprite wholly left of the display instead of aliasing
it into the far end of the counter. Positive positions through 767 remain
available for non-standard widened displays; under standard timing the
border/display compositor hides positions beyond 639. It preserves [KT]'s
R0>64 repeat for a valid positive start; the existing `s10` remains green.
`s09` pins `-256`, `-8`, and the positive `767` boundary, including the rule
that `-8` displays source pixels 8..15 only at X=0..7 and does not alias near
counter positions 1016..1023.

The exact `burnin_rubber_sprite_on_the_right_should_be_hidden.png` and
`crtc3_demo_sprite_on_the_right_should_be_hidden.png` screenshots both show
partial sprite columns at the far right where the title expected the sprite
hidden. That observation
is consistent with the removed unsigned alias, but the screenshots do not
provide the programmed coordinate or prove title causality.  Hardware closure
therefore requires an exact-tip retest and, if either leak remains, a trace of
the sprite X/magnification register and compositor X at the first bad pixel.

### Independent classification of the other screenshots

Each image is classified by its visible failure shape, without using it as an
oracle for an undocumented rule:

| Screenshot | Observation and current boundary | Next discriminator |
|---|---|---|
| `Sonic.png` | Large horizontal discontinuities and repeated/relocated scene bands affect the main picture; this is not the localized far-right sprite-X signature above. No source-backed pointer, split, scroll, or CRTC rule selects a cause. | Capture the first bad scan line with CRTC MA/RA, R0/R1/R4/R5/R6/R9, SPLT/SSA/SSCR, video fetch address/data, and whether the ASIC is locked. |
| `copter_271_top_logo_lines_colour.png` | Only the top rows of the logo carry the conspicuous cyan/purple colour difference. The image does not establish whether the logo is sprite pixels, main-screen pixels, or whether a palette write occurred at that seam. | Identify the winning plane and sprite index for the first wrong pixel, then record its source row, palette entry/value, CPU sprite/palette access, and dot phase. |
| `crtc3_demo_graphics_corruption.png` | Narrow horizontally displaced/streaked fragments recur through an otherwise recognisable fixed picture. That is a video-address/line-phase symptom family, distinct from a single off-screen sprite, but the picture alone does not choose CRTC3 counting, scroll, fetch, or memory data. | Record the first corrupt line and first divergent fetch with MA/RA, CRTC counters/registers, SSCR/SPLT/SSA, SDRAM address/data, and displayed pixel phase. |
| `dick_tracy_top_lines_offset_x_position.png` | A small number of top picture rows are horizontally displaced while later rows align. This is a line-boundary X-origin symptom, not evidence that a sprite coordinate is wrong. | At the first displaced row, capture HSYNC/DE, CRTC C0/C4/C9 and registers, SSCR/SPLT/SSA, MA reload, and the first two video fetch addresses. |

No RTL change is made for those four families.  Their required discriminators
are deliberately different so a later hardware capture cannot silently turn
one screenshot into a blanket timing fix.

## OUT(C),r versus block-OUT timing evidence gap

Accuracy commit `2e1eb72` preserves the dated ACCC author response for
§§4.4.3–4: OUTD is intentional and the ASIC uses the same `/WAIT`
request/repetition mechanism as the Gate Array. That is correspondence rather
than a published-v1.11 change, as the author-feedback record explicitly notes.
The production Plus ASIC path receives CPU address,
data, I/O/read/write strobes, M1, and phase enables; it receives no opcode-class
signal.  Consistently, the production VHDL T80 implements both OUT(C),r and
OUTI/OUTD/OTIR/OTDR, and both present an ordinary I/O write bus cycle to that
path.

The Verilator TV80 surrogate used by the production-shaped Plus fixtures
implements OUT(C),r but has no block-I/O decode.  The existing R2.JIT
discriminator is a synthetic bus cycle, not an instruction-level comparison.
It can pin the ASIC-visible write phase, but it cannot prove that the surrogate
emits the same bus cadence for OUTD as the production VHDL core.  Because the
ASIC cannot distinguish the opcodes, no opcode-specific ASIC patch is justified.
Closure needs either a real-T80-capable bus trace comparing OUT(C),r with OUTD,
or a hardware capture of their address/data/IORQ/WR/WAIT and accepted-write
phases under the same CRTC phase.
