# Interrupt Architecture & Timing Pipeline

This document details the interrupt subsystem, hardware priority arbitration, scanline rasters, and the 50 Hz frame lifecycle used in Sonic GX.

---

## Interrupt Mode 2 (IM 2) Justification

On classic Amstrad CPC systems, **Interrupt Mode 1 (IM 1)** is standard. However, IM 1 forces the CPU to jump to a fixed restart vector at address `&0038`.

In Sonic GX, physical address `&0038` falls directly within **VRAM Buffer 0** (`&0000 - &3FFF`). Storing an interrupt dispatch handler or vector table at `&0038` would cause the video gate array to render that handler's opcodes as visible on-screen pixels in the top-left corner of the screen.

Therefore, the engine strictly uses **Interrupt Mode 2 (IM 2)**:
* The Z80 `I` register points to an aligned 256-byte vector table located safely in system RAM (`&A000 - &BFFF`).
* When the ASIC asserts `/INT` and the CPU acknowledges with `/IORQ` + `/M1`, the ASIC places an interrupt vector byte on the data bus.
* The CPU combines `I` with the vector byte to form the 16-bit address of the specific ISR jump vector.

---

## 4 Interrupt Tasks per Frame

The engine relies on four distinct interrupt triggers during each 1/50th-second frame:

1. **Background Sky Rasters (`PRI`)**: Updates palette Pen 0 line-by-line to create the animated gradient sky.
2. **ASIC Screen Splits (`SPLT` / `SSA`)**: Re-arms the split screen address registers mid-frame for the lower third of the display.
3. **Multiplexed Rings (`DMA1`)**: Repositions the 6 hardware ring sprites across successive vertical bands to display up to 40 rings on screen.
4. **End of Visible Screen / Frame Init (`DMA0` / `PRI`)**: Fires immediately as the beam leaves the active display area to launch the next frame's initialization routines.

---

## DMA vs. PRI Interrupt Capabilities

The Amstrad Plus ASIC provides two distinct interrupt mechanisms:

| Feature | Programmable Raster Interrupt (`PRI`, `&6800`) | DMA Sound Engine Interrupts (`DMA0`, `DMA1`, `DMA2`) |
|---|---|---|
| **Trigger Source** | ASIC internal scanline counter comparator | Execution of an `INT` command in a DMA channel microprogram |
| **Placement Boundary** | Current core compares the raw nine-bit CRTC line value with `{0,PRI}` and excludes vertical adjustment; this is not a display-enable test. No-wrap is the accepted policy, supported by the Copter 271 device result; contrary scrape claims do not reopen it | **Unrestricted**: Can trigger anywhere, including top/bottom borders, overscan, and blanking intervals |
| **Typical Use in Sonic** | Visible-area background color rasters | Mid-screen split reloads, sprite multiplexing, and blanking frame-start triggers |

### Coincident Interrupt Hardware Priority Arbitration

If multiple interrupt conditions occur on the exact same scanline, the ASIC internal interrupt controller arbitrates the request with a strict, non-programmable hardware priority:

$$\mathbf{PRI} \quad > \quad \mathbf{DMA2} \quad > \quad \mathbf{DMA1} \quad > \quad \mathbf{DMA0}$$

* If all four fire simultaneously, `PRI` is acknowledged first.
* Upon return from the `PRI` handler, `DMA2` is serviced next.
* Then `DMA1` is serviced.
* Finally `DMA0` is serviced.

#### Hardware Core Verification
In the MiSTer core (`rtl/plus/asic_regs.v`, `ack_src` block), this priority is sampled on the first clock edge observing active interrupt acknowledge (`intack && !intack_d`), and held for that acknowledge:
```verilog
if (int_pending)
    ack_src <= 3'b110;  // PRI (raster interrupt) -> Highest priority
else if (dcsr_flags[2])
    ack_src <= 3'b000;  // DMA2 -> Second priority
else if (dcsr_flags[1])
    ack_src <= 3'b010;  // DMA1 -> Third priority
else if (dcsr_flags[0])
    ack_src <= 3'b100;  // DMA0 -> Lowest priority
```
Sonic GX's DMA channel assignment depends directly on this hardware priority:
* **`DMA2`**: Handles audio playback before the screen becomes visible, and triggers the **Second Split** during the visible screen.
* **`DMA1`**: Handles **Sprite Multiplexing (Rings)**, requiring lower latency than `DMA0` but deferring to screen splits.
* **`DMA0`**: Signals the **End of Visible Screen**.

---

## Background Rasters with PRI: The `EXX` Technique

To generate the smooth, multi-step blue sky gradient behind Green Hill Zone without eating into gameplay CPU time, the engine uses a technique credited to Overflow (Logon System):

