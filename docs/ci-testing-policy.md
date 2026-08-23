# CI testing and synthesis policy

The fast feedback loop and the FPGA integration gate have different jobs.  Verilator should
reject behavioral regressions on every code commit; a full Quartus compile should prove that a
meaningful integration checkpoint still elaborates, fits, meets timing, and produces an RBF.
Generating an RBF which nobody intends to test is not evidence by itself.

## Verification tiers

### Tier A: behavioral verification

Every non-documentation push and every pull request runs:

```sh
make -C sim clean test lint
```

This is the per-commit gate for production RTL, simulation vectors, co-simulation manifests,
and repository tooling.  A failed Tier A run blocks every higher tier.

### Tier B: full Quartus integration

The full `quartus_sh --flow compile Amstrad.qpf` job runs automatically when a push changes a
known synthesis boundary:

- the project, constraint, or production manifest files (`Amstrad.qpf`, `Amstrad.qsf`,
  `Amstrad.sdc`, `files.qip`);
- the production top level (`Amstrad.sv`), platform sources under `sys/`, or a nested QIP;
- the known motherboard, SDRAM, and RGB integration modules, the qip-listed GA40010
  netlist-recreation sources (the netlist is a frozen reference — any source edit is an
  integration event even when `ga40010.qip` itself is unchanged), or the u765 FDC
  controller; or
- the CI workflow or its synthesis-path classifier.

It also runs for every non-documentation push to the repository's default branch, every pushed
tag, and every manual workflow dispatch. Pull requests use the same path classifier as feature
branches; an internal-RTL milestone still needs its manually dispatched build before merge. The
exact path policy is executable and tested in `scripts/ci/classify-synthesis-paths.sh`.

Internal RTL commits may share one Tier B run at a named milestone after every constituent
commit has passed Tier A.  Path classification cannot recognize semantics, so manually dispatch
a Tier B build on the exact commit whenever a change affects top-level wiring, clocks, memory
arbitration, RGB width, a newly synthesizable source, or another integration boundary not listed
above:

```sh
gh workflow run build.yml --ref <branch-or-tag>
```

Confirm that the named ref points at the intended commit before dispatching; GitHub workflow
dispatch accepts a branch or tag, not an arbitrary detached commit SHA.

Use the same manual route before handing off a bitstream for real-hardware testing. Record the
commit, Actions run, fitter utilization, worst timing result, hardware purpose, and the retained
`quartus-cache.txt` clean/incremental provenance together.

### Tier C: real hardware

Real MiSTer and SHAKER checks remain deliberate milestone tests.  They are required where the
roadmap names them, but are neither automated nor expected for every successful RBF.

## Quartus database reuse

GitHub currently includes 10 GB of Actions cache storage per repository, and standard hosted
runner time is free for public repositories. Feature-branch pushes which require Tier B restore
and save a cache scoped to that branch. Pull requests, tags, and default-branch builds remain
clean. Manual milestone builds are clean by default; set `clean_quartus_build=false` when the
purpose is iterative development on a feature branch and database reuse is wanted. The override
is ignored for tags and the default branch so their clean-build rule remains an invariant.

Only `db/` and `incremental_db/` are cached, and saving occurs only after a successful compile
and package. The artifact retains the build mode plus the primary and restored cache keys in
`reports/quartus-cache.txt`, so an incremental RBF cannot be mistaken for a clean one.
The RBF filename itself does not encode the mode: retain this report with any RBF copied out of
the artifact bundle.

The current project has one root source partition, so database reuse may save little after a real
integration change. Treat the first measurements as a decision gate: disable automatic reuse if
cache transfer does not improve total feedback time or if clean/cached fitter evidence diverges.

Benchmark database reuse in a small series:

1. let a qualifying feature-branch push seed its branch cache, or manually dispatch with
   `clean_quartus_build=false`;
2. run the next qualifying integration change on that branch to measure reuse after a real
   change;
3. manually dispatch that exact branch ref with the default `clean_quartus_build=true` to build
   the same commit cleanly;
4. compare Analysis & Synthesis, Fitter, total job time, cache transfer time, database size,
   timing slack, and fitter utilization between the cached and clean runs.

Change the cache epoch in `.github/workflows/build.yml` whenever the Quartus image or database
policy changes. Periodically compare a cached development build against a clean manual, tagged,
or default-branch build. GitHub evicts cache entries under its repository limit, so cache churn
may reduce the hit rate without affecting correctness. GitHub's current allowances and billing
rules are documented in
[GitHub Actions billing](https://docs.github.com/en/billing/concepts/product-billing/github-actions).

## Local Quartus route

The existing Quartus VM is a second route to Tier B, not a weaker standard.  Bringing it into
routine routing and benchmarking clean versus incremental builds is a separate workstream; use
the same RBF, fitter, timing, and commit-provenance acceptance evidence when that route is
enabled.
