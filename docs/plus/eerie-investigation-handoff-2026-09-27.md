# Fresh-session handoff: Eerie Forest left-edge residual

## Current position

The three reveal-line leaks were repaired and accepted in the integrated
`edaa15b` build. The separate dotted-logo and green landscape sliver at the
left edge remains. The Eerie report records its current discriminator: SSCR
changes `AC→8C` at C0=2, dot 0, as the 16-dot first-character mask ends; old
RA6 fetch data then emerges through the 12-dot horizontal delay after RA
becomes 4. Existing evidence does not establish a repair.

FF2 source `d56ce1f` is integrated as `2f409e9`. Its hardware acceptance is
still pending. Preserve the accepted nine-bit PRI comparison, ACK provenance,
CRTC3 repairs, and sprite-row retargeting repair. The FF2 changed-PRI trigger has
no demonstrated path to Eerie: the recorded steady trace has 1064 PRI writes
and none matches the full current CRTC line.

## Resume path and question

Start a new Plus task from the latest `master` with `stream-start plus`; do not
resume an older investigation branch as the source of truth. The broad source
survey is complete; use its comparison below instead of repeating it. Use cheaper
subagents for bounded searches and mechanical work; keep causal analysis in the
main investigation.

Find the first divergence in the Eerie chain: software SSCR write, ASIC fetch
address/data, or emitted pixel/mask. If IRQ timing appears causal, demonstrate
the link with raw HSYNC, INT/acknowledge, and the actual write-bus onset. Keep
CPU instruction-stop times separate from raw bus events. Do not infer a flush,
mask change, pending-clear rule, or retiming from the visible artifact alone.

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
`stream-finish` is explicitly requested. No simulation was run for this
handoff.
