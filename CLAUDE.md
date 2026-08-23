# Working rules for this repository

This is a fork of the MiSTer Amstrad CPC core. Two work streams run in parallel and must not
be merged into one commit or one PR: classic CRTC accuracy for types 0 (HD6845S) and 1
(UM6845R), and Amstrad Plus/GX4000 ASIC support.

Start from `docs/implementation-roadmap.md` for dependency order and acceptance gates,
`docs/current-status.md` for the handoff state, and `docs/accuracy/audit-findings.md` for the
numbered findings F1-F12.

## Where authority lies

Sources rank in this order, and a lower rank never overturns a higher one:

1. Real hardware, and the Logon System reference photographs on `shaker.logonsystem.eu` that
   record it. SHAKER results are judged by visual comparison against those photographs. The
   stock upstream core is a regression baseline only: shared inaccuracy is invisible against it.
2. The Amstrad CPC CRTC Compendium (ACCC) v1.10, `docs/ACCC1.10-EN.pdf`.
3. The checked-in digests under `docs/accuracy/`.

The ACCC is our working oracle but it is not the final authority. A vector that encodes a
misreading of the Compendium passes cheerfully and hides the very bug it was meant to catch.
When simulation and hardware disagree, hardware wins and the vector is wrong.

`docs/ACCC1.10-EN.pdf` is user-owned and deliberately untracked. Never commit it.

## Writing test vectors

Write vectors where reading the RTL against the documented rule predicts a mismatch. Do not
write blanket coverage for behaviour nobody suspects.

The cheap step comes first: read the ACCC rule, read the corresponding RTL, and decide whether
they actually disagree. That costs a fraction of a vector and it is what turns testing into
progress. A batch of vectors that all pass on first run bought regression armour, not a
finding; that is occasionally worth doing on purpose, but it should be a deliberate choice
rather than the default motion.

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

`make -C sim` must pass before every commit. GitHub Actions then runs pinned Quartus 17.0.2
synthesis; any commit touching top-level wiring, clocks, memory arbitration, RGB width, or
`files.qip` needs its own run. Hardware results outrank simulation but never replace it.

## Independent review

The project requires a fresh cross-provider review of every non-trivial diff, so no model is
the sole reviewer of its own work. That capacity is currently unavailable. Work merged without
it gets a row in `docs/review-debt.md` in the same commit that introduces it, naming what a
reviewer should look at hardest. Delegated implementation stays provisional until the parent
has read the diff and rerun the gate itself.

## ACCC attribution

The Compendium is licensed CC BY-NC-ND 4.0 and its section 2.2 carries an explicit attribution
directive: the credit line belongs in the source headers of CRTC emulation modules and in the
credits of any distributed product built from them. `rtl/CRTC.v` and `sim/sim_main.cpp`
carry it. Any new module implementing CRTC behaviour from the Compendium must carry it too,
and individual rules cite their ACCC section at the point of implementation.
