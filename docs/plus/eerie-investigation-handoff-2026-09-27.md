# Fresh-session handoff: Eerie Forest left-edge residual

## Current position

The three reveal-line leaks were repaired and accepted in the integrated
`edaa15b` build. The remaining left-edge residual has two independent causes;
see the [trigger counterfactual and sprite-mask finding](references/eerie-pri-trigger-counterfactual-2026-09-27.md):

- **Green first-column bar** (rows 16–215, x16–31, landscape phase): these
  are sprite pixels inside SSCR[7]'s extended-border character. Branch
  `plus/eerie-hsync-trigger` gates sprites with the masked display enable,
  following Arnold V §2.1/§2.5. All four local emulators agree, and AmSpirit
  does not show the bar. The change is gated and independently reviewed, but
  integrated on 2026-09-27; MiSTer check pending.
- **Dotted logo strip** (rows 224–254, x32–59): the reveal loop's SSCR write
  at C0=2 follows the first-character fetch, which uses the old RA. Scratch
  replays that change only the ordinary PRI trigger remove it when the fire
  point is at most raw HSYNC + 4 µs (write by x4). Raw-HSYNC rise and
  shaped-HSYNC rise both qualify; +5 µs and the current +6 µs do not. No
  pending request exists at any PRI write under any tested timeline.

FF2 and Prehistorik II are hardware-accepted on `2f409e9`. Preserve the
nine-bit PRI comparison, FF2 live-write event, ACK provenance, CRTC3 repairs
and sprite-row retargeting.

## Resume path and question

Check the integrated sprite-mask build of Eerie on MiSTer:
the green bar should disappear and the dotted strip should remain. Also check a
horizontally scrolling title that keeps D7 set with left-edge sprites.

For the dotted strip, earlier-trigger sufficiency is established in the model,
not on hardware. Both documented sources place the trigger on the residue side
(revised Arnold 6 µs clamp, KT ~10 µs). Do not move the trigger without a
hardware measurement. Discriminators, cheapest first:

1. Derive the Z80 IM1 acknowledge-to-0038 fetch time on the CPC wait grid from
   the T80/Z80 sources. Compare it with production's measured INT→ACK 95 and
   ACK→0038 311 master clocks. An excess would move the fix from request
   generation to CPU response.
2. Original Plus/GX4000 results for the width 3/6/11 flat-plane CPRs, using
   absolute marker position as well as width dependence. AmSpirit's width-11
   marker is 5 µs earlier than production, inside the sufficient region.
3. A raw HSYNC/INT/ACK/SSCR-write trace for Eerie.

Keep CPU instruction-stop times separate from raw bus events. Original-hardware
cleanliness of the strip remains unverified.

## Existing emulator clues

These local checkouts and revisions were already inspected:

| Checkout and revision | Useful clue |
| --- | --- |
| `/Users/renaudg/code/cpcec` — `c025aab961a796b918cc99bc3e16216ea65bb5d1` | Ordinary PRI is requested on raw HSYNC assertion or entry during active HSYNC. A separate changed-PRI branch can clear the pending interrupt and explicitly mentions Eerie. That clear behavior is a distinct, unadopted policy; it is not evidence for changing the core. |
| `/Users/renaudg/code/caprice32` — `6c12c4c92360065cdc229ac9ada7551f941436b8` | Uses an elapsed scanline counter and checks PRI at programmed HSYNC termination; PRI writes have no immediate event. Shifted rendering reads preceding video bytes, but gives no Eerie-specific remedy. |
| `/Users/renaudg/code/CPCSyntaxError` — `ad9df3b13fc5aa7b3c47967afcc7b67d70828994` | Uses the same VC/RC line value as this core, with a delayed HSYNC-start comparison. Its changed-nonzero PRI path clears the CPU request and updates shadow state; it is not CPCEC's pending-clear rule. Per-character video revisions can preserve old-RA bytes across an SSCR update. |
| `/Users/renaudg/code/konCePCja` — `aec1b9306ba317611687bc887e6f83075bcd94f2` | Uses a separate scanline counter and evaluates a matching line without HSYNC phase qualification. Video state is sampled at HSYNC fall, so this model cannot settle the live sub-character fetch question. |

