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

The full `quartus_sh --flow compile Amstrad.qpf` job costs about **12 minutes** and the Quartus
database cache does not shorten it (measured 2026-08-24: 12.2 min on a cache hit, 12.2 min on a
clean build — see queue item D2). So the policy runs it where a milestone actually forms, and
nowhere else.

**Where it runs.** Tier B is triggered by *where a change has arrived*, not by which file it
touched:

- **Integration branches** — the default branch and `accc-review-and-fixes` (the
  `INTEGRATION_BRANCHES` list in `build.yml`): every push whose changed set affects the build.
  This is the automatic replacement for the old "named milestone" ritual. A merge is a push to
  an integration branch, so merging a stream branch synthesizes the result with nobody having
  to remember.
- **Pull requests**: the same path test, giving pre-merge signal to anyone who wants it.
- **Every pushed tag and every manual workflow dispatch**: unconditionally.
- **Stream branches**: never. Tier A only. The same code would otherwise be synthesized twice —
  once on the branch and again when it merges — at 12 minutes each.

**What counts as affecting the build** is decided by `scripts/ci/classify-synthesis-paths.sh`,
which no longer keeps a hand-written list of RTL. It resolves `files.qip` transitively through
nested QIPs via `scripts/ci/list-synthesized-sources.sh`, so *every source Quartus compiles is
covered the moment it is added to a manifest*. The old allowlist duplicated part of the manifest
and went stale twice, hiding the GA40010 netlist sources and then the u765 controller until a
review noticed. Only files no manifest can reach are listed by name: the Quartus project files,
the CI definition, the `sys/` platform tree, and the PLL chain — which hangs off a *Tcl-computed*
QIP name (`sys/sys.qip` builds `pll_q17.qip` from `$quartus(version)` at run time) that a static
walk cannot follow. **A new Tcl-computed reference would need the same explicit treatment; the
walk will not warn you.**

Path classification still cannot recognize semantics. Merging now covers the common case, but
dispatch a Tier B build on the exact commit when you need the answer *before* a merge, or when a
change affects top-level wiring, clocks, memory arbitration, or RGB width in a way no path
reveals:

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
