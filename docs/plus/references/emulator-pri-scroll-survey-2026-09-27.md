# Emulator source comparison: FF2 and Eerie Forest

Read-only delegated source review of the user's local checkouts. No emulator,
build, test, or game replay was run, and no production RTL was changed. Line
anchors below refer to these revisions; paths are relative to each checkout
under `/Users/renaudg/code/`.

| Checkout | Revision |
| --- | --- |
| caprice32 | `6c12c4c92360065cdc229ac9ada7551f941436b8` |
| CPCSyntaxError | `ad9df3b13fc5aa7b3c47967afcc7b67d70828994` |
| konCePCja | `aec1b9306ba317611687bc887e6f83075bcd94f2` |

## Conclusions and recommended next step

All three implement Plus hardware and exclude a PRI match at n+256. Only
CPCSyntaxError uses the same VC/RC-derived line value as our core and CPCEC;
the other two use a separate elapsed-scanline counter. This is useful
compatibility evidence, not three independent confirmations of the ASIC rule.
None supplies the same explicit current-line PRI-write trigger found in CPCEC
with its Fire & Forget 2 comment. None supplies title-specific evidence that
settles Eerie Forest's remaining left-edge spill.

Keep the full nine-bit comparison. Trace FF2's actual PRI writes and IRQ chain
next, distinguishing a missed write-trigger event from ordinary IRQ phase.
CPCSyntaxError supplies an additional delayed-event implementation to compare,
but its successful playback of these titles has not been established here.
Do not import another emulator's interrupt-clear policy as part of that fix.

For Eerie, no reviewed implementation establishes a reason to flush the
horizontal delay or widen the first-character mask. The unresolved comparison
remains software write phase versus address-to-pixel timing. See the existing
[phase evidence](amspirit-pri-phase-2026-09-26.md) and
[Eerie investigation](../../investigations/hardware-runs/eerie-forest-graphics-2026-09-23.md).

## PRI implementations

| Emulator | Compared line | Ordinary event | PRI write behavior |
| --- | --- | --- | --- |
| Caprice32 | Separate frame scanline counter, full equality | Programmed HSYNC termination | Stores PRI; new value can participate in the later termination check, no immediate write event |
| CPCSyntaxError | `((VC & 63) << 3) | (RC & 7)`, full equality | HSYNC-start scheduled compare delayed up to seven character ticks; separate wrap fallback | No immediate assertion; changed nonzero PRI clears CPU request and sets shadow state |
| konCePCja | Separate 16-bit frame scanline counter, full equality | New matching line, without HSYNC phase qualification | No immediate assertion or pending clear; already-observed current line does not retrigger |
| CPCEC (previous review) | `((VC & 63) << 3) | (RC & 7)`, full equality | Raw HSYNC assertion or line entry during active HSYNC | Changed current-line write during raw HSYNC can assert; separate conditional clear branch |

### Caprice32

`src/crtc.cpp:667–693` compares `sl_count == interrupt_sl` at sync termination.
Types are unsigned int and unsigned char (`src/cap32.h:335`). The counter
increments on new scanline (`crtc.cpp:1274–1277,1326–1332`) and resets on frame
restart (`650–660`); it is not derived from C4/C9. `src/asic.cpp:383` only
stores PRI. Thus a current-line write before the end check can affect it, but
a write after that check has no dedicated event.

The match sets CPU pending, raster pending and DCSR bit 7. Vector selection
prioritizes raster then DMA2/1/0; raster pending clears there only with IVR
bit 0 clear (`asic.cpp:218`). The reviewed acknowledge path does not similarly
clear DCSR bit 7. CPU acceptance/vector handling is at `src/z80.cpp:946`;
DCSR writes clear DMA flags (`asic.cpp:436–449`). These differences make this
a poor reference for our already-repaired ACK provenance without further
hardware evidence. Existing auto-clear tests cover DMA, not PRI
(`test/asic.cpp:276`).

### CPCSyntaxError

Line construction and event wiring: `src/core/emulator.cpp:105,169–172`;
HSYNC start: `src/core/crtc.cpp:332`; delayed comparison:
`src/core/asic.cpp:202–225`. The wrap fallback uses `R2 + width >= R0`.
The changed-nonzero PRI write branch (`asic.cpp:136`) clears the CPU request
through a callback and sets `shadowInterrupt`; it does not directly clear
the ASIC raster-pending flag. Same-value and zero writes skip that branch.
Do not equate this with CPCEC's pending-clear rule.

