<!--
Converted from "Snapshot (.SNA) file format.html" on 2026-08-26 using pandoc (HTML → GFM).
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# Snapshot (.SNA) file format

The format was first defined for the CPCEMU emulator but is now widely supported. The format was originally defined by Marco Vieth. Version 3 was defined by Ulrich Doewich, Martin Korth, Richard Wilson and Kevin Thacker.

There are 3 versions defined in this document. Version 3 is the most recent and is currently supported by a few of the most recent emulators.

Abbreviations:

- GA = "Gate Array"
- CRTC = "6845 Cathode Ray Tube Controller"
- PPI = "Intel 8255 Programmable Peripheral Interface"
- PSG = "AY-3-8912 Programmable Sound Generator"

## Version 1

| Offset (Hex) | Count | Description |
|----|----|----|
| 00-07 | 8 | The identification string "MV - SNA". This must exist for the snapshot to be valid. |
| 08-0f | 8 | (not used; set to 0) |
| 10 | 1 | snapshot version (1) |
| 11 | 1 | Z80 register F |
| 12 | 1 | Z80 register A |
| 13 | 1 | Z80 register C |
| 14 | 1 | Z80 register B |
| 15 | 1 | Z80 register E |
| 16 | 1 | Z80 register D |
| 17 | 1 | Z80 register L |
| 18 | 1 | Z80 register H |
| 19 | 1 | Z80 register R |
| 1a | 1 | Z80 register I |
| 1b | 1 | Z80 interrupt flip-flop IFF0 (note 2) |
| 1c | 1 | Z80 interrupt flip-flop IFF1 (note 2) |
| 1d | 1 | Z80 register IX (low) (note 5) |
| 1e | 1 | Z80 register IX (high) (note 5) |
| 1f | 1 | Z80 register IY (low) (note 5) |
| 20 | 1 | Z80 register IY (high) (note 5) |
| 21 | 1 | Z80 register SP (low) (note 5) |
| 22 | 1 | Z80 register SP (high) (note 5) |
| 23 | 1 | Z80 register PC (low) (note 5) |
| 24 | 1 | Z80 register PC (high) (note 5) |
| 25 | 1 | Z80 interrupt mode (0,1,2) (note 3) |
| 26 | 1 | Z80 register F' (note 4) |
| 27 | 1 | Z80 register A' (note 4) |
| 28 | 1 | Z80 register C' (note 4) |
| 29 | 1 | Z80 register B' (note 4) |
| 2a | 1 | Z80 register E' (note 4) |
| 2b | 1 | Z80 register D' (note 4) |
| 2c | 1 | Z80 register L' (note 4) |
| 2d | 1 | Z80 register H' (note 4) |
| 2e | 1 | GA: index of selected pen (note 10) |
| 2f-3f | 17 | GA: current palette (note 11) |
| 40 | 1 | GA: multi configuration (note 12) |
| 41 | 1 | current RAM configuration (note 13) |
| 42 | 1 | CRTC: index of selected register (note 14) |
| 43-54 | 18 | CRTC: register data (0..17) (note 15) |
| 55 | 1 | current ROM selection (note 16) |
| 56 | 1 | PPI: port A (note 6) |
| 57 | 1 | PPI: port B (note 7) |
| 58 | 1 | PPI: port C (note 8) |
| 59 | 1 | PPI: control port (note 9) |
| 5a | 1 | PSG: index of selected register (note 17) |
| 5b-6a | 16 | PSG: register data (0,1,....15) |
| 6b-6c | 1 | memory dump size in Kilobytes (e.g. 64 for 64K, 128 for 128k) (note 18) |
| 6d-ff | 93 | not used set to 0 |
| 100-... | (defined by memory dump size) | memory dump |

Notes:

