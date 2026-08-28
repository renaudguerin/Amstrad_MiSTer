# Phase P9 & Amstrad Plus Hardware Test Checklist

Targeted hardware verification plan for physical MiSTer testing of the Amstrad Plus & GX4000 core.

---

## 1. OSD & Model Selection Setup

- [ ] **OSD Media Menu Layout**:
  - Open OSD and verify that primary media loading entries are grouped together:
    - `Mount A: (DSK)`
    - `Mount B: (DSK)`
    - `Load tape (CDT)`
    - `Load Plus cartridge (CPR)`
- [ ] **Model Selection**:
  - Under `Hardware` menu, test all `Plus model` options:
    - `Off`: Classic CPC (CRTC type selectable 0 or 1, standard Gate Array, 64KB/128KB).
    - `GX4000`: 64KB RAM, no FDC, no tape, fixed upper ROM page 1.
    - `6128+`: 128KB RAM, FDC present, unexpanded bare machine (`/EXP=1`) resolves ROM-select 0 to AMSDOS page 3.
    - `464+`: 64KB RAM, tape present, unexpanded bare machine (`/EXP=1`) resolves ROM-select 0 to AMSDOS page 3.
- [ ] **Cartridge Detach**:
  - Verify `Reset & Detach Cartridge` unloads the current cartridge image and returns to basic unexpanded state.

---

## 2. Cartridge Boot & Auto-Reset (P0, P9)

- [ ] **Auto-Reset on CPR Load**:
  - Select and load a `.cpr` file from OSD.
  - Verify that the core automatically asserts system reset during download, atomic commit occurs, and the CPU resets directly into cartridge page 0 (`&0000`) without manual OSD reset.
- [ ] **Case Tolerances (P9)**:
  - Test `.cpr` dumps with both `AMS!` (uppercase) and `Ams!` (mixed case) RIFF headers.
  - Test `.cpr` dumps with both `CB00`..`CB31` and `cb00`..`cb31` chunk IDs.

---

## 3. Real Hardware Titles Test Matrix

### A. Firmware / System Cartridge
- [ ] **Amstrad System Cartridge (v4)**:
  - Boot on `6128+`: verify Firmware 4.0 banner, Locomotive BASIC 1.1, and AMSDOS banner.
  - Boot on `464+`: verify Firmware 4.0 banner and Locomotive BASIC 1.1 (no AMSDOS).
  - Boot on `GX4000`: verify French/English insert-cartridge splash screen.

### B. Commercial Cartridges (GX4000 / Plus)
- [ ] **Burnin' Rubber**:
  - Boot page 0 auto-start.
  - ASIC 12-bit RGB palette display.
  - Hardware sprite cars (scaling, priority, coordinate positioning).
  - Audio playback (PSG music + digital engine sound effects).
- [ ] **RoboCop 2**:
  - Screen split: SSA/SPLT split-screen score bar at top/bottom.
  - Soft horizontal scroll: sub-character 16-dot pixel shifting.
  - PRI (Programmable Raster Interrupt) raster effects.
  - 3-Channel DMA sound playback during gameplay.
  - Hardware sprites for player character and enemies.
- [ ] **Pang**:
  - PRI scanline interrupt synchronization.
  - 16 hardware sprites with dual-palette bank switching.
  - High-color background and sprite layers.
- [ ] **Navy Seals**:
  - Smooth multi-directional soft scrolling.
  - Split screen status display.
  - Multiplexed hardware sprites.
- [ ] **Klax**:
  - Isometric tile rendering and sprite falling animations.
  - Palette animation and timing stability.
- [ ] **Switchblade / Dick Tracy / Plotting / Tin Tin on the Moon**:
  - Cartridge loading, page banking, and gameplay stability.

### C. Demos & Diagnostics
- [ ] **`crtc3_v2fix.cpr`**:
  - Diagnostic test for CRTC 3 timing, syncs, and status registers.
- [ ] **PhX Demo**:
  - ASIC multiple raster splits per frame.
  - 12-bit RGB color cycling.
  - Soft vertical and horizontal hardware scrolling.
  - Continuous 3-channel DMA sound streaming.
- [ ] **Batman Forever Demo**:
  - High-density color raster splits and oversized display windows.
- [ ] **SHAKER Diagnostic Suite**:
  - With `Plus model = Off`: verify CRTC 0 and CRTC 1 results match reference photos (`shaker.logonsystem.eu`).
  - With `Plus model = 6128+`: verify CRTC 3 behavior and identification.

---

## 4. Subsystem Verification Matrix

| Subsystem | Key Registers / Features | Test Method | Pass Criteria |
|---|---|---|---|
| **MMU / Banking** | RMR2 (`&7Fxx` with `&B8`/`&A4`/`&AC`/`&B4`), MRER (`D[2]`, `D[3]`), `&DF00` | System Cartridge / BASIC `|CPM` / Soft relocation | Low ROM relocates to `&0000`, `&4000`, or `&8000`; MRER D[2]/D[3]=1 disables ROM and exposes underlying RAM |
| **ASIC Unlock** | 16-byte unlock sequence to `&BC00` | Any Plus title / ASIC register writes | Registers accept writes only after valid unlock sequence |
| **ASIC Video** | SSA (`&6802/3`), SPLT (`&6801`), SSCR (`&6804`) | RoboCop 2, PhX demo, Navy Seals | Split screen line is clean with no jitter; soft scroll shifts pixels smoothly |
| **ASIC Sprites** | Coordinates (`&6000-&607F`), Pixels (`&4000-&5FFF`), Magnification (`&6004+8*n`) | Burnin' Rubber, Pang | 16 sprites rendered without missing scanlines; x1/x2/x4 scaling correct |
| **ASIC DMA Sound** | SAR (`&6C00`), CDR (`&6C02`), CPR (`&6C04`), DCSR (`&6800`) | RoboCop 2, PhX, Burnin' Rubber | DMA channels A, B, C stream audio without clicks/underruns; DMA INT fires |
| **PPI & ADC** | PPI 8255 quirks, ADC registers (`&6808-&680F`) | Keyboard matrix, analog paddle readings | Default paddle reads `3F 3F 3F 3F 3F 00 3F 00`; keyboard fully responsive |
| **Classic Non-Interference** | CPC 464/664/6128 modes, DSK, CDT, SNA | Standard games (e.g. Gryzor, Arkanoid) | 100% bit-identical classic behavior; tape and disk load cleanly |
