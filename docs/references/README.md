# Hardware & Platform Reference Documents

This directory contains 34 technical reference documents and datasheets (plus this README) covering the Amstrad CPC, Amstrad Plus (464+, 6128+, GX4000), Gate Array, memory architectures, peripherals, and storage container formats.

---

## Authority & Precedence Ranking

When implementing and testing core behavior:

1. **Real Hardware & SHAKER Reference Photographs** (`shaker.logonsystem.eu`):
   Observed hardware behavior on real silicon outranks all written documentation.
2. **The Amstrad CPC CRTC Compendium (ACCC) v1.10** (`docs/ACCC1.10-EN.pdf`):
   Primary written authority for CRTC timing, internal counters, sync generation, and per-type behavior (Types 0, 1, 2, 3, 4). Where reference documents in this directory disagree with ACCC v1.10 on CRTC behavior, **ACCC v1.10 is the higher truth**.
3. **Hardware Specifications & Reverse-Engineering Documents** (this directory):
   Primary references for non-CRTC platform subsystems (ASIC registers, memory paging, DMA audio, Gate Array, PPI quirks, PSG masking, and container formats).

---

## Document Inventory

### 1. Amstrad Plus & ASIC ("Arnold V", AMS40489)

These documents form the foundation for the **Amstrad Plus / GX4000 workstream** ([`docs/plus/architecture.md`](../plus/architecture.md) and [`docs/plus/references/asic-reference.md`](../plus/references/asic-reference.md)).

