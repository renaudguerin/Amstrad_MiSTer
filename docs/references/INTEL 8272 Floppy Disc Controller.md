<!--
Converted from "INTEL 8272 Floppy Disc Controller.html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 24 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

Visit Intel's WWW site for more information on recent products and services.

------------------------------------------------------------------------

8272\
SINGLE/DOUBLE DENSITY\
FLOPPY DISK CONTROLLER
======================

&amstrad;

- IBM Compatible in Both Single and Double Density Recording Formats
- Programmable Data Record Lengths: 128,256,512, or 1024 Bytes/Sector
- Multi-Sector and Multi-Track Transfer Capability
- Drive up to 4 Floppy Disks
- Data Scan Capability - Will Scan a Single Sector or an Entire Cylinder's Worth of Data Fields, Comparing on a Byte by Byte Basis, Data in the Processor's Memory with Data Read from the Diskette
- Data Transfers in DMA or Non-DMA Mode
- Parallel Seek Operations on Up to Four Drives
- Compatible with Most Microprocessors including 8080A,8085A,8086 and 8088
- Single-Phase 8 Mhz Clock
- Single +5 Volt Power Supply
- Available in 40-Pin Plastic Dual-in-Line Package

The 8272 is a LSI Floppy Disk Controller (FDC) Chip, which contains the curcuitry and control functions for interfacing a processor to 4 Floppy Disk Drives. It is capable of supporting either IBM 3740 single density format (FM), or IBM System 34 Double Density format (MFM) including double sided recording. The 8272 provides control signals which simplify the design of an external phase locked loop, and write precompensation circuitry. The FDC simplifies and handles most of the burdens associated with implementing a Floppy Disk Drive Interface.\
\

------------------------------------------------------------------------

#### PIN CONFIGURATION