1.  All multi-byte values are stored in little-endian format (low byte followed by higher bytes).
2.  "IFF0" reflects the state of the maskable interrupt (INT). "IFF1" is used to store the state of IFF0 when a non-maskable interrupt (NMI) is executed. Bit 0 of these bytes is significant. For CPCEMU compatibility, these bytes should be set to "1" when the IFF flip-flop is "1" and "0" when the flip-flop is "0". For compatibility with other emulators, bits 7-1 should be set to "0". When bit 0 of "IFF0" is "0" maskable interrupts will be ignored. When bit 0 of "IFF1" is "1" maskable interrupts will be acknowledged and executed. See the document about the Z80 for more information.
3.  This byte will be 0, 1 or 2 for the interrupt modes 0, 1 or 2. The interrupt mode is set using the "IM x" instructions. See the document about the Z80 for more information.
4.  These registers are from the alternate register set of the Z80.
5.  These registers are 16-bit. "low" indicates bits 7..0, "high"indicates bits 15..8.
6.  This byte represents the inputs to PPI port A regardless of the input/output setting of this port.
7.  This byte represents the inputs to PPI port B regardless of the input/output setting of this port.
8.  This byte represents the outputs from port C regardless of the input/output setting of this port.
9.  This byte represents the PPI control byte which defines the input/output and mode of each port and not the last value written to this port. For CPCEMU compatibility bit 7 of this byte must be set to "1".
10. This byte in the snapshot represents the selected pen register of the Gate-Array. This byte is the last value written to this port. Bit 7,6,5 should be set to "0".
11. These bytes are the current palette. For CPCEMU compatibility, these bytes should have bit 7=bit 6=bit 5="0". Bits 4..0 define the colour using the hardware colour code. The colours are stored in the order pen 0, pen1, pen 2,...,pen 15 followed by border colour.
12. This byte in the snapshot represents the multi-configuration register of the Gate-Array. This byte is the last byte written to this register. For CPCEMU compatibility, bit 7 should be set to "1" and bit 6 and bit 5 set to "0".
13. This byte represents a ram configuration for a Dk'Tronics/Dobbertin/Amstrad compatible RAM expansion, or the built in RAM expansion of the CPC6128 and CPC6128+. Bits 5..0 define the ram expansion code. For CPCEMU compatibility, bit 7 and bit 6 of this byte should be set to "0".
14. This byte in the snapshot represents the index of the currently selected CRTC register. For compatibility with CPCEMU this value should be in the range 0-31.
15. These bytes represent the data of the CRTC's registers.
16. This byte in the snapshot represents the last byte written to the "ROM select" I/O port.
17. This byte in the snapshot represents the index of the currently selected PSG register. For CPCEMU compatibility, this byte should be in the range 0-15.
18. the first 64k is always the base 64k of ram. The second 64k (if present) is the additional ram in a Dk'Tronics/Dobbertin/Amstrad compatible RAM expansion or the internal ram of the CPC6128/CPC6128+. The memory dump is not dependant on the current RAM configuration. Note that CPCEMU can only write a 64K or 128K snapshot.

## Changes and additions in Version 2 from Version 1

| Offset (Hex) | Count | Description                                             |
|--------------|-------|---------------------------------------------------------|
| 10           | 1     | snapshot version (2)                                    |
| 6d           | 1     | CPC type: 0 = CPC464 1 = CPC664 2 = CPC6128 3 = unknown |
| 6e           | 1     | interrupt number (0..5) (note 1a)                       |
| 6f-74        | 6     | 6 multimode bytes (note 1b)                             |
| 75-ff        | x     | (not used)                                              |

Notes:

1.  If standard CPC raster interrupts are used, interrupts are acknowledged and "executed" at the time they are requested, then there will be 6 interrupts executed per screen update cycle.
    - CPCEMU uses a simple system to emulate the interrupts. It is assumed there are exactly 6 interrupts per screen update cycle (the interrupts are assumed to occur at a frequency of 300Hz). This byte records the interrupt number in the current screen update cycle. More accurate emulators use the correct interrupt generation method and may ignore this byte. For CPCEMU compatibility, these emulators should set this byte to "0".
    - CPCEMU uses a simple system to emulate the screen display. It allows the mode to be changed in each of the 6 interrupts that occur during a single 50Hz/60Hz period. These bytes represent the mode in each of these sections, i.e. the screen modes (0,1 or 2) for the interrupts 0..5. More accurate emulators support changing of the screen mode at any point supported by the Amstrad hardware, these emulators should write "0" for all these bytes.

## Changes and additions in Version 3 from Version 2

