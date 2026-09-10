# IA-1 type-0 R3-terminal HSYNC independent review

## Scope and verdict

**CLEAR — Gemini 3.7 Flash high through the guarded read-only bridge,
2026-08-31.**

A GPT-5.6 Sol xhigh deep worker implemented the bounded IA-1 delta in
`rtl/CRTC.v` and `sim/sim_main.cpp`. Gemini independently reviewed the
uncommitted diff against French ACCC v1.11 sections 15.3.2-15.3.3 pp.150-151,
the existing F20 phase model, and the complete `t33` oracle. The reviewer did
not edit or delegate.

## Source-model boundary

The French source says that on type 0, modifying R3l at the old terminal count
while C0 again equals relocated R2 restarts HSYNC without resetting C3l. The
p.151 chronogram describes the earliest restart as approximately 3.5 Mode-2
pixels after the prior pulse ends. English omits the earliest qualifier.

The implementation maps that approximate value to 14 master ticks only for
the deliberately controlled `t33b` bus phase, using the already-reviewed F20
scale of four master ticks per Mode-2 pixel. Neither the code comments nor the
test promote 14 ticks to a universal CPU-bus or hardware timing oracle. Real
type-0 hardware or SHAKER evidence remains required.

## Failure-first and counter separation

The paper route starts with R2=11/R3l=10, relocates R2 to 21 during the first
pulse, and writes R3l=1 at the old terminal collision C0=21/C3l=10.

On unchanged RTL, `t33a` already passed the documented C3l sequence
10,11,...,15,0,1. `t33b` failed independently at the first post-write phase
sample: character 21, tick 3/16 expected HSYNC low but observed it high. The
live C0==R2 comparator had restarted the pin one master tick after the old
pulse ended. This proves that the correction is pin-phase work, not a retrofit
of a failing counter oracle.

## Trigger, priority, and lifecycle review

The reviewed trigger is type-0-only and requires an active pulse, C0 equal to
the relocated R2, C3l equal to the old R3l terminal, and a genuine nonzero R3l
change. Rewriting the same R3l, changing only R3h, R3l=0, and type 1 do not arm
the path. The bus edge detector prevents repeated arming from a held write.

On the collision edge the ordinary terminal path drops HSYNC while the new
countdown is armed. During the low interval it takes priority over the still
live ordinary comparator and any R2.JIT pending start; on the fourteenth tick
it raises HSYNC without resetting C3l. Reset, snapshot load, and a live switch
to type 1 clear the pending state. Both new state registers join the soak
projection.

`t33b` includes the no-R3-write control. `t33c` covers reset, snapshot load,
live type switching, and R3l=0 suppression.

## Acceptance

- `make -C sim`: PASS, 181 required classic vectors plus all integrated Plus,
  Gate Array, and u765 suites.
- `make -C sim lint`: PASS with existing non-fatal warnings.
- `make -C sim soak SOAK_EXPECT=0x87a9d80a91381c9b`: PASS.
- `git diff --check`: PASS.

The hash moved from `0x654a244c2cce6e0b` because two new behavior-bearing
phase registers joined the sampled projection; changing the sampled field
sequence itself requires a re-mint. The directed `t33` vectors, not the soak,
prove the specific terminal-collision rule.
