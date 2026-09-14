<!--
Converted from "Motorola MC6845 Cathode Ray Tube Controller.html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 12 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# MC6845

# CRT Controller (CRTC)

The MC6845 CRT controller performs the interface between an MPU and a raster-scan CRT display. It is intended for use in MPU-based controllers for CRT terminals in stand-alone or cluster configurations.\
\
The CRTC is optimized for the hardware/software balance required for maximum flexibility. All keyboard functions, reads, writes, cursor movements, and editing are under processor control. The CRTC provides video timing and refresh memory addressing.

- Useful in Monochrome or Color CRT Applications
- Applications include "Glass-teletype", Smart, Programmable, Intelligent CRT Terminals; Video Games; Information displays
- Alphanumeric;Semi-Graphic and Full-Graphic Capability
- Fully programmable via processor data bus, timing may be generated for almost any alphanumeric screen format e.g. 80×24, 72×64,132×20.
- Single +5V Supply
- M6800 compatible bus interface.
- TTL-Compatible inputs and outputs
- Start Address Register provides hardware scroll (by Page or character)
- Programmable cursor register allows control of cursor format and blink rate.
- Light Pen Register
- Refresh (Screen) Memory may be multiplexed between the CRTC and the MPU, Thus removing the requirements for line buffers or external dma devices.
- Programmable interlace or non-interlace scan modes
- 14-bit refresh address allows up to 16k of refresh memory for use in character or semi-character displays
- 5-bit row address allows up to 32 scan-line character blocks
- By utilizing both the refresh addressess and the row addressess a 512k address space is available in graphics systems
- Refresh addresses are provided during retrace, allowing the CRTC to provide row addressess to refresh dynamic rams.
- Pin compatible with the MC6835

#### FIGURE 1 TYPICAL CRT CONTROLLER APPLICATION

## MAXIMUM RATINGS

| Rating                      | Symbol | Value       | Unit |
|-----------------------------|--------|-------------|------|
| Supply Voltage              | Vcc    | -0.3 to 7.0 | V    |
| Input Voltage               | Vin    | -0.3 to 7.0 | V    |
| Operating Temperature Range | TA     | Tl to Th    | °C   |
| MC6845,MC68A45, MC68B45     | TA     | 0 to 70     | °C   |
| MC6845C, MC68A45C           | TA     | -40 to +85  | °C   |
| Storage Temperature Range   | Tstg   | -55 to 150  | °C   |

## THERMAL CHARACTERISTICS

| Characteristics    | Symbol | Value | Unit |
|--------------------|--------|-------|------|
| Thermal Resistance | 0JA    |       | CW   |
| Plastic Package    |        | 100   | CW   |
| Cerdip Package     |        | 60    | CW   |

## RECOMMENDED OPERATING CONDITIONS

| Characteristics    | Symbol | Min. | Typ. | Max. | Unit |
|--------------------|--------|------|------|------|------|
| Supply Voltage     | Vcc    | 4.75 | 5.0  | 5.25 | V    |
| Input Low Voltage  | Vil    | -0.3 | \-   | 0.8  | V    |
| Input High Voltage | Vih    | 2.0  | \-   | Vcc  | V    |

\
\
The device contains circuitry to protect the inputs against damage due to high static voltages or electric fields, due to high static voltages of electric fields, however, it is advised that normal precautions be taken to avoid application of any voltage higher than maximum rater voltages to this high impedance curcuit. For proper operation it is recommended that Vin and Vout can be constrained to the range Vss\<=IVin or VoutI\<=Vcc.

## POWER CONSIDERATIONS

### D.C. Electrical Characteristics (Vcc = 5.0V +-5%, TA=0-70C, unless otherwise noted)

| Characteristics | Symbol | Min. | Typ. | Max. | Units |  |
|----|----|----|----|----|----|----|
| Input High Voltage | VIH | 2.0 | \- | Vcc | V |  |
| Input Low Voltage | VIL | -0.3 |  | 0.8 | V |  |
| Input Leakage Current | IIN | \- | 0.1 | 2.5 | uA |  |
| Hi-Z State Input Current (Vcc=5.25 V) VIN = 0.4 to 2.4V | ITSI | -10/td\> | \- | 10 | uA |  |
| Output High Voltage | VOH |  |  |  |  | V |
| (ILOAD = -205 uA) D0-D7 | VOH | 2.4 | 3.0 | \- |  | V |
| (ILOAD = -100 uA) Other outputs | VOH | 2.4 | 3.0 | \- |  | V |
| Output Low Voltage ILOAD = 1.6mA | VOL | \- | 0.3 | 0.4 | V |  |
| Internal Power Dissipation (Measured at Ta=0C) | PINT | \- | 600 | 750 | mW |  |
| Input Capacitance | CIN |  |  |  |  | pF |
| D0-D7 | CIN | \- | \- | 12.5 |  | pF |
| All Others | CIN | \- | \- | 10 |  | pF |
| Output Capacitance | COUT | \- | \- | 10 | pF |  |

