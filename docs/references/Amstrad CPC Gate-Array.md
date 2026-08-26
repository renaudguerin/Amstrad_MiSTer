<!--
Converted from "Amstrad CPC Gate-Array.html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 5 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# Gate Array

## Introduction

The gate array is a specially designed chip exclusively for use in the Amstrad CPC and was designed by Amstrad plc.

In the CPC+ system, the functions of the Gate-Array are integrated into a single ASIC. When the ASIC is "locked", the extra features are not available and the ASIC operates the same as the Gate-Array in the CPC allowing programs written for the CPC to work on the Plus without modification. The ASIC must be "un-locked" to access the new features.

In the KC compact system, the functions of the Gate-Array are "emulated" in TTL logic and by the Zilog Z8536 CIO.

In the "cost-down" version of the CPC6128, the functions of the Gate-Array are integrated into a ASIC.

The Gate Array is described here, as it is in a standard CPC.

## What does it do?

The Gate Array is responsible for the display (colour palette, resolution, horizontal and vertical sync), interrupt generation and memory arrangement.

## Controlling the Gate Array

The gate array is controlled by I/O. The gate array is selected when bit 15 of the I/O port address is set to "0" and bit 14 of the I/O port address is set to "1". The values of the other bits are ignored. However, to avoid conflict with other devices in the system, these bits should be set to "1".

The recommended I/O port address is &7Fxx.

The function to be performed is selected by writing data to the Gate-Array, bit 7 and 6 of the data define the function selected (see table below). It is not possible to read from the Gate-Array.

| Bit 7 | Bit 6 | Function                                                    |
|-------|-------|-------------------------------------------------------------|
| 0     | 0     | Select pen                                                  |
| 0     | 1     | Select colour for selected pen                              |
| 1     | 1     | Select screen mode, rom configuration and interrupt control |
| 1     | 1     | Ram Memory Management (note 1)                              |

Note:

