<!--
Converted from "mem.html" on 2026-08-26 using pandoc (HTML → GFM).
Note: 9 table(s) with rowspan/colspan were expanded to repeated cells to fit pipe tables.
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# Memory selection hardware

In the CPC6128, the memory selection hardware is controlled by a PAL chip. The input and output signals of this chip are known and can be found by studying the CPC6128 service manual.

The functions of the PAL chip are also known, as this is well documented, however the truth table and logic operation of the signals are not known.

This document presents a valid logic schematic which would perform the same action as the original chip.\
External Signals\
 \
 

|           |           |                                                    |
|-----------|-----------|----------------------------------------------------|
| Signal    | Direction | Description                                        |
| D7 AND D6 | IN        |                                                    |
| D0        | IN        |                                                    |
| /RESET    | IN        |                                                    |
| RAMDIS    | IN        | "1" to disable ram                                 |
| D1        | IN        |                                                    |
| D2        | IN        |                                                    |
| NCAS      | IN        |                                                    |
| A15       | IN        |                                                    |
| A14       | IN        |                                                    |
| A14OUT    | OUT       | final A14 defines memory region to access in bank  |
| A15OUT    | OUT       | final A15. defines memory region to access in bank |
| /IOWR     | IN        | "0": I/O Write opertation                          |
| /CAS0     | OUT       | "0": selects bank 0 of 64K ram                     |
| /CAS1     | OUT       | "0": selects bank 1 of 64K ram                     |
| /CPU      | IN        | 1MHZ CLOCK                                         |
| GND       |           |                                                    |

# Truth Tables

NOTE:

- sub-blocks are numbered 0..3.
- each sub-block is 16k.
- sub-block "0" corresponds to range &0000-&3fff within one of the 64k blocks.
- sub-block "1" corresponds to range &4000-&7fff within one of the 64k blocks.
- sub-block "2" corresponds to range &8000-&bfff within one of the 64k blocks.
- sub-block "3" corresponds to range &c000-&ffff within one of the 64k blocks.
- memory is split into blocks of 64k.
- "\*" indicates a sub-block in the second 64k of ram.
- there are 8 possible memory selections
- the memory select hardware responds to I/O writes only, and the following conditions must be true: A15="0",A14="1",D7="1",D6="1"
- In the standard CPC6128 hardware D2,D1,D0 choose the memory selection.

## SELECTION 0

 

| Memory Range | Block |
|--------------|-------|
| &0000-&3fff  | 0     |
| &4000-&7fff  | 1     |
| &8000-&bfff  | 2     |
| &c000-&ffff  | 3     |

\
 

| D2  | D1  | D0  | A15 | A14 | /CAS1 | /CAS0 | A15OUT | A14OUT | OUTPUTS |
|-----|-----|-----|-----|-----|-------|-------|--------|--------|---------|
| 0   | 0   | 0   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 0   | 0   | 0   | 0   | 1   | 1     | 0     | 0      | 1      |         |
| 0   | 0   | 0   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 0   | 0   | 0   | 1   | 1   | 1     | 0     | 1      | 1      |         |

## SELECTION 1

 

| Memory Range | Block |
|--------------|-------|
| &0000-&3fff  | 0     |
| &4000-&7fff  | 1     |
| &8000-&bfff  | 2     |
| &c000-&ffff  | 3\*   |

\
 \
 

| D2  | D1  | D0  | A15 | A14 | /CAS1 | /CAS0 | A15OUT | A14OUT | OUTPUTS |
|-----|-----|-----|-----|-----|-------|-------|--------|--------|---------|
| 0   | 0   | 1   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 0   | 0   | 1   | 0   | 1   | 1     | 0     | 0      | 1      |         |
| 0   | 0   | 1   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 0   | 0   | 1   | 1   | 1   | 0     | 1     | 1      | 1      |         |

## SELECTION 2

 

| Memory Range | Block |
|--------------|-------|
| &0000-&3fff  | 0\*   |
| &4000-&7fff  | 1\*   |
| &8000-&bfff  | 2\*   |
| &c000-&ffff  | 3\*   |

\
 \
 

| D2  | D1  | D0  | A15 | A14 | /CAS1 | /CAS0 | A15OUT | A14OUT | OUTPUTS |
|-----|-----|-----|-----|-----|-------|-------|--------|--------|---------|
| 0   | 1   | 0   | 0   | 0   | 0     | 1     | 0      | 0      |         |
| 0   | 1   | 0   | 0   | 1   | 0     | 1     | 0      | 1      |         |
| 0   | 1   | 0   | 1   | 0   | 0     | 1     | 1      | 0      |         |
| 0   | 1   | 0   | 1   | 1   | 0     | 1     | 1      | 1      |         |

## SELECTION 3

 

| Memory Range | Block |
|--------------|-------|
| &0000-&3fff  | 0     |
| &4000-&7fff  | 3     |
| &8000-&bfff  | 2     |
| &c000-&ffff  | 3\*   |

\
 \
 

| D2  | D1  | D0  | A15 | A14 | /CAS1 | /CAS0 | A15OUT | A14OUT | OUTPUTS |
|-----|-----|-----|-----|-----|-------|-------|--------|--------|---------|
| 0   | 1   | 1   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 0   | 1   | 1   | 0   | 1   | 1     | 0     | 1      | 1      |         |
| 0   | 1   | 1   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 0   | 1   | 1   | 1   | 1   | 0     | 1     | 1      | 1      |         |

## SELECTION 4

 

| Memory Range | Block |
|--------------|-------|
| &0000-&3fff  | 0     |
| &4000-&7fff  | 0\*   |
| &8000-&bfff  | 2     |
| &c000-&ffff  | 3     |

