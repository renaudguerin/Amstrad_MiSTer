# RTL model confidence and upstream review

This is a source-checked starting inventory, not an exhaustive count of
exceptions or an architecture approval. It addresses the risk that successive
local timing repairs preserve an accidental model rather than expose a simpler
hardware rule. The initial inventory binds experimental8ba384e and its documentation
checkpoint. The later reviewed Plus R4 branch candidate is identified below.

## Cases needing explicit judgment

| Area | Current model / restriction | Evidence and remaining risk |
|---|---|---|
| Classic type0 R0 deadline | `type0_r0_eq_kept` samples true equality at nCLKEN, ORs live equality, explicitly excludes sampledR0=1. | French§13.6.1 p124 plus qualified original caller, four instruction bins and native A4/B6 support the narrow fix. Exact sample edge is inferred. The R0=1 exclusion is a coverage guard; its broader deadline remains unproved. The independently repaired p127 row result now matches all photographed A4 fields on493cdea; earlier seven-lower-failure claims were an E/F transcription error. It is not a proved silicon exception. Source: `rtl/crtc_type0_engine.v`, `type0_r0_at_sample`. |
| Type1 RFD lifetime | `rfd_parity_flag` retires on successful address save as well as frame end. | Existing photographs fit this model and a competing arm-time model. Peer source does not decide the silicon lifetime. The unsent French author question remains needed. Source: `rtl/crtc_type1_engine.v`, successful-save clear. |
| Plus R0 deadline and lowering | `entered_terminal_match` retains a terminal opportunity; live equality can also end the line. Nonterminal lowering uses equality-based overflow. | Type3 deadline chart and original Plus A4 support bounded outcome; physical latch/clock is unidentified. Nonterminal lowering lacks a direct Type3 chronogram/hardware result; a peer trace is weaker evidence. Source: `rtl/plus/asic_video.v`, horizontal counter. |
| Plus HS collision at zero width | End/start collision extends HS only for explicit nonzeroR3l. | Nonzero collision behaviour is sourced; excluding zero-width collision is explicitly an unverified model assumption. A source rule or original-hardware discriminator must decide it. Source: `asic_video.v`, `hsync_end_start_collision`. |
| Plus compatible IRQ delivery | Raw request reaches CPU at next CCLK_EN_N. | Original Plus observations bound delivery, but do not identify that exact edge. Existing counter/provenance tests protect interactions; they cannot establish silicon topology. Source: `rtl/plus/asic_ga_timing.v`, `classic_delivery_n`. |
| Plus split during adjustment | `split_adj_end` accepts a late split at the selected R0 boundary, limited to noninterlace. | Existing original-Plus/AmSpirit results support bounded cases; exact edge and early/late original-hardware controls remain unmeasured. Source: `asic_video.v`, `split_adj_end` and `split_at_r0`. |

The task branch now also contains a reviewed **Plus R4 deadline model**:
`entered_r4_match` contributes only during the selected R4 write at terminal
CLKEN, using existing READY/A13-shaped ENABLE. Independent ordinary/ROM REDs,
all17 original callers and46 selected benches pass. Full synthesis and targeted native E/DU/demo/owner acceptance pass; it remains
an experimental behavioral model. The rival model commits
R4's counter-visible value at write end; both fit every existing E row and
predict differently for late false→true writes. Neither exact silicon mechanism
is established. The B26 comment names this limit and the extension beyond
R4→0. No new port/decode is required. See the [E evidence](hardware-runs/shaker-crtc3-status-overflow-2026-11-06.md).

