# RTL model confidence and upstream review

This is a source-checked starting inventory, not an exhaustive count of
exceptions or an architecture approval. It addresses the risk that successive
local timing repairs preserve an accidental model rather than expose a simpler
hardware rule. The initial inventory binds experimental8ba384e and its documentation
checkpoint. The later reviewed Plus R4 branch candidate is identified below.

## Cases needing explicit judgment

| Area | Current model / restriction | Evidence and remaining risk |
|---|---|---|
| Classic type0 R0 deadline | `type0_r0_eq_kept` samples true equality at nCLKEN, ORs live equality, explicitly excludes sampledR0=1. | French§13.6.1 p124 plus qualified original caller, four instruction bins and native A4/B6 support the narrow fix. Exact sample edge is inferred. The R0=1 exclusion is a coverage guard; native A4 retains seven low-bit residuals there. It is not a proved silicon exception. Source: `rtl/crtc_type0_engine.v`, `type0_r0_at_sample`. |
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


## First bounded strong-model audit

The first frozen source audit is complete in the private session packet
`b26-audit-results-20261106/REPORT.md`. It expands the classification and
finds a concrete independent test from French§13.7.2.1 p127: the illustrated
R0=1 widening increments C4 when C9=R9 even when C4 differs from R4, while
continued adjustment depends on frame-terminal qualification. The current
explicit widening path couples those decisions. A phase-qualified literal
OUT/NOP/OUT diagnostic is being prepared before any fix; another existing
path may still satisfy the rule, so the code reading is a prediction.

Two late oldR0=1 horizontal rules still fit available evidence. Removing the
numeric guard alone saves no proven state bits and is not endorsed. The
original lower A4 caller remains a separate diagnostic. An additive reference
correction after the frozen audit resolves row3 as5E5F; seven lower native VS
bits disagree, with only row4 a confirmed matching negative control.

## First bounded source audit

The frozen classic8ba384e audit classifies nine policy groups, separating sourced
quirks, inferred outcome models, coverage guards and initialization policy.
French §13.7.2.1 p127 supplies a missing constraint: after the specified oldR0=1
widening, C4 advances when C9=R9 even when C4 differs from R4. The qualified
production-T80 cohort has126/126 transport guards. Its true-last and R9-negative
controls match, while the non-last case reaches2/6/7 instead of2/7/7. The
initial39/40 transport stop and its analyzer remain immutable; the additive
qualifier follows the source's five-character OUT spacing without imposing an
unrelated NOP/HALT M1 phase convention.

Astra's proposed private candidate reuses the precommit write ticket for that
row advance and leaves entry into adjustment dependent on the previously armed
true-frame condition. It adds no state or ports and preserves the existing
direct-write horizontal hold. Original C0 B6 before/after is mandatory because
its four R0 results already match hardware. No production RTL change is accepted
yet. Two late oldR0=1 horizontal models remain admissible; removing the numeric
sampling guard alone saves no established state. The seven lower A4 mismatches
remain separate.

Private source/proof/design packets are `b26-audit-frozen-20261106/`,
`b26-audit-results-20261106/` and `c0-r0-p127-readiness-20261106/` under the active
session evidence root. Root verifies all174 final proof artifacts against the
frozen manifest. This is a behavioral counterexample and a narrower candidate,
not a recovered silicon schematic or a completed upstream audit.
