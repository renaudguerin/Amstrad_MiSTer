# AGENTS.md

Fork of the MiSTer Amstrad CPC core (Verilog/SV, Quartus 17.0.2 target, DE10-Nano).
`CLAUDE.md` carries the full working rules and is authoritative for workflow; this file
summarizes what an agent must not get wrong.

## Two streams, never mixed

Classic CRTC accuracy (types 0/1) and Amstrad Plus/GX4000 ASIC support are separate work
streams. Do not merge them into one commit or one PR.

Start from:
- `docs/implementation-roadmap.md` — dependency order and acceptance gates
- `docs/current-status.md` — handoff state, hardware-test milestones
- `docs/accuracy/audit-findings.md` — numbered findings F1–F12

## Authority ranking

1. Real hardware / Logon System reference photos (`shaker.logonsystem.eu`). SHAKER results
   are judged against these photographs; the stock upstream core is only a regression baseline.
2. The CRTC Compendium (ACCC) v1.10 — working oracle, not final authority.
3. Digests under `docs/accuracy/`.

When simulation and hardware disagree, hardware wins and the test vector is wrong.

`docs/ACCC1.10-EN.pdf` is user-owned and untracked. **Never commit it.** It is present in
the working tree, though — untracked, not missing. Prefer it over the digests whenever a rule
claim matters, and read it through the `pdf-inspector` skill under the verification protocol
in `docs/accuracy/extract/README.md` (its position-aware Markdown is the primary text layer,
with pdftotext only an optional second opinion; figures are judged from rendered pages,
never from a text layer).

## Commands

```sh
make -C sim          # Verilator suite: CRTC pin tests + sim/plus ASIC tests (gate for every commit)
make -C sim lint     # verilator --lint-only, both suites
make -C sim soak     # randomized equivalence soak vs the golden hash (see sim/README.md)
make -C sim clean
```

- Requires Verilator 5+, GNU Make, C++17 compiler (`brew install verilator` on macOS).
- Failures exit nonzero; failing CRTC tests leave a VCD at `sim/obj_dir/<test>.vcd`.
- Known divergences are marked `XFAIL` and don't fail the suite; an `XPASS` does fail it.
  After fixing RTL, remove that test's XFAIL flag in the same change so the fix becomes a
  regression test.
- The soak prints a hash that must equal the recorded golden value
  (`0xa9e5026de83d287c`, re-minted 2026-08-25 for the F10 review
  remediation: the type-1 leaving stage A now writes C9.0 (B-1), the
  §19.8.2 match-branch ParityC9 toggle also fires at frame boundaries
  (B-2), the leaving stage-B ParityC9 write is gated to the entering
  case (N-1), and IVM now engages from a reset/snapshot-loaded R8=3;
  tog_enter/tog_enter_line joined the sampled projection; previously
  `0x83e80134f7705b46` from the F10 type-0 IVM
  behavior: the §19.8.1 split C9/C9.VMA with the asymmetric entry/exit
  tests is now live on type 0, and the seam-latched IVM mode plus the
  line-scoped toggle status joined the sampled projection; previously
  `0x7d0e5c8bd984e899` from the F10 type-1 IVM
  behavior: the documented R8-toggle parity stages and §19.8.2 counting
  are now live on type 1, and the IVM/stage state joined the sampled
  projection; previously `0x1ac680cd2f12559a` from the F10 fixture-stage
  sampled-field expansion: the shared interlace parity flops joined the
  sampled projection while still holding reset values, no RTL behaviour
  change; previously `0x512eaae74a628dca` from the A2 exact-edge R4
  adjustment-reload suppression; previously `0x6439f9805b20acaa` from A1,
  `0xae27f2c3c758ed87` from F7 RFD,
  `0xf5f8ae01ffdf928d` from the sampled-field expansion,
  `0x326ea81358e7d88f` from F6 Stage 1, and
  `0x5b5004ff70148443` from the unsplit core)
  for behaviour-preserving changes: run `make -C sim soak SOAK_EXPECT=<hash>`.
  A hash change on a refactor commit means behaviour moved — stop and document
  why before proceeding.
- There is no native local Quartus path on Apple Silicon. GitHub Actions runs Verilator on
  every non-documentation push, then runs the pinned Quartus 17.0.2 container only for known
  integration paths, the default branch, pull requests to it, tags, or a manual milestone dispatch.
  Semantic clock/memory/RGB risks still require an exact manually dispatched build. The tiered
  rules are in `docs/ci-testing-policy.md`; local UTM/Docker options are in `docs/building.md`.
- Use `Amstrad.qpf` as the project file; `Amstrad_Q13.*` is a legacy alternate, ignore it.

## Core layout (since the 2026-08-22 per-type split)

The classic CRTC core is three files: wrapper `rtl/CRTC.v` (ports, register file/bus decode,
shared counters + sequencing) and two rule engines `rtl/crtc_type0_engine.v` /
`rtl/crtc_type1_engine.v`. Type-specific rules land in their type's engine; shared-counter
sequencing stays in the wrapper because `CRTC_TYPE` is a live input whose round-trip
behaviour is pinned by required vectors. The wrapper was `rtl/UM6845R.v` before the split —
the old name survives only in history and historical documents.

## Test vector discipline

- Write a vector only where reading the RTL against the documented ACCC rule predicts a
  mismatch. Read the rule and the RTL first; blanket coverage is not progress by default.
- Derive every expected value from the documented rule on paper and cite the ACCC section
  and page next to the assertion. Never read expectations back out of the simulator.
- Every behaviour change lands with a focused deterministic vector. A timing-sensitive
  finding does not start until its failing vector exists.
- Never weaken an assertion to go green. An unrelated test that starts failing is a finding.
- The classic CRTC core keeps singular shared state (see "Core layout" above) — findings
  still interact through the wrapper's counters; the suite is what catches collateral damage.

## Verification ownership (lesson from the F6 premise miss, 2026-08-22)

Documents make two kinds of claims and they need different owners:

1. **Rule claims** ("hardware does X") — verified against the ACCC PDF by the faithfulness
   review; within `audit-findings.md`, Rule sections are trusted.
2. **Integration assumptions** ("our DE is consumed at 1 µs", "co-simulation is infeasible",
   line references, "the current model does Y") — prose about *how code and system
   boundaries interact*. These are **unverified by default** no matter how authoritative the
   surrounding document sounds, until someone confirms them against the actual sources
   (`rtl/GA40010/`, motherboard wiring, current tests).

General lesson for every future phase: whenever a document asserts something about a system
boundary rather than a documented rule or a specific commit, assign an owner to check it
against the code before acting on it — phase gates defined by artifact type will otherwise
let boundary claims survive every gate, as the F6 one did. When you find such a claim:
correct it in the same pass if cheap, else record it in the session plan's addendum with a
named remediation milestone (never leave it silently stale).

## Other hard constraints

- Synthesis is automatic on every integration-branch push that touches a source Quartus
  compiles, so merging a stream branch synthesizes it. Dispatch by hand only for a pre-merge
  answer or a semantic risk no path reveals (top-level wiring, clocks, memory arbitration,
  RGB width). See `docs/ci-testing-policy.md`.
- New modules implementing CRTC behaviour from the Compendium must carry the CC BY-NC-ND
  attribution line (see `rtl/CRTC.v` header) and cite ACCC sections at the point of
  implementation.
- Non-trivial diffs require fresh cross-provider review. Work merged without it gets a row
  in `docs/review-debt.md` in the same commit, naming what a reviewer should check hardest.
