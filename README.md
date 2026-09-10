# Amstrad CPC & Plus / GX4000 for MiSTer

This repository is an experimental fork of the official [MiSTer Amstrad core](https://github.com/MiSTer-devel/Amstrad_MiSTer) (which itself originated as a port of [CoreAmstrad by Renaud Hélias](https://github.com/renaudhelias/CoreAmstrad) before extensive module rewrites).

While this work was initially envisioned as a series of bite-sized pull requests upstream, the implementation has diverged significantly (over 500 commits ahead of upstream) through major architectural additions. This fork focuses on two main goals:

1. **Cycle-Accurate Classic Video Emulation (CRTC Types 0 & 1)**: Rigorous hardware accuracy grounded in [*The Amstrad CPC CRTC Compendium* (ACCC)](https://shaker.logonsystem.eu/) by Longshot (Logon System).
2. **Amstrad Plus and GX4000 Range Support**: Comprehensive support for the Amstrad Plus series (464+, 6128+) and GX4000 console, including AMS40489 ASIC features, CPR cartridge loading, and expanded video/audio capabilities.

---

## Fork Status & Highlights

> [!NOTE]
> **Status: Active Work in Progress (WIP)**  
> The core is under active development. While many titles run well, work is ongoing to resolve remaining discrepancies.

### 1. Amstrad Plus & GX4000 Support
The core includes a dedicated parallel behavioral video and system path implementing the custom AMS40489 ASIC features alongside classic Gate Array operation:
* **Cartridge (.CPR) Support**: Direct loading of commercial and homebrew CPR cartridge files (RIFF container format) up to 512 KiB via the MiSTer OSD, backed by an atomic SDRAM cartridge memory service and MMU banking.
* **Compatibility Status**:
  * **Most commercial GX4000 CPR cartridges boot**.
  * **Several commercial games are playable without major issues** (e.g., *Burnin' Rubber*, *Navy Seals*, *RoboCop 2*, *Enforcer*, *Tintin on the Moon*).
  * Active troubleshooting continues for remaining hardware quirks, including input mapping/responsiveness on specific titles (e.g. Arnold 5 keyboard input, stuck fire button states on Plotting/Pang), subtle sprite-edge artifacts, audio DMA pitch/timing discrepancies on advanced demos (such as the CRTC3 demo), and system/BASIC cartridge disk ROM initialization.
* **16 Hardware Sprites**: 16×16 pixels with 15 colors plus transparency, 1×/2×/4× horizontal and vertical magnification, and priority layering over background graphics.
* **Enhanced 12-Bit Palette**: 4,096 colors across 32 hardware palette registers (16 for Gate Array / border ink and 16 for sprites).
* **3-Channel Audio DMA**: Autonomous sample list streaming directly to the PSG (YM2149 / AY-3-8912) without CPU overhead.
* **Programmable Raster Interrupts (PRI)** and split-screen / pixel-fine soft scrolling (SPLT/SSCR).
* **Snapshot (.SNA) Support**: CPC+ snapshot loading and state restoration (palette, MMU, DMA, ASIC registers).

### 2. Classic CRTC Accuracy
Classic CRTC emulation has been redesigned to align with the authoritative *Amstrad CPC CRTC Compendium* (ACCC v1.11):
* **Per-Type Engine Architecture**: Replaced the legacy monolithic CRTC module with dedicated, independent rule engines for **CRTC Type 0** (Hitachi HD6845S / UM6845) and **CRTC Type 1** (UM6845R), coordinated by an outer wrapper (`rtl/CRTC.v`).
* **Precise Counter & Sync Timing**: Cycle-accurate horizontal (C0–C3) and vertical (C4, C9) counting, interlace video modes (IVM), raster flash detection (RFD), skew compensation, VMA generation, and exact HSYNC/VSYNC trigger rules.
* **CPU/Bus Write Synchronization**: Accurate modeling of sub-cycle write races, R5/R0 write-event retention across character boundaries, C0 edge-boundary writes, and R2.JIT timing interactions with the Gate Array (GA40010) contention model.
* **Selectable Sync Filter**: Configurable in the OSD (Full / Live blanking / Off) to allow software relying on extreme CRTC sync manipulation and irregular line lengths to display properly while maintaining video scaler lock.
* **Continuous Verification**: Verified against hundreds of automated Verilator assertions, randomized equivalence soak testing against recorded golden hashes, and photographic reference captures from the Logon System [SHAKER](https://shaker.logonsystem.eu/) suite.

### 3. CI and Synthesis Pipeline
* Automated GitHub Actions workflows validate the Verilator simulation suites, linting rules, and soak tests on every non-documentation change.
* Automated full-effort Quartus 17.0.2 synthesis with strict timing closure verification (setup/hold slack and zero Total Negative Slack) ensures testable RBF artifacts.
* For implementation details and roadmaps, see [docs/current-status.md](docs/current-status.md) and [docs/implementation-roadmap.md](docs/implementation-roadmap.md).

---

## Upstream Core Features
(Retained from the base MiSTer Amstrad core)
* Precise CPU timings including proper contention model.
* Precise CRTC model supporting many tricks of Types 1 and 0.
* 2 disk drives.
* Disk write support.
* Close to real disk drive emulation with support of some protections.
* Selectable CPC 6128/664 mode with separate ROM sets.
* Multiface 2.
* Several monochrome modes and 2 types of palette (GA/ASIC).
* Selectable expansion ROM loading.
* Joystick support with up to 3 buttons.
* Kempston, SYMBiFACE II and Multiplay mice.
* HQ2x and Scanlines FX for scandoubler.
* Tape input through ADC board.
* Support *.CDT tape files.
* Tape output through speaker.

## Installation

> [!NOTE]
> This fork is still early in development, so pre-built `.rbf` release binaries are not yet provided on this repository.

To test a build (compiled locally or obtained from GitHub Actions CI build artifacts):
Place the `.rbf` file into the root of the SD card and **boot.rom** into the **Games/Amstrad** folder.

## Cartridge Support (.CPR)
For Amstrad Plus and GX4000 games:
* Place `*.CPR` files in the `Games/Amstrad` folder of your SD card.
* Mount the cartridge from the MiSTer OSD menu.
* Select machine model (GX4000, 6128 Plus, etc.) in the core settings as required.

## Disk support
Put some *.DSK files into Amstrad folder and mount it from OSD menu.
important Basic commands:
* cat - list the files on mounted disk.
* run" - load and start the program. ex: run"disc
* |a, |b - switch between drives

## Boot ROM
Boot ROM has following structure:

OS6128 + BASIC1.1 + AMSDOS + MF2 + OS664 + BASIC664 + AMSDOS + MF2 + OS464 + BASIC464

Every part is 16KB. You can create your own ROM if you have a special preference.

## Expansion ROM
Expansion ROM should have file extension .eXX, where XX is hex number 00-FF of ROM page to load.
Every page is 16KB. It's possible to load larger ROM. In this case every 16KB block will be loaded in subsequent pages.

### Special extensions:
* eZZ - LowROM(OS)
* eZ0 - LowROM(OS) + Page 0(Basic) + subsequent pages depending on size.

### Notes
You can load several expansions. With every load the system will reboot. System ROM also can be replaced the same way.
To restore original ROM you have to reload the core (Alt-F12).

You can define boot extensions to automatically load at start of core. Use following name rules:
* boot.eXX  - load to both 664 and 6128 configs
* boot0.eXX - load to 6128 config
* boot1.eXX - load to 664 config
* CPC464 ROM can be loaded from the core menu with **Load CPC464 ROM**. Use an eZ0 file containing LowROM(OS) + Page 0(BASIC), such as cpc464nd.eZ0.

whehe XX is 00-FF, ZZ, Z0.

## CDT tape files
CDT supported in very basic form for retro feeling and for some very specific apps. There is no way to rewind or fast forward the file. 
USER LED will lit if there is a tape in the memory and still have data to play and blink while playback.

Control keys:
* Alt+F1 - mute/unmute the tape sound
* Alt+F2 - unload the tape (turn off the LED)

CDT playback respects the tape motor ON/OFF state.

For loading a tape you need to type these commands

|TAPE + Enter (switch on tape mode) 
RUN" + Enter for loading a .CDT file after selected it from OSD menù 

## RAM
CPC664 and CPC464 models have only 64KB RAM - use these models for programs not compatible with 128KB RAM.

CPC6128 model has 64KB+512KB RAM. Upper 448KB are visible in special OS ROM or application aware of 512KB expansion.

## Multiface 2
* Multiface 2 can be activated with F11.
* USER LED shows if the MF2 ROM/RAM is active.
* Returning from the MF2 menu via (r)eturn makes the device invisible.
* Visibility can be restored via machine reset (original MF 2+).
* For loading a saved game, MF2 must be visible.
* ROM version is 8D.

## Technical references

Technical information for the CRTC accuracy work is sourced from the "Amstrad CPC CRTC
Compendium" by Longshot (CC BY-NC-ND). The latest French ACCC edition is the primary written
oracle; the matching English edition is a working translation. The current baseline is
v1.11.
