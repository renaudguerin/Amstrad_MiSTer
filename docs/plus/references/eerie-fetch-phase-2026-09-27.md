# Eerie Forest: current-master fetch and pending-request discriminator

Investigation base: `2f409e94e470876044ebe0ce8cad52aeb5bb4549`, including
the FF2 current-line PRI-write repair. This investigation preserves the
accepted IRQ, ACK, CRTC3 and sprite changes. It changes no production RTL
or committed simulation code.

## Current-master reproduction

The archived production-T80 fixture was regenerated from this checkout's
RTL, with extra read-only ports for fetch, serializer and bus observations.
It used 6128+, Full sync, production 64 MHz clocking and READY, and the
unchanged original CPR (SHA-256
`72485083d485e89e16c8367e6aceabe98c65a5511c3ccc0218ced02b24053215`).
The current parser accepts this known malformed outer RIFF length; the
cartridge payload is unchanged. The replay completed eight simulated
seconds, 512 million post-load master ticks, in 311.392 wall seconds:
`END frames=401`. This is a title diagnostic, not a selected-suite gate.

At frame 400, all 560 complete pixel records in rows 224–230, x16–95
match the archived pre-sprite-repair frame 400 exactly. Each record includes
RGB, palette/sprite RGB, MA, assembled video word, SSA, flags, raw/delayed
pens, SSCR, palette/sprite indices, mode, RA and SPLT. The left-edge residue
is therefore unchanged in this compared region on current master. The
rendered frame was also inspected. At frame 325, each of the three repaired
reveal rows (65, 108 and 159) has zero lit pixels across x16–639.

### Fetch-to-pixel chain on row 228

Coordinates are `x=C0*16+serializer_dot`, not monitor crop coordinates.
Events below are settled observations before master edge `tick`; values
are hexadecimal except ticks and x. VRAM addresses are **word addresses**.

| Tick | x | Observation |
|---:|---:|---|
| 510829272 | 5 | VRAM address becomes `185D`, using RA6 |
| 510829283 | 8 | Returned word becomes `0A0A` |
| 510829315 | 16 | Even serializer byte becomes `0A`; first-character mask active |
| 510829347 | 24 | Odd serializer byte becomes `0A`; next returned word is `0A07` |
| 510829379 | 32 | SSCR write onset; mask ends; even byte becomes `07` from old-RA data |
| 510829380 | 32 | SSCR `AC→8C`, effective RA `6→4` |
| 510829400 | 37 | VRAM address becomes `105F`, now using RA4 |
| 510829411 | 40 | Returned word becomes `0000` |
| 510829423 | 43 | Assembled word's low byte becomes zero |
| 510829443 | 48 | Even serializer byte becomes zero; assembled word becomes `0000` |
| 510829475 | 56 | Odd serializer byte becomes zero |

The first unmasked pen at x32 is pen 2 decoded at x20 from the already
latched old-RA byte. The last lit dot in this row's left fragment is x57.
Across rows 224, 226, 228, 230 and 252, x16–99, every delayed pen with
horizontal delay 12 equals the raw pen twelve dots earlier: zero mismatches
in 416 comparisons. Pattern zeros explain why the last lit dot can precede
the end of all stale-byte history.

The five-dot interval before the new VRAM address is visible is accounted
for by the motherboard's video-owned address update; the returned data
then changes before the next serializer load. Relevant current paths are
`rtl/Amstrad_motherboard.v:897–933` (word assembly and video-owned address/
byte updates), `rtl/sdram.v:223–233` (VRAM address admission), and
`rtl/plus/asic_video.v:1032–1067,1164–1200` (serializer, delay and mask).
No independent stale-cache or delay-history inconsistency is exposed by
these observations. They establish what this model does, not whether its
fetch latency matches original silicon.

For this chain, the software write occurs after the source of the first
visible stale pixel has already been fetched and decoded. Clearing the
delay or widening the mask would hide that fact. The missing comparison is
still the original hardware's write and fetch phases: this run does **not**
locate the first divergence from hardware or AmSpirit upstream of the
visible pens.

## Pending-clear hypothesis

CPCEC's separate Eerie policy applies to changed, nonzero PRI writes that
do not take its current-line/raw-HSYNC set branch. It clears the raster
request while preserving DMA request bits (`cpcec.c`, revision
`c025aab961a796b918cc99bc3e16216ea65bb5d1`, lines 2106–2114).
That is distinct from the FF2 write-trigger behavior already integrated.

A new examination of the archived production `diagnostic/phase.log`, from
tick 385900000 onward, finds all 1,064 PRI-write onsets with `irq=1`,
`irqstates=7`, and `iff=0`. The five `irqstates` bits encode, from high to
low, DMA request, deferred raster fire, programmed INT_N, classic INT_N,
and GA INT_N. Thus no programmed, classic, deferred or DMA request is
pending at these writes. Importing the pending-clear policy would have
nothing to clear at those observed onsets.

Of these writes, 1,040 are changed, nonzero values, so this result does not
merely reflect same-value writes bypassing CPCEC's conditional branch.

