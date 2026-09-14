# ASIC Hardware Sprites, Multiplexing & Pixel Formats

This document details the 16 hardware sprites subsystem, DMA1 raster multiplexing (up to 40 rings on screen), the PIX2 nibble compression format, and the **Sprite RAM write contention white artifact quirk**.

---

## Hardware Sprite Allocation (16 ASIC Sprites)

The Amstrad Plus ASIC provides 16 hardware sprites, each $16 \times 16$ pixels with 15 colors plus transparency (Pens 0..15). Sonic GX permanently maps the 16 hardware sprite slots as follows:

| Hardware Sprite Slot | Entity Assignment | Animation & Update Strategy |
|---|---|---|
| **0 + 1** | Sonic (32×16 compound sprite) | 50 Hz movement; pixel content refreshed at **25 Hz** (half a sprite per frame) |
| **2 + 3** | HUD Rings Counter | Refreshed on-demand only when surplus CPU cycles exist in the frame |
| **4 + 5** | HUD Lives Counter | Refreshed on-demand only when surplus CPU cycles exist in the frame |
| **6 + 7 + 8** | Dynamic Streaming Queue (Enemies, Platforms, Missiles) | Streamed from ROM via an update queue: **exactly 1 sprite updated per frame** |
| **9** | Ring Collected Animation | Preloaded explosion/sparkle phase |
| **10, 11, 12, 13, 14, 15** | Ring 6-Step Rotation Sequence | **Zero-copy animation**: Preloaded in ASIC RAM; animated purely by swapping sprite IDs |

---

## Sprite Movement vs. Content Refresh Throttling

To stay within the 50 Hz frame budget while animating complex scenes:

1. **Physical Motion at 50 Hz**: Every active sprite's X and Y screen coordinates are recalculated and applied every single frame (50 Hz), ensuring buttery-smooth visual motion.
2. **Pixel Content Update Throttling**:
   * **Sonic**: The character graphics are updated at **25 Hz** by refreshing half of Sonic's compound sprite per frame. Visual tests confirmed that at 50 Hz screen refresh, the human eye cannot perceive that the left and right halves are refreshed on alternating frames.
   * **HUD Sprites**: Only redrawn when a frame has surplus CPU time.
   * **Dynamic Entities**: Enemies, moving platforms, and projectiles are streamed dynamically based on Sonic's world coordinates. A FIFO update queue processes **at most one sprite copy per frame** at the very end of the frame cycle. If a copy cannot finish before the next frame boundary, it is cleanly deferred.

---

## Zero-Copy Ring Rotation Animation

Animating spinning rings typically requires copying new pixel data into sprite RAM every few frames. 

Sonic GX completely eliminates this CPU overhead:
* The **six rotation phases** of the ring are stored permanently in hardware sprite RAM slots **10 through 15**.
* To advance the rotation animation across the level, **no pixel memory is touched**. The engine simply swaps the hardware sprite IDs assigned to the ring coordinate slots.

---

## DMA1-Driven Ring Multiplexing (5×8 Matrix = 40 Rings)

To replicate the iconic fields of collectible rings from the Genesis original without exceeding the 16-sprite hardware limit, the engine implements a high-speed multiplexer driven by **DMA1 raster interrupts**:

* The screen is divided into a spatial grid of up to **5 columns and 8 rows** (up to 40 rings visible simultaneously).
* The 6 hardware ring sprites (slots 10–15) are dynamically reassigned as the video beam traverses the display:
  1. For each horizontal band of rings, a `DMA1` interrupt triggers just above the band.
  2. The ISR writes new Y coordinates, X coordinates, and enables to the ring sprite registers in ASIC RAM (`&4000-&407F`).
  3. The raster beam renders the rings for that band, after which the next DMA1 interrupt fires to reposition them for the row below.

```
Raster Beam
    |
    v   Line 40:  [DMA1 Interrupt] -> Reposition Sprites 10-14 for Row 0
        Line 48:  [Render Row 0: 5 Rings displayed]
    |
    v   Line 60:  [DMA1 Interrupt] -> Reposition Sprites 10-14 for Row 1
        Line 68:  [Render Row 1: 5 Rings displayed]
    |
    v   Line 80:  [DMA1 Interrupt] -> Reposition Sprites 10-14 for Row 2
        Line 88:  [Render Row 2: 5 Rings displayed]
```

