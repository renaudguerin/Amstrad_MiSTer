#!/usr/bin/env python3
"""CPC character/key translation for the CSL runner.

Three layers sit between a CSL ``key_output 'text'`` string and the bytes MBC
injects through ``/dev/uinput``:

1. A ROM keyboard layout maps each character to a CPC key position plus a SHIFT
   flag.  The character printed by a given key position is decided by the
   machine's ROM, not by the host, so there is one table per ROM (``uk``,
   ``fr``).
2. ``CPC_KEY_TO_LINUX`` maps a CPC key position to the Linux keycode that
   reaches it.  MBC emits the keycode, Main converts it to a PS/2 set-2 code,
   and ``rtl/hid.sv`` decodes that into the CPC keyboard matrix.  The table
   below was derived by walking ``rtl/hid.sv`` backwards through the standard
   PS/2 set-2 encoding of each Linux keycode.
3. ``translate_text`` turns the character stream into MBC ``raw_seq`` tokens.

Fifteen of these keycodes are confirmed against real hardware: the
``RUN"SHAKE27B`` + Enter and menu-key sequences captured in
``docs/b2-device-capture-2026-09-12.md`` decode to exactly the entries here.

Keypad positions are deliberately absent.  ``hid.sv`` routes the non-extended
keypad scancodes to the CPC function keys unless the OSD ``Keypad`` option is
set to ``Symbols`` (status bit 23), so a keypad keycode would mean two different
CPC keys depending on a setting the script never names.
"""

from __future__ import annotations

from typing import Dict, List, Optional, Tuple

SUPPORTED_LAYOUTS = ("uk", "fr")

# CPC key position -> Linux keycode (linux/input-event-codes.h).
# Position names use the UK CPC legend; the comment gives the hid.sv matrix cell.
CPC_KEY_TO_LINUX: Dict[str, int] = {
    "UP": 103,          # key[0][0]
    "RIGHT": 106,       # key[0][1]
    "DOWN": 108,        # key[0][2]
    "F9": 67,           # key[0][3]
    "F6": 64,           # key[0][4]
    "F3": 61,           # key[0][5]
    "ENTER": 107,       # key[0][6]  numeric-pad ENTER, reached via Linux END
    "LEFT": 105,        # key[1][0]
    "COPY": 110,        # key[1][1]  reached via Linux INSERT
    "F7": 65,           # key[1][2]
    "F8": 66,           # key[1][3]
    "F5": 63,           # key[1][4]
    "F1": 59,           # key[1][5]
    "F2": 60,           # key[1][6]
    "F0": 68,           # key[1][7]  reached via Linux F10
    "CLR": 111,         # key[2][0]  reached via Linux DELETE
    "[": 27,            # key[2][1]  reached via Linux RIGHTBRACE
    "RETURN": 28,       # key[2][2]
    "]": 43,            # key[2][3]  reached via Linux BACKSLASH
    "F4": 62,           # key[2][4]
    "SHIFT": 42,        # key[2][5]  Linux LEFTSHIFT (unconditional; RIGHTSHIFT
                        #            depends on the Right Shift OSD option)
    "\\": 86,           # key[2][6]  reached via Linux 102ND
    "CTRL": 97,         # key[2][7]  Linux RIGHTCTRL; LEFTCTRL is unmapped in hid.sv
    "^": 13,            # key[3][0]  reached via Linux EQUAL
    "-": 12,            # key[3][1]
    "@": 26,            # key[3][2]  reached via Linux LEFTBRACE
    "P": 25,            # key[3][3]
    ";": 40,            # key[3][4]  reached via Linux APOSTROPHE
    ":": 39,            # key[3][5]  reached via Linux SEMICOLON
    "/": 53,            # key[3][6]
    ".": 52,            # key[3][7]
    "0": 11,            # key[4][0]
    "9": 10,            # key[4][1]
    "O": 24,            # key[4][2]
    "I": 23,            # key[4][3]
    "L": 38,            # key[4][4]
    "K": 37,            # key[4][5]
    "M": 50,            # key[4][6]
    ",": 51,            # key[4][7]
    "8": 9,             # key[5][0]
    "7": 8,             # key[5][1]
    "U": 22,            # key[5][2]
    "Y": 21,            # key[5][3]
    "H": 35,            # key[5][4]
    "J": 36,            # key[5][5]
    "N": 49,            # key[5][6]
    "SPACE": 57,        # key[5][7]
    "6": 7,             # key[6][0]
    "5": 6,             # key[6][1]
    "R": 19,            # key[6][2]
    "T": 20,            # key[6][3]
    "G": 34,            # key[6][4]
    "F": 33,            # key[6][5]
    "B": 48,            # key[6][6]
    "V": 47,            # key[6][7]
    "4": 5,             # key[7][0]
    "3": 4,             # key[7][1]
    "E": 18,            # key[7][2]
    "W": 17,            # key[7][3]
    "S": 31,            # key[7][4]
    "D": 32,            # key[7][5]
    "C": 46,            # key[7][6]
    "X": 45,            # key[7][7]
    "1": 2,             # key[8][0]
    "2": 3,             # key[8][1]
    "ESC": 1,           # key[8][2]
    "Q": 16,            # key[8][3]
    "TAB": 15,          # key[8][4]
    "A": 30,            # key[8][5]
    "CAPSLOCK": 58,     # key[8][6]
    "Z": 44,            # key[8][7]
    "DEL": 14,          # key[9][7]  reached via Linux BACKSPACE
}

