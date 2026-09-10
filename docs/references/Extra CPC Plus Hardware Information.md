<!--
Converted from "Extra CPC Plus Hardware Information.html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 1 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# Extra CPC Plus Hardware Information

This document describes information about the CPC Plus hardware which is not documented in the official datasheet. These details have been found by running tests on a real CPC6128+ system.

There are two known versions of the CPC+ ASIC, but there may be more. It is possible to identify the version using the following procedure:

1.  Switch off computer (if it is on),
2.  Remove cartridge,
3.  Switch on computer,
4.  Observe the colour of the screen

If you see a "green-yellow" colour, you have a CPC+ with ASIC revision 1.

If you see a "purple/magenta" colour, you have a CPC+ with ASIC revision 2.

Richard Wilson has reported, that his CPC+ shows a blue colour.

Thankyou to Offset/Futurs for details of the ASIC versions.

## P.C.B

The circuit board (PCB) of the 464+ and 6128+ are identical, but, the 464+ curcuit board is missing the components for the disc interface, and the 6128+ is missing the components for the tape interface.

Hardware modifications exist which will add a cassette interface to the 6128+, and add a disc interface to the 464+. These modifications add the missing components to the circuit board.

## Cartridge

Thankyou to Peter Sorensen for his information which has been used for this part of the document.

The cartridge is simple in design. It has a small PCB with 2 IC's and a capacitor on it.

The PCB is marked "AMSTRO1" and "(C) AMSTRAD PLC 1990".

IC1 is the Game EPROM containing the game program and data. (This data is not encrypted or protected in any way)

IC2 is the "ACID" protection chip. ACID stands for "Amstrad Cartridge Identification Device". This IC has 16 pins and has the markings AMSTRAD 40908 1L03P1003, 8030EA1).

There are also 6 option links labeled LK1 to LK6.

Looking into the cartridge slot, the pin assignments are, from left-to-right:

TOP

|     |      |
|-----|------|
| a1  | +5v  |
| a2  | clk4 |
| a3  | a14  |
| a4  | a15  |
| a5  | a12  |
| a6  | a7   |
| a7  | a6   |
| a8  | a5   |
| a9  | a4   |
| a10 | a10  |
| a11 | nce  |
| a12 | d7   |
| a13 | d6   |
| a14 | d5   |
| a15 | d4   |
| a16 | d3   |
| a17 | cclr |
| a18 | 0v   |

BOTTOM

|     |     |
|-----|-----|
| b1  | +5v |
| b2  | a18 |
| b3  | a17 |
| b4  | a14 |
| b5  | a13 |
| b6  | a8  |
| b7  | a9  |
| b8  | a11 |
| b9  | a3  |
| b10 | a2  |
| b11 | a1  |
| b12 | a0  |
| b13 | d0  |
| b14 | d1  |
| b15 | d2  |
| b16 | sin |
| b17 | 0v  |
| b18 | 0v  |

The following is not confirmed:

- clk4, cclr, sin are signals for the ACID IC
- D7-D0, A18-A0 are signals for the EPROM

PLEASE NOTE THAT I CAN'T BE HELD RESPONSIBLE FOR ANY DAMAGE THAT MAY BE CAUSED TO YOUR CARTRIDGE OR COMPUTER FROM THE USE OF THIS CARTRIDGE CONNECTION INFORMATION!

## Furthur ASIC Information

This document provides additional "undocumented" information about the CPC+ ASIC custom chip.

The ASIC custom chip comprises:

- integration of printer port electronics,
- functionality of the RAM management PAL,
- functionality of the CRTC 6845,
- functionality of the 8255 PPI
- functionality of the "Gate array" (the Gate Array of the standard CPC)
- enhanced features providing Sprites, Raster Interrupts, smooth scrolling and DMA driven sound.

When the ASIC special features have not been enabled, the CPC+ acts almost identically to the old CPC series. There are however a few minor differences which cause software incompatibilities. But if these problems are considered software can be modified to run on all CPC/CPC+ computers without any problems.