The planned audit, model selection, hardware discriminator and acceptance steps are tracked in [B26](../backlog.md#b26-consolidate-inferred-rtl-exceptions-before-upstream-submission). New inferred exceptions receive the `MODEL LIMIT (B26):` implementation comment required by [AGENTS.md](../../AGENTS.md).

## Meaning for upstream adoption

These modules use synchronous register-transfer logic; behavioural modelling
and pending state are ordinary FPGA techniques. Decap-derived classic GA logic
has a stronger structural basis, while the CRTC and Plus models are behavioural.
Synthesis success does not make their inferred rules correct. Conversely, a
large conditional expression may implement a documented chip quirk faithfully.

The explicit coverage guards and competing lifetime models above are technical
debt worth resolving before presenting the complete branch as upstream-ready.
Keep the evidence-backed narrow fixes separable. For consolidation, trace bus
visibility, comparator sampling and consumer edges together, then seek one
state/phase contract that also handles the preserved residuals. Do not remove
guards merely for visual simplicity or keep an assertion solely to protect a
legacy implementation: real hardware can invalidate either.

A structural refactor should reproduce the pinned soak and focused independent
counterexamples; intended behavioural expansion needs its own fail-first
hardware/source oracle. No RTL refactor or additional configuration switch is
introduced by this inventory. The next audit should expand this table across
both classic engines and Plus, count independent policies rather than grep
matches, and distinguish sourced quirks, empirical outcome models, coverage
limits and simulation-only restoration policy.

See [C0 proof and native limits](hardware-runs/shaker-crtc0-a4-qualified-phase-2026-11-06.md),
[RFD lifetime](hardware-runs/rfd-frame-lifetime-2026-11-03.md),
[Plus divergences](../plus/source-divergences.md), and
[unsent questions](../classic/accc-author-questions-2026-11-06-fr.md).


## First bounded source audit and candidate

The frozen classic8ba384e audit classifies nine policy groups, separating
sourced quirks, inferred outcome models, coverage guards and initialization
policy. French §13.7.2.1 p127 supplies an independent missing constraint:
R0=1 widening increments C4 when C9=R9 even on a non-last frame row. The
production-T80 cohort qualifies126/126 guards; baseline misses that increment
while true-last and C9-negative controls agree. The candidate separates row
advance from continued frame adjustment without new state. The original C0 B6
replay qualifies46/46 and preserves all ten output artifacts byte-for-byte.

A shared combinational C4 commit now feeds the independently proved R6 border
and ParityR6 consumers. The selected suite has253 CRTC passes and46 passing
benches. Root verifies all271 combined proof artifacts and56 additive R4=7
artifacts; Opus finds no RTL composition defect and permits an experimental
build. Historical T80 port wiring and extra frozen/true-last R6 route checks now
pass; immediate DE and R8=1 frame length are also pinned. Final selected gate
passes46 benches; supplemental Opus review permits experimental synthesis. No native/page or master/release acceptance yet.
[Implementation and proof](hardware-runs/shaker-crtc0-r0-nonlast-advance-2026-11-06.md).

| Candidate model limit | Evidence and unresolved boundary |
|---|---|
| Pre-CLKEN widening admission | Literal early OUT requires the result; admitting other pre-CLKEN writes, including late OUTI, is inferred. Restricted to !in_adj and R5=R8=0. Direct first-CLKEN non-last writes and the old-one horizontal deadline retain their previous model. |
| Off-seam numeric C4=C9=0 | DE and parity preserve the old behavior without manufacturing an origin. The DE conflict coordinates are sourced; the matching parity exclusion is a coverage decision. Ordinary seam zero/origin handling is unchanged. |
| Parity pre-edge !in_adj and R6>R4 | Inherited qualification admits the first forced adjustment-entry landing. French§19.5.2 p206 describes capture when C4 reachesR6, explaining R6>R4 freezing by unreachability; §19.6.1 p217 also states an unconditional R6>R4 freeze. The new true-last test chooses the event reading, including R6=R4+1. A comparator-gated rival predicts no capture. This needs a hardware/author discriminator; later adjustment behavior is also unproved. |
| Off-seam R4 and R7 | No frame validation or VSYNC event is synthesized from the new C4 commit. R4=7 has a qualified local CPU trajectory, not a complete physical frame verdict. R7 onset requires a hardware discriminator. |
| Adjustment and IVM landings | Reusing the actual C4 event does not establish every interlace/adjustment corner, alter pc9, or recover silicon topology. Targeted route checks cover only their stated geometry. |

These are recorded by `MODEL LIMIT (B26):` implementation comments. They are
not proved chip exceptions. Two late oldR0=1 horizontal models remain
admissible; removing the numeric sampling guard alone saves no established
state. The earlier seven-lower-A4 constraint is invalid: archived8ba native fields
were FEFF×8, notFFFF. Onlyrow4 differed;493cdea corrects it toFFFF. All
photographed lower pairs now match, with extraC9sweep still ungraded. Use the
[corrected constraint](hardware-runs/shaker-crtc0-a4-native-glyph-correction-2026-11-06.md)
for future audits rather than the frozen report's misread digits.

Private immutable inputs/proofs remain under the active session evidence root:
`b26-audit-frozen-20261106/`, `b26-audit-results-20261106/`,
`c0-r0-p127-readiness-20261106/`, `c0-r0-p127-candidate-20261106/`,
`c0-r0-p127-r4-adjacent-20261106/` and
`c0-r6-commit-integration-20261106/`. Initial transport failures remain preserved;
additive source-qualified evidence does not rewrite them.

French§13.2.6 p110 also has an internal wording conflict: its bullet says C4
increments irrespective of R4, while its non-last summary literally gives
C4=R4+1. The existing frozen counter and new consumer tests use C4+1;
this candidate does not introduce that counter rule. Record both readings and
seek author clarification rather than treating the summary as silently fixed.
Before master, add both rival readings beside t35e/f/h when next editing them.
See the [unsent questions](../classic/accc-author-questions-2026-11-06-fr.md).

Peer-source check: CPCSyntaxErrorv0.12.0 (`22de765`) implements frozenC4+1
and has no R6<=R4 gate on ordinary ParityR6 capture (`src/core/crtc.cpp:1147,1443`).
Its midline widening closes display but does not capture parity at that edge
(`src/core/crtc_type_0.cpp:216`). It supports the ordinary event interpretation
without settling the new midline parity question. Original hardware remains
necessary where the two French readings disagree.

## Focused B6 R7 judgment

A prepared Astra review of the frozen493 source and original C0 B6 caller
finds the late R7 ordering underdetermined. Both photo-required low PPI samples
are high in the qualified model. IRQ decoding and the result encoder are
excluded; the fourth-microsecond trial (ordinal6) has preceding misses that
anchor its phase, while ordinal5 succeeds on its first drifting IN and has
weaker phase qualification. Neither
bus-write onset/end nor the next CLKEN alone identifies physical R7 acceptance.
Removing row-change rearm would conflict with French§16.3 p169's continuous-pulse
example. No timing exception or RTL candidate is selected.

The smallest proposed physical discriminator keeps ordinal6, its fourth-microsecond access and its
prehistory, comparing departing-rowR7=32 with entering-rowR7=33. A seam VS rise
would rule out late acceptance as the sole explanation; a negative remains
ambiguous. A C9-nonterminal companion can then separate row-carry dependence
from line-wide terminal inhibition. The unsent French question records this
boundary. Private report: `c0-b6-r7-strong-judgment-20261106/REPORT.md`.

A one-shot offline photo oracle over the already-qualified complete B6 RAM
confirms both original stores8D4A/8D4B fail1E expectations with actual1F/1F
(exit1). The RAM digest is1751f52d9d404d3b09c4c29b08df10e0f1a9ceb06ca519bdfe93e1bf1da3d3ab.
No new DUT run, phase assumption or production test was added. Receipt:
`c0-b6-r7-photo-failure-20261106/`.

## Plus STATUS2 bit0 adjustment-domain qualification

The scoped candidate qualifies the strictC4/R4,C9/R9,C0/R0 status0comparison
with `!in_adj`; no counter or other status bit changes. Canonical physical
CRTC3DU03C0showsbit0OK. Available26/27two-stage predicates and collector bytes
are identical; exact photographed binary identity remains unknown. The
original production-CPU baseline qualifies normalPASS/adjusted4DFFRED; the
candidate qualifies both stagesPASS with the same observer/input/oracle.
FocusedRED/GREEN and46selected benches pass. Fresh Opus source review gives
conditional experimentalGO, with parent verification of the out-of-sandbox
proof packet. Exact-source full fit and targeted native acceptance pass on
4e45417; bit4 remains a separate defect.

The source comment carries MODEL LIMIT(B26). A body-terminal event and a
last-adjustment-line suppression rival fit the same R9=7/R5=8pair; a physical
R5=16pair can distinguish them. Interlaced adjustment is also affected, while
the separate extra line is unchanged. Live writes, sibling status bits and
comparator topology remain unproved. Do not describe this as a decap-derived
shared comparator or silently generalize it to bit4. Evidence:
[status proof](hardware-runs/shaker-crtc3-status-overflow-2026-11-06.md).

## Plus STATUS2 bit4: adjustment terminal candidate

The separate candidate uses `R5==0 || (in_adj && adj_end_n && hcc_equal)`.
It reads existing state and changes only bit4 readback. Canonical physical
CRTC3 DU03C0 reports OK for the unchanged normal38/0 and adjusted37/8 tests;
the exact photographed binary is unknown, but available26/27 predicates and
collectors are identical. The original CPU baseline qualifies normal PASS and
adjusted0000 RED. AmSpirit executes the same resident unit to PASS. These
observations override the French §21.3.4.2 p249 Always1 row for that sampled
case; they do not establish a general hardware decoder.

The widest extrapolation is every stable nonzero R5: the candidate changes
the former constant to one terminal-character pulse throughout the frame.
Physical support covers R5=8, R9=7, R0=63 and R8=0. Diagnostic peer controls
add R5=1/16 at the same other settings. Other R5 values, live register writes,
the reuse of the sequencer's greater-than-or-equal termination, pulse width
and interlace remain inferred. No extra state or numeric timing aperture is
introduced. The RTL carries `MODEL LIMIT (B26)`.

End of adjustment and true frame restart coincide in the accepted progressive
case. A physical R8=1 run can separate them: in the field with an extra line,
sample the final character of the R5 block and the final character of the extra
line; the other field supplies a coincidence control. A stored flag could also
match the current observations. This is a proposed discriminator, not a new
hardware expectation. Photo identity comes from the canonical CRTC3 index and
path; its generic CRTC3/4 heading is not itself the type identifier.

Focused RED/GREEN and46 selected benches pass. The unchanged original CPU
replay now qualifies both stages PASS and natural RET9761 toC012; only adjusted
prefix bit4 bytes change. Fresh Opus review finds no RTL or test bug and permits
a private full fit. Exact-SHA full fit and native whole03C0 acceptance pass on
cb3a1a6, with E/R5 and scoped DSC4/Amazing preservation. This accepts the
sampled readback behaviour; the broader model limits above remain open.
Evidence and source-table conflict:
[status proof](hardware-runs/shaker-crtc3-status-overflow-2026-11-06.md).

## Focused CRTC1 terminal-write audit

A prepared Astra-high audit of the current eebf98d source identifies two local
algebraic simplifications: the current qualified write already implies the
ordinary terminal match, and duplicated adjustment admission can become one
Type1-line/Type0-row predicate. Both retained facts and the independent
same-edge positive-R4 VMA veto remain. Three converging register histories
require three distinguishable continuations at the unrestricted RTL pins;
this is a local-state argument, not a claim about physical latch count or stock
Z80 ability to issue two writes in one character.

The French §11.2.4 p85 was rendered and checked directly. Its single-write
counting/VMA rules do not establish system-clock aperture, simultaneous-write
arbitration or the current C5 restart policy. The prep packet mislabeled an
English page as French, overstated Module E physical-photo coverage and carried
a historical soak value; the audit records those corrections. Native MiSTer
and AmSpirit results remain distinct from original-CPC evidence.

Private report `b26-c1-terminal-astra-20261106/AUDIT.md` under the session root
contains algebra, counterexample histories and a single-write RA discriminator
for three versus four adjustment lines. A private full-wrapper equivalence
prototype is in preparation; no production edit, machine-equivalence result,
Opus acceptance or new hardware rule is claimed yet.
