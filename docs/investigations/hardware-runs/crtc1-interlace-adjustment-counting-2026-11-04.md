# CRTC1 interlace counting during adjustment — 4 November 2026

The native S state trace exposed a source-defined C9/C4 counting defect.
An active vertical adjustment incorrectly selects plain counting even while
effective IVM remains active. A private deterministic proof now fails for
both frame parities. The focused implementation passes240 CRTC cases and its45-bench selected
gate and fresh independent review. Neither S closure
nor new native acceptance is claimed.

## Rule and counter proof

French ACCC v1.11 §11.8 p93 explicitly retains the CRTC's interlace counting
during R5 adjustment. Its example uses R4=75, R9=7, R5=8 and R8=3: each row
has four physical lines, so C4 increments twice over eight adjustment lines.
§19.8.2 p226 gives Type1's masked row comparison and parity-preserving C9
step. Parent and worker read the rendered French pages directly.

The proof uses that geometry, R0=63 and R7=77. Ordinary counting naturally
reaches the first additional line at C4=76/C5=0, with C9 equal to frame parity.
No hidden counter or state is forced. For odd parity, the preceding even
frame is allowed to complete before the same natural entry is checked.

| Boundary after entry | Even C9 | Odd C9 | C4 | C5 |
|---|---:|---:|---:|---:|
|0|0|1|76|0|
|1|2|3|76|1|
|2|4|5|76|2|
|3|6|7|76|3|
|4|0|1|77|4|
|5|2|3|77|5|

The first boundary fails before any RTL change: even C9 is1 rather than2,
odd C9 is2 rather than3. At boundary4 both still have C4=76 rather than77.
C5 and active-management assertions pass throughout. The proof also checks
raw and published VSYNC by the fifth line: row77 is reached after four lines,
and that window includes the even-frame MID-VSYNC phase (§16.4.2 p171,
§19.7.2 p219). Both pulses are absent on the old engine. Eighteen assertions
fail in one build/run with both natural setup guards passing; source hashes
are unchanged before/after. The endpoint at C5=8 is outside this first proof.

## Consumer boundary

The defect spans more than C9's arithmetic. The adjustment branch also
replaces the row event used by C4, the row-structure event used by VMA and
VSYNC, and row-reset/re-engagement consumers. They require a consistent
IVM-aware ordinary row identity. C5 counts physical lines independently;
its endpoint and the extra interlace-line override remain separate owners.
A minimal fix must preserve canonical frame exit and added-line priority.

French §11.3.2 p87 still requires management/C5 to persist when active R5
becomes zero. This finding does not justify clearing either at R8=0 or an
ordinary R4 reset, and does not replace the photographed E0080 positive-R5
restart rule. The original Q20 correspondence is unavailable locally; the
tracked round2 response is a technical summary, not a literal quotation.

## Peer source and its limits

Pinned CPCSyntaxError v0.12.0 (`22de765`) also keeps IVM row counting during
adjustment. Its shared `updateVerticalType1()` re-entry path resets C5 when
the R4/R9 latch matches, including a positive-R5 re-entry; a plain R5 write
alone does not reset it. Its retained-zero R4 re-entry instead disarms the
state and can call `newFrame()`, unlike our author-resolved row-only reset.
That distinction prevents copying its lifetime rule just because S passes.
No peer hidden C5 cut was measured, so importing native C5=24 to explain its
4E40 would be unsupported. Source hashes and parent qualifications are in
`resumed-20261104/cpcse-c1-adjustment-source/`.

## Relationship to S

The [qualified passive pair](shaker-s-passive-observer-2026-11-04.md) has
IVM active at both R5/R7 cuts. Over sixteen physical boundaries, direct C4
advances four rows while history, already in adjustment, advances two. That
matches the current engine's counting distinction. History also carries
C5=24 into R5=8, reaches its endpoint after sixteen lines and enters an added
line before measurement. Correct row counting alone need not restore4E40.
The origin of that C5 phase and its correct later engagement remain separate
questions; no scalar is used to fit a reset rule.

Private source report, rendered pages, source plan, one-shot red and VCDs:
`local/overnight-20261103/resumed-20261104/c1-s-live-state-analysis/`.
Tracked implementation, retained red, gates, review and native results will
be recorded here when complete. No broad baseline is part of this unit.

Technical information sourced from the Amstrad CPC CRTC Compendium by
Longshot (CC BY-NC-ND 4.0).

## Candidate implementation and focused result

One shared ordinary row predicate now selects the effective IVM row boundary
inside adjustment as well as outside it. C9 progression, actual C4 commit,
VMA/VSYNC row structure, retained-zero R4 reset and positive re-engagement
eligibility use that identity. C5 completion, the inferred restart policy,
added-line ownership and canonical origin retain their separate priority.
No wrapper, new flop, C5 lifetime reset or scalar-specific rule was added.

The retained t08i/j/s extensions fail before RTL (237pass/3fail), then pass.
The two old t28a/b expectations of plain C9 counting during IVM also fail after
the correction; their red logs were preserved before updating only the C9
mode sequences and associated comments against French§11.8/§19.8.2. Their
C4/C5, extra-line duration, frame parity and origin assertions remain intact.
Final focused result is240/240. Intentional soak402410acec7f4988 uses the same
seed/projection and2,845,088 samples; the committed C0 golden row is retained.
The final `python3 sim/select_tests.py --run` passes45 benches once after the
last source edit. Parent verifies the frozen three-file hashes and unchanged
wrapper/Type0 owners; no gate rerun. Fresh Gemini4Argon high review20261104T221327Z-92939-f023 accepts the frozen
change with no findings. Parent rechecks all source and unchanged-owner hashes;
no code edit follows the gate. The reviewer's combinational non-IVM-equivalence
argument is scoped to ivm=0, not a claim that every prior IVM history or native
behavior is proven. Synthesis and native acceptance remain separate; no new
S result is inferred.
