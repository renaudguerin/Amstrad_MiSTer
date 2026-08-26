# Synthesis cost audit — 2026-08-26

Why Tier B went from ~12 to >20 minutes, what was done about it on the CI side, and the
finding handed to the Plus track.  All figures are from GitHub Actions job logs of runs named
below (`gh run view <id> --job <job> --log`, `Info: Elapsed time` lines).

## The slowdown is real and localized to one merge

Per-stage flow times (quartus_map / quartus_fit / assembler / TimeQuest / total):

| run | commit state | map | fit | asm | sta | flow |
|---|---|---|---|---|---|---|
| `32789356344` (Aug 24 23:26 push) | pre-P4 | 2:44 | 9:18 | 0:15 | 0:09 | 12:34 |
| `32845921357` (Aug 25 12:07 push) | pre-P4 | 2:28 | 8:17 | 0:15 | 0:08 | 11:14 |
| `32852900420` (Aug 25 13:22 push) | P4 sprites merged (`0dabcb8` + docs) | 2:34 | 8:44 | 0:15 | 0:08 | 11:48 |
| `32905376888` / `32905405634` (Aug 25 22:16 push + dispatch, same SHA) | P4 pass-2 merged (`85b0eaa`) | 3:08 / 3:20 | 15:32 / 16:19 | 0:19 / 0:20 | 0:12 / 0:13 | 19:19 / 20:19 |
| `32914211795` (Aug 26 00:13 dispatch) | pass-2 tip `dcbc6ad` | 3:15 | 15:56 | 0:20 | 0:12 | 19:51 |

Three facts fall out:

1. **The first P4 merge cost nothing** (8:44 fitter with 547 new sprite-engine RTL lines);
   the *small* review pass-2 remediation delta doubled fitter time (+80%).
2. **Runner variance is small**: the same-SHA push/dispatch pair differs by ~1 min of fitter
   time (~5%), and the three post-merge readings sit in a tight 15:32–16:19 band.  Pre-P4
   readings vary ~12%.  The cliff is not noise.
3. **Resource profile barely moved** — DSP-block register packing identical (785/228 both
   sides), utilization already at 72% ALMs / 34 / 112 DSPs per current-status.md.  Worst-case
   setup slack fell from +0.734 ns to +0.260 ns.  This is a placement/routing difficulty
   threshold being crossed, not logic growth.

## Finding for the Plus track (report only — no RTL change made here)

The entire RTL delta between the cheap and expensive builds is 12 lines in
`rtl/plus/asic_sprites.v`: the walker block increment `{walk[7], walk[6:3]} + 5'd1`
(review pass-2 finding 1, sprite-15 wrap into the opposite bank half) replacing
`{walk[7], walk[6:3] + 4'd1, 3'd0}`.  One extra carry bit into the walk register flipped the
fitter into a much harder problem.

Implications for whoever touches this next:

- The design sits just under a fitting threshold; small structural nudges can move fit cost
  or slack disproportionately.  Treat any further sprite-walker datapath change as a
  potential cliff event and compare fitter stage times against these baselines.
- Timing is still met (+0.260 ns) but margin thinned from +0.734 ns in the same step.
- If remediation is wanted, candidate directions (untested): compute the bank toggle outside
  the same adder chain as the sprite field, or register the increment result a cycle earlier;
  verify behaviour-preservation via an unchanged soak hash as usual.

## What was considered and rejected

- **Quartus database cache** — dead weight without design partitions; measured zero saving in
  D2 (see "Quartus database reuse" in `docs/ci-testing-policy.md`).  A cache cannot address a
  fitter-time cliff anyway.
- **Paid/larger runners** — out of budget, and the fitter dominates while being largely
  single-threaded; faster cores would shave minutes at most.
- **Docker image pull caching** — pull+start costs ~1:45 of the job; a cached multi-GB tarball
  saves roughly half of that after restore overhead.  Not worth the moving parts.
- **Partitioned incremental compilation** — the only caching lever that could actually pay,
  but it changes fit results and adds a netlist-review burden; explicitly deferred until
  someone chooses to own that policy (condition stated in ci-testing-policy.md).
- **Relaxing trigger rules** — rejected; where Tier B runs was already correct, only its cost
  per run needed fixing.

## What was implemented (CI-side)

1. **Two effort tiers** behind the unchanged trigger rules: routine default/integration-branch
   pushes run the **smoke** tier (`scripts/ci/apply-quartus-effort.sh` appends FAST FIT,
   AGGRESSIVE COMPILE TIME, physical-synthesis suite off, final-placement/periphery
   optimizations off); PRs, tags, and manual dispatches keep **full** effort and remain the
   only hardware-build evidence.  Dispatch accepts `effort=both` to run both tiers on one SHA
   for benchmarking.  Provenance values are now `build_mode=clean_full` /
   `clean_smoke`.
2. **Silent-degradation guard**: the smoke leg fails if the build log contains
   `Ignored assignment:` (Quartus drops assignments it cannot honor and continues at full
   cost).
3. **Simulation ∥ synthesis**: the synthesis job no longer waits for the Verilator job
   (~2.5 min wall-clock saving on every Tier B run); the required gate still enforces both.
4. **Double-synthesis avoidance is procedural**: dispatching right after pushing to an
   integration branch compiles the same SHA twice (observed repeatedly, e.g. runs
   `32905376888` + `32905405634`); documented in ci-testing-policy.md rather than engineered
   around.

## Validation status and watch items

- Classifier self-test passes locally (`scripts/ci/test-classify-synthesis-paths.sh`);
  workflow YAML parses; shell scripts are bash -n clean.  Full Verilator suite + lint green
  (303 passes) on the remediation branch.
- Cross-provider Gemini review of the whole diff returned CLEAR; one cosmetic suggestion
  declined (the audit-doc link style in current-status.md matches its sibling references).
- **Benchmark result** (dispatched `effort=both`, run `32921078236`, same SHA both legs):
  smoke fit 10:41 / flow 14:00 versus full fit 12:07 / flow 14:48 — and full-effort fit for
  identical code ranged 12:07–16:19 across runner draws on 2026-08-25/26, so free-runner
  variance exceeds the single-comparison tier delta.  Figures and the sampling rule are
  recorded in `docs/ci-testing-policy.md`.  The structural win stands regardless of the
  sample: smoke turns off physical synthesis and final-placement refinement, the passes
  whose cost exploded in the P4 pass-2 cliff.
- First benchmark attempt (`32919565759`) failed the smoke leg usefully: this container makes
  illegal assignment values a fatal project-open error (125036), so bad overrides can never
  degrade silently into a full-cost run.  `OPTIMIZATION_MODE` has no compile-time-aggressive
  value here (dropped); `FINAL_PLACEMENT_OPTIMIZATION` takes Always/Automatically/Never
  (fixed to Never).  The `Ignored assignment:` guard remains for the genuinely silent case.
- Empty-matrix semantics: when no synthesis leg is required the policy emits `[]`; the
  job-level `if` then skips the whole matrix before expansion, so the gate's expectation of
  result `skipped` holds by construction.  First sim-only integration-branch push will
  confirm live.
- Expected smoke effect on timing: worse slack by design (full-effort settings exist to buy
  it back).  The smoke leg asserts an RBF and STA report exist; it does not assert a slack
  bound.  If a smoke run ever fails timing after passing full on the parent PR, treat that
  as signal about the full run's margin, not a CI bug.
