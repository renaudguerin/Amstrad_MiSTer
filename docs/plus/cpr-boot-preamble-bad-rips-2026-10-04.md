# CPR bank-0 boot preamble: bad rips in test media (2026-10-04)

## Position

Two CPR files under `local/test_media/cartridges/` (`No Exit_broken.cpr`,
`Pro Tennis Tour_broken.cpr`) crash at boot in the core; their
`01_PlusGames/` counterparts work. The files differ by exactly 5 bytes, all in
the first 6 bytes of the `cb00` chunk data — cartridge bank 0 at
`&0000`–`&0005` — not in the RIFF container. Verdict: the `_broken` files are
bad rips whose boot preamble was mangled by a tool; the working 2015 dumps are
faithful. The loader is content-blind by design and already accepts both, so
no loader change is warranted.

## The differences

Both files are structurally valid, byte-identical containers: `RIFF`/`AMS!`,
8 chunks `cb00`–`cb07` of exactly 0x4000, no padding, valid RIFF length
(131140 = file size − 8). Bank 0 is byte-identical from offset 6 onward.

| | `&0000`–`&0002` | `&0003`–`&0005` |
|---|---|---|
| No Exit working | `31 00 C0` = LD SP,&C000 | `C3 00 01` = JP &0100 |
| No Exit broken | `C3 00 01` = JP &0100 | `41 4D 53` = "AMS" |
| PTT working | `ED 56 00` = IM 1; NOP | `C3 49 00` = JP &0049 |
| PTT broken | `C3 49 00` = JP &0049 | `41 4D 53` = "AMS" |

Corruption signature: the game's entry `JP` (originally at `&0003`) was moved
to `&0000` and the constant string "AMS" stamped at `&0003`–`&0005` — a
botched attempt to impose the documented CPC+ cartridge header. The legitimate
shape of that header is what Pang carries in the working set: `00 10 00`
(NOP; DJNZ +0 — branches to `&0003` either way) + "AMS" (harmless
`LD B,C/LD C,L/LD D,E`) + `JP &0010` at `&0006`. The broken files carry the
stamp without the harmless prefix and with the JP in the wrong slot, destroying
3 bytes of the game's preamble. The identical stamp in both files and the 2005
file dates mark them as the old rip set; the 2015 set is the re-dump.

## Why they crash (core and real hardware)

Boot model: the Z80 starts at `&0000` executing cartridge page 0
(`asic-reference.md` §11 [ARNOLD-REV §2.8]); T80 reset state is SP=0xFFFF,
IM 0 (`rtl/T80/T80.vhd:434`, `:419`).

- **PTT**: `ED 56` is IM 1 and is required. The game runs a real interrupt
  handler at `&0038` (LD HL,&00CE; LD BC,&BC0F; OUT loop; RET) and enables
  interrupts during boot. On real CPC+ hardware IM 0 never yields `&FF`: with
  ASIC features not yet unlocked the 8-bit vector is `&00`, executing a bare
  NOP (Thacker, "Operation of Z80 interrupt mode 0 in the CPC+ design",
  `docs/specs/plus/`, Note 2) — the `&0038` handler never runs and the game
  cannot boot. In the core, the IM 0 acknowledge presents the undriven-bus
  AND-mask (`Amstrad_motherboard.v:296`), likewise not a useful opcode.
- **No Exit**: missing `LD SP,&C000`. Boot runs with SP=0xFFFF, so the early
  `CALL &0081`/`&0092` stack writes land at `&FFxx` (RAM bank 3) instead of
  `&BFFx`. The boot path reconverges at `&0151` (SP re-established to
  `&C000`), so the exact fatal path has a residual: the two stray bytes at
  `&FFxx` if the game keeps state there, or a later bank-0 integrity check.
  The preamble is required boot code either way.

## Why "the working ones are patched" is rejected

1. **Tool fingerprint.** A dump is verbatim copying; the identical constant
   "AMS" stamp at `&0003` in exactly these two files is tool-written, not
   original content.
2. **PTT's preamble is non-generic.** `ED 56 00` (IM 1; NOP) is exactly the
   instruction PTT's `&0038` handler needs; an invented patch would write the
   generic `31 00 C0`. Only the original bytes (or a full RE of PTT's
   interrupt scheme) yield IM 1 — and a RE-capable patcher would have used the
   emulator-side fix (as CPCEC did), not per-file patches.
3. **Amstrad's own carts set the retail style.** The system-cart firmware
   preamble `01 89 7F ED 49 C3 91 05` (LD BC,&7F89; OUT (C),C; JP &0591) is
   identical across `Plus_EN/FR/SP`, `FW316_EN` and `6128_FR`: real functional
   code at `&0000`, no JP+AMS convention. The working game set preserves
   exactly that heterogeneity — including Pang's AMS header, which a patch
   campaign would have normalized.
4. **The flip requires impossible hardware behavior.** For the broken PTT to
   be faithful, retail PTT ships without IM 1 and relies on the IM 0
   floating-bus `&FF` accident — which Thacker shows never happens on the
   CPC+ (`&00`/NOP instead).

Residual: provenance of the physical rips is unknown; the working set's
fidelity is presumed from the above, not proven against the original EPROMs.

## Emulator cross-reference

CPCEC boots these broken rips by pre-initializing the reset state:
`z80_sp.w=0xC000; // implicit in "No Exit" PLUS!` and
`z80_imd=1; // implicit in "Pro Tennis Tour" PLUS!` (reset routine of
`cpcec.c`, `/Users/renaudg/code/cpcec/cpcec.c:2554-2555` at the time of
writing). That masks exactly the two lost instructions emulator-side —
independent confirmation of the diagnosis. It is a compatibility shortcut for
circulating bad rips, not evidence about the machine: the canonical Z80 reset
is IM 0 with SP undefined (0xFFFF on real silicon), which is what this core
keeps.

## Loader implication

No relaxation, and no content validation:

- The parser (`rtl/plus/plus_cpr_parser.v`) never inspects chunk data and
  already accepts both files; the failure is the game's own boot code
  executing. The P0 content-blind policy (`architecture.md`) stays correct.
- Do **not** add a bank-0 header/signature check: the working set proves no
  universal header exists (Pang carries "AMS", most carts don't, Amstrad's own
  firmware uses neither), so any content check would reject legitimate dumps.
- The fix is media-side: use the 2015 dumps, or patch the 6 bytes
  (`31 00 C0` + `C3 00 01` for No Exit; `ED 56 00` + `C3 49 00` for PTT).