\
 \
 

| D2  | D1  | D0  | A15 | A14 | /CAS1 | /CAS0 | A15OUT | A14OUT | OUTPUTS |
|-----|-----|-----|-----|-----|-------|-------|--------|--------|---------|
| 1   | 0   | 0   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 1   | 0   | 0   | 0   | 1   | 0     | 1     | 0      | 0      |         |
| 1   | 0   | 0   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 1   | 0   | 0   | 1   | 1   | 1     | 0     | 1      | 1      |         |

## SELECTION 5

 

| Memory Range | Block |
|--------------|-------|
| &0000-&3fff  | 0     |
| &4000-&7fff  | 1\*   |
| &8000-&bfff  | 2     |
| &c000-&ffff  | 3     |

\
 \
 

| D2  | D1  | D0  | A15 | A14 | /CAS1 | /CAS0 | A15OUT | A14OUT | OUTPUTS |
|-----|-----|-----|-----|-----|-------|-------|--------|--------|---------|
| 1   | 0   | 1   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 1   | 0   | 1   | 0   | 1   | 0     | 1     | 0      | 1      |         |
| 1   | 0   | 1   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 1   | 0   | 1   | 1   | 1   | 1     | 0     | 1      | 1      |         |

## SELECTION 6

 

Memory Range

Block

&0000-&3fff

0

&4000-&7fff

2\*

&8000-&bfff

2

&c000-&ffff

s

3

\
 \
 

| D2  | D1  | D0  | A15 | A14 | /CAS1 | /CAS0 | A15OUT | A14OUT | OUTPUTS |
|-----|-----|-----|-----|-----|-------|-------|--------|--------|---------|
| 1   | 1   | 0   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 1   | 1   | 0   | 0   | 1   | 0     | 1     | 1      | 0      |         |
| 1   | 1   | 0   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 1   | 1   | 0   | 1   | 1   | 1     | 0     | 1      | 1      |         |

## SELECTION 7

 

| Memory Range | Block |
|--------------|-------|
| &0000-&3fff  | 0     |
| &4000-&7fff  | 3\*   |
| &8000-&bfff  | 2     |
| &c000-&ffff  | 3     |

\
 \
 

| D2  | D1  | D0  | A15 | A14 | /CAS1 | /CAS0 | A15OUT | A14OUT | OUTPUTS |
|-----|-----|-----|-----|-----|-------|-------|--------|--------|---------|
| 1   | 1   | 1   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 1   | 1   | 1   | 0   | 1   | 0     | 1     | 1      | 1      |         |
| 1   | 1   | 1   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 1   | 1   | 1   | 1   | 1   | 1     | 0     | 1      | 1      |         |

\
 

## ALL SELECTIONS

 

| D2  | D1  | D0  | A15 | A14 | /CAS1 | /CAS0 | A15OUT | A14OUT | OUTPUTS |
|-----|-----|-----|-----|-----|-------|-------|--------|--------|---------|
| 0   | 0   | 0   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 0   | 0   | 0   | 0   | 1   | 1     | 0     | 0      | 1      |         |
| 0   | 0   | 0   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 0   | 0   | 0   | 1   | 1   | 1     | 0     | 1      | 1      |         |
| 0   | 0   | 1   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 0   | 0   | 1   | 0   | 1   | 1     | 0     | 0      | 1      |         |
| 0   | 0   | 1   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 0   | 0   | 1   | 1   | 1   | 0     | 1     | 1      | 1      |         |
| 0   | 1   | 0   | 0   | 0   | 0     | 1     | 0      | 0      |         |
| 0   | 1   | 0   | 0   | 1   | 0     | 1     | 0      | 1      |         |
| 0   | 1   | 0   | 1   | 0   | 0     | 1     | 1      | 0      |         |
| 0   | 1   | 0   | 1   | 1   | 0     | 1     | 1      | 1      |         |
| 0   | 1   | 1   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 0   | 1   | 1   | 0   | 1   | 1     | 0     | 1      | 1      |         |
| 0   | 1   | 1   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 0   | 1   | 1   | 1   | 1   | 0     | 1     | 1      | 1      |         |
| 1   | 0   | 0   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 1   | 0   | 0   | 0   | 1   | 0     | 1     | 0      | 0      |         |
| 1   | 0   | 0   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 1   | 0   | 0   | 1   | 1   | 1     | 0     | 1      | 1      |         |
| 1   | 0   | 1   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 1   | 0   | 1   | 0   | 1   | 0     | 1     | 0      | 1      |         |
| 1   | 0   | 1   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 1   | 0   | 1   | 1   | 1   | 1     | 0     | 1      | 1      |         |
| 1   | 1   | 0   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 1   | 1   | 0   | 0   | 1   | 0     | 1     | 1      | 0      |         |
| 1   | 1   | 0   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 1   | 1   | 0   | 1   | 1   | 1     | 0     | 1      | 1      |         |
| 1   | 1   | 1   | 0   | 0   | 1     | 0     | 0      | 0      |         |
| 1   | 1   | 1   | 0   | 1   | 0     | 1     | 1      | 1      |         |
| 1   | 1   | 1   | 1   | 0   | 1     | 0     | 1      | 0      |         |
| 1   | 1   | 1   | 1   | 1   | 1     | 0     | 1      | 1      |         |

## Observations

- For selections 4,5,6,7: A15OUT=D1, A14OUT=D0 when A15="0",A14="1" and D2="1".
- /CAS1 = ~/CAS0 and therefore both are derived from NCAS
- For selections 0,1,2: A15OUT=A15, A14OUT=A14.
- Only memory ranges &4000-&7fff and &c000-&ffff are effected by memory paging.
  \
   
