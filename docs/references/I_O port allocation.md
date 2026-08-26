<!--
Converted from "I_O port allocation.html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 2 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# I/O port allocation

Many thanks to **Mark Rison** for providing the original information. Thankyou to Richard Wilson for his discoveries concerning RAM management I/O decoding.

This document will explain the decoding of the I/O ports. The port address is not decoded fully which means a hardware device can be accessed through more than one address, in addition, using some addressess can access more than one element of the hardware at the same time. The CPC IN/OUT design differs from the norm in that port numbers are defined using 16 bits, as opposed to the traditional 8 bits.

- IN r,(C)/OUT (C),r instructions: Bits b15-b8 come from the B register, bits b7-b0 come from "r"
- IN A,(n)/OUT (n),A instructions: Bits b15-b8 come from the A register, bits b7-b0 come from "n"

Listed below are the internal hardware devices and the bit fields to which they respond. In the table:

- "-" means this bit is ignored,
- "0" means the bit must be set to "0" for the hardware device to respond,
- "1" means the bit must be set to "1" for the hardware device to respond.
- "r1" and "r0" mean a bit used to define a register

| Hardware device | Read/Write | b15 | b14 | b13 | b12 | b11 | b10 | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
| Gate-Array | Write Only | 0 | 1 | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- |
| RAM Configuration | Write Only | 0 | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- |
| CRTC | Read/Write | \- | 0 | \- | \- | \- | \- | r1 | r0 | \- | \- | \- | \- | \- | \- | \- | \- |
| ROM select | Write only | \- | \- | 0 | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- |
| Printer port | Write only | \- | \- | \- | 0 | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- |
| 8255 PPI | Read/Write | \- | \- | \- | \- | 0 | \- | r1 | r0 | \- | \- | \- | \- | \- | \- | \- | \- |
| Expansion Peripherals | Read/Write | \- | \- | \- | \- | \- | 0 | \- | \- | \- | \- | \- | \- | \- | \- | \- | \- |

Notes:

- The Gate-Array and CRTC can't be selected simultaneously, which would otherwise cause potential display corruption. This fact has been used in demos which perform "horizontal splitting": e.g. "Gozeur's Madness" and "Dreamend Megademo"
- In the case of the CRTC and PPI, bits b9 and b8 are used to define the register to select.
- It is entirely possible to select multiple devices and access these simultaneously (except for the CRTC and Gate Array)
- The "official" ports are defined to eliminate conflict between devices, as follows ("r" defines a bit which is reserved for register selection on the specific device):
  | Hardware device       | official port address | b15 | b14 | b13 | b12 | b11 | b10 | b9  | b8  |
  |-----------------------|-----------------------|-----|-----|-----|-----|-----|-----|-----|-----|
  | Gate Array            | &7F                   | 0   | 1   | 1   | 1   | 1   | 1   | 1   | 1   |
  | RAM Management        | &7F                   | 0   | 1   | 1   | 1   | 1   | 1   | 1   | 1   |
  | CRTC                  | &BC-&BF               | 1   | 0   | 1   | 1   | 1   | 1   | r   | r   |
  | ROM select            | &DF                   | 1   | 1   | 0   | 1   | 1   | 1   | 1   | 1   |
  | Printer port          | &EF                   | 1   | 1   | 1   | 0   | 1   | 1   | 1   | 1   |
  | PPI                   | &F4-&F7               | 1   | 1   | 1   | 1   | 0   | 1   | r   | r   |
  | Expansion Peripherals | &F8-&FB               | 1   | 1   | 1   | 1   | 1   | 0   | x   | x   |

## CRTC:

| b9  | b8  | Function        | Read/Write state |
|-----|-----|-----------------|------------------|
| 0   | 0   | Register select | Write Only       |
| 0   | 1   | Register write  | Write Only       |
| 1   | 0   | (note 1)        | \-               |
| 1   | 1   | (note 1)        | \-               |

Notes:

1.  These functions are dependant on the model of 6845. Please see the documentation on the 6845 for more information

## PPI:

| b9  | b8  | Function         | Read/Write state |
|-----|-----|------------------|------------------|
| 0   | 0   | Port A           | Read/Write       |
| 0   | 1   | Port B           | Read/Write       |
| 1   | 0   | Port C           | Read/Write       |
| 1   | 1   | Control register | Write only       |

## Expansion Peripherals (including FDC):

If b10 is reset, bits b7-b5 are used to select an expansion peripheral. Again, this is done by resetting the peripheral's bit:

| Bit | device                       |
|-----|------------------------------|
| b7  | FDC                          |
| b6  | Reserved (was it ever used?) |
| b5  | Serial port                  |

In the case of the FDC, bits b8 and b0 are used to select the specific mode of operation; all the other bits (b9,b4-b1) are ignored:

| b8  | b0  | Read                        | Write                           |
|-----|-----|-----------------------------|---------------------------------|
| 0   | 0   | not used                    | Floppy disc drive motor control |
| 0   | 1   | not used                    | Floppy disc drive motor control |
| 1   | 0   | Main status register of FDC | Data register of FDC            |
| 1   | 1   | Data register of FDC        | Data register of FDC            |

If b10 is reset but none of b7-b5 are reset, user expansion peripherals are selected. The exception is the case where none of b7-b0 are reset (i.e. port &FBFF), which causes the expansion peripherals to reset.
