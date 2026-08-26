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

**Queued hardening (2026-08-26 hardware session).** The classic-video black screen shipped
because no gate elaborates `Amstrad.sv` and Quartus reports implicit nets only as Warning
10236, three commits' worth of which sat in every build log since `3d7a178` unnoticed.
Two queued guards, cheapest first:

1. **Cheap**: fail the synthesis-policy job when the compile log contains
   `Implicit Net warning` (Warning 10236) — catches this exact class in any synthesized
   file for one line of policy script.
2. **Full**: elaborate `Amstrad.sv` under Verilator in the Tier A lint chain with
   IMPLICIT/UNDRIVEN fatal, like the motherboard hierarchy pass. Needs generated-file and
   sys-leaf stubs first (`build_id.v` define stub, `ltc2308_tape`, hps_io dependencies).

### Tier B: Quartus integration

A Quartus compile proves that a meaningful integration checkpoint still elaborates, fits,
meets timing, and produces an RBF.  The Quartus database cache does not shorten it (measured
2026-08-24: 12.2 min on a cache hit, 12.2 min on a clean build — see queue item D2), so the
policy runs it where a milestone actually forms, and nowhere else.  Since 2026-08-26 it runs
at two effort levels (below); the *trigger* rules did not change.

**Effort tiers.** Both tiers are the same `quartus_sh --flow compile Amstrad.qpf`; they differ
only in fitter settings, applied by `scripts/ci/apply-quartus-effort.sh`:

- **full** — the checked-in QSF unchanged (`STANDARD FIT`, `HIGH PERFORMANCE EFFORT`, the
  physical-synthesis suite, `FINAL_PLACEMENT_OPTIMIZATION ALWAYS`).  This is the evidence
  tier: its RBFs are the only ones retained for hardware handoff.
- **smoke** — appends `FAST FIT`, the physical-synthesis suite off,
  `FINAL_PLACEMENT_OPTIMIZATION Never`, and periphery-to-core optimization off to the QSF at
  compile time.  Fast signal that an integration tip still elaborates, fits, and roughly
  meets timing; not hardware-build evidence.  Two constraints learned empirically from the
  pinned container (run `32919565759`): illegal assignment *values* are fatal project-open
  errors, not ignorable warnings (`OPTIMIZATION_MODE` has no compile-time-aggressive value in
  this edition), and an edition that silently drops an assignment warns with
  `Ignored assignment:`.  A post-compile guard fails the smoke leg if the log contains that
  string, so a quiet fall-back to full cost cannot masquerade as smoke.

Measured on one SHA with both legs in one run (`32921078236`, branch tip `b68d4be`,
Quartus inputs identical to milestone `dcbc6ad`):

| leg | map | fit | asm | sta | flow |
|---|---|---|---|---|---|
| full | 2:14 | 12:07 | 0:13 | 0:09 | 14:48 |
| smoke | 2:45 | 10:41 | 0:16 | 0:12 | 14:00 |

Read that table together with the spread of full-effort fitter times for *identical* code:
12:07 here versus 15:32 / 15:56 / 16:19 on 2026-08-25/26 (runs `32905376888`,
`32914211795`).  Runner-to-runner variance on free ubuntu-latest is larger than the tier
delta of a single comparison; treat any single-run timing as one sample and use a dispatched
`effort=both` run whenever a tier or setting claim needs to be defended.  The structural
argument matters more than the single-sample numbers: smoke disables exactly the expensive
passes (physical synthesis, final placement refinement) whose work explodes when a change
crosses a congestion cliff like the P4 pass-2 event, so its worst case stays bounded while
full effort's does not.

**Where each runs.** The trigger rule is unchanged from before the tiers existed — Tier B is
triggered by *where a change has arrived*, not by which file it touched.  The event class
then picks the effort:

- **Integration branches** — the default branch and `accc-review-and-fixes` (the
  `INTEGRATION_BRANCHES` list in `build.yml`): every push whose changed set affects the build,
  at **smoke** effort.  This is the automatic replacement for the old "named milestone"
  ritual.  A merge is a push to an integration branch, so merging a stream branch synthesizes
  the result with nobody having to remember.
- **Pull requests**: the same path test, at **full** effort — pre-merge evidence for anyone
  who wants it.
- **Every pushed tag and every manual workflow dispatch**: unconditionally, at **full**
  effort.  The dispatch input also accepts `smoke`, or `both` to benchmark the two tiers on
  one SHA.
- **Stream branches**: never.  Tier A only.  The same code would otherwise be synthesized
  twice — once on the branch and again when it merges.

Two operational notes.  First, simulation and synthesis jobs run in parallel (they share no
state; the required gate still enforces both), so a red simulation no longer saves the
synthesis compute on that run.  Second, dispatched builds are not privileged: they are
superseded by newer builds under exactly the rules of the next section, so dispatch into a
quiet window when a milestone RBF must complete undisturbed.

**Run supersession (2026-08-26).** Builds cancel each other instead of queueing, at two
levels:

- **Workflow level**, per ref+event (`build-Build core-<ref>-<event>`): a newer push, PR
  sync, or manual dispatch with the same ref and event type cancels the older run wholesale —
  simulation jobs included.
- **Synthesis-job level**, repository-wide (`build-core-synthesis`, shared with
  `local-build.yml`): every Quartus leg contends for a single slot regardless of branch,
  event, or effort tier, and the newest arrival cancels whichever compile is in flight. Runs
  whose policy decision skips synthesis never join the group, so a Tier-A-only stream push
  cannot interrupt any build.

