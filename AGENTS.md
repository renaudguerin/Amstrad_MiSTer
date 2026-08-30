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

## Worktree layout and stream lifecycle

Three worktrees partition the repository:
- `/Users/renaudg/code/Amstrad_MiSTer` — Main integration worktree (`accc-review-and-fixes` branch), generic work, docs, upstream sync (`master`).
- `/Users/renaudg/code/Amstrad_MiSTer-accuracy` — CRTC accuracy stream (`accuracy/*` branches).
- `/Users/renaudg/code/Amstrad_MiSTer-plus` — Amstrad Plus/GX4000 ASIC stream (`plus/*` branches).

Stream workflow is formalized in project skills (`.agents/skills/`):
- **Start a task**: Invoke `$stream-start <accuracy|plus> [topic]`. Re-anchors the stream worktree on `accc-review-and-fixes` and cuts `<stream>/<topic>` (autofills next todo if omitted; no baseline simulation).
- **Finish a task**: Invoke `$stream-finish <accuracy|plus>`. Performs non-fast-forward merge into `accc-review-and-fixes`, resolves conflicts, semantically reconciles shared docs (`current-status.md`, `review-debt.md`, golden hashes), runs `make -C sim` once (skipped for doc-only changes), and pushes to origin to trigger CI synthesis.
- **Coordinate parallel tasks**: Invoke `$stream-orchestrate` when the user explicitly asks
  one Codex Desktop task to create and supervise separate accuracy/Plus conversations. It
  opens tasks against the saved project, directs them to the fixed stream worktrees,
  exchanges peer IDs and discoveries, and serializes `$stream-finish` integration.

### Parallel stream coordination

Parallel tasks still have one writer per worktree and one integration writer at a time.
Stream-local files belong to that stream. Top-level/common peripheral RTL, root build/CI
files, and shared status/review documents require an assigned owner before overlapping
edits. A task that discovers cross-stream evidence messages its peer promptly with the
finding, impact, proposed owner, and any commit the peer must consume.

Each coordinated stream proves sibling-worktree writability, then reports WRITABLE, STARTED,
DISCOVERY, READY, and INTEGRATED milestones. The coordinator relays messages by default and
assigns shared owners in the initial prompts, avoiding a peer-ID startup race. A stream stops
at READY until the coordinator grants an integration lease containing the current exact
integration SHA. Unless dependencies require another order, accuracy integrates first; the
Plus task then rebases with merge topology preserved onto the verified integration SHA.
Never run two `$stream-finish` operations concurrently, and never force-push after a stale
lease or moved integration tip. Automatic finish/push requires explicit user authorization.
When only one stream task exists, ordinary `$stream-start`/`$stream-finish` behavior is
unchanged.

## Authority ranking

1. Real hardware / Logon System reference photos (`shaker.logonsystem.eu`). SHAKER results
   are judged against these photographs; the stock upstream core is only a regression baseline.
2. The CRTC Compendium (ACCC) v1.11 — working oracle, not final authority.
3. Digests under `docs/accuracy/`.

When simulation and hardware disagree, hardware wins and the test vector is wrong.

`docs/references/ACCC1.11-EN.pdf` is user-owned and untracked. **Never commit it.** It is present in
the working tree, though — untracked, not missing. Prefer it over the digests whenever a rule
claim matters, and read it through the `pdf-inspector` skill under the verification protocol
in `docs/accuracy/extract/README.md` (its position-aware Markdown is the primary text layer,
with pdftotext only an optional second opinion; figures are judged from rendered pages,
never from a text layer).

## Commands

```sh
make -C sim          # Verilator suite: CRTC pin tests + sim/plus ASIC tests (gate for code/RTL/sim commits)
make -C sim lint     # verilator --lint-only, both suites
make -C sim soak     # randomized equivalence soak vs the golden hash (see sim/README.md)
make -C sim clean
```

- `make -C sim` is required for any change touching RTL, testbenches, simulation models, or build manifests. Pure documentation, markdown, and reference changes do not require running simulation.

- Requires Verilator 5+, GNU Make, C++17 compiler (`brew install verilator` on macOS).
- Failures exit nonzero; failing CRTC tests leave a VCD at `sim/obj_dir/<test>.vcd`.
- Known divergences are marked `XFAIL` and don't fail the suite; an `XPASS` does fail it.
  After fixing RTL, remove that test's XFAIL flag in the same change so the fix becomes a
  regression test.