### BUS TIMING CHARACTERISTICS (See Notes 1 and 2) (Reference Figures 2 and 3)

| Ident. Number | Characteristics | Symbol | Min. | Max. | Min. | Max. | Min. | Max. | Units |
|----|----|----|----|----|----|----|----|----|----|
| 1 | Cycle Time | tCYC | 1.0 | 10\*\* | 0.67 | 10 | 0.5 | 10\*\* | us |
| 2 | E Pulse Width, Low | PWEL | 430 | \- | 280 | \- | 210 | \- | ns |
| 3 | PWEH | E Pulse Width, High | 450 | \- | 280 | \- | 220 | \- | ns |
| 4 | Clock Rise and Fall Time | tr,tf | \- | 25 | \- | 25 | \- | 20 | ns |
| 9 | Address Hold Time | tAH | 10 | \- | 10 | \- | 10 | \- | ns |
| 13 | RS Setup Time Before E | tAS | 80 | \- | 60 | \- | 40 | \- | ns |
| 14 | /W/R, /CS Set-Up Time Before E | tCS | 80 | \- | 60 | \- | 40 | \- | ns |
| 15 | /W/R, /CS Hold Time | tCH | 10 | \- | 10 | \- | 10 | \- | ns |
| 18 | Read Data Hold Time | tDHR | 20 | 50\* | 20 | 50\* | 20 | 50\* | ns |
| 21 | Write Data Hold Time | tDHW | 10 | \- | 10 | \- | 10 | \- | ns |
| 30 | Peripheral Output Data Delay Time | tDDR | \- | 290 | \- | 180 | 0 | 150 | ns |
| 31 | Peripheral Input Data Delay Time | tDSW | 165 | \- | 80 | \- | 60 |  | ns |

\* The data bus output buffers are no longer sourcing or sinking current by tDHR maximum high impedance.\
\*\* The E clock may be low for extended periods provided the CLK input is active.

### CRTC TIMING CHARACTERISTICS (Reference Figures 4 and 5)

| Characteristic                       | Symbol  | Min. | Max. | Units |
|--------------------------------------|---------|------|------|-------|
| Maximum Clock Pulse Width, Low       | PWCL    | 150  | \-   | ns    |
| Maximum Clock Pulse Width, High      | PWCH    | 150  | \-   | ns    |
| Clock Frequency                      | fc      | \-   | 3.0  | Mhz   |
| Rise and Fall Time for Clock Input   | Tcr,Tcf | \-   | 20   | ns    |
| Memory Address Delay Time            | tMAD    | \-   | 160  | ns    |
| Raster Address Delay Time            | tRAD    | \-   | 160  | ns    |
| Display Timing Delay Time            | tDTD    | \-   | 250  | ns    |
| Horizontal Sync Delay Time           | tHSD    | \-   | 250  | ns    |
| Vertical Sync Delay Time             | tVSD    | \-   | 250  | ns    |
| Cursor Display Timing Delay Time     | tCDD    | \-   | 250  | ns    |
| Light Pen Strobe Maximum Pulse Width | PWLPH   | 80   | \-   | ns    |
| Light Pen Strobe Disable Time        | tLPD1   | \-   | 80   | ns    |
| Light Pen Strobe Disable Time        | tLPD2   | \-   | 10   | ns    |

NOTE: The light pen strobe must fall to low level before VS pulse rises.

## CRTC INTERFACE SYSTEM DESCRIPTION

The CRT controller generates the signals necessary to interface a digital system to a taster scan CRT display. In this type of display, an electron beam starts in the upper left hand corner, moves quickly across the screen and returns. This action is called a horizontal scan. After each horizontal scan the beam is incrementally moved down the vertical direction until it has reached the bottom. At this point one frame has been displayed, as the beam has made many horizontal scans and one vertical scan.\
\
Two types of raster scanning are used in CRTs, interlace and on-interlace, shown in Figures 6 and 7. Non-interlace scanning consists of one field per frame. The scan lines in Figure 6 are shown as solid lines and the retrace patterns are indicated by the dotted lines. Increasing the number of frames per second will decrease the flicker. Ordinarily, either a 50 or 60 frame per second refresh rate is used to minimize beating between the CRT and the power line frequency. This prevents the displayed data from weaving.\
\
Interlace scannins is used in broadcast TV and on data monitors where high-density or high-resolution data must be displayed. Two fields, or vertical scans are made down the screen for each picture or frame. The first field (even field) starts in the upper left hand corner, the second (odd field) in the upper center. Both fields overlap as shown in Figure 7, thus interlacing the two fields into a single frame.\
\
In order to display the characters on the CRT screen the frames must be continually repeated. The data to be displayed is stored in the refresh (screen) memory by the MPU controlling the data processing system. The data is usually written in ASCII code, so it cannot be directly displayed as characters. A character generator ROM is typically used to convert the ASCII codes into the "dot" pattern for every character.\
\
The most common method of generating characters is to create a matrix of dots "x" dots (columns) wide and "y" dots (rows) high. Each character is created by selectively filling in the dots. As "x" and "y" get larger a more detailed character may be created. Two common dot matrices are 5x7 and 7x9. Many variations of these standards will allow Chinese, Japanese or Arabic letters instead of English. Since characters require some space between them, a character block larger than the character is typically used, as shown in Figure 8. The figure also shows the corresponding timing and levels for a video signal that would generate the characters.\
\
Refering to Figure 1, the CRT controller generates the refresh addressess (MA0-MA13), row addressess (RA0--RA4), and the video timing (vertical sync - VS, horizontal sync - HS, and display enable - DE). Other functions include an internal cursor register which generates a cursor output when its contents compare to the current refresh address. A light pen strobe input signal allows capture of the refresh address in an internal light pen register.\
\
All timing in the CRTC is derived from the CLK input. In alphanumerical terminals, this signal is the character rate. The video rate or "dot" clock is externally divided by high-speed logic (TTL) to generate the CLK input. The high-speed logic must also generate the timing and control signals necessary for the shift register, latch and MUX control.\
\
The processor communicates with the CRTC through an 8-bit data bus by reading and writing into the 18 registers.\
\
The refresh memory address is multiplexed between the processor and the CRTC. Data appears on a secondary bus seperate from the processor's bus. The secondary data bus concept in no way precludes using the refresh RAM for other purposes. It looks like any other RAM to the processor. A number of approaches are possible for solving contentions for the refresh memory.

