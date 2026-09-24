# Working rules for this repository

Fork of the MiSTer Amstrad CPC core (Verilog/SystemVerilog, Quartus 17.0.2, DE10-Nano).

Two main work streams: classic CRTC accuracy for types 0 (HD6845S) and 1 (UM6845R), and
Amstrad Plus/GX4000 ASIC support. Prefer separate commits for their behaviour changes, so a
hardware regression can be bisected to one stream.

Start from:
- `docs/backlog.md`: cross-cutting architecture and methodology items (observability,
  harnesses, structural debt) underneath feature work; some block roadmap items in ways the
  roadmap does not show.
- `docs/implementation-roadmap.md`: dependency order and acceptance gates.
- `docs/current-status.md`: handoff state per stream, open hardware residuals, latest artifact.
- `docs/classic/audit-findings.md` (symlinked from `docs/accuracy/`): numbered findings.

## Proportionate engineering

This is a solo hobby project. Prefer the simplest solution that handles the normal workflow.
Use existing Git, shell and host features before adding custom infrastructure. Routine local
setup does not need integrity manifests, exhaustive failure recovery, portability hardening,
or a dedicated test suite without a concrete recurring problem. Occasional manual recovery is
acceptable when cost and risk are small. Do not turn a minor edge case into a review/fix
cycle; explain material tradeoffs briefly and keep scope proportionate.

Keep rigorous source-derived tests and review for RTL, timing/state behaviour, and changes
that could lose valuable work. Scale verification to consequences.

## Worktrees and task lifecycle

Tasks use ad-hoc, environment-owned worktrees. General work covers shared infrastructure,
peripherals, docs and tooling. Existing fixed worktrees may hold unfinished work: preserve
them, but do not require or recreate them. Integration normally targets `master`. Discover
actual paths and branch ownership with Git and host task metadata.

Task skills live in `.agents/skills/`:

- `stream-start [accuracy|plus|general|auto] [brief]`: selection, overlap assessment, a named
  branch, and reference provisioning. Omitted scope means auto.
- `stream-orchestrate`: creates compatible steerable tasks, or adopts existing ones and adds
  more without restarting them. The agent assesses shared-interface risks; ordinary textual
  overlap can wait for merge reconciliation.
- `stream-finish [source] [--no-push]`: integrates and validates one branch, reconciles shared
  docs, and pushes by default; explicit invocation is the authorization. Coordinated tasks stop
  at READY unless finish was requested; the integrator finishes them sequentially.

One writer per checkout, one integrator at a time. `stream-start` prepares the ignored ACCC
PDFs in the assigned checkout; bridge workers use that checkout without another clone. Never
reset unfinished branches for a fresh start. See [docs/task-workflow.md](docs/task-workflow.md)
for host routing, task briefs, provisioning, artifact delivery and cleanup. Hooks may prepare
the environment but never select roadmap work just because a conversation opened.

## Where authority lies

Sources rank in this order; a lower rank never overturns a higher one:

1. Real hardware, and the Logon System reference photographs on `shaker.logonsystem.eu` that
   record it. SHAKER results are judged by visual comparison against those photographs. The
   stock upstream core is a regression baseline only: shared inaccuracy is invisible against it.
2. The latest French edition of the Amstrad CPC CRTC Compendium (ACCC), currently v1.11 at
   `docs/specs/ACCC1.11-FR.pdf`. The English edition is a working translation and navigation
   aid, not the tie-breaker. When they differ, record both readings and use the French one
   unless hardware or an author clarification supersedes it.
3. The checked-in digests under `docs/classic/` (symlinked from `docs/accuracy/`).

The ACCC is the working oracle, not the final authority. A vector that encodes a misreading
passes cheerfully and hides the bug it was meant to catch. When simulation and hardware
disagree, hardware wins and the vector is wrong.

`docs/specs/ACCC1.11-FR.pdf` and `docs/specs/ACCC1.11-EN.pdf` are user-owned and deliberately
untracked. **Never commit them. Untracked does not mean absent**: both are in the working
tree and readable. Read the French source directly whenever a rule matters.

- Run `scripts/accc/lookup.py "<question>"` first to locate sections; its output is navigation,
  not verification.
- Read the PDFs through the `pdf-inspector` skill under the protocol in
  `docs/classic/extract/README.md`: position-aware Markdown is the primary text layer
  (pdftotext only a second opinion), and table or chronogram rules are judged from rendered
  pages, never from a text layer. Flattened figures are what most digest ⚠ VERIFY flags record.
- Cite ACCC section numbers as the durable key. New or materially revised rule claims cite the
  French page and may add the English page. Do not bulk-rewrite historical reports, quotations
  or unaffected code comments; migrate an English-only anchor when a bilingual finding affects
  it or the surrounding claim is substantively revised.

## Verification ownership

Documents make two kinds of claims with different owners:

1. **Rule claims** ("hardware does X"): verified against the ACCC by faithfulness review.
   Rule sections in `audit-findings.md` are trusted.
2. **Integration assumptions** ("DE is consumed at 1 µs", "co-simulation is infeasible", line
   references, "the current model does Y"): unverified by default, however authoritative the
   document sounds, until checked against the sources (`rtl/GA40010/`, motherboard wiring,
   current tests).

Before acting on a boundary claim, check it against the code. Fix it in the same pass if
cheap; otherwise record it with a named remediation milestone. Never leave it silently stale.

## Core layout