The special features of the ASIC are documented in the official Amstrad document.

## Timing

The CPC+ appears to be slightly different in respect to the timing. It appears all colour changes are 1/2 NOP later than they would be on a CPC.

When programming some CRTC changes some timings appear to be 2 us later!

## Ports

The CPC+ has the same ports and port address decoding as the CPC with the following exceptions:

In the CPC and CPC+ the Gate Array is accessed when bit 15 of the port address is 0, and bit 14 is 1. On the CPC+, performing a IN operation from the Gate Array has the same effect as an OUT. On the CPC, performing an IN on this port has no effect.

When a IN is performed from this port:

- 0x079 is always reported on a CPC6128+,
- 0x078 is always reported on a CPC464+,
- on the CPC 0x0ff is mostly reported ,

The resulting input value is always written.

This weird bug can be seen on the game "Into the Eagles Nest". The code that exhibits the bug is:

;; before entering this code, B has been set to 0x07f 0x05c3: LD C,&8C ;; set video mode OUT (C),C LD C,&10 ;; select border OUT (C),C LD C,&54 ;; select black colour IN A,(C) ;; !!!!!!!!

- On the CPC6128+, the bottom border is coloured green, (the green colour is the equivalent of OUTing with 0x079),

- On the CPC464+, the bottom border is purple, (the purple colour is the equivalent of OUTing with 0x078),

- on a CPC it remains black.

- In the CPC and CPC+ design, the CRTC is written to via 2 ports ("select register" and "write data"). On the CPC+, performing an IN operation on either of these ports has the same effect as performing an OUT. On the CPC, performing an IN on these ports has no effect.

  This bug can be reproduced from basic using the following code:

  OUT &BC00,6:? INP(&BD00)

  The result of this code causes the vertical displayed to be 0x079 on a CPC6128+ or 0x078 on a CPC464+.

## Interrupts

In interrupt mode 2, the CPU fetches a byte from the bus to form a lookup into a table of interrupt handler routines.

On the CPC this byte is normally 0x0ff, although this is not guaranteed.

On the CPC+ when the ASIC advanced features are \*not\* enabled, this byte is always 0x00.

Richard Wilson reports that on his CPC+, this byte is always 0x56.

On the CPC+ when the ASIC advanced features are enabled, this byte depends on the interrupt source (Raster Ints or DMA channel ints).

The official documents are not clear about the interrupt vector provided in mode 2, I found the following are correct:

| Interrupt Source  | Vector |
|-------------------|--------|
| DMA Channel 0     | 4      |
| DMA Channel 1     | 2      |
| DMA Channel 2     | 0      |
| Raster Interrupts | 6      |

The interrupt vector is therefore ((IVR & 0x0f8) \| Vector).

When the ASIC raster interrupt is active, there are no interrupts caused by the standard CPC raster int.

When a ASIC raster interrupt occurs, the top-bit of the Gate Array 6-bit interrupt counter is reset, so that if the CPC raster interrupt is re-enabled it will not occur closer than 32 lines.

It is possible to use the Z80 interrupt mode 0 with the CPC+ because the 8-bit interrupt-vector can be predicted and is reliable.

See the document about using interrupt mode 0 with the CPC+ for more information.

## PPI

The Intel 8255 PPI is "emulated" in the ASIC. However, the emulation is not perfect, and this is the reason why some programs do not work correctly.

In the programs that are affected the keyboard does not respond, or there is no sound, or both.

There are two differences between the ASIC implementation of the 8255 PPI and the 8255 PPI inside the CPC:

1.  When the PPI Control register is re-programmed the output latches are not cleared. In the 8255 PPI used by the CPC and KC Compact the port output \*is\* cleared. Some games use this fact in the keyboard scanning algorithm, and as a result they will not work on the CPC+.
2.  Port B is always defined as input and Port C is always defined as output.

