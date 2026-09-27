#!/usr/bin/env python3
"""Build a standalone Plus/GX4000 PRI experiment. Python standard library only."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ORIGIN = 0x8000
COUNT, STATUS, RESULTS = 0xB000, 0xB002, 0xB010
CASES = [(100, 25), (10, 25), (10, 34), (255, 25)]
# Original compact 5x7 glyphs, padded to one Mode 2 byte per character.
FONT = {
' ': '00000 00000 00000 00000 00000 00000 00000',
'-': '00000 00000 00000 11111 00000 00000 00000',
'=': '00000 11111 00000 11111 00000 00000 00000',
':': '00000 00100 00100 00000 00100 00100 00000',
'0': '01110 10001 10011 10101 11001 10001 01110',
'1': '00100 01100 00100 00100 00100 00100 01110',
'2': '01110 10001 00001 00010 00100 01000 11111',
'3': '11110 00001 00001 01110 00001 00001 11110',
'4': '00010 00110 01010 10010 11111 00010 00010',
'5': '11111 10000 10000 11110 00001 00001 11110',
'6': '01110 10000 10000 11110 10001 10001 01110',
'7': '11111 00001 00010 00100 01000 01000 01000',
'8': '01110 10001 10001 01110 10001 10001 01110',
'9': '01110 10001 10001 01111 00001 00001 01110',
'A': '01110 10001 10001 11111 10001 10001 10001',
'B': '11110 10001 10001 11110 10001 10001 11110',
'C': '01111 10000 10000 10000 10000 10000 01111',
'D': '11110 10001 10001 10001 10001 10001 11110',
'E': '11111 10000 10000 11110 10000 10000 11111',
'F': '11111 10000 10000 11110 10000 10000 10000',
'G': '01111 10000 10000 10111 10001 10001 01111',
'H': '10001 10001 10001 11111 10001 10001 10001',
'I': '01110 00100 00100 00100 00100 00100 01110',
'J': '00111 00010 00010 00010 00010 10010 01100',
'K': '10001 10010 10100 11000 10100 10010 10001',
'L': '10000 10000 10000 10000 10000 10000 11111',
'M': '10001 11011 10101 10101 10001 10001 10001',
'N': '10001 11001 10101 10011 10001 10001 10001',
'O': '01110 10001 10001 10001 10001 10001 01110',
'P': '11110 10001 10001 11110 10000 10000 10000',
'Q': '01110 10001 10001 10001 10101 10010 01101',
'R': '11110 10001 10001 11110 10100 10010 10001',
'S': '01111 10000 10000 01110 00001 00001 11110',
'T': '11111 00100 00100 00100 00100 00100 00100',
'U': '10001 10001 10001 10001 10001 10001 01110',
'V': '10001 10001 10001 10001 10001 01010 00100',
'W': '10001 10001 10001 10101 10101 10101 01010',
'X': '10001 10001 01010 00100 01010 10001 10001',
'Y': '10001 10001 01010 00100 00100 00100 00100',
'Z': '11111 00001 00010 00100 01000 10000 11111',
}


class Program:
    def __init__(self):
        self.data = bytearray()
        self.labels = {}
        self.fixups = []
        self.listing = []

    def emit(self, text, *values):
        self.listing.append((len(self.data), len(values), text))
        self.data.extend(values)

    def label(self, name):
        self.labels[name] = ORIGIN + len(self.data)
        self.listing.append((len(self.data), 0, name + ':'))

    def word(self, text, opcode, value):
        if isinstance(value, str):
            self.fixups.append((len(self.data) + 1, value))
            encoded = 0
        else:
            encoded = value
        self.emit(text, opcode, encoded & 255, encoded >> 8)

    def ld(self, reg, value):
        self.word(f'LD {reg},{value}', {'BC': 1, 'DE': 0x11, 'HL': 0x21, 'SP': 0x31}[reg], value)

    def store(self, addr, value):
        self.emit(f'LD A,{value:02X}h', 0x3E, value)
        self.word(f'LD ({addr:04X}h),A', 0x32, addr)

    def out(self, value):
        self.emit(f'LD A,{value:02X}h', 0x3E, value)
        self.emit('OUT (C),A', 0xED, 0x79)

    def crtc(self, reg, value):
        self.ld('BC', 0xBC00)
        self.out(reg)
        self.ld('BC', 0xBD00)
        self.out(value)

    def jump(self, name, opcode=0xC3):
        self.word({0xC3: 'JP', 0xCA: 'JP Z,', 0xC2: 'JP NZ,', 0xCD: 'CALL'}[opcode] + ' ' + name, opcode, name)

    def fill(self, base, length, value):
        self.store(base, value)
        self.ld('HL', base)
        self.ld('DE', base + 1)
        self.ld('BC', length - 1)
        self.emit('LDIR', 0xED, 0xB0)


def build(destination):
    p = Program()
    p.word('JP 0100h ; cold boot in cartridge ROM', 0xC3, 0x100)
    p.emit('Boot padding', *bytes(0x100 - len(p.data)))
    p.label('boot_at_rom_0100')
    p.emit('DI', 0xF3)
    p.ld('SP', 0xBFF0)
    p.ld('BC', 0x7F00)
    p.out(0xC0)  # Base RAM bank configuration.
    p.out(0x8A)  # Mode 2, upper ROM off, lower cartridge ROM still readable.
    p.ld('HL', 0)
    p.ld('DE', ORIGIN)
    p.ld('BC', 0x4000)
    p.emit('LDIR ; relocate entire cartridge bank to RAM', 0xED, 0xB0)
    p.jump('ram_entry')
    p.label('ram_entry')
    p.ld('BC', 0x7F00)
    p.out(0x8E)  # Both ROMs off; IM1 reads the RAM vector.
    p.ld('BC', 0xF700)
    p.out(0x82)  # PPI mode 0: port B input, A/C output.
    p.ld('BC', 0xF600)
    p.out(0)  # PSG inactive, tape motor off.
    p.ld('BC', 0xBC00)
    for value in [255, 0, 255, 0x77, 0xB3, 0x51, 0xA8, 0xD4, 0x62, 0x39, 0x9C, 0x46, 0x2B, 0x15, 0x8A, 0xCD, 0xEE]:
        p.out(value)
    p.ld('BC', 0x7F00)
    p.out(0xB8)  # ASIC register page at 4000-7FFF.
    p.out(0x8E)
    p.store(0x6C0F, 0x70)  # Disable all DMA; clear all DMA interrupt flags.
    for sprite in range(16):
        p.store(0x6004 + 8 * sprite, 0)  # Disable magnification without ASIC readback.
    for addr, value in [(0x6800, 100), (0x6801, 0), (0x6802, 0), (0x6803, 0), (0x6804, 0), (0x6805, 1),
                        (0x6400, 0), (0x6401, 0), (0x6402, 255), (0x6403, 15), (0x6420, 0), (0x6421, 0)]:
        p.store(addr, value)
    for reg, value in enumerate([63, 40, 49, 0x0E, 38, 0, 25, 30, 0, 7, 0, 0, 0x30, 0, 0, 0]):
        p.crtc(reg, value)
    p.fill(COUNT, 0x20, 0)
    p.fill(0xC000, 0x4000, 0)
    p.ld('HL', 'isr')
    p.ld('DE', 0x38)
    p.ld('BC', 14)
    p.emit('LDIR ; install IM1 handler in low RAM', 0xED, 0xB0)
    p.emit('IM 1', 0xED, 0x56)
    lines = [
        (2, 'PLUS PRI ALIAS PROBE V1'),
        (4, 'COUNTS OVER 32 COMPLETE VSYNC INTERVALS'),
        (6, 'PRI DEC   R6 DEC   IRQ COUNT HEX'),
        (8, '100       25       ----'),
        (10, '10        25       ----'),
        (12, '10        34       ----'),
        (14, '255       25       ----'),
        (17, '0020 HEX = 32 IRQ   0040 HEX = 64 IRQ'),
        (19, 'PHOTOGRAPH ALL FOUR RESULTS'),
        (21, 'RUNNING - PLEASE WAIT'),
    ]
    for index, (row, _) in enumerate(lines):
        p.ld('HL', f'text_{index}')
        p.ld('DE', 0xC000 + row * 80 + 6)
        p.jump('print', 0xCD)
    for index, (pri, r6) in enumerate(CASES):
        p.store(STATUS, index + 1)
        p.crtc(6, r6)
        p.store(0x6800, pri)
        p.jump('measure', 0xCD)
        p.word('LD HL,(count)', 0x2A, COUNT)
        p.word(f'LD (result_{index}),HL', 0x22, RESULTS + 2 * index)
    p.crtc(6, 25)  # Restore readable screen only AFTER the tall case measurement.
    for index, row in enumerate([8, 10, 12, 14]):
        p.ld('DE', 0xC000 + row * 80 + 6 + 19)
        for addr in [RESULTS + index * 2 + 1, RESULTS + index * 2]:
            p.word(f'LD A,({addr:04X}h)', 0x3A, addr)
            p.jump('hex_byte', 0xCD)
    p.ld('HL', 'done_text')
    p.ld('DE', 0xC000 + 21 * 80 + 6)
    p.jump('print', 0xCD)
    p.store(STATUS, 0x80)
    p.label('finished')
    p.jump('finished')

    p.label('measure')
    p.emit('DI ; configure and settle with IRQs masked', 0xF3)
    p.jump('vsync_rising', 0xCD)
    p.jump('vsync_rising', 0xCD)
    p.emit('EI / NOP / DI ; drain pending PRI through IM1 before zeroing count', 0xFB, 0x00, 0xF3)
    p.ld('HL', 0)
    p.word('LD (count),HL', 0x22, COUNT)
    p.ld('BC', 0x7F00)
    p.out(0x9E)  # Reset classic IRQ state; pending PRI was drained through IM1 above.
    p.emit('LD D,32 ; number of whole frame intervals', 0x16, 32)
    p.emit('EI', 0xFB)
    p.label('count_frames')
    p.jump('vsync_rising', 0xCD)
    p.emit('DEC D', 0x15)
    p.jump('count_frames', 0xC2)
    p.emit('DI ; no tested raster line coincides with VSYNC rising', 0xF3)
    p.emit('RET', 0xC9)

    p.label('vsync_rising')
    p.ld('BC', 0xF500)
    p.label('wait_low')
    p.emit('IN A,(C)', 0xED, 0x78)
    p.emit('AND 1', 0xE6, 1)
    p.jump('wait_low', 0xC2)
    p.label('wait_high')
    p.emit('IN A,(C)', 0xED, 0x78)
    p.emit('AND 1', 0xE6, 1)
    p.jump('wait_high', 0xCA)
    p.emit('RET', 0xC9)

    p.label('isr')
    start = len(p.data)
    p.emit('PUSH AF', 0xF5)
    p.emit('PUSH HL', 0xE5)
    p.word('LD HL,(count)', 0x2A, COUNT)
    p.emit('INC HL', 0x23)
    p.word('LD (count),HL', 0x22, COUNT)
    p.emit('POP HL', 0xE1)
    p.emit('POP AF', 0xF1)
    p.emit('EI', 0xFB)
    p.emit('RETI', 0xED, 0x4D)
    assert len(p.data) - start == 14
    isr_length = len(p.data) - start

    p.label('print')
    p.emit('LD A,(HL)', 0x7E)
    p.emit('OR A', 0xB7)
    p.emit('RET Z', 0xC8)
    p.emit('INC HL', 0x23)
    p.emit('PUSH HL', 0xE5)
    p.jump('glyph', 0xCD)
    p.emit('POP HL', 0xE1)
    p.jump('print')
    p.label('hex_byte')
    p.emit('PUSH AF', 0xF5)
    p.emit('RRCA / RRCA / RRCA / RRCA', 0x0F, 0x0F, 0x0F, 0x0F)
    p.jump('hex_digit', 0xCD)
    p.emit('POP AF', 0xF1)
    p.label('hex_digit')
    p.emit('AND 0Fh', 0xE6, 15)
    p.ld('HL', 'hex_chars')
    p.emit('LD C,A / LD B,0 / ADD HL,BC / LD A,(HL)', 0x4F, 6, 0, 9, 0x7E)
    p.label('glyph')
    p.emit('SUB 32 / LD L,A / LD H,0', 0xD6, 32, 0x6F, 0x26, 0)
    p.emit('ADD HL,HL / ADD HL,HL / ADD HL,HL', 0x29, 0x29, 0x29)
    p.ld('BC', 'font')
    p.emit('ADD HL,BC / PUSH DE / LD B,8', 9, 0xD5, 6, 8)
    p.label('glyph_row')
    p.emit('LD A,(HL) / LD (DE),A / INC HL', 0x7E, 0x12, 0x23)
    p.emit('LD A,D / ADD A,8 / LD D,A / DEC B', 0x7A, 0xC6, 8, 0x57, 5)
    p.jump('glyph_row', 0xC2)
    p.emit('POP DE / INC DE / RET', 0xD1, 0x13, 0xC9)

    for index, (_, line) in enumerate(lines):
        p.label(f'text_{index}')
        p.emit(repr(line), *line.encode('ascii'), 0)
    p.label('done_text')
    p.emit('Final status text', *b'DONE - RESULTS READY \x00')
    p.label('hex_chars')
    p.emit('Hex alphabet', *b'0123456789ABCDEF')
    p.label('font')
    for char in map(chr, range(32, 91)):
        p.emit(f'Glyph {char!r}', *[int(row, 2) << 2 for row in FONT.get(char, FONT[' ']).split()], 0)
    for offset, name in p.fixups:
        p.data[offset:offset + 2] = struct.pack('<H', p.labels[name])
    assert len(p.data) < COUNT - ORIGIN, 'Code/font must not overlap count/result RAM'
    bank = bytes(p.data) + bytes(0x4000 - len(p.data))
    payload = b'AMS!cb00' + struct.pack('<I', len(bank)) + bank
    cpr = b'RIFF' + struct.pack('<I', len(payload)) + payload
    assert cpr[8:16] == b'AMS!cb00' and len(cpr) == 16404
    assert struct.unpack_from('<I', cpr, 4)[0] == len(cpr) - 8
    assert bank[:3] == b'\xc3\x00\x01' and bank[0x100] == 0xF3
    destination.mkdir(parents=True, exist_ok=True)
    (destination / 'pri-alias-probe.cpr').write_bytes(cpr)
    (destination / 'pri-alias-probe.bin').write_bytes(bank)
    listing = ['; Runtime addresses except boot block (ROM 0100h = listed 8100h).']
    for offset, length, text in p.listing:
        chunk = p.data[offset:offset + length]
        listing.append(f'{ORIGIN + offset:04X}  {chunk.hex(" ")[:47]:47s}  {text}')
    (destination / 'pri-alias-probe.lst').write_text('\n'.join(listing) + '\n')
    metadata = dict(sha256=hashlib.sha256(cpr).hexdigest(), program_bytes=len(p.data),
                    labels=p.labels, count=COUNT, status=STATUS, results=RESULTS,
                    cases=CASES, intervals=32, isr_length=isr_length)
    (destination / 'pri-alias-probe.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'{destination / "pri-alias-probe.cpr"}: sha256={metadata["sha256"]}')
    return metadata


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=Path(__file__).resolve().parents[2] / 'output_files/pri-alias-probe')
    build(parser.parse_args().output_dir)