1.  This function is not available in the Gate-Array, but is performed by a device at the same I/O port address location. In the CPC464,CPC664 and KC compact, this function is performed in a memory-expansion (e.g. Dk'Tronics 64K Ram Expansion), if this expansion is not present then the function is not available. In the CPC6128, this function is performed by a PAL located on the main PCB, or a memory-expansion. In the 464+ and 6128+ this function is performed by the ASIC or a memory expansion. Please read the document on Ram Management for more information.

## Pen selection

When bit 7 and bit 6 are set to "0", the remaining bits determine which pen is to have its colour changed. When bit 4 is set to "0", bits 3 to 0 define which pen is to be selected. When bit 4 is set to "1", the value contained in bits 3-0 is ignored and the border is selected.

The pen remains selected until another is chosen.

Each mode has a fixed number of pens. Mode 0 has 16 pens, mode 1 has 4 pens and mode 2 has 2 pens.

### Summary

| Bit | Value | Function                            |
|-----|-------|-------------------------------------|
| 7   | 0     | Gate Array function "Pen Selection" |
| 6   | 0     | Gate Array function "Pen Selection" |
| 5   | x     | not used                            |
| 4   | 1     | Select border                       |
| 3   | x     | ignored                             |
| 2   | x     | ignored                             |
| 1   | x     | ignored                             |
| 0   | x     | ignored                             |

| Bit | Value | Function                            |
|-----|-------|-------------------------------------|
| 7   | 0     | Gate Array function "Pen Selection" |
| 6   | 0     | Gate Array function "Pen Selection" |
| 5   | x     | not used                            |
| 4   | 0     | Select pen                          |
| 3   | x     | Pen Number                          |
| 2   | x     | Pen Number                          |
| 1   | x     | Pen Number                          |
| 0   | x     | Pen Number                          |

## Colour selection

Once the pen has been selected the colour can then be changed. Bits 4 to 0 specify the hardware colour number from the hardware colour palette.

Even though there is provision for 32 colours, only 27 are possible. The remaining colours are duplicates of those already in the colour palette.

### Summary

| Bit | Value | Function                               |
|-----|-------|----------------------------------------|
| 7   | 0     | Gate Array function "Colour Selection" |
| 6   | 1     | Gate Array function "Colour Selection" |
| 5   | x     | not used                               |
| 4   | x     | Colour number                          |
| 3   | x     | Colour number                          |
| 2   | x     | Colour number                          |
| 1   | x     | Colour number                          |
| 0   | x     | Colour number                          |

Hardware colour palette:

| Colour Number | Colour Name            |
|---------------|------------------------|
| 0             | White                  |
| 1             | White (note 1)         |
| 2             | Sea Green              |
| 3             | Pastel Yellow          |
| 4             | Blue                   |
| 5             | Purple                 |
| 6             | Cyan                   |
| 7             | Pink                   |
| 8             | Purple (note 1)        |
| 9             | Pastel Yellow (note 1) |
| 10            | Bright Yellow          |
| 11            | Bright White           |
| 12            | Bright Red             |
| 13            | Bright Magenta         |
| 14            | Orange                 |
| 15            | Pastel Magenta         |
| 16            | Blue (note 1)          |
| 17            | Sea Green (note 1)     |
| 18            | Bright Green           |
| 19            | Bright Cyan            |
| 20            | Black                  |
| 21            | Bright Blue            |
| 22            | Green                  |
| 23            | Sky Blue               |
| 24            | Magenta                |
| 25            | Pastel Green           |
| 26            | Lime                   |
| 27            | Pastel Cyan            |
| 28            | Red                    |
| 29            | Mauve                  |
| 30            | Yellow                 |
| 31            | Pastel Blue            |

Notes:

- This is not an official colour

## Select screen mode and rom configuration

This is a general purpose register responsible for the screen mode and the rom configuration.\
\

### Screen mode selection

The function of bits 1 and 0 is to define the screen mode. The settings for bits 1 and 0 and the corresponding screen mode are given in the table below.

| Bit 1 | Bit 0 | Screen mode                                    |
|-------|-------|------------------------------------------------|
| 0     | 0     | Mode 0, 160x200 resolution, 16 colours         |
| 0     | 1     | Mode 1, 320x200 resolution, 4 colours          |
| 1     | 0     | Mode 2, 640x200 resolution, 2 colours          |
| 1     | 1     | Mode 3, 160x200 resolution, 4 colours (note 1) |

- This mode is not official. From the combinations possible, we can see that 4 modes can be defined, although the Amstrad only has 3. Mode 3 is similar to mode 0, because it has the same resolution, but it is limited to only 4 colours.
- Mode changing is synchronised with HSYNC. If the mode is changed, it will take effect from the next HSYNC.

### Rom configuration selection

Bit 2 is used to enable or disable the lower rom area. The lower rom area occupies memory addressess &0000-&3fff and is used to access the operating system rom. When the lower rom area is is enabled, reading from &0000-&3FFF will return data in the rom. When a value is written to &0000-&3FFF, it will be written to the ram underneath the rom. When it is disabled, data read from &0000-&3FFF will return the data in the ram.

Similarly, bit 3 controls enabling or disabling of the upper rom area. The upper rom area occupies memory addressess &C000-&FFFF and is BASIC or any expansion roms which may be plugged into a rom board/box. See the document on upper rom selection for more details. When the upper rom area enabled, reading from &c000-&ffff, will return data in the rom. When data is written to &c000-&FFFF, it will be written to the ram at the same address as the rom. When the upper rom area is disabled, and data is read from &c000-&ffff the data returned will be the data in the ram.

Bit 4 controls the interrupt generation. It can be used to delay interrupts. See the document on interrupt generation for more information.

### Summary

| Bit | Value | Function                     |
|-----|-------|------------------------------|
| 7   | 0     | Gate Array function          |
| 6   | 1     | Gate Array function          |
| 5   | x     | not used                     |
| 4   | x     | Interrupt generation control |
| 3   | 1     | Upper rom area disable       |
| 3   | 0     | Upper rom area enable        |
| 2   | 1     | Lower rom area disable       |
| 2   | 0     | Lower rom area enable        |
| 1   | x     | Mode selection               |
| 0   | x     | Mode selection               |

------------------------------------------------------------------------

## Programming the Gate Array - Examples

1.  Defining the colours,

    Setting pen 0 to Bright White.

    LD BC,&7F00 ;Gate Array port LD A,%00000000+0 ;Pen number (and Gate Array function) OUT (C),A ;Send pen number LD A,%01000000+11 ;Pen colour (and Gate Array function) OUT (C),A ;Send it RET

2.  Setting the mode and rom configuration,

    Mode 2, upper and lower rom disabled.

    LD BC,&7F00 ;Gate array port LD A,%10000000+%00001110 ;Mode and rom selection (and Gate ;Array function) OUT (C),A ;Send it RET

------------------------------------------------------------------------

## conversion chart

The hardware colour number is different to the colour range used by the firmware, so a conversion chart is provided for the corresponding firmware/hardware colour values and the corresponding colour name.

Note:

- The firmware keeps track of the colours it is using. Every VSYNC (assuming interrupts are enabled) the firmware sets the colours. This enables the user to have flashing colours. If the user selects a new colour using the gate array, the new colour will flash temporarily and then return to it's original colour. This is due to the firmware re- setting the colour. When using the firmware, use it's routines to select the colour, and the colour will remain.

Firmware Colour Number

Colour Name

Hardware Colour Number

Quick reference hardware colour select value

0

Black

20

&54

1

Blue

4

&44

2

Bright Blue

21

&55

3

Red

28

&5C

4

Magenta

24

&58

5

Mauve

29

&5D

6

Bright Red

12

&4C

7

Purple

5

&45

8

Bright Magenta

13

&4D

9

Green

22

&56

10

Cyan

6

&46

11

Sky Blue

23

&57

12

Yellow

30

&5E

13

White

0

&40

14

Pastel Blue

31

&5F

15

Orange

14

&4E

16

Pink

7

&47

17

Pastel Magenta

15

&4F

18

Bright Green

18

&52

19

Sea Green

2

&42

20

Bright Cyan

19

&53

21

Lime

26

&5A

22

Pastel Green

25

&59

23

Pastel Cyan

27

&5B

24

Bright Yellow

10

&4A

25

Pastel Yellow

3

&43

26

Bright White

11

&4B

This chart also gives a quick reference guide for programming the colours. The number is the colour number which can be sent directly, once the pen has been selected, to get the colour wanted.

Example:\

ld bc,&7f00+1 ;Gate array function (set pen) ;and pen number out (c),c ld bc,&7f00+&41 ;Gate array function (set colour) ;and colour number out (c),c ret

------------------------------------------------------------------------

## Pallette R,G,B definitions

There are 27 colours which are generated from red, green and blue mixed in different quantities. There are 3 levels of red, 3 levels of green and 3 levels of blue, and these can be thought of as off/no colour, half-on/half-colour, and on/full-colour.

To display a CPC image you will need to use a analogue monitor with a composite sync.

This table shows the relationship between hardware colour number, colour name and RGB mixing.

| Hardware Colour Index | Colour Name    | R % | G % | B % |
|-----------------------|----------------|-----|-----|-----|
| 0                     | White          | 50  | 50  | 50  |
| 1                     | White          | 50  | 50  | 50  |
| 2                     | Sea Green      | 0   | 100 | 50  |
| 3                     | Pastel Yellow  | 100 | 100 | 50  |
| 4                     | Blue           | 0   | 0   | 50  |
| 5                     | Purple         | 100 | 0   | 50  |
| 6                     | Cyan           | 0   | 50  | 50  |
| 7                     | Pink           | 100 | 50  | 50  |
| 8                     | Purple         | 100 | 0   | 50  |
| 9                     | Pastel Yellow  | 100 | 100 | 50  |
| 10                    | Bright Yellow  | 100 | 100 | 0   |
| 11                    | Bright White   | 100 | 100 | 100 |
| 12                    | Bright Red     | 100 | 0   | 0   |
| 13                    | Bright Magenta | 100 | 0   | 100 |
| 14                    | Orange         | 100 | 50  | 0   |
| 15                    | Pastel Magenta | 100 | 50  | 100 |
| 16                    | Blue           | 0   | 0   | 50  |
| 17                    | Sea Green      | 0   | 100 | 50  |
| 18                    | Bright Green   | 0   | 100 | 0   |
| 19                    | Bright Cyan    | 0   | 100 | 100 |
| 20                    | Black          | 0   | 0   | 0   |
| 21                    | Bright Blue    | 0   | 0   | 100 |
| 22                    | Green          | 0   | 50  | 0   |
| 23                    | Sky Blue       | 0   | 50  | 100 |
| 24                    | Magenta        | 50  | 0   | 50  |
| 25                    | Pastel Green   | 50  | 100 | 50  |
| 26                    | Lime           | 50  | 100 | 0   |
| 27                    | Pastel Cyan    | 50  | 100 | 100 |
| 28                    | Red            | 50  | 0   | 0   |
| 29                    | Mauve          | 50  | 0   | 100 |
| 30                    | Yellow         | 50  | 50  | 0   |
| 31                    | Pastel Blue    | 50  | 50  | 100 |

------------------------------------------------------------------------

## RGB assignments for the software colours

This is simply a sidenote to illustrate a pattern in the RGB assignments of the software colours and to show how their value is calculated.

| Firmware Colour Number | Colour Name    | R % | G % | B % |
|------------------------|----------------|-----|-----|-----|
| 0                      | Black          | 0   | 0   | 0   |
| 1                      | Blue           | 0   | 0   | 50  |
| 2                      | Bright Blue    | 0   | 0   | 100 |
| 3                      | Red            | 50  | 0   | 0   |
| 4                      | Magenta        | 50  | 0   | 50  |
| 5                      | Mauve          | 50  | 0   | 100 |
| 6                      | Bright Red     | 100 | 0   | 0   |
| 7                      | Purple         | 100 | 0   | 50  |
| 8                      | Bright Magenta | 100 | 0   | 100 |
| 9                      | Green          | 0   | 50  | 0   |
| 10                     | Cyan           | 0   | 50  | 50  |
| 11                     | Sky Blue       | 0   | 50  | 100 |
| 12                     | Yellow         | 50  | 50  | 0   |
| 13                     | White          | 50  | 50  | 50  |
| 14                     | Pastel Blue    | 50  | 50  | 100 |
| 15                     | Orange         | 100 | 50  | 0   |
| 16                     | Pink           | 100 | 50  | 50  |
| 17                     | Pastel Magenta | 100 | 50  | 100 |
| 18                     | Bright Green   | 0   | 100 | 0   |
| 19                     | Sea Green      | 0   | 100 | 50  |
| 20                     | Bright Cyan    | 0   | 100 | 100 |
| 21                     | Lime           | 50  | 100 | 0   |
| 22                     | Pastel Green   | 50  | 100 | 50  |
| 23                     | Pastel Cyan    | 50  | 100 | 100 |
| 24                     | Bright Yellow  | 100 | 100 | 0   |
| 25                     | Pastel Yellow  | 100 | 100 | 50  |
| 26                     | Bright White   | 100 | 100 | 100 |

To calculate the colour value:

- Red
  - 0% =\> do not add anything
  - 50% =\> add 3
  - 100% =\> add 6
- Green
  - 0% =\> do not add anything
  - 50% =\> add 9
  - 100% =\> add 18
- Blue
  - 0% =\> do not add anything
  - 50% =\> add 1
  - 100% =\> add 2

------------------------------------------------------------------------

## Green Screen Colours

On a green screen (where all colours are shades of green), the colours (in the software/firmware colours), are in order of increasing intensity. So that black is very dark, and white is bright green, and colour 13 is a medium green. (Thanks to Mark Rison for this information)