The PPI in the ASIC is a cut-down version of the 8255PPI. It has all the operations used by the CPC including the bit set/reset feature. It may not have the functions to set the port operation (mode 0, 1 or 2) as these features are not used on the CPC (to my knowledge).

### PPI Port A

When this port is set to output, any byte written to this port can be read.

When this port is set to input mode, this port returns the input data only. Writing to this port does not change the value read.

When this port is set to input mode, the value present on the port to the PSG is 0x0ff. (I found this out by selecting a PSG register, setting port A to input and then performing a PSG write data). There is no change even when a write is performed to the port.

### PPI Port B

When this port is set to input or output, and data is read it will always return the same data. Writing to this port has no effect.

The data that can be read is the same as the data that can be read from the 8255 in the CPC.

### PPI Port C

When this port is set to input or output, any byte written to this port can be read back without any change. The byte is not affected by the input/output status of either half. It appears that you cannot define the input/output status of each half of this port as you can on a real 8255.

### PPI Control port

Writing to this port will set the I/O status or perform the bit set/reset feature.

Writing to this port and then reading the data back gives the following results:

Writing data in the range 0x00-0x07f returns 0x0ff, 0x080-0x08f returns 0, 0x090-0x09f returns 0x0ff, 0x0a0-0x0af returns 0, 0x0b0-0x0bf returns 0x0ff, 0x0c0-0x0cf returns 0, 0x0d0-0x0df returns 0x0ff, 0x0e0-0x0ef returns 0, 0x0f0-0x0ff returns 0x0ff.

Changing the input/output status of any port does not clear it.

### Notes

- Since the 8255 emulation in the ASIC lacks many features in a real 8255, it is doubtful if the mode operations will work. From the above, it appears that the input/output status of Port B or Port C cannot be controlled.The only port that the input/output status appears to make a difference is with port A.
- The ASIC chip is designed to be used in the CPC+ only, and since port B is always used as input and port C is always used as output, it makes sense that the designers did not waste space on the chip to recreate the features of the original chip that were not used.

## CRTC

Please see the document describing the basic operation of the CRTC.

Since this type exists only on the CPC+, many detection programs enable the ASIC ram, and detect if it is available. If it is, a CRTC type 3 is assumed.

When reading, the following register data is returned:

| 6845 Selected Register | 6845 Register data returned |
|------------------------|-----------------------------|
| 0                      | 16                          |
| 1                      | 17                          |
| 2                      | Status 1 (note 1)           |
| 3                      | Status 2 (note 1)           |
| 4                      | 12                          |
| 5                      | 13                          |
| 6                      | (note 2)                    |
| 7                      | (note 2)                    |
| 8                      | 16                          |
| 9                      | 17                          |
| 10                     | Status 1 (note 1)           |
| 11                     | Status 2 (note 1)           |
| 12                     | 12                          |
| 13                     | 13                          |
| 14                     | (note 2)                    |
| 15                     | (note 2)                    |
| 16                     | 16                          |
| 17                     | 17                          |
| 18                     | Status 1 (note 1)           |
| 19                     | Status 2 (note 1)           |
| 20                     | 12                          |
| 21                     | 13                          |
| 22                     | (note 2)                    |
| 23                     | (note 2)                    |
| 24                     | 16                          |
| 25                     | 17                          |
| 26                     | Status 1 (note 1)           |
| 27                     | Status 2 (note 1)           |
| 28                     | 12                          |
| 29                     | 13                          |
| 30                     | (note 2)                    |
| 31                     | (note 2)                    |

Note:

1.  These do not appear to be actual CRTC registers. When read, they appear to give the status of the internal operation of the CRTC. I have named them "Status 1" and "Status 2" because they contain different information. These status registers are only present in the CPC+ version of the CRTC. I believe these registers were used to test the operation of the CRTC during development.
2.  It is not clear if these registers hold any data, they always return "0".

### Status 1

When Status register 1 is read, the bits have the following meaning:

|       |                                                              |
|-------|--------------------------------------------------------------|
| Bit 7 | (function of this bit is unknown)                            |
| Bit 6 | (function of this bit is unknown)                            |
| Bit 5 | 0: CRTC is on last line of VSYNC                             |
| Bit 4 | 0: CRTC is on last char of HSYNC                             |
| Bit 3 | 0: CRTC Horizontal Count == Horizontal Sync Position (Reg 2) |
| Bit 2 | 0: CRTC Horizontal Count == Horizontal Displayed (Reg 1)     |
| Bit 1 | 0: CRTC Horizontal Count == (Horizontal Total/2)             |
| Bit 0 | 0: CRTC Horizontal Count != Horizontal Total                 |

### Status 2

When Status register 2 is read, the bits have the following meaning:

|       |                                   |
|-------|-----------------------------------|
| Bit 7 | 0: when RC!=0                     |
| Bit 6 | (function of this bit is unknown) |
| Bit 5 | 0: when RC==R9                    |
| Bit 4 | (function of this bit is unknown) |
| Bit 3 | (function of this bit is unknown) |
| Bit 2 | (function of this bit is unknown) |
| Bit 1 | (function of this bit is unknown) |
| Bit 0 | (function of this bit is unknown) |

## ASIC Ram Information

Before the ASIC ram can be accessed, the ASIC must be enabled. A special sequence must be programmed before the features are unlocked. (See the official ASIC documentation for details of this.)

When the ASIC Ram is paged in it occupies the address space from &4000-&7fff.

Not all of the memory in this 16k address space is used. In this document the areas that cannot be used will be called "invalid areas". The areas that can be used will be called "valid areas".

Over half of the memory in the 16k address range is invalid. No useful information can be stored in this area. (Writing to this memory has no effect. Data that is written cannot be read back, because this ram always returns the same data)

There are a number of limited ranges. The information below describes what happens when read and writes are performed in these valid areas. Please read the official documentation for furthur information on their exact functions.

### Invalid areas\

In invalid areas, on my CPC+, the data is a mixture of 0x0b0 and 0x0b1, when read in a 16k block, but 0x07e or 0x07f, when reading the same location over and over again. (The mixture of 0x0b0 and 0x0b1 appear to be random)

From furthur investigation it appears that reading from a even address in the invalid area will always give an even result, reading from a odd address may give a odd or even result.

I have yet to confirm if the data is actually useful. Could this data be the result of data fetched from the screen?

### Sprite Data

Writing to any byte in the range &4000-&5000 will be masked with &0F. Therefore, when reading, the result is:

DATA_READ = DATA_WRITTEN & 0x0f

#### Sprite Position and magnification information:

Offsets are given from the base of the sprite information for each sprite.

|          |                           |
|----------|---------------------------|
| Offset 0 | Low byte of X coordinate  |
| Offset 1 | High byte of X coordinate |
| Offset 2 | Low byte of Y coordinate  |
| Offset 3 | High byte of Y coordinate |
| Offset 4 | Sprite magnification      |
| Offset 5 | Sprite magnification      |
| Offset 6 | Sprite magnification      |
| Offset 7 | Sprite magnification      |

When read, offset 4,5,6 and 7 (in that order) are a mirror of reading offset 0,1,2 and 3 (in that order).

When reading offsets 0,1,2 and 3, the following apply:

Offset 0 and 2: any written byte is readable, the same value that is written is readable!

BYTE_READ = BYTE_WRITTEN;

Offset 1: If byte written & 0x03 is 0,1 or 2, the value & 0x03 is read back. If the byte written & 0x03 is 3 then 0x0ff is read back.

if ((BYTE_WRITTEN & 0x03)==0x03) { BYTE_READ = 0x0ff; } else { BYTE_READ = BYTE_WRITTEN & 0x03; }

The range is &000-&3FF (0-1023) or &FF00-&2FF (-256 to +767). The sprite coordinates are based on the internal CRTC horizontal character counter. If CRTC R0\>64, then the sprites may repeat horizontally.

