# SHAKER SSM marker inventory, 2026-09-12

Static inventory of the SSM codes the published SHAKER discs actually contain.
Taken because the B4 phase 1 acceptance gate assumed `#FFFE` screenshot markers exist
in the corpus, and nothing had checked.

**Headline: neither `shaker26.dsk` nor `shaker27.dsk` contains a single `#FFFE`
screenshot marker, or a single `#FFFF` snapshot marker.** What they contain is two
`#0000` sync markers per module, and in 2.7 a Sikoview logging pair. The `#FFFE`
capture path built in phase 1 is therefore correct per the standard but has nothing on
the available media to trigger it, and the phase 1 acceptance gate as written cannot
be met.

## Method

[shaker_ssm_inventory.py](../scripts/hardware-loop/shaker_ssm_inventory.py) reads the
EDSK image, walks the AMSDOS DATA-format directory, reassembles each `.BIN` from its
extents, strips the 128-byte AMSDOS header and scans the body for `ED LL ED HH` where
both bytes are legal SSM values — the same byte set as
[ssm_marker.v](../rtl/ssm_marker.v).

```sh
python3 scripts/hardware-loop/shaker_ssm_inventory.py \
  docs/references/Shaker_CSL/shaker26.dsk docs/references/Shaker_CSL/shaker27.dsk
```

Two properties make the result trustworthy rather than merely plausible:

- **The reassembly is self-checking.** Each AMSDOS binary header declares its own
  length. All ten modules reassemble to exactly their declared length, so no module
  was truncated or mis-ordered and the scan covered every byte that executes. The
  tool exits non-zero on any mismatch, and an earlier revision of it *did* mismatch
  (a records-per-sector error halved the data) — that is what the check caught.
- **The code is not packed.** Literal `ED 00 ED 00` byte sequences appear in the
  images, which they could not if the modules were compressed and unpacked at
  runtime. So absence of `#FFFE` in the file is absence at execution too, for any
  marker present as static code.

The one thing this method cannot see is a marker **written into RAM at run time**.
See "What this does not rule out".

## Findings

| Disc | `#0000` sync | `#FFFE` screenshot | `#FFFF` snapshot | `#FFFD`/`#FFFC` Sikoview |
|---|---|---|---|---|
| `shaker26.dsk` | 10 (2 per module) | **0** | **0** | 0 |
| `shaker27.dsk` | 10 (2 per module) | **0** | **0** | 4 + 4 (all but module A) |

Whole-image scans, independent of the filesystem parsing, agree on the absence: 426
`ED ?? ED ??` occurrences in 2.6 of which 30 are SSM-legal and all are `#0000`; 447 in
2.7 of which 58 are legal, being `#0000`, `#FFFC` and `#FFFD` only. The excess
whole-image counts over the per-module counts are copies in loader or free sectors and
do not change the conclusion.

**The two `#0000` sites per module are not one per test screen.** In `SHAKE26B.BIN`
they sit at `&A109` and `&A128`, 31 bytes apart, bracketing a short region whose two
entry points `&A11B` and `&A121` are each `CALL`ed twice. That reads as a
module-level handshake, not a per-screen signal. Runtime frequency is low and is not
determined by a static scan.

## What this changes

**The phase 1 acceptance gate is unachievable as written.** "Each `FFFE` produced a
named capture" cannot happen against these discs. Rewritten in
[the plan](csl-ssm-implementation-plan.md).

**Phase 2 has no consumer yet.** Exact frame capture exists to serve `#FFFE`. Until a
SHAKER build that emits `#FFFE` is known to exist and be obtainable, building it is
speculative work. This is now the precondition on that phase, ahead of the frame
semantics the author already settled.

**Neither SSM code gives per-screen captures on the published discs.** `#FFFE` is
absent and `#0000` is sparse and module-level. So `--screenshot-at [SCRIPT:]LINE`,
shipped in phase 0, remains the only working per-screen capture path, and the
SSM-labelled naming that motivated the whole item is not currently reachable. That is
the single most important fact for planning the next phase.

**`wait_ssm0000` is still worth having, but it is a coarse handshake.** It is
implemented and the discs do emit the marker; no bundled CSL script uses it. A
purpose-written CSL script could use it to synchronise module start instead of a
fixed `wait`, which is strictly better than guessing, just not a per-screen solution.

**The runner already behaves correctly on what does appear.** `#FFFD` and `#FFFC` are
recorded in the manifest and acted on by nothing, which is right: they are the
author's own code-inspector markers.

## What this does not rule out

The SSM standard says of the mechanism that "cette gestion peut être conditionnelle
(ou compilée exprès lors des tests)" — it can be conditional, or compiled in
deliberately for testing. So a **SHAKER build compiled with `#FFFE` markers** may
exist and may be what produced the SHAKERLAND reference images. A static scan of the
public discs cannot see it, and a marker constructed in RAM at run time would also be
invisible here.

This is the question to put to the author, and it gates phase 2 entirely:

> The public `shaker26.dsk` and `shaker27.dsk` emit `#0000` twice per module, plus
> `#FFFD`/`#FFFC` in 2.7, but no `#FFFE` at all. How were the SHAKERLAND reference
> images produced — from a build compiled with `#FFFE` markers, or by CSL
> `wait_ssm0000` plus `screenshot`? If such a build exists, is it obtainable?

Until it is answered, treat `#FFFE` support as implemented-and-idle.

## Reproduction and provenance

Disc images are user-owned and gitignored under `docs/references/Shaker_CSL/`. The
tool reads them in place, writes nothing to the repository and reproduces no disc
content; this document records counts and addresses only.

| File | SHA-256 |
|---|---|
| `shaker26.dsk` | `f7082f8eab521d632c343a288f54038af6df090c59b372e0d2866269c2cc4d08` |
| `shaker27.dsk` | `65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b` |
