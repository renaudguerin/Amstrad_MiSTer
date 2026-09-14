<!--
Converted from "µPD765A_µPD7265 Floppy Disc Controller.html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 27 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

NEC\
NEC Electronics Ltd
===================

µPD765A/µPD765B\
Single/Double Density\
Floppy Disk Controller
======================

## Description

The µPD765A is an LSI floppy disk controller (FDC) chip which contains the circuitry and control functions for interfacing a processor to 4 floppy disk drives, it is capable of either IBM 3740 single density format (FM), or IBM System 34 double density format (MFM) including double- sided recording. The µPD765A/B provides control signals which simplify the design of an external phase-locked loop and write precompensation circuitry. The FDC simplifies and handles most of the burdens associated with implementing a floppy disk interface.\
\
Hand-shaking signals are provided in the µPD765A/B which make DMA operation easy to incorporate with the aid of an external DMA controller chip, such as the µPD8257. The FDC will operate in either the DMA or non-DMA mode. In the non-DMA mode the FDC generates interrupts to the processor every time a data byte is to be transferred. In the DMA mode, the processor need only load the command into the FDC and all data transfers occur under control of the FDC and DMA controllers.\
\
There are 16 commands which the µPD765A/µPD765B will execute. Most of these commands require multiple 8-bit bytes to fully specify the operation which the processor wishes the FDC to perform. The following commands are available:\
\

|                    |                         |
|--------------------|-------------------------|
| Read Data          | Read Deleted Data       |
| Read ID            | Write Data              |
| Specify            | Write ID (Format Write) |
| Read Diagnostic    | Write Deleted Data      |
| Scan Equal         | Seek                    |
| Scan High or Equal | Recalibrate             |
| Scan Low or Equal  | Sense Interrupt Status  |
| Version            | Sense Drive Status      |

\
\

## Features

Address mark detection circuitry is internal to the FDC which simplifies the phase-locked loop and read electronics. The track stepping rate, head load time, and head unload time are user-programmable. The µPD765A/µPD765B offers additional features such as multi-track and multi-side read and write commands and single and double-density capabilities.

- FM,MFM control
- Variable recording length: 128,256,....8192 bytes/sector
- IBM-compatible format (single- and double-sided, single- and double- density)
- Multi-Sector and Multi-track transfer capability
- Drive up to 4 floppy or micro floppydisk drives
- Data scan capability - will scan a single sector or an entire cylinder comparing byte-for-byte host memory and disk data
- Data transfers in DMA or non-DMA mode
- Parallel seek operations on up to four drives
- Compatible with µPD8080/85, µPD8086/88, V-series and µPD780 (Z80(R)) microprocessors
- Single-phase clock: 8 Mhz maximum
- +5V only

. z80 is a registered trademark of the Zilog Corporation

## Pin Configuration

# Ordering Information

| Device Number       | Package Type       | Max Freq. of Operation |
|---------------------|--------------------|------------------------|
| µPD765AC,µPD765AC2  | 40-pin plastic DIP | 8 Mhz                  |
| µPD7265C,µPD7265C-2 | 40-pin plastic DIP | 8 Mhz                  |

## Pin Identification

| No.   | Symbol                        | Function                     |
|-------|-------------------------------|------------------------------|
| 1     | RESET                         | Reset input                  |
| 2     | /RD                           | Read control input           |
| 3     | /WR                           | Write control input          |
| 4     | /CS                           | Chip select input            |
| 5     | A<sub>0</sub>                 | Data or status select input  |
| 6-13  | DB<sub>0</sub>-DB<sub>7</sub> | Bidirectional data bus       |
| 14    | DRQ                           | DMA request output           |
| 15    | /DACK                         | DMA acknowledge input        |
| 16    | TC                            | Terminal count input         |
| 17    | INDEX                         | Index output                 |
| 18    | INT                           | Interrupt request output     |
| 19    | CLK                           | Clock input                  |
| 20    | GND                           | Ground                       |
| 21    | WCLK                          | Write clock input            |
| 22    | WINDOW                        | Read data window input       |
| 23    | RDATA                         | Read data input              |
| 24    | SYNC                          | VCO Sync output              |
| 25    | WE                            | Write enable output          |
| 26    | MFM                           | MFM output                   |
| 27    | SIDE                          | Head select output           |
| 28,29 | US<sub>1</sub>,US<sub>0</sub> | FDD unit select output       |
| 30    | WDATA                         | Write data output            |
| 31,32 | PS<sub>1</sub>,PS<sub>0</sub> | Preshift output              |
| 33    | FLT/TRK0                      | Fault/Track zero input       |
| 34    | WRT/2SIDE                     | Write protect/two side input |
| 35    | READY                         | Ready input                  |
| 36    | HDLD                          | Head load output             |
| 37    | FLTR/STEP                     | Fault reset/step output      |
| 38    | LCT/DIR                       | Low current direction output |
| 39    | /RW / SEEK                    | Read/write/seek output       |
| 40    | Vcc                           | DC power (+5V)               |

## Pin Functions

### RESET (Reset)

The RESET input places the FDC in the idle state. It resets the output lines to the FDD to 0 (low), except PS0, 1 and WDATA (undefined), INT and DRQ also go low; DB0-7 goes to an input state. It does not affect SRT, HUT or HLT in the Specify command. If the RDY input is held high during reset, the FDC will generate an interrupt with 1.024ms. To clear this interrupt, use the Sense Interrupt Status Command.

### /RD (Read Strobe)

The RD input allows the transfer of data from the FDC to the data bus when low and either /CS or /DACK is asserted.

### /WR (Write Strobe)

The /WR input allows the transfer of data to the FDC from the data bus when low. Disabled when /CS is high.

### A<sub>0</sub> (Data/Status Select)

The A<sub>0</sub> input selects the data register (A<sub>0</sub>=1) or status register (A<sub>0</sub>=0) contents to be accessed through the data bus.

### /CS (Chip Select)

The FDC is selected when /CS is low, enabling /RD and /WR.

### DB<sub>0</sub>-DB<sub>7</sub> (Data Bus)

DB<sub>0</sub>-DB<sub>7</sub> are a bidirectional 8-bit data bus. Disabled when /CS is high.

### DRQ (DMA Request)

The FDC asserts the DRQ output high to request a DMA transfer

### /DACK (DMA Acknowledge)

When the /DACK input is low, a DMA cycle is active and the controller is performing a DMA transfer.

### TC (Terminal Count)

When the TC input is high, it indicates the termination of a DMA transfer. It terminates data transfer during Read/Write/Scan commands in DMA or interrupt code.

### INDEX (Index)

The INDEX Input goes high at the beginning of a disk track.

### INT (Interrupt)

The INT output is FDC's interrupt request. In Non-DMA mode, the signal is output for each byte. In DMA mode, it is output at the termination of a command operation.

### CLK (Clock)

CLK is the input for the FDC's single-phase, TTL-level squarewave clock; 8 Mhz or 4 Mhz (Requires a pull-up resistor).

### WCLK (Write Clock)