Offset 3: If byte written & 0x01 is 0, the value & 0x01 is read back. If the byte written & 0x01 is 1, then 0x0ff is read back.

if ((BYTE_WRITTEN & 0x01)==0x01) { BYTE_READ = 0x0ff; } else { BYTE_READ = BYTE_WRITTEN & 0x01; }

The range is &000-&1FF (0-512) or &FF00-&FF (-256 to +256). The sprite coordinates are based on the internal CRTC character row counter and CRTC raster row counter.

This value is \*not\* dependant on Vertical Displayed.

### Palette:

|                |                                           |
|----------------|-------------------------------------------|
| Entry Offset 0 | Upper nibble is Red, lower nibble is blue |
| Entry Offset 1 | Lower nibble is Green.                    |

When a byte is written to offset 1 and then read back it's value is returned & 0x0f. When writing to offset 0, and then read back, it's value is the same as written.

For Offset 0:

BYTE_READ = BYTE_WRITTEN;

For Offset 1:

BYTE_READ = BYTE_WRITTEN & 0x0f;

When a colour is set using the old method (using I/O), the corresponding entry in the ASIC Ram is updated with the RGB of the colour chosen. The table below shows the "hardware colour index" and the corresponding R,G,B for the colour.

| Hardware Colour Index | Colour Name    | R   | G   | B   |
|-----------------------|----------------|-----|-----|-----|
| 0                     | White          | 6   | 6   | 6   |
| 1                     | White          | 6   | 6   | 6   |
| 2                     | Sea Green      | 0   | 15  | 6   |
| 3                     | Pastel Yellow  | 15  | 15  | 6   |
| 4                     | Blue           | 0   | 0   | 6   |
| 5                     | Purple         | 15  | 0   | 6   |
| 6                     | Cyan           | 0   | 6   | 6   |
| 7                     | Pink           | 15  | 6   | 6   |
| 8                     | Purple         | 15  | 0   | 6   |
| 9                     | Pastel Yellow  | 15  | 15  | 6   |
| 10                    | Bright Yellow  | 15  | 15  | 0   |
| 11                    | Bright White   | 15  | 15  | 15  |
| 12                    | Bright Red     | 15  | 0   | 0   |
| 13                    | Bright Magenta | 15  | 0   | 15  |
| 14                    | Orange         | 15  | 6   | 0   |
| 15                    | Pastel Magenta | 15  | 6   | 15  |
| 16                    | Blue           | 0   | 0   | 6   |
| 17                    | Sea Green      | 0   | 15  | 6   |
| 18                    | Bright Green   | 0   | 15  | 0   |
| 19                    | Bright Cyan    | 0   | 15  | 15  |
| 20                    | Black          | 0   | 0   | 0   |
| 21                    | Bright Blue    | 0   | 0   | 15  |
| 22                    | Green          | 0   | 6   | 0   |
| 23                    | Sky Blue       | 0   | 6   | 15  |
| 24                    | Magenta        | 6   | 0   | 6   |
| 25                    | Pastel Green   | 6   | 15  | 6   |
| 26                    | Lime           | 6   | 15  | 0   |
| 27                    | Pastel Cyan    | 6   | 15  | 15  |
| 28                    | Red            | 6   | 0   | 0   |
| 29                    | Mauve          | 6   | 0   | 15  |
| 30                    | Yellow         | 6   | 6   | 0   |
| 31                    | Pastel Blue    | 6   | 6   | 15  |

The sprite colours can not be changed using the old method. To change the sprite colours, the ASIC must be enabled and the ASIC ram paged into the memory space.

### Misc registers

Reading from 0x06800-0x06807 will read the same values as for invalid data.

Writing to &6800 will set the interrupt raster line,\
Writing to &6801 will set the screen split line,\
Writing to &6802 and &6803 will set the screen split address,\
Writing to &6804 will set the soft scroll,\
Writing to &6805 will set the interrupt vector (Arnold V specification 1.4).\
Writing to &6806 has no effect.\
Writing to &6807 has no effect.

