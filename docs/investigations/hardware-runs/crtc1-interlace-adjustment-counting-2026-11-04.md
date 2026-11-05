# CRTC1 interlace counting during adjustment — 4 November 2026

The native S state trace exposed a source-defined C9/C4 counting defect:
an active vertical adjustment incorrectly selected plain counting even while
effective IVM remained active. A private deterministic proof failed for
both frame parities. The focused implementation (`ccf2be7`) passed 240 CRTC
cases, its 45-bench selected gate, and fresh independent review. On exact
full build `076eab1` and parent-verified diagnostic build `c16e7ec`, native
hardware execution confirms bounded resolution: Test S prints 4E40 in both direct
entry and the full canonical history walk, matching the qualified baseline direct
S image. The live state trace on `c16e7ec` confirms that history arrives without
inherited adjustment (C5=0, adjustment OFF). Neither comprehensive lifetime
policy, phase robustness, nor physical photo compliance is claimed closed.

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
behavior is proven. Synthesis and native acceptance were validated separately on
full build `076eab1`, detailed below.

## Native verification on full build 076eab1

Clean full Quartus synthesis and native hardware execution confirm bounded resolution
of Test S under the corrected counting rule:

- **Source and build**: Functional change `ccf2be7` is the sole C1 change in exact
  full build source `076eab11a948e91e44279f5ef128c924a26b92ea`. CI workflow
  41925025171 passed all required jobs (artifact 13370705502, build 368).
- **Bitstream and timing**: `Amstrad_20261104_076eab1.rbf` (3,139,692 bytes, SHA256
  `7d7fc3f6f1cc2ea6197a05cfc273ca4bc308e7e6a2b14bfc3774ffac1b6cb4b6`). Built with
  clean_full Quartus 17.0.2.602 (Standard Fit / High Performance Effort), 7 setup and
  7 hold clock groups, TNS 0, min slack +0.539 ns setup / +0.242 ns hold, zero errors
  and zero critical warnings, with the observer macro turned OFF. Installed bitstream
  hash matched on the target device.
- **Native S verification**:
  - Original direct S completed 10 commands, 1 capture, 0 failures, 1 SSM record,
    printing 4E40.
  - Canonical history through S completed 131 commands, 64 captures, 0 failures,
    121 SSM records, printing 4E40.
  - Full RGB captures for direct S and history S are identical to each other and
    identical to the qualified `9a91920` direct 4E40 baseline image (SHA256
    `b03c5e572b9d72e2a0cfb56b5c687f2e089b52d5cf38150f97fcee2dd50f4e2d`).
  - Preceding checkpoints E (`02C9`/`02CA`) and P (`02CB`) full RGB are identical
    to baseline 9a (E prints 0080, P prints 26). Root viewed full S, E, and P
    images.
- **Corpus comparison and bounds**:
  - Across all 64 captures from the canonical history prefix against the exact 9a
    baseline, 53 images are RGB identical and 11 changed (including S). The remaining
    10 changed images contain raster patterns. Parent inspected all64 contact views
    and all11 enlarged paired panels; no physical-photo verdict is refreshed.
  - Title regression checks: C1 DSC4 completed 4 views root-inspected (clear colons
    and names, visible otter and rotated fire effects); Amazing completed 2 views
    (visible logo, text, and QR code, showing bounded regression only).
- **State trajectory and remaining limits**:
  - In production build `076eab1`, the observer macro was OFF; the earlier
    native C5=24 reading belongs strictly to pre-fix traces.
  - Parent-verified diagnostic build `c16e7ec` (branch `codex/s-passive-ivm-native-20261104`,
    commit `c16e7eca6c574410d0f5d2b2cfee9bd30c19d898`, exact `076eab1` plus ONE
    QSF enable `SSM_S_PASSIVE_TRACE=1`, workflow 41933969717 PASS, artifact 13375070713,
    build 372; bitstream `Amstrad_20261104_c16e7ec.rbf`, 3,169,908 bytes, SHA256
    `e6e6b848fff15d3e37c1bf5948aeb1c36654b856409955eb7d8ee01201f75f5e`, clean_full
    Quartus 17.0.2.602, 7 setup and 7 hold clock groups, TNS 0, min slack +0.398 ns /
    +0.246 ns, zero errors/critical warnings) completed native execution. Direct S
    (10 cmd, 1 capture, 0 fail, 16 SSM) and canonical history (131 cmd, 64 captures,
    0 fail, 136 SSM) both produce exact S PNG SHA256
    `b03c5e572b9d72e2a0cfb56b5c687f2e089b52d5cf38150f97fcee2dd50f4e2d` (4E40), with
    E02C9/02CA (0080) and P02CB (26) RGB identical to default `076eab1` and baseline 9a.
  - Newly observed hidden state: in canonical history, the caller already arrives at
    A4CF with C5=0 and adjustment OFF (before acquisition). A067 and A06B are all
    0 / off. Before R5 at 5BFB: C0=58, C4/C9/C5 = 0/0/0, adjustment OFF, effective
    IVM=1. At R7 fetch 5B0A: C0=13, C4/C9/C5 = 4/0/0, adjustment OFF, added line 0,
    effective IVM=1. At exit 5B17: C0=17, C4/C9/C5 = 0/1/0, adjustment OFF, parities=1.
    Direct entry observes identical R5, R7, and exit states.
  - This replaces the old pre-fix history state (A4CF C5=31 / adj ON, 5BFB C5=24 / adj ON,
    and 5B0A C4=2 / adj ON / added line 1). Do NOT say new code clears C5 by R8 write
    or adds a reset policy; it corrects IVM row counting during adjustment, and the full
    canonical prefix now naturally arrives at S without inherited adjustment. The source
    fix remains independently derived from French ACCC §11.8 p93; this is observed
    bounded state normalization, not scalar coincidence or comprehensive C5 author adjudication.
  - Diagnostic `c16e7ec` canonical history vs default `076eab1`: 51/64 RGB identical, 13
    raster pattern views changed (`MISTER_1_022B.png`, `022C`, `0230`, `0231`, `0233`,
    `0236`, `0239`, `023A`, `023B`, `0287`, `0288`, `0298`, `0299`), while S, E, and P
    are identical. Parent inspected all 64 contact views. Each of the 13 changed RGB contents
    exactly matches a previously inspected view in the 076/9a/ae9/599 corpus;
    no novel content was found. No physical-photo verdict is refreshed.
  - The physical photograph CS_02D3_A.webp (4E30) versus current program (4E40)
    revision binding remains open. Scope is a targeted history walk, not a broad
    baseline or physical photo rejudgment. See the [passive observer qualification](shaker-s-passive-observer-2026-11-04.md)
    and [S history investigation](shaker-s-history-2026-11-04.md).
