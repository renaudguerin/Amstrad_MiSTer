# IA-4 type-0 R4-history independent review

## Scope and verdict

**CLEAR — GPT-5.6 Sol high, 2026-08-31.**

Gemini 3.7 Flash high implemented the bounded IA-4 delta in
`rtl/crtc_type0_engine.v` and `sim/sim_main.cpp`. A separate native Sol
reviewer then inspected the uncommitted diff read-only. The reviewer did not
edit or delegate.

Fresh extraction of French ACCC v1.11 section 13.2.1 p.106 confirms that the
ordinary next-line `C4=R4+1` path requires R4 to have remained equal to C4
throughout the line. Section 11.2.2 pp.82-83 confirms that a qualifying R4
modification switches the line-end comparison from C9/R9 to C9/R5. The
sticky-history correction is therefore supported by the documentary model;
it is not hardware evidence.

## Failure-first discriminator

`t16z` compares two histories with the same final register values. Both reach
the final line at C4=2/C9=3 and write R5=5 at C0=2. The control leaves R4
equal and reaches adjustment at C4=3/C9=0. The transient arm writes R4
2 to 1 at C0=3 and restores 1 to 2 at C0=4; the French history condition and
section 11.2.2 require C4=2/C9=4.

Before the RTL correction, the transient arm failed at C4: expected 2, actual
3. The old latch had been cleared by the restoring write, so the vector
strictly distinguishes the corrected history rather than merely exercising
the final equality.

## RTL and lifecycle review

Once an accepted window write makes R4 unequal to C4,
`type0_r4_adjust_switch` remains set for the rest of the character line. An
equal restoring write no longer clears or masks it. Reset, snapshot load, and
a live type switch clear the state. `line_new` has sequential clear priority,
while the combinational current-write term still lets an unequal exact-R0
write affect the imminent rollover without leaking into the next line.

No state or sampled-field topology changed: the existing one-bit latch keeps
its name, width, lifecycle ownership, and soak projection.

## Acceptance

- `make -C sim`: PASS, 178 required classic vectors plus all integrated
  Plus, Gate Array, and u765 suites.
- `make -C sim lint`: PASS with the existing non-fatal warnings.
- `make -C sim soak SOAK_EXPECT=0x654a244c2cce6e0b`: PASS. The hash is
  unchanged because the fixed randomized schedule does not reach this narrow
  restored-equality history; `t16z`, not the soak, carries the proof.
- `git diff --check`: PASS.

Real CRTC-0 hardware or SHAKER confirmation of the transient restore case
remains the external validation boundary.
