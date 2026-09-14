<!--
Converted from "Tape-Image (.CDT) file format (Amstrad specific).html" on 2026-08-26 using pandoc (HTML → GFM).
Source: cpctech.org.uk — Kevin Thacker's CPC documentation archive.
-->

# Tape-Image (.CDT) file format

The ".CDT" (CPC Digital Tape) tape image format is the same as the TZX image format.

This document gives some additional information relating to creating CDTs, or how they should be supported by an Amstrad emulator.

NOTE: All timings are in Spectrum T-States.

### ID:10 - Standard speed data block

This block MUST be supported and CAN exist in a CDT. Emulators should use standard Spectrum ROM timings for playback.

### ID:11 - Turbo Loading Data Block

This block MUST be supported and CAN exist in a CDT.

The timings for playback are stored in the block header.

Details and functionality of this block are the same as described in the official TZX/CDT specification.

### ID:13 - Sequence of pulses of different length

This block MUST be supported and CAN exist in a CDT.

The timings for playback are stored in the block header.

Details and functionality of this block are the same as described in the official TZX/CDT specification.

### ID:14 - Pure Data Block

This block MUST be supported and CAN exist in a CDT.

The timings for playback are stored in the block header.

Details and functionality of this block are the same as described in the official TZX/CDT specification.

### ID:15 - Direct Recording

This block MUST be supported but SHOULD be avoided when creating a CDT by a sample-to-CDT converter. This block can be used by emulators to support writing to CDTs.

The timings for playback are stored in the block header.

Details and functionality of this block are the same as described in the official TZX/CDT specification.

### ID : 20 - Pause (Silence) or \`Stop the Tape' Command

When the pause is defined as "0" an Amstrad emulator SHOULD NOT 'Stop the Tape', but this value should be treated as "no pause".

### ID:2A - Stop tape if in 48K Mode

This block is Spectrum specific.

This block MUST NOT be added to a new CDT. Amstrad emulator's MUST ignore this block, and it MUST NOT have any effect. There will not be a pause, and the tape must not stop.

### ID: 33 - Hardware Type

Hardware types of 0x01 (External Storage) to 0x0f (EPROM programmers) MUST not be added to a new CDT. These types ONLY apply to the Spectrum. Hardware type 0x00 (Computers) CAN be used but only as a guideline.

### ID:34 - Emulation Info

This block is Spectrum specific.

This block MUST not be added to a new CDT. Amstrad emulator's MUST ignore this block.

### ID:40 - Snapshot block

This block is Spectrum specific.

This block MUST not be added to a new CDT. Amstrad emulator's MUST ignore this block.
