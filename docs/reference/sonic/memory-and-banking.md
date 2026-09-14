# Memory Organization & ROM Banking

This document details the cartridge banking architecture, memory mapping across the Z80 64 KB address space, and RAM allocation strategies used in Sonic GX.

---

## 512 KB Cartridge (CPR) Bank Architecture

Sonic GX targets a **512 KB Amstrad Plus cartridge (CPR format)**, mapped into **32 banks of 16 KB each** (ROM banks `00` through `31`).

The complete cartridge memory map, extracted directly from the author's production build pipeline (`memmap.txt`), reveals the exact functional allocation of all 32 banks:

| Bank # | Bank Label | Description & Embedded Payload |
|---|---|---|
| **00** | `ROM_Tileset0_X0Y1` | Compiled Z80 tile code: quadrant X0Y1 for Tileset 0 (`BigIndices0/1` + `BigTileset0`) |
| **01** | `ROM_Tileset0_X1Y0` | Compiled Z80 tile code: quadrant X1Y0 for Tileset 0 (`Tileset0_TileOpcodeOffsets` + `BigTileset1`) |
| **02** | `ROM_Tileset1_X0Y1` | Compiled Z80 tile code: quadrant X0Y1 for Tileset 1 (`BigIndices2/3`) |
| **03** | `ROM_Tileset1_X1Y0` | Compiled Z80 tile code: quadrant X1Y0 for Tileset 1 (`Tileset1_TileOpcodeOffsets` + `BigTileset1/3` + Bonus Code) |
| **04** | `ROM_Tileset2_X0Y1` | Compiled Z80 tile code: quadrant X0Y1 for Tileset 2 (`BigIndices4/5`) |
| **05** | `ROM_Tileset2_X1Y0` | Compiled Z80 tile code: quadrant X1Y0 for Tileset 2 (`Tileset2_TileOpcodeOffsets` + `BigTileset4/5`) |
| **06** | `ROM_Tileset3_X0Y1` | Compiled Z80 tile code: quadrant X0Y1 for Tileset 3 (`BigIndices6/7` + Bonus Code/Data) |
| **07** | `ROM_Tileset3_X1Y0` | Compiled Z80 tile code: quadrant X1Y0 for Tileset 3 (`Tileset3_TileOpcodeOffsets` + `BigTileset6/7`) |
| **08** | `ROM_Tileset0_X0Y0` | Compiled Z80 tile code: quadrant X0Y0 for Tileset 0 + `Level0_MapData` |
| **09** | `ROM_Tileset0_X1Y1` | Compiled Z80 tile code: quadrant X1Y1 for Tileset 0 + `Level1_MapData` |
| **10** | `ROM_Tileset1_X0Y0` | Compiled Z80 tile code: quadrant X0Y0 for Tileset 1 + `Level2_MapData` |
| **11** | `ROM_Tileset1_X1Y1` | Compiled Z80 tile code: quadrant X1Y1 for Tileset 1 + `Level3_MapData` |
| **12** | `ROM_Tileset2_X0Y0` | Compiled Z80 tile code: quadrant X0Y0 for Tileset 2 + `Level4_MapData` |
| **13** | `ROM_Tileset2_X1Y1` | Compiled Z80 tile code: quadrant X1Y1 for Tileset 2 + `Level5_MapData` |
| **14** | `ROM_Tileset3_X0Y0` | Compiled Z80 tile code: quadrant X0Y0 for Tileset 3 + `Level6_MapData` |
| **15** | `ROM_Tileset3_X1Y1` | Compiled Z80 tile code: quadrant X1Y1 for Tileset 3 + `Level7_MapData` |
| **16** | `ROM_RAM_8000_BFD0` | Pre-initialized RAM image loaded to `&8000-&BFD0` on boot: TitleScreenData/Code, GameData/Code, BonusData/Code, EndScreenData/Code, `FlowInitPlayIntMu`, Global Code+Data |
| **17** | `ROM_Audio` | Targhan audio engine + 50 Hz music and sound effects (PSG/DMA) |
| **18** | `ROM_Game0` | In-game hardware sprite definitions (`GameSprites`) + Main 50 Hz `GameLoop` |
| **19** | `ROM_Game1` | Boss fight sprite graphics (`BossFightSprites`) + Boss sprite control logic |
| **20** | `ROM_Game2` | Sonic terrain curves math tables + core gameplay physics |
| **21** | `ROM_Game3` | Sprite animation sequences (`SpriteSequences`) + level clear (`SonicHasPassed`) |
| **22** | `ROM_Game4` | Boss fight state machine data + background decoration sprites |
| **23** | `ROM_Game5` | High-level entity managers (`Manage`) + collision engine (`Collide`) |
| **24** | `ROM_Bonus0` | Special Stage: 3D checkerboard perspective tables (`R12/R13 + SSCR + FramesScaleY`) |
| **25** | `ROM_Bonus1` | Special Stage: Ring differential sprites + scanline start/width tables |
| **26** | `ROM_Bonus2` | Special Stage: Checkerboard horizontal offsets + differential sprite graphics |
| **27** | `ROM_Bonus3` | Special Stage: Background clouds and mountains parallax art |
| **28** | `ROM_Bonus4` | Special Stage: Stage logic, collision, and score calculation |
| **29** | `ROM_Bonus5` | Special Stage: Stage art assets and sequences |
| **30** | `ROM_StartupAndTitleScreen` | Cold boot sequence, hardware unlocking, and interactive title screen |
| **31** | `ROM_EndScreen` | Victory sequence, ending animations, and credits |