The classic CRTC core is three files: wrapper `rtl/CRTC.v` (ports, register file and bus
decode, shared counters and sequencing) and rule engines `rtl/crtc_type0_engine.v` and
`rtl/crtc_type1_engine.v`. Type-specific rules go in their engine. Shared-counter sequencing
stays in the wrapper because `CRTC_TYPE` is a live input whose round-trip behaviour is pinned
by required vectors. The wrapper was `rtl/UM6845R.v` before the 2026-08-22 split; that name
survives only in history. Use `Amstrad.qpf` as the project file; ignore legacy `Amstrad_Q13.*`.

## Writing test vectors

Write vectors where reading the RTL against the documented rule predicts a mismatch. The cheap
step comes first: read the ACCC rule and the RTL and decide whether they disagree. A batch of
vectors that all pass on first run bought regression armour, not a finding; do that only
deliberately.

**A test earns its place only if it could fail for a reason you did not already know.** A
vector asserting an ACCC rule you have just implemented is documentation with a `make` target.
Prefer vectors that pin a cross-module interaction, a degenerate case, or an unimplemented
rule. A large green suite that cannot surprise you is what lets a wrong core look verified.

This does not license skipping tests for behaviour changes. The CRTC wrapper and both engines
share state, findings routinely touch each other's state, and the Verilator suite is what
catches collateral damage, so the global test policy's keep rule always treats them as shared
state that other changes touch.

- Every behaviour change is proven by a focused deterministic vector that fails before the fix.
  A timing-sensitive finding does not start until its failing vector exists.
- Derive every expected value from the documented rule on paper and cite the ACCC section and
  page beside it. Never read an expectation back out of the simulator.
- When a finding is implemented, its named expected-failure (`XFAIL`) cases become required
  passes in the same commit, or are deleted there if they fail the keep rule. An `XPASS` fails
  the suite.
- Never weaken an assertion to go green. An unrelated test that starts failing is a finding.

## Commands

```sh
python3 sim/select_tests.py        # benches this change needs and why (index: sim/TESTS.md)
python3 sim/select_tests.py --run  # run them: the READY gate
make -C sim crtc-test              # CRTC pin tests only, ~20 s
make -C sim                        # every fast bench, parallel (JOBS=1 for serial output)
make -C sim full                   # every bench including slow fixtures, on demand only
make -C sim lint                   # verilator --lint-only, both suites
make -C sim soak SOAK_EXPECT=<hash>  # randomized equivalence soak against the golden hash
```

Requires Verilator 5+, GNU Make and a C++17 compiler (`brew install verilator`). Failing CRTC
tests leave a VCD at `sim/obj_dir/<test>.vcd`. There is no native Quartus on Apple Silicon.

## Gates

`python3 sim/select_tests.py --run` must pass before a branch with code, RTL, simulation or
build changes is marked READY or integrated; pure documentation changes skip simulation. It
runs only the benches the change can break, chosen from `sim/TESTS.md`. Intermediate commits
need only the focused target, such as `make -C sim crtc-test` or one `make -C sim/plus
<target>`. Slow benches are listed but run only with `--slow`, added only when the change
targets what one protects and no fast bench shows it. A new bench needs a `sim/TESTS.md` row;
`--check` fails otherwise.

**Soak golden hash**: the current value is the top row of `sim/soak-golden-history.md`. A
behaviour-preserving change must reproduce it; a hash change on a refactor means behaviour
moved, so stop and explain why. An intended behaviour change re-mints it: add a row there.

One gate run per change set. Whoever makes the last code edit runs the selection once and
reports the exact command and final `select_tests:` line; everyone else trusts that report.

- No baseline run before starting work or delegating: `master` is already green in CI.
- The parent accepts a delegate's green result when the diff matches the report and nothing
  was edited since; confirm it in the bridge run's `output.log`, do not rerun.
- Reviewers do not rerun the gate. They may run a focused bite-test the brief names; any other
  check goes back to the parent.
- A green CI simulation job on the exact SHA counts as the gate run.
- Rerun only after a later code edit, a failure, or a missing or mismatched report.

## CI and synthesis

GitHub Actions runs the simulation selection on every non-documentation push. Pinned Quartus
17.0.2 synthesis runs automatically on every push to `master` that touches anything Quartus
compiles, plus pull requests, tags and manual dispatches, at full effort to produce
hardware-testable RBFs. Stream branches stay on simulation until integration. Hardware results
outrank simulation but never replace it.

The integration push workflow picks the local or hosted runner; do not duplicate it with a
manual dispatch just because the VM is online. Dispatch by hand only for an explicitly
requested pre-merge or milestone answer, or a semantic risk no path reveals (top-level wiring,
clocks, memory arbitration, RGB width): prefer `local-build.yml --ref <branch> -f effort=full`
when `quartus-vm` is online, hosted `build.yml` otherwise. Runner registration:
`local/infra/ansible/local-runner.yml` (ignored, main checkout). Details: `docs/ci-testing-policy.md`.

Runs supersede instead of queueing: a newer run on the same ref and event cancels the older,
and among Quartus builds the newest cancels the oldest across refs via
`build-core-synthesis`. A `cancelled` run means *superseded*, never failed: find its successor
with `gh run list --branch <ref> --limit 5` and judge that one.

## Independent review

Every non-trivial code diff needs a fresh cross-provider review, so no model reviews its own
work alone; documentation-only changes skip review. When no cross-provider reviewer is
available, merge anyway and add a row to `docs/review-debt.md` in the same commit, naming what
a reviewer should check hardest. Delegated implementation stays provisional until the parent
has read the diff.

## ACCC attribution

The Compendium is licensed CC BY-NC-ND 4.0 and its section 2.2 requires the credit line in the
source headers of CRTC emulation modules and in the credits of any distributed product built
from them. `rtl/CRTC.v` and `sim/sim_main.cpp` carry it. Any new module implementing CRTC
behaviour from the Compendium must carry it too, and individual rules cite their ACCC section
at the point of implementation.
