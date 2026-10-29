# Combined private CRTC1 disk delivery

## Final artifact

The private disk is delivered at `output_files/crtc1-real-cpc/CRTC1-COMBINED.dsk`.
French instructions are adjacent in `GUIDE-FR.md`. Both are ignored, not released.
SHA-256: `4e30b18d3db73ad962e4c3370f68a2f0ac75ccd4f6a3a741b98ab4499536a874`.
The disk contains copied BCOPY synchronization bytes for this private diagnosis;
do not redistribute it as a public diagnostic release.

Use the original classic CRTC1 CPC at normal speed, not the 6128 Plus/CRTC3.
Power-cycle before each separate program. Run CONTROL first, then BEFORE0,
BEFORE1, AFTER0 and AFTER1. Preserve labelled whole-screen photographs or short
clips. CONTROL cycles through advancing A/B/C pages; capture all three.
BEFORE results require the T2 label and AFTER results require F2 next to the
third character-row result. Mixed or unreadable phase labels are inconclusive.
A short video is preferable when a photograph mixes phases.

SCOPE0 and SCOPE1 are optional EXPERIMENTAL cases, not validated policy tests.
Capture the whole display but do not infer policy unless their source-active,
source-retirement/sentinel and rearm controls all satisfy the French guide.
Their AmSpirit source-retirement control remains invalid/inconclusive.

## Validation and limits

CPU checks cover accepted write phases, first/steady trials, startup offsets,
relocation and full glyph witnesses. The final independently extracted low-load
body executes its own relocation from 4000 to 8000; no hidden body is injected.
All seven programs launch through normal AMSDOS in AmSpirit with readable IDs.
CONTROL advances all three pages; both BEFORE programs show N00 with T2 and
both AFTER programs show O01 with F2. These observed emulator values are not
original-CPC expectations. Competing outcomes were declared before testing.

The final SCOPE correction changes only two wait bytes in each SCOPE body.
The other five accepted bodies are byte-identical to the previously checked
low-load disk. Live reruns show complete rearm glyphs after the correction,
but premature B tokens still invalidate the scope interpretation in AmSpirit.
No cause is assigned to that discrepancy without accepted-write observations.

The earlier high-load disk failed live firmware loading despite valid headers;
that artifact and its failure evidence are preserved and must not be delivered.
The replacement's load/entry4000 range ends7F7F; the CPU then relocates16224
bytes to8000..BF5F. All final live boot receipts cover the low-load package.

Private sources, traces, package checks and full guide are preserved under
`local/investigations/amazing-lower-2026-10-29-v3/continued-evidence/v3-combined-crtc1/`.
Independent boot evidence is in the A bundle's `combined-crtc1-lowload-validation/`
and `combined-crtc1-scope-readout-validation/` directories. No physical CPC
result has yet been collected. The FIELD repair is separate, integrated and
hardware-tested; this disk does not change the core's RFD/parity policy.