---

## Z80 64 KB Memory Map

The Z80 address space is partitioned into four 16 KB pages, dynamically reconfigured via Gate Array / ASIC banking registers depending on whether the system is executing gameplay logic, servicing interrupts, or streaming tile graphics to VRAM.

```
+------------------+ &FFFF
| Page 3:          | Upper ROM (Tile code X0Y0 / X1Y1, Audio, Game logic)
| &C000 - &FFFF    | OR VRAM Buffer 1 (16 KB, write-only during tile draw)
+------------------+ &C000
+------------------+ &BFFF
| Page 2:          | &A000-&BFFF: RAM Code (ISRs, trampolines, self-mod code)
| &8000 - &BFFF    | &8000-&9FFF: Fast RAM (GameVars/BonusVars, stack, DMA buf)
+------------------+ &8000
+------------------+ &7FFF
| Page 1:          | ASIC Registers (&4000-&7FFF) when unlocked
| &4000 - &7FFF    | OR Unpacked Map Data (636x64 level data) when locked
+------------------+ &4000
+------------------+ &3FFF
| Page 0:          | Lower ROM (Tile code X0Y1 / X1Y0, SFX)
| &0000 - &3FFF    | OR VRAM Buffer 0 (16 KB, write-only during tile draw)
+------------------+ &0000
```

### Page 0: `&0000 - &3FFF`
* **When Lower ROM is Disabled**: Maps to physical RAM `&0000 - &3FFF`, serving as **VRAM Buffer 0** (the first 16 KB of the 32 KB display memory). Only write access is required by the Z80.
* **When Lower ROM is Active**: Maps the selected lower ROM bank. During tile row/column drawing routines:
  * Offset `&2000 - &3FFF` executes compiled Z80 machine code for char tiles quadrant `X0Y1` and `X1Y0`.
  * Also houses in-game sound effect player code and data.