The WCLK input sets the data write rate to the FDD.It is 500Khz for FM, 1Mhz for MFM drives,for 8Mhz operation of the FDC; 250KhzFM or 500Khz MFM for 4Mhz FDC operation.\
\
This signal must be input for read and write cycles. WCLK's rising edge must be synchronised with CLK's rising edge, except for the µPD765B.

### WINDOW (Read Data Window)

The WINDOW input is generated by the phase-locked loop (PLL). It is used to sample data from the FDD and in distinguishing between clock and data bits in the FDC.

### RDATA (Read Data)

The RDATA input is the read data from the FDD, containing clock and data bits. To avoid a deadlock situation, input RDATA and WINDOW together.

### WDATA (Write Data)

WDATA is the serial clock and data output to the FDD.

### WE (Write Enable)

The WE output enables write data into the FDD.

### SYNC (VCO Sync)

The SYNC output inhibits the VCO in the PLL when low, enables it when high.

### MFM (MFM Mode)

The MFM output shows the VCO's operation mode. It is high for MFM, low for MF.

### SIDE (Head Select)

Head 1 is selected when the SIDE ouput is 1 (high), head 0 is selected when SIDE is 0 (low).

### US<sub>0</sub>,US<sub>1</sub> (Unit Select 0,1)

The US<sub>0</sub> and US<sub>1</sub> outputs select up to 4 floppy disk drive units an external decoder.

### PS<sub>0</sub>,PS<sub>1</sub> (Preshift 0,1)

The PS<sub>1</sub> and PS<sub>0</sub> outputs are the write precompensation request signals for MFM mode. They determine early, late, and normal times for WDATA shifting.\
\

| **PS0** | **PS1** | **Shift (MFM WDATA)** |
|---------|---------|-----------------------|
| 0       | 0       | Normal                |
| 0       | 1       | Late                  |
| 1       | 0       | Early                 |
| 1       | 1       | \-                    |

### READY (Ready)

The READY input indicates that the FDD is ready to receive data.

### HDLD (Head Load)

The HDLD output is the command which causes the read/write head in the FDD to contact the diskette.

### FLT/TRK0 (Fault/Track 0)

In the read/write mode, the FLT input detects FDD fault conditions. In the seek mode, TRK0 indicates track 0 head position.

### WPRT/2SIDE (Write Protect/Two Side)

In the read/write mode, the WPRT input senses write protected status (at the drive or media). In the seek mode, 2SIDE senses two-side media.

### FLTR/STEP (Fault Reset/Step)

In the read/write mode, the FLTR output resets the fault flip-flop in the FDD. In the seek mode, STEP outputs step pulses to move the head to another cylinder. A fault reset pulse is issued at the beginning or each Read or each Write command prior to the HDLD signal.

### LCT/DIR (Low Current/Direction

In the read/write mode, the LCT output indicates that the R/W head is position at cylinder 42 or greater. In the seek mode, the DIR output determines the direction the head will move in when it receives a step pulse. If DIR is 0, seeks are performed in the outward direction; DIR is 1, seeks are performed in the inward direction.

### /RW / SEEK (Read/Write/Seek)

The /RW / SEEK output specifies the read/write mode when low, and the seek mode when high

### GND (Ground)

Ground.

### Vcc (+5V)

+5v power supply.

## Block Diagram

## Absolute Maximum Ratings

## DC characteristics

## Capacitance

DIFFERENCES BETWEEN µPD765A AND µPD765B

The µPD765B is a functionally enhanced version of the µPD765A. Differences are explained below.

### Overrun bit (OR)

In µPD765A, when executing a read- or write-type command (except READ ID and SCAN types), the result status OR bit is not set if there is an overrun on the final byte of a sector. An improvement in the µPD765B allows it to set the OR bit in any situation.

### DRQ Reset

When an overrun occurs, the µPD765A needs /DACK input to reset DRQ. If /DACK is not available, an external DMA controller continues to operate even after the FDC enters the R-Phase (Result Phase), and stored result status may be transferred accidentally as ordinary data.\
\
On the other hand, the µPD765B resets DRQ automatically just before the R-Phase entry and independant of the /DACK input. See AC Characteristics for DRQ reset timing.

### Clock Synchronisation

The µPD765B does not require synchronisation between the CLK and WCLK inputs.

### Version Command

The version command distinguishes the µPD765B from other devices. The ST0 response to the Version command is:

| Part No. | ST0 value |
|----------|-----------|
| µPD765A  | 80H       |
| µPD765B  | 90H       |

AC Characteristics

# Timing waveforms

# Internal Registers

The µPD765A/µPD765B contains two registers which may be accessed by the main system processor; a status register and a data register. The 8-bit main status register contains the status information of the FDC, and may be accessed at any time. The 8-bit data register (which actually consists of four registers, ST0-ST3, in a stack with only one register presented to the data bus at a time), stores data, commands, parameters, and FDD status information. Data bytes are read out of, or written into the, the data register in order to program or obtain results after a particular command (table 3). Only the status register may be read and used to facilitate the transfer of data between the processor and µPD765A/µPD765B.\
\
The relationship between the status/data registers and the signals /RD,/WR and A<sub>0</sub> are shown in table 1.\
\

**Table 1. Status/Data Register Addressing**

| **A<sub>0</sub>** | **/RD** | **/WR** | **Function**              |
|-------------------|---------|---------|---------------------------|
| 0                 | 0       | 1       | Read Main Status Register |
| 0                 | 1       | 0       | Illegal                   |
| 0                 | 0       | 0       | Illegal                   |
| 1                 | 0       | 0       | Illegal                   |
| 1                 | 0       | 1       | Read from Data Register   |
| 1                 | 1       | 0       | Write into Data Register  |

\
\
The bits in the main status register are defined in table 2.\
\

**Table 2. Main Status Register**

| **No.** | **Name** | **Function** |
|----|----|----|
| DB<sub>0</sub> | D<sub>0</sub>B (FDD 0 Busy) | FDD number 0 is in the Seek mode. If any of the D<sub>n</sub>B bits is set FDC will not accept read or write command. |
| DB<sub>1</sub> | D<sub>1</sub>B (FDD 1 Busy) | FDD number 1 is in the Seek mode. If any of the D<sub>n</sub>B bits is set FDC will not accept read or write command. |
| DB<sub>2</sub> | D<sub>2</sub>B (FDD 2 Busy) | FDD number 2 is in the Seek mode. If any of the D<sub>n</sub>B bits is set FDC will not accept read or write command. |
| DB<sub>3</sub> | D<sub>3</sub>B (FDD 3 Busy) | FDD number 3 is in the Seek mode. If any of the D<sub>n</sub>B bits is set FDC will not accept read or write command. |
| DB<sub>4</sub> | CB (FDC Busy) | A read or write command is in process. FDC will not accept any other commands |
| DB<sub>5</sub> | EXM (Execution Mode) | This bit is set only during execution phase in non-DMA mode. When DB<sub>5</sub> goes low, execution phase has ended and result phase has started. It operates only during non-DMA mode of operation. |
| DB<sub>6</sub> | DIO (Data Input/Output) | Indicates direction of data transfer between FDC and Data Register. If DIO = "1" then transfer is from Data Register to the processor. If DIO = "0", then transfer is from the processor to the data register. |
| DB<sub>7</sub> | RQM (Request for Master) | Indicates Data Register is ready to send or receive data to or from the processor. Both bits DIO and RQM should be used to perform the handshaking functions of "ready" and "direction" to the processor. |

