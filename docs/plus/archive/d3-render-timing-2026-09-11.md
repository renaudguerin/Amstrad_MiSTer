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
path report. The accepted full-fit result below establishes timing closure separately
from source equivalence. Existing hardware/title/flicker and
post-BSR PPI acceptance boundaries remain unchanged.


## Accepted full fit

Automatic run [34556279033](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34556279033)
passed simulation, synthesis policy, routing, local Quartus synthesis and the
required gate for `6a06ec907d4e8713f0470366405b2c315349fc7d`. The hosted
synthesis leg was correctly skipped. Artifact `Amstrad-local-build-203-1-full`
records `build_mode=clean_full` with Quartus 17.0.2.

The downloaded timing summary passes the repository checker: minimum setup
**+0.368 ns**, minimum hold **+0.228 ns**, zero TNS across seven clocks each.
The previously failing core clock now has **+0.844 ns setup slack**. Resources
are 22,950 / 41,910 ALMs (55%), 26,626 registers, 701,596 block-memory bits
(12%) and 35 / 112 DSP blocks (31%). There are no illegal or unconstrained
clocks. The existing 27 unconstrained input ports and 90 output ports match
the failed integration report; this check does not establish external-I/O or
hardware acceptance.

The exact artifact RBF is copied to
`output_files/Amstrad_20260911_6a06ec9.rbf`; downloaded and copied SHA-256 match:

```text
8667d4dadacf4f0bdb196e3b88d932de919eb74529b551afa9d289d45f333ab9
```

Artifact ID is `10183053996`; GitHub's archive digest is
`e3f84e8b6cb07231bd9a2ffeaaffc31d16a776aaf613131ce5acda92beb905f3`.
The reports and CI metadata remain under the ignored evidence directory.
This RBF includes D1/D6, D5 and D3/D4 and is ready for the recorded hardware
retests. Physical hardware results remain pending.
