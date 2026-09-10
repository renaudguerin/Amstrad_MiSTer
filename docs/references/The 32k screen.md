<!--
Converted from "The 32k screen.html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 1 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# The 32K screen

This document describes how the CRTC can be programmed so that it can display graphics from a range of 32k without. Once initialised, this effect does not require maintenance.

The CPC display hardware generates a memory address by using the CRTC MA (MA0-MA13) and RA (RA0-RA5) outputs in the following way:

| Memory address Signal | Signal Source | Signal name |
|-----------------------|---------------|-------------|
| A15                   | 6845          | MA13        |
| A14                   | 6845          | MA12        |
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

Notes:

- From this we can see that, MA11, MA10, RA3, RA4 and RA5 are not used to generate the memory address.
- CCLK is supplied by the Gate-Array to fetch two bytes for each CRTC character.
- The display base is defined by CRTC register 12 and 13.
- MA0-MA13 outputs allow a 16K range to be accessed from the display base, but the CPC display hardware fetches two bytes for every CRTC character, and therefore the CPC display hardware allows a 32k range to be access from the display base.
- the current MA12 and MA13 outputs define the 16K RAM block to fetch the display data from, the other signals define the offset within that block.

The CRTC has a internal MA register which has the same value as the MA signal outputs. This register is incremented for each CRTC character and reloaded at the start of a CRTC frame from R12 and R13.

Using R12 and R13 we can define the initial state of the internal MA register in the following way:

| CRTC Register | Register Bit | CRTC signal |
|---------------|--------------|-------------|
| R12           | b7           | not used    |
| R12           | b6           | not used    |
| R12           | b5           | MA13        |
| R12           | b4           | MA12        |
| R12           | b3           | MA11        |
| R12           | b2           | MA10        |
| R12           | b1           | MA9         |
| R12           | b0           | MA8         |
| R13           | b7           | MA7         |
| R13           | b6           | MA6         |
| R13           | b5           | MA5         |
| R13           | b4           | MA4         |
| R13           | b3           | MA3         |
| R13           | b2           | MA2         |
| R13           | b1           | MA1         |
| R13           | b0           | MA0         |

Notes:

- From this we can see that it is possible to set MA10 and MA11, and although these are not used to generate the memory address to fetch display data from, they can be used to indirectly affect MA12 and MA13 and therefore, indirectly, they can affect the generated memory address.
- Although MA11 and MA10 are not used to generate a memory address to fetch display data from, they play a role in allowing the display hardware to fetch data accross a 16k boundary.

If we want to display data from a 32k range, then MA12 output MUST increment from the initial setting defined by R12.

Depending on the initial setting of R12 and R13, the internal MA register must increment between:

- 1 character (when MA11=MA10=MA9=MA8=MA7=MA6=MA5=MA4=MA3=MA2=MA1=MA0="1" or MA11-MA0 = 4095) and
- 4096 characters (when MA11=MA10=MA9=MA8=MA7=MA6=MA5=MA4=MA3=MA2=MA1=MA0="0" or MA11-MA0 = 0).

## Overscan

Using R12 we can set MA11 and MA10, and from this we can create a overscan screen (a screen which fills the entire monitor display), which does not require any maintenance. Once this effect has been initialised, no furthur CPU time is require to maintain the display.

Properties of the overscan screen:

- Width is 48 CRTC characters,
- Height is 30 CRTC character lines,
- Each character is 8 scanlines tall
- the screen will use 24K of RAM.

The screen has 1440 characters, and the internal MA counter of the CRTC will increment by 1440 from the initial MA settings defined by R12 and R13.

The following table shows the settings of MA11 and MA10 for the maximum (&3ff) and minimum (&000) scroll settings. In this example, the scroll setting is defined by MA9-MA0.

| MA11 | MA10 | Number of characters for MA12 to change |
|------|------|-----------------------------------------|
| 0    | 0    | 3074-4096                               |
| 0    | 1    | 2049-3072                               |
| 1    | 0    | 1025-2048                               |
| 1    | 1    | 1-1024                                  |

From the table above we can see that:

- when MA11="0" and MA10="0" OR MA11="0" and MA10="1", MA12 will not change. The minimum number of characters required is larger than the number of characters in our visible display, and it is not possible to set a scroll offset that will cause MA12 to change.
- when MA11="1" and MA10="0" it is possible for MA12 to change, however we would require a scroll setting of at least &92a for MA12 to change.
- when MA11="1" and MA10="1" it is possible for MA12 to change for all scroll offset settings (&0-&3ff)

Therefore the best settings for our overscan screen are MA11="1" and MA10="1".

Example: \[ [highlighted](http://www.cpctech.org.uk/source/overscn.html) \| [original](http://www.cpctech.org.uk/source/overscn.asm) \]

Can we scroll this screen?

Yes, but the range is limited.

If MA11="1" and MA10="1" we are limited to scrolling 1024 characters. For a full scroll through the entire 32k range we would need to be able to scroll 4096 characters.

Scrolling 4096 characters requires using MA11 and MA10 for the scroll offset, and as we have already seen, when MA11 and MA10 are not "1", then at some point MA12 will not change, and the effect will be lost, and we will only see repeating graphics from a 16k range.