\
\
The DIO and RQM bits in the status register indicate when data is ready and in which direction data will be transferred on the data bus. See figure 1.

**Figure 1. DIO and RQM**

\
\

**TABLE 3. STATUS REGISTER IDENTIFICATION**

\
\

**STATUS REGISTER 0**

| **No.** | **Name** | **Function** |  |
|----|----|----|----|
| D<sub>7</sub>,D<sub>6</sub> | IC (Interrupt Code) |  |  |
| D<sub>7</sub>,D<sub>6</sub> | IC (Interrupt Code) | D<sub>7</sub>=0 and D<sub>6</sub>=0; Normal Termination of Command (NT), Command was completed and properly executed. |  |
| D<sub>7</sub>,D<sub>6</sub> | IC (Interrupt Code) | D<sub>7</sub>=0 and D<sub>6</sub>=1; Abnormal Termination of Command (AT), Execution of Command was started, but was not successfully completed. |  |
| D<sub>7</sub>,D<sub>6</sub> | IC (Interrupt Code) | D<sub>7</sub>=1 and D<sub>6</sub>=0; Invalid Command issued (IC), Command which was issued was never started. |  |
| D<sub>7</sub>,D<sub>6</sub> | IC (Interrupt Code) | D<sub>7</sub>=1 and D<sub>6</sub>=1; Abnormal Termination because during command execution the ready signal from the FDD changed state. |  |
| D<sub>5</sub> | SE (Seek End) | When the FDC completes the SEEK command, this flag is set to 1 (high). |  |
| D<sub>4</sub> | EC (Equipment Check) | If a fault signal is received from the FDD, or if the Track 0 signal fails to occur after 77 Step Pulses (Recalibrate Command) then this flag is set. |  |
| D<sub>3</sub> | NR (Not Ready) | When the FDD is in the not-ready state and a read or write command is issued, this flag is set. If a read or write command is issued to Side 1 of a single sided drive then this flag is set. |  |
| D<sub>2</sub> | HD (Head Address) | This flag is used to indicate the state of the head at interrupt. |  |
| D<sub>1</sub> | US<sub>1</sub> (Unit Select 1) | These flags are used to indicate the drive unit number at interrupt. |  |
| D<sub>0</sub> | US<sub>0</sub> (Unit Select 0) | These flags are used to indicate the drive unit number at interrupt. |  |

\
\
\
\

**STATUS REGISTER 1**

| **No.** | **Name** | **Function** |
|----|----|----|
| D<sub>7</sub> | EN (End of Cylinder) | When the FDC tries to access a sector beyond the final Sector of a Cylinder, this flag is set. |
| D<sub>6</sub> | Not used. This bit is always 0 (low). |  |
| D<sub>5</sub> | DE (Data Error) | When the FDC detects a CRC(1) error in either the ID field or the data field, this flag is set. |
| D<sub>4</sub> | OR (Overrun) | If the FDC is not serviced by the main systems during data transfers within a certain time interval, this flag is set. |
| D<sub>3</sub> |  | Not used. This bit always 0 (low). |
| D<sub>2</sub> | ND (No Data) |  |
| D<sub>2</sub> | ND (No Data) | During execution of READ DATA, Read Deleted Data, Write Data,WRITE DELETED DATA or SCAN Command, if the FDC cannot find the Sector specified in the IDR(2) register, then this flag is set. |
| D<sub>2</sub> | ND (No Data) | During executing the READ ID Command, if the FDC cannot read the ID field without an error, then this flag is set. |
| D<sub>2</sub> | ND (No Data) | During execution of the READ Diagnostic Command, if the starting sector cannot be found then this flag is set. |
| D<sub>1</sub> | NW (Not Writeable) | During execution of WRITE DATA, WRITE DELETED DATA or Write ID Command, if the FDC detects a write protect signal from the FDD, then this bit is set. |
| D<sub>0</sub> | MA (Missing Address Mark) | This bit is set if the FDC does not detect the IDAM before 2 index pulses. It is also set if the FDC cannot find the DAM or DDAM after the IDAM is found, MD bit of ST2 is also set at this time. |

\
\
\
\

**STATUS REGISTER 2**

| **No.** | **Name** | **Function** |
|----|----|----|
| D<sub>7</sub> |  | Not used. This bit is always 0 (low). |
| D<sub>6</sub> | CM (Control Mark) | During execution of the Read Data or Scan Command, if the FDC encounters a Sector which contains a Deleted Data Address Mark, this flag is set. Also set if DAM found during Read Deleted Data |
| D<sub>5</sub> | DD (Data Error in Data Field) | If the FDC detects a CRC error in the data field then this flag is set. |
| D<sub>4</sub> | WC (Wrong Cylinder) | This bit is related with the ND bit, and when the contents of C(3) on the medium is different from that stored in the IDR, this flag is set. |
| D<sub>3</sub> | SH (Scan Equal Hit) | During execution of the Scan Command, if the condition of the "equal" is satisfied, then this flag is set. |
| D<sub>2</sub> | SN (Scan Not Satisfied) | During execution of the Scan Command, if the FDC cannot find a sector on the cylinder which meets the condition, then this flag is set. |
| D<sub>1</sub> | BC (Bad Cylinder) | This bit is related with the ND bit, when the contents of C on the mdeium is different from that stored in the IDR and the contents of C is &FF, then this flag is set. |
| D<sub>0</sub> | MD (Missing Address Mark in Data Field) | When data is read from the medium, if the FDC cannot find a Data Address Mark or Deleted Data Address Mark, then this flag is set. |

\
\
\
\

**STATUS REGISTER 3**

| **No.** | **Name** | **Function** |
|----|----|----|
| D<sub>7</sub> | FT (Fault) | This bit is used tto indicate the status of the Fault signal from the FDD. |
| D<sub>6</sub> | WP (Write Protected) | This bit is used to indicate the status of the Write Protected signal from the FDD. |
| D<sub>5</sub> | RY (Ready) | This bit is used to indicate the status of the Ready signal from the FDD. |
| D<sub>4</sub> | T0 (Track 0) | This bit is used to indicate the status of the Track 0 signal from the FDD. |
| D<sub>3</sub> | TS (Two Side) | This bit is used to indicate the status of the Two Side signal from the FDD. |
| D<sub>2</sub> | HD (Head Address) | This bit is used to indicate the status of the Side Select signal to the FDD. |
| D<sub>1</sub> | US<sub>1</sub> (Unit Select 1) | This bit is used to indicate the status of the Unit Select 1 signal to the FDD. |
| D<sub>0</sub> | US<sub>0</sub> (Unit Select 0) | This bit is used to indicate the status of the Unit Select 0 signal to the FDD. |

