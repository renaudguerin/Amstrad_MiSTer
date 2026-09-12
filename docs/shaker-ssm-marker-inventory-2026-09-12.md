# SHAKER SSM marker inventory, 2026-09-12

Static inventory of the SSM codes in the published SHAKER discs, plus the run-time
mechanism that inventory could not see, plus the portal's code table.

**Revised the same day after an author clarification.** A first pass found no `#FFFE`
in either disc and inferred that SHAKER emits no screenshot markers at all. That
inference was wrong, and correcting it exposed a real bug in the shipped runner. The
record of both readings is kept deliberately: the mistake was a misreading of which
SSM code is the screenshot trigger, and it is an easy one to repeat.

## How it actually works

`#FFFE` is **not** the general screenshot trigger. Per SSM v1.1, "when SSM handling is
enabled in an emulator, it can use the read SSM code to generate a screenshot
immediately after reading the `#HH` byte", and "code `#0000` and all `#FFxx` codes are
reserved". So **every non-reserved code is a screenshot request**, named
`<Emulator>_<CRTC>_<HHLL>.<ext>`. The standard's own examples confirm it: `ED E3 ED 02`
becomes `AMSPIRIT_2_02E3.bmp`. `#FFFE` is only the variant that says "name this one
from the CSL `screenshot_name` instead of from the code".

The author, asked directly:

> Le code FFFE du SSM permet juste au CSL de nommer lui même ses images. Si tu utilises
> les CSL déjà écrits pour SHAKER, ils n'utilisent pas cette possibilité. SHAKER génère
> pour chaque test un code spécifique (voir le fichier EXCEL) et ce code sert à
> numéroter le fichier produit.

So `#FFFE` being absent is expected and harmless. What matters is the per-test codes.

The reserved set is `#0000` plus every `#FFxx`. The standard puts it at 178 values,
which is exactly 1 + the 177 legal values of `LL`, and 177 x 177 = 31,329 matches its
stated combination count. That arithmetic independently corroborates the allowed byte
set in [ssm_marker.v](../rtl/ssm_marker.v).

## Why a static scan sees none of them

The per-test codes are **built at run time by patching a template**, so they are not
in the file as literal bytes. In `SHAKE26B.BIN` (load `&3900`):

```
&A0F4: 21 00 00     LD HL,&0000     ; the immediate doubles as the table pointer
&A0F7: 11 0A A1     LD DE,&A10A     ; -> the LL slot of the template below
&A0FA: ED A0        LDI             ; patch LL
&A0FC: 13           INC DE          ; skip the second #ED
&A0FD: ED A0        LDI             ; patch HH
&A0FF: 11 19 A1     LD DE,&A119
&A102: ED A0 ED A0  LDI / LDI       ; two more bytes into a data slot
&A106: 22 F5 A0     LD (&A0F5),HL   ; advance the table pointer for next time
&A109: ED 00 ED 00  <- the template, now carrying this test's code
&A10D: C9           RET
```

`&A0F5` is read back as a pointer by `LD HL,(&A0F5)` at `&A0E3` and written at `&A0D5`
and `&A106`, so it is a self-advancing cursor over a table of 4-byte records. The
second `ED 00 ED 00` in each module, at `&A128`, is a genuine `#0000` sync emitter
gated by a one-shot flag at `&A121` (`B7` = `OR A` clears carry so `RET NC` returns;
patched to `37` = `SCF` to let it through).

So each module carries **one run-time-patched per-test emitter and one gated `#0000`
sync emitter**, and a static scan sees both as `ED 00 ED 00`. The first pass reported
them as "two sync markers per module", which was the visible shape but not the meaning.

## What the static scan does establish

[shaker_ssm_inventory.py](../scripts/hardware-loop/shaker_ssm_inventory.py) walks the
AMSDOS DATA directory, reassembles each `.BIN` from its extents, strips the 128-byte
header and scans for `ED LL ED HH` with both bytes legal.

```sh
python3 scripts/hardware-loop/shaker_ssm_inventory.py \
  docs/references/Shaker_CSL/shaker26.dsk docs/references/Shaker_CSL/shaker27.dsk
```

