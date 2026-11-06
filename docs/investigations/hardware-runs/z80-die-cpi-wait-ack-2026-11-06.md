# Independent Z80 die-reference checks for CRTC3 BR

Five bounded transistor-netlist runs support the current CPU contract for two
questions in the remaining BR CPI mismatch. They select no RTL correction:
BR still produces CC against the physical/table C4. Original Plus CPU variant,
board propagation, complete BR antecedents and ASIC counter clearing remain open.

## Qualified reference

The private runner comes from [RZ80 revision6b1ffce](https://github.com/rejunity/z80-from-schematics/tree/6b1ffceab57561881e9cae429182dc3d4fc28038),
but executes its perfectz80 transistor solver, not the reconstructed RZ80 CPU.
Its netlist header is byte-identical to [v6502r revision1bfa22d](https://github.com/floooh/v6502r/blob/1bfa22d64a861f330bc4318755cfd91cb78c0c59/ext/perfect6502/netlist_z80.h).
All6,813 transistor triples and3,597 pullup flags match the acquired
[Visual6502 Z80 source](https://github.com/trebonian/visual6502/tree/d8ecc129b34e0eaf320e0400fcf33329475bdb1e/chip-z80).
The only runner diff appends read-only pin/state diagnostics to its existing
output. External WAIT node22 is driven and read back directly; no architectural
or hidden CPU state is injected. Reset and register setup use original normal
program instructions. This qualifies a discrete NMOS die-netlist reference,
not the exact CPU/electrical timing of an original CPC Plus.

## Fixed CPI WAIT discriminator

The selected first CPI follows a marker NOP, with ED/A1 fetched at0201/0202
and the operand read at8000. Manufacturer timing4/4/3/5 and existing upstream
cases fix the operand/internal T2 pulses; no phase search was performed.

| Case | WAIT-low half clocks | Following0203 fetch | CPI duration |
|---|---|---:|---:|
| Baseline | None |40|16T|
| Operand-read T2 positive control |26/27|42|17T|
| Internal-tail T2 discriminator |32/33|40|16T|

Half8 is the first ED fetch, so duration is `(next_fetch−8)/2`. The operand
WAIT holds MREQ/RD one clock longer. Internal WAIT leaves every public output,
address, data and drive-mask field unchanged. All three original18-column traces
match pre-existing upstream recordings exactly. The explanatory selection note
was written after execution but before inspecting either WAIT outcome; the
selected cases/events existed before execution. It is not a formal preregistration.

This supports the motherboard's idle-cycle WAIT bypass for this specific CPI
interval. It does not establish a universal policy for every internal cycle.

## CPI to ACK address retention

Two further runs use the existing normally initialized EI/IM1 caller, I=3F,
early INT on the first CPI and a return PC0203. The second adds one fixed WAIT
pulse across the second automatic ACK wait clock. Its contract was saved before
either run. No additional compile or phase scan was needed.

| Boundary | No external WAIT | One external WAIT |
|---|---:|---:|
| ACK M1 begins |40|40|
| Raw IORQ asserts |45|45|
| Active M1+IORQ rows |45–47|45–49|
| Active address / drive mask |0203 / FFFF|0203 / FFFF|
| M1/IORQ release and I:R handoff |48 /3F22|50 /3F22|

Raw IORQ asserts2.5T after ACK M1 starts. PC remains actively driven throughout
ACK, including the added wait; I:R reaches A13 only when ACK releases. Stack
writes8FFF=02 and8FFE=03 independently bind return PC0203. The no-WAIT caller's
original18 fields also match its pre-existing recording. Both0203 and BR's4A9B
have A13=0, so changing I supplies no direct route to earlier ASIC ACK in this
contract. The experiment does not reproduce BR's synchronization history or its
observed two READY waits, and supplies no numerical ASIC delay.

## Evidence and acceptance

Private evidence is under
`local/overnight-20261103/resumed-20261104/c3-br-cpi-die-wait-20261106/`.
Its REPORT, observer-only diff, provenance closure, source/input receipts, raw
25-field logs, unchanged upstream recordings and ACK extension are preserved.
Parent read both reports/diff and verified47 artifact/source/input hashes;
completed experiments were not rerun. The primary-source discovery report's
earlier unanchored netlist claim is superseded by these actual pin experiments.
No die-source code is added to the repository or release archive.

The [BR investigation](shaker-crtc3-br-ack-boundary-2026-11-06.md) retains the
original-CPC bus discriminator: CPUCLK, INT, M1, raw/shaped IORQ, READY/WAIT,
A13 and HSYNC with original antecedents and the resulting numeric result.