- Processor always gets priority. (Generally, "hash" occurs as MPU and CRTC clocks are not synchronised).
- Processor gets priority access anytime, but can be synchronised by an interrupt to perform accessess only during horizontal and vertical retrace times.
- Synchronize the processor with memory way cycles (waits)
- Synchronize the processor to the character rate as shown in Figure 9. The M6800 processor family works very well in this configuration as constant cycle lengths are present. This method provides no overhead for the processor as there is never a contention for a memory access. All accessess are transparent.

![](http://www.cpctech.org.uk/mc4.gif)

## PIN DESCRIPTION

### PROCESSOR INTERFACE

The CRTC interfaces to a processor bus on the bi-directional data bus (D0-D7) using /CS, /RS. E and R/W for control signals.\
\

**Data Bus (D0-D7)**  
The bidirectional data lines (D0-D7) allow data transfer between the internal CRTC register file and the processor. Data bus output drivers are in the high impedance state until the processor performs a CRTC read operation.\
\

**Enable (E)**  
The enable signal is a high impedance TTL/MOS compatible input which enables the data bus input/output buffers and clocks data to and from the CRTC. This signal is usually derived from the processor clock. The high-to-low transition is in the active edge.\
\

**Chip Select (/CS)**  
The /CS line is a high-impedance TTL/MOS compatible input which selects the CRTC, when low, to read or write to the internal register file. This signal should only be active when there is a valid stable address being decoded from the processor.\
\

**Register Select (RS)**  
The RS line is a high-impedance TTL/MOS compatible input which selects either the address register (RS=0) or one of the data register (RS=1) or the internal register file.\
\

**Read/Write (R/W)**  
The R/W line is a high-impedance TTL/MOS compatible input which determines whether the internal register file gets written or read. A write is defined as a low level.

### CRT CONTROL

The CRTC provides horizontal sync (HS), vertical sync (VS), and display enable (DE) signals.

#### NOTE

Care should be exercised when interfacing to CRT monitors, as many monitors claiming to be "TTL compatible" have transistor input curcuits which require the CRTC or TTL devices buffering signals from the CRTC/video circuits to exceed the maximum rated drive currents.\
\

**Vertical Sync (VS) and Horizontal Sync (HS)**  
These TTL-compatible outputs are active high signals which drive the monitor directly or are fed to the video processing circuitry to generate a composite video signal. The VS signal determines the vertical position of the displayed text while the HS signal determines the horizontal position of the displayed text.\
\

**Display Enable (DE)**  
This TTL compatible output is an active high signal which indicates the CRTC is providing addressing in the active display area.

### REFRESH MEMORY/CHARACTER GENERATION ADDRESSING

The CRTC provides memory addressing (MA0-MA13) to scan the refresh RAM. Row addressess (RA0-RA4) are also provided for use with character generator ROMS. In a graphics system, both the memory addresses and the row addresses would be used to scan the refresh RAM. Both the memory addresses and the row addressess continue to run during vertical retrace thus allowing the CRTC to provide the refresh addresses required to refresh dynamic RAMs.\
\

**Refresh Memory Addresses (MA0-MA13)**  
These 14 outputs are used to refresh the CRT screen with pages of data located within a 16k block of refresh memory. These outputs are capable of driving one standard TTL load and 30pF.\
\

**Row Addresses (RA0-RA4)**  
These five outputs from the internal row address counter are used to address the character generator ROM. These outputs are capable of driving one standard TTL load and 30pF.

### OTHER PINS

**Cursor**  
This TTL-compatible output indicates a valid cursor address to external video processing logic. It is an active high signal.\
\

**Clock (CLK)**  
The CLK is a TTL/MOS-compatible input used to synchronise all CRT functions except for the processor interface. An external dot counter is used to derive this signal which is usually the character rate in an alphanumeric CRT. The active transition is high-to-low.\
\

**Light Pen Strobe (LPSTB)**  
A low-to-high transition on this high-impedance TTL/MOS-compatible input latches the current Refresh Address in the light pen register. The latching of the refresh address is internally synchronised to the character clock (CLK).\
\

**Vcc and Vss**  
These inputs supply +5 Vdc ± 5% to the CRTC\
\

**/RESET**  
The /RESET input is used to reset the CRTC. A low level on the /RESET input forces the CRTC into the following state.

- All counters in the CRTC are cleared and the device stops the display operation.
- All the outputs are driven low
  - NOTE: The horizontal sync output is not defined until after R2 is programmed.
- The control registers of the CRTC are not affected and remain unchanged

Functionality of /RESET differs from that of other M6800 parts in the following functions.

- The /RESET input and the LPSTB input are encoded as shown in Table 1.\
  \
  **TABLE 1 - CRTC OPERATING MODE**
  | /RESET | /LPSTB | Operating Mode |
  |--------|--------|----------------|
  | 0      | 0      | Reset          |
  | 0      | 1      | Test Mode      |
  | 1      | 0      | Normal Mode    |
  | 1      | 1      | Normal Mode    |

  \
  \
  The test mode configures the memory addresssess as two independant 7-bit counters to minimize test time.
- After /RESET has gone low and (LPSTB=0), MA0-MA13 and RA0-RA4 will be driven low on the falling edge of CLK. /RESET must remain low for at least one cycle of the character clock (CLK).
- The CRT resumes the display operation immediatly after the release of /RESET, DE, and the CURSOR are not active until after the first frame has been displayed

## CRTC DESCRIPTION

The CRTC consists of programmable horizontal and vertical timing generators, programmable linear address register, programmable cursor logic, light pen capture register, and control circuitry for interface to a processor bus. A block diagram of the CRTC is shown in Figure 10.\
\
All CRTC timing is derived from the CLK, usually the output of an external dot rate counter. Coincidence (CD) circuits continuously compare counter contents to the contents of the programmable register file, R0--R17 for horizontal timing generation, comparisons result in: 1) horizontal sync pulse (HS) of a frequency, position, and width determined by the registers, 2) horizontal display signal of a frequency, position, and duration determined by the registers.\
\
The horizontal counter produces H clock which drives the scan line counter and vertical control. The contents of the raster counter are continuously compared to the maximum scan line address register. A coincidence resets the raster counter and clocks the vertical counter.\
\
Comparisons of vertical counter contents and vertical registers result in: 1) vertical sync pulse (VS) of a frequency and position determined by the registers; 2) vertical display of a frequency and position determined by the registers.\
\
The vertical control logic has other functions:

