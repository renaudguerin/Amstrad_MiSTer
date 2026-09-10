<!--
Converted from "The 6845 Cathode Ray Tube Controller (CRTC).html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 1 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# The 6845 Cathode Ray Tube Controller (CRTC)

## Introduction

The 6845 Cathode Ray Tube Controller (CRTC) is a programmable IC used to generate video displays. This IC is used in a variety of computers including the Amstrad CPC, Amstrad CPC+ and KC Compact.

## CRTC in the Amstrad CPC/CPC+ and KC Compact designs

The CRTC was a common part available from many different manufacturers. During the life of the CPC, Amstrad sourced the CRTC from various manufacturers.

All ICs used were based on the same design but have a different implementation. As a result they do not operate identically in all situations. This document highlights these differences.

This table lists the known ICs used, with their part number, manufacturer and type number.

| Part number | Manufacturer | Type number (note 3) |
|-------------|--------------|----------------------|
| UM6845      | UMC          | 0                    |
| HD6845S     | Hitachi      | 0                    |
| UM6845R     | UMC          | 1                    |
| MC6845      | Motorola     | 2                    |
| AMS40489    | Amstrad      | 3 (note 1)           |
| ???         | Amstrad?     | 4 (note 2)           |

NOTES:

1.  The CRTC functionality is integrated into the CPC+ ASIC. This type exists only in the CPC464+,CPC6128+ and GX4000.

2.  As far as I know, this type exists only in "cost-down" CPC6128 systems. In the "cost-down" CPC6128, the CRTC functionality is integrated into a single ASIC IC. This ASIC is often refered to as the "Pre-ASIC" because it preceeded the CPC+ ASIC.

3.  In the Amstrad community each 6845 implementation has been assigned a type number. This type identifies a group of implementations which operate in exactly the same way.

    As far as I know, the type number system was originally used by demo programmers.

    It is possible to detect the 6845 present using software methods, and this is done to:

    - warn that the software was not designed for the detected 6845 and may function incorrectly,
    - to adapt the software so that it will run with the detected 6845

    In most cases, the type of the detected 6845 is reported.

4.  As far as I know, the KC compact used HD6845S only.

The 6845 is selected when bit 14 of the I/O port address is set to "0". Bit 1 and 0 of the I/O port address define the function to access. The remaining bits can be any value, but it is adviseable to set these to "1" to avoid conflict with other devices in the system.

The recommended I/O port addressess are:

| I/O port address | Function                 | Read/Write |
|------------------|--------------------------|------------|
| &BCxx            | Select 6845 register     | Write only |
| &BDxx            | Write 6845 register data | Write only |
| &BExx            | (note 1)                 | Read only  |
| &BFxx            | (note 1)                 | Read only  |

NOTE:

1.  The function of these I/O ports is dependant on the CRTC type

### Signals

The following table defines the generated memory address from the CRTC and Gate-Array signals.

| Memory Address Signal | Signal source | Signal name |
|-----------------------|---------------|-------------|
| A15                   | 6845          | MA13        |
| A15                   | 6845          | MA12        |
| A14                   | 6845          | MA11        |
| A13                   | 6845          | RA2         |
| A12                   | 6845          | RA1         |
| A11                   | 6845          | RA0         |
| A10                   | 6845          | MA9         |
| A9                    | 6845          | MA8         |
| A8                    | 6845          | MA7         |
| A7                    | 6845          | MA6         |
| A6                    | 6845          | MA5         |
| A5                    | 6845          | MA4         |
| A4                    | 6845          | MA3         |
| A3                    | 6845          | MA2         |
| A2                    | 6845          | MA1         |
| A1                    | 6845          | MA0         |
| A0                    | Gate-Array    | CCLK        |

DISPTMG  
DISPTMG signal defines the border. When DISPTMG is "1" the border colour is output to the display.

HSYNC and VSYNC  
HSYNC and VSYNC from the CRTC are passed into the Gate-Array. The Gate-Array modifies the signals and then mixes these to form the Composite-Sync which is output to the display

## The 6845 Design

### Registers

The Internal registers of the 6845 are:

Register Index

Register Name

0

Horizontal Total

1

Horizontal Displayed

2

