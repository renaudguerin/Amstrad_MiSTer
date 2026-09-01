# Plus ASIC documentation and ownership gap map

This is backlog B5's implementation-facing inventory. It answers four questions for every
register family and externally visible ASIC behavior described by the Arnold V specification
and the local CPCWiki reference snapshots: where the rule comes from, which RTL owns it, which
deterministic fixture can currently object, and what remains unknown.

The source files under `docs/references/` are user-owned inputs. This document records their
claims but does not make those files tracked. `docs/plus/references/asic-reference.md` remains
the detailed, citation-bearing rule digest; this file is the coverage and routing index.

Status vocabulary:

- **owned** — production RTL and a deterministic fixture exist;
- **partial** — the common behavior is implemented, but a sourced variant or production seam
  is not closed;
- **intentional omission** — the source describes real-hardware failure or protection that the
  core deliberately does not reproduce;
- **unowned** — no production mechanism can currently produce the sourced behavior.

## Register and memory-page inventory

| Register / range | Documented behavior | Production owner | Deterministic evidence | Status / gap |
|---|---|---|---|---|
| Lock sequence on `&BCxx` | Power-on locked; sequence selects lock/unlock; trailing `&EE` unnecessary | `rtl/plus/asic_unlock.v`, authoritative instance in `rtl/plus/plus_mmu.v` | `sim/plus/asic_unlock_test.cpp`, `sim/plus/plus_mmu_test.cpp` | **owned**; the source disagreement about the lock byte is resolved to the CPCWiki/hardware reading and kept explicit in the reference digest. |
| RMR2 on `&7Fxx`, pattern `101xxxxx` | Low-ROM position, ASIC-page enable, cartridge page; aliases MRER while locked | `rtl/plus/plus_mmu.v`, `rtl/plus/asic_ga_timing.v`, `rtl/Amstrad_motherboard.v` | `sim/plus/plus_mmu_test.cpp`, `sim/plus/asic_ga_timing_diff_test.cpp`, motherboard m7/m9 cases | **owned** for the internal machine. External expansion priority is separate below. |
| `&4000-&4FFF` sprite pixels | 16 × 256 low-nibble pixels, read/write, access blanks only the addressed sprite | `rtl/plus/plus_sprite_ram.v`, `rtl/plus/asic_regs.v`, `rtl/plus/asic_sprites.v` | `sim/plus/plus_sprite_ram_test.cpp`, `sim/plus/asic_regs_test.cpp`, `sim/plus/asic_sprites_test.cpp`, P4 production seam | **partial**: storage, readback, write-through coherence and per-sprite suppression are pinned; the exact real-ASIC blanking-hole duration is unmeasured (`blank_cnt=2` is a named model assumption). |
| `&5000-&5FFF`, `&6080-&63FF`, `&6440-&67FF`, `&6810-&7FFF` | Invalid/unmapped; reads expose the open instruction bus and writes do nothing | `rtl/plus/asic_regs.v` plus motherboard wired-AND bus | `sim/plus/asic_regs_test.cpp`, motherboard m9 | **owned** in the reduced production path; full-top third-party bus contributors remain outside those fixtures. |
| `&6000+8n` / `&6002+8n` X/Y | 10-bit X and 9-bit Y, byte readback rules, signed/offscreen geometry | `rtl/plus/asic_regs.v`, `rtl/plus/asic_sprites.v` | `sim/plus/asic_regs_test.cpp`, `sim/plus/asic_sprites_test.cpp`, `sim/plus/p1_mobo_bench_test.cpp` m13 | **owned** for the documented field widths and production origin; Arnold's printed max-magnification `-64` left edge conflicts with the derived one-dot overlap at `-63`, so hardware remains the geometry oracle. |
| `&6004+8n` and write mirrors | Magnification and X/Y read mirrors | `rtl/plus/asic_regs.v` | `sim/plus/asic_regs_test.cpp`, `sim/plus/p4_sprites_regs_test.cpp` | **partial**: offsets `+4..+7` are modeled as magnification writes; the Arnold-revision claim that `+3` also writes magnification conflicts with the Y-high register and is not emulated without hardware evidence. |
| `&6400-&643F` palette | 16 screen pens, border, 15 sprite colours; 12-bit `G,R,B`; legacy PENR/INKR translation | `rtl/plus/asic_regs.v`, `rtl/plus/asic_video.v`, `rtl/Amstrad_motherboard.v` | `sim/plus/asic_regs_test.cpp`, `sim/plus/asic_video_test.cpp`, P4 and motherboard palette seams | **owned**; two sequential byte writes naturally expose the intermediate colour, but there is no dedicated assertion for that analog-visible interval. |
| `&6800` PRI | Programmable raster line; zero selects classic 52-line source; line aliases above 255 | `rtl/plus/asic_regs.v`, `rtl/plus/asic_ga_timing.v` | `sim/plus/asic_pri_test.cpp`, `sim/plus/asic_ga_timing_diff_test.cpp`, `sim/plus/asic_regs_test.cpp` | **partial**: counter, arbitration and interrupt path are owned; the source conflict between a 6 µs and approximately 10 µs fire offset remains a hardware discriminator. |
| `&6801` SPLT | Eight-bit line compare; zero disables; split captures SSA at the display seam | `rtl/plus/asic_regs.v`, `rtl/plus/asic_video.v` | `sim/plus/asic_video_test.cpp` t08 family, motherboard m10 | **partial** only for the pathological 312-line wrap value: Arnold-revision says 55, KT says 56. Ordinary split capture and 8-bit aliasing are owned. |
| `&6802-&6803` SSA | Secondary display start address, high byte then low byte | `rtl/plus/asic_regs.v`, `rtl/plus/asic_video.v` | `sim/plus/asic_regs_test.cpp`, `sim/plus/asic_video_test.cpp` t08 family, motherboard m10 | **owned**. |
| `&6804` SSCR | Horizontal 0–15-dot delay, vertical raster offset, first-character border mask; sprites are unscrolled | `rtl/plus/asic_regs.v`, `rtl/plus/asic_video.v`, `rtl/plus/asic_sprites.v` | `sim/plus/asic_video_test.cpp` t08 family, motherboard m10/m13 | **partial**: the compositor seam is pinned, but the two source formulations for vertical behavior diverge when R9 is greater than 7 (low-three-bit addition versus a wider effective-raster reading). |
| `&6805` IVR | Interrupt vector high bits, source low bits, bit-0 auto-clear behavior | `rtl/plus/asic_regs.v`, `rtl/plus/asic_ga_timing.v`, motherboard CPU vector mux | `sim/plus/asic_regs_test.cpp`, `sim/plus/asic_pri_test.cpp`, motherboard m11 | **owned** for the selected non-corrupting model. The real A13-dependent IM2 corruption is intentionally omitted below. |
| `&6806-&6807` | Writes ignored; reads open bus | `rtl/plus/asic_regs.v` | `sim/plus/asic_regs_test.cpp` | **owned**. |
| `&6808-&680F` ADC0–7 | Eight 6-bit channels, four physically connected; the measured 464+ no-device default differs from GX4000 wiring | `rtl/plus/asic_regs.v` | `sim/plus/asic_regs_test.cpp` a11 | **partial / unowned input**: the 464+ default table is pinned globally, but no model-specific wiring or host paddle/analog source updates the registers at the documented approximately 200 Hz. |
| `&6C00-&6C0A` SAR/PPR | Three DMA source pointers and pause prescalers; write-only | `rtl/plus/asic_regs.v`, `rtl/plus/asic_dma.v` | `sim/plus/asic_regs_test.cpp`, `sim/plus/asic_dma_test.cpp` | **owned**. |
| `&6C0F` and reads across `&6C00-&6C0F` | DCSR enables, DMA INT flags/W1C, last-raster source, whole-range read mirror | `rtl/plus/asic_regs.v`, `rtl/plus/asic_dma.v`, `rtl/plus/asic_ga_timing.v` | register, DMA, PRI, and motherboard m11 fixtures | **owned**. |
| CRTC R0–R18 and modulo-8 read groups | Type-3 counters, programmable syncs, R12–R15/read statuses, read-only light pen | `rtl/plus/asic_video.v` | `sim/plus/asic_video_test.cpp`, `sim/plus/p1_video_test_top.v`, motherboard P1/P5 cases | **partial**: timing and read matrix are owned; R16/R17 have storage/readback but no light-pen strobe source, so they stay at reset and constitute gap G1. |