| Offset (Hex) | Count | Description |
|----|----|----|
| 10 | 1 | snapshot version (3) |
| 6d | 1 | CPC type: 0 = CPC464 1 = CPC664 2 = CPC6128 3 = unknown 4 = 6128 Plus 5 = 464 Plus 6 = GX4000 |
| 9C | 1 | FDD motor drive state (0=off, 1=on) |
| 9D-A0 | 1 | FDD current physical track (note 15) |
| A1 | 1 | Printer Data/Strobe Register (note 1) |
| A4 | 1 | CRTC type: 0 = HD6845S/UM6845 1 = UM6845R 2 = MC6845 3 = 6845 in CPC+ ASIC 4 = 6845 in Pre-ASIC |
| A9 | 1 | CRTC horizontal character counter register (note 11) |
| AA | 1 | unused (0) |
| AB | 1 | CRTC character-line counter register (note 2) |
| AC | 1 | CRTC raster-line counter register (note 3) |
| AD | 1 | CRTC vertical total adjust counter register (note 4) |
| AE | 1 | CRTC horizontal sync width counter (note 5) |
| AF | 1 | CRTC vertical sync width counter (note 6) |
| B0-B1 | 2 | CRTC state flags. (note 7) |
| Bit | Function |  |
| 0 | if "1" VSYNC is active, if "0" VSYNC is inactive (note 8) |  |
| 1 | if "1" HSYNC is active, if "0" HSYNC is inactive (note 9) |  |
| 2-7 | reserved |  |
| 7 | if "1" Vertical Total Adjust is active, if "0" Vertical Total Adjust is inactive (note 10) |  |
| 8-15 | Reserved (0) |  |

B2

1

GA vsync delay counter (note 14)

B3

1

GA interrupt scanline counter (note 12)

B4

1

interrupt request flag (0=no interrupt requested, 1=interrupt requested) (note 13)

B5-FF

75

unused (0)

Notes:

1.  This byte in the snapshot represents the last byte written to the printer I/O port (this byte does not include the automatic inversion of the strobe caused by the Amstrad hardware).
2.  This register is internal to the CRTC and counts the number of character-lines. The counter counts up. This value is in the range 0-127. (This counter is compared against CRTC register 4).
3.  This register is internal to the CRTC and counts the number of raster-lines. The counter counts up. This value is in the range 0-31. (This counter is compared against CRTC register 9).
4.  This register is internal to the CRTC and counts the number of raster-lines during vertical adjust. The counter counts up. This value is in the range 0-31. This should be ignored if the CRTC is not "executing" vertical. adjust.(This counter is compared against CRTC register 5).
5.  This register is internal to the CRTC and counts the number of characters during horizontal sync. This counter counts up. This value is in the range 0-16. This should be ignored if the CRTC is not "executing" horizontal sync. (This counter is compared against CRTC register 3).
6.  This register is internal to the CRTC and counts the number of scan-lines during vertical sync. This counter counts up. This value is in the range 0-16. This should be ignored if the CRTC is not "executing" vertical sync. (This counter is compared against CRTC register 3).
7.  These bytes define the internal state of the CRTC. Each bit in these bytes represents a state.
8.  When VSYNC is active, the CRTC is "executing" vertical sync, and the vertical sync width counter in the snapshot is used.
9.  When HSYNC is active, the CRTC is "executing" horizontal sync width counter in the snapshot is used.
10. When Vertical total adjust is active, the CRTC is "executing" vertical total adjust and the vertical total adjust counter in the snapshot is used.
11. This register is internal to the CRTC and counts the number of characters. This counter counts up. This value is in the range 0-255. (This counter is compared against CRTC register 0).
12. This counter is internal to the GA and counts the number of HSYNCs. This counter is used to generate CPC raster interrupts. This counter counts up. This value is in the range 0-51.
13. This flag is "1" if a interrupt request has been sent to the Z80 and it has not yet been acknowledged by the Z80. (A interrupt request is sent by the GA for standard CPC raster interrupts or by the ASIC for raster or dma interrupts).
14. This is a counter internal to the GA and counts the number of HSYNCs since the start of the VSYNC and it is used to reset the interrupt counter to synchronise interrupts with the VSYNC. This counter counts up. This value is between 0 and 2. If this value is 0, the counter is inactive. If this counter is 1 or 2 the counter is active.

Immediatly following the memory dump there is optional data which is seperated into chunks.

Each chunk of data has a header and this is followed by the data in the chunk. The header has the following format:

| Offset (Hex) | Count | Description                |
|--------------|-------|----------------------------|
| 0            | 4     | Chunk name (note 1)        |
| 4            | 4     | Chunk data length (note 2) |

Notes:

1.  The chunks are defined with 4-byte character codes. (e.g. "CPC+"). In this example, the 4-byte character code would be stored in the file as 'C' then 'P' then 'C' then '+'.
2.  The "Chunk data length" defines the length of data following the header and does not include the size of the header. This number is stored in little endian format.
3.  If a emulator finds a chunk which it does not support then it should skip the chunk and continue with the next chunk in the file. Therefore an emulator author may add emulator specific chunks to the file and it will not prevent the snapshot from being used with other emulators that do not recognise the added chunks.
4.  There is not a terminator chunk. The snapshot reader should determine if there are more chunks based on the size of data remaining to be read from the file.