### Note

- CRC = Cyclic Redundancy Check
- IDR = Internal Data Register
- Cylinder (C) is described more fully in the Command Symbol Description

## Command Sequence

The µPD765A/µPD765B is capable of performing 15 different commands. Each command is initiated by a multibyte transfer from the processor, and the result after execution of the command may also be a multibyte transfer back to the processor. Because of this multibyte interchange of information between the µPD765A/µPD765B and the processor, it is convienient to consider each command as consisting of three phases:

|  |  |
|----|----|
| Command Phase | The FDC receives all information required to perform a particular operation from the processor. |
| Execution Phase | The FDC performs the operation it was instructed to do. |
| Result Phase | After completion of the operation, status and other housekeeping information are made available to the processor. |

\
\
Table 4 shows the required preset parameters and results for each command. Most commands require 9 command bytes and return 7 bytes during the result phase. The "W" to the left of each byte indicates a command phase byte to be written, and an "R" indicates a result byte. The definitions of other abbreviations used in table are given in the Command Symbol Description table.\
\

**Command Symbol Description**

| **Name** | **Function** |
|----|----|
| A<sub>0</sub> (Address Line 0) | A<sub>0</sub> controls selection of Main Status Register (A<sub>0</sub>=0) or Data Register (A<sub>0</sub>=1) |
| C (Cylinder Number) | C stands for the current/selected Cylinder (track) numbers 0 through 76 of the medium |
| D (Data) | D stands for the data pattern which is going to be written into a Sector during a WRITE ID operation |
| D<sub>7</sub>--D<sub>0</sub> (Data bus) | 8-bit Data Bus, where D<sub>7</sub> stands for the most significant bit, and D<sub>0</sub> stands for a least significant bit. |
| DTL (Data Length) | When N is defined as 00, DTL stands for the data length which users are going to read out or write into the sector. |
| EOT (End Of Track) | EOT stands for the final Sector number on a Cylinder. During read or write operations, FDC will stop data transfer after a sector number equal to EOT. |
| GPL (Gap Length) | GPL stands for the length of Gap 3. During Read/Write operations this value determines the number of bytes that VCO sync will stay low after two CRC bytes. During Format command it determines the size of gap 3. |
| H (Head Address) | H stands for logical head number 0 or 1, as specified in the ID field |
| HD (Head) | HD stands for a the physical head number 0 or 1 and controls the polarity of pin 27. (H=HD in all command words.) |
| HLT (Head Load Time) | HLT stands for the head load time in the FDD (2 to 254ms in 2ms increments) |
| HUT (Head Unload Time) | HUT stands for the head unload time after a read or write operation has occured (16 to 240ms in 16ms increments) |
| MF (FM or MFM Mode) | If MF is low, FM mode is selected and if it is high, MFM mode is selected. |
| MT (Multi-track) | If MT is high, a multi-track operation is to be performed. If MT=1 after finishing read/write operation on side 0, FDC will automatically start searching for sector 1 on side 1. |
| N (Number) | N stands for the number of data bytes written in a sector |
| NCN (New Cylinder Number) | NCN stands for a new Cylinder Number which is going to be reached as a result of the Seek operation; Desired Position of head. |
| ND (Non-DMA Mode) | ND stands for operation in the Non-DMA mode |
| PCN (Present Cylinder Number) | PCN stands for the Cylinder number at the completion of Sense Interrupt Status Command. Position of head at present time. |
| R (Record) | R stands for the Sector Number which will be read or written. |
| R/W (Read/Write) | R/W stands for either Read (R) or Write (W) signal. |
| SC (Sector) | SC indicates the number of sectors per track |
| SK (Skip) | SK stands for Skip Deleted Data Address Mark |
| SRT (Step Rate Time) | SRT stands for the Stepping Rate for the FDD (1 to 16ms in 1ms increments). The same Stepping Rate applies to all drives (F=1ms, E=2ms, etc). |
| ST0-ST3 (Status 0-3) | ST0-ST3 stand for one of four registers which store the status information after a command has been executed. This information is available during the result phase after command execution. These registers should not be confused with the Main Status Register (selected by A<sub>0</sub>=0). ST0-ST3 may be read only after a command has been executed and contains information relevant to that particular command. |
| STP | During a scan operation, if STP=1 the data in contiguous sectors is compared byte-by-byte with data sent from the processor (or DMA) and if STP=2 then alternate sectors are read and compared. |
| US<sub>0</sub>,US<sub>1</sub>(Unit Select) | DS stands for a select drive number 0 or -3 |

**Table 4. Instruction Set (Notes 1,2)**

\
\

| **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** | **READ DATA** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MF | SK | 0 | 0 | 1 | 1 | 0 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | (Note 3) |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk |  |
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
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MF | SK | 0 | 1 | 1 | 0 | 0 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
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
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MF | 0 | 0 | 0 | 1 | 0 | 1 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
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
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MF | 0 | 0 | 1 | 0 | 0 | 1 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution. The 4 bytes are compared against header on floppy disk. |  |
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

| **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** | **READ DIAGNOSTIC** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | MF | SK | 0 | 0 | 0 | 1 | 0 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
| Command | W | C | C | C | C | C | C | C | C | Sector ID Information prior to command execution |  |
| Command | W | H | H | H | H | H | H | H | H | Sector ID Information prior to command execution |  |
| Command | W | R | R | R | R | R | R | R | R | Sector ID Information prior to command execution |  |
| Command | W | N | N | N | N | N | N | N | N | Sector ID Information prior to command execution |  |
| Command | W | EOT | EOT | EOT | EOT | EOT | EOT | EOT | EOT |  |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL |  |  |
| Command | W | DTL | DTL | DTL | DTL | DTL | DTL | DTL | DTL |  |  |
| Execution |  |  |  |  |  |  |  |  |  | Data transfer between the FDD and main system. FDC reads all data fields from index hole to EOT. |  |
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
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | MF | 0 | 0 | 1 | 0 | 1 | 0 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
| Execution |  |  |  |  |  |  |  |  |  | The first correct ID information on the cylinder is stored in data register |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | Sector ID information during Execution Phase |  |
| Result | R | H | H | H | H | H | H | H | H | Sector ID information during Execution Phase |  |
| Result | R | R | R | R | R | R | R | R | R | Sector ID information during Execution Phase |  |
| Result | R | N | N | N | N | N | N | N | N | Sector ID information during Execution Phase |  |

\
\