1. Standard Z80 registers are preserved across interrupts without expensive `PUSH`/`POP` sequences by dedicating the **Z80 alternate register set (`EXX`)** exclusively to the raster interrupt handler.
2. Register assignments in the alternate set:
   * `HL'`: Kept permanently pointing to ASIC Palette Pen 0 (`&6400`).
   * `C'`: Holds the current PRI scanline comparison index.
   * `B'`, `DE'`: Point to color gradient tables and step counters.
3. Because the alternate registers are already primed, the ISR needs only execute:
   ```z80
   EXX
   INC C                    ; Next raster line
   LD A, C
   LD (&6800), A            ; Update PRI register
   LD A, (DE)               ; Fetch next 12-bit color byte
   INC DE
   LD (HL), A               ; Update Pen 0 color in left border
   EXX
   EI
   RETI
   ```
4. This keeps the raster handler execution window within the horizontal blanking / left border window, preventing pixel distortion on active display lines.

---

## The "Plus Vectored Interrupt Bug" Safe Zones

The captured CPCWiki *Plus Vectored Interrupt Bug*, PDF pp.3–4, describes a
board-level `/IORQ` timing issue involving A13 and two acknowledges seen by the
ASIC. Its workaround constrains the **instruction being interrupted** to A13=1
(`&2000–&3FFF`, `&6000–&7FFF`, `&A000–&BFFF`, `&E000–&FFFF`). The I register,
vector table and handler addresses are explicitly not the deciding addresses.
The French *Modes et fonctionnements…*, p.5, corroborates that distinction.

The reported placement of Sonic's handlers/table in `&A000–&BFFF` therefore does
not prove immunity. Establish the interrupted PC and instruction/bus activity
at each problematic acknowledge. Current RTL does not reproduce the described
A13-conditioned doubled acknowledge; its no-pending source fallback is DMA2
(code 0), whereas the article reports DMA0 (code 4) after the spurious second
acknowledge. This is a source/model difference, not an established Sonic failure.
See the [source comparison and discriminators](../../plus/references/scrapes-interrupt-findings-2026-09-22.md).

---

## Frame Lifecycle: "Forget the VBL"

Classic CPC software can poll raw CRTC VSYNC through PPI port B bit 0, then use Gate Array interrupts to refine synchronization. The regular Gate Array interrupt is not a dedicated vertical-blank interrupt.

In Sonic GX, waiting for VBL is abandoned:
* **The frame lifecycle starts on the last visible line** via an interrupt.
* By starting the frame loop as soon as the last scanline of pixel data leaves the screen, the Z80 gains access to the entire **bottom border, vertical sync, and top border (overscan)** period before line 0 of the next visible frame is drawn.
* All critical display state—including CRTC R12/R13 starting pointers, initial ASIC `SPLT` and `SSA` values, palette cycling, and shadowed hardware sprite coordinates—is updated during this off-screen window, guaranteeing tearing-free rendering.

---

## The 5-Phase 50 Hz Game Loop

The entire engine executes within a locked 50 Hz frame cycle (20 ms / 80,000 Z80 clock cycles):

```
+------------------------------------------------------------------+
| Phase 1: Frame Init (During Vertical Blanking)                   |
| - Disable interrupts                                             |
| - Fill Audio DMA circular buffer (music + SFX)                   |
| - Program ASIC frame registers (R12/R13, SPLT, SSA, DMA2_SPLT)   |
| - Prepare stack-based column/row tile drawing pointers           |
| - Flush shadowed sprite coordinates into ASIC RAM                |
| - Enable interrupts                                              |
+------------------------------------------------------------------+
                               |
                               v
+------------------------------------------------------------------+
| Phase 2: High-Level Entity Managers                              |
| - Process active Scripted Event sequence (1 action per frame)    |
| - Check spatial Trigger Boxes (Eggman, checkpoints, animal box)  |
| - Update Map Sprites group positions                             |
+------------------------------------------------------------------+
                               |
                               v
+------------------------------------------------------------------+
| Phase 3: Hardware Scrolling Update (40-50% CPU frame budget)     |
| - Draw new VRAM column AND/OR row using compiled Z80 code        |
| - Compute viewport displacement deltas for next frame            |
+------------------------------------------------------------------+
                               |
                               v
+------------------------------------------------------------------+
| Phase 4: Gameplay Physics & Collision Logic                      |
| - Sample digital controller inputs                               |
| - Update Sonic world coordinates (slopes, loops, acceleration)   |
| - Terrain elevation collision check (1D heightmap + masks)       |
| - Enemy and moving platform bounding box collisions              |
| - Smooth camera tracking window calculations                     |
+------------------------------------------------------------------+
                               |
                               v
+------------------------------------------------------------------+
| Phase 5: Sprite Management & Dynamic Streaming                   |
| - Compute shadowed screen coordinates for all active entities    |
| - Multiplexed ring array visibility update                       |
| - Dynamic sprite streaming queue (1 enemy/missile sprite/frame)  |
| - Sonic character animation sprite refresh                       |
+------------------------------------------------------------------+
```