The retired rule this replaces: dispatched milestones used to be uncancellable and queued
FIFO behind each other, so iterating by repeated dispatch stacked ~25-minute runs and the
policy had to warn against dispatching right after pushing to an integration branch.

Consequences for humans and agents watching runs:

- A superseded run ends with conclusion `cancelled`. That means *superseded*, never failed,
  and it is not regression evidence. Find the successor before diagnosing anything:
  `gh run list --branch <ref> --limit 5` (or match the head SHA) and judge that run.
- The required-gate check of a cancelled run stays red or pending by design; only the newest
  synthesizing run on a ref is meaningful.
- An `effort=both` benchmark serializes its legs (`max-parallel: 1`) because parallel matrix
  legs would cancel each other through the shared slot.
- A milestone build that gets superseded anyway can be replayed exactly:
  `gh run rerun <run-id>` re-runs that attempt on the same SHA.

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
`quartus-cache.txt` build-mode provenance together.  The provenance value names the tier:
`clean_full` for milestone evidence, `clean_smoke` for integration-tip feedback — a smoke RBF
is not a hardware-test artifact.

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

## Local Quartus route and agent routing policy

The existing Quartus VM is a second route to Tier B, not a weaker standard. Since
2026-08-26 it is reachable as a self-hosted GitHub runner: register the runner once with
`ansible/local-runner.yml` (e.g. `ansible-playbook -i ansible/inventory.yml ansible/local-runner.yml`),
then dispatch `.github/workflows/local-build.yml`:

```sh
gh workflow run local-build.yml --ref <branch-or-tag> -f effort=full|smoke
```

The local leg joins the same repository-wide synthesis slot (`build-core-synthesis`) as hosted builds,
so local and hosted compiles supersede each other under the run supersession rules above. Acceptance
evidence is identical: RBF, fitter summary, TimeQuest report, and the same `quartus-cache.txt` provenance values.
The VM executes repository-controlled Tcl and RTL directly, without the container boundary the hosted route gets
from Docker, which is why the checked-in workflow is dispatch-only and also guards its event and repository.
Those controls do not secure the runner label against a malicious edit to another workflow; the disposable VM is
the isolation boundary for this public repository. Provisioning, registration, and removal are documented in
`ansible/README.md`; iteration-speed background remains in `docs/building.md`.

### Performance benchmark (exact SHA `27cb993`, 2026-08-26)

Measured clean compiles on exact commit `27cb9930a8f9dc074ae83ae16b2e23a9fd0a175d` comparing GitHub Actions
hosted runners (`ubuntu-latest` amd64 container) versus the local Debian 13 UTM VM (Apple Silicon M-series,
6 vCPUs, 8 GB RAM, Rosetta 2 emulation):

| Tier | Route | Map | Fit | Asm | Sta | Flow Total | Total Job Turnaround |
|---|---|---|---|---|---|---|---|
| **Smoke** | Hosted GHA (`32970235843`) | 3:17 | 10:23 | 0:19 | 0:12 | **14:18** | **16:00** |
| **Smoke** | Local UTM VM (`quartus-vm`) | 2:20 | 9:14 | 0:13 | 0:10 | **12:03** | **~12:15** |
| **Full** | Hosted GHA (`32970234749`) | 3:13 | 15:46 | 0:19 | 0:13 | **19:38** | **21:17** |
| **Full** | Local UTM VM (`quartus-vm`) | 2:23 | 15:59 | 0:13 | 0:10 | **18:45** | **~19:00** |

*Note on Rosetta emulation:* In `Amstrad.qsf`, `NUM_PARALLEL_PROCESSORS ALL` causes `quartus_map` to spawn
multi-process helper children communicating over Linux named pipes (`mkfifo`), which deadlock on `wait_for_partner`
under Rosetta 2 virtualization. Overriding `NUM_PARALLEL_PROCESSORS 1` during local VM compilation avoids this IPC
bottleneck and allows `quartus_map` to execute single-process multi-level synthesis in 2m20s.

### Agent routing policy

1. **Automatic triggers remain hosted**: Non-documentation pushes to integration branches (`master`,
   `accc-review-and-fixes`), pull requests, and pushed tags always run through hosted GitHub Actions.
2. **Manual dispatches prefer local when online**: For deliberate Tier B milestone dispatches, pre-merge
   semantic-risk checks (top-level wiring, clocks, memory arbitration, RGB width), and bitstream builds for
   real-hardware handoff, **prefer dispatching to the local VM runner** when it is reachable (`quartus-vm` status `online`):
   ```sh
   gh workflow run local-build.yml --ref <branch-or-tag> -f effort=full|smoke
   ```
   Local compilation is faster in both tiers (~4m faster turnaround for smoke, ~2m20s faster for full) and saves
   GitHub runner queue latency.
3. **Hosted fallback when VM is offline**: If `quartus-vm` is offline or unreachable, dispatch hosted `build.yml`
   without hesitation:
   ```sh
   gh workflow run build.yml --ref <branch-or-tag> -f effort=full|smoke
   ```
4. **Never dispatch both routes concurrently for the same SHA**: Both routes share the single
   `build-core-synthesis` concurrency group, so a newer dispatch will cancel the earlier in-flight build.