# CSL \(XXX) escape -> CPC key position (CSL v1.4 annex "Specific key coding").
# \(KOF) is handled separately: it is a timing marker, not a key.
CSL_SPECIAL_KEYS: Dict[str, str] = {
    "ESC": "ESC",
    "TAB": "TAB",
    "CAP": "CAPSLOCK",
    "SHI": "SHIFT",
    "CTR": "CTRL",
    "COP": "COPY",
    "CLR": "CLR",
    "DEL": "DEL",
    "RET": "RETURN",
    "ENT": "ENTER",
    "ARL": "LEFT",
    "ARR": "RIGHT",
    "ARU": "UP",
    "ARD": "DOWN",
    "FN0": "F0",
    "FN1": "F1",
    "FN2": "F2",
    "FN3": "F3",
    "FN4": "F4",
    "FN5": "F5",
    "FN6": "F6",
    "FN7": "F7",
    "FN8": "F8",
    "FN9": "F9",
}

KOF = "KOF"

# Letters that keep their UK position on a French CPC keyboard are omitted from
# the swap table below.
_FR_LETTER_SWAPS = {"A": "Q", "Q": "A", "Z": "W", "W": "Z", "M": ";"}

# Characters the UK CPC prints when the key is held with SHIFT.
_UK_SHIFTED = {
    "!": "1", '"': "2", "#": "3", "$": "4", "%": "5", "&": "6", "'": "7",
    "(": "8", ")": "9", "_": "0", "=": "-", "£": "^", "|": "@", "+": ";",
    "*": ":", "?": "/", ">": ".", "<": ",", "{": "[", "}": "]", "`": "\\",
}

# Characters the French CPC prints unshifted on the digit row.  The digits
# themselves need SHIFT, which is how the B2 capture typed "27" as
# {SHIFT 2 7}.  See docs/b2-device-capture-2026-09-12.md.
_FR_DIGIT_ROW_UNSHIFTED = {
    "&": "1", "é": "2", '"': "3", "'": "4", "(": "5", "§": "6",
    "è": "7", "!": "8", "ç": "9", "à": "0",
}


def _build_layout(layout: str) -> Dict[str, Tuple[str, bool]]:
    """Return the character -> (CPC key position, shift) table for one ROM."""
    table: Dict[str, Tuple[str, bool]] = {" ": ("SPACE", False)}

    for letter in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        position = _FR_LETTER_SWAPS.get(letter, letter) if layout == "fr" else letter
        # The CPC powers on with CAPS LOCK active, so an unshifted letter key
        # prints uppercase.  Lowercase is intentionally absent: producing it
        # would need the runner to track and toggle the CAPS state.
        table[letter] = (position, False)

    if layout == "uk":
        for digit in "0123456789":
            table[digit] = (digit, False)
        for key in ("-", "^", "@", ";", ":", "/", ".", ",", "[", "]", "\\"):
            table[key] = (key, False)
        for char, position in _UK_SHIFTED.items():
            table[char] = (position, True)
    else:
        for digit in "0123456789":
            table[digit] = (digit, True)
        for char, position in _FR_DIGIT_ROW_UNSHIFTED.items():
            table[char] = (position, False)

    return table


LAYOUTS: Dict[str, Dict[str, Tuple[str, bool]]] = {
    name: _build_layout(name) for name in SUPPORTED_LAYOUTS
}


class KeyTranslationError(ValueError):
    """A character or escape in a key_output string has no mapping."""


class KeyGroup:
    """One CSL keystroke unit: a single key, or a braced chord.

    ``kof`` records that the script asked for no delay before the next group.
    """

    __slots__ = ("positions", "shift", "kof", "source")

    def __init__(self, positions: List[str], shift: bool, source: str):
        self.positions = positions
        self.shift = shift
        self.kof = False
        self.source = source

    def keycodes(self) -> List[int]:
        codes = [CPC_KEY_TO_LINUX["SHIFT"]] if self.shift else []
        codes.extend(CPC_KEY_TO_LINUX[p] for p in self.positions)
        return codes

    def tokens(self) -> str:
        """MBC raw_seq fragment for this group on its own.

        Prefer ``sequence_tokens`` for a whole key_output string: it holds
        SHIFT across a run of shifted keys the way a typist does, which is the
        form confirmed on hardware in docs/b2-device-capture-2026-09-12.md.
        """
        return sequence_tokens([self])

    def __repr__(self) -> str:  # pragma: no cover - debugging aid
        return f"KeyGroup({self.source!r}, shift={self.shift}, kof={self.kof})"