- Generate row selects, RA0--RA4, from the raster count for the corresponding interlace or non-interlace modes.
- Extend the number of scan lines in the vertical total by the amount programmed in the vertical total adjust register.

The linear address generator is driven by the CLK and locates the relative positions of characters in memory with their positions on the screen. Fourteen lines, MA0-MA13 are available for addressing up to four pages of 4k characters, eight pages of 2k characters, etc. Using the start address register, hardware scrolling through 16k characters is possible. The linear address generator repeats the same sequence of addressess for each scan line of a character row.\
\
The cursor logic determines the cursor location, size, and blink rate on the screen. All are programmable.\
\
The light pen strobe going high causes the current contents of the address counter to be latched in the light pen register. The contents of the light pen register are subsequently read by the processor.\
\
Internal CRTC registers are programmed by the processor through the data bus, D0--D7, and the control signals -- /W/R, /CS, RS and E.

## REGISTER FILE DESCRIPTIONS

The nineteen registers of the CRTC may be accessed through the data bus. Only two memory locations are required as one location is used as a pointer to address one of the remaining eighteen registers. These eighteen registers control horizontal timing, vertical timing, interlace operation, row address operation, and define the cursor, cursor address, start address, and light pen register. The register addresses and sizes are shown in Table 2.

## ADDRESS REGISTER

The address register is a 5-bit write-only register used as an "indirect" or "pointer" register. It contains the address of one of the other eighteen registers. When both RS and /CS are low, the address register is selected. When /CS is low and RS is high, the register pointed to by the address register is selected.

## TIMING REGISTERS R0-R9

Figure 11 shows the visible display area of a typical CRT monitor giving the point of reference for horizontal registers as the left-most displayed character position. Horizontal registers are programmed in character clock time units with respect to the reference as shown in Figure 12. The point of reference for the vertical registers is the top character position displayed. Vertical registers are programmed in scan line times with respect to the reference as shown in Figure 13.\
\
**Horizontal Total Register (R0)** - This 8-bit write-only register determines the horizontal Sync (HS) frequency by defining the HS period in character times. It is the total of the displayed characters plus the non-displayed character times (retace) minus one.\
\

