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

You can also select a bundle using **Load Main ROM**. Use this to
switch to a bundle with a different country firmware. MiSTer Main's CPC
startup looks for `boot.rom`. Its separate `boot0.rom` through `boot3.rom`
search applies to other cores. If your CPC system bundle is named `boot0.rom`,
select it manually or copy it to `boot.rom` for automatic loading.

To build a country bundle from a directory of per-model ROMs, such as the
ROM directory from your AmSpirit installation, run from the checkout:

```sh
python3 roms/build_bundle.py /path/to/ROMs fr boot-fr.rom
```

The supported codes are `en`, `fr`, `es` and `da`. The tool replaces the
three OS/BASIC pairs and retains AMSDOS and Multiface 2 from the base bundle.
Copy the output to the Amstrad games directory and select it via
**Load Main ROM**. MiSTer remembers the file for that core profile.

### Load an expansion ROM

For a headerless 16 KiB `.rom` file, select the classic machine under
**Hardware → Model**, then use **ROM slots → ROM slot** to pick **Lower OS** or
**Upper 0** through **Upper 15**. Select the file with **Load into selected
slot**. That load writes the chosen model's bank and resets into that model.
The override survives a core reset and is cleared when you reload the FPGA
core. Use the startup filenames below for persistent overrides.

For a raw 16 KiB upper ROM, give the file an `.eNN` extension where `NN` is
the two-digit slot number in hexadecimal. For example, rename a ROM for
slot 10 to `myrom.e0A` and select it using **Load expansion**. This writes the
slot in both the CPC 6128 and 664 banks. Use **Load CPC464 ROM** to write the
same slot in the CPC 464 bank. Slot 0 is normally BASIC, and slot 7 is
normally AMSDOS on the disk models.

The `.eZZ` extension selects the lower OS ROM. For a combined image with
the lower ROM followed by upper slots starting at 0, use `.eZ0`. The
included `cpc464nd.eZ0` uses that format. Copy it into the Amstrad games
directory and select it through **Load CPC464 ROM** to load its CPC 464 bank.

### Load ROM overrides at startup

Place overrides in `/media/fat/Games/Amstrad/` alongside `boot.rom`. For
example, Main loads `boot.e0A` into upper slot 10 on both disk models,
`boot0.e0A` into the 6128 bank, and `boot1.e0A` into the 664 bank. The same
prefixes work with `.eZZ` and `.eZ0`. Use the manual CPC 464 picker for its
overrides.

The **Distributor** setting changes the PPI jumpers read by the firmware.
The installed OS/BASIC pair supplies the CPC's keyboard layout. **Right
Shift** and **Keypad** are separate host key mapping options.

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
