---
name: stream-finish
description: Finish a task branch, merge into master, reconcile shared docs, run relevant gates, and push by default with exact-SHA CI/artifact verification. Accepts any task scope and ad-hoc worktree.
---

# stream-finish

Explicit invocation means local integration **and push by default**. `--no-push` requests a
local finish; do not add another permission question when this invocation already authorizes
publication. Discussing or editing the skill is not invocation. Read
[the common workflow](../../../docs/task-workflow.md) for checkout routing and cleanup.

## Inputs

- **Source**: current task branch by default, or explicit branch/worktree/task ID. Legacy
  `accuracy`/`plus` and `general` select a scope only when exactly one eligible READY task
  matches. With multiple candidates, resolve the source before mutation; never guess a path.
- **--no-push**: local merge only, without publication/build dispatch.
- **Destination**: `master` by default; explicit alternate destination allowed.
- **starting-base**: optional original base SHA from the task handoff.

## Pre-flight

Resolve actual source checkout, branch and exact tip, and the checkout holding the destination
using Git and the task handoff. Confirm no competing writer/in-progress Git operation and
preserve all dirty work. A sandboxed task unable to access the destination must hand off READY
for an integration task, not redirect Git around host restrictions. No fixed paths or leases.

Read the source diff and review evidence/debt. Require the repository's gates and fresh
cross-provider review for non-trivial changes, or its documented review-debt fallback.
Docs-only changes require neither simulation nor independent review. A task handoff must
state unresolved evidence, including hardware-only residuals.

Fetch origin. Fast-forward a clean behind destination; on divergence, reconcile explicitly
without discarding either history or force-pushing. Record the exact destination SHA. Verify
`starting-base`, when supplied, is ancestral to the source and expected integration history;
account explicitly for dependent branches rather than merging an unrelated history.

Update the source against this destination in its owning checkout. For an unpublished branch,
use `git rebase --rebase-merges <destination-sha>` when appropriate. Preserve published/shared
branch history by merging the destination into the source instead, unless rewriting that
history was explicitly requested. Conflict resolutions or changed interfaces require affected
gates and refreshed review evidence; an old review is not proof of a materially changed diff.
In a coordinated run, let the source owner produce the refreshed READY tip before integrating.

## Merge and reconcile

In the clean destination checkout, recheck its SHA against the recorded value and recheck the
source tip. If either moved, inspect and refresh the plan/gates. Merge the exact accepted tip
with `git merge --no-ff --no-commit <source-sha>`. If already integrated, do not manufacture a
merge; inspect any remaining requested publication or artifact work.

Resolve textual conflicts and perform a semantic audit even after a clean merge:
- `docs/current-status.md`: both tasks' progress, residuals and current focus remain accurate.
- `docs/review-debt.md`: preserve active debt, tie evidence to the reviewed changes.
- `docs/implementation-roadmap.md`: completion/dependencies reflect the combined state.
- `AGENTS.md` and `sim/README.md`: golden hashes and their explanations agree.

Inspect the full staged change, not just extension names. Pure documentation changes skip
simulation. RTL, simulation, testbench and build-manifest changes require `make -C sim` in
the merged checkout; use lint/soak as required by repository policy. For behavior-preserving
CRTC changes, compare soak against the recorded expected hash. Files under `docs/` are not
automatically documentation if they execute or affect builds. Run the merged gate once;
repeat only after changes or failures justify it. Never weaken assertions to pass.

Commit the merge after gates pass. Do not combine unrelated accuracy and Plus behavior into
one feature commit; their branches integrate separately. For --no-push, report the local SHA
and remaining publication/build work without inventing CI evidence.

## Push, CI and artifact delivery

Push the destination without force. A non-fast-forward rejection means concurrent history:
fetch, reconcile and rerun affected gates before retrying. Keep valid local work. Do not reset
a completed merge merely because publication failed.

Let the push workflow's `synthesis-policy` decide routing. Do not dispatch `local-build.yml`
merely because the VM is online: the push workflow already selects the runner. Manual dispatch
is for an explicitly requested pre-merge/milestone answer not provided by normal policy.
Follow `docs/ci-testing-policy.md`:
- Verify the pushed SHA and each required CI job, not only aggregate green. Inspect run
  `headSha`, jobs and artifacts; follow cancellation to its successor and state which SHA
  its evidence covers. A successor for another tip does not prove this exact tip passed.
- If synthesis is unnecessary, verify changed paths with
  `scripts/ci/classify-synthesis-paths.sh`; reuse the latest successful full-effort ancestor
  RBF and report both artifact commit and new tip. Do not relabel it as a new build.
- If synthesis is required, follow the selected leg through completion. Download the exact
  artifact; verify timing/resource reports and RBF SHA-256. Copy the SHA-bearing RBF into
  `output_files/` in the resolved integration checkout and verify the copied hash. Report a
  clickable absolute local path. Remote-only or temporary-only artifacts are not delivery.

Report **INTEGRATED** with source/integration SHAs, relevant shared-interface changes, CI jobs,
artifact provenance and hardware residuals. Notify the coordinator when present. Cleanup is
separate from successful integration and must preserve unmerged/dirty/private work.
