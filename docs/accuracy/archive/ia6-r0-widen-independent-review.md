# IA-6 type-0 R0=1 widening independent review

## Scope and verdict

**CLEAR — Gemini 3.7 Flash high through the guarded read-only bridge,
2026-08-31.**

Gemini 3.7 Flash high authored the focused `t35a` failure-first vector in
`sim/sim_main.cpp`; a GPT-5.6 Sol xhigh deep worker separately authored the
phase-sensitive correction in `rtl/CRTC.v` and `rtl/crtc_type0_engine.v`.
Claude review was attempted once but hit its session quota and produced no
verdict. The authorized Gemini fallback independently reviewed the complete
uncommitted delta. It did not edit or delegate.

## Premise audit and failure-first evidence

The vector-to-premise table found existing coverage for the French v1.11
section 13.2.4 and 13.7.2 prerequisites: the C4/R4-unequal R5 comparator
route, the C0=0 normal-action conjunction, and the initial R0=1 C4=R4
condition. The only gap was section 13.7.2 pp.126-127's partially completed
true-last-line route after widening R0 from 1 at C0=1.

`t35a` programs R4=2, R9=3, R5=R8=0. Its safe control widens R0 at C0=0 and
observes the normal frame reset. Its main arm widens R0 1 to 7 at C0=1 on
C4=R4/C9=R9. On unchanged RTL the first post-write assertion failed:
expected C0=2, actual C0=1. The old R0 equality had driven both the documented
vertical decision and an incorrect horizontal wrap, so the model reset rather
than entering persistent additional management.

## Counter separation, priority, and lifecycle

The corrected type-0 accept condition is restricted to a true last line,
R0=1 to R0>1, C0=1, R5=0, R8=0, and no active adjustment. The accepting edge
consumes the old equality but suppresses a real horizontal line boundary and
holds C0=1 across the register write. A private one-character pending action
then advances to C0=2, increments C4 to R4+1, retains C9, enters adjustment,
and clears the consumed last-line captures. The widened remainder ends at the
new R0 and counts C9 through effective R5-1=31 before frame reset.

The suppressed edge does not advance the type-0 VSYNC-delay pipeline. MA
continues linearly instead of taking a row reload; DE/HSYNC use the continued
`hcc_next` path. Reset, snapshot load, live type-1 selection, and consumption
clear the pending action. Type 1 and the safe C0=0 route are unchanged. The
new state joins the soak projection.

## Acceptance and boundary

- `make -C sim`: PASS, 183 required classic vectors plus all integrated Plus,
  Gate Array, and u765 suites.
- `make -C sim lint`: PASS with existing non-fatal warnings.
- `make -C sim soak SOAK_EXPECT=0xd6bc1649ff2058a1`: PASS.
- `git diff --check`: PASS.

The implementation deliberately does not claim French section 13.7.2.1's
non-last-line overflow variant, positive-R5/interlace variants, or other bus
phases. The accepted result is documentary-model evidence, not real CRTC-0
hardware confirmation.
