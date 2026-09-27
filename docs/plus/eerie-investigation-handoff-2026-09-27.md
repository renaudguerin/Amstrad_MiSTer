# Fresh-session handoff: Eerie Forest left-edge residual

## Current position

The three reveal-line leaks were repaired and accepted in the integrated
`edaa15b` build. The remaining left-edge residual had two independent causes;
see the [trigger counterfactual, adopted trigger and sprite-mask finding](references/eerie-pri-trigger-counterfactual-2026-09-27.md):

- **Green first-column bar** (rows 16–215, x16–31, landscape phase): sprite
  pixels inside SSCR[7]'s extended-border character. Sprites are now gated by
  the masked display enable, following Arnold V §2.1/§2.5. The fix was
  integrated as `8306e6f`, and a MiSTer capture on that RBF shows the bar
  gone.
- **Dotted logo strip** (rows 224–254, x32–59): the reveal loop's SSCR write
  follows the first-character fetch, which uses the old RA. Ordinary PRI now
  requests 1 µs after raw HSYNC start at every width. This reproduces
  AmSpirit's width-independent probe markers and removes the strip in
  simulation. It overrides revised Arnold's +6 µs clamp and KT's ~10 µs, and
  has no original-hardware confirmation.

FF2 and Prehistorik II are hardware-accepted on `2f409e9`. Preserve the
nine-bit PRI comparison, FF2 live-write event, ACK provenance, CRTC3 repairs
and sprite-row retargeting.

## Resume path and question

1. On MiSTer, confirm the PRI-retimed build keeps FF2 gameplay (sky gradient,
   music), Prehistorik II's HUD, Copter 271's title, and the CRTC3 demo clean,
   and that the Eerie strip is gone.
2. On an original Plus/GX4000, run `pri-width-3.cpr`, `pri-width-6.cpr` and
   `pri-planes-4-12.cpr` (width 11). The new rule predicts the same final
   marker at every width; revised Arnold predicts a 48-dot step from width 3
   to 6. A step means revert the trigger and look for the Eerie strip on the
   fetch side.
3. Short-width PRI (R3=1/2) and the line-boundary coincidence remain
   unmeasured.

Keep CPU instruction-stop times separate from raw bus events.

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