## Behavior inventory outside the register page

| Behavior | Production owner | Deterministic evidence | Status / gap |
|---|---|---|---|
| Sprite priority, transparency, magnification, repeat for R0>64, offscreen clipping and display-origin alignment | `rtl/plus/asic_sprites.v`, `rtl/plus/asic_video.v` | sprite unit suite, P4 real-register fixture, motherboard m13 | **owned** as a source/model contract; title and hardware confirmation remain open. |
| DMA instruction words: LOAD, PAUSE, REPEAT, reserved `&3xxx`, NOP, LOOP, INT, STOP | `rtl/plus/asic_dma.v` | `sim/plus/asic_dma_test.cpp` | **owned**, including the documented/observed PAUSE-then-REPEAT interpretation of `&3xxx`. |
| DMA line scheduling and PPI/PSG contention (`8/+1/+2` LOAD duration, WAIT and state restore) | `rtl/plus/asic_dma.v`, `rtl/Amstrad_motherboard.v` | DMA unit suite, `p10_dma_ppi`, real motherboard `p10_dma_mobo` | **owned** for the implemented cadence. Whether DMA RAM fetches ever stall the Z80 is absent from the sources and remains G6. |
| Cartridge paging, `/EXP`, ROM-select 0/7 model rule, reset boot and CPR atomic replacement | `rtl/plus/plus_mmu.v`, `rtl/plus/plus_cartridge_memory.v`, `rtl/plus/plus_cpr_parser.v`, `Amstrad.sv` | MMU, cartridge-memory, parser, P0/P10 production boot suites | **owned**; expansion ROM priority is only modeled for the core's existing expansion services, not an arbitrary physical expansion bus. |
| ACID protection/check failure | none by design | CPR/parser/boot fixtures prove omission does not block a cartridge | **intentional omission**: the ASIC-side failure window is undocumented and emulators omit the protection check. |
| Model capabilities: RAM size, FDC, tape, keyboard/joypad matrix, ROM page 7 rule | `rtl/plus/plus_model_select.v`, motherboard gates, `plus_mmu` | model-select, P8, P10 input/boot/FDC fixtures | **partial**: internal selectors and principal gates are owned; full-top FDC/tape and real keyboard/title behavior remain validation boundaries. |
| Plus PPI quirks and DMA arbitration | `rtl/i8255.v`, `rtl/Amstrad_motherboard.v` | P8/P10 input and DMA motherboard suites | **owned** for port direction/latch and live-input rules. Printer BUSY's approximately 500 Hz sampling behavior has no owner or test (G7). |
| GA/CRTC `IN`-performs-write traps and open instruction bus | `rtl/plus/asic_ga_timing.v`, `rtl/plus/asic_video.v`, `rtl/plus/asic_regs.v`, motherboard mux | GA differential suite, P5/P8 register and motherboard cases | **owned** in the instantiated production path. |
| Undefined power-on sprite, palette, position, SSA and IVR fields | explicit zero/reset initialization in the relevant Plus modules | reset cases across the register, video and sprite suites | **intentional deterministic approximation**: FPGA/simulation state is normalized to zero where the physical source says undefined; defined POR bits retain their specified values. |
| Plus colour-change phase and GX4000 39.90257 MHz master clock | shared 16/64 MHz core cadence and model-independent video path | pixel-phase/differential fixtures cover the chosen common cadence | **intentional approximation**: the documented approximately 0.5 µs Plus colour delay is modeled where the ASIC pipeline requires it; the GX4000 0.25% clock difference is deliberately normalized/ignored. |
| External expansion ASIC-page write-through bug | no generic external RAM expansion owner | none | **unowned / intentional pending evidence** (G4): internal 6128+ RAM must not show the bug; arbitrary external expansion RAM is not modeled. |
| RAM refresh-loss hardware failure and destructive HSYNC/DMA contention | none | none | **intentional omission**: do not emulate physical RAM decay or silicon damage. |

