# Sonic GX Technical Reference & FPGA Emulation Guide

This documentation synthesizes the technical architecture, ASIC tricks, timing mechanisms, and hardware quirks used in the **Amstrad GX4000 / Plus** release of **Sonic the Hedgehog (Sonic GX)**, developed by Arnaud "norecess464" Storq and presented at the Benediction Coding Party #5 (October 2025).

The primary source document is `Sonic the Hedgehog (Technical-EN).pdf` (100-slide technical presentation). This reference suite is structured specifically for engineers and LLM agents working on FPGA core implementations (such as the MiSTer Amstrad core) and cycle-accurate emulators.

---

## Executive Summary

Sonic GX is one of the most advanced technical showcases ever developed for the Amstrad Plus ASIC architecture, pushing the machine far beyond its nominal design parameters while adhering strictly to a **64 KB RAM / 512 KB cartridge (CPR)** budget running at a locked **50 Hz**.

Key architectural achievements:
1. **Full-screen 50 Hz 8-way scrolling** in Mode 0 (up to 4px horizontal, 8px vertical per frame) consuming 40–50% of total frame CPU budget.
2. **3-Screen Split Display Engine** combining CRTC R12/R13 with ASIC `SPLT` (`&6801`) and `SSA` (`&6802/&6803`), dynamically reprogramming the split registers **mid-frame** via a DMA interrupt to achieve three independent VRAM scan windows on a single field.
3. **Hardware Interrupt Pipeline**: Strict utilization of Z80 **IM 2** with simultaneous interrupt arbitration exploiting the hardware priority order: `PRI -> DMA2 -> DMA1 -> DMA0`.
4. **Hardware Sprite Multiplexing**: Displays up to **40 simultaneous rings** on a 5×8 matrix using only 6 hardware sprites through real-time `DMA1` interrupt repositioning.
5. **Zero-Copy Sprite Rotations**: 6 preloaded rotation phases in ASIC sprite RAM (sprites 10–15) animated purely by ID swapping with zero CPU copy overhead.
6. **Compiled Tile Generation ("Compiled Sprites" for Map Tiles)**: Map tiles are compiled Z80 machine code streaming VRAM writes via stack pointer (`POP HL`) operations across 4 dedicated ROM banks, indexed via aligned `JP` jump tables.
7. **"Forget the VBL" Frame Lifecycle**: Frame processing is triggered by a line interrupt on the **last visible line**, exploiting overscan/blanking for critical state updates prior to visible line 0.

---

## Current source checks

The [2026-09-22 interrupt source comparison](../../plus/references/scrapes-interrupt-findings-2026-09-22.md)
corrects stale B19 guidance, A13 safe-zone applicability and SPLT wrap arithmetic.
Use its evidence boundaries before treating the hypotheses below as RTL defects.
The [cartridge and IRQ audit](irq-audit-2026-09-22.md) records the bounded
production-T80 evidence, harness correction and next checkpoint requirements.
The [hardware and DMA cadence investigation](hardware-loop-2026-09-22.md) records
matching defects on two timing-clean builds, the production-T80 PAUSE terminal-line
discrepancy, and an AmSpirit counterfactual. Hardware acceptance of the correction
remains separate from its simulation and review gates.

## Document Index

| Document | Focus Area | Key FPGA/Emulation Topics |
|---|---|---|
| [1. Memory Architecture](memory-and-banking.md) | 512 KB CPR layout & Z80 64 KB address space | 32 ROM banks, Lower/Upper ROM bank switching during VRAM drawing, ASIC page gating at `&4000-&7FFF`, shared `GameVars`/`BonusVars` RAM layout. |
| [2. Interrupts & Timing](interrupts-and-timing.md) | Interrupt pipeline, arbitration & line rasters | IM 2 vectoring, `PRI > DMA2 > DMA1 > DMA0` priority order, DMA interrupts outside visible screen, Plus Vectored Interrupt Bug safe zones, alternate register set (`EXX`) beam racing. |
| [3. Hardware Scrolling & Splits](scrolling-and-splits.md) | Dual-buffer 3-split display & compiled tiles | Dual 16 KB VRAM buffers (`&0000` / `&C000`), mid-frame `SPLT`/`SSA` reprogramming on DMA2, `SSCR` fine scroll, 34-row VRAM with 2 hidden guard rows, compiled tile streaming via stack (`POP HL`). |
| [4. Sprites & Multiplexing](sprites-and-multiplexing.md) | 16 ASIC sprites, ring matrix & PIX2 unpack | Fixed sprite assignment, 5×8 ring multiplexing via DMA1, PIX2 2:1 nibble packing with on-the-fly horizontal mirroring, shadowed sprite registers, **sprite RAM write contention white artifact**. |
| [5. FPGA Emulation & Audit Guide](fpga-emulation-guide.md) | RTL verification checklist for MiSTer core | Audit checklist against `rtl/plus/` (`asic_video.v`, `asic_regs.v`, `asic_dma.v`, `plus_sprite_ram.v`), testbench vector recommendations, and capture analysis. |

---

## Critical Silicon Quirks & Emulator Traps

When verifying an FPGA core or cycle-accurate emulator against Sonic GX, keep these critical findings in mind:

### 1. Sprite RAM Write Contention (Silicon Glitch)
* **Real Hardware Behavior**: Writing to ASIC sprite RAM (`&4000-&43FF`) while the raster beam's video generation logic is actively reading that same sprite produces visible **white pixel artifacts** on screen.
* **Sonic GX Engine Mitigation**: The engine carefully schedules sprite updates either before or after the raster beam passes, and streams dynamic sprites in an end-of-frame update queue.
* **Emulator Implication**: Almost no software emulator models this internal ASIC dual-port collision behavior. In our FPGA core (`rtl/plus/plus_sprite_ram.v`), M10K block RAM is configured with `OLD_DATA` read-during-write semantics, which avoids the white artifact but guarantees glitch-free rendering.

### 2. Simultaneous Interrupt Priority Arbitration
* **Real Hardware Behavior**: If multiple interrupt sources (PRI and DMA channels 0, 1, 2) coincide on the exact same scanline, the ASIC interrupt controller arbitrates in strict priority order:
  $$\text{PRI} \longrightarrow \text{DMA2} \longrightarrow \text{DMA1} \longrightarrow \text{DMA0}$$
* **Verification in Core**: Verified in `rtl/plus/asic_regs.v` lines 627–636 (`ack_src` selection logic).

### 3. Mid-Frame Dynamic `SPLT` and `SSA` Reprogramming
* **Real Hardware Behavior**: Sonic GX reprograms `SPLT` (`&6801`) and `SSA` (`&6802/&6803`) *mid-frame* from within a DMA2 interrupt handler to split the screen into 3 segments. The ASIC line comparator must match the new line target even if a split has already occurred earlier in the same frame.
* **Verification in Core**: Must confirm that `rtl/plus/asic_video.v` does not latch or freeze split state after the first match of a frame.

### 4. Plus Vectored Interrupt Bug Safe Execution Zones
* **Real Hardware Behavior**: Silicon defect during Z80 interrupt acknowledge cycle (`/IORQ` + `/M1`) when the ASIC supplies the interrupt vector byte.
* **Workaround Rule**: The instruction being interrupted must execute within specific 8 KB windows:
  $$\&2000-\&3FFF, \quad \&6000-\&7FFF, \quad \&A000-\&BFFF, \quad \&E000-\&FFFF$$
  Handler or vector-table placement does not establish immunity. Capture the interrupted
  instruction and acknowledge bus activity before applying this explanation to Sonic.
