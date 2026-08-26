<!--
Converted from "Additional information about the AY-3-8912.html" on 2026-08-26 using pandoc (HTML → GFM).
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# AY-3-8912 Additional information

The AY-3-8912 is a programmable sound generator made by General Instruments.

## Registers

When reading and writing data to the AY-3-8912 registers, I found that:

- When writing data to register 1,3,5 and 13, and then reading the data back, bits 7..4 will always be "0", regardless of the data written. bits 3..0 will be the same as bits 3..0 of the data.
- When writing data to register 6, 8, 9 and 10, and then reading the data back, bits 7..5 will always be "0", regardless of the data written. bits 4..0 will be the same as bits 4..0 of the data.
- Writing to all the other registers, 0, 2,4,7, 11,12, and then reading the data back will return the data written. e.g. bits 7..0 of the data read is identical to bits 7..0 of the data written.
- The data read from registers 14 and 15 will be different depending on the input/output state of the I/O ports connected to these registers. See below for more details.

Example source code: \[[highlighted](http://www.cpctech.org.uk/source/psgn0.html) \| [original](http://www.cpctech.org.uk/source/psgn0.asm) \]

## I/O ports

I want to thank Russell Marks for his information which I used in this section.

I read a message on the [comp.sys.sinclair](news:comp.sys.sinclair) newsgroup from Russell Marks. He had discovered a problem between running "Matchday 2" on a emulator and a real Spectrum+2 computer.

There are 3 chips in the AY-3-891x Programmable Sound Generator (PSG) family.

- The AY-3-8910 model has 2 I/O ports.
- The AY-3-8912 model has 1 I/O port.
- The AY-3-8913 model has 0 I/O ports.

Port "A" is accessed through register number 14 and Port "B" is accessed through register number 15.

The Amstrad CPC and Spectrum use the AY-3-8912 model.

From furthur investigation he found the program was using AY-3-8912 register 15 (Port B). But this model of the AY-3-891x does not use Port B.

He discovered the following:

- When a port is defined as output (For port A, set bit 6 of register 7 to "1". For port B set bit 7 of register 7 to "1"), writing to the port register (register 14 for Port A, register 15 for port B) will store the data written. A read of the port register will then return the last value written logically ANDed with the input to that port. All 8-bit values can be written.
- Port B (register 15) can be used. The operation is identical to port A.
- When a port is defined as input (For port A, set bit 6 of register 7 to "0". For port B set bit 7 of register 7 to "0"):
- writing to the port register (register 14 for Port A, or register 15 for port B) will store the data written. If the port is then defined as output, a read of the port register will return the last value written ANDed with the input to the port. All 8-bit values can be used.
- reading from the port register (register 14 for Port A, or register 15 for port B) will ONLY return the inputs!
- each port has a "output register" which is internal to the AY-3-8912,
- when data is written to the port's register (register 14 for port A or register 15 for port B), it is stored in the port's output register, regardless of the input/output state of the port.
- when the port is defined as output it is possible to read the port's "output register". The data read is a logical AND of the "output register" and the inputs to the port.
- when the port is defined as output, the contents of the port's output register will be present on the port's I/O signals on the IC.
- The inputs to each port are not latched, the data that is read through the port's register is "realtime".

Example source code: \[[highlighted](http://www.cpctech.org.uk/source/psgn1.html) \| [original](http://www.cpctech.org.uk/source/psgn1.asm) \]

Example source code: \[[highlighted](http://www.cpctech.org.uk/source/psgn2.html) \| [original](http://www.cpctech.org.uk/source/psgn2.asm) \]

Example source code: \[[highlighted](http://www.cpctech.org.uk/source/psgn3.html) \| [original](http://www.cpctech.org.uk/source/psgn3.asm) \]

Example source code: \[[highlighted](http://www.cpctech.org.uk/source/psgn4.html) \| [original](http://www.cpctech.org.uk/source/psgn4.asm) \]

## Additional notes for the AY-3-8912 in the CPC design

- Port A of the AY-3-8912 is connected to the keyboard. The data for a selected keyboard line can be read through Port A, as long as it is defined as input. The operating system and I believe all programs assume this port has been defined as input. (NWC has found a bug in the Multiface 2 software. The Multiface does not reprogram the input/output state of the AY-3-8912's registers, therefore if port A is programmed as output, the keyboard will be unresponsive and it will not be possible to use the Multiface functions.)

- When port B is defined as input (bit 7 of register 7 is set to "0"), a read of this port will return &FF.

  Example source code: \[[highlighted](http://www.cpctech.org.uk/source/psgn5.html) \| [original](http://www.cpctech.org.uk/source/psgn5.asm) \]
