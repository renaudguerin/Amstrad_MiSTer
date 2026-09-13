# Hardware Scrolling & Split-Screen Architecture

This document details the 50 Hz dual-buffer scrolling engine, the 3-screen split-screen pipeline combining CRTC R12/R13 with ASIC registers, sub-pixel soft scrolling (`SSCR`), and compiled tile rendering.

---

## Scrolling Engine Specifications

* **Target Resolution**: Mode 0 (160×200 nominal, extended to 256 visible scanlines = 32 character rows of 8 scanlines each).
* **Frame Rate**: Locked 50 Hz full screen.
* **Displacement Envelope**: Up to **4 pixels horizontal** (1 byte / 1 CRTC character) AND/OR **8 pixels vertical** (1 character row = 8 scanlines) per frame.
* **CPU Budget**: Consumes approximately **40–50% of the total Z80 frame time** when scrolling diagonally in both directions simultaneously.

---

## Dual 16 KB VRAM Buffers

The display layout is mapped across two independent 16 KB VRAM buffers in system RAM:

* **VRAM Buffer 0**: Physical RAM `&0000 - &3FFF` (16 character rows = 128 scanlines).
* **VRAM Buffer 1**: Physical RAM `&C000 - &FFFF` (16 character rows = 128 scanlines).
* **Total Display**: 32 character rows ($32 \times 8 = 256$ scanlines).

By default (unscrolled), the top half of the screen (16 rows) renders from Buffer 0, and the bottom half (16 rows) renders from Buffer 1.

---

## The 3-Screen Split Display Pipeline

When vertical scrolling is applied, circular address wrapping requires partitioning the visible field into up to **three distinct screen segments**, each displaying a slice of either Buffer 0 or Buffer 1.

For example, when scrolling upwards by 1 character row (8 scanlines):
1. **Top Segment**: 15 character rows from Buffer 0 (starting at row 1 instead of row 0).
2. **Middle Segment**: 16 character rows from Buffer 1 (the full buffer).
3. **Bottom Segment**: 1 character row from Buffer 0 (row 0, wrapped around to the bottom).

```
Scanline 0   +---------------------------------------+
             | Segment 1 (Top): 15 rows from Buf 0   | <- Set before line 0 via CRTC R12/R13
Scanline 119 +---------------------------------------+ <- Split 1 (SPLT = 119)
             | Segment 2 (Middle): 16 rows from Buf 1| <- Reloads SSA R12/R13 (#3000)
Scanline 247 +---------------------------------------+ <- Split 2 (SPLT rewritten to 255)
             | Segment 3 (Bottom): 1 row from Buf 0  | <- Reloads SSA R12/R13 (#0000)
Scanline 255 +---------------------------------------+
```

### Hardware Register Coordination Across the 3 Segments

| Segment | Timing / Trigger | Register Control Mechanism |
|---|---|---|
| **1. Top Segment** | Initialized during vertical blanking prior to scanline 0 | Written directly to CRTC registers **R12 / R13** |
| **2. Middle Segment** | Initialized during vertical blanking prior to scanline 0 | Programmed into ASIC **`SPLT`** (`&6801`) and ASIC **`SSA`** (`&6802/&6803`) |
| **3. Bottom Segment** | Triggered **mid-frame** via a dedicated **DMA2 interrupt** | The DMA2 ISR rewrites **`SPLT`** and **`SSA`** on the fly with the third segment's boundary and address |

### Split Table Entry Data Structure

The engine maintains a precalculated lookup table with 32 entries (one per character row displacement). The table entry for a 1-character vertical scroll (`SplitScreen1`), extracted from the source assembly, shows the exact encoding:

```z80
; SplitScreen1 definition:
    db (15 * 8) - 1       ; SPLT Page 1 comparison scanline (= line 119)
    db (32 * 8) - 1       ; SPLT Page 2 comparison scanline (= line 255)
    dw #0000 + (2 * R1)   ; CRTC R12/R13 for Page 0 (Buffer 0 offset)
    dw #3000 + (0 * R1)   ; SSA R12/R13 for Page 1 (Buffer 1 base: &C000 >> 2 = #3000)
    dw #0000 + (0 * R1)   ; SSA R12/R13 for Page 2 (Buffer 0 base: &0000 >> 2 = #0000)
```

> [!IMPORTANT]
> **CRTC Word Addressing**: The CRTC hardware addresses 16-bit video words where each unit corresponds to 4 bytes in RAM. Thus, physical address `&C000` is represented as `&C000 >> 2 = #3000`, and `&0000` is represented as `#0000`.
>
> **SPLT Scanline Value**: The ASIC `SPLT` register comparator matches against `{VC[4:0], RC[2:0]}`. The value written is the scanline count minus one.

