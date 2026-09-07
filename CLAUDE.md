# Working rules for this repository

This is a fork of the MiSTer Amstrad CPC core. Two work streams run in parallel and must not
be merged into one commit or one PR: classic CRTC accuracy for types 0 (HD6845S) and 1
(UM6845R), and Amstrad Plus/GX4000 ASIC support.

Start from `docs/backlog.md` for the cross-cutting architecture and methodology items
(observability, harnesses, structural debt) that sit underneath feature work,
`docs/implementation-roadmap.md` for dependency order and acceptance gates,
`docs/current-status.md` for the handoff state, and `docs/accuracy/audit-findings.md` for the
numbered findings F1-F12.

## Proportionate engineering

This is a solo hobby project. Prefer the simplest solution that handles the normal workflow.
Use existing Git, shell and host features before adding custom infrastructure. Routine local
setup does not need integrity manifests, exhaustive failure recovery, portability hardening,
or a dedicated test suite without a concrete recurring problem. Treat occasional manual
recovery as acceptable when the cost and risk are small. Do not turn a minor edge case into
a review/fix cycle; explain material tradeoffs briefly and keep scope proportionate.

Retain rigorous source-derived tests and review for RTL, timing/state behavior, and changes
that could lose valuable work. Scale verification to consequences; these hardware correctness
requirements do not justify production-grade machinery around copying local reference files.

## Worktree layout and task lifecycle

Tasks use ad-hoc, environment-owned worktrees; accuracy and Plus remain separate behavior
streams. General work covers shared infrastructure, peripherals, docs and tooling. Existing
fixed worktrees may hold unfinished work: preserve them, but do not require or recreate them.
Integration normally targets `accc-review-and-fixes` from a checkout the integration task can
access. Discover actual paths and branch ownership with Git and host task metadata.

- `$stream-start [accuracy|plus|general|auto] [brief]` is the manual shortcut for selection,
  overlap assessment, a named branch, and reference provisioning. Omitted scope means auto.
- `$stream-orchestrate` creates compatible steerable tasks, or adopts existing tasks and adds
  more without restarting them. The agent assesses shared-interface risks; the user need not
  know which files collide. Ordinary textual overlap can wait for merge reconciliation.
- `$stream-finish [source] [--no-push]` integrates and validates one branch, reconciles shared
  docs, and pushes by default. Explicit invocation supplies that authorization. Coordinated
  tasks stop at READY unless finish was requested; the integrator finishes them sequentially.

Keep one writer per checkout and one integrator at a time. No permanent directory mapping,
sibling-write probe or custom lease protocol is required. `stream-start` prepares the ignored
ACCC PDFs in the assigned checkout; bridge workers use that same checkout without another
clone/worktree. Never commit the PDFs or reset unfinished branches for a fresh start.

See [docs/task-workflow.md](docs/task-workflow.md) for host routing, task briefs, provisioning,
artifact delivery and cleanup. Hooks may prepare the environment but never select roadmap
work just because a conversation opened.

## Where authority lies

Sources rank in this order, and a lower rank never overturns a higher one:

1. Real hardware, and the Logon System reference photographs on `shaker.logonsystem.eu` that
   record it. SHAKER results are judged by visual comparison against those photographs. The
   stock upstream core is a regression baseline only: shared inaccuracy is invisible against it.
2. The latest French edition of the Amstrad CPC CRTC Compendium (ACCC), currently v1.11 at
   `docs/references/ACCC1.11-FR.pdf`. The matching English edition is a working translation
   and accessibility aid, not the tie-breaker when the editions differ.
3. The checked-in digests under `docs/accuracy/`.

The ACCC is our working oracle but it is not the final authority. A vector that encodes a
misreading of the Compendium passes cheerfully and hides the very bug it was meant to catch.
When simulation and hardware disagree, hardware wins and the vector is wrong.

`docs/references/ACCC1.11-FR.pdf` and `docs/references/ACCC1.11-EN.pdf` are user-owned and
deliberately untracked. Never commit them. **Untracked does not mean absent**: both files are
in the working tree, gitignored, and readable. Read the French source directly whenever a
rule matters; use the English edition to aid navigation and review. If they differ, record
both readings and use the French reading unless hardware or an author clarification
supersedes it.

Use ACCC section numbers as the durable citation key. New or materially revised rule claims
cite the French page and may add the English page for readers. Do not mechanically rewrite
historical reports, quotations, or unaffected code comments: migrate an English-only anchor
when a bilingual finding affects it or when the surrounding claim is substantively revised.

