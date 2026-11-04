# SHAKER DI02E3: first-IRQ proof boundary — 4 November 2026

The historical CRTC1 page prints58 against photograph59. Source extraction now
identifies the exact first-interrupt return-PC measurement. It does **not** yet
identify a faulty plane or justify adding a fixed delay to CRTC, GA or CPU.
The fresh original-caller AmSpirit result is59 and the native275af8a result
is58; raw physical generation/acceptance timing remains unassigned.

## What the original code measures

Current disk6b9ff106…0eb61 contains SHAKE27D payload27060bytes at3A00,
SHA25665a90825dc674953b5dd9d4918bddf8079ea44877d89b08542b7fabb6b8d5ec5.
Module D labels testI “NOT RELIABLE ON CRTC1”. Entry873F installs the DEC DE
pattern generator89B6 through8892. Runner8891 generates the1B sled0100..0E16,
installs EI/RET at0038, and waits18interrupts with HALT/DJNZ. It then copies
the custom ISR89AA to0038, sets SP8A5C, HL0100 and B30hex, and jumps into the
sled. The ISR decrements B, reads FF, executes EI and JP(HL), leaving each
interrupt's return PC on the descending stack. Exit restores caller SP and
sets IX8A5A. At87C3, LD H,(IX) reads the **first** pushed PCL, which is formatted
into string8683. The48-interrupt loop does not make the printed value an average.
Original expected text distinguishes58 for CRTC3/4 and59 for other types.

Frozen extraction/disassembly is in ignored
`local/overnight-20261103/resumed-20261104/di-02e3-source/`.
The source-only Gemini report is useful retrieval, not an accepted causal
verdict. Its proposed shortened2-IRQ program does not inherit the original
caller's expected59 without independent phase qualification.

## Fresh original-caller reference

Parent executed the prepared normal-boot6128/CRTC1/128K/FR capture once on
AmSpirit1.17.1/core2491682, same disk6b9. Original NOP and CP(IX+d) runners
remain before DEC DE. At87BA the original stack/return84F2 qualifies entry;
at88C0, after the ORIGINAL18HALTs, the full3351-byte1B sled, original ISR,
B30/SP8A5C/HL0100/IX88C1/IM1/IFF1 and all14CRTC register values are checked.
The actual caller usesR3=0E, unlike the generic IRQ fixture's8E.

The first0038 interrupt hasSP8A5A andB30 before DEC B, proving its ordinal.
Stack8A5A contains5907, hence **firstPC0759**, PCL59 and1625completed DEC DE
instructions. DE=F9A7 independently corroborates that count. After the original
48ISR loop, the first stack value is unchanged;87CC formats59 at8683. Parent
inspected the final original02E3 screenshot: DEC DE59, SET44, CPindexedC5/C5
and the remaining controls are visible. This confirms the software caller
result without asserting59 as a synthetic fixture expectation. In particular,
PCL59 is **not** proof of only89instructions or a firstPC0159.

No state/code/counter/PC patch, request injection, shortened calibration or
beam-conditioned CPU release occurred. The original keyI coroutine and its2s
wait finish naturally. Classic SNA is version2; hidden CRTC/GA phase remains
unobserved. Timestamps are instruction-level microseconds, not a64MHz oracle.
Evidence and script are under ignored `resumed-20261104/di-authentic-proof/`;
run `reference-request/runs/original-di-001/` is complete. No production IRQ
change follows solely from this final-number agreement.

A matching direct D-I MiSTer run on275af8a/CRTC1/Full/French/current disk
completed10commands/onecapture/zero failures/oneSSM. Parent inspected02E3:
DEC DE58 remains, SET40 and CPindexedC2/C2 take other source-listed variants;
pending-INT/R52/break controls remain00/0018/00. These CPU-computed differences
cannot be caused by the screenshot sync filter. Evidence is
`resumed-20261104/device/di-1-275af8a/`. This is a targeted page, not a fresh D
baseline. No CRTC/GA IRQ fix has been inferred from the screenshot alone.

## Source and current test limits

Parent read French ACCC1.11 §27.7.2 renderedpp288–290. The source describes
HSYNC-end phase variation and examples in which a small delay changes whether
the Z80 accepts an interrupt after this or the following instruction. NOP/HALT
are robust examples; the source also reports differences among type1 chips.
French §16.2.2 p166 places HSYNC edges1–2Pixel-M2 around CRTC CLK depending on
type and circuit tolerance. It does not specify one universal positive
propagation offset for every register width and instruction phase.

`sim/classic_irq_phase_test.cpp` checks production CRTC→GA→T80 sampling against
an uninterrupted instruction stream and the **observed** GA IRQ falling edge.
Its N anchor is a NOP T2 edge selected relative to that request. Reported
HS−N/IRQ−N values therefore do not independently prove that physical IRQ is
one or two master ticks early. They can prove CPU acceptance relative to the
simulated pin edge while missing an absolute generation-phase error.

CPCSyntaxError v0.12.0 source models a late-edge instruction effect using
`lastBusAccessEnd` and `hsyncEndMissesInterruptSample()`. This is useful to
explain how that emulator obtains the page result. Its instruction-level
algorithm is not a physical pin timing oracle and is not a direct RTL recipe.
The decap-derived GA must not be changed merely to match this inference.

## Next bounded proof

Preserve the original18-interrupt alignment, first pushed stack byte, ISR,
sled and initial CRTC/GA setup. Observe actual HSYNC fall, GA IRQ fall, CPU
sampling and acknowledge through production T80/GA. Qualify the unchanged
original caller before asserting its photograph59 in a reduced fixture.
A changed calibration or injected request cannot silently reuse that expected
value. Keep setup, generation, acceptance and display acquisition as separate
possible planes until this proof identifies the first divergence.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