### High-Speed Spatial Collision Detection: $O(1)$
Naively checking Sonic's bounding box against 40 individual rings would stall the Z80:
* The multiplexed field is stored as a compact **2D bitfield** (one byte per column, where bit $N=1$ indicates a ring is present in row $N$).
* Sonic's current world position is divided by the grid cell dimensions to identify his immediate grid coordinate.
* **Only the single ring occupying that specific grid cell is tested for collision**, reducing 40 bounding box tests to a single constant-time $O(1)$ calculation.

---

## PIX2 Sprite Compression & Real-Time Mirroring

### 2:1 Nibble Packing
In native Amstrad Plus ASIC sprite RAM, each $16 \times 16$ sprite occupies 256 bytes. Each byte stores a 4-bit palette index (0..15) in bits 0..3, while **bits 4..7 are completely unused and ignored by the hardware**.

Copying raw 256-byte sprites directly from cartridge ROM wastes half the cartridge capacity. Sonic GX introduces the **PIX2** packed format:
* **Storage in ROM**: Two 4-bit pixels are packed into each byte (128 bytes per 16×16 sprite).
* **Unpack Loop**: An unrolled Z80 assembly routine reads each packed byte, splits the high and low nibbles, and writes them into consecutive bytes in ASIC sprite RAM (`&4000 - &43FF`).
* **Performance vs ZX0**: For single-sprite streaming, PIX2 unpacks significantly faster than ZX0 decompression while achieving comparable compression ratios on 16-color sprite data.

### Real-Time Y-Axis Mirroring (Horizontal Flip)
Because the unpack routine writes bytes into ASIC RAM programmatically, it can write scanlines backwards with zero cycle penalty:
* Left-facing and right-facing versions of Sonic are not duplicated in ROM.
* Only the right-facing sprite is stored; when Sonic faces left, the PIX2 unpacker mirrors the pixel stream along the horizontal axis on the fly.

---

## Shadowed Sprite Registers Protocol

Writing sprite coordinates directly to ASIC registers during the middle of the frame can cause visual tearing or misaligned compound sprites if the beam passes mid-update.

* **RAM Shadow Buffers**: All physics, entity tracking, and camera logic write exclusively to shadow coordinate arrays in RAM (`GameVars`).
* **Blanking Flush**: The shadow coordinates are copied to the ASIC hardware registers in a single contiguous burst during Phase 1 (Frame Init), while the beam is off-screen.

### Off-Screen Debug Conventions
When an entity is inactive or off-screen, it is hidden by setting its X coordinate to negative values (`PosX = -80, -79, -78`) or $Y > 256$.

The engine assigns distinct negative values as semantic debug tags:
* `PosX = -80`: `HIDDEN_FROM_CAMERA_CULL`
* `PosX = -79`: `HIDDEN_FROM_ENTITY_DEATH`
* `PosX = -78`: `HIDDEN_FROM_LEVEL_BOUNDS`

When inspecting sprite RAM in an emulator or FPGA debugger, the developer can instantly diagnose why a sprite is disabled.

---

## Critical ASIC Quirk: Sprite RAM Write Contention White Artifact

> [!WARNING]
> **Real Silicon Behavior Discovered by Author**:
> If the Z80 writes to an ASIC sprite's pixel data memory (`&4000 - &43FF`) at the exact scanline moment the internal ASIC video generator is fetching that sprite's row data, **the video DAC outputs corrupted white pixels (`Pen 15 / 4'hF`) on screen**.

### Cause on Real Hardware
The Amstrad Plus ASIC ASIC integrates internal single-port / pseudo-dual-port RAM for the 16 sprites. Simultaneous read-access by the video serialization pipeline and write-access by the Z80 host bus produces a bus contention spike that pulls the internal data lines high, displaying a momentary flash of white pixels.

### Software Mitigation in Sonic GX
Arnaud Storq structured the sprite pipeline so that sprite pixel RAM is **never written on scanlines where that sprite is actively being rendered**. Dynamic sprite updates are deferred to the end of the frame or scheduled outside the active raster beam window.

### FPGA Emulation Analysis (MiSTer Core)
In our MiSTer FPGA implementation (`rtl/plus/plus_sprite_ram.v`), sprite RAM is mapped to Cyclone V M10K true dual-port block RAM:
* **Port A (Host)**: 12-bit CPU/SNA read/write port.
* **Port B (Video)**: 11-bit read-only video row-fetch port.
* **Mixed-Port Semantics**: Configured with `OLD_DATA` (read-first) read-during-write semantics.

Because FPGA M10K block RAM cleanly returns `OLD_DATA` rather than floating bus high on read/write collision, **the MiSTer core will render the sprite cleanly without the white glitch**. However, software developed or tested on emulators without this mitigation will show white artifacts when deployed to real Amstrad Plus hardware.