For the interrupt raster line, soft scroll and split line please see furthur in this document for details of how the ASIC generates the comparison value to compare against each of these programmed values.

There is a bug with the interrupt vector feature. In the 1.4 specification it states that if bit 0 is set to 0, then interrupts will automatically be cleared before executing the interrupt service routine.

However, I found that with this bit set to 0, there is a bug.

Example showing bug: \[ [highlighted](http://www.cpctech.org.uk/source/asicbug.html) \| [original](http://www.cpctech.org.uk/source/asicbug.asm) \]

A demo program used a single DMA channel to issue an interrupt, a few lines later a raster interrupt would issue an interrupt. The first interrupt executed the correct interrupt handler routine (0,2 or 4), the second executed the interrupt handler for vector of 0 (the raster interrupt is vector 6). This was clearly wrong! My advice is that you should not use the automatic interrupt clear feature.

### Analogue inputs

Without any peripherals attached, the following readings were observed:

ADC0: 0x03f\
ADC1: 0x03f\
ADC2: 0x03f\
ADC3: 0x03f\
ADC4: 0x03f\
ADC5: 0x000\
ADC6: 0x03f\
ADC7: 0x000

Writing to these appears to have no effect, with the readings not changing.

I have tested the analogue port with different PC analogue controllers to see how well they work, but I have not been completely successful. See the results at the end of this document.

### DMA channels and interrupt control

When reading &6c00-&6c0f will give result of reading DCSR register.

The DMA channels data are located at &6c00, &6c04 and &6c08, for channels 0, 1 and 2 in that order.

Writing to offset 0 and 1 will set the DMA channel pointer. Writing to offset 2 will set the channel prescalar. Writing to offset 3 has no effect.

The official documentation is not clear about the bits in the DCSR register (&6c0f), I found the following are correct:

|       |                         |
|-------|-------------------------|
| Bit 7 | Raster Int              |
| Bit 6 | DMA Channel 0 Interrupt |
| Bit 5 | DMA Channel 1 Interrupt |
| Bit 4 | DMA Channel 2 Interrupt |
| Bit 3 | not used                |
| Bit 2 | DMA Channel 2 Enable    |
| Bit 1 | DMA Channel 1 Enable    |
| Bit 0 | DMA Channel 0 Enable    |

Each DMA opcode is two bytes, and is always fetched from an even address. The top 4-bits of the opcode fetched in the DMA cycle are used to specify the function to perform by the DMA.

I found the following rule:

|       |                               |
|-------|-------------------------------|
| bit 3 | not used                      |
| Bit 2 | Nop/loop/Int/Stop instruction |
| Bit 1 | Repeat N instruction          |
| Bit 0 | Pause instruction.            |

If (opcode & 0x07 == 0) then write data to register else perform function defined by bits.

------------------------------------------------------------------------

### Describing how various comparisons are calculated

This part of the document explains how the comparison values are calculated which are compared against the programmed interrupt line number, split line number and sprite coordinates.

Since much of this part of the documentation relies on the operation of the CRTC, you are advised to read the documents on the CRTC, and specifically the documents describing the CRTC implementation in the ASIC (known as type 3).

### Sprite Y coordinates

The ASIC generates the "compare line" from the CRTC Line Counter and CRTC Raster Counter. The "compare line" is the line number that is compared to the sprite Y range to determine if a line of sprite should be displayed. The sprite Y range is defined as the Y coordinate range occupied by the sprite with magnification.

The following equation is used to generate the "compare line":

(LineCounter\<\<3) \| (RasterCounter & 0x07)

### Sprite X coordinates

The CRTC horizontal character count is used to determine where the sprite line should be displayed on a line it is active on.

The CRTC char containing the first pixel (leftmost pixel) is:

((Sprite_X & 0x0fff8)\>\>3)

The pixel within the char is:

Sprite_X & 0x07

### Raster Interrupts

The raster interrupt is not dependant on the width of the horizontal sync. It's position is dependant on the position of the horizontal sync. The raster interrupt occurs 10us following the HSYNC start.

The following comparison is made:

if ((LineCounter == ((InterruptLine\>\>3) & 0x01f) && ( (RasterCounter == ((InterruptLine & 0x07))

InterruptLine is the value programmed to ASIC interrupt line register. LineCounter is the CRTC character line counter. Raster counter is the CRTC Raster Counter.

### Screen Split

A 8-bit number is formed from the LineCounter and RasterCounter in the following way:

((LineCounter & 0x01f)\<\<3) \| (RasterCounter & 0x07)

This is then compared against the programmed split line.

The split line can be re-programmed at any time. If a split line is programmed and the CRTC is currently on that line, the split will still occur at the end of the line.

Since the comparison is 8-bit, in a frame with 312 lines, it is possible a screen split will occur twice.

The first at the programmed line, and the next after the counter has wrapped around (at 256 + programmed line).

There is a single case with a 312 line frame, with a programmed value of 56, where the screen split bug occurs.

With this bug, the first split occurs normally, but the second split occurs at the very end of the frame. The result is a single visible split after line 0 of the frame. (The second split effectively overrides the second split that would occur).

The only way to avoid this condition is to introduce extra lines into the frame, or to not program the value.

Extra lines can be appended by programming reg 5 of the CRTC to a value other than 0.

A second way to eliminate the problem without extra lines is to use the vertical scroll of the ASIC. When the programmed vertical scroll is 7, with the bug occuring, the split will occur normally. It appears that the scroll changes the screen address in preference to the screen split causing the problem to disapear.

------------------------------------------------------------------------

### Equation Notes:

The equation notation in this document uses standard C.

For those who do not know this language, here is a quick list of the operators and their functions:

\| - bitwise/logical OR\
& - bitwise/logical AND\
\<\< - shift left by number of bits specified\
\>\> - shift right by number of bits specified\
== - equals comparison\
= - assign value to variable\
&& - and, as used in "if a==b and c==d ..."

Hex numbers are represented with a prefix of 0x. e.g. 0x0abcd is the hex number abcd.

In if statements, the operations enclosed in { } brackets following the if are executed if the condition is true, otherwise the operations enclosed in the brackets following the "else" statement are executed.

------------------------------------------------------------------------

In the ASIC documentation it states that following the leading edge of the HSYNC, there is one dead cycle, followed by an instruction fetch for each channel. For each cycle 1 2 byte instruction is fetched.

(since an instruction is 2 bytes long, each instruction fetch will require 1us)

If all channels are enabled:

(in 1us, original gate array accessed two bytes of information)

0: dead cycle 1: opcode for channel 0 2: opcode for channel 1 3: opcode for channel 2 4: execute channel 0 5: execute channel 1 6: execute channel 2

1 cycle execute per channel, except load which requires 8 cycles.

For raster int it appears to start at least 10 cycles following HSYNC beginning. To be checked!!!!

### Analogue tests

### Competition Pro

When competition pro attached only.

6808 = left/right\
6809 = up/down\
680a = Z button\
680b = y button

pressing X button sets bit 4 of all keys on keyboard matrix, pressing A button sets bit 5 of all keys on keyboard matrix

pressing delete causes all lines to go to 0x080, except line 15. All keys except delete work as expected.

keyboard normally all keys pressed gives 0-9 0x0ff\
10 is joystick\
anything above 10 is zero.

### joypad and analogue joypad

pressing delete causes all key lines to go to 0x080. pressing joypad 0 fire 1,2 sets all lines to 0x010 or 0x020 (e.g. same as pressing X,A), left does nothing, right produces 0x08 in the upper lines (10-\>) and after a bit of time, the remaining keyboard lines reflect this. up gives 0x01 in the 10-15 range. Down gives nothing.

Joypad 1 works as expected.

Microsoft sidewinder doesn't work!