- The soak prints a hash that must equal the recorded golden value
  (`0x32d468e81eac63c9`, re-minted 2026-08-30 for the reviewed F20 R2.JIT
  correction: type-0/type-1 starts move by 4/3 mode-2 pixels while their
  ordinary trailing-edge positions remain fixed, shortening the raw pulse by
  4/3 pixels; same-value writes remain normal; previously `0x005deed28be80fa1`
  from the pre-review, incorrectly width-preserving F20 model and its
  sampled-state expansion; previously `0xc769ea4605afbe04`, re-minted 2026-08-30 for the F13
  half-character type-0
  R1>R0 border pulse: the no-skew C0=R0 event is now low only from nCLKEN to
  the following CLKEN, while SKEW-DISPTMG 1/2 retains the documented rounded
  full-character displacement; previously `0x48146d2b681268ab`, re-minted
  2026-08-26 for the F16, F17, and F18 closures:
  F16: type-0 post-IVM exit frozen C9.VMA line-end comparison retains the last
  computed IVM C9.VMA in exit_frozen_vma after leaving IVM (R8->0), comparing
  it against plain R9 until match or IVM re-entry (ACCC section 19.8.1 p.220;
  vectors t22l-t22s extended and t30a/t30b recovery recipe added); F17: type-1
  RFD triggered on C9=R9 disables the VMA-source state while arming the parity
  flag (ACCC section 11.6.1 p.88 Case 2; t13d updated and t13n added); F18: type-1
  readable register matrix validated and pinned per ACCC section 21.2.2 (t01);
  previously `0x85b3f8e847430495`, re-minted 2026-08-26 for the F15 closure: type-0
  odd-R9 IVM counting is implemented — the limit target becomes
  R9+(ParityC9 xor R9.0) so rows end at the first C9.VMA at or past R9
  (the p.206 5/4 alternation), the p.219 row-end ParityC9 update
  ParityC9=C4.0(new) xor ParityFrame is live for odd R9 with the origin
  re-anchoring it to the frame parity, the switch-line target is the
  p.219 addition form R9+ParityFrame, and the section 19.5.2 VSYNC
  delay-by-1-line correction fires on ParityFrame-odd frames when R7 is
  odd; even-R9 behavior is bit-identical to the previous mint;
  previously `0x627bdc9923a60677`, re-minted 2026-08-26 for the F14 closure: the
  additional interlace line is implemented on both types — type 0 appends
  one line after the R5 adjustment lines when R8∈{1,3} and ParityR6 is odd
  (the line holds C4=R4+1/C9=R5, the frame origin moves to its end, and the
  R6>R4 freeze persists the gate), and type 1 defers the adjustment end by
  one line when R8∈{1,3}, ParityFrame is even and R9+1 is a multiple of R5
  (the extra line holds C9=0 at C4 one past the last adjustment row);
  previously `0x63d9de100ac9f6f2`, re-minted 2026-08-25 for the B-1 remediation:
  during type-1 IVM the VSYNC now starts at the half-line tick on the
  ParityFrame-even frame (the p.208 MID-VSYNC prose) via a seam-latched
  fire decision, while the odd-parity frame keeps the seam start;
  previously `0xd620fce8b1c05b25`, re-minted 2026-08-25 for the t24
  closure: during
  type-1 IVM the VSYNC now fires from the IVM-aware row-structure test on
  both frame parities and the legacy field=1 MID-VSYNC arm no longer
  hijacks it (ACCC p.208 table; the engine arm also switched from plain
  C9==R9 to the IVM-aware row-end test); previously
  `0x801a59096c192d26`, re-minted 2026-08-25 for the F11h closure: the
  type-1 §20.3.2 row-0 VMA reload now samples the post-edge register file,
  so an R12/R13 write landing exactly on a row-0 line-boundary edge is
  caught by that reload (ACCC p.242 chronogram 2); previously
  `0xa9e5026de83d287c`, re-minted 2026-08-25 for the F10 review
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
  every non-documentation push, then runs the pinned Quartus 17.0.2 container for known
  integration paths (`accc-review-and-fixes`, default branch), pull requests, tags, or a manual
  dispatch. All integration builds compile at full effort by default to produce hardware-testable
  RBFs. Stream branches stay on Tier A (Verilator simulation) and never synthesize unless
  explicitly requested.
- When full synthesis is needed, always check whether the self-hosted Quartus VM is up
  (`quartus-vm` online via `gh api repos/:owner/:repo/actions/runners`). If online, prefer it:
  `gh workflow run local-build.yml --ref <branch> -f effort=full` (~2m–4m faster than hosted CI;
  fallback to hosted `build.yml` when offline). One-time registration: `ansible/local-runner.yml`.
- CI is last-write-wins: newer pushes/dispatches cancel older runs (same ref outright; among
  expensive Quartus compiles, across refs too). A `cancelled` Actions run means *superseded* —
  find its successor with `gh run list --branch <ref> --limit 5` before diagnosing anything.
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
