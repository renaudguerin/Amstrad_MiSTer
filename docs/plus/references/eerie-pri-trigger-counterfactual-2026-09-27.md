# Eerie Forest: ordinary-PRI trigger counterfactual and sprite-mask split

Base: `2754e45bd1c374fa426b378ee1434e9456aba06c`. This investigation keeps the
integrated FF2 live-write event, the nine-bit comparison, ACK provenance, and
the CRTC3 and sprite repairs. The earlier handoff treated "the dotted logo and
green left-edge sliver" as one residual. It is two unrelated artifacts:

| Artifact | Where (dot grid `x=C0*16+dot`) | Mechanism | Status |
|---|---|---|---|
| Dotted logo strip | rows 224–254, x32–59, frames 303 onward | Old-RA screen bytes fetched before the reveal loop's SSCR write at C0=2 | **Fixed** by requesting ordinary PRI 1 µs after raw HSYNC start ([adopted trigger](#adopted-trigger-1-µs-after-raw-hsync-start)); hardware-unverified |
| Green bar | rows 16–215, x16–31, from about frame 1125 | Hardware-sprite pixels drawn inside SSCR[7]'s extended-border character | **Fixed** from the Arnold V source rule; confirmed on MiSTer `8306e6f` |

## Method

The archived production-T80 fixture from the
[fetch-phase investigation](eerie-fetch-phase-2026-09-27.md) was combined
into one harness. It captures frame records and the rows 224–230 fetch chain,
and logs raw HSYNC, monitor HSYNC, raster fire, INT, ACK, opcode consumption,
`&6800–&6805` write onsets and SSCR changes from tick 385.9M. Every variant
uses the unchanged CPR, 6128+, Full sync and production clocking. Each variant
replaces only the ordinary edge term of `raster_fire` in a scratch copy of
`asic_ga_timing.v`. The line-entry event, FF2 `pri_value_change`,
comparison, pending latch and ACK logic stay byte-identical:

- `rawrise`: raw CRTC HSYNC assertion (`hsync_n_d & ~hsync_n`).
- `monrise`: shaped monitor HSYNC_O rise.
- `d320`, `d256`, `d192`: a counter N master clocks after raw HSYNC assertion,
  giving a 1-character sweep between those points.

Eight-second runs reach frame 401. Baseline, `rawrise` and `monrise` were
also run for 24 seconds (1,203 frames). The scratch sources, per-run logs,
event logs, frame binaries and analysis are in the main checkout at
`local/task-archives/eerie-hsync-trigger-2026-09-27/evidence/`.

## Dotted logo strip: trigger sufficiency

Measured timelines (master clocks after raw HSYNC rise; mode values over
4,252 programmed interrupts per run):

| Variant | Fire | INT | Reveal SSCR write (dominant) | Left-strip pixels, frames 320–400 |
|---|---:|---:|---|---:|
| baseline (monitor HSYNC fall) | 384 | 385 | C0=2 dot 0 (x32) | 9,272 in 80 frames |
| `d320` | 320 | 321 | C0=1 dot 0 (x16) | 220 in 2 frames (frame 400: x32–41) |
| `d256` | 256 | 257 | C0=0 dot 0 (x0) | 0 |
| `d192` | 192 | 193 | C0=63 of previous line | 0 |
| `monrise` | 128 | 129 | C0=62 of previous line | 0 |
| `rawrise` | 0 | 1 | C0=60 of previous line | 0 |

Everything downstream of the fire translates rigidly: ACK, opcode and write
distributions shift by exactly the fire offset. CPU writes stay on the 1 µs
(64-clock) grid, so the sweep is complete at character resolution.

Frame 400 maps each write phase to its residue. A change at C0=0 dot 4
(x4) is clean. At x16 the strip covers x32–41, at x20 x32–43, and at the current
x32 x32–57. Row 228's fetch chain shows why: the first-character VRAM
address is formed with the current RA at x5. With the write on the preceding
line, the first fetch already uses RA4, so no RA6 byte enters the serializer.
**In this model the SSCR change must land by x4; the ordinary trigger must be
at most raw HSYNC + 4 µs.** This supports the Opus-high 28-dot threshold
(x32→x4) as a model property. It is not a hardware measurement.

No collateral change: frames 275–1200 were sampled every 25 frames, and each
variant differs from baseline only at x<100. Per-frame pixel counts outside
x16–99 are identical across all 1,203 frames. Apart from the strip, the only
difference is a 9-pixel tone change on rows 3–5 at x48–51 (`0xA9A` against
`0xAAA`), from a palette write that now lands earlier. The frame-325 reveal
rows 65/108/159 remain black in every variant.

### Pending-clear exclusion under each timeline