### Page 1: `&4000 - &7FFF`
* **ASIC Page Active (Unlocked via `&BC00` sequence)**:
  * Exposes the hardware ASIC registers and sprite RAM:
    * `&4000 - &43FF`: 16 Hardware Sprites (16×16 pixels × 4 bits, 256 bytes per sprite).
    * `&6400 - &643F`: Palette RAM (16 classic CPC pens + 16 sprite pens, 12-bit RGB each).
    * `&6800`: `PRI` (Programmable Raster Interrupt line compare).
    * `&6801`: `SPLT` (Split Screen line compare).
    * `&6802 - &6803`: `SSA` (Split Screen Address R12/R13 reload).
    * `&6804`: `SSCR` (Soft Scroll Control: bits 0..3 horizontal, 4..6 vertical).
    * `&6805`: `SCTR` (Soft Scroll Delay counter).
    * `&6808`: `DCSR` (DMA Control and Status Register).
    * `&6C00 - &6C0F`: DMA Channels 0, 1, and 2 control registers.
* **ASIC Page Deactivated (Locked/Paged Out)**:
  * Maps physical RAM `&4000 - &7FFF`.
  * Contains the **Unpacked Map Data** for the active level:
    * Green Hill Zone Act 1 is $636 \times 64$ tiles. Grouped into $2 \times 2$ big tiles, the map unpacks to $(636 \times 64) / 4 = 10,176$ bytes.
    * Accommodates the vertical slices index array and big tileset lookup tables directly in RAM.

### Page 2: `&8000 - &BFFF` (Dedicated RAM Workspace)
This 16 KB window is permanently mapped to system RAM and is never swapped with ROM, providing stable execution context and scratchpad storage:
* **`&8000 - &9FFF` (Dynamic RAM & Buffers)**:
  * Global variables (life count, current score, ring counter).
  * Gameplay state variables (`GameVars`: Sonic world position X/Y, camera offsets, velocities).
  * **Memory Sharing Strategy**: The Special Stage (`BonusVars`) and Main Game (`GameVars`) share the **exact same fixed memory address** (`&8000 - &9F00`), since both modes are mutually exclusive.
  * Audio DMA circular buffers and command queues.
  * Z80 Stack.
* **`&A000 - &BFFF` (RAM Code & Execution Safe Zone)**:
  * Interrupt Service Routines (ISRs) for IM 2.
  * Self-modifying code routines (unrolled draw inner loops patched with live coordinates).
  * ROM banking trampoline helpers (inter-bank jump and call dispatchers).
  * Falls strictly inside the **Plus Vectored Interrupt Bug Safe Zone** (`&A000-&BFFF`).

### Page 3: `&C000 - &FFFF`
* **When Upper ROM is Disabled**: Maps physical RAM `&C000 - &FFFF`, serving as **VRAM Buffer 1** (the second 16 KB of the 32 KB display memory).
* **When Upper ROM is Active**:
  * Offset `&E000 - &FFFF` executes compiled Z80 machine code for char tiles quadrant `X0Y0` and `X1Y1`.
  * Hosts core gameplay physics, entity state machines, and Targhan's PSG/DMA music driver.

---

## Architectural Lessons & Optimization Philosophy

Arnaud Storq highlights several essential Z80/ASIC principles that run counter to textbook software engineering ("Good Design Sucks!"):

1. **Stack-Based Addressing**: The Z80 stack pointer `SP` is repurposed during tile rendering to point to precalculated VRAM destination address tables. Instructions like `POP HL` fetch a 16-bit destination pointer in only 10 T-states (versus 16 T-states for `LD HL,(nn)`).
2. **Elimination of Stack Parameter Passing**: SDCC 4.5.0 was used strictly for C-to-assembly transpilation, with all functions modified to communicate exclusively via **global variables** in RAM. Stack frame setup (`PUSH IX / LD IX,0 / ADD IX,SP`) is completely avoided.
3. **Commit-Level Memory Footprint Tracking**: Every commit to version control automatically regenerates:
   * `memmap.txt`: An ASCII map detailing the exact byte headroom remaining in each bank.
   * `game.cpr`: The production binary.
   Regression diffs in `memmap.txt` immediately identify when a code change exceeds the safe bank ceiling (`#9F00`, `#A000`, or `#B400`).