---

## 34-Row Buffer with Hidden Guard Rows

While the displayed screen is 32 character rows (256 scanlines), the internal VRAM buffer allocates **34 character rows**:

* The **2 extra character rows** remain permanently off-screen outside the active CRTC display window.
* When Sonic moves vertically, the tile engine writes incoming landscape rows into these hidden guard rows.
* Because the drawing operations occur in non-displayed memory, **no tearing, flickering, or partial tile rendering is ever visible to the player**.

---

## Sub-Pixel Fine Scrolling via SSCR (`&6804`)

Character-level scrolling moves the screen in coarse 4-pixel (horizontal) and 8-pixel (vertical) steps. Smooth 50 Hz motion is achieved by combining character offsets with the ASIC **Soft Scroll Control Register (`SSCR`, `&6804`)**:

* **Horizontal Pixel Delay (`SSCR[3:0]`)**:
  * Values `0` to `7` introduce a sub-character delay of 0 to 7 half-pixels in Mode 0 (or 0 to 7 pixels in Mode 1).
  * Works in tandem with CRTC R12/R13 horizontal character offsets (`&000 - &3FF`).
* **Vertical Scanline Delay (`SSCR[6:4]`)**:
  * Values `0` to `7` delay the start of the vertical raster counter by 0 to 7 physical scanlines.
  * Works in tandem with the character-based vertical split table.
* **Border Masking (`SSCR[7]`)**:
  * Masks the first character column to suppress artifacts when scrolling horizontally.

---

## Interleaved VRAM Address Arithmetic

Because Amstrad CPC VRAM scanlines are interleaved in 2 KB steps (scanlines 0..7 of character row 0 are at `&0000, &0800, &1000, &1800, &2000, &2800, &3000, &3800`), advancing horizontally across character boundaries within a 16 KB buffer requires circular modular arithmetic:

```z80
    LD HL, &C000    ; Start of VRAM buffer
    INC L           ; Advance across 256-byte page boundary
    INC HL          ; Advance to next character cell (4 Mode 0 pixels)
    RES 3, H        ; Mask bit 11 to keep HL rolling inside the 16 KB window
```

---

## Compiled Tile Rendering ("Compiled Sprites" for Map Tiles)

Streaming new landscape tiles into VRAM during rapid scrolling is the engine's primary bottleneck. Standard data copying (`LDI` or `LD A,(HL) / LD (DE),A`) was too slow to maintain 50 Hz.

### The Compiled Code Approach
Tiles are not stored as raw pixel bitmap data. Instead, they are compiled into pure Z80 machine code consisting almost exclusively of direct memory writes:

```z80
; Disassembly of compiled tile code (Tileset0_Tile0_X0Y0, 45 bytes):
Tileset0_Tile0_X0Y0:
    POP HL          ; Fetch VRAM target address directly from stack (10 T-states!)
    LD (HL), D      ; Write pixel pair to scanline 0
    INC L           ; Advance to next byte
    LD (HL), D      ; Write second pixel pair
    SET 3, H        ; Advance to scanline 1 (&0800 offset)
    LD (HL), D
    DEC L
    LD (HL), D
    SET 4, H        ; Advance to scanline 2 (&1000 offset)
    ...
```

### Stack Pointer Exploitation
At the beginning of each frame, the engine sets the Z80 stack pointer `SP` to point to an array of precomputed VRAM target addresses. Each compiled tile begins with `POP HL`, pulling its destination address from the stack in a single 10 T-state opcode, completely eliminating pointer calculation inside the render loop.

### 4-ROM Quadrant Banking
* Each "Big Tile" ($8 \times 16$ pixels) is composed of $2 \times 2$ "Char Tiles" ($4 \times 8$ pixels each).
* The four char tiles forming a big tile are distributed across **four separate ROM banks**:
  * `X0Y0` in Upper ROM
  * `X1Y0` in Lower ROM
  * `X0Y1` in Lower ROM
  * `X1Y1` in Upper ROM
* When rendering a column or row, the appropriate Lower and Upper ROM banks are paged into `&0000-&3FFF` and `&C000-&FFFF`.

### Jump Table Alignment at Offset `&0000`
Because compiled tiles vary in byte length depending on pixel complexity (e.g., solid color runs vs complex textures), tiles cannot be indexed by simple multiplication. 

To maintain $O(1)$ dispatch:
* Offset `&0000` of every tile ROM contains a **jump table of 3-byte `JP nn` instructions**.
* Tile index $N$ is called via:
  $$\text{Target} = \text{ROM Base} + (N \times 3)$$
  providing constant-time entry into variable-length compiled code.
