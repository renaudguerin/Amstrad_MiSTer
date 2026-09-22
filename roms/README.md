# Amstrad MiSTer System ROMs

This directory contains the firmware and system ROM files required to run the Amstrad CPC core on MiSTer FPGA.

## Required Files

### 1. `boot.rom` (Required)
The multi-machine system ROM bundle. The core **will not boot without this file**.

- **Structure**: 160 KiB containing ten 16 KiB chunks:
  - OS6128 + BASIC 1.1 + AMSDOS + Multiface 2
  - OS664 + BASIC 664 + AMSDOS + Multiface 2
  - OS464 + BASIC 464
- **Installation**: Copy this file to your MiSTer SD card at:
  ```
  /media/fat/Games/Amstrad/boot.rom
  ```

### 2. `cpc464nd.eZ0` (Optional)
Expansion ROM image for CPC 464.
- Copy to `/media/fat/Games/Amstrad/` if utilizing 464 expansion ROM slots.

---

## Looking for Core Bitstream Releases (`.rbf`)?

Pre-compiled core bitstreams are **not** stored in this git repository to avoid repository bloat.

Official releases and release bitstreams are published on GitHub Releases:
👉 **[Latest Release on GitHub](https://github.com/renaudguerin/Amstrad_MiSTer/releases/latest)**

### Core Installation
Download `Amstrad_YYYYMMDD.rbf` from the release assets and copy it to:
```
/media/fat/_Computer/Amstrad_YYYYMMDD.rbf
```