None of these results establishes the original ASIC behavior or an Eerie
repair. The survey has source anchors and the remaining debugger limitations.

## Evidence and tools

- Trigger-variant and sprite-mask replays (combined frame/fetch/bus-event
  harness, scratch GA/video variants, per-run logs, frames and analysis) are
  at `/Users/renaudg/code/Amstrad_MiSTer/local/task-archives/eerie-hsync-trigger-2026-09-27/evidence/`.
  Its `sources/trig.mk` builds from `sim/plus` with
  `-f obj_dir/eerie-trigger/trig.mk eerie-trigger-build VARIANT=<name>`, after
  staging `sources/` at `sim/plus/obj_dir/eerie-trigger/`.
- Current replay sources, fetch CSV, frame records, analysis and full Opus
  review are preserved at
  `/Users/renaudg/code/Amstrad_MiSTer/local/task-archives/eerie-left-edge-2026-09-27/evidence/`.
  The report explains the adapter's required staging path and reproduction
  commands. Stage needed old evidence inside a worker's checkout when its
  sandbox cannot read the main archive; do not mistake derived summaries
  for an independent read of the original logs.
- Existing hardware probes are in
  `/Users/renaudg/code/Amstrad_MiSTer/local/task-archives/crtc3-2026-09-27/evidence/edge/pri/`:
  `pri-width-3.cpr`, `pri-width-6.cpr`, `pri-planes-4-12.cpr` (width 11).

- Eerie production-T80 replay, bus phase log, and device captures are archived
  in the main checkout at
  `/Users/renaudg/code/Amstrad_MiSTer/local/task-archives/eerie-graphics-2026-09-23/evidence/`.
- FF2 PRI replay evidence is archived at
  `/Users/renaudg/code/Amstrad_MiSTer/local/task-archives/ff2-pri-2026-09-27/evidence/`.
- The earlier ordinary-PRI width probe and AmSpirit comparison are archived at
  `/Users/renaudg/code/Amstrad_MiSTer/local/task-archives/crtc3-2026-09-27/evidence/`.
- Use the existing corrected Eerie cartridge/replay inputs. The original CPR
  has malformed RIFF sizing; consult the [container diagnosis](eerie-forest-container-2026-09-23.md)
  before treating a loader failure as an emulation failure.
- AmSpirit API is at `http://127.0.0.1:6128`. Save and restore the user's
  snapshot, configuration, rendering, and pause state; clear temporary
  breakpoints. Its instruction breakpoints and beam coordinates are not raw
  CRTC subphase or bus timestamps, and its live API does not expose raw HSYNC
  or full C4/C9. The previous Eerie investigation restored and checked state.
- Original Plus/GX4000 visual acceptance remains user-owned. MiSTer capture
  evidence is not original-hardware acceptance.

Read the [Eerie investigation](../investigations/hardware-runs/eerie-forest-graphics-2026-09-23.md),
[FF2 runtime trace](references/ff2-runtime-pri-2026-09-27.md),
[AmSpirit PRI phase comparison](references/amspirit-pri-phase-2026-09-26.md),
[completed emulator survey](references/emulator-pri-scroll-survey-2026-09-27.md),
and [FF2 handoff](ff2-investigation-handoff-2026-09-27.md) for full methods and
limits.

## Change and stopping gates

Do not begin a timing/state fix until a focused deterministic test fails on the
current RTL and its expected result is derived from source or a hardware
measurement. After the last code edit, run `python3 sim/select_tests.py --run`
once and obtain a fresh cross-provider review for a non-trivial RTL diff.
Report the finding, evidence, and remaining uncertainty; stop at READY unless
`stream-finish` is explicitly requested.
