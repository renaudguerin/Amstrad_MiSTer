# IA-2 type-1 frame-origin independent review

## Scope and verdict

**CLEAR — Gemini 3.7 Flash high through the guarded read-only bridge, 2026-08-31.**

The reviewed delta is the IA-2 type-1 frame-origin correction in
`rtl/crtc_type1_engine.v` with its directed and reconciled expectations in
`sim/sim_main.cpp`. The documentary rule is French ACCC v1.11 section 19.5.3
p.209: at frame start ParityC9 equals ParityFrame, and the worked table starts
an odd IVM frame at C9=1. This review confirms the local source model; it is not
hardware evidence.

A guarded Claude Opus review was requested first but did not run because the
provider reported that its session limit would reset later that day. No Claude
verdict exists for this delta. The independent review therefore used the
available cross-provider Gemini route and records its exact tier rather than
claiming Opus coverage.

## Load-bearing review finding and remediation

The initial Gemini pass found one blocking priority mismatch: `pc9_write`
qualified the stage-B ParityC9 write with `tog_enter`, but `pc9_value` selected
`stage_b_pc9_value` for every stage-B edge. A leaving-stage-B edge coincident
with a frame origin or an even-R9 row toggle could therefore assert the write
through the coincident event while selecting the leaving-stage value.

The value mux now uses the same `(stage_b_edge && tog_enter)` qualification as
the write strobe. Stage A and entering stage B retain their explicit ParityC9
semantics; leaving stage B writes only ParityFrame and therefore falls through
to the frame-origin or row-toggle ParityC9 value when one coincides. The narrow
remediation re-review returned CLEAR.

## Test integrity and acceptance

- `t32a` reaches `(ParityFrame,ParityC9)=(0,1)` with even R9 before the origin.
  With the RTL correction absent, it fails with ParityC9 actual 0 versus
  expected 1, so it distinguishes the stale toggle-both model.
- `t28a` pins both even-to-odd and odd-to-even origins, including C9 and
  ParityC9, so the correction is not accepted from a one-direction fixture.
- Full simulation passes with 177 required classic vectors and the integrated
  Plus/peripheral suites.
- Lint passes.
- The deterministic soak reproduces `0x654a244c2cce6e0b`.
- `git diff --check` passes.

The exact OUT R8,0 leaving-stage edge coincident with a frame origin is not a
separate directed vector. The corrected shared qualification removes the
reviewed combinational hazard, and the existing leaving-stage fixtures plus
the origin tests cover the constituent operations. A hardware trace remains
the external confirmation boundary for the Compendium model.

## Narrow remediation re-review verdict excerpt

> ### IA-2 Remediation Re-Review: **CLEAR**
>
> The `(stage_b_edge && tog_enter)` value-selection fix exactly mirrors the
> write-strobe condition. A leaving Stage B no longer selects
> `stage_b_pc9_value`; on coincidence it correctly falls through to
> `frame_new_w` or `c4_increment_toggle`. `t32a` strictly distinguishes the
> stale toggle-both logic, and `t28a` verifies the complementary odd-to-even
> origin. `make -C sim` and `make -C sim soak` pass; the soak hash is
> `0x654a244c2cce6e0b`.

The excerpt preserves the reviewer's verdict and substantive closure finding.
The preceding sections retain the complete decision basis without carrying the
terminal report's repeated file links and narration into a fresh session.