![\[8272 Pin configuration\]](http://www.cpctech.org.uk/fdc1.gif)

#### 8272 INTERNAL BLOCK DIAGRAM

![\[8272 Internal Block Diagram\]](http://www.cpctech.org.uk/fdc3.gif)

#### 8272 SYSTEM BLOCK DIAGRAM

![\[8272 System Block Diagram\]](http://www.cpctech.org.uk/fdc2.gif)

------------------------------------------------------------------------

## DESCRIPTION

Hand-shaking signals are provided in the 8272 which make DMA operation easy to incorporate with the aid of an external DMA controller chip, such as the 8237. The FDC will operate in either DMA or Non-DMA mode. In the Non-DMA mode, the FDC generates interrupts to the processor for every transfer of a data byte between the CPU and 8272. In the DMA mode, the processor need only to load a command into the FDC and all data transfers occur under control of the 8272 and DMA controller.\
\
There are 15 seperate commands which the 8272 will execute. Each of these commands require multiple 8-bit bytes to fully specify the operation which the processor wishes the FDC to perform. The following commands are available.\
\

|                    |                                  |
|--------------------|----------------------------------|
| Read Data          | Write Data                       |
| Read ID            | Format a Track                   |
| Read Deleted Data  | Write Deleted Data               |
| Read a Track       | Seek                             |
| Scan Equal         | Recalibrate (Restore to track 0) |
| Scan High or Equal | Sense Interrupt Status           |
| Scan Low or Equal  | Sense Drive Status               |
| Specify            |                                  |

\
\

------------------------------------------------------------------------

## FEATURES

Address mark detection curcuitry is internal to the FDC which simplifies the phase locked loop and read electronics. The track stepping rate, head load time, and head unload time may be programmed by the user. The 8272 offers many additional features such as multiple sector transfers in both read and write modes with a single command, and full IBM compatibility in both single (FM) and double density (MFM) modes.

------------------------------------------------------------------------

## 8272 REGISTERS - CPU INTERFACE

The 8272 contains two registers which may be accessed by the main system processor, a Status Register and a Data Register. The 8-bit Main Status Register contains the status information of the FDC, and may be accessed at any time. The 8-bit Data Register (actually consists of several registers in a stack with only one register presented to the data bus at a time), stores data, commands, parameters, and FDD status information. Data bytes are read out of, or written into, the Data Register in order to program or obtain the results after execution of a command. The Status Register may only be read and is used to facilitate the transfer of data between the processor and the 8272.\
\
The relationship between the Status/Data registers and the signals /RD,/WR and A0 is shown below.\
\

| **A<sub>0</sub>** | **/RD** | **/WR** | **FUNCTION**              |
|-------------------|---------|---------|---------------------------|
| 0                 | 0       | 1       | Read Main Status Register |
| 0                 | 1       | 0       | Illegal                   |
| 0                 | 0       | 0       | Illegal                   |
| 1                 | 0       | 0       | Illegal                   |
| 1                 | 0       | 1       | Read from Data Register   |
| 1                 | 1       | 0       | Write into Data Register  |

\
\
The bits in the Main Status Register are defined as follows:\
\

| **Bit Number** | **Name** | **Symbol** | **Description** |
|----|----|----|----|
| DB<sub>0</sub> | FDD 0 Busy | D<sub>0</sub>B | FDD number 0 is in the Seek mode |
| DB<sub>1</sub> | FDD 1 Busy | D<sub>1</sub>B | FDD number 1 is in the Seek mode |
| DB<sub>2</sub> | FDD 2 Busy | D<sub>2</sub>B | FDD number 2 is in the Seek mode |
| DB<sub>3</sub> | FDD 3 Busy | D<sub>3</sub>B | FDD number 3 is in the Seek mode |
| DB<sub>5</sub> | Non-DMA mode | NDM | The FDC is in the non-DMA mode. This bit is set only during the execution phase in non-DMA mode. Transition to "0" state indicates execution phase has ended. |
| DB<sub>6</sub> | Data Input/Output | DIO | Indicates direction of data transfer between FDC and Data Register. If DIO = "1" then transfer is from Data Register to the processor. If DIO = "0", then transfer is from the processor to the data register. |
| DB<sub>7</sub> | Request for Master | RQM | Indicates Data Register is ready to send or receive data to or from the processor. Both bits DIO and RQM should be used to perform the handshaking functions of "ready" and "direction" to the processor. |

\
\

------------------------------------------------------------------------

## PIN DESCRIPTION

| **PIN NO.** | **I/O** | **CONNECTION TO** | **DESCRIPTION** | **SYMBOL** |
|----|----|----|----|----|
| 1 | RST | I | µP | Reset: Places FDC in idle state. Resets output lines to FDD to "0" (low) |
| 2 | /RD | I ¹ | µP | Read: Control signal for transfer of data from FDC to Data Bus, when "0" (low). |
| 3 | /WR | I ¹ | µP | Write: Control signal for transfer of data to FDC via Data Bus, when "0" (low). |
| 4 | /CS | I | µP | Chip Select: IC selected when "0" (low), allowing /RD and /WR enabled. |
| 5 | A<sub>0</sub> | I ¹ | µP | Data/Status Reg Select: Selects Data Reg (A<sub>0</sub>=1), or Status Reg (A<sub>0</sub>=0) content be send to Data Bus. |
| 6-13 | DB<sub>0</sub>-DB<sub>7</sub> | I/O ¹ | µP | Data Bus: Bidirectional 8-bit Data Bus |
| 14 | DRQ | O | DMA | Data DMA Request: DMA Request is being made by FDC when DRQ "1" |
| 15 | /DACK | I | DMA | DMA Acknowledge: DMA cycle is active when "0" (low) and Controller is performing DMA transfer. |
| 16 | TC | I | DMA | Terminal Count: Indicates the termination of a DMA transfer when "1" (high) |
| 17 | IDX | I | FDD | Index: Indicates the beginning of a disk track |
| 18 | INT | O | µP | Interrupt: Interrupt request generated by FDC |
| 19 | CLK | I |  | Clock: Single Phase 8 Mhz Squarewave Clock |
| 20 | GND |  |  | Ground: D.C. Power Return |
| 40 | Vcc |  |  | D.C. POWER +5v |
| 39 | /RW / SEEK | O | FDD | Read Write/SEEK: When "1" (high) Seek mode selected and when "0" Read/Write mode selected. |
| 38 | LCT/DIR | O | FDD | Low Current/Direction: Lowers write current on inner tracks in Read/Write mode, determines direction head will step in seek mode. |
| 37 | FR/STP | O | FDD | Fault Reset/Step: Resets fault FF in FDD in Read/Write mode, provides step pulses to move head to another cylinder in seek mode. |
| 36 | HDL | O | FDD | Head Load: Command which causes read/write head in FDD to contact diskette. |
| 35 | RDY | I | FDD | Ready: Indicates FDD is ready to send or receive data. |
| 34 | WP/TS | I | FDD | Write Protect/Two Side: Senses Write Protect status in Read/Write mode and Two side media in Seek mode. |
| 33 | FLT/TRK0 | I | FDD | Fault/Track 0: Senses FDD fault condition in Read/Write mode and Track 0 condition in Seek mode. |
| 31,32 | PS<sub>1</sub>,PS<sub>0</sub> | O | FDD | Precompensation (pre-shift): Write precompensation status during MFM mode. Determines early, late and normal times. |
| 30 | WR DATA | O | FDD | Write data: Seial clock and data bits to FDD. |
| 28,29 | DS<sub>1</sub>,DS<sub>0</sub> | O | FDD | Drive Select: Selects FDD unit |
| 27 | HDSEL | O | FDD | Head Select: Head 1 selected when "1" (high). Head 0 selected when "0" (low). |
| 26 | MFM | O | PLL | MFM Mode: MFM mode when "1", FM mode when "0" |
| 25 | WE | O | FDD | Write Enable: Enables write data into FDD |
| 24 | VCO | O | PLL | VCO Sync: Inhibits VCO in PLL when &quot0" (low), enables VCO when "1". |
| 23 | RD DATA | I | FDD | Read Data: Read data from FDD containing clock and data bits |
| 22 | DW | I | PLL | Data window. Generated by PLL, and used to sample data from FDD |
| 21 | WR CLK | I |  | Write Clock: Write data rate to FDD FM = 500Khz, MFM = 1 Mhz, with a pulse width of 250ns for both FM and MFM. Must be enabled for all operations, both Read and Write. |

¹Disabled when /CS=1

\
\
The DIO and RQM bits in the Status Register indicate when Data is ready and in which direction data will be transfered on the Data Bus.\
\

**STATUS REGISTER TIMING** \[Status Register Timing Diagram\]

\
\
The 8272 is capable of executing 15 different commands. Each command is initiated by a multi-byte transfer from the processor, and the result after execution of the command may also be a multi-byte transfer back to the processor. Because of this multi-byte interchange of information between the 8272 and the processor, it is convenient to consider each command as consisting of three phases.\
\

|  |  |
|----|----|
| Command Phase | The FDC receives all information required to perform a particular operation from the processor. |
| Execution Phase | The FDC performs the operation it was instructed to do. |
| Result Phase | After completion of the operation, status and other housekeeping information are made available to the processor. |

\
\
During Command or Result Phases the Main Status Register (described earlier) must be read by the processor before each byte of information is written into or read from the Data Register. Bits D6 and D7 in the Main Status Register must be in a 0 and 1 state respectively, before each byte of the command word may be written into the 8272. Many of the commands require multiple bytes, and as a result the Main Status Register must be read prior to each byte transfer to the 8272. On the other hand, during the Result Phase, D6 and D7 in the Main Status Register must both be 1's. (D6=1 and D7=1) before reading each byte from the Data Register. Note, this reading of the Main Status Register before each byte transfer to the 8272 is required only in the command and result phases, and NOT during the Execution Phase.\
\
During the Execution Phase, the Main Status Register need not be read. If the 8272 is in the Non-DMA mode, then the receipt of each data byte (if 8272 is reading data from FDD) is indicated by an interrupt signal on pin 18 (INT=1). The generation of a Read signal (/RD=0) will reset the interrupt as well as output the data onto the data bus. For example, if the processor cannot handle interrupts fast enough (every 13µs for MFM mode) then it may poll the Main Status Register and then bit D7 (RQM) functions just like the interrupt signal. If a Write Command is in process then the /WR signal performs the reset to the interrupt signal.\
\
If the 8272 is in the DMA mode, no interrupts are generated during the execution phase. The 8272 generates DRQ's (DMA Requests) when each byte of data is available. The DMA controller responds to this request with both a /DACK=0 (DMA Acknowledge) and a /RD=0 (Read Signal). When the DMA Acknowledge signal goes low (/DACK=0) then the DMA Request is reset (DRQ=0). If a write command has been programmed then a /WR signal will appear instead of /RD. After the execution phase has been completed (Terminal Count has occured) then an interrupt will occur (INT=1). This signifies the beginning of the Result Phase. When the first byte of data is read during the Result Phase, the Interrupt is automatically reset (INT=0).\
\
It is important to note that during the Result Phase all bytes shown in the Command Table must be read. The Read Data Command, for example, has seven bytes of data in the Result Phase. All seven bytes must be read in order to sucessfully complete the Read Data Command. The 8272 will not accept a new command until all seven bytes have been read. Otehr commands may require fewer bytes to be read during the Result Phase.\
\
The 8272 contains five Status Registers. The Main Status Register mentioned above may be read by the processor at any time. The other four status registers (ST0,ST1,ST2 and ST3) are only available during the Result Phase, and may be read only after sucessfully completing a command. The particular command that has been executed determines how many of the Status Registers will be read.\
\
The bytes of data which are sent to the 8272 to form the Command Phase, and are read out of the 8272 in the Result Phase, must occur in the order shown in the Command Table. That is, the Command Code must be sent first and the other bytes sent in the prescribed sequence. No foreshortening of the Command or Result Phases are allowed. After the last byte of data in the Command Phase is sent to the 8272 the Execution Phase automatically starts. In a similar fashion, when the last byte of data is read out in the Result Phase, the command is automatically ended and the 8272 is ready for a new command. A command may be aborted by simply sending a Terminal Count signal to pin 16 (TC=1). This is a convienient means of ensuring that the processor always gets the 8272's attention even if the disk system hangs up in an abnormal manner.

------------------------------------------------------------------------

## POLLING FEATURE OF THE 8272

After the specify command has been sent to the 8272, the Drive Select Lines DS0 and DS1 will automatically go into a polling mode. In between commands (and between step pulses in the SEEK command) the 8272 polls all four FDDs looking for a change in the Ready Line from any of the drives. If the Ready line changes state (usually due to a door opening or closing) then the 8272 will generate an interrupt. When Status Register 0 (ST0) is read (after Sense Interrupt Status is issued), Not Ready (NR) will be indicated. The polling of the Ready line by the 8272 continues continously between instructions thus notifying tthe processor which drives are on or off line.

------------------------------------------------------------------------

## COMMAND DESCRIPTIONS

**TABLE 1. 8272 COMMAND SET**

\
\

| **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MFM | SK | 0 | 0 | 1 | 1 | 0 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution |  |
| Command | W | EOT | EOT | EOT | EOT | EOT | EOT | EOT | EOT |  |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL |  |  |
| Command | W | DTL | DTL | DTL | DTL | DTL | DTL | DTL | DTL |  |  |
| Execution |  |  |  |  |  |  |  |  |  | Data transfer between the FDD and main system |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information after command execution |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information after command execution |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information after command execution |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information after command execution |  |

\
\

| **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** | **READ DELETED DATA** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MFM | SK | 0 | 1 | 1 | 0 | 0 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution |  |
| Command | W | EOT | EOT | EOT | EOT | EOT | EOT | EOT | EOT |  |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL |  |  |
| Command | W | DTL | DTL | DTL | DTL | DTL | DTL | DTL | DTL |  |  |
| Execution |  |  |  |  |  |  |  |  |  | Data transfer between the FDD and main system |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information after command execution |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information after command execution |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information after command execution |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information after command execution |  |

\
\

| **WRITE DATA** | **WRITE DATA** | **WRITE DATA** | **WRITE DATA** | **WRITE DATA** | **WRITE DATA** | **WRITE DATA** | **WRITE DATA** | **WRITE DATA** | **WRITE DATA** | **WRITE DATA** | **WRITE DATA** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MFM | 0 | 0 | 0 | 1 | 0 | 1 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution |  |
| Command | W | EOT | EOT | EOT | EOT | EOT | EOT | EOT | EOT |  |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL |  |  |
| Command | W | DTL | DTL | DTL | DTL | DTL | DTL | DTL | DTL |  |  |
| Execution |  |  |  |  |  |  |  |  |  | Data transfer between the FDD and main system |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information after command execution |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information after command execution |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information after command execution |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information after command execution |  |

\
\

| **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** | **WRITE DELETED DATA** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MFM | 0 | 0 | 1 | 0 | 0 | 1 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution |  |
| Command | W | EOT | EOT | EOT | EOT | EOT | EOT | EOT | EOT |  |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL |  |  |
| Command | W | DTL | DTL | DTL | DTL | DTL | DTL | DTL | DTL |  |  |
| Execution |  |  |  |  |  |  |  |  |  | Data transfer between the FDD and main system |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information after command execution |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information after command execution |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information after command execution |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information after command execution |  |

\
\

| **READ A TRACK** | **READ A TRACK** | **READ A TRACK** | **READ A TRACK** | **READ A TRACK** | **READ A TRACK** | **READ A TRACK** | **READ A TRACK** | **READ A TRACK** | **READ A TRACK** | **READ A TRACK** | **READ A TRACK** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | MFM | SK | 0 | 0 | 0 | 1 | 0 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution |  |
| Command | W | EOT | EOT | EOT | EOT | EOT | EOT | EOT | EOT |  |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL |  |  |
| Command | W | DTL | DTL | DTL | DTL | DTL | DTL | DTL | DTL |  |  |
| Execution |  |  |  |  |  |  |  |  |  | Data transfer between the FDD and main system FDC reads all of cylinders contents from index hole to EOT |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information after command execution |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information after command execution |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information after command execution |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information after command execution |  |

\
\

| **READ ID** | **READ ID** | **READ ID** | **READ ID** | **READ ID** | **READ ID** | **READ ID** | **READ ID** | **READ ID** | **READ ID** | **READ ID** | **READ ID** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | MFM | 0 | 0 | 1 | 0 | 1 | 0 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Execution |  |  |  |  |  |  |  |  |  | The first correct ID information on the cylinder is stored in Data Register |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information during Execution Phase |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information during Execution Phase |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information during Execution Phase |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information during Execution Phase |  |

\
\

| **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** | **FORMAT A TRACK** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | MFM | 0 | 0 | 1 | 1 | 0 | 1 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | N | N | N | N | N | N | N | N | Bytes/sector |  |
| Command | W | SC | SC | SC | SC | SC | SC | SC | SC | Sectors/Cylinder |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL | Gap 3 |  |
| Command | W | D | D | D | D | D | D | D | D | Filler Byte |  |
| Execution |  |  |  |  |  |  |  |  |  | FDC formats an entire cylinder |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | In this case the ID information has no meaning |  |
| Result | R | H | H | H | H | H | H | H | H | In this case the ID information has no meaning |  |
| Result | R | R | R | R | R | R | R | R | R | In this case the ID information has no meaning |  |
| Result | R | N | N | N | N | N | N | N | N | In this case the ID information has no meaning |  |

\
\

| **SCAN LOW** | **SCAN LOW** | **SCAN LOW** | **SCAN LOW** | **SCAN LOW** | **SCAN LOW** | **SCAN LOW** | **SCAN LOW** | **SCAN LOW** | **SCAN LOW** | **SCAN LOW** | **SCAN LOW** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MFM | SK | 1 | 0 | 0 | 0 | 1 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution |  |
| Command | W | EOT | EOT | EOT | EOT | EOT | EOT | EOT | EOT |  |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL |  |  |
| Command | W | STP | STP | STP | STP | STP | STP | STP | STP |  |  |
| Execution |  |  |  |  |  |  |  |  |  | Data compared between the FDD and main system |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information after command execution |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information after command execution |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information after command execution |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information after command execution |  |

\
\

| **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** | **SCAN LOW OR EQUAL** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MFM | SK | 1 | 1 | 0 | 0 | 1 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution |  |
| Command | W | EOT | EOT | EOT | EOT | EOT | EOT | EOT | EOT |  |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL |  |  |
| Command | W | STP | STP | STP | STP | STP | STP | STP | STP |  |  |
| Execution |  |  |  |  |  |  |  |  |  | Data compared between the FDD and main system |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information after command execution |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information after command execution |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information after command execution |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information after command execution |  |

\
\

| **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** | **SCAN HIGH OR EQUAL** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MFM | SK | 1 | 1 | 1 | 0 | 1 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution |  |
| Command | W | EOT | EOT | EOT | EOT | EOT | EOT | EOT | EOT |  |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL |  |  |
| Command | W | STP | STP | STP | STP | STP | STP | STP | STP |  |  |
| Execution |  |  |  |  |  |  |  |  |  | Data compared between the FDD and main system |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information after command execution |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information after command execution |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information after command execution |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information after command execution |  |

\
\

| **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** | **RECALIBRATE** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | 1 | 1 | 1 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | 0 | DS1 | DS0 | Command Codes |  |
| Execution |  |  |  |  |  |  |  |  |  | Head retracked to track 0 |  |

\
\

| **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | Command Codes |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information at the end of each seek operation about the FDC |  |
| Result | R | PCN | PCN | PCN | PCN | PCN | PCN | PCN | PCN | Status information at the end of each seek operation about the FDC |  |

\
\

| **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 1 | Command Codes |  |
| Command | W | SRT | SRT | SRT | SRT | HUT | HUT | HUT | HUT | Command Codes |  |
| Command | W | HLT | HLT | HLT | HLT | HLT | HLT | HLT | ND | Command Codes |  |

\
\

| **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Result | R | ST3 | ST3 | ST3 | ST3 | ST3 | ST3 | ST3 | ST3 | Status information about FDD |  |

\
\

| **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 1 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | HDS | DS1 | DS0 | Command Codes |  |
| Command | W | NCN | NCN | NCN | NCN | NCN | NCN | NCN | NCN | Command Codes |  |
| Execution |  |  |  |  |  |  |  |  |  | Head is positioned over proper Cylinder on diskette |  |

\
\

| **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Data Bus** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid command codes (NoOp - FDC goes into standby state) |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0=80H |  |

\
\

#### Note

- Symbols used in this table are described at the end of this section.
- A<sub>0</sub>=1 for all operations
- X=Don't care, usually made to equal binary 0.

\
\

**TABLE 2: COMMAND MNEUMONICS**

| **SYMBOL** | **NAME** | **DESCRIPTION** |
|----|----|----|
| A<sub>0</sub> | Address Line 0 | A<sub>0</sub> controls selection of Main Status Register (A<sub>0</sub>=0) or Data Register (A<sub>0</sub>=1) |
| C | Cylinder Number | C stands for the current selected Cylinder track number 0 through 76 of the medium |
| D | Data | D stands for the data pattern which is going to be written into a Sector |
| D<sub>7</sub>--D<sub>0</sub> | Data bus | 8-bit Data Bus where D<sub>7</sub> is the most significant bit, and D<sub>0</sub> is the least significant bit. |
| DS0,DS1 | Drive Select | DS stands for a select drive number 0 or 1 |
| DTL | Data Length | When N is defined as 00, DTL stands for the data length which users are going to read out or write into the sector. |
| EOT | End Of Track | EOT stands for the final Sector number of a Cylinder |
| GPL | Gap Length | GPL stands for the length of Gap 3 (spacing between sectors excluding VCO Sync field) |
| H | Head Address | H stands for head number 0 or 1, as specified in the ID field |
| HDS | Head Select | HDS stands for a selected head number 0 or 1 (H=HDS in all command words) |
| HLT | Head Load Time | HLT stands for the head load time in the FDD (2 to 254ms in 2ms increments) |
| HUT | Head Unload Time | HUT stands for the head unload time after a read or write operation has occured (16 to 240ms in 16ms increments) |
| MFM | FM or MFM Mode | If MF is low, FM mode is selected and if it is high, MFM mode is selected. |
| MT | Multi-track | If MT is high, a multi-track operation is to be performed (a cylinder under both HD0 and HD1 will be read or written) |
| N | Number | N stands for the number of data bytes written in a sector |
| NCN | New Cylinder Number | NCN stands for a new Cylinder Number which is going to be reached as a result of the Seek operation. Desired Position of head. |
| ND | Non-DMA Mode | ND stands for operation in the Non-DMA mode |
| PCN | Present Cylinder No. | PCN stands for the Cylinder number at the completion of SENSE INTERRUPT STATUS Command. Position of head at present time. |
| R | Record | R stands for the Sector Number which will be read or written. |
| R/W | Read/Write | R/W stands for either Read (R) or Write (W) signal. |
| SC | Sector | SC indicates the number of sectors per track |
| SK | Skip | SK stands for Skip Deleted Data Address Mark |
| SRT | Step Rate Time | SRT stands for the Stepping Rate for the FDD (1 to 16ms in 1ms increments). The same Stepping Rate applies to all drives (F=1ms, E=2ms, etc). |
| ST0,ST1,ST2,ST3 | Status 0,Status 1,Status 2,Status 3 | ST0-3 stand for one of four registers which store the status information after a command has been executed. This information is available during the result phase after command execution. These registers should not be confused with the Main Status Register (selected by A0=0). ST0-3 may be read only after a command has been executed and contain information relevant to that particular command. |
| STP |  | During a scan operation, if STP=1 the data in contiguous sectors is compared byte-by-byte with data sent from the processor (or DMA) and if STP=2 then alternate sectors are read and compared. |

------------------------------------------------------------------------

## COMMAND DESCRIPTIONS

During the Command Phase, the Main Status Register must be polled by the CPU before each byte is written into the Data Register. The DIO (DB6) and RQM (DB7) bits in the Main Status Register must be in the "0" and "1" states respectively, before each byte of the command may be written into the 8272. The beginning of the exection phase for any of these commands will cause DIO and RQM to switch to "1" and "0" states respectively.

### READ DATA

A set of nine (9) byte words are required to place the FDC into the Read Data Mode. After the Read Data command has been issued the FDC loads the head (if it is in the unloaded state), waits the specified head settling time (defined in the Specy Command), and begins reading ID Address Marks and ID fields. When the current sector number ("R") stored in the ID Register (IDR) compares with the sector number read off the diskette, then the FDC outputs data (from the data field) byte-by-byte to the main system via the data bus.\
\
After completion of the read operation from the current sector, the Sector Number is incremented by one, and the data from the next sector is read and output on the data bus. This continuous read function is called a "Multi-Sector Read Operation". The Read Data Command may be terminated by the receipt of a Terminal Count signal. Upon receipt of this signal, the FDC stops outputting data to the processor, but will continue to read data from the current sector, check CRC (Cyclic Redundancy Count) bytes, and then at the end of the sector terminate the Read Data Command.\
\
The amount of data which can be handled with a single command to the FDC depends upon MT (multi-track), MFM (MFM/FM), and N (Number of Bytes/Sector). Table 3 below shows the Transfer Capacity.\
\

**TABLE 3. TRANSFER CAPACITY**

| **Multi-Track MT** | **MFM/FM** | **Bytes/Sector N** | **Maximum Transfer Capacity (Bytes/Sector)(Number of Sectors)** | **Final Sector Read from Diskette** |
|----|----|----|----|----|
| 0 | 0 | 0 | (128)(26)=3,328 | 26 at Side 0 or 26 at Side 1 |
| 0 | 1 | 1 | (256)(26)=6,656 | 26 at Side 0 or 26 at Side 1 |
| 1 | 0 | 0 | (128)(52)=6,656 | 26 at Side 1 |
| 1 | 1 | 1 | (256)(52)=13,312 | 26 at Side 1 |
| 0 | 0 | 1 | (256)(15)=3,840 | 15 at Side 0 or 15 at Side 1 |
| 0 | 1 | 2 | (512)(15)=7,680 | 15 at Side 0 or 15 at Side 1 |
| 1 | 0 | 1 | (256)(30)=7,680 | 15 at Side 1 |
| 1 | 1 | 2 | (512)(30)=15,360 | 15 at Side 1 |
| 0 | 0 | 2 | (512)(8)=4,096 | 8 at Side 0 or 8 at Side 1 |
| 0 | 1 | 3 | (1024)(8)=8,192 | 8 at Side 0 or 8 at Side 1 |
| 1 | 0 | 2 | (512)(16)=8,192 | 8 at Side 1 |
| 1 | 1 | 3 | (1024)(16)=16,384 | 8 at Side 1 |

\
\
The "multi-track" function (MT) allows the FDC to read data from both sides of the diskette. For a particular cylinder, data will be transferred starting at Sector 1, Side 0 and completing at Sector L, side 1 (Sector L = last sector on the side). Note, this function pertains to only one cylinder (the same track) on each side of the diskette.\
\
When N=0, then DTL defines the data length which the FDC must treat as a sector. If DTL is smaller than the actual data length in a sector, the data beyond DTL in the sector, is not sent to the Data Bus. The FDC reads (internally) the complete sector performing the CRC check, and depending upon the manner of command termination, may perform a Multi-Sector Read Operation. When N is non-zero, then DTL has no meaning and should be set to 0ffh.\
\
At the completion of the Read Data Command, the head is not unloaded until after Head Unload Time Interval (specified in the Specify Command) has elapsed. If the processor issues another command before the head unloads then the head settling time may be saved between subsequent reads. This time out is particularly valuable when a diskette is copied from one drive to another.\
\
If the FDC detects the Index Hole twice without finding the right sector, (indicated in "R"), then the FDC sets the ND (No Data) flag in Status Register 1 to a 1 (high), and terminates the Read Data Command. (Status Register 0 also has bits 7 and 6 set to 0 and 1 respectively).\
\
After reading the ID and Data Fields in each sector, the FDC checks the CRC bytes. If an error is detected, (incorrect CRC in ID field), the FDC sets the DE (Data Error) flag in Status register 1 to a 1 (high), and if a CRC error occurs in the Data Field the FDC also sets the DD (Data Error in Data Field) flag in Status Register 2 to a 1 (high), and terminates the Read Data Command. (Status Register 0 also has bits 7 and 6 set to 0 and 1 respectively.)\
\
If the FDC reads a Deleted Data Address Mark off the diskette, and SK bit (bit D5 in the first Command Word) is not set (SK=0), then the FDC sets the CM (Control Mark) flag in Status Register 2 to a 1 (high), and terminates the Read Data Command, after reading all the data in the Sector. If SK=1, the FDC skips the sector with the Deleted Data Address Mark and reads the next sector.\
\
During disk data transfers between the FDC and the processor, via the data bus, the FDC must be serviced by the processor every 27µs in the FM mode, and every 13µs in the MFM mode, or the FDC sets the OR (Over Run) flag in Status Register 1 to a 1 (high), and terminates the Read Data Command.\
\
If the processor terminates a read (or write) operation in the FDC, then the ID information in the Result Phase is dependant upon the state of the MT bit and the EOT byte. Table 4 shows the values for C,H,R and N, when the processor terminates the Command.\
\

**TABLE 4: ID INFORMATION WHEN PROCESSOR TERMINATES COMMAND**

| **MT** | **EOT** | **Final Sector Transfered to Processor** | **C** | **H** | **R** | **N** |
|----|----|----|----|----|----|----|
| 0 | 1a | Sector 1 to 25 at Side 0 | NC | NC | R+1 | NC |
| 0 | 0f | Sector 1 to 24 at Side 0 | NC | NC | R+1 | NC |
| 0 | 08 | Sector 1 to 7 at Side 0 | NC | NC | R+1 | NC |
| 0 | 1a | Sector 26 at Side 0 | C+1 | NC | R=01 | NC |
| 0 | 0f | Sector 15 at Side 0 | C+1 | NC | R=01 | NC |
| 0 | 08 | Sector 8 at Side 0 | C+1 | NC | R=01 | NC |
| 0 | 1a | Sector 1 to 25 at Side 1 | NC | NC | R+1 | NC |
| 0 | 0f | Sector 1 to 14 at Side 1 | NC | NC | R+1 | NC |
| 0 | 08 | Sector 1 to 7 at Side 1 | NC | NC | R+1 | NC |
| 0 | 1a | Sector 26 at Side 1 | C+1 | NC | R=01 | NC |
| 0 | 0f | Sector 15 at Side 1 | C+1 | NC | R=01 | NC |
| 0 | 08 | Sector 8 at Side 1 | C+1 | NC | R=01 | NC |
| 1 | 1a | Sector 1 to 25 at Side 0 | NC | NC | R+1 | NC |
| 1 | 0f | Sector 1 to 14 at Side 0 | NC | NC | R+1 | NC |
| 1 | 08 | Sector 1 to 7 at Side 0 | NC | NC | R+1 | NC |
| 1 | 1a | Sector 26 at Side 0 | NC | LSB | R=01 | NC |
| 1 | 0f | Sector 15 at side 0 | NC | LSB | R=01 | NC |
| 1 | 08 | Sector 8 at side 0 | NC | LSB | R=01 | NC |
| 1 | 1a | Sector 1 to 25 at Side 1 | NC | NC | R+1 | NC |
| 1 | 0f | Sector 1 to 14 at Side 1 | NC | NC | R+1 | NC |
| 1 | 08 | Sector 1 to 7 at Side 1 | NC | NC | R+1 | NC |
| 1 | 1a | Sector 26 at Side 1 | C+1 | LSB | R=01 | NC |
| 1 | 0f | Sector 15 at Side 1 | C+1 | LSB | R=01 | NC |
| 1 | 08 | Sector 7 at side 1 | C+1 | LSB | R=01 | NC |

NC (No change): The same value as the one at the beginning of command execution\
LSB (Least Significant Bit): The least significant bit of H is complemented.

### WRITE DATA

A set of nine (9) bytes are required to set the FDC into the Write Data Mode. After the Write Data command has been issued the FDC loads the head (if it is in the unloaded state), waits the specified head settling time (defined in the Specify Command), and begins reading ID Fields. When the current sector number ("R"), stored in the ID Register (IDR) compares with the sector number read off the diskette, then the FDC takes data from the processor byte-by-byte via the data bus, and outputs it to the FDD.\
\
After writing data into the current sector, the Sector Number stored in "R" is incremented by one, and the next data field is written into. The FDC continues this "Multi-Sector Write Operation" until the issuance of a Terminal Count signal. If a Terminal Count Signal is sent to the FDC it continues writing into the current sector to complete the data field. If the Terminal Count signal is received while a data field is being written then the remainder of the data field is filled with 00 (zeros).\
\
The FDC reads the ID field of each sector and checks the CRC bytes. If the FDC detects a read error (incorrect CRC) in one of the ID fields, it sets the DE (Data Error) flag of Status Register 1 to a 1 (high), and terminates the Write Data Command. (Status Register 0 also has bits 7 and 6 set to 0 and 1 respectively).\
\
The Write Command operates in much the same manner as the Read Command. The following items are the same, refer to the Read Data Command for details:

- Transfer Capacity
- EN (End of cylinder flag)
- ND (No data) flag
- Head Unload Time Interval
- ID Information when processor terminates command (see table 2)
- Definition of DTL when N=0 and when N\<\>0

In the Write Data mode, data transfers between the processor and FDC must occur every 31µs in the FM mode and every 15µs in the MFM mode. If the time interval between data transfers is longer than this then the FDC sets the OR (Over Run) flag in Status Register 1 to a 1 (high), and terminates the Write Data Command.

### WRITE DELETED DATA

This command is the same as the Write Data Command except a Deleted Data Address Mark is written at the beginning of the Data Field instead of the normal Data Address Mark.

### READ DELETED DATA

This command is the same as the Read Data Command except that when the FDC detects a Data Address Mark at the beginning of a Data Field (and SK=0 (low)). It will read all the data in the sector and set the CM flag in Status Register 2 to a 1 (high), and then terminate the command. If SK=1, then the FDC skips the sector with the Data Address Mark and reads the next sector.

### READ A TRACK

This command is similar to READ DATA command except that the entire data field is read continuously from each of the sectors of a track. Immediatly after encountering the INDEX HOLE, the FDC starts reading all data fields on the track as continuous blocks of data. If the FDC finds an error in the ID or DATA CRC check bytes, it continues to read data from the track. The FDC compares the ID information read from each sector with the value stored in the IDR, and sets the ND flag of Status Register 1 to a 1 (high) if there is no comparison. Multi-track or skip operations are not allowed with this command.\
\
This command terminates when EOT number of sectors have been read. If the FDC does not find an ID address mark on the diskette after it encounters the INDEX HOLE for the second time, then it sets the MA (missing address mark) flag in Status Register 1 to a 1 (high), and terminates the command. (Status Register 0 has bits 7 and 6 set to 0 and 1 respectively).

### READ ID

The READ ID command is used to give the present position of the recording head. The FDC stores the values from the first ID Field it is able to read. If no proper ID Address Mark is found on the diskette, before the INDEX HOLE is encountered for the second time then the MA (Missing Address Mark) flag in Status Register 1 is set to a 1 (high), and if no data is found then the ND (No Data) flag is set in Status Register 1 to a 1 (high) and the command is terminated.

### FORMAT A TRACK

The Format Command allows an entire track to be formatted. After the INDEX HOLE is detected, Data is written on the Diskette: Gaps, Address Marks, ID Fields and Data Fields, all per the IBM System 34 (Double Density) or System 3740 (Single Density) Format are recorded. The particular format which will be written is controlled by the values programmed into N (Number of bytes/sector), SC (Sectors/cylinder), GPL (Gap Length) and D (Data Pattern) which are supplied by the processor during the Command Phase. The Data Field is filled with the byte of data stored in D. The ID Field for each sector is supplied by the processor; that is, four data requests per sector are made by the FDC for C (Cylinder Number), H (Head Number), R (Sector Number) and N (Number of bytes/sector). This allows the diskette to be formatted with non-sequential sector numbers, if desired.\
\
After formatting each sector, the processor must send new values for C,H,R and N to the 8272 for each sector on the track. The contents of the R register is incremented by one after each sector is formatted, thus, the R register contains a value of R+1 when it is read during the Result Phase. This incrementing and formatting continues for the whole track until the FDC encounters the INDEX HOLE for the second time, whereupon it terminates the command.\
\
If a FAULT signal is received from the FDD at the end of a write operation, then the FDC sets the EC flag of Status Register 0 to a 1 (high), and terminates the command after setting bits 7 and 6 of Status Register 0 to a 0 and 1 respectively. Also the loss of the READY signal at the beginning of a command execution phase causes command termination.\
\
Table 5 shows the relationship between N, SC and GPL for various sector sizes:\
\

**TABLE 5: SECTOR SIZE RELATIONSHIPS**

| **FORMAT** | **SECTOR SIZE**  | **N** | **SC** | **GPL¹** | **GPL²** | **REMARKS**     |
|------------|------------------|-------|--------|----------|----------|-----------------|
| FM Mode    | 128 bytes/sector | 0     | 1Ah    | 07h      | 1Bh      | IBM Diskette 1  |
| FM Mode    | 256              | 1     | 0Fh    | 0Eh      | 2Ah      | IBM Diskette 2  |
| FM Mode    | 512              | 2     | 08     | 1Bh      | 3Ah      | IBM Diskette 2  |
| FM Mode    | 1024             | 3     | 4      | \-       | \-       |                 |
| FM Mode    | 2048             | 4     | 2      | \-       | \-       |                 |
| FM Mode    | 4096             | 5     | 1      | \-       | \-       |                 |
| MFM Mode   | 256              | 1     | 1Ah    | 0Eh      | 36h      | IBM Diskette 2D |
| MFM Mode   | 512              | 2     | 0Fh    | 1Bh      | 54h      | IBM Diskette 2D |
| MFM Mode   | 1024             | 3     | 08     | 35h      | 74h      | IBM Diskette 2D |
| MFM Mode   | 2048             | 4     | 4      | \-       | \-       | IBM Diskette 2D |
| MFM Mode   | 4096             | 5     | 2      | \-       | \-       | IBM Diskette 2D |
| MFM Mode   | 8192             | 6     | 1      | \-       | \-       | IBM Diskette 2D |

¹Suggested values of GPL in Read or Write Commands to avoid splice point between data field and ID field of contiguous sections.\
²Suggested values of GPL in format command.

### SCAN COMMANDS

The SCAN commands allow data which is being read from the diskette to be compared against data which is being supplied from the main system (Processor in NON-DMA mode, and DMA Controller in DMA mode). The FDC compares the data on a byte-by-byte basis, and looks for a sector of data which meets the conditions of D<sub>Fdd</sub>=D<sub>Processor</sub>, D<sub>Fdd</sub>\<=D<sub>Processor</sub> or D<sub>Fdd</sub>\>=D<sub>Processor</sub>. Ones complement arithmetic is used to comparison (FF = largest number, 00 = smallest number). After a whole sector of data is compared, if the conditions are not met, the sector number is incremented (R+STP -\> R), and the scan operation is continued. The scan operation continues until one of the following conditions occur; the conditions for scan are met (equal, low or high), the last sector on the track is reached (EOT), or the terminal count signal is received.\
\
If the conditions for scan are met then the FDC sets the SH (Scan Hit) flag of Status Register 2 to a 1 (high), and terminates the Scan Command. If the conditions for scan are not met between the the starting sector (as specified by R) and the last sector on the cylinder (EOT), then the FDC sets the SN (Scan Not Satisfied) flag of Status Register 2 to a 1 (high), and terminates the Scan Command. The receipt of a TERMINAL COUNT signal from the processor or DMA Controller during the scan operation will cause the FDC to complete the comparison of the particular byte which is in process, and then to terminate the command. Table 6 shows the status of bits SH and SN under various conditions of SCAN.\
\

**TABLE 6. SCAN STATUS CODES**

| **Command** | **Bit 2 = SN** | **Bit 3 = SH** | **Comments** |
|----|----|----|----|
| Scan Equal | 0 | 1 | D<sub>Fdd</sub>=D<sub>Processor</sub> |
| Scan Equal | 1 | 0 | D<sub>Fdd</sub>\<\>D<sub>Processor</sub> |
| Scan Low or Equal | 0 | 1 | D<sub>Fdd</sub>=D<sub>Processor</sub> |
| Scan Low or Equal | 0 | 0 | D<sub>Fdd</sub>\<D<sub>Processor</sub> |
| Scan Low or Equal | 1 | 0 | D<sub>Fdd</sub>\>D<sub>Processor</sub> (not \<=) |
| Scan High or Equal | 0 | 1 | D<sub>Fdd</sub>=D<sub>Processor</sub> |
| Scan High or Equal | 0 | 0 | D<sub>Fdd</sub>\>D<sub>Processor</sub> |
| Scan High or Equal | 1 | 0 | D<sub>Fdd</sub>\<D<sub>Processor</sub> (not \>=) |

\
\
If the FDC encounters a Deleted Data Address Mark on one of the sectors (and SK=0), then it regards the sector as the last sector on the cylinder, sets CM (Control Mark) flag of Status Register 2 to a 1 (high) and terminates the command. If SK=1, the FDC skips the sector with the Deleted Address Mark, and reads the next sector. In the second case (SK=1), the FDC sets the CM (Control Mark) flag of Status Register 2 to a 1 (high) in order to show that a Deleted Sector had been encountered.\
\
When either the STP (contiguous sectors STP=01, or alternate sectors STP=02 sectors are read) or the MT (Multi-Track) are programmed, it is necessary to remember that the last sector on the track must be read. For example, if STP=02, MT=0, the sectors are numbered sequentially 1 through 26, and we start the scan at sector 21; the following will happen. Sectors 21,23 and 25 will be read, then the next sector (26) will be skipped and the Index Hole will be encountered before the EOT value of 26 can be read. This will result in an abnormal termination of the command. If the EOT had been set at 25, or the scanning started at sector 20, then the Scan Command would be completed in a normal manner.\
\
During the Scan Command data is supplied by either the processor or DMA Controller for comparison against the data read from the diskette. In order to avoid having the OR (Over Run) flag set in Status Register 1, it is necessary to have the data available in less than 27µs (FM mode) or 13µs (MFM mode). If an Overrun occurs the FDC terminates the command.

### SEEK

The read/write head within the FDD is moved from cylinder to cylinder under control of the Seek Command. The FDC compares the PCN (Present Cylinder Number) which is the current head position with the NCN (New Cylinder Number), and performs the following operation if there is a difference.\
\
PCN\<NCN: Direction signal to FDD set to a 1 (high) and Step Pulses are issued (Step In).\
\
PCN\>NCN: Direction signal to FDD set to a 0 (low), and Step Pulses are issued (Step Out).\
\
The rate at which Step Pulses are issued is controlled by SRT (Stepping Rate Time) in the SPECIFY command. After each Step Pulse is ussued NCN is compared against PCN, and when NCN=PCN, then the SE (Seek End) flag is set in Status Register 0 to a 1 (high), and the command is terminated.\
\
During the Command Phase of the Seek operation the FDC is in the FDC BUSY state, but during the Execution Phase it is in the NON BUSY state. While the FDC is in the NON BUSY state, another Seek Command may be issued, and in this manner parallel seek operations may be done on up to 4 Drives at once.\
\
If an FDD is in a NOT READY state at the beginning of the command execution phase or during the seek operation, then the NR (NOT READY) flag is set in Status Register 0 to a 1 (high), and the command is terminated.

### RECALIBRATE

This command causes the read/write head within the FDD to retract to the Track 0 position. The FDC clears the contents of the PCN counter, and checks the status of the Track 0 signal from the FDD. As long as the Track 0 signal is low, the Direction signal remains 1 (high) and Step Pulses are issued. When the track 0 signal goes high, he SE (SEEK END) flag in Status Register 0 is set to a 1 (high) and the command is terminated. If the Track 0 signal is still low after 77 Step Pulses have been issued, the FDC sets the SE (SEEK END) and EC (EQUIPMENT CHECK) flags of Status Register 0 to both 1s (highs), and terminates the command.\
\
The ability to overlap RECALIBRATE Commands to multiple FDDs, and the loss of the READY signal, as described in the SEEK Command, also applies to the RECALIBRATE command.

### SENSE INTERRUPT STATUS

An Interrupt signal is generated by the FDC for one of the following reasons:

- Upon entering the Result Phase of:
  - Read Data command
  - Read a Track command
  - Read ID Command
  - Read Deleted Data Command
  - Write Data Command
  - Format a Cylinder Command
  - Write Deleted Data Command.
  - Scan Commands
- Ready line of FDD changes state
- End of Seek or Recalibrate Command
- During Execution Phase in the NON-DMA mode.

Interrupts caused by reasons 1 and 4 above occur during normal command operations and are easily decernible by the processor. However, interupts causes by reasons 2 and 3 above may be uniquely identified with the aid of the Sense Interrupt Status Command. This command when issued resets the interrupt signal and via bits 5,6 and 7 of the Status Register 0 identifies the cause of the interrupt.\
\

**TABLE 7. SEEK INTERRUPT CODES**

| **BIT 5** | **BIT 6** | **BIT 7** | **CAUSE** |
|----|----|----|----|
| 0 | 1 | 1 | Ready Line Changed state, either polarity |
| 1 | 0 | 0 | Normal Termination of Seek or Recalibrate Command |
| 1 | 1 | 0 | Abnormal Termination of Seek or Recalibration command |

\
\
Neither the Seek or Recalibrate Command have a result phase. Therefore it is mandatory to use the Sense Interrupt Status Command after these commands to effectively terminate them and to provide verification of head position (PCN).

### SPECIFY

The Specify Command sets the initial values for each of the three internal timers. The HUT (Head Unload Time) defines the time from the end of the Execution Phase of one of the Read/Write Commands to the head unload state. The Timer is programmable from 16 to 240ms in increments of 16 ms (01=16 ms, 02=32 ms.....0F=240ms). The SRT (Step Rate Time) defines the time interval between adjacent step pulses. This timer is programmable from 1 to 16ms in increments of 1 ms (F = 1ms, E = 2ms, D=3ms, etc). The HLT (Head Load Time) defines the time between when the Head Load signal goes high and when the Read/Write operation starts. This timer is programmable from 2 to 254 ms in increments of 2ms (01=2ms, 02=4ms, 03=6ms ..... FE=254 ms).\
\
The time intervals mentioned above are a direction function of the clock (CLK on pin 19). Times indicated above are for an 8 Mhz clock, if the clock was reduced to a 4Mhz (mini-floppy application) then all time intervals are increased by a factor of 2.\
\
The choice of DMA or NON-DMA operation is made by the ND (NON-DMA) bit. When this bit is high (ND=1) the NON-DMA mode is selected, and when ND=0 the DMA mode is selected.

### SENSE DRIVE STATUS

This command may be used by the processor whenever it wishes to obtain the status of the FDDs. Status Register 3 contains the Drive Status information.

### INVALID

If an invalid command is sent to the FDC (a command not defined above), then the FDC will terminate the command. No interrupt is generated by the 8272 during this condition. Bit 6 and Bit 7 (DIO and RQM) in the Main Status Register are both high ("1") indicating to the processor that the 8272 is in the Result Phase and the contents of Status Register (ST0) must be read. When the processor reads Status Register 0 it will find a 80H indicating an invalid command was received.\
\
A Sense Interrupt Status Command must be sent after a Seek or Recalibrate interrupt, otherwise the FDC will consider the next command to be an Invalid Command.\
\
In some applications the user may wish to use this command as a No-Op command, to place the FDC in a standby or no operation state.\
\

**TABLE 8. STATUS REGISTERS**

\
\

**STATUS REGISTER 0**

| **Bit No.** | **Name** | **Symbol** | **Description** |
|----|----|----|----|
| D<sub>7</sub>,D<sub>6</sub> | Interrupt Code | IC |  |
| D<sub>7</sub>,D<sub>6</sub> | Interrupt Code | IC | D<sub>7</sub>=0 and D<sub>6</sub>=0; Normal Termination of Command (NT), Command was completed and properly executed. |
| D<sub>7</sub>,D<sub>6</sub> | Interrupt Code | IC | D<sub>7</sub>=0 and D<sub>6</sub>=1; Abnormal Termination of Command (AT), Execution of Command was started, but was not successfully completed. |
| D<sub>7</sub>,D<sub>6</sub> | Interrupt Code | IC | D<sub>7</sub>=1 and D<sub>6</sub>=0; Invalid Command issued (IC), Command which was issued was never started. |
| D<sub>7</sub>,D<sub>6</sub> | Interrupt Code | IC | D<sub>7</sub>=1 and D<sub>6</sub>=1; Abnormal Termination because during command execution the ready signal from the FDD changed state. |
| D<sub>5</sub> | Seek End | SE | When the FDC completes the SEEK command, this flag is set to 1 (high). |
| D<sub>4</sub> | Equipment Check | EC | If a fault signal is received from the FDD, or if the Track 0 signal fails to occur after 77 Step Pulses (Recalibrate Command) then this flag is set. |
| D<sub>3</sub> | Not Ready | NR | When the FDD is in the not-ready state and a read or write command is issued, this flag is set. If a read or write command is issued to Side 1 of a single sided drive then this flag is set. |
| D<sub>2</sub> | Head Address | HD | This flag is used to indicate the state of the head at interrupt. |
| D<sub>1</sub> | Unit Select 1 | US 1 | These flags are used to indicate the drive unit number at interrupt. |
| D<sub>0</sub> | Unit Select 0 | US 0 | These flags are used to indicate the drive unit number at interrupt. |

\
\
\
\

**STATUS REGISTER 1**

| **Bit No.** | **Name** | **Symbol** | **Description** |
|----|----|----|----|
| D<sub>7</sub> | End of Cylinder | EN | When the FDC tries to access a sector beyond the final Sector of a Cylinder, this flag is set. |
| D<sub>6</sub> |  |  | Not used. This bit is always 0 (low). |
| D<sub>5</sub> | Data Error | DE | When the FDC detects a CRC error in either the ID field or the data field, this flag is set. |
| D<sub>4</sub> | Over Run | OR | If the FDC is not serviced by the main systems during data transfers within a certain time interval, this flag is set. |
| D<sub>3</sub> |  |  | Not used. This bit always 0 (low). |
| D<sub>2</sub> | No Data | ND |  |
| D<sub>2</sub> | No Data | ND | During execution of READ DATA, WRITE DELETED DATA or SCAN Command, if the FDC cannot find the Sector specified in the IDR register, then this flag is set. |
| D<sub>2</sub> | No Data | ND | During executing the READ ID Command, if the FDC cannot read the ID field without an error, then this flag is set. |
| D<sub>2</sub> | No Data | ND | During execution of the READ A Cylinder Command, if the starting sector cannot be found then this flag is set. |
| D<sub>1</sub> | Not Writeable | NW | During execution of WRITE DATA, WRITE DELETED DATA or Format a Cylinder Command, if the FDC detects a write protect signal from the FDD, then this bit is set. |
| D<sub>0</sub> | Missing Address Mark | MA |  |
| D<sub>0</sub> | Missing Address Mark | MA | If the FDC cannot detect the ID Address Mark after encountering the index hole twice, then this flag is set. |
| D<sub>0</sub> | Missing Address Mark | MA | If the FDC cannot detect the Data Address Mark or the Deleyed Data Address Mark, this flag is set. Also at the same time, the MD (Missing Address Mark in Data Field) of Status Register is set. |

\
\
\
\

**STATUS REGISTER 2**

| **Bit No.** | **Name** | **Symbol** | **Description** |
|----|----|----|----|
| D<sub>7</sub> |  |  | Not used. This bit is always 0 (low). |
| D<sub>6</sub> | Control Mark | CM | During executing the READ DATA or SCAN Command, if the FDC encounters a Sector which contains a Deleted Data Address Mark, this flag is set. |
| D<sub>5</sub> | Data Error in Data Field | DD | If the FDC detects a CRC error in the data field then this flag is set. |
| D<sub>4</sub> | Wrong Cylinder | WC | This bit is related with the ND bit, and when the contents of C on the medium is different from that stored in the IDR, this flag is set. |
| D<sub>3</sub> | Scan Equal Hit | SH | During execution, the SCAN Command, if the condition of the "equal" is satisfied, then this flag is set. |
| D<sub>2</sub> | Scan Not Satisfied | SN | During executing the SCAN Command, if the FDC cannot find a sector on the cylinder which meets the condition, then this flag is set. |
| D<sub>1</sub> | Bad Cylinder | BC | This bit is related with the ND bit and when the contents of C on the mdeium is different from that stored in the IDR and the content of C is &FF, then this flag is set. |
| D<sub>0</sub> | Missing Address Mark in Data Field | MD | When data is read from the medium, if the FDC cannot find a Data Address Mark or Deleted Data Address Mark, then this flag is set. |

\
\
\
\

**STATUS REGISTER 3**

| **Bit No.** | **Name** | **Symbol** | **Description** |
|----|----|----|----|
| D<sub>7</sub> | Fault | FT | This bit is used tto indicate the status of the Fault signal from the FDD. |
| D<sub>6</sub> | Write Protected | WP | This bit is used to indicate the status of the Write Protected signal from the FDD. |
| D<sub>5</sub> | Ready | RDY | This bit is used to indicate the status of the Ready signal from the FDD. |
| D<sub>4</sub> | Track 0 | T0 | This bit is used to indicate the status of the Track 0 signal from the FDD. |
| D<sub>3</sub> | Two Side | TS | This bit is used to indicate the status of the Two Side signal from the FDD. |
| D<sub>2</sub> | Head Address | HD | This bit is used to indicate the status of the Side Select signal to the FDD. |
| D<sub>1</sub> | Unit Select 1 | US 1 | This bit is used to indicate the status of the Unit Select 1 signal to the FDD. |
| D<sub>0</sub> | Unit Select 0 | US 0 | This bit is used to indicate the status of the Unit Select 0 signal to the FDD. |
