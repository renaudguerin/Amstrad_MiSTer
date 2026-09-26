# Ordinary Plus PRI: AmSpirit, revised Arnold and the retained timing rule

## Which source disagrees?

This is **not an AmSpirit-versus-ACCC finding**. The rule used here is
[Arnold V Specs Revised §2.4](https://web.archive.org/web/20250102091704/https://www.cpcwiki.eu/index.php/Arnold_V_Specs_Revised),
CPCWiki revision 115989, last modified 2024-05-13. We have not cited or tested an
ACCC rule governing this programmable Plus ASIC interrupt in this experiment.
CRTC-generated raw HSYNC and ASIC-generated programmable interrupts are distinct
signals; describing this as a generic CRTC/ACCC disagreement would obscure that
boundary.

Revised Arnold locates ordinary PRI at the trailing edge of monitor HSYNC,
with timing dependent on the programmed width and capped at six character
cycles after raw HSYNC starts. It explicitly uses width 14 firing at cycle 6 as
an example. One sentence says “position” where its surrounding width discussion
and example imply width; this wording limitation is preserved in the
[earlier source assessment](b20-pri-phase-2026-09-23.md#fresh-source-reading).
The complete archived section was re-read for this investigation.

There is a competing account:
[Kevin Thacker, Extra CPC Plus Hardware Information, Raster Interrupts](../../specs/plus/Extra%20CPC%20Plus%20Hardware%20Information.md)
says fixed ten microseconds after HSYNC starts, independent of width. The
original Arnold issue 1.5 §2.4 says end of the programmed line without resolving
this phase distinction. Neither the core's simulation nor an emulator image
can adjudicate these conflicting descriptions of original hardware.

## Experiment and observations

A small generated CPR uses red and green flat raster planes, horizontal
scroll delay 12, IM1, PRI 7 and a HALT loop. Its handler uses EXX, optional four
NOPs, an SSCR write, eight NOPs, a second SSCR write, EXX, EI and RET. There are
no sprites or DMA channels. R0=63 and R2=49 keep HSYNC within its scanline for
all tested widths: the new cross-line PRI fix is inactive in this probe.

The production T80/motherboard/SDRAM model uses the two-fix source committed as
`bd65578` and `1eec469`. The reference is AmSpirit Lite 1.15.1/core 2491682,
model 4/CRTC3, native unfiltered rendering. The preceding Status1-driven version
of the flat-plane probe matched AmSpirit in colour classification, including
writes near the first displayed characters and the exact 12-dot scroll delay.

The table uses the final green-to-red transition with four NOPs of handler
padding. Pixel positions are relative to each image's observed display left,
**not** emulator beam coordinates relabelled as CRTC counters. This matters:
AmSpirit's cropped display origin changes when width is 3.

| R3 low nibble | Production INT after raw HSYNC | Production final green pixel | AmSpirit final green pixel |
|---:|---:|---:|---:|
|3|193 master clocks (3µs + one clock)|171|139|
|6|385 master clocks (6µs + one clock)|219|139|
|11|385 master clocks (6µs + one clock)|219|139|

One master clock is 15.625ns; one mode 2 dot is 62.5ns. Production therefore
moves the marker 48 dots for width 3→6 and clamps for 6→11. AmSpirit's marker
stays fixed. Removing the four NOPs moves the final marker 64 dots earlier in
both systems; at width 11 it is 155 in production versus 75 in AmSpirit, retaining
the 80-dot (5µs) difference. We compare colour classes because RGB amplitudes
differ between renderers.

Production instrumentation separates the events: INT-to-raw-ACK is 95 clocks,
and raw-ACK-to-consumption of the first handler opcode at 0038 is 311 clocks,
for all three widths. Each subsequent write translates by exactly 192 clocks
between widths 3 and 6. Thus the production width-dependent component originates
at request generation, not at an additional width-dependent CPU delay.

## What this proves, and why no retiming is proposed

The systems have different width sensitivity in their rendered ordinary-PRI
response. Production follows revised Arnold's width dependence; AmSpirit's
response is compatible with the *width-independent shape* of Thacker's account.
That does **not** establish that AmSpirit asserts IRQ at +10µs, or +1µs, or any
other absolute phase. Its API does not expose an equivalent IRQ/bus event;
the rendered marker includes CPU acceptance, handler execution and video
latency. Subtracting the production downstream delay from the emulator marker
would assume the very equivalence we are trying to establish.

We retain the existing ordinary PRI rule because this experiment supplies no
new original-Plus measurement that resolves the source conflict, and no
source-backed CPU WAIT defect. Moving PRI five microseconds earlier solely to
match AmSpirit could hide a different timing problem and regress software
that already relies on the retained rule. This does not close Eerie Forest's
left-edge residual or establish that either source is universally correct.

The useful external discriminator is the same small CPR on original Plus
hardware: compare marker positions for widths 3/6/11, ideally also observing raw
HSYNC and interrupt assertion at named signal taps. Widths 1–2 and exact
sub-character ordering remain separate unresolved questions.

## Reproduction and evidence

Private, ignored evidence is at
`docs/references/crtc3-2026-09-25/edge/pri/`: `README.md` contains the exact
production event chain; `make_pri_probe.py`, `probe.mk`, `amspirit_probe.py` and
`compare.py` reproduce the fixture and measurements. `comparison.json`,
`*-events.txt` and images preserve raw origins and results. No tracked RTL
was changed for this observation. All probe processes completed; AmSpirit's
snapshot, configuration, rendering and pause state were restored and checked.
