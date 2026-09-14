# Hardware & Platform Reference Specifications

This directory contains 34 canonical technical reference documents and datasheets covering the Amstrad CPC, Amstrad Plus (464+, 6128+, GX4000), Gate Array, memory architectures, audio, peripherals, and storage container formats.

---

## Authority & Precedence Ranking

When implementing and testing core behavior:

1. **Real Hardware & SHAKER Reference Photographs** (`shaker.logonsystem.eu`):
   Observed hardware behavior on real silicon outranks all written documentation.
2. **The Amstrad CPC CRTC Compendium (ACCC) v1.11 French edition**
   (`ACCC1.11-FR.pdf`; user-owned and untracked): Primary written oracle for
   CRTC timing, counters, sync generation, and per-type behavior. The matching English PDF is
   a working translation and reader aid. When language editions differ, the French reading
   controls unless hardware or an author clarification supersedes it.
3. **Hardware Specifications & Reverse-Engineering Documents** (this directory):
   Primary references for platform subsystems (ASIC registers, memory paging, DMA audio, Gate Array, PPI quirks, PSG masking, and container formats). Note: Core RTL architecture and implementation phasing are documented under [`docs/plus/architecture.md`](../plus/architecture.md) and [`docs/classic/`](../classic/).

---

## Document Inventory

### 1. Common / Shared Subsystems (`docs/specs/common/`)

Specifications covering chips, buses, and memory controllers that apply to both Classic and Plus hardware:

| Document | Description | Key Focus Areas |
|---|---|---|
| [`8255 PPI.md`](common/8255%20PPI.md) | Intel 8255 Programmable Peripheral Interface. | Port A (PSG databus), Port B (cassette input, printer busy, straps, VSYNC input), Port C (PSG BDIR/BC1 controls, keyboard matrix). |
| [`AY-3-8912 PSG.md`](common/AY-3-8912%20PSG.md) | Programmable Sound Generator register overview. | PPI-to-PSG access, register addressing, tone/noise/envelope channels, 8-bit I/O Port A. |
| [`Additional information about the AY-3-8912.md`](common/Additional%20information%20about%20the%20AY-3-8912.md) | Register masking and I/O port quirks. | Unused bit masking on readback (R1/3/5/13 = 4-bit, R6/8/9/10 = 5-bit); internal output latch retention; Port B readback. |
| [`General Instruments AY-3-8910_12_13 Programmable Sound Generator.md`](common/General%20Instruments%20AY-3-8910_12_13%20Programmable%20Sound%20Generator.md) | Complete General Instruments PSG datasheet. | Pinouts, electrical specs, internal block diagrams, register map for AY-3-8910/12/13. |
| [`mem.md`](./common/mem.md) | CPC6128 128KB PAL MMU logic specification. | Combinatorial logic and signal truth tables (`/CAS0`, `/CAS1`, `A14OUT`, `A15OUT`) for RAM banking selections 0–7 at `&7Fxx`. |
| [`Expansion ROM Selection.md`](common/Expansion%20ROM%20Selection.md) | Upper ROM selection mechanism. | Writing ROM selection codes 0–255 to `&DFxx`, Gate Array upper ROM enable, fallback to BASIC. |
| [`I_O port allocation.md`](common/I_O%20port%20allocation.md) | Partial 16-bit I/O port address decoding map. | Partial decoding table for Gate Array, RAM PAL, CRTC, ROM select, Printer, PPI, and Expansion / FDC. |
| [`INTEL 8272 Floppy Disc Controller.md`](common/INTEL%208272%20Floppy%20Disc%20Controller.md) | Intel 8272 FDC technical specification. | Functional description, command phases, register set, sector transfer sequences. |
| [`µPD765A_µPD7265 Floppy Disc Controller.md`](common/µPD765A_µPD7265%20Floppy%20Disc%20Controller.md) | NEC µPD765A / µPD7265 FDC specification. | Command set (Read/Write Sector, Format, Seek), status registers ST0–ST3, drive select timing, MFM/FM decoding. |
| [`Interrupts on the CPC_CPC+ and KC Compact.md`](common/Interrupts%20on%20the%20CPC_CPC+%20and%20KC%20Compact.md) | Comparative analysis of interrupt generation. | Comparison of Gate Array 52-line interrupt, ASIC PRI/DMA interrupts, and East German KC Compact (Z8536 CIO). |