| **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** | **WRITE ID (Format Write)** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | MF | 0 | 0 | 1 | 1 | 0 | 1 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
| Command | W | N | N | N | N | N | N | N | N | Bytes/sector |  |
| Command | W | SC | SC | SC | SC | SC | SC | SC | SC | Sectors/track |  |
| Command | W | GPL | GPL | GPL | GPL | GPL | GPL | GPL | GPL | Gap 3 |  |
| Command | W | D | D | D | D | D | D | D | D | Filler Byte |  |
| Execution |  |  |  |  |  |  |  |  |  | FDC formats an entire track |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information after command execution |  |
| Result | R | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | ST1 | Status information after command execution |  |
| Result | R | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | ST2 | Status information after command execution |  |
| Result | R | C | C | C | C | C | C | C | C | In this case the ID information has no meaning |  |
| Result | R | H | H | H | H | H | H | H | H | In this case the ID information has no meaning |  |
| Result | R | R | R | R | R | R | R | R | R | In this case the ID information has no meaning |  |
| Result | R | N | N | N | N | N | N | N | N | In this case the ID information has no meaning |  |

\
\

| **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** | **SCAN EQUAL** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MF | SK | 1 | 0 | 0 | 0 | 1 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
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
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MF | SK | 1 | 1 | 0 | 0 | 1 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
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
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | MT | MF | SK | 1 | 1 | 1 | 0 | 1 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
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
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | X | X | X | X | X | 1 | 1 | 1 | Command Codes |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | 0 | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
| Execution |  |  |  |  |  |  |  |  |  | Head retracked to track 0 |  |

\
\

| **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** | **SENSE INTERRUPT STATUS** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | Command Codes |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | Status information about the FDC at the end of seek operation |  |
| Result | R | PCN | PCN | PCN | PCN | PCN | PCN | PCN | PCN | Status information about the FDC at the end of seek operation |  |

\
\

| **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** | **SPECIFY** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 1 | Command Codes |  |
| Command | W | SRT | SRT | SRT | SRT | HUT | HUT | HUT | HUT | Command Codes |  |
| Command | W | HLT | HLT | HLT | HLT | HLT | HLT | HLT | ND | Command Codes |  |

\
\

| **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** | **SENSE DRIVE STATUS** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
| Result | R | ST3 | ST3 | ST3 | ST3 | ST3 | ST3 | ST3 | ST3 | Status information about FDD |  |

\
\
\
\

| **VERSION** | **VERSION** | **VERSION** | **VERSION** | **VERSION** | **VERSION** | **VERSION** | **VERSION** | **VERSION** | **VERSION** | **VERSION** | **VERSION** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | X | X | X | 1 | 0 | 0 | 0 | 0 | Command Codes |  |
| Command | Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | 90H indicates 765B 80H indicates 765A/A-2 |

\
\

| **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** | **SEEK** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 1 | Command Codes |  |
| Command | W | X | X | X | X | X | HD | US<sub>1</sub> | US<sub>0</sub> | Command Codes |  |
| Command | W | NCN | NCN | NCN | NCN | NCN | NCN | NCN | NCN | Command Codes |  |
| Execution |  |  |  |  |  |  |  |  |  | Head is positioned over proper cylinder on diskette |  |

\
\

| **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** | **INVALID** |
|----|----|----|----|----|----|----|----|----|----|----|----|
| **Phase** | **R/W** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Instruction Code** | **Remarks** |  |
| **Phase** | **R/W** | **D<sub>7</sub>** | **D<sub>6</sub>** | **D<sub>5</sub>** | **D<sub>4</sub>** | **D<sub>3</sub>** | **D<sub>2</sub>** | **D<sub>1</sub>** | **D<sub>0</sub>** | **Remarks** |  |
| Command | W | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid Codes | Invalid command codes (NoOp - FDC goes into standby state) |  |
| Result | R | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0 | ST0=80H |  |

\
\
Note:

- Symbols used in this table are described at the end of this section.
- A<sub>0</sub> should equal 1 for all operations.
- X = Don't care, usually made to equal 0.

## System Configuration

Figure 2 shows an example of a system using a µPD765A/B.

**Figure 2 System Configuration**

## Processor Interface

During command or result phases the main status register (described earlier) must be read by the processor before each byte of information is written into or read from the data register. After each byte of data read or written to the data register, the CPU should wait for 12µs before reading main status register, bits D6 and D7 in the main status register must be in a 0 and 1 state, respectively, before each byte of the command word may be written into the µPD765A/µPD765. Many of the commands require multiple bytes and, as a result the main status register must be read prior to each byte transfer to the µPD765A/µPD765. On the other hand, during the result phase, D6 and D7 in the main status register must both be 1's. (D6=1 and D7=1) before reading each byte from the data register. Note that this reading of the main status register before each byte transfer to the µPD765A/µPD765 is required only in the command and result phases, and *not* during the execution phase.\
\
During the Execution Phase, the Main Status Register need not be read. If the µPD765A/µPD765 is in the Non-DMA mode, then the receipt of each data byte (if µPD765A/µPD765 is reading data from FDD) is indicated by an interrupt signal on pin 18 (INT=1). The generation of a Read signal (/RD=0) will reset the interrupt as well as output the data onto the data bus. For example, if the processor cannot handle interrupts fast enough (every 13µs for MFM mode and 27µus for the FM mode) then it may poll the Main Status Register and bit D7 (RQM) functions as the interrupt signal. If a Write Command is in process then the /WR signal performs the reset to the interrupt signal.\
\
Note that in the non-DMA mode it is necessaey to examine the main status register to determine the cause of the interrupt ,since it could be a data interrupt or a command termination interupt, either normal or abnormal.\
\
If the µPD765A/µPD765 is in the DMA mode, no interrupts are generated during the execution phase. The µPD765A/µPD765 generates DRQ's (DMA Requests) when each byte of data is available. The DMA controller responds to this request with both a /DACK=0 (DMA Acknowledge) and a /RD=0 (Read Signal). When the DMA Acknowledge signal goes low (/DACK=0) then the DMA Request is cleared (DRQ=0). If a write command has been programmed then a /WR signal will appear instead of /RD. After the execution phase has been completed (Terminal Count has occured) or the EOT sector read/written, then an interrupt will occur (INT=1). This signifies the beginning of the Result Phase. When the first byte of data is read during the Result Phase, the Interrupt is automatically cleared (INT=0).\
\
The /RD or /WR signals should be asserted while /DACK is true. The /CS signal is used in conjunction with /RD and /WR as a gating function during programmed I/O operations. /CS has no effect during DMA operations. If the non-DMA mode is chosen, the /DACK signal should be pulled up to Vcc.\
\
It is important to note that during the Result Phase all bytes shown in the command table (table 4) must be read. The Read Data Command, for example, has seven bytes of data in the Result Phase. All seven bytes must be read in order to sucessfully complete the Read Data Command. The µPD765A/µPD765 will not accept a new command until all seven bytes have been read. Other commands may require fewer bytes to be read during the Result Phase.\
\
The µPD765A/µPD765 contains five Status Registers. The Main Status Register mentioned above may be read by the processor at any time. The other four status registers (ST0,ST1,ST2 and ST3) are only available only during the Result Phase, and may be read only after sucessfully completing a command. The particular command that has been executed determines how many of the Status Registers will be read.\
\
The bytes of data which are sent to the µPD765A/µPD765 to form the Command Phase and are read out of the µPD765A/µPD765 in the Result Phase, must occur in the order shown in table 4. That is, the Command Code must be sent first and the other bytes sent in the prescribed sequence. No foreshortening of the Command or Result Phases are allowed. After the last byte of data in the Command Phase is sent to the µPD765A/µPD765 the Execution Phase automatically starts. In a similar fashion, when the last byte of data is read out in the Result Phase, the command is automatically ended and the µPD765A/µPD765 is ready for a new command.

