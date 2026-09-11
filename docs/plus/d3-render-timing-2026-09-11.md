# Plus sprite render timing follow-up — 2026-09-11

The combined integration `f8e9372` passed behavioral CI, but its full local
Quartus run [34553543211](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34553543211)
failed setup closure: **−0.217 ns slack, −3.053 ns TNS** on the 64 MHz core
clock. Hold slack was +0.249 ns. The fitter completed at 23,045 / 41,910 ALMs
(55%), 26,747 registers and 701,596 block-memory bits. No accepted RBF was
delivered from that run, and the timing gate was not weakened.

## Fitted-path evidence

TimeQuest analysis of the retained exact-SHA database found all twenty worst
setup paths from `plus_sprites.hp` to `asic_vid.RGB_*`. The worst path starts
at `hp[2]` and ends at `RGB_G[3]_OTERM1881`: thirteen logic levels,
15.142 ns data delay, −0.499 ns clock skew and a 15.624 ns clock relationship.
Its data route crosses X equality, the offset mux, magnification shift, staged
pixel lookup, opacity/priority and RGB selection.

This identifies the failing render path. It does not establish that D3's
one-line prefetch change caused the delay; placement can move existing paths.
The report and analysis-only Tcl are retained under ignored
`docs/references/d3-d5-integration-2026-09-11/`. The query reused the completed
database, made no assignment changes and did not run another compile.

## Bounded combinational change

The source-pixel selector now applies fixed magnification slices to the two
data arms before the late X-equality/window mux. It preserves the previous
six-bit offset truncation and all four magnification-code decodes. The equal-X
arm still selects pixel zero, and negative-X overlap retains priority over it.
No register, enable, output latency, fetch/priority behavior, clock constraint,
fitting seed or effort setting changes.

The intended benefit is to remove the magnification shifter from the path
after the late horizontal-position comparison. Only a new fit can establish
the actual mapped timing benefit.

## Verification

The retained `check_source_pixel_equivalence.py` extracts the original
expressions from `f8e9372` and the current function/expression from the source,
compiles them together with Verilator, and checks all 2,097,152 binary input
combinations. They match, including every counter value, both window flags,
all ten-bit X magnitudes and all four magnification codes. This is scoped
combinational equivalence, not a physical timing proof.

Full simulation and lint pass. Fresh Gemini 3.8 Flash high review returned
CLEAR (run `20260911T024819Z-37796-cead`, clean exit 0), covering truncation,
all magnification codes, window priority, state/phase preservation and the
limits of the equivalence check. The review and logs are retained with the
path report. A new full-fit result remains required; source equivalence does
not establish physical timing closure. Existing hardware/title/flicker and
post-BSR PPI acceptance boundaries remain unchanged.
