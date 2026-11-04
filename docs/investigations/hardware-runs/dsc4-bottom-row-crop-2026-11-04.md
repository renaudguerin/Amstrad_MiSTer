# DSC4 CRTC1 bottom row: reference crop — 4 November 2026

The apparent corrupt last line is also present in AmSpirit's uncropped buffer.
The clean default reference excludes it. This establishes a coverage difference;
it does not justify changing the CRTC or choosing new global monitor bounds.

## Matched observations

MiSTer's source `db4be80` Full capture is768×273. Its static image, including
row272, is byte-identical to the previous `bedb1c5` capture, SHA256
`d811cce0582fcaa25c098194898e07d36dcace76ac8c77c71d793c9e4e88f373`.
CRTC0's corresponding bottom row is black. The [classic milestone](classic-blank-alignment-2026-11-04.md)
records the RBF, media, settings and native captures.

The original genuine AmSpirit CRTC1 image-at43s snapshot was loaded unchanged,
with CRTC retained, then allowed to render complete frames. No RAM, PC or
counter was patched. Paused1.17.1/core2491682 frame224923/tick119808 supplied
both captures: `crop=true` and `crop=false`, with `full=true&live=false`.
The [documented screenshot endpoint](https://raw.githubusercontent.com/AMSpiriT-Emulator/amspirit-releases/94c101bfab15039c89187a54d99fa8fb5322aba3/docs/lite/web_api.md)
exposes this full-buffer view; it is not a different emulation model.

The new cropped PNG is byte-identical to the original clean reference, SHA256
`2b686314eea0981b5fa230eed3ffd96f327e7e3dc08d253a35e18e608ff4177c`.
It is also an exact RGB subrectangle of the simultaneous uncropped PNG.
The initial script stopped because its state-output directory was absent;
creating that directory and finishing the captures required no reload, resume
or repeated emulation. That failed attempt remains recorded.

## Exhaustive row comparison

The uncropped PNG is1024×700, with each native scanline doubled. An exhaustive
search of all700 PNG rows and horizontal offsets0..256 finds exactly two
zero-error matches for MiSTer's entire last row: PNG rows654/655 at x171.
They represent one unique native source row327. All768 pen-class positions
match, including all108 coloured pixels. This uses four explicit DAC colour
correspondences, not raw RGB equality; the two black pens are indistinguishable.

The response headers independently report crop `[171,55,768,271]`: native rows
55..325, or doubled PNG rows110..651. The matched native row327 lies outside
that crop, at crop-relative row272. No viewport was fitted to obtain the match.

Earlier localization also identified the saved Mode0 VRAM stream at MA33AB,
RA0, physical bytesC756..C7B5 as the source of the coloured tail. The full-row
comparison removes that earlier partial match's unexplained black-prefix gap.

## Consequence and remaining boundary

`rtl/crt_filter.v` begins acquisition on vborder30 and ends at the horizontal
end of vborder302, admitting273 line slots. Its coverage differs from this
reference's271-row viewport. The observed tail therefore cannot be called a
MiSTer-only pixel-generation failure on the evidence of the cropped image.
AmSpirit agreement remains software evidence, not proof of a real CPC's
uncropped signal or its monitor's visible area.

No filter, GA or CRTC behavior changed in this investigation. Desired monitor
bounds need their own contract; the comparison alone does not select a new
height, origin, or automatic crop. Backlog B1 owns that presentation boundary.
A B18 v3 snapshot saved from the device would give actual RAM/counter
observations if further discrimination is needed. It would not make a faithful
replay automatic: hidden GA/filter phases are absent, and the current classic
loader does not restore those captured counters.

Evidence remains under ignored `local/overnight-20261103/classic/`:
`image-uncropped-reference/`, `bottom-line-localization/UNCROPPED-HANDOFF.*`,
`uncropped-comparison.json` and the analysis script/log. Original evidence and
the earlier partial localization remain intact.
