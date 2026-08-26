<!--
Converted from "UM6845 Cathode Ray Tube Controller.html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 12 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# UM6845R/RA/RB - CRT Controller

## Features

- Single +5 volt (±5%) power supply
- Alphanumeric and limited graphics capabilities
- Fully programmable display (rows, columns, blanking, etc).
- Interlaced or non-interlaced scan
- 50/60Hz operation
- Fully programmable operation
- External light gun capability
- Capable of addressing up to 16K character Video Display RAM
- No DMA required
- Compatible with SYS6845R
- Straight binary addressing for Video Display RAM

## General Description

The UM6845R is a CRT Controller intended to provide capability for interfacing any microprocessor family to CRT or TV-type raster scan displays. A unique feature is the inclusion of several modes of operation, so that the system designer can configure the system with a wide assortment of techniques.

#### Pin Configuration

![](http://www.cpctech.org.uk/um5.gif)

#### Block Diagram

![](http://www.cpctech.org.uk/um4.gif)

### Absolute Maximum Ratings

### D.C. Electrical Characteristics

(Vcc = 5.0V +-5%, TA=0-70C, unless otherwise noted).

| Symbol | Characteristics | Min. | Typ. | Max. | Units |  |
|----|----|----|----|----|----|----|
| VIH | Input High Voltage | 2.0 |  | Vcc | V |  |
| VIL | Input Low Voltage | -0.3 |  | 0.8 | V |  |
| IIN | Input Leakage (02, /W/R, /RES, /CS, RS, LPEN, CCLK | \- |  | 2.5 | uA |  |
| ITSI | Three-State Input Leakage (DB0-DB7) VIN = 0.4 to 2.4V | -10.0 |  | \- | V |  |
| VOH | Output High Voltage | ILOAD = -205 uA (DB0-DB7) ILOAD = -100 uA (all others) | 2.4 |  | \- | V |
| VOL | Output Low Voltage ILOAD = 1.6mA | \- |  | 0.4 | V |  |
| PD | Power Dissipation | \- | 325 | 650 | mW |  |
| CIN | Input Capacitance |  |  |  |  |  |
| CIN | 02, /W/R, /RES, /CS, RS, LPEN, CCLK | \- |  | 10.0 | pF |  |
| CIN | DB0-DB7 | \- |  | 12.5 | pF |  |
| COUT | Output Capacitance | \- |  | 10.0 | pF |  |

### TEST LOAD

### WRITE TIMING CHARACTERISTICS (Vcc = 5.0V +-5%, TA = 0-70C, unless otherwise noted)

| Symbol | Characteristics       | Min. | Max. | Min. | Max. | Min. | Max. | Units |
|--------|-----------------------|------|------|------|------|------|------|-------|
| tCYC   | Cycle Time            | 1.0  | \-   | 0.5  | \-   | 0.33 | \-   | us    |
| PWEH   | E Pulse Width, High   | 440  | \-   | 200  | \-   | 150  | \-   | ns    |
| PWEL   | E Pulse Width, Low    | 420  | \-   | 190  | \-   | 140  | \-   | ns    |
| tAS    | Address Set-Up Time   | 80   | \-   | 40   | \-   | 30   | \-   | ns    |
| tAH    | Address Hold Time     | 0    | \-   | 0    | \-   | 0    | \-   | ns    |
| tCS    | /W/R, /CS Set-Up Time | 80   | \-   | 40   | \-   | 30   | \-   | ns    |
| tCH    | /W/R, /CS Hold Time   | 0    | \-   | 0    | \-   | 0    | \-   | ns    |
| tDSW   | Data Bus Set-Up Time  | 165  | \-   | 60   | \-   | 60   | \-   | ns    |
| tDHW   | Data Bus Hold Time    | 10   | \-   | 10   | \-   | 10   | \-   | ns    |

(tr and tf = 10 to 30 ns)

### READ TIMING CHARACTERISTICS (Vcc = 5.0V +-5%, TA = 0-70C, unless otherwise noted)

| Symbol | Characteristics       | Min. | Max. | Min. | Max. | Min. | Max. | Units |
|--------|-----------------------|------|------|------|------|------|------|-------|
| tCYC   | Cycle Time            | 1.0  | \-   | 0.5  | \-   | 0.33 | \-   | us    |
| PWEH   | E Pulse Width, High   | 440  | \-   | 200  | \-   | 150  | \-   | ns    |
| PWEL   | E Pulse Width, Low    | 420  | \-   | 190  | \-   | 140  | \-   | ns    |
| tAS    | Address Set-Up Time   | 80   | \-   | 40   | \-   | 30   | \-   | ns    |
| tAH    | Address Hold Time     | 0    | \-   | 0    | \-   | 0    | \-   | ns    |
| tCS    | /W/R, /CS Set-Up Time | 80   | \-   | 40   | \-   | 30   | \-   | ns    |
| tCH    | /W/R, /CS Hold Time   | \-   | 290  | \-   | 150  | \-   | 100  | ns    |
| tDSW   | Data Bus Set-Up Time  | 20   | 60   | 20   | 60   | 20   | 60   | ns    |
| tDHW   | Data Bus Hold Time    | 40   | \-   | 40   | \-   | 40   | \-   | ns    |

(tr and tf = 10 to 30 ns)

### MEMORY AND VIDEO INTERFACE CHARACTERISTICS

| Symbol | Parameter                          | Min. | Max. | Units |     |
|--------|------------------------------------|------|------|-------|-----|
| TCCH   | Maximum Clock Pulse Width, High    | 200  |      |       | ns  |
| TCCV   | Clock Frequency                    |      |      | 2.5   | Mhz |
| Tr,Tf  | Rise and Fall Time for Clock Input |      |      | 20    | ns  |
| tMAD   | Memory Address Delay Time          |      | 100  | 160   | ns  |
| tRAD   | Raster Address Delay Time          |      | 100  | 160   | ns  |
| tDTD   | Display Timing Delay Time          |      | 160  | 300   | ns  |
| tHSD   | Horizontal Sync Delay Time         |      | 160  | 300   | ns  |
| tVSD   | Vertical Sync Delay Time           |      | 160  | 300   | ns  |
| tCDD   | Cursor Display Timing Delay Time   |      | 160  | 300   | ns  |

### LIGHT PEN STROBE TIMING

NOTE: "Safe" time positions for LPEN positive edge to cause address n+2 to load into Light Pen Register. tLP2 and tLP1 are time positions causing uncertain results.

| Symbol | Characteristics    | Min. | Max. | Min. | Max. | Min. | Max. | Unit |
|--------|--------------------|------|------|------|------|------|------|------|
| tLPH   | LPEN Strobe Width  | 100  | \-   | 100  | \-   | 100  | \-   | ns   |
| tLP1   | LPEN to CCLK Delay | \-   | 120  | \-   | 120  | \-   | 120  | ns   |
| tLP2   | CCLK to LPEN Delay | \-   | 0    | \-   | 0    | \-   | 0    | ns   |

## Pin Description

### MPU INTERFACE SIGNAL DESCRIPTION

**E (Enable)**  
The enable signal is the system input and is used to trigger all data transfers between the system microprocessor and the UM6845R. Since there is no maximum limit to allowable E cycle time, it is not necessary for it to be a continuous clock. This capability permits the UM6845R to be easily interfaced to non-6500 compatible microprocessors.\
\

**R/W (Read Write)**  
The R/W signal is generated by the microprocessor and is used to control the direction of data transfers. A high on the R/W pin allows the processor to read the data supplied by the UM6845R; a low on the R/W pin allows a write to the UM6845R.\
\

**/CS (Chip Select)**  
The Chip Select Input is normally connected to the processor address bus either directly or through a decoder. The UM6845R is selected when /CS is low.\
\

**/RS (Register Select**  
The Register Select Input is used to access internal registers. A low on this pin permits writes into the Address Register and reads from the Status Register. The contents of the Address Register is the identify of the register accessed when RS is high.\
\

**DB<sub>0</sub>--DB<sub>7</sub> (Data Bus)**  
The DB<sub>0</sub>--DB<sub>7</sub> pins are the eight data lines used for transfer of data between the processor and the UM6845R. These lines are bi-directional and are normally high-impedance except during read/write cycles when the chip is selected.

### VIDEO INTERFACE SIGNAL DESCRIPTION

**HSYNC (Horizontal Sync)**  
The HSYNC signal is an active-high output used to determine the horizontal position of displayed text. It may drive a CRT monitor directly or may be used for composite video generation. HSYNC time position and width are fully programmable.\
\

**VSYNC (Vertical Sync)**  
The VSYNC signal is an active-high output used to determine the vertical position of displayed text. Like HSYNC, VSYNC may be used to drive a CRT monitor or composite video generation circuits. VSYNC position and width are both fully programmable.\
\

**DISPLAY ENABLE**  
The DISPLAY ENABLE signal is an active-high output and is used to indicate when the UM6845R is generating active display information. The number of horizontal displayed characters and the number of vertical displayed characters are both fully programmable and together are used to generate the DISPLAY ENABLE signal.\
\

**CURSOR**  
The CURSOR signal is an active-high output and is used to indicate when the scan coincides with the programmed cursor position. The cursor position may be programmed to be any character in the address field. Furthurmore, within the character, the cursor may be programmed to be any block of scan lines, since the start scan line and the end scan line are both programmable.\
\

**LPEN**  
The LPEN signal is an edge-sensitive input and is used to load the Internal Light Pen Register with the contents of the Refresh Scan Counter at the time he active edge occurs. The active edge of LPEN is the low-to-high transition.\
\

**CCLK**  
The CCLK signal is the character timing clock input and is used as the same time base for all internal count/control functions.\
\

**/RES**  
The /RES signal-is an active-low input used to initialise all internal scan counter circuits. When /RES is low, all internal counters are stopped and cleared, all scan and video outputs are low, and control registers are unaffected. /RES must stay low for at least one CCLK period. All scan timing is initiated when /RES goes high. In this way, /RES can be used to synchronise display frame timing with line frequency.

### MEMORY ADDRESS SIGNAL DESCRIPTION

**MA0--MA13 (Video Display RAM Address Lines)**  
These signals are active-high outputs and are used to address the Video Display RAM for character storage and display operations. The starting scan address is fully programmable and the ending scan address is determined by the total number of characters displayed, which is also programmable, in terms of characters/line and lines/frame.

- **\* Binary Addressing**\
  \
  Characters are stored in successive memory locations. Thus, the software must be developed so that row and column co-ordinates are translated to sequentially-numberered addresses for video display memory operations.

\
\

**RA0-RA4 (Raster Address Lines)**

These signals are active-high outputs and are used to select each raster scan within an individual character row. The number of raster scan lines is programmable and determines the character height, including spaces between character rows.

**Figure 1. Video Display Format** \[Video Display Format Diagram\]

## Description of Internal Registers

Figure 1 illustrates the former of a typical video display and shows the functions of the various UM6845R internal registers. Figure 2 illustrates vertical and horizontal timing. Figure 3 illustrates the internal registers and indicates their address-selection and read/write capabilities.

### Address Register

This is a 5-bit register which is used as a "pointer" to direct UM6845R data transfers to and from the system MPU using the number of the desired register (0-31). When RS is low, this register may be loaded; when RS is high, the register selected is the one whose identity is stored in this register.

### Status Register

This 2-bit register is used to monitor the status of the CRTC as follows:\
\

|     |     |     |     |     |     |     |     |
|-----|-----|-----|-----|-----|-----|-----|-----|
| 7   | 6   | 5   | 4   | 3   | 2   | 1   | 0   |

\
\
**VERTICAL BLANKING**

- "0":Scan currently not in vertial blanking position of its timing\
  "1":Scan currently in its vertical blanking time\
  \

**LPEN REGISTER FULL**

- "0": This bit goes "0" whenever either register R16 or R17 is read by the MPU\
  "1": This bit goes "1" whenever a LPEN strobe occurs.

### Horizontal Total (R0)

This 8-bit register contains the total of displayed and non-displayed characters, minus one, per horizontal line. The frequency of HSYNC is thus determined by this register.

### Horizontal Displayed (R1)

This 8-bit register contains the number of displayed characters per horizontal line.

### Horizontal Sync Position (R2)

This 8-bit register contains the position of the HSYNC on the horizontal line. In terms of the character location number on the line. The position of the HSYNC determines the left-to-right location of the displayed text on the video screen. In this way, side margins are adjusted.

### Horizontal and Vertical SYNC Widths (R3)

This 4-bit register programs the width of HSYNC\
\

|  |  |  |  |  |  |  |  |
|----|----|----|----|----|----|----|----|
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|  |  |  |  | 8 | 4 | 2 | 1 |
|  |  |  |  | HSYNC (NUMBER OF CHARACTER CLOCK TIMES | HSYNC (NUMBER OF CHARACTER CLOCK TIMES | HSYNC (NUMBER OF CHARACTER CLOCK TIMES | HSYNC (NUMBER OF CHARACTER CLOCK TIMES |

\
\
VSYNC width is set to 16 scan line times.

### Vertical Total (R4)

The Vertical Total Register is a 7-bit register containing the total number of character rows in a frame, minus one. This register, along with R5, determines the overall frame rate, which should be close to the line frequency to ensure flicker-free appearance. If the frame time is adjusted to longer than the period of the line frequency, then /RES may be used to provide absolute synchronisation.

### Vertical Total Adjust (R5)

The Vertical Total Adjust Register is a 5-bit write-only register containing the number of additional scan lines needed to complete an entire frame scan and is intended as a fine adjustment for the video frame time.

### Vertical Displayed (R6)

This 7-bit register contains the number of displayed character rows in each frame. In this way, the vertical size of the displayed text is determined.

### Vertical Sync Position (R7)

This 7-bit register is used to select the character row time at which the VSYNC pulse is desired to occur and, thus, is used to position the displayed text vertically.

#### Figure 2. Vertical and Horizontal Timing

\[Vertical and Horizontal Timing Diagram\]

### Mode Control (R8)

This register is used to select the operating modes of the UM6845R and is configured as follows:\
\

|     |     |     |     |     |     |                 |                 |
|-----|-----|-----|-----|-----|-----|-----------------|-----------------|
| 7   | 6   | 5   | 4   | 3   | 2   | 1               | 0               |
|     |     |     |     |     |     | See table below | See table below |

\
\

| **1** | **0** | **Operation**                        |
|-------|-------|--------------------------------------|
| x     | 0     | Non-interlace                        |
| 0     | 1     | Interlace SYNC Raster Scan           |
| 1     | 1     | Interlace SYNC and Video Raster Scan |

\
\

### Scan Line (R9)

This 5-bit register controls the number of scan lines per character row, including spacing minus one.

### Cursor Start (R10) and Cursor End (R11)

These 5-bit registers select the starting and edning scan lines for the cursor. In addition, bits 5 and 6 of R10 are used to select the cursor mode, as follows:\
\

| **6** | **5** | **Cursor mode**           |
|-------|-------|---------------------------|
| 0     | 0     | No Blinking               |
| 0     | 1     | No cursor                 |
| 1     | 0     | Blink at 16x field period |
| 1     | 1     | Blink at 32x field period |

\
\
Note that the ability to program both the start and end scan line for the cursor enables either block cursor or underline to be accomodated. Registers R14 and R15 are used to control position of the cursor over the entire 16k address field.

### Display Start Address High (R12) and Low (R13)

These registers together comprise a 14-bit register containing the memory address of the first character of the displayed scan line (character on the top left of the video display, as in Figure 4). Subsequent memory addressess are generated by the UM6845R as a result of CCLK input pulses. Scrolling of the display is accomplished by changing R12 and R13 to the memory address associated with the first character of the desired line of text to be displayed first. Entire pages of text may be scrolled or changed as well via R12 and R13.

### Cursor Position High (R14) and Low (R15)

These registers together comprise a 14-bit register containing the memory address of the current cursor position. When the video display scan counter (MA lines) matches the contents of this register, and when the scan line counter (RA lines) falls within the bounds set by R10 and R11, then the CURSOR output becomes active. Bit 5 of the Mode Control Register (R8) may be used to delay the CURSOR output by a full CCLK time to accomodate slow access memories.

### LPEN high (R16) and Low (R17)

These registers together comprise a 14-bit register whose contents is the light pen strobe position, in terms of the video display address at which the strobe occured. When the LPEN input changes from low to high on the next negative-going edge of CCLK the contents of the internal scan counter are stored in registers R16 and R17.\
\

**Figure 3. Internal Register Summary**

| **/CS** | **RS** | **4** | **3** | **2** | **1** | **0** | **Register No.** | **Register Name** | **Stored Info** | **RD** | **WR** | **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
| 1 |  | \- | \- | \- | \- | \- | \- |  |  |  |  |  |  |  |  |  |  |  |  |
| 0 | 0 | \- | \- | \- | \- | \- | \- | Address Register | Reg No. |  | Y |  |  |  | A<sub>4</sub> | A<sub>3</sub> | A<sub>2</sub> | A<sub>1</sub> | A<sub>0</sub> |
| 0 | 0 | \- | \- | \- | \- | \- | \- | Status Reg. |  | Y |  |  | L | V |  |  |  |  |  |
| 0 | 1 | 0 | 0 | 0 | 0 | 0 | R0 | Horiz. Total | \#Charac. -1 |  | Y | \* | \* | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 0 | 0 | 0 | 1 | R1 | Horiz. Displayed | \#Charac. |  | Y | \* | \* | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 0 | 0 | 1 | 0 | R2 | Horiz. Sync Position | \#Character. |  | Y | \* | \* | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 0 | 0 | 1 | 1 | R3 | VSYNC, HSYNC Widths | \#Scan lines and \#Char. Times |  | Y |  |  |  |  | H<sub>3</sub> | H<sub>2</sub> | H<sub>1</sub> | H<sub>0</sub> |
| 0 | 1 | 0 | 0 | 1 | 0 | 0 | R4 | Vert. Total | \#Char Rows.-1 |  | Y |  | \* | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 0 | 1 | 0 | 1 | R5 | Vert. Total Adjust | \#Scan Lines |  | Y |  |  |  | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 0 | 1 | 1 | 0 | R6 | Vert. Displayed | \#Char Rows |  | Y |  | \* | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 0 | 1 | 1 | 1 | R7 | Vert. Sync Position | \#Char. Rows |  | Y |  | \* | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 1 | 0 | 0 | 0 | R8 | Mode Control |  |  | Y |  |  |  |  |  |  | I<sub>1</sub> | I<sub>0</sub> |
| 0 | 1 | 0 | 1 | 0 | 0 | 1 | R9 | Scan Line | \#Scan Lines-1 |  | Y |  |  |  | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 1 | 0 | 1 | 0 | R10 | Cursor Start | Scan line no |  | Y |  | B<sub>1</sub> | B<sub>0</sub> | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 1 | 0 | 1 | 1 | R11 | Cursor End | Scan Line No |  | Y |  |  |  | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 1 | 1 | 0 | 0 | R12 | Display Start Addr (H) |  |  | Y |  |  | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 1 | 1 | 0 | 1 | R13 | Display Start Addr (L) |  |  | Y | \* | \* | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 1 | 1 | 1 | 0 | R14 | Cursor Position (H) |  | Y | Y |  |  | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 0 | 1 | 1 | 1 | 1 | R15 | Cursor Position (L) |  | Y | Y | \* | \* | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 1 | 0 | 0 | 0 | 0 | R16 | Light Pen Reg (H) |  | Y |  |  |  | \* | \* | \* | \* | \* | \* |
| 0 | 1 | 1 | 0 | 0 | 0 | 1 | R17 | Light Pen Reg (L) |  | Y |  | \* | \* | \* | \* | \* | \* | \* | \* |

\
\

#### Notes

|  |  |  |  |
|----|----|----|----|
| \* | Designates binary bit |  | Designates unused bit. These bits are always "0", except for /CS=1, which does not drive the data bus at all. |

\
\

#### Figure 4. Display Address Sequences (with Start Address = 0) for 80x24 Example

#### Figure 5. Shared Memory System Configuration

#### Memory Contention Schemes for Shared Memory Addressing

From Figure 5, it is clear that both the UM6845R and the system MPU must be capable of addressing the video display memory. The UM6845R repetitively fetches character information to generate the video signals in order to keep the screen display active. The MPU occassionally accesses the memory to change the displayed information or to read out current data characters. There are three ways to resolving this dual contention.

#### MPU PRIORITY

With this method, the address line to the video display memory are normally driven by the UM6845R unless the MPU needs access, in which case the MPU addressess immediatly override those from the UM6845R and the MPU has immediate access.

#### Ø1/Ø2 MEMORY INTERLEAVING

This method permits both the UM6845R and the MPU access to the video display memory by time-sharing via the system Ø1 and Ø2 clocks. During the Ø1 portion of each cycle (the time when E is low), the UM6845R address outputs are gated to the video display memory. In the Ø time, the MPU address lines are switched in. In this way, both the UM6845R and the MPU have periods of unimpeded access to the memory. Figure 6 illustrates the timings.

<img src="http://www.cpctech.org.uk/um6.gif" data-]="" alt="[Ø1/Ø2 Interleaving" />\
**Figure 6. Ø1/Ø2 Interleaving**

#### INTERLACE MODES

There are three raster-scan display modes (see Figure 7):

1.  a\) *Non-Interlaced Mode*. In this mode each scan line is refereshed at the vertical field rate (50 or 60Hz). In the interlaced scan modes, even and odd fields alternate to generate frames. The horizontal and vertical timing relationship causes the scan lines in the odd fields to be displaced from those in the even fields. The two additional raster-scan display modes pertain to interlaced scans.
2.  b\) *Interlace Sync Mode*. This mode is used when the same information is to be displayed in both odd and even fields. Enhanced readability results because the spaces between adjacent rows are filled and a higher quality character is displayed. This is achieved with only a slight alteration to the device operation: in alternate fields the position of the VSYNC signal is delated by ½ of a scan line time. This is illustrated in Figure 8 and is only difference in the UM6845R operation in this mode.
3.  c\) *Interlaced Sync and Video Mode*. This mode is used to double the character density on the screen by displaying the even lines in even fields and the odd lines in odd fields. As in the Interlace-Sync mode, the VSYNC position is delayed in alternate display fields. In addition, the address generation is altered.

#### Figure 7. Comparison of Display Modes

![](http://www.cpctech.org.uk/um1.gif)\
**NON-INTERLACE**

![](http://www.cpctech.org.uk/um2.gif)\
**INTERLACED SYNC**

![](http://www.cpctech.org.uk/um3.gif)\
**INTERLACED SYNC AND VIDEO**

#### Figure 8. Interlace Sync Mode and Interlace Sync & Video Mode Timing

<span id="CRTC_DIFFERENCES"></span>

#### CRTC Register Comparison Table

**NON-INTERLACE**

| **Register** | **Register** | **UM6845R, MC6845, MC6845\*1** | **MC6845R, HD6845R** | **UM6845, HD6845S** | **UM6845E** | **SYS6545-1** |
|----|----|----|----|----|----|----|
| R0 HTotal | R0 HTotal | Total-1 | Total-1 | Total-1 | Total-1 | Total-1 |
| R1 HDisp | R1 HDisp | Actual | Actual | Actual | Actual | Actual |
| R2 HSync | R2 HSync | Actual | Actual | Actual | Actual | Actual |
| R3 Sync Width | R3 Sync Width | Horizontal (& Vertical \*1) | Horizontal | Horizontal & Vertical | Horizontal & Vertical | Horizontal & Vertical |
| R4 VTotal | R4 VTotal | Total-1 | Total-1 | Total-1 | Total-1 | Total-1 |
| R5 VTotal Adjustment | R5 VTotal Adjustment | Any Value | Any Value | Any Value | Any Value | Any Value Except R5 |
| R6 VDisp | R6 VDisp | Any Value \<R4 | Any Value \<R4 | Any Value \<R4 | Any Value \<R4 | Any Value \<R4 |
| R7 VSync | R7 VSync | Actual-1 | Actual-1 | Actual-1 | Actual-1 | Actual-1 |
| R8 Mode Select | Bit 0, Bit 1 | Interlace | Interlace | Interlace | Interlace | Interlace |
| R8 Mode Select | Bit 2 | \- | \- | \- | Row/Column or Binary Addr. | Row/Column or Binary Addr. |
| R8 Mode Select | Bit 3 | \- | \- | \- | Shared or Transparent Addr. | Shared or Transparent Addr. |
| R8 Mode Select | Bit 4 | (Display Enable Skew \*1) | \- | Display Enable Skew | Display Enable Skew | Display Enable Skew |
| R8 Mode Select | Bit 5 | (Display Enable Skew \*1) | \- | Display Enable Skew | Cursor Skew | Cursor Skew |
| R8 Mode Select | Bit 6 | (Cursor Skew \*1) | \- | Cursor Skew | RA4/ | RA4/ |
| R8 Mode Select | Bit 7 | (Cursor Skew \*1) | \- | Cursor Skew | Transparent | Transparent |
| R9 Scan Lines | R9 Scan Lines | Total-1 | Total-1 | Total-1 | Total-1 | Total-1 |
| R10 Cursor Start | R10 Cursor Start | Actual | Actual | Actual | Actual | Actual |
| R11 Cursor End | R11 Cursor End | Actual | Actual | Actual | Actual | Actual |
| R12/R13 Display Addr. | R12/R13 Display Addr. | Write-Only, Read/Write (MC6845 & \*1) | Read/Write | Read/Write | Write Only | Write Only |
| R14/R15 Cursor Position | R14/R15 Cursor Position | Read/Write | Read/Write | Read/Write | Read/Write | Read/Write |
| R16/R17 Position | R16/R17 Position | Read-only | Read-only | Read-only | Read-only | Read-only |
| R18/R19 Update Addr. Register | R18/R19 Update Addr. Register | N/A | N/A | N/A | Transparent Mode Only | Transparent Mode Only |
| R31 Dummy Register | R31 Dummy Register | N/A | N/A | N/A | Transparent Mode Only | Transparent Mode Only |
| Status Register | Status Register | Yes (UM6845R) | No | No | Yes | Yes |

\
\

**INTERLACE SYNC**

| **Register** | **UM6845R, MC6845, MC6845\*1** | **MC6845R, HD6845R** | **UM6845, HD6845S** | **UM6845E** | **SYS6545-1** |
|----|----|----|----|----|----|
| R0 HTotal | Total-1 = Odd or Even | Total-1 = Odd | Total-1 = Odd | Total-1 = Odd or Even | Total-1 = Odd |

\
\

**INTERLACED SYNC AND VIDEO**

| **Register** | **UM6845R, MC6845, MC6845\*1** | **MC6845R, HD6845R** | **UM6845, HD6845S** | **UM6845E** | **SYS6545-1** |
|----|----|----|----|----|----|
| R4 VTotal | Total-1 | Total-1 | Total-1 | Total-1 | Total/2-1 |
| R6 VDisp | Total | Total/2 | Total | Total | Total/2 |
| R7 VSync | Actual-1 | Actual-1 | Actual-1 | Actual-1 | Actual/2 |
| R9 Scan Lines | Total-1 Odd/Even | Total-1 Only Even | Total-1 Odd/Even | Total-1 Odd/Even | Total-1 Odd/Even |
| R10 Cursor Start | Odd/Even | Both Odd or Both Even | Odd/Even | Odd/Even | Odd/Even |
| R11 Cursor End | Odd/Even | Both Odd or Both Even | Odd/Even | Odd/Even | Odd/Even |
| CCLK | 2.5Mhz | 2.5Mhz | 3.7Mhz | 3.7Mhz | 2.5Mhz |

#### Ordering Information

| **Part Number** | **CPU Clock Rate** | **Package** |
|-----------------|--------------------|-------------|
| UM6845R         | 1Mhz               | Plastic     |
| UM6845RA        | 2Mhz               | Plastic     |
| UM6845RB        | 3Mhz               | Plastic     |

------------------------------------------------------------------------

Document HTMLised from original Chip Specification by Kev.
