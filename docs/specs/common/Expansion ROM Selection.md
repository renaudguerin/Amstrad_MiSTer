<!--
Converted from "Expansion ROM Selection.html" on 2026-08-26 using pandoc (HTML → GFM).
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# Selecting an Expansion ROM

A expansion rom is selected by writing a code to the rom select I/O address.

The rom select I/O address is not fully decoded, and at a minimum bit 13 of the I/O address must be set to "0" for roms to be selected. To avoid conflict with other devices, the remaining bits in the I/O address should be set to "1". The recommended I/O address is &DFxx.

The rom select I/O address is write only.

An expansion rom is identified with a code which is in the range 0-255. BASIC is defined with a code of 0 and AMSDOS with a code of 7. BASIC will be selected if an attempt is made to select a rom which is not connected.

This process will select a rom, but will not allow access to the rom data.

To access the rom data:

- Select the rom using the method above,
- Enable the upper rom region using the Gate Array (see the document on the Gate Array for more information)
- The rom data can be accessed in the Z80 memory range &C000-&FFFF.

------------------------------------------------------------------------

## Programming Examples

Selecting an expansion rom,

To select rom 7. (AMSDOS)

ld bc,&7f00 ;Gate Array ld a,%10000100 ;enable upper rom, disable lower rom, mode 0 out (c),a ;send it ld bc,&DF00 ;expansion rom select port ld a,7 ;expansion rom wanted out (c),a ;select it ret

Disabling an expansion rom

ld bc,&7f00 ;Gate Array ld a,%10001100 ;upper and lower rom disabled, mode 0 out (c),a ;send it ret
