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
`quartus-cache.txt` build-mode provenance together.

### Tier C: real hardware

Real MiSTer and SHAKER checks remain deliberate milestone tests.  They are required where the
roadmap names them, but are neither automated nor expected for every successful RBF.

## Quartus database reuse (removed 2026-08-24, D2)

The branch Quartus database cache (`db/` + `incremental_db/`) was removed after the D2
investigation concluded it cannot help this project. Evidence, from the cache-restored run
`32657783842` (`quartus_database_reuse` mode) versus clean runs:

- Restoring the databases saved zero time: the flow took the same ~12 minutes either way
  (the queue item's original observation). The fitter report shows why — the design has no
  QSF design partitions exporting post-fit netlists (the only partition is the default
  `Top` with `Netlist Type Used: Source File`), so `quartus_sh --flow compile` re-runs
  analysis & synthesis and re-fits from source on every build and merely overwrites the
  restored databases. No "incremental" reuse message appears anywhere in the build log.
- Per-stage split on the restored run: Analysis & Synthesis 1:37, Fitter 8:07 (dominant),
  Assembler 0:14, TimeQuest 0:08 — 10:06 flow total; the rest of the 12.2 min is container
  and job overhead.
- The primary cache key embedded `github.sha`, so the exact key could never hit; only the
  branch-prefix restore key matched, which is why every run "restored successfully" while
  saving nothing.

Every synthesis is now a clean compile and `reports/quartus-cache.txt` records
`build_mode=clean` for provenance continuity. Do not reintroduce a database cache without
also introducing a partitioned incremental-compilation policy (QSF partitions with
exported post-fit netlists and their review burden); a cache alone is dead weight.

Manual milestone dispatch keeps its `workflow_dispatch` route; the former
`clean_quartus_build` input is gone because every build is clean.

## Local Quartus route

The existing Quartus VM is a second route to Tier B, not a weaker standard.  Bringing it into
routine routing and benchmarking clean versus incremental builds is a separate workstream; use
the same RBF, fitter, timing, and commit-provenance acceptance evidence when that route is
enabled.
