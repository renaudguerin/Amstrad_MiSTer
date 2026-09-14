<!--
Converted from "DSC disk image format.html" on 2026-08-26 using pandoc (HTML → GFM).
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# DSC file structure

This document describes the structure of the "dsc" disk image file used by Richard Wilson's Amstrad CPC emulators.

The DSC disk image uses two files, one with the extension "hdr" and a second with the extension "dsc".

The "HDR" file describes the format of the disc format described by this disk image.

If the "DSC" file exists then it contains the data for one or more sectors.

## Structure of the "HDR" file

The "HDR" file describes the structure of the disc format described by this disk image format.

The file is composed of multiple "track description" records. Each record describes the structure of a single track on the disc. The records are stored continuously and in ascending numerical track order.

Each "track description" record has the form:

| Length               | Description                       |
|----------------------|-----------------------------------|
| 1                    | track and side (note 1)           |
| 1                    | number of sectors on this track   |
| 6\*Number of sectors | Sector ID information (see below) |

Notes:

1.  The track and side are encoded as follows:
    | Bits | Description |
    |------|-------------|
    | 7..1 | track       |
    | 0    | side        |

Sector ID information:

| Length | Description                                          |
|--------|------------------------------------------------------|
| 1      | C from ID field                                      |
| 1      | H from ID field                                      |
| 1      | R from ID field                                      |
| 1      | N from ID field                                      |
| 1      | Bit 1: Sector is filled entirely with a single value |
| 1      | Byte to fill sector                                  |

## Structure of the "DSC" file

The "DSC" file, if it exists, contains the data from one or more sectors.

If the sector is filled with a single value, then there will not be any data for it in the DSC file. Therefore the DSC file contains the data of sectors which are not filled by a single byte. The data in the DSC file is ordered according to the track and sector ordering given in the HDR file.
