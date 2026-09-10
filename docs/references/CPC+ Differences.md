<!--
Converted from "CPC+ Differences.html" on 2026-08-26 using pandoc (HTML → GFM).
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

## Differences

### Cartridge

The 464+ and 6128+ are supplied with a system cartridge, which contains the operating system, Locomotive BASIC, AMSDOS and the free game "Burnin' Rubber".

The CPC464, 664 and 6128 had a 32K ROM on the PCB which contained the operating system and BASIC. The CPC6128 was also fitted with a 16K ROM which contained AMSDOS.

In the CPC+ these "roms" are contained in a single 128K ROM/EPROM inside the cartridge.

On the old CPC system, the operating system and BASIC were internal and always available, but this cartridge **must** be inserted if you want to use the operating system or BASIC.

What this also means, is that if you have a game cartridge inserted, there will not be any operating system or firmware and the game must provide it's own functions to access the hardware.

The 464+ and 6128+ are therefore useless without a cartridge.

If a cartridge is not inserted I see a coloured screen and nothing happens.

When the Plus is switched on with the system cartridge plugged-in, a menu is displayed, and you can select to enter Basic (by pressing f1) or play the free game "Burnin' Rubber" (by pressing f2).

The cartridge for the 464+ is identical to the cartridge for the 6128+ and also contains AMSDOS.

When the 464+ is initialised it starts up in cassette mode. Typing:

RUN"

will return the message

Press PLAY then any key:

The AMSDOS disc commands are available, and, if you type:

\|DISC CAT

then the message:

Drive A: user 0

will be displayed and the computer will lock up. Pressing SHIFT+CTRL+ESC will reset it.

If a user wishes to use a DDI-1 disc drive controller with the 464+ it must be modified do prevent the AMSDOS rom inside the interface from being activated.

### Cartridge - Operating System

### Cartridge - BASIC

The version of BASIC supplied with the CPC+ is Locomotive BASIC v1.1 and is the same as the CPC6128, except byte 3 of the rom data, the "ROM Version Number", has changed from 2 to 4. This is minor and programs written for the BASIC that comes with the CPC6128 will run on the CPC+ computers without change.

### Cartridge - AMSDOS

The AMSDOS rom data has been modified.

- The third byte of the rom the "ROM Version Number" has changed from 5 to 7, and this can be checked using the Firmware to indentify between this version of AMSDOS and the version of AMSDOS in the CPC6128.

- Some commands have been removed, and others have been added:

  Commands removed:

  \|TAPE.IN, \|TAPE.OUT, \|DISC.IN and \|DISC.OUT

  Commands added: (all of these commands will start the free game, "Burnin' Rubber"):

  \|GAME, \|JEUX, \|SPEIL, \|JUEGO

- In the AMSDOS supplied in the CPC6128 and the DDI-1 disc interface, the second 8K of the AMSDOS rom data was used by Logo, in the CPC+ this is filled with &0ff.

- There is code in the AMSDOS rom to show the start-up menu

- The CPC+ version of AMSDOS has functions to check for the presence of the NEC765 floppy disc controller. If detected, then disc operations are enabled, and the disc will be accessed by default. If not detected, then the tape operations are enabled by default. It is possible to access the disc versions, but a "Bad command" error will be the result.

As a result, any programs which use the removed commands, or access functions directly in the disc ROM will not work.

## Hardware Differences

### Internal differences

The CPC+ has a ASIC which comprises:

- functionality of the RAM management PAL,
- functionality of the 8255 PPI,
- functionality of the Gate-Array,
- functionality of the 6845 CRTC
- the enhanced features of the CPC+ (extended colour palette, hardware sprites, smooth hardware scrolling)

The functionality of the 8255, Gate-Array and 6845 is not identical to the CPC. As a result programs written which rely on special features of these chips may not work correct.

There are a number of titles which do not work. A common problem results in the keyboard or joysticks being unresponsive. These programs must be patched to allow them to work with the CPC+.

The details of the enhanced features can be found in the official datasheet.

Extra details, found from testing and experiments can be found in the unofficial datasheet.

On the CPC it was possible to hear cassette sounds (the sound from the cassette when a program is loaded or saved) through the speaker if the volume was high. However, the CPC464+ and CPC6128+ are silent.

### External differences

- The English CPC's had PCB edge connectors, but on the 464+ and 6128+ these are replaced by decent connectors. What this means is that you will need an adaptor (a "Widget") if you want to connect CPC peripherals.
  German CPC's already have these good connectors and as a result, a "Widget" may not be needed.
- Unlike the CPC6128, the CPC6128+ does not have a cassette connector (the CPC6128+ is missing the hardware required for cassette support), and therefore it is not possible to use cassette software without modification to the computer.
- The CPC464+ does not have a second drive disc connector.
- The CPC+ has additional connectors: Analogue joystick, 2nd digital joystick port, cartridge connector, light-pen/light-gun connector.
- It is not possible to use a Amstrad CPC colour monitor directly (CTM644/CTM640) with the CPC+. An adaptor must be used. The Amstrad CPC+ monitor (CM14) has a power lead with a different inner diameter and the number of pins on the monitor DIN plug is different (there are more pins to accomodate the stereo sound output to the monitor).
- The CPC464+ does not have a tape counter, so it is not easy to rewind to specific points on a cassette.
- The CPC464+ keyboard has the same layout and colour as the CPC6128 and CPC6128+.
- Both the CPC464+ and CPC6128+ have a newer design. The case is flatter and longer.
- The case has "Amstrad" logo embossed in it.
