# IA-3 type-0 live-R6 first-line conflict independent review

## Scope and verdict

**CLEAR — Gemini 3.7 Flash high through the guarded read-only bridge,
2026-08-31.**

One Luna leaf added the source-derived `t34a` failure-first vector in
`sim/sim_main.cpp`; a separate Luna leaf made the bounded wrapper correction
in `rtl/CRTC.v`. Gemini independently reviewed the uncommitted two-file diff
against French ACCC v1.11 section 18.3.2 p.191. It did not edit or delegate.

## Source-model boundary and failure-first evidence

The French source says that, on the first frame line with C4=R6=C9=0,
DISPLAY ENABLE is on at character start and off 0.5 microseconds later. The
alternation remains cancellable: writing R6 nonzero before a reachable C0=R1
prevents the earlier R6=0 conflict from becoming definitive. If live R6 is
still zero at R1, border becomes definitive.

`t34a` programs R0=7 and reachable R1=4. Its control leaves R6=0; its main arm
writes R6 0 to 2 at C0=3. On unchanged RTL the control failed at the first
frame-line half: expected DE high, actual low. The ordinary `row_next==R6`
assignment had overwritten the frame-origin display start. This was not the
anticipated passing guard, so the oracle was preserved and the mismatch was
fixed rather than normalized to current model behavior.

## Priority, live comparison, and lifecycle review

At a type-0 frame origin the ordinary R6 row-border assignment no longer
overrides the high start when R6=0. The existing half-character toggle then
produces the p.191 low second half. A dedicated type-0 latch is set only when
`hcc_next` reaches live R1 on row 0/line 0 while live R6 remains zero; that
same edge forces vertical display low and later toggles cannot reassert it.
An earlier R6 nonzero write prevents the latch from setting, while the normal
horizontal R1 border still applies locally.

Reset, snapshot load, live type-1 selection, and every row transition clear
the latch. The frame-origin priority exception is type-0-only; type 1 and
ordinary nonzero R6 row matches retain their previous behavior. The new latch
joins the soak projection so lifecycle leaks cannot hide behind pin sampling.

## Acceptance

- `make -C sim`: PASS, 182 required classic vectors plus all integrated Plus,
  Gate Array, and u765 suites.
- `make -C sim lint`: PASS with existing non-fatal warnings.
- `make -C sim soak SOAK_EXPECT=0x21bbf9c29ab08413`: PASS.
- `git diff --check`: PASS.

The directed vector establishes the local source model, not physical CRTC-0
hardware behavior. A real-chip trace or SHAKER discriminator remains the
external validation boundary.