---

### 2. Classic CPC Hardware (`docs/specs/classic/`)

Specifications specific to Classic CPC (464, 664, 6128) Gate Array and CRTC controllers:

| Document | Description | Key Focus Areas |
|---|---|---|
| [`Amstrad CPC Gate-Array.md`](classic/Amstrad%20CPC%20Gate-Array.md) | Overview of Amstrad Gate Array (40007 / 40010). | Port `&7Fxx`, pen selection, colour selection, 32 hardware colours, Modes 0/1/2/3, ROM banking. |
| [`Interrupt Generation Facility of the Amstrad Gate Array.md`](classic/Interrupt%20Generation%20Facility%20of%20the%20Amstrad%20Gate%20Array.md) | Exact logic of the 6-bit scanline interrupt counter. | Increment on CRTC HSYNC falling edge, 52-line trigger, 2-HSYNC delay after VSYNC start, clearing on INTACK. |
| [`Furthur details of interrupt timing.md`](classic/Furthur%20details%20of%20interrupt%20timing.md) | Z80 interrupt acknowledge cycle timing. | Z80 2 T-state forced wait states during INTACK, 1 µs hardware boundary quantization. |
| [`The 32k screen.md`](classic/The%2032k%20screen.md) | CRTC address generation and overscan programming. | Memory address generation `{MA13, MA12, RA2..0, MA9..0, CCLK}`; CRTC R12/R13 spanning across 16KB boundaries. |
| [`The 6845 Cathode Ray Tube Controller (CRTC).md`](classic/The%206845%20Cathode%20Ray%20Tube%20Controller%20(CRTC).md) | High-level register overview of 6845 CRTC family. | General register layout and timing roles. |
| [`Hitachi HD6845 Cathode Ray Tube Controller.md`](classic/Hitachi%20HD6845%20Cathode%20Ray%20Tube%20Controller.md) | Hitachi HD6845R / HD6845S datasheet. | CRTC Type 0 reference. |
| [`Motorola MC6845 Cathode Ray Tube Controller.md`](classic/Motorola%20MC6845%20Cathode%20Ray%20Tube%20Controller.md) | Motorola MC6845 datasheet. | CRTC Type 1 reference. |
| [`UM6845 Cathode Ray Tube Controller.md`](classic/UM6845%20Cathode%20Ray%20Tube%20Controller.md) | UMC UM6845 / UM6845R datasheet. | CRTC Types 0/1 reference. |

---

### 3. Amstrad Plus & ASIC ("Arnold V", AMS40489) (`docs/specs/plus/`)

Specifications for the Amstrad Plus ASIC and cartridge subsystems:

