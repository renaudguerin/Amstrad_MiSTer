# CRTC 1 R7: optical address qualification — 4 November 2026

The new reference control does not justify an admission fix. AmSpirit returns
PPI bit0=0 for an R7 write near a horizontal carry even though address-tagged
pixels identify adjacent nonterminal rasters of the same displayed character
row. This is diagnostic evidence requiring hardware/author discrimination;
do not turn it into a new C0 gate in MiSTer.

## Why the earlier pair was insufficient

A local actual-T80 pair gives terminalR9=7 and nonterminalR9=8 the same bus
schedule: first datumC0/C4/C9=63/32/7, carry7masterticks later, both PPI1F,
complete published pulses960.109375 us. Root's normal-boot AmSpirit pair with
32/480 us padding instead gives initial0/0 and immediately clear polls. Its
assumed referenceC4/C9 tuple had been borrowed from a DIFFERENT local guest.
Exact software padding alone did not qualify hidden reference counters.
Four subsequent CPCSE loader attempts failed guest code/R7-write guards:
French key_output uses UK mapping; UK input reaches AMSDOS lookup but not the
guest at the saved checkpoint. No PPI/counter result is claimed from their
zero RAM, and no further timing/loader workaround was attempted.

## Independent pixel-address observation

One own diagnostic changes only guest-side VRAM initialization before the
existing synchronizer:8192 words hold101 followed by their13 bit physical word
index inC000..FFFF. Code, all16 KiB VRAM, DI/SP, RAM mapping, configuration and
normal input are guarded. No host RAM/PC/counter/beam patch is used. AmSpirit
Lite 1.17.1/core2491682,6128/CRTC1A/FR, uncropped LIVE framebuffer throughout.
Origin→OUTI start remains9788 us; the original OUTI takes5 us. No NOP/phase fit.

The independent three-word decoder first exhaustively proves8190 valid windows,
no misaligned or inverted-palette match,48 single-bit corruption rejections and
swapped/truncated rejection. It uses the known tag encoding, not emulator
counter outputs. Fresh palette colours select newly painted pixels; the LIVE
image dims the not-yet-painted suffix, which is excluded from decoding.

| Paused checkpoint | Freshest decoded PNG rows | First word/address | Consecutive three-word windows |
|---|---|---|---:|
|Before OUTI, tick8167694|350/351|400 /C320|38 each|
|After OUTI, tick8167699|350/351|400 /C320|38 each|
|Original PPI-result store, tick8167716|352/353|1424 /CB20|14 each|

Pixels, not beam.y, select the freshest valid word runs. Their difference is
1024 words: equal VMA low10=400, raster-address bits0→1. French§20.2p241's
rendered address diagram supplies the RAM-to-VMA/C9 bit mapping; current
motherboard wiring agrees (`crtc_vram_addr={MA13:12,RA2:0,MA9:0}`). The rendered
standard40 word/R9=7 sequence in§17.2.1p179 maps VMA400 to displayed row10.
The guest keeps R8/R5=0 and normal R4=38/R1=40; no address rupture is introduced.
Only C9bits 0..2 are directly encoded by CPC video addressing; the full-counter
nonterminal inference additionally uses the settled, unchanged R9=7 recipe and
its regular eight-raster row sequence. It is not a direct read of full C9.
The adjacent visible rasters are both nonterminal under that constraint, without claiming an exact
instantaneous busC0 from canvas coordinates or restoring nonexistent v2 fields.

The original later IN still stores bit0=0; no full pulse width is observed.
This does not exclude an unobserved short glitch between checkpoints. It does
exclude using the old local tuple as a measured reference counter, and supplies
an independent row/raster constraint for the next author/hardware question.

## Interpretation boundary

French§16.4.2p171 says a matching Type1R7 write starts VSYNC immediately and
counts the current partial line. A C4-only delayed reevaluation/inertial model
would preserve equality across this nonterminal carry; the new AmSpirit sample
cannot be used as its positive full-width oracle. The original hardware B6
photograph samples a terminal case and does not distinguish a terminal-C0 rule
from C4 carry/visibility. No physical nonterminal verdict follows from software.

CPCSE22de765's source defaults Type1live writes to allowed, with no C0/raster
exception (`crtc.h:110,522,527`; `crtc.cpp:436–438,759–782`), conditional on
its reentrancy latch. This is source prediction, not a successful run of these
new guests; character-granularity I/O cannot establish the subpixel aperture.
AmSpirit remains the closest software reference, not an authority overruling
hardware or the French rule.

Private reproducible inputs, four PNGs, snapshots, exact identities, decoder and
analysis are in `resumed-20261104/r7-counter-optical-qualification/`;
`r7-inertial-pair/` retains the earlier pair/failed loader evidence. No timing
candidate, source-derived expected value or retained test was changed here.

Technical information sourced from the Amstrad CPC CRTC Compendium by Longshot
(CC BY-NC-ND4.0).

## Subsequent bounded CPCSE loader qualification

One UK coldboot uses the explicit R7I0.BIN filename and a 10-second post-input
wait. All 623 guest bytes match, hide/target R7 writes occur at expected PCs,
and the final snapshot reaches the original HALT/SP guard. PPI bit0 is 1,
normal fall count 64 with timeout 0; target trace is C0=0/C4=10/C9=1 at
character-rounded dispatch. Root viewed the final PNG and checked provenance.
The combined loading changes qualify that recipe without isolating the old
extension-fallback versus motor-delay cause. No sibling was run. This supplies
a successful software peer observation, not a subpixel or physical verdict.
Evidence: resumed-20261104/cpcse-r7-loader-qualification/.