Read the PDFs through the `pdf-inspector` skill, and follow the verification protocol already
written down in `docs/accuracy/extract/README.md`: pdf-inspector's position-aware Markdown is
the primary text layer (2026-08-24 decision; pdftotext is a weaker extractor kept only as an
optional second opinion), and table or chronogram rules are judged from rendered pages, never
from a text layer alone. Reaching for raw text where a figure is involved is how figure
content silently flattens — which is what most of the digests' ⚠ VERIFY flags record.

## Writing test vectors

Write vectors where reading the RTL against the documented rule predicts a mismatch. Do not
write blanket coverage for behaviour nobody suspects.

The cheap step comes first: read the ACCC rule, read the corresponding RTL, and decide whether
they actually disagree. That costs a fraction of a vector and it is what turns testing into
progress. A batch of vectors that all pass on first run bought regression armour, not a
finding; that is occasionally worth doing on purpose, but it should be a deliberate choice
rather than the default motion.

**A test earns its place only if it could fail for a reason you did not already know.** A
vector derived from an ACCC rule you have just implemented, asserting that same rule, is
documentation with a `make` target: it will never fail until someone edits the line it mirrors.
Prefer vectors that pin a *cross-module* interaction, a degenerate case, or a rule you have not
yet implemented. Suites grow without limit otherwise, and a large green suite that cannot
surprise you is what lets a wrong core look verified. This applies to review passes too: do not
run independent review on documentation-only changes.

This does not license skipping tests for behaviour changes. The classic CRTC core keeps
singular shared state across three files (wrapper `rtl/CRTC.v` plus the two per-type rule
engines), findings routinely touch each other's state, and the Verilator suite is the only
thing that catches collateral damage. Every behaviour change still lands with a focused
deterministic vector, and a timing-sensitive finding does not start until its failing vector
exists.

Derive every expected value from the documented rule on paper and cite the ACCC section and
page beside it in the test. Never read an expectation back out of the simulator: that produces
a suite which agrees with a wrong core.

When a finding is implemented, its named expected-failure cases become required passes in the
same commit. Never weaken an assertion to make the suite green. An unrelated test that starts
failing is a finding, not something to edit.

## Gates

`make -C sim` must pass before every code/RTL/simulation commit (pure documentation or markdown changes do not require running simulation). GitHub Actions runs that fast gate on every
non-documentation push. Pinned Quartus 17.0.2 synthesis is automatic wherever work integrates:
every push to an integration branch (the default branch, `accc-review-and-fixes`) that touches
anything Quartus compiles, plus pull requests, tags, and manual dispatches. All integration builds
compile at full effort by default to produce hardware-testable RBFs. Stream branches
stay on simulation until integration. The integration push workflow classifies synthesis and
selects the local or hosted runner; do not duplicate that run with a manual dispatch merely
because the VM is online. For an explicitly requested pre-merge/milestone answer, prefer the
online local VM and otherwise use hosted CI. Hardware results outrank simulation but never
replace it. See `docs/ci-testing-policy.md` for routing and artifact requirements.

CI runs supersede each other instead of queueing: a newer run with the same ref and event
type cancels the older run outright, and among expensive Quartus builds the newest cancels
the oldest across branches, events, and hosted/local routes via `build-core-synthesis`. A run
that ends `cancelled` therefore means *superseded*, never failed — check `gh run list --branch <ref> --limit 5`
for its successor and judge that one instead of diagnosing from the corpse or blindly re-running. Full semantics:
`docs/ci-testing-policy.md`, "Run supersession".

## Independent review

The project requires a fresh cross-provider review of every non-trivial diff, so no model is
the sole reviewer of its own work. That capacity is currently unavailable. Work merged without
it gets a row in `docs/review-debt.md` in the same commit that introduces it, naming what a
reviewer should look at hardest. Delegated implementation stays provisional until the parent
has read the diff; if the delegated agent already executed and confirmed `make -C sim` green,
the parent does not need to re-run the gate redundantly.

## ACCC attribution

The Compendium is licensed CC BY-NC-ND 4.0 and its section 2.2 carries an explicit attribution
directive: the credit line belongs in the source headers of CRTC emulation modules and in the
credits of any distributed product built from them. `rtl/CRTC.v` and `sim/sim_main.cpp`
carry it. Any new module implementing CRTC behaviour from the Compendium must carry it too,
and individual rules cite their ACCC section at the point of implementation.