#### FIGURE 10 - CRTC BLOCK DIAGRAM

\
\

**TABLE 2: CRTC INTERNAL REGISTER ASSIGNMENT**

| **/CS** | **RS** | **4** | **3** | **2** | **1** | **0** | **Register \#** | **Register File** | **Program Unit** | **READ** | **WRITE** | **7** | **6** | **5** | **4** | **3** | **2** | **1** | **0** |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
| 1 | x | x | x | x | x | x | x | \- | \- | \- | \- |  |  |  |  |  |  |  |  |
| 0 | 0 | x | x | x | x | x | AR | Address Register | \- | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 0 | 0 | 0 | 0 | R0 | Horizontal Total | Char | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 0 | 0 | 0 | 1 | R1 | Horizontal Displayed | Char | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 0 | 0 | 1 | 0 | R2 | H. Sync Position | Char | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 0 | 0 | 1 | 1 | R3 | Sync Width | \- | No | Yes | wv3 | wv3 | wv3 | wv3 | H | H | H | H |
| 0 | 1 | 0 | 0 | 1 | 0 | 0 | R4 | Vertical Total | Char row | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 0 | 1 | 0 | 1 | R5 | V. Total Adjust | Scan line | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 0 | 1 | 1 | 0 | R6 | Vertical Displayed | Char row | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 0 | 1 | 1 | 1 | R7 | V. Sync Position | Char row | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 1 | 0 | 0 | 0 | R8 | Interlace Mode and Skew | Note 1 | No | Yes |  |  |  |  |  |  | I | I |
| 0 | 1 | 0 | 1 | 0 | 0 | 1 | R9 | Max Scan Line Address | Scan line | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 1 | 0 | 1 | 0 | R10 | Cursor Start | Scan line | No | Yes |  | B | P |  |  |  |  |  |
| 0 | 1 | 0 | 1 | 0 | 1 | 1 | R11 | Cursor End | Scan line | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 1 | 1 | 0 | 0 | R12 | Start Address (H) | \- | No | Yes | 0 | 0 |  |  |  |  |  |  |
| 0 | 1 | 0 | 1 | 1 | 0 | 1 | R13 | Start Address (L) | \- | No | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 0 | 1 | 1 | 1 | 0 | R14 | Cursor (H) | \- | Yes | Yes | 0 | 0 |  |  |  |  |  |  |
| 0 | 1 | 0 | 1 | 1 | 1 | 1 | R15 | Cursor (L) | \- | Yes | Yes |  |  |  |  |  |  |  |  |
| 0 | 1 | 1 | 0 | 0 | 0 | 0 | R16 | Light Pen (H) | \- | Yes | No | 0 | 0 |  |  |  |  |  |  |
| 0 | 1 | 1 | 0 | 0 | 0 | 1 | R17 | Light Pen (L) | \- | Yes | No |  |  |  |  |  |  |  |  |

\
\

#### Notes

- The interlace is shown in Table 3.
- Bit 5 of the cursor start register is used for blink period control, and bit 6 is used to select blink or non-blink.

#### FIGURE 11 - ILLUSTRATION OF THE CRT SCREEN FORMAT

**Horizontal Displayed Register (R1)** - This 8-bit write only register determines the number of displayed characters per line. Any 8-bit number may be programmed as long as the contents of R0 are greater than the contents of R1.\
\
**Horizontal Sync Position Register (R2)** - This 8-bit write only register controls the HS position. The horizontal sync position defines the horizontal sync delay (front porch) and the horizontal scan delay (back porch). When the programmed value of this register is increased, the display on the CRT is shifted to the left. When the programmed value is decreased the display is shifted to the right. Any 8-bit number may be programmed as long as the sum of the contents of R2 and R3 are less than the contents of R0. R2 must be greater than R1.\
\
**Sync Width Register (R3)** - This 8-bit write only register determines the width of the horizontal (HS) pulse. The vertical sync pulse width is fixed at 16 scan-line times. The HS pulse width may be programmed from 1-to-15 character clock periods thus allowing compatibility with the HS pulse width specifications of many different monitors. If zero is written into this register then no HS is provided.\
\
Horizontal Timing Summary (Figure 12) - The difference between R0 and R1 is the horizontal blanking interval. This interval in the horizontal scan period allows the beam to return (retrace) to the left side of the screen. The retrace time is determined by the monitor's horizontal scan components. Retrace time is less than the horizontal blanking interval. A good rule of thumb is to make the horizontal blanking about 20% of the total horizontal scanning period for a CRT. In in-expensive TV recievers, the beam overscans the displau screen so that ageing of parts does not result in underscanning. Because of this, the retrace time should be about one third the horizontal scanning period. The horizontal sync delay, HS pulse width, and horizontal scan delay are typically programmed with a 1:2:2 ratio.\
\
**Vertical Total Register (R4) and Vertical Total Adjust Register (R5)** - The frequency of VS is determined by both R4 and R5. The calculated number of character rows times is usually an integer plus a fraction to get exactly a 50 or 60Hz vertical refresh rate. The integer number of character row times minus one is programmed in the 7-bit write-only vertical total register (R4). The fraction of character line times is programmed in the 5-bit write-only vertical total adjust register (R5) as the number of scan lines required.\
\
**Vertical Displayed Register (R6)** - This 7-bit write-only register specifis the number of displayed character rows on the CRT screen, and is programmed in character row times. Any number smaller than the contents of R4 may be programmed into R6.\
\
**Vertical Sync Position (R7)** - The 7-bit write-only register controls the position of vertical sync with respect to the reference. It is programmed in character row times. When the programmed value of this register is increased, the display position of the CRT screen is shifted up. When the programmed value is decreased the display position is shifted down. Any number equal to or less than the vertical total (R4) and greater than or equal to the vertical displayed (R6) may be used.\
\
**Interlace Mode and Skew Register (R9)** - The MC6845 only allows control of the interlace modes as programmed by the low order two bits of this write-only register. Table 3 shows the interlace modes available to the user. These modes are selected using the two low order bits of this 6-bit write only register.\
\
**TABLE 3 - INTERLACE MODE REGISTER**\
\