## Polling

After reset has been sent to the µPD765A/µPD765, the unit select Lines US0 and US1 will automatically go into a polling mode. In between commands (and between step pulses in the seek command) the µPD765A/µPD765 polls all four FDDs looking for a change in the Ready Line from any of the drives. If the Ready line changes state (usually due to a door opening or closing) then the µPD765A/µPD765 will generate an interrupt. When Status Register 0 (ST0) is read (after Sense Interrupt Status is issued), Not Ready (NR) will be indicated. The polling of the Ready line by the µPD765A/µPD765 continues continously between commands thus notifying tthe processor which drives are on or off line. Each drive is polled every 1.024ms except during the Read/Write commands. When used with a 4MHz clock for interfacing to minifloppies, the polling rate is 2.048ms. See figure 3.\
\
**Figure 3. Polling Feature**

### Read Data

A set of nine (9) byte words are required to place the FDC into the Read Data Mode. After the Read Data command has been issued the FDC loads the head (if it is in the unloaded state), waits the specified head settling time (defined in the Specify Command), and begins reading ID Address Marks and ID fields. When the current sector number (R) stored in the ID Register (IDR) compares with the sector number read off the diskette, then the FDC outputs data (from the data field) byte-by-byte to the main system via the data bus.\
\
After completion of the read operation from the current sector, the Sector Number is incremented by one, and the data from the next sector is read and output on the data bus. This continuous read function is called a Multi-Sector Read Operation. The Read Data Command may be terminated by the receipt of a Terminal Count signal. TC should be issued at the same time that the /DACK for the last byte of data is sent. Upon receipt of this signal, the FDC stops outputting data to the processor, but will continue to read data from the current sector, check CRC (Cyclic Redundancy Count) bytes, and then at the end of the sector terminate the Read Data Command. The amount of data which can be handled with a single command to the FDC depends upon MT (multi-track), MF (MFM/FM), and N (number of bytes/sector). Table 5 shows the transfer capacity.\
\
The "multi-track" function (MT) allows the FDC to read data from both sides of the diskette. For a particular cylinder, data will be transferred starting at Sector 1, Side 0 and completing at Sector L, side 1 (Sector L = last sector on the side). Note, this function pertains to only one cylinder (the same track) on each side of the diskette.\
\
When N=0, then DTL defines the data length which the FDC must treat as a sector. If DTL is smaller than the actual data length in a sector, the data beyond DTL in the sector is not sent to the Data Bus. The FDC reads (internally) the complete sector performing the CRC check and, depending upon the manner of command termination, may perform a Multi-Sector Read Operation. When N is non-zero, then DTL has no meaning and should be set to ffh.\
\

**Table 5. Transfer Capacity**

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
At the completion of the Read Data Command, the head is not unloaded until after Head Unload Time Interval (specified in the Specify Command) has elapsed. If the processor issues another command before the head unloads then the head settling time may be saved between subsequent reads. This time out is particularly valuable when a diskette is copied from one drive to another.\
\
If the FDC detects the Index Hole twice without finding the right sector, (indicated in "R"), then the FDC sets the ND (No Data) flag in Status Register 1 to a 1 (high), and terminates the Read Data Command. (Status Register 0 also has bits 7 and 6 set to 0 and 1 respectively).\
\
After reading the ID and Data Fields in each sector, the FDC checks the CRC bytes. If an error is detected, (incorrect CRC in ID field), the FDC sets the DE (Data Error) flag in Status register 1 to a 1 (high), and if a CRC error occurs in the Data Field the FDC also sets the DD (Data Error in Data Field) flag in Status Register 2 to a 1 (high), and terminates the Read Data Command. (Status Register 0 also has bits 7 and 6 set to 0 and 1 respectively.)\
\
If the FDC reads a Deleted Data Address Mark off the diskette, and SK bit (bit D5 in the first Command Word) is not set (SK=0), then the FDC sets the CM (Control Mark) flag in Status Register 2 to a 1 (high), and terminates the Read Data Command, after reading all the data in the Sector. If SK=1, the FDC skips the sector with the Deleted Data Address Mark and reads the next sector. The CRC bits in the deleted data field are not checked when SK=1.\
\
During disk data transfers between the FDC and the processor, via the data bus, the FDC must be serviced by the processor every 27µs in the FM mode, and every 13µs in the MFM mode, or the FDC sets the OR (Over Run) flag in Status Register 1 to a 1 (high), and terminates the Read Data Command.\
\
If the processor terminates a read (or write) operation in the FDC, then the ID information in the Result Phase is dependant upon the state of the MT bit and the EOT byte. Table 2 shows the values for C,H,R and N, when the processor terminates the Command.

### Functional Description of Commands

### Write Data

A set of nine (9) bytes are required to set the FDC into the Write Data Mode. After the Write Data command has been issued the FDC loads the head (if it is in the unloaded state), waits the specified head settling time (defined in the Specify Command), and begins reading ID Fields. When all four bytes loaded during the command (C,H,R,N) match the four bytes of the ID field from the diskette, the FDC takes data from the processor byte-by-byte via the data bus and outputs it to the FDD. See table 6.\
\

**Table 6: Command Description**

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

\
\
After writing data into the current sector, the Sector Number stored in R is incremented by one, and the next data field is written into. The FDC continues this Multi-Sector Write Operation until the issuance of a Terminal Count signal. If a Terminal Count Signal is sent to the FDC it continues writing into the current sector to complete the data field. If the Terminal Count signal is received while a data field is being written then the remainder of the data field is filled with zeros.\
\
The FDC reads the ID field of each sector and checks the CRC bytes. If the FDC detects a read error (CRC error) in one of the ID fields, it sets the DE (Data Error) flag of Status Register 1 to a 1 (high), and terminates the Write Data Command. (Status Register 0 also has bits 7 and 6 set to 0 and 1 respectively).\
\
The Write Command operates in much the same manner as the Read Command. The following items are the same, and one should refer to the Read Data Command for details:

- Transfer Capacity
- EN (End of cylinder flag)
- ND (No data) flag
- Head Unload Time Interval
- ID Information when processor terminates command (see table 2)
- Definition of DTL when N=0 and when N\<\>0

In the Write Data mode, data transfers between the processor and FDC, via the data bus, must occur every 27µs in the FM mode and every 13µs in the MFM mode. If the time interval between data transfers is longer than this then the FDC sets the OR (Over Run) flag in Status Register 1 to a 1 (high), and terminates the Write Data Command. (Status register 0 also has bits 7 and 6 set to 0 and 1 respectively).

### Write Deleted Data

This command is the same as the Write Data Command except a Deleted Data Address Mark is written at the beginning of the Data Field instead of the normal Data Address Mark.

### Read Deleted Data