| Document | Description | Key Focus Areas |
|---|---|---|
| [`_Arnold V_ Specification - Issue 1.5 - 10th April 1990.md`](plus/_Arnold%20V_%20Specification%20-%20Issue%201.5%20-%2010th%20April%201990.md) | Official Amstrad engineering specification (Issue 1.5). | Hardware sprites (16×16, 15 colours), 32×12-bit palette (`&6400-&643F`), split screen (SPLT/SSA), PRI (`&6800`), soft scroll (`&6804`), 3-channel DMA sound AY-lists, cartridge paging (RMR2 `&7Fxx`, `&DFxx`), ADC0-7 (`&6808-&680F`), lock sequence, pinouts. |
| [`Extra CPC Plus Hardware Information.md`](plus/Extra%20CPC%20Plus%20Hardware%20Information.md) | Kevin Thacker's reverse-engineering & hardware measurements. | CRTC3 status registers 1 & 2 readback, open-bus/invalid area behavior, sprite position masking/sign-extension, magnification read-mirroring, 32-entry legacy hardware colour translation table, PRI/split compare logic, DMA opcode decoding rules, PPI ASIC emulation quirks. |
| [`CPC+ Differences.md`](plus/CPC+%20Differences.md) | Firmware and architectural differences between Classic CPC and Plus. | System cartridge layout and ROM versions (Firmware page 0, BASIC 1.1 page 1, AMSDOS page 3, Burnin' Rubber pages 4–6), hardware differences. |
| [`Operation of Z80 interrupt mode 0 in the CPC+ design.md`](plus/Operation%20of%20Z80%20interrupt%20mode%200%20in%20the%20CPC+%20design.md) | Z80 IM0 and IM2 execution on the Plus ASIC. | Vector byte generation during INTACK (`((IVR & 0xF8) | vector)`), vector sources (DMA0=4, DMA1=2, DMA2=0, Raster=6), multi-byte opcode behavior in IM0. |
| [`Furthur details of timing.md`](plus/Furthur%20details%20of%20timing.md) | Timing differences between Classic CPC and Plus ASIC. | CRTC3 HSYNC 1 character later than CRTC0; 1 µs write delay on ASIC CRTC registers; Gate Array monitor HSYNC generation; monitor PLL phase locking. |
| [`Format_CPR CPC Plus cartridge file format - CPCWiki - THE Amstrad CPC encyclopedia!.md`](plus/Format_CPR%20CPC%20Plus%20cartridge%20file%20format%20-%20CPCWiki%20-%20THE%20Amstrad%20CPC%20encyclopedia!.md) | CPR cartridge container definition. | RIFF container structure, `Ams!` form-type, `cb00`–`cb31` 16KB chunk format, zero-padding and truncation rules. |
| [`_.CPR_ CPC Plus Cartridge file data structure.md`](plus/_.CPR_%20CPC%20Plus%20Cartridge%20file%20data%20structure.md) | Compact specification of CPR RIFF format. | Companion specification to the CPR file format. |

---

### 4. Storage & Container Formats (`docs/specs/formats/`)

| Document | Description | Key Focus Areas |
|---|---|---|
| [`Snapshot (.SNA) file format.md`](formats/Snapshot%20(.SNA)%20file%20format.md) | Snapshot file format specification (v1, v2, and v3). | CPU registers, Gate Array palette/banking state, CRTC counter snapshots, and **Version 3 `CPC+` chunk format** (packed sprite RAM, attributes, palette, ASIC registers, DMA channel states). |
| [`Disk image file format.md`](formats/Disk%20image%20file%20format.md) | Standard CPC DSK floppy image format ("MV - CPC"). | Track and sector headers, geometry definition, uniform sector allocation. |
| [`Extended DiSK Image definition.md`](formats/Extended%20DiSK%20Image%20definition.md) | Extended DSK (EDSK, Rev 5) format specification. | Variable track lengths, copy-protection support, weak/random sectors, FM/MFM recording modes, oversized sectors. |
| [`DSC disk image format.md`](formats/DSC%20disk%20image%20format.md) | Header + DSC two-file disk image format. | Compact representation omitting data blocks for uniform sectors. |
| [`Tape-Image (.CDT) file format.md`](formats/Tape-Image%20(.CDT)%20file%20format.md) | Complete CDT (TZX v1.13 based) tape image format. | Standard speed, turbo speed, tone, pulse sequence, and direct recording blocks. |
| [`Tape-Image (.CDT) file format (Amstrad specific).md`](formats/Tape-Image%20(.CDT)%20file%20format%20(Amstrad%20specific).md) | Amstrad-specific CDT handling. | Supported vs ignored block definitions and standard CPC tape loader considerations. |

---

### 5. Datasheets (`docs/specs/datasheets/`)

| Document | Description |
|---|---|
| [`msm82c55a.pdf`](./datasheets/msm82c55a.pdf) | OKI MSM82C55A CMOS PPI datasheet. |
| [`z8536.pdf`](./datasheets/z8536.pdf) / [`z8536sgs.pdf`](./datasheets/z8536sgs.pdf) | Zilog Z8536 / SGS Z8536 Counter/Timer & Parallel I/O (CIO) datasheets (KC Compact clone). |