The following chunks are currently defined:

### CPC+ Chunk

Chunk name: "CPC+"

Chunk data:

Offset (Hex)

Length

Addr in ASIC register-ram

Description

000-7FF

800h

4000-4FFF

Sprite Bitmaps (note 1)

800-87F

8\*16

6000-607F

Sprite Attributes (see below) (note 2)

880-8BF

32\*2

6400-643F

Palettes (note 3)

8C0

1

6800

Programmable Raster Interrupt (note 4)

8C1

1

6801

Screen split scan-line (note 4)

8C2

2

6802-6803

Screen split secondary screen-address (note 4)

8C4

1

6804

Soft scroll control register (note 4)

8C5

1

6805

Interrupt vector (note 4)

8C6-8C7

2

\-

unused (0)

8C8-8CF

8

6808-680f

Analogue input channels 0-7 (note 5)

8D0-8DB

3\*4

6C00-6C0B

Sound DMA channel attributes 0-2 (see below) (note 6)

8DC-8DE

3

\-

unused (0)

8DF

1

6C0F

DMA Control/Status (note 4)

8E0-8F4

3\*7

internal

DMA channel 0-2 internal registers (see below) (note 7)

8F5

1

internal

gate array A0 register value (note 8)

8F6

1

internal

gate array A0 lock: 0=locked, 1=unlocked (note 9)

8F7

1

internal

ASIC unlock sequence state (note 10)

Notes:

1.  The sprite data is packed, with two sprite pixels per byte. Bits 7..4 define the first pixel and bits 3..0 define the second pixel.
2.  The attributes for each sprite take 8 bytes. Each attribute block has the following format:
    | Offset | Length | Description                     |
    |--------|--------|---------------------------------|
    | 0      | 2      | Sprite X (see note)             |
    | 2      | 2      | Sprite Y (see note)             |
    | 4      | 1      | Sprite Magnification (see note) |
    | 5-7    | 3      | unused (0)                      |

    Note: the Sprite X, Y and magnification are in the same order as the ASIC registers
3.  This is a direct copy of the palette in CPC+ ASIC Ram. There are 32 colours each with 2-bytes per colour.
4.  These bytes in the snapshot represent the last value written to these ASIC registers.
5.  These bytes represent the inputs to the analogue channels.
6.  The attributes for each DMA channel take 4 bytes. Each attribute block has the following format:
    | Offset | Length | Description                      |
    |--------|--------|----------------------------------|
    | 0      | 2      | DMA Channel address (see note)   |
    | 2      | 1      | DMA Channel prescalar (see note) |
    | 3      | 1      | unused (0)                       |

    Note: the DMA address and prescalar are in the same order as the ASIC registers.
7.  These registers are internal to the CPC+ and define the current DMA operation:
    | Offset | Length | Description                    |
    |--------|--------|--------------------------------|
    | 0      | 2      | loop counter (note a)          |
    | 2      | 2      | loop address (note b)          |
    | 4      | 2      | pause count (note c)           |
    | 6      | 1      | pause prescalar count (note d) |

    1.  This value represents the number of loops remaining. 0 = none. This count is between 0..0FFF. This counter counts down.
    2.  This is the Amstrad memory address to loop back to. It is a pointer to the DMA instruction after the last REPEAT instruction.
    3.  This value represents the pause count and the count is between 0...0FFF. (TO BE CHECKED: down counter? what exactly does it represent)
    4.  This value represents the pause prescalar count and the count is between 0..FF. (TO BE CHECKED: down counter? what exactly does it represent)
8.  This value represents the last value written to this I/O port.
9.  This value represents the lock status of the ASIC. If the ASIC is un-locked then the advanced features and ASIC registers are accessible.
10. This value represents the current unlock sequence state.
    | State ID | Synchronised State | Note |
    |----|----|----|
    | 0 | not synchronised | ASIC is waiting for first non-zero byte to be written, this is the first synchronisation byte required |
    | 1 | not synchronised | ASIC is waiting for zero byte to be written, this is the second synchronisation byte required |
    | 2..10h | synchronised | ASIC is waiting for byte from unlock sequence. e.g. if "2", ASIC is waiting for &FF, the first byte of the unlock sequence. if "3" ASIC is waiting for &77, the second byte of the unlock sequence. |
