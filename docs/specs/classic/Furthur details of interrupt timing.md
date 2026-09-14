<!--
Converted from "Furthur details of interrupt timing.html" on 2026-08-26 using pandoc (HTML → GFM).
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

## Furthur details of interrupt timing

Here is some information I got from Richard about the interrupt timing:

"Just when I finally thought I had the interrupt timing sorted out (from real tests on a 6128 and 6128+), I decided to look at the Arnold V diagnostic cartridge in WinAPE, and the Interrupt Timing test failed.

After pulling my hair out for a few hours, I checked out some info I found on the Z80 which states something like:

The Z80 forces 2 wait-cycles (2 T-States) at the start of an interrupt.

The code I had forced a 1us wait state for an interrupt acknowledge. For the most part this is correct, but it's not necessarily so. Seems the instruction currently being executed when an interrupt occurs can cause the extra CPC forced wait-state to be removed.

Those instructions are:

INC ss (ss = HL, BC, DE or SP) INC IX INC IY DEC ss DEC IX DEC IY RET cc (condition not met) EX (SP),HL EX (SP),IX EX (SP),IY LD SP,HL LD SP,IX LD SP,IY LD A,I LD I,A LD A,R LD R,A LDI (and both states of LDIR) LDD (and both states of LDDR) CPIR (when looping) CPDR (when looping)

This seems to be related to a combination of the T-States of the instruction, the M-Cycles, and the wait states imposed by the CPC hardware to force each instruction to the 1us boundary.

Richard"