This command is the same as the Read Data Command except that when the FDC detects a Data Address Mark at the beginning of a Data Field (and SK=0 (low)), It will read all the data in the sector and set the CM flag in Status Register 2 to a 1 (high), and then terminate the command. If SK=1, then the FDC skips the sector with the Data Address Mark and reads the next sector.

### Read a Track

This command is similar to Read Data command except that this is a continuous read operation where the entire data field from each of the sectors is read. Immediatly after encountering the index hole, the FDC starts reading all data fields on the track as continuous blocks of data. If the FDC finds an error in the ID or DATA CRC check bytes, it continues to read data from the track. The FDC compares the ID information read from each sector with the value stored in the IDR, and sets the ND flag of Status Register 1 to a 1 (high) if there is no comparison. Multi-track or skip operations are not allowed with this command.\
\
This command terminates when the number of sectors read is equal to EOT. If the FDC does not find an ID address mark on the diskette after it senses the index hole for the second time, then it sets the MA (missing address mark) flag in Status Register 1 to a 1 (high), and terminates the command. (Status Register 0 has bits 7 and 6 set to 0 and 1 respectively).

### Read ID

The Read ID command is used to give the present position of the recording head. The FDC stores the values from the first ID Field it is able to read. If no proper ID Address Mark is found on the diskette, before the index hole is encountered for the second time then the MA (Missing Address Mark) flag in Status Register 1 is set to a 1 (high), and if no data is found then the ND (No Data) flag is set in Status Register 1 to a 1 (high). The command is then terminated with bits 7 and 6 in status register 0 set to 0 and 1, respectively. During this command there is no data transfer between FDC and CPU except during the result phase.

### Format a Track

The Format a Track Command allows an entire track to be formatted. After the index hole is detected, Data is written on the Diskette: Gaps, Address Marks, ID Fields and Data Fields, all per the IBM System 34 (Double Density) or System 3740 (Single Density) format, are recorded. The particular format which will be written is controlled by the values programmed into N (Number of bytes/sector), SC (Sectors/cylinder), GPL (Gap Length) and D (Data Pattern) which are supplied by the processor during the Command Phase. The Data Field is filled with the byte of data stored in D. The ID Field for each sector is supplied by the processor; that is, four data requests per sector are made by the FDC for C (Cylinder Number), H (Head Number), R (Sector Number) and N (Number of bytes/sector). This allows the diskette to be formatted with non-sequential sector numbers, if desired.\
\
The processor must send new values for C,H,R and N to the µPD765A/µPD765 for each sector on the track. If the FDC is set for the DMA mode, it will issue four DMA requests per sector. If it is set for the interrupt mode, it will issue four interrupts per sector and the processor must supply C,H,R and N loads for each sector. The contents of the R register is incremented by one after each sector is formatted, thus, the R register contains a value of R when it is read during the Result Phase. This incrementing and formatting continues for the whole track until the FDC encounters the index hole for the second time, whereupon it terminates the command.\
\
If a fault signal is received from the FDD at the end of a write operation, then the FDC sets the EC flag of Status Register 0 to a 1 (high), and terminates the command after setting bits 7 and 6 of Status Register 0 to a 0 and 1 respectively. Also the loss of the ready signal at the beginning of a command execution phase causes bits 7 and 6 of status register 0 to be set to 0 and 1, respectively..\
\
Table 7 shows the relationship between N, SC and GPL for various sector sizes:\
\
**TO BE DONE**\
\

**Table 7: Sector Size**

| **FORMAT** | **SECTOR SIZE** | **N** | **SC** | **GPL(1)** | **GPL(2,3)** | **REMARKS** |
|----|----|----|----|----|----|----|
| 8" Standard Floppy | 8" Standard Floppy | 8" Standard Floppy | 8" Standard Floppy | 8" Standard Floppy | 8" Standard Floppy | 8" Standard Floppy |
| FM Mode | 128 bytes/sector | 0 | 1Ah | 07h | 1Bh | IBM Diskette 1 |
| FM Mode | 256 | 1 | 0Fh | 0Eh | 2Ah | IBM Diskette 2 |
| FM Mode | 512 | 2 | 08 | 1Bh | 3Ah | IBM Diskette 2 |
| FM Mode | 1024 | 3 | 4 | \- | \- |  |
| FM Mode | 2048 | 4 | 2 | \- | \- |  |
| FM Mode | 4096 | 5 | 1 | \- | \- |  |
| MFM Mode | 256 | 1 | 1Ah | 0Eh | 36h | IBM Diskette 2D |
| MFM Mode | 512 | 2 | 0Fh | 1Bh | 54h | IBM Diskette 2D |
| MFM Mode | 1024 | 3 | 08 | 35h | 74h | IBM Diskette 2D |
| MFM Mode | 2048 | 4 | 4 | \- | \- | IBM Diskette 2D |
| MFM Mode | 4096 | 5 | 2 | \- | \- | IBM Diskette 2D |
| MFM Mode | 8192 | 6 | 1 | \- | \- | IBM Diskette 2D |

¹Suggested values of GPL in Read or Write Commands to avoid splice point between data field and ID field of contiguous sections.\
²Suggested values of GPL in format command.

### Scan Commands

The Scan commands allow data which is being read from the diskette to be compared against data which is being supplied from the main system. The FDC compares the data on a byte-by-byte basis, and looks for a sector of data which meets the conditions of D<sub>Fdd</sub>=D<sub>Processor</sub>, D<sub>Fdd</sub>\<=D<sub>Processor</sub> or D<sub>Fdd</sub>\>=D<sub>Processor</sub>. The hexidecimal byte of FF either from memory or from FDD can be used as a mask byte because it always meets the condition of the comparison. One's complement arithmetic is used for comparison (FF = largest number, 00 = smallest number). After a whole sector of data is compared, if the conditions are not met, the sector number is incremented (R+STP -\> R), and the scan operation is continued. The scan operation continues until one of the following conditions occur; the conditions for scan are met (equal, low or high), the last sector on the track is reached (EOT), or the terminal count signal is received.\
\
If the conditions for scan are met, then the FDC sets the SH (Scan Hit) flag of Status Register 2 to a 1 (high), and terminates the Scan Command. If the conditions for scan are not met between the the starting sector (as specified by R) and the last sector on the cylinder (EOT), then the FDC sets the SN (Scan Not Satisfied) flag of Status Register 2 to a 1 (high), and terminates the Scan Command. The receipt of a terminal count signal from the processor or DMA Controller during the scan operation will cause the FDC to complete the comparison of the particular byte which is in process, and then to terminate the command. Table 8 shows the status of bits SH and SN under various conditions of Scan.\
\