Horizontal Sync Position

3

Horizontal and Vertical Sync Widths

4

Vertical Total

5

Vertical Total Adjust

6

Vertical Displayed

7

Vertical Sync position

8

Interlace and Skew

9

Maximum Raster Address

10

Cursor Start Raster

11

Cursor End Raster

12

Display Start Address (High)

13

Display Start Address (Low)

14

Cursor Address (High)

15

Cursor Address (High)

16

Light Pen Address (High)

17

Light Pen Address (High)

## CRTC Differences

In this section I will attempt to identify all the differences between each CRTC.

The following tables list the functions that can be accessed for each type:

#### Type 0

| b1  | b0  | Function                                  | Read/Write |
|-----|-----|-------------------------------------------|------------|
| 0   | 0   | Select internal 6845 register             | Write Only |
| 0   | 1   | Write to selected internal 6845 register  | Write Only |
| 1   | 0   | \-                                        | \-         |
| 1   | 1   | Read from selected internal 6845 register | Read only  |

#### Type 1

| b1  | b0  | Function                                  | Read/Write |
|-----|-----|-------------------------------------------|------------|
| 0   | 0   | Select internal 6845 register             | Write Only |
| 0   | 1   | Write to selected internal 6845 register  | Write Only |
| 1   | 0   | Read Status Register                      | Read Only  |
| 1   | 1   | Read from selected internal 6845 register | Read only  |

#### Type 2

| b1  | b0  | Function                                  | Read/Write |
|-----|-----|-------------------------------------------|------------|
| 0   | 0   | Select internal 6845 register             | Write Only |
| 0   | 1   | Write to selected internal 6845 register  | Write Only |
| 1   | 0   | \-                                        | \-         |
| 1   | 1   | Read from selected internal 6845 register | Read only  |

#### Type 3 and 4

| b1  | b0  | Function                                  | Read/Write |
|-----|-----|-------------------------------------------|------------|
| 0   | 0   | Select internal 6845 register             | Write Only |
| 0   | 1   | Write to selected internal 6845 register  | Write Only |
| 1   | 0   | Read from selected internal 6845 register | Read Only  |
| 1   | 1   | Read from selected internal 6845 register | Read only  |

It is not possible to read from all the internal registers, this table shows the read/write status of each register for each type:

| Register Index | Register Name | 0 | 1 | 2 | 3 |
|----|----|----|----|----|----|
| 0 | Horizontal Total | Write Only | Write Only | Write Only | (note 2) |
| 1 | Horizontal Displayed | Write Only | Write Only | Write Only | (note 2) |
| 2 | Horizontal Sync Position | Write Only | Write Only | Write Only | (note 2) |
| 3 | Horizontal and Vertical Sync Widths | Write Only | Write Only | Write Only | (note 2) |
| 4 | Vertical Total | Write Only | Write Only | Write Only | (note 2) |
| 7 | Vertical Sync position | Write Only | Write Only | Write Only | (note 2) |
| 8 | Interlace and Skew | Write Only | Write Only | Write Only | (note 2) |
| 9 | Maximum Raster Address | Write Only | Write Only | Write Only | (note 2) |
| 10 | Cursor Start Raster | Write Only | Write Only | Write Only | (note 2) |
| 11 | Cursor End Raster | Write Only | Write Only | Write Only | (note 2) |
| 12 | Display Start Address (High) | Read/Write | Write Only | Write Only | Read/Write (note 2) |
| 13 | Display Start Address (Low) | Read/Write | Write Only | Write Only | Read/Write (note 2) |
| 14 | Cursor Address (High) | Read/Write | Read/Write | Read/Write | Read/Write (note 2) \*\*\*check |
| 15 | Cursor Address (Low) | Read/Write | Read/Write | Read/Write | Read/Write (note 2) \*\*check |
| 16 | Light Pen Address (High) | Read Only | Read Only | Read Only | Read Only (note 2) \*\*check |
| 17 | Light Pen Address (High) | Read Only | Read Only | Read Only | Read Only (note 2) \*\*check |

Notes:

1.  On type 0 and 1, if a Write Only register is read from, "0" is returned.
2.  See the document "Extra CPC Plus Hardware Information" for more details.
