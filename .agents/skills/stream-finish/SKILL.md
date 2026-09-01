---
name: stream-finish
description: Finalizes and merges a stream branch back into accc-review-and-fixes in the main worktree. Handles simulation gates (skipping for doc-only changes), git merge conflicts, semantic reconciliation of shared docs (current-status, review-debt, roadmap, golden hashes), and triggers CI synthesis.
---

# stream-finish

Finalizes the current stream branch, performs semantic merge into `accc-review-and-fixes`, validates gates, and pushes to trigger CI synthesis.

## Inputs
- **`stream`** (required): `accuracy` or `plus`
- **`push`** (optional flag): Push to GitHub origin by default. Pass `--no-push` only if performing an intermediate local merge without triggering synthesis.
- **`peer-task`** (optional): Codex task ID for the other stream.
- **`integration-base`** (optional): Exact current integration SHA supplied with the
  coordinator's lease. It is issued at finish time, not copied from task creation. In a
  coordinated run, do not proceed without it.
- **`starting-base`** (optional): The task's original integration SHA. It is an ancestor
  floor, not the expected current tip.

## Worktree Mapping
- `accuracy` -> `/Users/renaudg/code/Amstrad_MiSTer-accuracy`
- `plus`     -> `/Users/renaudg/code/Amstrad_MiSTer-plus`
- Main       -> `/Users/renaudg/code/Amstrad_MiSTer` (Branch: `accc-review-and-fixes`)

## Procedure

### Phase 1: Stream Worktree Pre-flight
1. Inspect the stream worktree `/Users/renaudg/code/Amstrad_MiSTer-<stream>`:
   - Identify active feature branch: `git rev-parse --abbrev-ref HEAD`.
   - Confirm working tree is clean: `git status --porcelain`.
2. Inspect the main worktree `/Users/renaudg/code/Amstrad_MiSTer`:
   - Fetch origin and confirm it is clean, on `accc-review-and-fixes`, and not in a
     merge/rebase operation. Ignored/private local assets are not dirt.
   - If local `accc-review-and-fixes` is behind its remote, fast-forward it in the main
     worktree before rebasing the stream. If they diverged, stop and reconcile; do not reset
     or force either side. In a coordinated run, the resulting local and remote SHA must
     equal `integration-base`; otherwise report a stale lease and wait for the coordinator
     to issue the new current SHA. Repeating the old lease is not a retry.
   - If `starting-base` was supplied, require it to be an ancestor of `integration-base` with
     `git merge-base --is-ancestor <starting-base> <integration-base>`.
   - Rebase the stream branch onto the current integration tip while preserving merge
     topology:
     ```sh
     git -C /Users/renaudg/code/Amstrad_MiSTer-<stream> fetch origin
     git -C /Users/renaudg/code/Amstrad_MiSTer-<stream> rebase --rebase-merges accc-review-and-fixes
     ```
     Resolve conflicts semantically. If main or origin moves before the merge, report the
     stale lease and obtain a newly issued `integration-base` before repeating this phase.
