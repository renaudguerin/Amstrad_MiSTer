# B1 hybrid live blanking

**Status (2026-09-01): implementation candidate; simulation-gated, hardware confirmation required.**

This change repairs the signal contract of the `Live blanking` sync-filter mode. It does not
claim that DSC4 or SHAKER is fixed on hardware.

## Hardware evidence and diagnosis

The complete local evidence set supplied on 2026-09-01 was inspected: the four Full/Live
captures for Amazing Demo and Pulpo, all sixteen captures under
`docs/screenshots/sync filter/shaker_module_A_live_blanking/`, and both DSC4 captures. Full
keeps a normal acquisition geometry. Live produces a materially narrower or cropped picture,
with black side regions in the ordinary demos, and does not repair the SHAKER Module A R2/R3
update cases or DSC4. Off can lose scaler lock when software varies line geometry. These are
hardware observations and take precedence over the simulation model.

The source seam explains the Live geometry regression:

- `ga40010` drives `HSYNC_O` from the delayed monitor-sync shaper (`hdelay[2]`). It is a short
  shaped sync pulse, not a complete MiSTer scaler acquisition blank.
- The classic Gate Array RGB path actually force-blanks on `HCNTLT28 | HSYNC_I`, and exports
  `VBLANK = HCNTLT28`. The Plus pixel path likewise force-blanks from its raw CRTC HSYNC.
- The first Live implementation assigned `hblank = hsync_ga`. That labelled only the shaped
  sync pulse as HBLANK, so the scaler treated too much of each line as active and shrank the
  visible CPC picture. Replacing it with raw HSYNC alone would repeat the same category error:
  raw HSYNC is a live force-blank event, not a complete acquisition interval.

## Implemented contract

Full and Off retain their existing mux branches. Live continues to use regenerated HSYNC and
VSYNC so the scaler has stable timing. Its HBLANK now uses raw CRTC/ASIC HSYNC only as the live
phase anchor, then holds a minimum 64-CE-tick acquisition blank. In the production 64 MHz / 4
MHz clock relationship this is 1024 master clocks, so the window retains sub-CE phase rather
than rounding a type-1 R2.JIT displacement of three Pixel-M2 (12 master clocks) to a CE edge.
The raw R2.JIT pulse still shortens at its physical trailing edge; only the scaler acquisition
metadata keeps the stable width and moves as a whole. That is a deliberate separation, not a
claim that R2.JIT preserves raw sync width.

The resulting normal 256-tick line has 192 acquisition ticks, matching the Full window derived
from `BEGIN_HBORDER=49` and `END_HBORDER=241`. A raw force-blank pulse longer than the minimum
remains blank for its full duration. A second short raw pulse inside the minimum does not restart
and indefinitely lengthen the acquisition window. A pulse observed on the exact expiry clock does
start its own minimum window; the expiry edge belongs to the new pulse rather than the old one.
In the controlled fixture, Live begins at the observed raw phase and the regenerated Full HBLANK
begins seven CE_4 enables later. That +7-CE relationship is a model measurement of the current
filter pipeline, not a hardware timing oracle.

Raw `HSYNC_I` deliberately bypasses the too-frequent-sync mask for Live force blanking. An edge
inside an open window is ignored, but an edge at or after expiry may open another window. Malformed
cadence can therefore create adjacent or repeated acquisition windows; Live preserves raw blanking
intent and is not a sync sanitizer. Once the horizontal watchdog classifies sync as absent, Live
falls back to Full HBLANK and its synthetic cadence. The watchdog now retains the last healthy
Full cadence downstream and promotes that fallback on timeout even if raw HSYNC is stuck high and
VSYNC never arrives. A composed guard confirms that a masked short retrigger cannot disturb final
Full HBLANK cadence even though raw `HSYNC_I` remains Live's force-blank input. VBLANK continues to
use the live Gate Array/ASIC vertical blank output.

The focused seam vector first failed on the old route:

```text
HBLANK_LIVE fell early at CE tick 4 (observed width 4 CE ticks, expected 64 CE ticks)
```

It now pins nine distinct properties: the unchanged Full geometry; an exact 1024-master-clock
Live window at both the normal and +3-Pixel-M2 R2.JIT phases; the live force-blank override for
a longer pulse; non-restarting behavior for too-frequent raw pulses; exact-expiry reacquisition;
Full fallback after the existing missing-sync lifecycle; no-VSYNC stuck-high watchdog recovery;
masked-retrigger composition with that fallback; and the production Full/Live/Off selector tuple
for final HSYNC, VSYNC, HBLANK, and VBLANK.

## Honest hardware boundary

This hybrid preserves live blanking phase while deliberately regenerating the sync delivered
to the scaler. If the visible DSC4 or SHAKER result depends on the physical output HSYNC edge
rather than on the RGB/blanking phase, a scaler-safe regenerated-sync path cannot reproduce it.
That outcome needs a hardware A/B test of this build and, if still negative, a pre-filter capture
tap or analog-output discriminator. A passing seam test, full simulation, synthesis, or an RBF
does not close that hardware residual.