CPU acceptance calls ASIC acknowledge (`src/core/z80.cpp:127`,
`emulator.cpp:1156`, `asic.cpp:339`), handling raster/GA before DMA. The
DCSR-write path is separate (`asic.cpp:182`). README's ACCC attribution is
for the CRTC (`README.md:195`), not a cited derivation of these Plus rules.
No CPCEC attribution was found; the ASIC appears in the initial public
release, so this checkout does not establish independent provenance.

### konCePCja

Active Plus path: `src/subcycle_bridge.cpp:296–323,715` and
`src/subcycle/machine.cpp:36–53,96–99`. `src/hw/asic.cpp:480–490,502–522`
uses full-width line equality and a previous-line guard. `asic_tick` calls
IRQ evaluation before register-write decode (`554–577`). The line comes
from CRTC `scanline`, incremented at horizontal total and reset at new frame,
including adjust lines (`src/hw/crtc.cpp:21–51,161–169,191–205,328–335`).
PRI pending clears on IACK, which also clears DMA interrupt flags; PRI/DCSR
writes do not clear PRI pending (`asic.cpp:236–271,502–519`).

The new ASIC file names legacy `src/asic.cpp` as its behavioral oracle
(`src/hw/asic.cpp:1–5`). The rewrite plan says zero copied code, with legacy
behavior and hardware references consulted
(`docs/plans/2026-07-09-001-feat-runtier-fast-plan.md:19–29,268–271`).
A rewrite is therefore not automatically an independent behavioral source.
No FF2/Eerie-specific evidence was found. The ASIC spec's header still says
implementation pending despite the implemented code; use code over that label.

## Eerie Forest: live SSCR and left edge

Our known residual is screen-plane data: SSCR AC→8C at C0=2 as the first
16-dot mask ends, with old RA6 data emerging through a 12-dot horizontal delay
after RA changes to 4. The write changes vertical scroll 2→0, retaining
horizontal scroll 12 and border extension.

- **Caprice32:** shifted rendering reads preceding video bytes
  (`src/crtc.cpp:882`); extended border increments `hstart` by one (`490`).
  Sprite clipping has fixed geometry and a CRTC-derived-geometry TODO
  (`src/asic.cpp:498`). No title-specific remedy was found.
- **CPCSyntaxError:** SSCR immediately updates scroll/border and current VLC
  (`src/core/asic.cpp:148`, `emulator.cpp:212`). Per-character byte capture
  records video-revision segments (`emulator.cpp:698`); rendering applies
  horizontal shift and the 16-pixel mask (`src/core/video.cpp:814,825`).
  Captured old-RA bytes can consequently survive a later vertical-scroll
  write. This does not demonstrate the exact real-hardware fetch latency.
- **konCePCja:** Plus video snapshots state at HSYNC fall and shifts background
  through preceding-cell carry, reset at the left edge
  (`src/hw/video.cpp:203–215,393–501,597–605`). CRTC border extension is one
  character (`src/hw/crtc.cpp:86–97,228–244`), and split registers are latched
  once per frame (`129–141,161–169`). This model cannot settle the live,
  sub-character SSCR/pixel-history question. A visually clean result could
  reflect that simplification.

## Add konCePCja to the debugging tools?

Yes, as an optional guest-code investigation tool; retain AmSpirit/CPCEC for
the current Plus comparison. No installation or integration was performed.

Its IPC has real MREQ-edge watchpoints: `wp add 0x6800 1 w` and `wait bp`
can report PC, address, value and old value (`docs/ipc-protocol.md:210–223`,
`src/koncepcja_ipc_server.cpp:985–1005`, `src/hw/probe.cpp:80–129`). Check that
the ASIC is unlocked and paged in, since an address match alone does not prove
an ASIC transaction (`src/hw/asic.cpp:562–577`). This can efficiently locate
FF2's PRI-writing instructions and values.

Limitations before using its results as timing evidence:

- `regs asic interrupts` exposes PRI/vector/DMA state but omits PRI pending
  (`src/hw/asic.h:26–37,107–124`, `asic_debug.cpp:79–109`).
- `regs crtc` exposes paused counters, but its SL field is narrowed from
  16 bits to unsigned char in `src/subcycle_bridge.cpp:1063–1072`. The display
  can alias line n and n+256 even though its PRI comparison does not.
- `trace` is a retired-Z80-instruction ring, not a raw HSYNC/IRQ/bus timeline
  (`src/koncepcja_ipc_server.cpp:1285–1297,1882–1904`).

First establish that the unchanged FF2 cartridge reaches the relevant scene
in this emulator before relying on its guest execution trace. Source review
alone does not establish game compatibility, build portability or debugger
runtime reliability.