## Actionable gaps, ordered by value

1. **G1 — CRTC3 light pen R16/R17 input is unowned.** This is the only ordinary documented
   register function with storage/readback but no producer. First establish whether any MiSTer
   input or CPC software in scope can assert LPSTB; if yes, add the strobe at the production
   CRTC3 boundary and a cross-module capture/readback test. Do not invent a free-running value.
2. **G2 — ADC registers are fixed defaults, not analog inputs.** Route an existing MiSTer
   paddle/joystick analog surface only after deciding the user-facing control and update rate.
   This is an integration/UI task, not an `asic_regs` arithmetic fix.
3. **G3 — exact sprite access-blanking duration is unknown.** The current two-dot tail is a
   named model assumption and the source only says approximately one byte/1 µs. A hardware
   capture or Longshot/author clarification must precede an RTL timing change.
4. **G4 — sprite `+3` write mirror and external expansion double-write are evidence-gated.**
   Both depend on conflicting or absent hardware evidence; keep the present conservative
   behavior until a discriminator exists.
5. **G5 — PRI offset, SPLT wrap and SSCR R9>7 are source conflicts.** These are small, focused
   hardware discriminators. Do not turn either written source into a synthetic oracle.
6. **G6 — DMA RAM-fetch CPU stalls are undocumented.** The implemented PPI/PSG WAIT path is
   sourced and tested; no extra Z80 wait should be added without bus evidence.
7. **G7 — printer BUSY slow sampling has no owner.** Low priority because no current title
   symptom points at printing, but it is a real Plus compatibility gap rather than a test gap.

The map changes B6's architect brief materially: most ASIC features do have an RTL owner and
deterministic fixture. The architect pass should concentrate on the seven gaps above, the
always-both-live classic/Plus structure, menu reachability, and B1's rejected blanking owner;
it should not reopen the already-owned register page wholesale.