**Table 8 6. Scan Conditions**

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
When either the STP (contiguous sectors=01, or alternate sectors=02 sectors are read) or the MT (Multi-Track) are programmed, it is necessary to remember that the last sector on the track must be read. For example, if STP=02, MT=0, the sectors are numbered sequentially 1 through 26, and the scan command is started at sector 21; the following will happen. Sectors 21,23 and 25 will be read, then the next sector (26) will be skipped and the index hole will be encountered before the EOT value of 26 can be read. This will result in an abnormal termination of the command. If the EOT had been set at 25, or the scanning started at sector 20, then the Scan Command would be completed in a normal manner.\
\
During the Scan Command, data is supplied by either the processor or DMA Controller for comparison against the data read from the diskette. In order to avoid having the OR (Over Run) flag set in Status Register 1, it is necessary to have the data available in less than 27µs (FM mode) or 13µs (MFM mode). If an Overrun occurs the FDC ends the command with bits 7 and 6 of status register 0 set to 0 and 1 respectively.

### Seek

The read/write head within the FDD is moved from cylinder to cylinder under control of the Seek Command. FDC has four independant present cylinder registers for each drive. They are cleared only after the Recalibrate command. The FDC compares the PCN (Present Cylinder Number) which is the current head position with the NCN (New Cylinder Number), and if there is a difference, performs the following operations:\
\
PCN\<NCN: Direction signal to FDD set to a 1 (high) and Step Pulses are issued (Step In).\
\
PCN\>NCN: Direction signal to FDD set to a 0 (low), and Step Pulses are issued (Step Out).\
\
The rate at which Step Pulses are issued is controlled by SRT (Stepping Rate Time) in the Specify command. After each Step Pulse is ussued NCN is compared against PCN, and when NCN=PCN, then the SE (Seek End) flag is set in Status Register 0 to a 1 (high), and the command is terminated. At this point FDC interrupt goes high. Bits D0B-D3B in the main status register are set during the seek operation and are cleared by the Sense Interrupt Status command.\
\
During the Command Phase of the Seek operation the FDC is in the FDC busy state, but during the Execution Phase it is in the non-busy state. While the FDC is in the non-busy state, another Seek Command may be issued, and in this manner parallel seek operations may be done on up to four Drives at once. No other command can be issued for as long as the FDC is in the process of sending step pulses to any drive.\
\
If an FDD is in a not ready state at the beginning of the command execution phase or during the seek operation, then the NR (not ready) flag is set in Status Register 0 to a 1 (high), and the command is terminated after bits 7 and 6 of status register 0 are set to 0 and 1, respectively.\
\
If the time to write three bytes of Seek command exceeds 150µs, the timing between the first two step pulses may be shorter than set in the Specify command by as much as 1ms.

### Recalibrate

The function of this command is to retract the read/write head within the FDD to the track 0 position. The FDC clears the contents of the PCN counter, and checks the status of the Track 0 signal from the FDD. As long as the Track 0 signal is low, the Direction signal remains 0 (low) and Step Pulses are issued. When the track 0 signal goes high, he SE (seek end) flag in Status Register 0 is set to a 1 (high) and the command is terminated. If the Track 0 signal is still low after 77 Step Pulses have been issued, the FDC sets the SE (seek end) and EC (equipment check) flags of Status Register 0 to both 1s (highs), and terminates the command after bits 7 and 6 of status register 0 are set to 0 and 1, respectively.\
\
The ability to overlapping recalibrate commands to multiple FDDs, and the loss of the ready signal, as described in the seek Command, also applies to the recalibrate command. If the diskette has more than 77 tracks, then Recalibrate command should be issued twice, in order to position the read/write head to track 0.

### Sense Interrupt Status

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

Interrupts caused by reasons 1 and 4 above occur during normal command operations and are easily decernible by the processor. During an execution phase in DMA mode, DB5 in the main status register is high. Upon entering the result phase this bit gets cleared. Reasons 1 and 4 do not require Sense Interrupt Status Commands. The interrupt is cleared by reading/writing data to the FDC. Interrupts caused by reasons 2 and 3 above may be uniquely identified with the aid of the Sense Interrupt Status Command. This command when issued resets the interrupt signal and, via bits 5,6 and 7 of the Status Register 0 identifies the cause of the interrupt. See table 9.\
\

**Table 9. Interrupt Status**

| **BIT 5** | **BIT 6** | **BIT 7** | **CAUSE** |
|----|----|----|----|
| 0 | 1 | 1 | Ready Line Changed state, either polarity |
| 1 | 0 | 0 | Normal Termination of Seek or Recalibrate Command |
| 1 | 1 | 0 | Abnormal Termination of Seek or Recalibration command |

\
\
The Sense Interrupt Status command is used in conjunction with the Seek and Recalibrate commands which have no result phase. When the disk drive has reached the desired head position the µPD765A/µPD765 will set the interrupt line true. The host CPU must then issue a Sense Interrupt Status command to determine the actual cause of the interrupt, which could be seek end or a change in ready status from one of the drives. A graphic example is given in figure 4.\
\
**Figure 4. Seek, Recalibrate and Sense Interrupt Status**

### Specify

The Specify Command sets the initial values for each of the three internal timers. The HUT (Head Unload Time) defines the time from the end of the Execution Phase of one of the Read/Write Commands to the head unload state. The Timer is programmable from 16 to 240ms in increments of 16 ms (01=16 ms, 02=32 ms.....0F=240ms). The SRT (Step Rate Time) defines the time interval between adjacent step pulses. This timer is programmable from 1 to 16ms in increments of 1 ms (F = 1ms, E = 2ms, D=3ms, etc). The HLT (Head Load Time) defines the time between when the Head Load signal goes high and the Read/Write operation starts. This timer is programmable from 2 to 254 ms in increments of 2ms (01=2ms, 02=4ms, 03=6ms ..... FE=254 ms).\
\
The time intervals mentioned above are a direction function of the clock (CLK on pin 19). Times indicated above are for an 8 Mhz clock, if the clock was reduced to a 4Mhz (mini-floppy application) then all time intervals are increased by a factor of 2.\
\
The choice of DMA or NON-DMA operation is made by the ND (NON-DMA) bit. When this bit is high (ND=1) the NON-DMA mode is selected, and when ND=0 the DMA mode is selected.

### Sense Drive Status

This command may be used by the processor whenever it wishes to obtain the status of the FDDs. Status Register 3 contains the Drive Status information stored internally in FDC registers.

### Invalid

If an invalid command is sent to the FDC (a command not defined above), then the FDC will terminate the command after bits 7 and 6 of status register 0 are set to 1 and 0, respectively. No interrupt is generated by the µPD765A/µPD765 during this condition. Bit 6 and Bit 7 (DIO and RQM) in the Main Status Register are both 1 (high) indicating to the processor that the µPD765A/µPD765 is in the Result Phase and the contents of Status Register (ST0) must be read. When the processor reads Status Register 0 it will find a 80H indicating an invalid command was received.\
\
A Sense Interrupt Status Command must be sent after a Seek or Recalibrate interrupt, otherwise the FDC will consider the next command to be an Invalid Command.\
\
In some applications the user may wish to use this command as a No-Op command, to place the FDC in a standby or no operation state.

## Data Format

Figure 5 shows the data transfer format for the µPD765A and µPD2765 in various modes.

**Figure 5 Data Format (Sheet 1 of 2)**

**Figure 5 Data Format (Sheet 2 of 2)**