| Disc | `ED 00 ED 00` sites | `#FFFE` | `#FFFF` | `#FFFD`/`#FFFC` Sikoview |
|---|---|---|---|---|
| `shaker26.dsk` | 2 per module | 0 | 0 | 0 |
| `shaker27.dsk` | 2 per module | 0 | 0 | 1 + 1 per module except A |

All ten modules reassemble to exactly the length their own AMSDOS header declares, so
the scan covered every byte that executes; the tool exits non-zero on a mismatch, and
an earlier revision of it *did* fail that check on a records-per-sector error. Literal
`ED 00 ED 00` sequences appear, so the modules are not packed. The scan is therefore
complete and correct about **static** markers — it simply cannot see a patched
template, which is the whole point above.

The remaining value of the tool is as a tripwire: if a future disc ever carries
per-test codes as literal bytes, the emitter changed and the guard test says so.

## The portal code table

`docs/references/Shaker_CSL/SHAKER_SCREENSHOT_CODE.xlsx` (user-owned, untracked),
sheet `SNAPSHOT REF`, is the authority on what each code means. Its own header states
`Ref Hexa = YYXX`, confirming `HH*256+LL` as implemented.

- **712 per-test codes**, `0001` to `040C`, with `0000` marked RESERVED.
- Per module: A 127, B 280, C 113, D 170, E 17.
- Columns for CRTC 0/1/2/3/4 mark applicability with `O`/`x`: 483 codes apply to
  CRTC 0 and 478 to CRTC 1, so roughly 480 reference images are reachable per classic
  CRTC type this core supports.
- Each row also carries a test id, a subset, a label and, in 16 rows only, an explicit
  Compendium section reference. The ACCC cross-reference is sparse, not systematic.

Sheet `ED NOT USED` is a 16x16 grid of the unused ED opcodes, but the marking is cell
fill colour rather than text, so it is not machine-readable without parsing styles. Not
pursued: the 177/178/31,329 arithmetic above already corroborates the byte set.

## What this changes

**A bug in the shipped runner, now fixed.** `csl_runner` captured only on `#FFFE` and
would have ignored every real SHAKER marker. It now captures on any non-reserved code,
names it `MISTER_<crtc>_<HHLL>.png`, keeps `#FFFE` as the `screenshot_name` variant,
records other reserved codes without acting, and guards against a repeated code
overwriting an earlier capture.

**Phase 2 has a consumer after all.** The first pass concluded the opposite and gated
the phase on an author question. That gate is lifted: around 480 codes per supported
CRTC type are emitted by the discs we have, so exact frame capture has real work to do.

**Per-screen captures are reachable, and SSM-labelled naming works.** The pessimistic
conclusion that `--screenshot-at LINE` was the only per-screen path is withdrawn.

**The paired-marker case is real and distinct codes make it visible.** Two captures for
a flashing test take the next two table entries, so they arrive as two different codes
rather than two `#FFFE`s — which is why the runner's `state_uncertain` flag keys off
marker proximity in VSYNC periods rather than off the name.

## Still open

- How far apart the paired markers are in time. The record's 64 MHz `tick` measures it
  on the first device run (one VSYNC period is 1.28 M ticks).
- The runtime code sequence per module, which only execution reveals. The table is
  walked with a self-advancing cursor, so the order is the table order.
- Whether to read the xlsx at run time to annotate each capture with its test id and
  CRTC applicability. Worth it as a cross-check — a code arriving that the table says
  does not apply to the selected CRTC would mean the wrong module or CRTC was loaded.

## Provenance

Disc images and the code table are user-owned and gitignored under
`docs/references/`. The tool reads them in place, writes nothing to the repository and
reproduces no disc content or table content; this document records counts, addresses
and summary statistics only.

| File | SHA-256 |
|---|---|
| `shaker26.dsk` | `f7082f8eab521d632c343a288f54038af6df090c59b372e0d2866269c2cc4d08` |
| `shaker27.dsk` | `65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b` |