This is a bounded negative result for the recorded steady reveal interval,
not proof about startup or every later demo state. It complements the
[earlier no-matching-write finding](ff2-runtime-pri-2026-09-27.md#eerie-follow-up-this-trigger-does-not-match-the-recorded-writes).
Neither PRI-write policy supplies a demonstrated repair for this residual.

## Remaining comparison boundary

The existing AmSpirit API investigation exposes instruction breakpoints,
instruction stepping and beam coordinates, but not raw HSYNC, interrupt
assertion or SSCR write-bus onset. The archived STATUS1 calibration contains
asynchronous/duplicate step observations; it cannot establish an exact
input-sampling-to-completion offset. No live emulator or device session
was altered for this follow-up.

The [flat-plane width/padding discriminator](amspirit-pri-phase-2026-09-26.md)
already separates the systems' width sensitivity at the rendered output.
Production follows the retained width-dependent, six-microsecond-clamped
rule; the AmSpirit marker is width-independent. Its absolute phase cannot
be recovered by subtracting production CPU/video latency from an emulator
pixel position.

The next decisive external evidence remains either:

- Original Plus/GX4000 results for the existing flat-plane CPRs at HSYNC
  widths 3, 6 and 11, using their visible marker positions; or
- A raw trace of HSYNC, INT, interrupt acknowledge and the SSCR write bus
  for Eerie, or an emulator trace exposing those equivalent events.

The existing hardware inputs are in the main checkout's
`local/task-archives/crtc3-2026-09-27/evidence/edge/pri/`:
`pri-width-3.cpr`, `pri-width-6.cpr`, and `pri-planes-4-12.cpr` (width 11).
They share four NOPs of handler padding and a 12-dot horizontal delay. Compare
final green-to-red transition positions relative to each image's unmasked
screen left, since monitor cropping can change with HSYNC width. The recorded
production positions are 171, 219 and 219 dots; AmSpirit gives 139 for all
three. These are predictions to discriminate, not original-hardware results.

The first option resolves the width-dependent source disagreement without
requiring a logic analyzer. The second can locate the first timing
divergence in Eerie itself. A screenshot alone cannot justify flushing
pixel history, enlarging the mask, or moving the ordinary interrupt.

The demo author's [published source announcement](https://www.pouet.net/prod.php?which=72271)
links a Google Drive archive (`1hJge_dv2bdkiuKk4zWM3sTNy1DL9xFwL`);
its public download returned HTTP 404 on 2026-09-27. No guest-source timing
claim is inferred from that unavailable archive.

## Reproduction and retained evidence

Private evidence is under this checkout's ignored
`docs/specs/eerie-left-edge-2026-09-27/`, with a durable copy at the main
checkout's `local/task-archives/eerie-left-edge-2026-09-27/evidence/`.
`inputs.json` identifies the RTL and media. The `diagnostic/` directory
contains source adapters, build/run logs, `run/fetch.csv`, retained frame
325/400 records, rendered frame 400, and `analyse.py` with its JSON/log
results. Generated build objects are not part of the durable copy.

From this investigation checkout:

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' make -C sim/plus -f ../../docs/specs/eerie-left-edge-2026-09-27/diagnostic/trace.mk eerie-graphics-build GHDL=/tmp/eerie-ghdl/ghdl-llvm-6.0.0-macos15-aarch64/bin/ghdl CXX=/opt/homebrew/opt/llvm/bin/clang++
docs/specs/eerie-left-edge-2026-09-27/diagnostic/build/eerie_graphics 'local/test_media/cartridges/Eerie_Forest_(Logon_System_2017).cpr' docs/specs/eerie-left-edge-2026-09-27/diagnostic/run 8
python3 docs/specs/eerie-left-edge-2026-09-27/diagnostic/analyse.py
```

Stage the archived adapter at that same relative docs path before replay:
its root calculation depends on that depth. `analyse.py` also reads the
earlier Eerie archive for comparisons. No timing/state fix was attempted,
so no new behavioral regression vector or selected-suite run is claimed.
Documentation-only handoff: READY for integration, residual unresolved.

## Independent Opus-high assessment

Requested by the user, Opus 5.5 at high effort reviewed this investigation
in run `20260927T033059Z-21358-c101` (exit 0). It verified the reported
counts and fetch transitions from this checkout's derived artifacts and
found no factual error in those measurements. Its access to the older
main-checkout archives, CPCEC source and web page was denied; it did not
independently reparse the old IRQ trace or run simulations. The full
assessment and invocation metadata are retained under private `review/`.

Its useful challenge is that cheap **causal interventions** remain before
original-hardware testing. A scratch-only GA variant using an earlier
ordinary-PRI trigger can determine whether moving that event alone removes
the residual. Compare raw-HSYNC rise and shaped-HSYNC rise against the
unchanged replay, recording the actual IRQ/ACK/SSCR bus chain, pixels and
pending requests at PRI writes. Preserve the FF2 event and ACK logic.
This tests sufficiency, not silicon correctness; a clean image cannot
authorize adopting the alternate trigger. The next bounded investigation
should perform this intervention before expanding the title matrix.

The reviewer also correctly highlights two limits of the hardware handoff:

- Establish whether this exact sliver is absent on original Plus/GX4000
  hardware. No such matched observation is recorded here.
- Measure the width-11 marker's absolute position as well as the width
  dependence. Width sensitivity alone cannot distinguish a constant CPU
  response offset from a trigger-phase offset or explain Eerie's cleanup.

The earlier [six synthetic raster probes](../../investigations/hardware-runs/crtc3-demo-2026-09-25.md#eerie-forest-left-edge-bounded-exclusions)
already agree with AmSpirit at early-line SSCR writes. Together with the
IRQ-driven marker difference, they make an upstream write-timeline
explanation the leading hypothesis. They do not prove identical video
behavior for every Eerie state. Accordingly, the review's stronger claim
that the first Eerie divergence is conclusively localized upstream is not
adopted. Nor is its proposed exact 28-dot correction/191-dot hardware
threshold established by a phase sweep yet. CPCEC's clean Eerie playback
has not been demonstrated in this investigation.

One further exclusion was checked directly after review: all 2,688 new
fetch-log samples have `shift=0`. Full-sync shifted-byte compensation is
therefore inactive in this sampled window. Finally, the no-pending-request
finding is conditional on the current IRQ timeline; an earlier-trigger
experiment must recheck it rather than assuming that exclusion transfers.