In every run, all 4,449 PRI writes in the logged interval have
`irq=1 irqstates=7 iff=0`, and 4,350 of them are changed, nonzero values. No
programmed, classic, deferred or DMA request is pending at any PRI write under
any trigger. CPCEC's separate pending-clear policy would have nothing to clear
in any tested timeline, so it cannot explain the strip.

### What this does and does not establish

Moving only the ordinary trigger at least 2 µs earlier is *sufficient* to
remove the strip in this model. It does not show that hardware fires there.
Both documented trigger accounts lie on the **residue** side of the threshold:
revised Arnold's clamp at HSYNC start + 6 µs, which production implements, and
KT's ~10 µs. Adopting either still produces the strip. AmSpirit's width-11
marker is 80 dots (5 µs) earlier than production end to end, inside the
sufficient region
([width discriminator](amspirit-pri-phase-2026-09-26.md)). That difference can
come from request generation or from CPU response latency. Production measures
INT→raw ACK = 95 clocks and ACK→first 0038 opcode = 311 clocks. No trigger
change is proposed: it would contradict both written sources without a
hardware measurement.

The remaining discriminators are unchanged: original Plus/GX4000 results for
the width 3/6/11 flat-plane CPRs (absolute marker position, not only width
dependence), or a raw HSYNC/INT/ACK/SSCR-write trace for Eerie. A
source-derived check of the Z80 IM1 acknowledge-to-0038 time on the CPC wait
grid against the measured 311 clocks is a cheap next step. It would separate
CPU response from request generation.

## Green bar: sprites under the SSCR[7] border

The bar's pixel records show `show_spr=1`, `eff_de=0`, `de_first_char=1`,
`SSCR=&8C`, and screen palette entry 16 (border) with sprite RGB `0x572`. These
are hardware-sprite pixels inside the first-character mask. The previous RTL
gated sprites on unmasked `de_hold`, so they showed through the extended border.
The PRI trigger does not affect them: the bar is identical in every trigger
variant.

Source reading, Arnold V Issue 1.5
(`docs/specs/plus/_Arnold V_ Specification - Issue 1.5 - 10th April 1990.md`):
§2.5 says D7 "causes the border to extend over the first two bytes (16 high
resolution pixels) of each scan line", and §2.1 says "the border has the
highest priority, followed by sprites 0 to 15 … then the main screen data".
The earlier vectors relied on §2.5's opening sentence: "This soft scrolling
mechanism affects the whole of the main screen … but it does not affect
sprites." That sentence describes the scroll offsets, which do not move
sprites. It does not say the extended border stops being border.

All four local emulator checkouts keep sprites out of the extended border:
CPCEC `c025aab` (`video_plus_cleanup`, cpcec.c:716–720, overwrites 16 border
pixels after sprites), CPCSyntaxError `ad9df3b` (`spriteClipLeft`,
video.cpp:174–179, 953), konCePCja `aec1b93` (extra skew holds display enable;
sprites are skipped outside it), and Caprice32 `6c12c4c` (frame-level: the
`asic_draw_sprites` border width includes the extra 16, asic.cpp:500). The user
reports that AmSpirit does not show the bar. Emulators are corroboration, not
authority.

The repair changes `show_spr` from `de_hold & SPR_EN` to `eff_de & SPR_EN` in
`rtl/plus/asic_video.v`. It inverts the two vectors that pinned the old
reading: `t08g` (masked dots hide the sprite, dot 17 shows it, D7=0 shows it in
character 0) and the m13 `SSCR[7] X=0` motherboard case. Before the change,
`t08g` failed with `RGB: expected 0, actual 240`. A 24-second replay with the
change differs from baseline in exactly 3,200 pixels per affected frame
(16×200). All of them are sprite pixels in the masked character that become
border black. The other 34 sampled frames are identical.

Selected gate after the last edit:
`MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/select_tests.py --run`
→ `select_tests: PASS 3 benches: run/asic_video_tests, run/p1_video_tests, run/p1_mobo_bench_tests`.
The slow `b6-plus-layers` bench programs SSCR without D7 and cannot exercise
this change.

Independent Sol-high review (Codex bridge run `20260927T055140Z-79558-3c8a`,
log archived under `evidence/sources/review.log`) found no code issue and
recommended shipping. `eff_de` and `show_spr` feed the same RGB register on the
same `PIXEN && CLKEN` edge, so no dot shift is introduced. No other logic
consumes `show_spr`, and both vectors assert the full rule. Its documentation
note on the ASIC reference's sprite/SSCR wording is addressed.