def sequence_tokens(groups: List["KeyGroup"]) -> str:
    """Build one MBC raw_seq for an ordered run of key groups.

    SHIFT is pressed once and held across consecutive shifted groups rather
    than tapped per key: that is what the hardware-verified B2 sequence does
    ({2A:03:08}2A for "27"), and it halves the uinput events MBC has to emit.
    A single key inside the hold is a tap (:HH); a chord holds each of its keys
    so they overlap, then releases them in reverse order.
    """
    shift_code = CPC_KEY_TO_LINUX["SHIFT"]
    out: List[str] = []
    shift_held = False
    for group in groups:
        if group.shift and not shift_held:
            out.append("{%02X" % shift_code)
            shift_held = True
        elif not group.shift and shift_held:
            out.append("}%02X" % shift_code)
            shift_held = False
        codes = [CPC_KEY_TO_LINUX[p] for p in group.positions]
        if len(codes) == 1:
            out.append(":%02X" % codes[0])
        else:
            out.extend("{%02X" % c for c in codes)
            out.extend("}%02X" % c for c in reversed(codes))
    if shift_held:
        out.append("}%02X" % shift_code)
    return "".join(out)


def _lex(text: str) -> List[object]:
    """Split a key_output payload into characters, escapes and chord markers.

    Escapes are returned as ``("escape", name)`` tuples, chord delimiters as
    the bare strings ``"{"`` and ``"}"``, everything else as single characters.
    """
    out: List[object] = []
    idx = 0
    while idx < len(text):
        char = text[idx]
        if char == "\\":
            if idx + 1 >= len(text) or text[idx + 1] != "(":
                raise KeyTranslationError(
                    rf"backslash at index {idx} is not the start of a \(XXX) escape"
                )
            end = text.find(")", idx + 2)
            if end < 0:
                raise KeyTranslationError(f"unterminated \\( escape at index {idx}")
            out.append(("escape", text[idx + 2 : end]))
            idx = end + 1
        elif char in "{}":
            out.append(char)
            idx += 1
        else:
            out.append(char)
            idx += 1
    return out


def translate_text(text: str, layout: str) -> List[KeyGroup]:
    """Translate a CSL key_output payload into ordered key groups.

    Raises KeyTranslationError for any character or escape without a mapping.
    CSL says an emulator may silently skip unknown characters; this runner
    refuses instead, because a skipped character turns into a SHAKER menu that
    never advanced and a capture labelled with the wrong test.
    """
    if layout not in LAYOUTS:
        raise KeyTranslationError(f"unknown keyboard layout {layout!r}")
    table = LAYOUTS[layout]

    groups: List[KeyGroup] = []
    chord: Optional[List[Tuple[str, bool]]] = None
    chord_source = ""

    def resolve(item: object, index: int) -> Tuple[str, bool, str]:
        if isinstance(item, tuple):
            name = item[1]
            if name == KOF:
                raise KeyTranslationError(r"\(KOF) is a timing marker, not a key")
            if name not in CSL_SPECIAL_KEYS:
                raise KeyTranslationError(rf"unknown CSL escape \({name}) at index {index}")
            return CSL_SPECIAL_KEYS[name], False, rf"\({name})"
        char = str(item)
        if char not in table:
            raise KeyTranslationError(
                f"character {char!r} at index {index} has no {layout} CPC key mapping"
            )
        position, shift = table[char]
        return position, shift, char

    items = _lex(text)
    for index, item in enumerate(items):
        if isinstance(item, tuple) and item[1] == KOF:
            if chord is not None:
                raise KeyTranslationError(r"\(KOF) cannot appear inside a {chord}")
            if not groups:
                raise KeyTranslationError(r"\(KOF) appeared before any key")
            groups[-1].kof = True
            continue

        if item == "{":
            if chord is not None:
                raise KeyTranslationError(f"nested '{{' at index {index}")
            chord, chord_source = [], "{"
            continue

        if item == "}":
            if chord is None:
                raise KeyTranslationError(f"unmatched '}}' at index {index}")
            if not chord:
                raise KeyTranslationError(f"empty chord at index {index}")
            positions = [p for p, s in chord]
            shift = any(s for _, s in chord)
            if shift and "SHIFT" in positions:
                positions = [p for p in positions if p != "SHIFT"]
            groups.append(KeyGroup(positions, shift, chord_source + "}"))
            chord = None
            continue

        position, shift, source = resolve(item, index)
        if chord is None:
            groups.append(KeyGroup([position], shift, source))
        else:
            chord.append((position, shift))
            chord_source += source

    if chord is not None:
        raise KeyTranslationError("unterminated '{' chord")
    return groups


def coverage_report(texts: List[str], layout: str) -> Dict[str, List[str]]:
    """Return {'ok': [...], 'unmappable': ['<item>: <reason>', ...]} for a corpus."""
    ok: List[str] = []
    bad: List[str] = []
    for text in texts:
        try:
            translate_text(text, layout)
        except KeyTranslationError as exc:
            bad.append(f"{text!r}: {exc}")
        else:
            ok.append(text)
    return {"ok": ok, "unmappable": bad}
