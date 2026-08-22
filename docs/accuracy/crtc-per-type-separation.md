# Per-CRTC model separation — status and recommendation

Context: Longshot's advice to Renaud (relayed 2026-08-22): "each CRTC must have its own
emulation code. If you try anything else, it will be either failure, or an unmaintainable
headache." This file records where our core stood against that advice and what changing it
would mean.

**OUTCOME (2026-08-22): option S2 executed.** The split landed behaviour-preserving on
`accuracy/crtc-type-split` (27efc2d): wrapper `rtl/CRTC.v` (renamed from `rtl/UM6845R.v` —
the old filename was the type-1 part number and misdescribed a two-variant component) keeps
ports, register file, shared counters and sequencing; `rtl/crtc_type0_engine.v` /
`rtl/crtc_type1_engine.v` hold every type-specific rule plus provably-private flops.
Bit-identity proven by the soak golden hash (`0x5b5004ff70148443`, minted pre-split; later
re-minted to `0x326ea81358e7d88f` by the intended F6 Stage 1 behaviour change — this
historical statement refers to the split proof only), all 87
vectors, lint, and a lockstep differential run against the pre-split core (~45.5M CLKEN
samples, no divergence). See `docs/accuracy/type-split-review-guide.md`. The sections below
are kept as the pre-split record.

## Status before the split: one shared model for types 0 and 1

`rtl/UM6845R.v` implements **both** classic types in a single state machine selected by the
`CRTC_TYPE` port (`0` → HD6845S/UM6845, `1` → UM6845R). The filename is the type-1 part
number — historical, not scope. There is no separate CRTC0 file; type 0 is everything in the
module gated on `~CRTC_TYPE`, e.g.:

- `r0_frozen = !CRTC_TYPE && !R0_h_total && !hcc` (`UM6845R.v:196`);
- `line_max = CRTC_TYPE ? crtc1_line_max : crtc0_line_max` (`:209`),
  `row_frame_last`/`row_new` muxes (`:256`, `:260`);
- `row_addr_save = ... (CRTC_TYPE ? line_last : type0_live_line_last)` (`:434`);
- the whole `type0_*` arbitration latch cluster inside the shared sequential blocks.

The Plus stream already follows Longshot's principle by decision: roadmap integration rule 2
keeps ASIC CRTC3/4 out of `UM6845R.v` entirely (`plus/architecture.md` behavioural path).
So the open question is only the type-0/type-1 split.

## Why it matters (evidence it is already costing us)

- Review action item **A1** (`docs/review-debt.md`): a type-1 adjustment behaviour was
  patched into the shared VSYNC comparator via a `row+1` substitution term, creating a
  corner that fires only on the adjustment-ending line. That is Longshot's predicted
  failure mode at branch-point scale.
- **F8** had to carve out per-type `line_max`/`row_frame_last`/`row_new` wires; the seams
  widen with every fidelity increase.
- The remaining classic findings concentrate exactly where the divergence is largest:
  F7 RFD is type-1-only, and F10's interlace parity machines are structurally different
  per type (digest-03 §19.5/§19.8).
- AGENTS.md already warns: "`rtl/UM6845R.v` is one shared state machine — findings
  interact; the suite is what catches collateral damage."
- Upcoming findings concentrate exactly where divergence is largest: F7 RFD is type-1-only;
  F10 interlace parity machines differ structurally per type (digest-03 §19.5-§19.8).

## Options

| Option | When | Cost / risk |
|---|---|---|
| **S1. Keep shared**, internal discipline only | now | Zero upfront; every future finding pays again in gating discipline and review surface |
| **S2. Split before F7/F10 RTL** | next classic refactor | One behaviour-preserving refactor commit (register file + bus interface shared or duplicated; two counter engines). Big diff over delicate logic; must be bit-identical under all 87 vectors; files.qip change → own CI synthesis run |
| **S3. Split now** | immediately | Same as S2 but collides with the just-closed F9 state and delays both queues |

## Recommendation

**S2**: schedule the split as its own refactor milestone before F7 RFD and F10 RTL work.
Those two findings add the most type-specific machinery — landing them into their natural
homes beats deepening the tangle and re-paying the cost later. Constraints if adopted:
bit-identical behaviour proven by the existing suite plus lint; no assertion changes; new
modules carry the ACCC attribution header; `files.qip` in the same commit; CI synthesis run
required (file-list change); fresh review pass afterwards since it touches the most
sensitive logic in the core.

If S1 is chosen instead, record it here as a deliberate exception to Longshot's advice so
the reasoning is preserved either way.

Related: [f6-decision-gate.md](f6-decision-gate.md) — option C's Stage 1 touches only the
type-0 DE path and is unaffected by the split choice, but doing S2 first would give F6's
stage work a cleaner home.