| **Bit 1** | **Bit 0** | **Mode**                          |
|-----------|-----------|-----------------------------------|
| 0         | 0         | Normal Sync Mode (Non-Interlaced) |
| 1         | 0         | Normal Sync Mode (Non-Interlaced) |
| 0         | 1         | Interlace Sync Mode               |
| 1         | 1         | Interlace Sync and Video Mode     |

In the normal sync mode (non-interlace) only one field is available as shown in Figures 6 and 14a. Each scan line is refreshed at the VS frequency (e.g. 50 or 60 Hz).\
\
Two interlace modes are available as shown in Figures 7, 14b and 14a. The frame time is divided between even and odd alternating fields. The horizontal and vertiacl timing relationship (VS delayed by one half scan line time) results in the displacement of scan lines in the odd field with respect to the even field.\
\
In the interlace sync mode the same information is painted in both fields as shown in figure 14b. This is a useful mode for filling in a character to enhance readability.\
\
In the interlace sync and video mode, shown in Figure 14c, alternating lines of the character are displayed in the even field and the odd field. This affectively doubles the given bandwidth of the CRT monitor.\
\
Care must be taken when using either interlace mode to avoid an apparent flicker effect. This flicker effect is due to the doubling of the refresh time for all scan lines since each field is displayed alternatively and may be minimized with proper monitor design (e.g. longer persistence phosphors)\
\
In addition there are restrictions on the programming of the CRTC registers for interlace operations.

- The horizontal total register value, R0 must be odd (i.e. an even number of character times)
- For interlace sync and video mode only, the maximum scan-line address, R9, must be odd (i.e. an even number of scan lines)
- For interlace sync and video mode only,the number (Nvd) programming into the vertical display register (R6) must be one half the actual number required. The even numbered scan lines are displayed in the even field and the odd numbered scan lines ae displayed in the odd field.
- For interlace sync and video mode only, the cursor start register (R10) and cursor end register (R11) must both be even or both odd depending on which field the cursor is to be displayed in. A full block cursor will be displayed in both the even and odd field when the cursor end register (R11) is programmed to a value greater than the value in the maximum scan line address register (R9).

#### FIGURE 12 - CRTC HORIZONTAL TIMING

#### FIGURE 13 - CRTC VERTICAL TIMING

#### FIGURE 14 - INTERLACE CONTROL