Hardware acceptance is outstanding. On a MiSTer build with this change, the
green first-column bar during Eerie's landscape phase should disappear and the
dotted logo strip should remain. Titles that scroll horizontally with D7 set
and place sprites at the left edge are the regression class to watch.

## Adopted trigger: 1 µs after raw HSYNC start

The user accepted an aggressive change for the strip. The decision rests on
three measurements, and it overrides both written trigger accounts.

**CPU response matches the ACCC.** ACCC §27.4 (FR p285, EN p286) says an
interrupt's call to #38 lasts 5 µs on the CPC. M1-edge logging in the
production Eerie replay measures 320 master clocks (5 µs) from the start of
the acknowledge M1 cycle to the M1 fetch at 0038, and 304 clocks in a minority
of alignments. Production's INT-to-write latency is therefore not the
missing time.

**The strip is set by the loop phase, not only by per-line interrupts.** The
reveal writes at C0=2 come from a cycle-counted loop at PC 4A8. It is entered
from the line-223 interrupt, so its phase inherits that entry's jitter (1 µs
steps): 80 frames write at C0=2 and 18 at C0=1. Both are beyond the x4
threshold. The handler's own SSCR write at PC 3A lands between x4 (clean) and
x20 (residue).

**AmSpirit's width-independent markers fix the phase.** In the
[width discriminator](amspirit-pri-phase-2026-09-26.md), AmSpirit's final
marker sits at 139 dots for R3=3/6/11, and production's at 171/219/219. With
equal CPU response, a request at raw HSYNC + 64 clocks moves production by
−32 dots at width 3 (fire 192→64 clocks) and by −80 dots at widths 6/11
(384→64). That lands exactly on 139 at all three widths. The no-NOP variant
(155 against 75, an 80-dot gap) agrees. The +1 µs point lies inside the
≤ +4 µs region that cleans Eerie.

**Implementation.** The monitor-HSYNC trailing-edge term in
`rtl/plus/asic_ga_timing.v` is replaced by a saturating counter from raw HSYNC
assertion. The event fires at 64 clocks if HSYNC is still active. The
line-entry event, FF2 live-write event, nine-bit comparison, pending latch and
ACK provenance are unchanged. Consequences, from the independent review:

- Width 1 never requests, because HSYNC ends as the counter reaches 64.
  Width 2 does request.
- If the 1 µs point coincides with entry to the next line, both terms compare
  the new line and deliver one request.
- An ordinary event still due inside a snapshot-restored HSYNC is dropped.

**Tests.** `asic_pri_test` pr10 is new. It requires INT 64 clocks after the
first edge sampling raw HSYNC at widths 3/6/7, and failed before the change
with `width 3 INT 192 clocks after raw HSYNC, expected 64`. The connected
`p1_pixel_phase_test` cross-line vector now requires the request 64 ticks
after raw HSYNC first reads high, while HSYNC is still active, at R2=49/50/51.
Its line-entry counts are unchanged. Gate after the last code edit:
`select_tests: PASS 7 benches` (asic_ga_timing_diff, p1_video, p1_mobo_bench,
asic_pri, p10_dma_ppi, p10_dma_mobo, b8_palette). The slow benches
b20-ack-diag, d5-cart-timing and b20-bus-diag pass.

**Replays.** Over a 24-second production-RTL Eerie replay, every ordinary fire
lands at +64. Logo-phase strip pixels drop from 13,172 to 0. The only other
change is the 36-pixel rows 3–5 palette tone, and all 40,500 PRI writes still
see no pending request. A 10-second title matrix (baseline, +4 µs and +0 µs
triggers) shows:

- CRTC3 demo (397 PRI writes): identical in all sampled frames.
- Copter 271: a near-black `0x001`/`0x002` palette split on rows 57–60 moves
  earlier within the line.
- Burnin' Rubber, Switchblade and World of Sports never use PRI in the window.
- FF2 did not reach PRI-driven play without input. Its archived snapshot
  replay could not be staged in the checkout, because the session's permission
  policy blocked copying it, so FF2 relies on the MiSTer check.

Independent Astra-high review (Codex run `20260927T065024Z-5208-c27f`):
ship for the authorized MiSTer trial, with no blocking RTL defect. Its
stale-comment and reference notes are addressed. Its boundary consequences are
documented above rather than pinned by further vectors.

**Status.** This is an emulator-derived, source-contradicting rule. Original
Plus/GX4000 results for `pri-width-3.cpr`, `pri-width-6.cpr` and
`pri-planes-4-12.cpr` (width 11) decide it. The new rule predicts the same
final marker at every width, as AmSpirit shows; the revised-Arnold rule
predicts a 48-dot step from width 3 to 6. If hardware shows that step, revert
this trigger and look for the Eerie strip on the fetch side.