3. Confirm review requirements:
   - Check if cross-provider review occurred or if a debt row was logged in [`docs/review-debt.md`](file:///Users/renaudg/code/Amstrad_MiSTer/docs/review-debt.md).
4. In a coordinated run, report that the integration lease is active so the coordinator can
   relay it to the peer. Send directly only when peer messaging was explicitly confirmed.
   In a coordinated run, do not proceed without an explicit lease: stop at READY.

### Phase 2: Merge in Main Worktree
1. Switch to the main worktree `/Users/renaudg/code/Amstrad_MiSTer`:
   ```sh
   git -C /Users/renaudg/code/Amstrad_MiSTer checkout accc-review-and-fixes
   ```
2. Re-check that main still equals the pre-flight integration SHA. Never merge on top of an
   unexpected concurrent integration.
3. Attempt the merge without auto-committing:
   ```sh
   git -C /Users/renaudg/code/Amstrad_MiSTer merge <stream-branch> --no-ff --no-commit
   ```

### Phase 3: Conflict Resolution & Semantic Reconciliation
1. **Textual Merge Conflicts**: If git reports conflicts, inspect the files, resolve conflict markers, and stage them (`git add`).
2. **Semantic Document Audit** (Mandatory, even for clean merges):
   Inspect staged changes (`git diff --staged`) on shared files:
   - [`docs/current-status.md`](file:///Users/renaudg/code/Amstrad_MiSTer/docs/current-status.md): Reconcile progress from both streams. Ensure "Current Focus" and recent milestone logs coexist coherently without overwriting each other or inverting chronological order.
   - [`docs/review-debt.md`](file:///Users/renaudg/code/Amstrad_MiSTer/docs/review-debt.md): Ensure newly added debt rows from either stream do not clobber existing active debt entries.
   - [`docs/implementation-roadmap.md`](file:///Users/renaudg/code/Amstrad_MiSTer/docs/implementation-roadmap.md): Ensure completed checkboxes and section headers accurately reflect the merged state.
   - [`AGENTS.md`](file:///Users/renaudg/code/Amstrad_MiSTer/AGENTS.md) & [`sim/README.md`](file:///Users/renaudg/code/Amstrad_MiSTer/sim/README.md): If golden hashes were re-minted or closures added, ensure the golden hash strings and descriptions are consistent.

### Phase 4: Verification Gate (Run once; skip for doc-only)
1. Check staged files (`git diff --staged --name-only`):
   - **Doc-only change** (only `.md`, `.txt`, `docs/` files): **SKIP** `make -C sim` entirely per repo policy.
   - **Code/RTL/Sim change** (touches `rtl/`, `sim/`, `sys/`, `*.sv`, `*.v`, `*.qip`): Run `make -C sim` **once** in the main worktree:
     ```sh
     make -C /Users/renaudg/code/Amstrad_MiSTer/sim
     ```
     *(If CRTC behavior or hash changed, also run `make -C /Users/renaudg/code/Amstrad_MiSTer/sim soak`)*.

### Phase 5: Commit, Push & Build Dispatch
1. Commit the merge:
   ```sh
   git -C /Users/renaudg/code/Amstrad_MiSTer commit -m "Merge branch '<stream-branch>' into accc-review-and-fixes"
   ```
2. Unless `--no-push` was requested, push to origin:
   ```sh
   git -C /Users/renaudg/code/Amstrad_MiSTer push origin accc-review-and-fixes
   ```
   Never force-push. A non-fast-forward rejection invalidates the integration lease: abort
   the finish, fetch, rebase/reconcile, rerun affected gates, and try again only after the
   coordinator grants a fresh lease.
   *(Note: Pushing `accc-review-and-fixes` automatically triggers a full-effort Quartus synthesis build when code files changed, producing an RBF for hardware testing).*
3. Let the push-triggered `build.yml` policy decide whether synthesis is needed and, when
   needed, route it to the available runner. **Do not dispatch `local-build.yml` merely
   because the VM is online.** The push workflow already selects the local VM when appropriate;
   a second dispatch duplicates or cancels useful work.
   - Inspect the exact push run's `synthesis-policy` result. When it says synthesis is required,
     follow the automatically selected local or hosted leg.
   - When it says synthesis is not required, verify that the changed paths classify false with
     `scripts/ci/classify-synthesis-paths.sh`. Reuse the most recent successful full-effort RBF
     whose commit is an ancestor of the new tip, and report both the artifact commit and the new
     tip. This is valid because the classifier covers the transitive Quartus manifest plus the
     explicit build-affecting paths documented in `docs/ci-testing-policy.md`; simulation-only
     and documentation-only commits do not change the bitstream.
   - A manual `local-build.yml` dispatch is reserved for an explicitly requested milestone or
     pre-merge hardware answer that the ordinary push policy cannot provide.
4. Verify the exact pushed SHA and each required CI job, not only the aggregate workflow
   conclusion. Follow last-write-wins cancellation to the successor run. Summarize the merge,
   CI run, and RBF artifact once available.
   Use commands that bind evidence to the exact tip:
   ```sh
   git -C /Users/renaudg/code/Amstrad_MiSTer rev-parse HEAD origin/accc-review-and-fixes
   gh run list --branch accc-review-and-fixes --limit 5 --json databaseId,headSha,status,conclusion,url
   gh run view <run-id> --json headSha,status,conclusion,jobs,url
   ```
   Follow the run-supersession policy in `docs/ci-testing-policy.md` and inspect the exact
   run's artifacts before naming the RBF.
5. In a coordinated run, report an **INTEGRATED** message with the integration SHA, changed
   shared files, CI job results, artifact, and residual hardware gates. The coordinator
   relays it to the peer by default; send directly only when that capability was confirmed.
   Release the integration lease only after the push and required CI state are known.