| Document | Description | Key Focus Areas |
|---|---|---|
| [`_Arnold V_ Specification - Issue 1.5 - 10th April 1990.md`](./_Arnold%20V_%20Specification%20-%20Issue%201.5%20-%2010th%20April%201990.md) | Official Amstrad engineering specification (Issue 1.5). | Hardware sprites (16×16, 15 colours), 32×12-bit palette (`&6400-&643F`), split screen (SPLT/SSA), programmable raster interrupt (PRI `&6800`), soft scroll (SSCR `&6804`), 3-channel DMA sound AY-lists, cartridge paging (RMR2 `&7Fxx`, `&DFxx`), 6-bit ADC0-7 (`&6808-&680F`), feature lock sequence, connector pinouts. |
| [`Extra CPC Plus Hardware Information.md`](./Extra%20CPC%20Plus%20Hardware%20Information.md) | Kevin Thacker's reverse-engineering & hardware test measurements on real CPC6128+ hardware. | CRTC3 status registers 1 & 2 readback bitfields, open-bus/invalid area behavior, sprite position masking/sign-extension, sprite magnification read-mirroring at offsets 4–7, 32-entry legacy hardware colour translation ROM table, PRI/split compare logic, DMA opcode decoding rules, PPI ASIC emulation quirks. |
| [`CPC+ Differences.md`](./CPC+%20Differences.md) | System and firmware deltas between Classic CPC and Plus models. | System cartridge layout and ROM versions (Firmware page 0, BASIC 1.1 v4 page 1, AMSDOS v7 page 3 with `\|GAME` command, Burnin' Rubber game pages 4–6), hardware differences (silent cassette, connector pinouts, no cassette hardware on 6128+). Physical cartridge banking architecture is detailed in Arnold V. |
| [`Operation of Z80 interrupt mode 0 in the CPC+ design.md`](./Operation%20of%20Z80%20interrupt%20mode%200%20in%20the%20CPC+%20design.md) | Detailed analysis and test traces of Z80 IM0 and IM2 execution on the Plus ASIC. | Vector byte generation during INTACK (`((IVR & 0xF8) \| vector)`), vector sources (DMA0=4, DMA1=2, DMA2=0, Raster=6), multi-byte opcode behavior in IM0 (ASIC does not supply subsequent opcode bytes; Z80 fetches from current PC without incrementing PC), locked ASIC vector default (`0x00` disabled; `0x56` hardware observation). |
| [`Format_CPR CPC Plus cartridge file format - CPCWiki - THE Amstrad CPC encyclopedia!.md`](./Format_CPR%20CPC%20Plus%20cartridge%20file%20format%20-%20CPCWiki%20-%20THE%20Amstrad%20CPC%20encyclopedia!.md) | CPR cartridge container definition. | RIFF container structure, `Ams!` form-type, `cb00`–`cb31` 16KB chunk format, zero-padding for short chunks (<16KB), and truncation rules. |
| [`_.CPR_ CPC Plus Cartridge file data structure.md`](./_.CPR_%20CPC%20Plus%20Cartridge%20file%20data%20structure.md) | Compact specification of the CPR RIFF format. | Companion specification to the CPR file format. |

---

### 2. Classic Gate Array, Bus Decoding & Memory Architecture

| Document | Description | Key Focus Areas |
|---|---|---|
| [`Amstrad CPC Gate-Array.md`](./Amstrad%20CPC%20Gate-Array.md) | Comprehensive overview of the Amstrad Gate Array (40007 / 40010 / 40489 legacy mode). | I/O port `&7Fxx`, pen selection (`00xxxxxx`), colour selection (`01xxxxxx`), 32 hardware colour values (27 unique colors), screen modes 0, 1, 2, and undocumented Mode 3, ROM banking (lower `&0000-&3FFF`, upper `&C000-&FFFF`). (Scanline interrupt generation is detailed in the interrupt document). |
| [`Interrupt Generation Facility of the Amstrad Gate Array.md`](./Interrupt%20Generation%20Facility%20of%20the%20Amstrad%20Gate%20Array.md) | Exact logic of the 6-bit scanline interrupt counter. | Increment on CRTC HSYNC falling edge, 52-line trigger, 2-HSYNC delay after VSYNC start with `<32` vs `>=32` check, clearing bit 5 on Z80 INTACK, software delay via MRER bit 4. |
| [`Interrupts on the CPC_CPC+ and KC Compact.md`](./Interrupts%20on%20the%20CPC_CPC+%20and%20KC%20Compact.md) | Comparative analysis of interrupt generation across hardware variants. | Comparison of Gate Array 52-line interrupt, ASIC PRI and DMA vectored interrupts, and the East German KC Compact clone (Z8536 CIO counter/timer). |
| [`Furthur details of interrupt timing.md`](./Furthur%20details%20of%20interrupt%20timing.md) | Richard Wilson's notes on Z80 interrupt acknowledge cycle timing. | Z80 2 T-state forced wait states during INTACK, 1 µs hardware boundary quantization, and specific instruction classes that alter forced wait-states. |
| [`Furthur details of timing.md`](./Furthur%20details%20of%20timing.md) | Timing differences between Classic CPC and Plus ASIC. | CRTC3 HSYNC 1 character (17 pixels) later than CRTC0; 1 µs write delay on ASIC CRTC registers; Gate Array monitor HSYNC generation (delayed 2 µs from CRTC, clamped to 6 chars max width); monitor PLL phase locking. |
| [`I_O port allocation.md`](./I_O%20port%20allocation.md) | 16-bit partial I/O port address decoding map. | Partial decoding table for Gate Array (`b15=0, b14=1`), RAM PAL (`b15=0`), CRTC (`b14=0`), ROM select (`b13=0`), Printer (`b12=0`), PPI (`b11=0`), and Expansion / FDC (`b10=0`). |
| [`mem.md`](./mem.md) | Functional reconstruction and logic specification for the CPC6128 128KB PAL MMU. | Combinatorial logic and signal truth tables (`/CAS0`, `/CAS1`, `A14OUT`, `A15OUT`) for RAM banking selections 0 through 7 at `&7Fxx` with data bits `7:6 = 11`. (Note: source notes original PAL equations are unknown; presents an equivalent working model). |
| [`Expansion ROM Selection.md`](./Expansion%20ROM%20Selection.md) | Upper ROM selection mechanism. | Writing ROM selection codes 0–255 to `&DFxx`, coordination with Gate Array upper ROM enable, fallback to BASIC (ROM 0) if the requested ROM is unmapped. |
| [`The 32k screen.md`](./The%2032k%20screen.md) | CRTC address generation and overscan programming. | Memory address generation `{MA13, MA12, RA2..0, MA9..0, CCLK}`; using CRTC R12/R13 `MA11/MA10` bits to span across 16KB boundaries to display a 24K overscan screen across a 32K address range. |

---

### 3. Peripheral Controllers & Audio

| Document | Description | Key Focus Areas |
|---|---|---|
| [`8255 PPI.md`](./8255%20PPI.md) | Intel 8255 Programmable Peripheral Interface role in CPC/Plus. | Port A (PSG databus), Port B (cassette input, printer busy, LK1–LK3 manufacturer name straps, LK4 50/60Hz refresh strap, `/EXP` pin, VSYNC input), Port C (PSG BDIR/BC1 controls, cassette motor, keyboard matrix row select 0–15). |
| [`msm82c55a.pdf`](./msm82c55a.pdf) | OKI MSM82C55A CMOS PPI datasheet. | Industry datasheet for timing specifications, bus interface, and control word decoding. |
| [`AY-3-8912 PSG.md`](./AY-3-8912%20PSG.md) | Programmable Sound Generator register overview. | CPC PPI-to-PSG access, register addressing, and I/O programming (3 tone channels, noise generator, mixer, volume/envelope controls, 8-bit I/O Port A). Detailed electrical and internal architecture is covered in the General Instruments datasheet. |
| [`Additional information about the AY-3-8912.md`](./Additional%20information%20about%20the%20AY-3-8912.md) | Reverse-engineered register masking and I/O port quirks. | Unused bit masking on readback (R1/3/5/13 = 4-bit, R6/8/9/10 = 5-bit); internal output latch retention when Port A/B are configured as input; reading Port B returning `0xFF` on AY-3-8912; keyboard matrix interaction. |
| [`General Instruments AY-3-8910_12_13 Programmable Sound Generator.md`](./General%20Instruments%20AY-3-8910_12_13%20Programmable%20Sound%20Generator.md) | Complete General Instruments PSG datasheet. | Pinouts, electrical specifications, internal block diagrams, and register map for AY-3-8910 (2 I/O ports), AY-3-8912 (1 I/O port), and AY-3-8913 (0 I/O ports). |
| [`INTEL 8272 Floppy Disc Controller.md`](./INTEL%208272%20Floppy%20Disc%20Controller.md) | Intel 8272 FDC technical specification. | Functional description, command phases, register set, and sector data transfer sequences. |
| [`µPD765A_µPD7265 Floppy Disc Controller.md`](./µPD765A_µPD7265%20Floppy%20Disc%20Controller.md) | NEC µPD765A / µPD7265 Floppy Disc Controller specification. | Command set (Read/Write Sector, Format Track, Read ID, Seek, Recalibrate), status registers ST0–ST3, drive select timing, and MFM/FM decoding. |
| [`z8536.pdf`](./z8536.pdf) / [`z8536sgs.pdf`](./z8536sgs.pdf) | Zilog Z8536 / SGS Z8536 (partial scan) Counter/Timer & Parallel I/O (CIO) datasheets. | Datasheets for the CIO chip used in the East German KC Compact clone to emulate Gate Array interrupts and peripheral I/O. |

---

### 4. Storage & Snapshot Container Formats

| Document | Description | Key Focus Areas |
|---|---|---|
| [`Snapshot (.SNA) file format.md`](./Snapshot%20%28.SNA%29%20file%20format.md) | Snapshot file format specification (v1, v2, and v3). | CPU registers, Gate Array palette/banking state, CRTC internal counter snapshots, and **Version 3 `CPC+` chunk format** (packed 2bpp sprite bitmap RAM, sprite attributes, 12-bit palette, ASIC registers, and DMA channel states). |
| [`Disk image file format.md`](./Disk%20image%20file%20format.md) | Standard CPC DSK floppy image format ("MV - CPC"). | Track and sector headers, geometry definition, uniform sector allocation within tracks. |
| [`Extended DiSK Image definition.md`](./Extended%20DiSK%20Image%20definition.md) | Extended DSK (EDSK, Rev 5) format specification. | Variable track lengths, copy-protection support, weak/random sectors, data rate and FM/MFM recording mode definitions, 8KB and oversized sector handling. |
| [`DSC disk image format.md`](./DSC%20disk%20image%20format.md) | Header + DSC two-file disk image format. | Compact representation omitting data blocks for single-byte-filled uniform sectors. |
| [`Tape-Image (.CDT) file format.md`](./Tape-Image%20%28.CDT%29%20file%20format.md) | Complete CDT (TZX v1.13 based) tape image format. | Standard speed, turbo speed, pure tone, pulse sequence, and direct recording blocks. |
| [`Tape-Image (.CDT) file format (Amstrad specific).md`](./Tape-Image%20%28.CDT%29%20file%20format%20%28Amstrad%20specific%29.md) | Amstrad-specific CDT handling. | Supported vs ignored block definitions and standard CPC tape loader considerations. |

---

### 5. Classic CRTC Datasheets & Summaries *(Reference Only)*

> [!NOTE]
> For all CRTC behavior and counter edge cases, **ACCC v1.10 (`docs/ACCC1.10-EN.pdf`) takes strict precedence** over these datasheets.

| Document | Description |
|---|---|
| [`The 6845 Cathode Ray Tube Controller (CRTC).md`](./The%206845%20Cathode%20Ray%20Tube%20Controller%20%28CRTC%29.md) | High-level register overview of the 6845 CRTC family. |
| [`Motorola MC6845 Cathode Ray Tube Controller.md`](./Motorola%20MC6845%20Cathode%20Ray%20Tube%20Controller.md) | Motorola MC6845 datasheet (CRTC Type 2 reference). |
| [`Hitachi HD6845 Cathode Ray Tube Controller.md`](./Hitachi%20HD6845%20Cathode%20Ray%20Tube%20Controller.md) | Hitachi HD6845R / HD6845S datasheet (CRTC Type 0 reference). |
| [`UM6845 Cathode Ray Tube Controller.md`](./UM6845%20Cathode%20Ray%20Tube%20Controller.md) | UMC UM6845 / UM6845R datasheet (CRTC Types 0/1 reference). |

---

## Key Gap-Fills & Architectural Guidance for Agents

When implementing features in the core, agents should consult the following specific files:

1. **Plus Snapshot Support (Phase P8 Polish)**:
   - Consult [`Snapshot (.SNA) file format.md`](./Snapshot%20%28.SNA%29%20file%20format.md) for the `"CPC+"` chunk layout (offsets `0x000-0x8F7`: packed sprite RAM, attributes, palette, ASIC registers, and DMA attributes/state). Note: DMA pause-count / pause-prescaler snapshot restoration carries open verification items in the specification.
2. **ASIC 8255 PPI Quirks (Phase P8 Polish)**:
   - Consult [`Extra CPC Plus Hardware Information.md`](./Extra%20CPC%20Plus%20Hardware%20Information.md) and [`8255 PPI.md`](./8255%20PPI.md). On the Plus ASIC, Port B is fixed to input, Port C is fixed to output, control word rewrites do not clear output latches, and Port A in input mode presents `0xFF` to the PSG.
3. **Plus DMA Audio Engine (Phase P7)**:
   - Consult [`_Arnold V_ Specification`](./_Arnold%20V_%20Specification%20-%20Issue%201.5%20-%2010th%20April%201990.md) and [`Extra CPC Plus Hardware Information.md`](./Extra%20CPC%20Plus%20Hardware%20Information.md) for AY-list opcode formats (`0RDD`, `1NNN`, `2NNN`, `4xxx`), HSYNC-relative cycle slots, and 8255/PPI WAIT arbitration (CPU PPI accesses may wait up to 8 µs during DMA `LOAD`).
4. **PRI, Sprites & Split Screen Logic (Phases P3, P4, P6)**:
   - Consult [`Extra CPC Plus Hardware Information.md`](./Extra%20CPC%20Plus%20Hardware%20Information.md) for raster line and split screen comparison equations (`(LineCounter << 3) | (RasterCounter & 7)`). Note: check PRI formulas against Arnold V and [`docs/plus/references/asic-reference.md`](../plus/references/asic-reference.md) as the PRI expression in Kevin Thacker's note contains source typographical errors, and PRI trigger timing is ~6 µs from line start.
5. **MMU 128KB Banking (Classic CPC6128 & 6128+)**:
   - Consult [`mem.md`](./mem.md) for functional PAL banking tables (`/CAS0`, `/CAS1`, `A14OUT`, `A15OUT`) and compare with I/O address decoding rules in [`I_O port allocation.md`](./I_O%20port%20allocation.md).
