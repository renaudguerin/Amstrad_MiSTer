<!--
Converted from "Format_CPR CPC Plus cartridge file format - CPCWiki - THE Amstrad CPC encyclopedia!.html" on 2026-08-26 using pandoc (HTML → GFM).
Source: https://www.cpcwiki.eu/index.php?title=Format:CPR_CPC_Plus_cartridge_file_format&oldid=65181 (CPCWiki)
-->

<div style="border: 1px solid rgb(228, 222, 222); margin: 0px 0px 5px; padding: 0.5em 1em; background-color: rgb(249, 249, 249);">

***This article originally came from Kevin Thackers' archive at <a href="http://www.cpctech.org.uk" class="external free">http://www.cpctech.org.uk</a>.***

</div>

# <span id=".22.CPR.22_CPC_Plus_Cartridge_file_data_structure" class="mw-headline">".CPR" CPC Plus Cartridge file data structure</span>

The file structure used is "RIFF" (Resource Interchange File Format).

## <span id="Outline_of_the_basic_RIFF_file_structure" class="mw-headline">Outline of the basic RIFF file structure</span>

A RIFF file is constructed of "chunks".

Every chunk has a header with the form:

| Offset | Size | Description                                                   |
|--------|------|---------------------------------------------------------------|
| 0      | 4    | chunk id (4 character code)                                   |
| 4      | 4    | Length of data following chunk-header. (little-endian format) |

The file begins with a "RIFF" chunk. This chunk contains a 4-byte "form-type" which identifies the file sub-type, followed by the remaining chunks in the file. The RIFF chunk is a container for all the remaining chunks in the file, therefore the length of the RIFF chunk data is equivalent to the total size of the file excluding the length of the header for the RIFF chunk.

## <span id=".22.CPR.22_specific_file_structure" class="mw-headline">".CPR" specific file structure</span>

The .CPR file uses the "Ams!" form-type.

Each chunk in the file contains (at most) a 16k range from the total cartridge data.

In the CPC+ system, a cartridge is made of up to 32 16k blocks. In the CPR file these blocks are numbered 0..31.

The id of the chunk is used to identify the cartridge block that the data corresponds to. (e.g. "cb00" corresponds to cartridge block 0, and contains the data for the range &0000-&3FFF, "cb01" corresponds to cartridge block 1, and contains the data for the range &4000-&7FFF).

A block may contain less than 16k, but should not contain more than 16k. If a emulator encounters a block with less than 16k, it should fill the rest of the range with 0's, if a emulator encounters a block with more than 16k, it should stop reading at 16k and ignore the remaining data.
