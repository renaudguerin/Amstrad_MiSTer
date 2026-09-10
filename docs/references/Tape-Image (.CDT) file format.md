<!--
Converted from "Tape-Image (.CDT) file format.html" on 2026-08-26 using pandoc (HTML → GFM).
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# Tape-Image (.CDT) file format

The ".CDT" tape image file format is identical to the ".TZX" file format designed by Tomaz Kac and used by Spectrum emulators. The filename has a different extension to differentiate between tape-images for use on the Amstrad and tape-images for use on the Spectrum.

The most recent version of this format can be found at [World of Spectrum](http://www.worldofspectrum.org).

## ZX TAPE FILE FORMAT v1.13

*Last changed by Tomaz Kac, November 2, 1998*

This format lets you preserve all (hopefully) of the tapes with turbo or custom loading routines. Even though some of the newer and 'smarter' emulators can find most of the info about the loader from the code itself, this isn't posible if you want to save the file to tape, or to a real Spectrum. And with all this information in the file, the emulators don't have to bother with finding out the baud rate and other things.

This file format was originally written for usage on ZX Spectrum compatible computers only, but as it turned out the SAM Coupe, Amstrad CPC, Jupiter ACE and Enterprise range of computers use the same tape encoding. It can be used for ZX-81 type of computers also, even though the encoding is somewhat different (for now there is no convertor for ZX-81 tapes into TZX). There are some special blocks which can be used on Commodore 64.

There is a table at the end of this file which displays (not totally official) timings for each machine ROM routines use. Also the description of the difference of encoding is written there. If you know of any other machines that have similar encoding that could be used with this file format then let me know.

The latest version of this format can be found at 'World of Spectrum'. You can also find many TZX files at the same address. The URL of 'WOS' is:

<http://www.worldofspectrum.org/>

The format was first started off by Tomaz Kac who was maintainer until revision 1.13, now this task is taken over by Martijn v.d. Heide ... if you have any questions about the format please e-mail to <mheide@worldofspectrum.org>.

The default format file extension is "TZX". Hopefully this won't have to change in the future. (For RISC OS, the current [TAP](http://www.worldofspectrum.org/faq/fileform.html#TAPZ) filetype will be used.)

Amstrad CPC files should use the extension "CDT" to distinguish them from the ZX Spectrum files. Otherwise the inner structure is totally the same.

- [Intro](http://www.cpctech.org.uk#intro)
- [Changes Since v1.13](http://www.cpctech.org.uk#changes)
- [Changes Since v1.12](http://www.cpctech.org.uk#changesold)
- [The Rules](http://www.cpctech.org.uk#rules)
- [The Blocks](http://www.cpctech.org.uk#blocks)
  - [ID 10](http://www.cpctech.org.uk#id10) - Standard Speed Data Block
  - [ID 11](http://www.cpctech.org.uk#id11) - Turbo Loading Data Block
  - [ID 12](http://www.cpctech.org.uk#id12) - Pure Tone
  - [ID 13](http://www.cpctech.org.uk#id13) - Sequence of Pulses of Different Lengths
  - [ID 14](http://www.cpctech.org.uk#id14) - Pure Data Block
  - [ID 15](http://www.cpctech.org.uk#id15) - Direct Recording
  - [ID 16](http://www.cpctech.org.uk#id16) - C64 ROM Type Data Block
  - [ID 17](http://www.cpctech.org.uk#id17) - C64 Turbo Tape Data Block
  - [ID 20](http://www.cpctech.org.uk#id20) - Pause (Silence) or \`Stop the Tape' Command
  - [ID 21](http://www.cpctech.org.uk#id21) - Group Start
  - [ID 22](http://www.cpctech.org.uk#id22) - Group End
  - [ID 23](http://www.cpctech.org.uk#id23) - Jump To Block
  - [ID 24](http://www.cpctech.org.uk#id24) - Loop Start
  - [ID 25](http://www.cpctech.org.uk#id25) - Loop End
  - [ID 26](http://www.cpctech.org.uk#id26) - Call Sequence
  - [ID 27](http://www.cpctech.org.uk#id27) - Return From Sequence
  - [ID 28](http://www.cpctech.org.uk#id28) - Select Block
  - [ID 2A](http://www.cpctech.org.uk#id2A) - Stop Tape if in 48K Mode
  - [ID 30](http://www.cpctech.org.uk#id30) - Text Description
  - [ID 31](http://www.cpctech.org.uk#id31) - Message Block
  - [ID 32](http://www.cpctech.org.uk#id32) - Archive Info
  - [ID 33](http://www.cpctech.org.uk#id33) - Hardware Type
  - [ID 34](http://www.cpctech.org.uk#id34) - Emulation Info
  - [ID 35](http://www.cpctech.org.uk#id35) - Custom Info Block
  - [ID 40](http://www.cpctech.org.uk#id40) - Snapshot Block
  - [ID 5A](http://www.cpctech.org.uk#id5A) - (90 dec, ASCII Letter \`Z')
- [Machine Specific Information](http://www.cpctech.org.uk#machine)
- [Hardware Information](http://www.cpctech.org.uk#hardware)

<span id="intro"></span>

### INTRO

The file is identified with the first 8 bytes being 'ZXTape!' plus the 'end of file' byte 26 (1A hex). This is followed by two bytes containing the major and minor version numbers. (For this format revision: major=01 and minor=13)

To be able to use a TZX file, your program (emulator, utility, or whatever) must be able to handle files of at least its major version number.

If your program can handle (say) version 1.05 and you encounter a file with version number 1.06, your program *must* be able to handle it, even if it cannot handle *all* the data in the file. The data it cannot handle should be relatively unimportant.

The main body of the file follows; it consists of a mixture of blocks, each identified by an ID byte. There are currently 20 types of blocks.

The file structure is:

0 "ZXTape!" 7 0x1A 8 Major version number 9 Minor version number A ID of first block B Body of first block ...

<span id="changes"></span>

### CHANGES SINCE v1.13

The Commodore 64/128 is now supported by the format. The encoding is the same, but because of different block structure two new block types had to be added, which can be used by other machines too if necessary, but their primary usage is on C-64. The new blocks are [16](http://www.cpctech.org.uk#id16) and [17](http://www.cpctech.org.uk#id17). The hardware specific information about the encoding is stored in another file because there is quite a lot of it, but you can also find out from the blocks.

<span id="changesold"></span>

### CHANGES SINCE v1.12

- Added the following fields to the '[Archive Info](http://www.cpctech.org.uk#id32)' block:
  - 05 - Game/Utility Type
  - 06 - Price
  - 07 - Protection Scheme / Loader
  - 08 - Origin
- Added two new types of '[Custom Info](http://www.cpctech.org.uk#id35)' block:
  - 'ZX-Edit document' - for .ZED files generated by the great ZX-Editor!
  - 'Picture ' - for .GIF and .JPEG (.JPG) pictures
- Added the new ID for the Enterprise computer with encoding and timings information (these might not be 100% correct though, Paul Hodgson is working on emulating this computer and will also supply new values when done).

<span id="rules"></span>

### THE RULES

Relevant rules and definitions important for the emulator and utility authors who will support the format:

- Any value requiring more than one byte is stored in little endian format (ie. LSB first).
- All unused bits should be set to zero.
- The timings are given in Z80 clock ticks (T states) unless otherwise stated.
  1 T state = (1/3500000)s
- The values in the tables are as follows:
  offset (in hex) length (1 = byte, 2 = short word, etc.)
  The block IDs are also given in hex.
- All ASCII texts must include only characters from 32 to 127 (decimal), some of them can have several lines, which should be separated by ASCII 13 (dec).
- You might interpret full-period as ----\_\_\_\_ or \_\_\_\_----, and half-period as ---- or \_\_\_\_. One half-period will also be referred to as a *pulse*.
- The values in square brackets \[\] are the default values for things such as the Spectrum ROM load/save routines - they should be used for all standard loading blocks. These values are in decimal.
- If there is *no* pause between two data blocks then the second one should follow *immediately*; not even so much as one T state between them.
- This document refers to 'high' and 'low' pulse levels. Whether this is implemented as ear=1 and ear=0 respectively or the other way around is not important, as long as it is done consistently.
- Zeros and ones in Direct Recording blocks signify low and high pulse levels respectively. The 'current pulse level' after playing a Direct Recording block is the last level played.
- Standard Speed Data blocks, Turbo Loading blocks, Pure Tone blocks and Sequence Of Pulses With Different Timings blocks consists of a (possibly odd) number of pulses. (The 'current pulse level' after playing these blocks is therefore the opposite of the last pulse level played, so that a subsequent pulse will produce an edge.)
- A 'Pause' block consists of a 'low' pulse level of some duration. To ensure that the last edge produced is properly finished there should be atleast 1ms pause of the *opposite* level and only after that the pulse should go to 'low'. At the end of a 'Pause' block the 'current pulse level' is low. (Note that the first pulse will therefore not immediately produce an edge.) A 'pause' block of zero duration is completely ignored, so the 'current pulse level' will NOT change in this case. This also applies to 'Data' blocks that have some pause duration included in them.
- An emulator should put the 'current pulse level' to 'low' when starting to play a TZX file, either from the start or from a certain position. The writer of a TZX file should ensure that the 'current pulse level' is well-defined in every sequence of blocks where this is important, i.e. in any sequence that includes a Direct Recording block, or that depends on edges generated by Pause blocks. The recommended way of doing this is to include a Pause after each sequence of blocks.
- When creating a Direct Recording block please stick to the standard sampling frequencies of 22050 or 44100 Hz. This will ensure correct playback when using PC's soundcards.
- The length of a block is given in the following format: Numbers in square brackets \[\] mean that the value must be read from the offset in the brackets. Other values are normal numbers. Example: \[02,03\]+0A means: get number (a word) from offset 02 and add 0A. All numbers are in hex !
- ALL Custom blocks that will be added after version 1.00 will have the length of the block in first 4 bytes (long word) after the ID (this length does *not* include these 4 length bytes). This should enable programs that can only handle older versions to skip that block.

Just in case:

MSB = most significant byte LSB = least significant byte MSb = most significant bit LSb = least significant bit

<span id="blocks"></span>

### THE BLOCKS

<span id="id10">**<u>ID : 10</u> - Standard Speed Data Block (as in [TAP](http://www.worldofspectrum.org/faq/fileform.html#TAPZ) files)**</span>

This block must be replayed with the standard ROM values for all loading speed variables (values in brackets \[\]).This block can be used for the ROM loading routines AND for custom loading routines that use the same timings as ROM ones do.

00 2 Pause After this block in milliseconds (ms) \[1000\] 02 2 Length of following data 04 x Data, as in [.TAP](http://www.worldofspectrum.org/faq/fileform.html#TAPZ) File

\- Length: \[02,03\]+04

<span id="id11">**<u>ID : 11</u> - Turbo Loading Data Block**</span>

This block is very similar to the normal [TAP](http://www.worldofspectrum.org/faq/fileform.html#TAPZ) block but with some additional info on the timings and other important differences. The same tape encoding is used as for the standard speed data block. If a block should use some non-standard sync or pilot tones (i.e. all sorts of protection schemes) then use the next three blocks to describe it.

00 2 Length of PILOT pulse \[2168\] 02 2 Length of SYNC First pulse \[667\] 04 2 Length of SYNC Second pulse \[735\] 06 2 Length of ZERO bit pulse \[855\] 08 2 Length of ONE bit pulse \[1710\] 0A 2 Length of PILOT tone (in PILOT pulses) \[8064 Header, 3220 Data\] 0C 1 Used bits in last byte (other bits should be 0) \[8\] i.e. if this is 6 then the bits (x) used in last byte are: xxxxxx00 0D 2 Pause After this block in milliseconds (ms) \[1000\] 0F 3 Length of following data 12 x Data; format is as for [TAP](http://www.worldofspectrum.org/faq/fileform.html#TAPZ) (MSb first)

\- Length: \[0F,10,11\]+12

<span id="id12">**<u>ID : 12</u> - Pure Tone**</span>

This will produce a tone which is basically the same as the pilot tone in the first two data blocks. You can define how long the pulse is and how many pulses are in the tone.

00 2 Length of pulse in T-States 02 2 Number of pulses

\- Length: 04

<span id="id13">**<u>ID : 13</u> - Sequence of Pulses of Different Lengths**</span>

This will produce n pulses, each having its own timing. Up to 255 pulses can be stored in this block; this is useful for non-standard sync tones used by some protection schemes.

00 1 Number of pulses 01 2 Length of first pulse in T-States 03 2 Length of second pulse... .. . etc.

\- Length: \[00\]\*02+01

<span id="id14">**<u>ID : 14</u> - Pure Data Block**</span>

This is the same as in the turbo loading data block, except that it has no pilot or sync pulses.

00 2 Length of ZERO bit pulse 02 2 Length of ONE bit pulse 04 1 Used bits in LAST Byte 05 2 Pause after this block in milliseconds (ms) 07 3 Length of following data 0A x Data (MSb first)

\- Length: \[07,08,09\]+0A

<span id="id15">**<u>ID : 15</u> - Direct Recording**</span>

This block is used for tapes which have some parts in a format such that the turbo loader block cannot be used. This is *not* like a VOC file, since the information is much more compact. Each sample value is represented by one bit only (0 for low, 1 for high) which means that the block will be at most 1/8 the size of the equivalent VOC.

*Please* use this block *only* if you cannot use the turbo load block. The preffered sampling frequencies are 22050 (158 T states) or 44100 Hz (79 T states/sample). Please, if you can, don't use other sampling frequencies.

00 2 Number of T states per sample (bit of data) 02 2 Pause after this block in milliseconds (ms) 04 1 Used bits (samples) in last byte of data (1-8) i.e. If this is 2 only first two samples of the last byte will be played 05 3 Data length 08 x Data. Each bit represents a state on the EAR port (i.e. one sample); MSb is played first.

\- Length: \[05,06,07\]+08

<span id="id16">**<u>ID : 16</u> - C64 ROM Type Data Block**</span>

Well, this block was created to support the Commodore 64 standard ROM and similar tape blocks. It is made so basically anything that uses two or four pulses (which are the same in pairs) per bit can be written with it. Some explanation:

- A wave consists of TWO pulses. In the structure the length of one pulse is written !
- The wave MUST always start with the LOW amplitude ... since the C64 can only detect the transistion HIGH -\> LOW !
- If some pulse length is 0 then the whole wave must not be present. This applies to DATA too !
- The XOR checksum (if it is set to 0 or 1) is a XOR of all bits in the byte XOR-ed with the value in this field as the start value.
- Finish Byte waves should be played after each byte EXCEPT last one.
- Finish Data waves should be ONLY played after last byte of data.
- When all the Data has finished there is an optional Trailer Tone, which is standard for the Repeated Blocks in C64 ROM Loader.

The replay procedure looks like this:

1.  Pilot Tone
2.  Sync waves
3.  Data Bytes (with XOR and/or Finish Byte waves)
4.  Finish Data pulses
5.  Trailing Tone

The numbers in \[\] brackets represent the values for C64 ROM loader ! For more information look in the special Commodore [64loader.txt](http://www.void.jump.org/64loader.txt) file !

00 4 Length of the WHOLE block including the data (extension rule) 04 2 PILOT TONE pulse length \[616\] 06 2 Number of waves in PILOT TONE 08 2 SYNC 1st wave pulse length \[1176\] 0A 2 SYNC 2nd wave pulse length \[896\] 0C 2 ZERO bit 1st wave pulse length \[616\] 0E 2 ZERO bit 2nd wave pulse length \[896\] 10 2 ONE bit 1st wave pulse length \[896\] 12 2 ONE bit 2nd wave pulse length \[616\] 14 1 XOR Checksum bit for each Data byte: \[1\] 00 - Start XOR checksum with value 0 01 - Start XOR checksum with value 1 FF - No checksum bit 15 2 FINISH BYTE 1st wave pulse length \[1176\] 17 2 FINISH BYTE 2nd wave pulse length \[896\] 19 2 FINISH DATA 1st wave pulse length \[1176\] 1B 2 FINISH DATA 2nd wave pulse length \[616\] 1D 2 TRAILING TONE pulse length \[616\] 1F 2 Number of waves in TRAILING TONE 21 1 Used bits in LAST byte \[8\] 22 1 General Purpose, bit-mapped: \[1\] bit 0 - Data Endianess : Set - Most Significant bit first Reset - Least Significant bit first 23 2 Pause after this block in milliseconds (ms) 25 3 Length of the Data 28 x Data

\- Length: \[00,01,02,03\]

<span id="id17">**<u>ID : 17</u> - C64 Turbo Tape Data Block**</span>

This block is made to support another type of encoding that is commonly used by the C64. Most of the commercial software uses this type of encoding, i.e. the Pilot tone is not made from one type of Wave only, but it is made from actual Data byte which is repeated many times. As the Sync value another, different, Data byte is sent to signal the start of the data. The Data Bits are made from ONE wave only and there is NO XOR checksum either! Trailing byte is played AFTER the DATA has ended.

00 4 Length of the WHOLE block including the data (extension rule) 04 2 ZERO bit pulse 06 2 ONE bit pulse 08 1 Additional Bits in Bytes (bit-mapped): bits 0-1 : Number of bits (0-3) 2 : 0 - Play additional bit(s) BEFORE the Byte 1 - Play additional bit(s) AFTER the Byte 3 : 0 - Value of additional bit(s) is 0 1 - Value of additional bit(s) is 1 09 2 Number of Lead-In Bytes 0B 1 Lead-In Byte 0C 1 Used bits in LAST byte \[8\] 0D 1 General Purpose, bit-mapped: \[0\] bit 0 - Data Endianess : Set - Most Significant bit first Reset - Least Significant bit first 0E 2 Number of Trailing Bytes 10 1 Trailing Byte 11 2 Pause after this block in milliseconds (ms) 13 3 Length of the Data 16 x Data

\- Length: \[00,01,02,03\]

<span id="id20">**<u>ID : 20</u> - Pause (Silence) or \`Stop the Tape' Command**</span>

This will make a silence (low amplitude level (0)) for a given time in milliseconds. If the value is 0 then the emulator or utility should (in effect) STOP THE TAPE, i.e. not continue loading until the user or emulator requests it.

00 2 Value of the pause in milliseconds (ms)

\- Length: 02

<span id="id21">**<u>ID : 21</u> - Group Start**</span>

This block marks the start of a group of blocks which are to be treated as one single (composite) block. This is *very* handy for tapes that use protected blocks like Speedlock 1 (which has around 64 pure tone blocks) or Bleeploads (which may well have over 160 custom loading blocks). You can also give the group a name (example 'Speedlock 1 Block').

For each group start block, there must be a group end block. Nesting of groups is *not* allowed.

00 1 Length of the Group Name 01 x Group name in ASCII (please keep it under 30 characters long)

\- Length: \[00\]+01

<span id="id22">**<u>ID : 22</u> - Group End**</span>

This indicates the end of a group. This block type has *no* body!

\- Length: 00

<span id="jumps"></span><span id="id23">**<u>ID : 23</u> - Jump To Block**</span>

This block will enable you to jump from one block to another within the file. The value is a signed short word (usually 'signed short' in C); eg. Some examples:

Jump 0 = 'Loop Forever' - this should *never* happen

Jump 1 = 'Go to the next block' - it is like NOP in assembler ;)

Jump 2 = 'Skip one block'

Jump -1 = 'Go to the previous block'

*All* blocks are included in the block count !

00 2 Relative jump value

\- Length: 02

<span id="id24">**<u>ID : 24</u> - Loop Start**</span>

If you have a sequence of identical blocks, or of identical groups of blocks, you can use this block to tell how many times they should be repeated.

This block is the same as the FOR statement in BASIC.

For simplicity reasons *don't* nest Loop blocks!

00 2 Number of repetitions (greater than 1)

\- Length: 02

<span id="id25">**<u>ID : 25</u> - Loop End**</span>

This is the same as BASIC's NEXT statement. It means that the utility should jump back to the start of the loop if it hasn't been run for the specified number of times.

This block is also without any body!

\- Length: 00

<span id="id26">**<u>ID : 26</u> - Call Sequence**</span>

This block is analogue of the CALL Subroutine statement. It basically executes a sequence of blocks that are somewhere else and then goes back to the next block. Because more than one call can be normally used you can include a list of sequences to be called.

The 'nesting' of call blocks is also not allowed for the same reasons. You can, of course, use the CALL blocks in the LOOP sequences and vice versa ...

The value is *relative* for the obvious reasons - so you can add some blocks in the beginning of the file without disturbing the call values. Please take a look at '[Jump To Block](http://www.cpctech.org.uk#jumps)' for reference on the values ...

00 2 Number of Calls to be made 00 2 Call 1 block number (*relative* signed offset) .. . Next Call block number

\- Length: \[00,01\]\*02+02

<span id="id27">**<u>ID : 27</u> - Return From Sequence**</span>

This block indicates the end of the Called Sequence. The next block played will be the block after the last CALL block (or the next Call, if the Call block had multiple calls).

Again, this block has no body.

\- Length: 00

<span id="id28">**<u>ID : 28</u> - Select Block**</span>

This block is useful when the tape consists of two or more separately-loadable parts. With this block, you are able to select one of the parts and the utility/emulator will start loading from that block. For example you can use it when the game has a separate Trainer or when it is a multiload. Of course, to make some use of it the emulator/utility has to show a menu with the selections when it encounters such a block. All offsets ar [*relative*](http://www.cpctech.org.uk#jumps) signed words.

00 2 Length of the whole block (without these two bytes) 02 1 Number of Selections 03 2 Relative Offset of Selection 1 05 1 Length of the Description 1 Text 06 x Description 1 Text in ASCII Please use only one line per Selection and limit it to 30 characters! .. . Next Selection Offset

\- Length: \[00,01\]+02

<span id="id2A">**<u>ID : 2A</u> - Stop Tape if in 48K Mode**</span>

When this block is encountered, the tape will stop ONLY if the machine is an 48K Spectrum.

This block is to be used for multiloading games that load one level at a time in 48K mode, but load the entire tape at once if in 128K mode.

This block has no body of its own, but follows the extension rule:

00 4 Length of the whole block (0)

\- Length: 4

<span id="id30">**<u>ID : 30</u> - Text Description**</span>

This is meant to identify parts of the tape, so you know where level 1 starts, where to rewind to when the game ends, etc. This description is not guaranteed to be shown while the tape is playing, but can be read while browsing the tape or changing the tape pointer.

The description can be up to 255 characters long but please keep it down to about 30 so the programs can show it in one line (where this is appropriate).

Please use '[Archive Info](http://www.cpctech.org.uk#id32)' block for title, authors, publisher,...

00 1 Length of the Text 01 x Text in ASCII

\- Length: \[00\]+01

<span id="id31">**<u>ID : 31</u> - Message Block**</span>

This will enable the emulators to display a message for a given time. This should *not* stop the tape and it should *not* make silence. If the time is 0 then the emulator should wait for the user to press a key.

It is not guaranteed that the emulator will actually display the message.

00 1 Time for which the message should be displayed in seconds (s) 01 1 Length of the message 02 x Message that should be displayed (in ASCII) Suggested format: - Stick to a maximum of 30 chars per line - Use single 0x0D (13 decimal) to separate lines. - Stick to a maximum of 8 lines. If you do not obey these rules, emulators may display your message in any way they like!

\- Length: \[01\]+02

<span id="id32">**<u>ID : 32</u> - Archive Info**</span>

Use this block at the beginning of the tape to identify the title of the game, author, publisher, year of publication, price (including the currency), type of software (Arcade Adventure, Puzzle, Word Processor, ...), protection scheme it uses (Speedlock 1, Alkatraz, ...) and its origin (Original, Budget re-release, ...), etc. This block is built in a way that allows easy future expansion. The block consists of a series of text strings. Each text has its identification number (which tells us what the text means) and then the ASCII text. To make it possible to skip this block, if needed, the length of the *whole* block is on the start of it.

If all texts in the tape are in English language then you don't have to supply the 'Language' field

The information about what hardware the tape uses is in the 'Hardware Type' block, so no need for it here.

00 2 Length of the block (without these two bytes) 02 1 Number of text strings 03 x Text strings: 00 1 Text Identification byte: 00 - Full Title 01 - Software House / Publisher 02 - Author(s) 03 - Year of Publication 04 - Language 05 - Game/Utility Type 06 - Price 07 - Protection Scheme / Loader 08 - Origin FF - Comment(s) 01 1 Length of text 02 x Text in ASCII format .. . Next Text

\- Length: \[00,01\]+02

<span id="id33">**<u>ID : 33</u> - Hardware Type**</span>

This selects what hardware the programs on this tape use. Please include only machines and hardware for which you are 100% sure that it either runs (or doesn't run) on or with, or you know it uses (or doesn't use) the hardware or special features of that machine.

If the tape runs only on the ZX81 (and TS1000, etc.) then it clearly won't work on any Spectrum or Spectrum variant, so there's no need to list this information.

If you are not sure or you haven't tested a tape on some particular machine/hardware combination then *do not* include it in the list.

00 1 Number of machines and hardware types for which info is supplied 01 x List of machines and hardware: 00 1 Hardware type 01 1 Hardware ID 02 1 Value 00 - The tape RUNS on this machine or with this hardware, but may or may not use the hardware or special features of the machine. 01 - The tape USES the hardware or special features of the machine, such as extra memory or a sound chip. 02 - The tape RUNS but it DOESN'T use the hardware or special features of the machine. 03 - The tape DOESN'T RUN on this machine or with this hardware. .. . Next Machine/Hardware Info (if any) The list of types and IDs is somewhat large, and may be found at the end of the format description.

\- Length: \[00\]\*03+01

<span id="id34">**<u>ID : 34</u> - Emulation Info**</span>

This is a special block that would normally be generated only by emulators. For now it contains info on everything I could find that other formats support. Please inform me of any additions/corrections since this is a very important part for emulators.

Those bits that are not used by the emulator that stored the info, should be left at their DEFAULT values.

00 2 General Emulation Flags Bit 0 : R-register emulation \[1\] 1 : LDIR emulation \[1\] 2 : High Resolution Color emulation with True Interrupt Freq. \[1\] 3,4 : Video Synchronisation : 1=High, 3=Low, 0,2=Normal \[0\] 5 : Fast Loading when ROM load routine is used \[1\] 6 : Border emulation \[1\] 7 : Screen Refresh mode (1: ON, 0: OFF) \[1\] 8 : Start Playing the tape Immediately \[0\] If this is 0 then the emulator should only load the info blocks and WAIT when it encounters first DATA block 9 : Auto type LOAD""\<ENTER\> or press \<ENTER\> when in 128k mode \[0\] 02 1 Screen Refresh Delay : 1 - 255 (Interrupts between refreshes) \[1\] (used when Screen Refresh Mode is ON) 03 2 Interrupt Frequency : 0 - 999 Hz \[50\] 05 3 Reserved for future expansion

\- Length: 08

<span id="id35">**<u>ID : 35</u> - Custom Info Block**</span>

This block can be used to save any information you want. For example, it might contain some information written by a utility, extra settings required by a particular emulator, the title screen, the cover art or even poke data.

Of course, some of the uses of this block may become standardised...

00 10 Identification string (in ASCII) 10 4 Length of the custom info 14 x Custom info

\- Length: \[10,11,12,13\]+14

**<u>Standardised Custom Info Blocks</u>**

- <u>POKEs block</u>

  The purpose of this Custom block is to hold any amount of different trainers for the game. Each trainer can have a description and any number of POKEs...

  In addition you can supply the memory Page number and/or the Original value of the address (if you want to restore it some way through the game). Normally you would enter these pokes with the help of some freezer-type tool like Multiface, but hopefully in the future the emulators will support this block directly... in which case you could use the 'User Inserts the POKE Value' feature. You can specify the point at which to insert the POKEs in the 'General Description'.

  0 10 'POKEs ' - Custom Block ID 10 4 Length of the following Data 14 1 General Description Length 15 x General Description in ASCII format 1 Number of trainers 1 Trainer 1 Description Length x Trainer 1 Description in ASCII format 1 Trainer 1 Number of POKEs 1 POKE 1 Type: bit 0-2 : Memory Page Number 3 : Ignore Memory Page Number ? 4 : User Inserts the POKE Value ? 5 : Unknown Original Value ? 2 POKE 1 Address 1 POKE 1 Value (leave at 0 if User Inserts it) 1 POKE 1 Original Value (leave at 0 if it is Unknown) . Next POKE (if any) . Next Trainer (if any)

  NOTE: All ASCII Descriptions can use more than one line ... please use only up to 30 characters per line and separate the line by one CR (13 dec).

- <u>Instructions block</u>

  This block can hold any general .TXT file... with the main purpose of storing the instructions to the program or game that is in the tape.

  0 10 'Instructions ' - Custom Block ID 10 4 Length of the following Data 14 x Instructions Text in ASCII format

  To ensure consistency with all other ASCII texts in this format please use a single CR character (13 dec) to separate lines, also please use only up to 80 characters per line.

- <u>Spectrum Screen block</u>

  If the game in the tape is not an original and lacks the original Loading screen then you can supply it sperately within this block. This is also very handly when you want the loading screen stored separately because the original is either encrypted (like with the 'SpeedLock' or 'Alkatraz' loaders) or it is corrupted by some on-screen info (like the 'BleepLoad' loader). Ofcourse not only loading screens can be stored here... you can use it to store maps or any other picture that is in Spectrum Video format (that's why the Description is there for), but because the Loading Screen will be the most common you can just set the Length of the Description to 0 when you use it for that. Also the Border colour can be specified.

  0 10 'Spectrum Screen ' - Custom Block ID 10 4 Length of the following Data 14 1 Description Length (if this is 0 then handle it as 'Loading Screen') 15 x Description of the Picture in ASCII format 1 BORDER Colour (0=Black, 1=Blue, ... in Spectrum Coulour Format) x Screen in Standard Spectrum Video format, always 6912 bytes long

- <u>ZX-Edit document block</u>

  This block can hold files created with the new utility called ZX-Editor.

  This utility gives documents the look and feel of ZX-Spectrum and its documents can contain text, graphics (with Spectrum attributes), different type faces, colours, etc. Normally these files use extension .ZED. Also the description is added, in case you want to use it for something else than 'Instructions' - you can use it for MAPs, etc.

  Please check on the official TZX format site for the .ZED format description. (hopefully it will be available soon)

  0 10 'ZX-Edit document' - Custom Block ID 10 4 Length of the following Data 14 1 Description Length (if this is 0 then handle it as 'Instructions') 15 x Description of the Document in ASCII format x The ZX-Editor document (.ZED file)

- <u>Picture block</u>

  Finally you can include *any* picture (in supported formats) in the TZX file too. So Cover pictures, maps, etc. can now be included in full colour (or whatever the formats supports).

  The best way for utilities to use this block is to spawn an external viewer, or the authors can write their own viewers (yeah, right ;) ).

  For Inlay Cards and other pictures that have zillions of colours use the JPEG format, for more simple pictures (drawing, maps, etc.) use the GIF format.

  0 10 'Picture ' - Custom Block ID 10 4 Length of the following Data 14 1 Picture format: 00 - GIF 01 - JPEG 15 1 Description Length (if this is 0 then handle it as 'Inlay Card') 16 x Description of the Document in ASCII format x The picture itself

<span id="id40">**<u>ID : 40</u> - Snapshot Block**</span>

This would enable one to snapshot the game at the start and still have all the tape blocks (level data, etc.) in the same file. Only .Z80 and .SNA snapshots are supported for compatibility reasons!

The emulator should take care of that the snapshot is not taken while the actual Tape loading is taking place (which doesn't do much sense). And when an emulator encounters the snapshot block it should load it and then continue with the next block.

00 1 Snapshot type: 00 - Z80 (Z80 Emulator, Warajevo, Z80Em ...) 01 - SNA (JPP, Amiga Spectrum Emulator, Z80Em ...) 01 3 Snapshot length 04 x Snapshot itself

\- Length: \[01,02,03\]+04

<span id="id5A">**<u>ID : 5A</u> (90 dec, ASCII letter 'Z')**</span>

This block is generated when you merge two ZX Tape files together. It is here so you can easily copy the files together and use them. Of course, this means that resulting file would be 10 bytes longer than if this block was not used. All you have to do if you encounter this block ID is to skip next 9 bytes.

If you can avoid using this block for this purpose, then do so; it is preferable to use a utility to join the two files and ensure that they are both of the higher version number

00 9 'XTape!' 0x1A MajR MinR Just skip these 9 bytes and you will end up on the next ID.

\- Length: 09

<span id="machine"></span>

### MACHINE SPECIFIC INFORMATION

Currently supported machines are: ZX Spectrum, Amstrad CPC, SAM Coupe, ZX-81 Jupiter ACE and Enterprise. Only ZX-81 (or the old Timex 1000) and Jupiter ACE use different tape encoding than the others.

The Commodore 64 encoding is described in a separate file [64loader.txt](http://www.worldofspectrum.org/64loader.txt)!

**<u>ZX Spectrum, Amstrad CPC, SAM Coupe, Jupiter ACE & Enterprise Tape Encoding</u>**

These computers share the same tape encoding, which is normal Frequency type encoding. Each bit is represented by one 'wave' of different duration for bit 0 and bit 1. Normally bit 1 is twice as big as bit 0. In the blocks, the timings are presented with Z80 T-states (cycles) per pulse. One wave is made from two pulses.

<u>IMPORTANT</u>

Z80 Clock rate is always timed at 3.5 MHz, so if the tape should be used on an Amstrad CPC with 4MHz clock then you should multiply ALL timings with 4/3.5 when you use that tape (this should be trivial), similary goes for SAM Coupe, that uses a 6MHz clock and Jupiter ACE that uses 3.2448 MHz and Enterprise that uses 4 MHz!

The Standard data blocks have always the same structure: First there is the Pilot Tone, then the two SYNC pulses (one wave) and after that the actual data (which can include a FLAG byte at the beginning and a Check-Sum byte at the end).

Here is a table showing the pulse timings for each part of the standard ROM load/save routine for wach of these machines. If you have more accurate information then I would be glad to include it here. Enterprise has two loading speeds so timings for both are included. (more accurate timings and more info on Pilot length for the Enterprise will be provided in the next release of this format).

Machine Pilot pulse, length Sync1 Sync2 Bit-0 Bit-1 ----------------------------------------------------------------------- ZX Spectrum 2168 (1) 667 735 855 1710 SAM Coupe 58+19\*W 6000 58+9\*H 113+9\*H 10+8\*W 42+15\*W (\*) Amstrad CPC Bit-1 4096 Bit-0 Bit-0 (2) (2) Jupiter ACE 2011 (3) 600 790 801 1591 Enterprise (fast) 742 ? 1280 1280 882 602 (\*\*) Enterprise (slow) 1750 ? 2800 2800 1982 1400 (\*\*)

\(1\) The Spectrum uses different Pilot Lengths for Header and Data blocks. Header blocks have 8064 and data blocks have 3220 pilot pulses.

\(2\) Amstrad CPC ROM Load/Save routine can use variable speed for loading, so the Bit-1 pulse must be read from the Pilot Tone and Bit-0 can be read from the Sync pulses, and is always half the size of Bit-1. The Check-Sum is also different than the other two machines. The speed can vary from 1000 to 2000 baud.

\(3\) Jupiter ACE also uses different Pilot Lengths for Header and Data blocks. Header blocks have 8192 and Data blocks have 1024 pilot pulses. The last byte of the data is a XOR of all previous bytes (as in Spectrum). Also, at the end of everything there is a pulse of 762 T-states. The Header block has Sync byte (first byte after Sync pulses) 0 and the Data blocks can have anything but 0.

(\*) The SAM Coupe timings can be user selected by a System Variable... the standard value being at 112, which is VERY close to ZX Spectrum loading speed, and therefore the 'Standard Speed Data' block can be used for those blocks. However if this System Variable is changed then the timings will change accordingly... in the above table the value of this variable is given as 'W', the 'H' value is 'W/2'. Of course the best way to determine it is to calculate it from the timings you get when sampling the Pilot tone... The values in the table could be by a fraction off the real ones, but it should not matter. Note: All timings are written in 3.5MHz clock. Also, there might be some junk bits (usually 7 or 8) AFTER the CheckSum (XOR) byte at the end of the block... they can just be ignored...

(\*\*) The Enterprise stores data in a different way than all other computers do. It stores the Least Significant Bit first, but the Data blocks require the data to be Most Significant Bit first. This might lead to some confusion, but if the same mechanism is used to replay data for all machines then there will be no problem. Just store the data as Most Significant Bit first, but if you want to view the raw data in the TZX file then you will have to mirror each byte to get the correct values.

NOTE: The ZXTape format has ALL timings written according to a 3.5MHz clock... when you want to use it on a, for example, 4MHz clock based computer you should multiply ALL timings with 1.142857 (Do NOT use 4MHz or 6MHz base clock to store the timings in a TZX file!)

**<u>ZX-81 (and compatibles) Tape Encoding</u>**

ZX-81 uses tape encoding which makes loading A LOT slowe than the other machines. Unfortunately this type of encoding can *not* be described with the 'Custom Loading Data' block, but with a series of 'Pure Tone' and 'Pause' blocks. Since no custom loaders were used on it, All ZX-81 tapes used this encoding and therefore the 'Standard Speed Data' block can be used for it.

Main difference is that the encoding is not frequency-based, it is rather based on the number of waves. Bit-0 is represented by 3 to 5 (normally 4) waves and Bit-1 is represented by 6 to 9 waves (normally 8). The frequency of the waves is 12kHz (each pulse is ~637 T-states long). After each series of waves there is a small pause (approximately the same duration as Bit-0).

So the 01 sequence would look like this (\~~\_\_ is one wave, --- is pause):

---\~~\_\_\~~\_\_\~~\_\_\~~\_\_---------\~~\_\_\~~\_\_\~~\_\_\~~\_\_\~~\_\_\~~\_\_\~~\_\_\~~\_\_---------\~~\_\_\~~...

The Pilot Tone seems to be structured as the following sequence: 10101010 (the same timing as the data), after which the Data follows imemdiately (*no* Sync pulses are present between them).

If the tape should be used for any *other* computer than ZX-Spectrum then it should have the 'Hardware Info' block as the first block in which the computer type should be described.

Because the Standard Loading data blocks of ZX-Spectrum. SAM Coupe and ZX-81 are so unique (i.e. they are always the same) the 'Standard Loading Data' block can be used for their description. Amstrad CPC tapes should always use the 'Custom Loading Data' block, since the speed is not always the same.

<span id="hardware"></span>

### HARDWARE INFORMATION

This is the list of all Hardware Types and Hardware Identification ID's that are used in the 'Hardware Info' block. Please send any additions you might have to me so they can be included.

By Default you don't have to write any of the information if the game is made for the ZX Spectrum and complies with the following:

\- Runs on ZX Spectrum 48K

\- Runs on, but doesn't use any of the special hardware of ZX Spectrum 128K

\- Doesn't run on ZX Spectrum 16K

If, for example, game works on BOTH ZX 48K and 128K, and uses the hardware of the 128K Spectrum, then you would just include the 128K Spectrum in the list (because by default it has to work on 48K too)...

If the game is 128K ONLY then you would include two entries: The game works on AND uses the hardware of a 128K Spectrum AND the game DOESN'T work on a 48K Spectrum.

If the game works on both 48K and 128K Spectrum, but it only uses the Sound chip (AY) of the 128K Spectrum and none of its extra memory then you would only include the entry that says that the game uses the 'Classic AY Hardware (Spectrum 128 Compatible Sound Device)'.

These were only some examples of the usage of this block.

Hardware Type Hardware ID ----------------------------------------------------------------------------- 00 - Computers 00 - ZX Spectrum 16k 01 - ZX Spectrum 48k, Plus 02 - ZX Spectrum 48k ISSUE 1 03 - ZX Spectrum 128k (Sinclair) 04 - ZX Spectrum 128k +2 (Grey case) 05 - ZX Spectrum 128k +2A, +3 06 - Timex Sinclair TC-2048 07 - Timex Sinclair TS-2068 08 - Pentagon 128 09 - Sam Coupe 0A - Didaktik M 0B - Didaktik Gama 0C - ZX-81 with 1k RAM 0D - ZX-81 with 16k RAM or more 0E - ZX Spectrum 128k, Spanish version 0F - ZX Spectrum, Arabic version 10 - TK 90-X 11 - TK 95 12 - Byte 13 - Elwro 14 - ZS Scorpion 15 - Amstrad CPC 464 16 - Amstrad CPC 664 17 - Amstrad CPC 6128 18 - Amstrad CPC 464+ 19 - Amstrad CPC 6128+ 1A - Jupiter ACE 1B - Enterprise 1C - Commodore 64 1D - Commodore 128 01 - External storage 00 - Microdrive 01 - Opus Discovery 02 - Disciple 03 - Plus-D 04 - Rotronics Wafadrive 05 - TR-DOS (BetaDisk) 06 - Byte Drive 07 - Watsford 08 - FIZ 09 - Radofin 0A - Didaktik disk drives 0B - BS-DOS (MB-02) 0C - ZX Spectrum +3 disk drive 0D - JLO (Oliger) disk interface 0E - FDD3000 0F - Zebra disk drive 10 - Ramex Millenia 11 - Larken 02 - ROM/RAM type add-ons 00 - Sam Ram 01 - Multiface 02 - Multiface 128k 03 - Multiface +3 04 - MultiPrint 05 - MB-02 ROM/RAM expansion 03 - Sound devices 00 - Classic AY hardware (compatible with 128k ZXs) 01 - Fuller Box AY sound hardware 02 - Currah microSpeech 03 - SpecDrum 04 - AY ACB stereo (A+C=left, B+C=right); Melodik 05 - AY ABC stereo (A+B=left, B+C=right) 04 - Joysticks 00 - Kempston 01 - Cursor, Protek, AGF 02 - Sinclair 2 Left (12345) 03 - Sinclair 1 Right (67890) 04 - Fuller 05 - Mice 00 - AMX mouse 01 - Kempston mouse 06 - Other controllers 00 - Trickstick 01 - ZX Light Gun 02 - Zebra Graphics Tablet 07 - Serial Ports 00 - ZX Interface 1 01 - ZX Spectrum 128k 08 - Parallel ports 00 - Kempston S 01 - Kempston E 02 - ZX Spectrum +3 03 - Tasman 04 - DK'Tronics 05 - Hilderbay 06 - INES Printerface 07 - ZX LPrint Interface 3 08 - MultiPrint 09 - Opus Discovery 0A - Standard 8255 chip with ports 31,63,95 09 - Printers 00 - ZX Printer, Alphacom 32 & compatibles 01 - Generic Printer 02 - EPSON Compatible 0A - Modems 00 - VTX 5000 01 - T/S 2050 or Westridge 2050 0B - Digitisers 00 - RD Digital Tracer 01 - DK'Tronics Light Pen 02 - British MicroGraph Pad 0C - Network adapters 00 - ZX Interface 1 0D - Keyboards & keypads 00 - Keypad for ZX Spectrum 128k 0E - AD/DA converters 00 - Harley Systems ADC 8.2 01 - Blackboard Electronics 0F - EPROM Programmers 00 - Orme Electronics

*Format devised by Tomaz Kac, sep 1997. HTML-ed by Martijn v.d. Heide, sep 1997*