![](http://www.cpctech.org.uk/mc1.gif)\
**(a) Normal Sync**\
\
![](http://www.cpctech.org.uk/mc2.gif)\
**(b) Interlace Sync**\
\
![](http://www.cpctech.org.uk/mc3.gif)\
**(c) Interlace Sync and Video**\
\

**Maximum Scan Line Address Register (R9)** - This 5-bit write-only register determines the number of scan lines per character row including the spacing, thus, controlling operation of the row address counter. The programmed value is a maximum address and is one less than the number of scan lines.

## CURSOR CONTROL

**Cursor Start Register (R10) and Cursor End Register (R11)** - These registers allow a cursor of up to 32 scan line in height to be placed on any scan line of the character block as shown in Figure 15. R10 is a 7-bit write-only register used to define the start scan line and the cursor blink rate. Bits 5 and 6 of the cursor start address register control the cursor operation as shown in Table 4. Non-display, display, and two blink modes (16 times or 32 times the field period) are available. R11 is a 5-bit write-only register which defines the last scan line of the cursor.\
\

**TABLE 4 - CURSOR START REGISTER**

| **Bit 6** | **Bit 5** | **Cursor Display Mode** |
|-----------|-----------|-------------------------|
| 0         | 0         | Non-Blink               |
| 0         | 1         | Cursor Non-Display      |
| 1         | 0         | Blink, 1/16 field rate  |
| 1         | 1         | Blink, 1/32 field rate  |

Example of cursor display mode.

\
\
When an external blink feature on characters is required, it may be necessary to perform cursor blink externally so that both blink rates are synchronised. Note than an invert/non-invert cursor is easily implemented by programming the CRTC for blinking cursor and externally inverting the video signal with an exclusive-OR gate.\
\
**Cursor Register (R14-H, R15-L)** - This 14-bit read/write register pair is programmed to position the cursor anywhere in the refresh RAM area, thus, allowing hardware paging and scrolling through memory without loss of the original cursor position. It consits of a 8-bit low order (MA0-MA7) register and a 6-bit high order (MA8-MA13) register.

## OTHER REGISTERS

**Start Address Register (R12-H, R13-L)** - This 14-bit write-only register pair controls the first address output by the CRTC after vertical blanking. It consists of an 8-bit low order (MA0-MA7) register and a 6-bit high order (MA8-MA13) register. This start address rgister determines which portion of the refresh RAM is displayed on the CRT screen. Hardware scrolling by character or page may be accomplished by modifying the contents of this register.\
\
**Light Pen Register (R16-H, R17-L)** - This 14-bit read-only register pair captures the refresh address output by the CRTC on the positive edge of a pulse input to the LPSTB pin. It consists of an 8-bit low order (MA0-MA7) register and a 6-bit high order (MA8-MA13) register. Since the light pen pulse is asynchronous with respect to refresh address timing an internal synchronizer is designed into the CRTC. Due to delays (Figure 5) in this circuit, the value of R16 and R17 will need to be corrected in software. Figure 16 shows an interrupt driven approach although a polling routine could be used.

#### FIGURE 15 - CURSOR CONTROL

![](http://www.cpctech.org.uk/mc9.gif)\
\
\
![](http://www.cpctech.org.uk/mc6.gif)![](http://www.cpctech.org.uk/mc7.gif)![](http://www.cpctech.org.uk/mc8.gif)\

#### FIGURE 16 - INTERFACING OF LIGHT PEN

![](http://www.cpctech.org.uk/mc10.gif)

## OPERATION OF THE CRTC

Timing charts of CRT interface signals are illustrated in this section. When values listed in Table 5 are programmed into CRTC control registers, the device provides the outputs as show in the timing diagrams (Figures 12, 13, 17 and 18). The screen format is shown in Figure 11 which illustrates the relationship between refresh memory address (MA0-MA13), raster address (RA0-RA4), and the position on the screen. In this example, the start address is assumed to be zero.\
\

**TABLE 5 - VALUES PROGRAMMED INTO CRTC REGISTERS**

| Reg. \# | Register Name         | Value   | Programmed Value |
|---------|-----------------------|---------|------------------|
| R0      | H. Total              | Nht + 1 | Nht              |
| R1      | H. Displayed          | Nhd     | Nhd              |
| R2      | H. Sync Position      | Nhsp    | Nhsp             |
| R3      | H. Sync Width         | Nhsw    | Nhsw             |
| R4      | V. Total              | Nvt + 1 | Nvt              |
| R5      | V. Scan Line Adjust   | Nadj    | Nadj             |
| R6      | V. Displayed          | Nvd     | Nvd              |
| R7      | V.Sync Position       | Nvsp    | Nvsp             |
| R8      | Interlace Mode        |         |                  |
| R9      | Max Scan Line Address | Nsl     | Nsl              |

#### FIGURE 17 - CURSOR TIMING

#### FIGURE 18 - REFRESH MEMORY ADDRESSING (MA0--MA13) STAGE CHART

## DETERMINING REGISTER CONTENTS

Some of the register contents are determined rather easily. They are:\
\

| Register | Name | Contents |  |
|----|----|----|----|
| R8 | Interlace Mode Register | See Table 3 |  |
| R10 | Cursor Start | See Figure 15 and Table 4 |  |
| R11 | Cursor End | See Figure 15 |  |
| R12 | Start Address (H) | User programs first memory location to be displayed |  |
| R14 | Cursor (H) | User programs first memory location to be displayed | User programs desired cursor location |
| R16 | Light Pen (H) | Can be loaded via light-pen strobe only | User programs desired cursor location |
| R17 | Light Pen (L) | Can be loaded via light-pen strobe only |  |

\
\
The remaining register contents must be determined from some basic data related to the CRT monitor and from the user-desired display format. The CRTC reference sheet (see Figure 19) gives a set of formulas for calculating the register contents as well as other useful characteristics of the display. This type of data is summarized under basic parameters in Figures 20 and 21, most or all of this data must be supplied by the user before he can determine the contents for registers R0-R7 and R9. All variables B1-B10 are equal to basic parameters 1 through 10.

### FIGURE 19 - CRTC REFERENCE SHEET

| Register | Function                  |
|----------|---------------------------|
| R0       | Horizontal Total          |
| R1       | Horizontal Displayed      |
| R2       | Horizontal Sync Position  |
| R3       | Horizontal Sync Width     |
| R4       | Vertical Total            |
| R5       | Vertical Total Adjust     |
| R6       | Vertical Displayed        |
| R7       | Vertical Sync Position    |
| R8       | Interlace Mode            |
| R9       | Maximum Scan Line Address |
| R10      | Cursor Start              |
| R11      | Cursor End                |
| R12      | Start Address (H)         |
| R13      | Start Address (L)         |
| R14      | Cursor (H)                |
| R15      | Cursor (L)                |
| R16      | Light Pen (H)             |
| R17      | Light Pen (L)             |

\
\
\
\

**Intermediate Calculations**

| Symbol | Description                | Calculation                     |
|--------|----------------------------|---------------------------------|
| f'     | Dot Frequency (1st approx) | (B5×(B7+B9))÷((1÷B1)-B3)        |
| tc     | Character time             | 1÷(\[(R0)+1\]×B1                |
| f      | Dot frequency              | (B7+B9)÷tc                      |
| ts     | Scan line time             | \[(R0)+1\]×tc                   |
| n      | Total \# of scan lines     | 1÷(B2×tsl)                      |
| N      | Integer                    | (n÷(B8+B10)) = N + (R÷(B8+B10)) |
| R      | Integer Remainder          | (n÷(B8+B10)) = N + (R÷(B8+B10)) |
| tcr    | Character row time         | (B8+B10)×tsl                    |
| thr    | Horizontal retrace time    | \<= (\[(R0)+1-B5\]×(B7+B9))/f   |
| tvr    | Vertical retrace time      | \<= (B1÷B2) - (B6(B8+B10)×tsl)  |

\
\
\
\

**Register Calculations**

| Register | Calculation                                          |
|----------|------------------------------------------------------|
| R0       | (f'÷(B1×(B7+B9))-1                                   |
| R1       | B5                                                   |
| R2       | (R1) + ((R3)÷R2)                                     |
| R3       | ((R0) - (R1))÷3                                      |
| R4       | N-1                                                  |
| R5       | R                                                    |
| R6       | B6                                                   |
| R7       | \[(R4)+1\] - ((16 - (R5))÷(B8+B10) \>= (R7) \>= (R6) |
| R9       | (B8 + B10)-1                                         |

#### FIGURE 21 - CRTC WORKSHEET EXAMPLE CALCULATION (80×24)

#### FIGURE 22 - CRTC WORKSHEET

## CRTC INITIALIZATION

Register R0-R15 must be initialised after the system is powered up. The processor will normally load the CRTC register file from a firmware table. The program required to initialise the CRTC for a 80x24 format (example calculation \#2) is shown in Figure 23.\
\
The CRTC registers will have an initial value at power up. When using a direct drive monitor (sans horizontal oscillator) these initial values may result in out-of-tolerance operation. CRTC programming should be done immediatly after power up especially in this type of system.

## ADDITIONAL CRTC APPLICATIONS

The foremost system function which may be performed by the CRTC controller is the refreshing of dynamic RAM. This is quite simple as the refresh addresesses continually run.\
\
Note that the LPSTB input may be used to support additional system functions other than a light pen. A digital-to-analog converter (DAC) and comparator could be configured to use the refresh address as a refrence to a DAC composed of a resistive adder network connected to a comparator. The output of the comparator would generate the LPSTB input signifying a match between the refresh address analog level and the unknown voltage.\
\
The light-pen strobe input could also be used as a character strobe to allow the CRTC refresh addressess to decode a keyboard matrix. Debouncing would need to be done in software.\
\
Both the VS and HS outputs may be used as a real-time clock. Once programmed, the CRTC will provide a stable reference frequency.

#### FIGURE 23 - M6800 PROGRAM FOR CRTC INITIALISATION

## ORDERING INFORMATION

| Package Type     | Frequency Mhz | Temperature    | Order number |
|------------------|---------------|----------------|--------------|
| Cerdip S suffix  | 1.0           | 0°C to 70°C    | MC6845S      |
| Cerdip S suffix  | 1.0           | -40°C to -85°C | MC6845CS     |
| Cerdip S suffix  | 1.5           | 0°C to 70°C    | MC68A45S     |
| Cerdip S suffix  | 1.5           | -40°C to +85°C | MC68A45CS    |
| Cerdip S suffix  | 2.0           | 0°C to 70°C    | MC68B45S     |
| Plastic P suffix | 1.0           | 0°C to 70°C    | MC6845P      |
| Plastic P suffix | 1.0           | -40°C to -85°C | MC6845CP     |
| Plastic P suffix | 1.5           | 0°C to 70°C    | MC68A45P     |
| Plastic P suffix | 1.5           | -40°C to +85°C | MC68A45CP    |
| Plastic P suffix | 2.0           | 0°C to 70°C    | MC68B45P     |

## PIN ASSIGNMENT

![](http://www.cpctech.org.uk/mc5.gif)
