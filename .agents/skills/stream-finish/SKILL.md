---
name: stream-finish
description: Finalizes and merges a stream branch back into accc-review-and-fixes in the main worktree. Handles simulation gates (skipping for doc-only changes), git merge conflicts, semantic reconciliation of shared docs (current-status, review-debt, roadmap, golden hashes), and triggers CI synthesis.
---

# stream-finish

Finalizes the current stream branch, performs semantic merge into `accc-review-and-fixes`, validates gates, and pushes to trigger CI synthesis.

## Inputs
- **`stream`** (required): `accuracy` or `plus`
- **`push`** (optional flag): Push to GitHub origin by default. Pass `--no-push` only if performing an intermediate local merge without triggering synthesis.

## Worktree Mapping
- `accuracy` -> `/Users/renaudg/code/Amstrad_MiSTer-accuracy`
- `plus`     -> `/Users/renaudg/code/Amstrad_MiSTer-plus`
- Main       -> `/Users/renaudg/code/Amstrad_MiSTer` (Branch: `accc-review-and-fixes`)

## Procedure

### Phase 1: Stream Worktree Pre-flight
1. In `/Users/renaudg/code/Amstrad_MiSTer-<stream>`:
   - Identify active feature branch: `git rev-parse --abbrev-ref HEAD`.
   - Confirm working tree is clean: `git status --porcelain`.
2. Confirm review requirements:
   - Check if cross-provider review occurred or if a debt row was logged in [`docs/review-debt.md`](file:///Users/renaudg/code/Amstrad_MiSTer/docs/review-debt.md).

### Phase 2: Merge in Main Worktree
1. Switch to the main worktree `/Users/renaudg/code/Amstrad_MiSTer`:
   ```sh
   git -C /Users/renaudg/code/Amstrad_MiSTer checkout accc-review-and-fixes
   ```
2. Attempt the merge without auto-committing:
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

### Phase 5: Commit & Push
1. Commit the merge:
   ```sh
   git -C /Users/renaudg/code/Amstrad_MiSTer commit -m "Merge branch '<stream-branch>' into accc-review-and-fixes"
   ```
2. Unless `--no-push` was requested, push to origin:
   ```sh
   git -C /Users/renaudg/code/Amstrad_MiSTer push origin accc-review-and-fixes
   ```
   *(Note: Pushing `accc-review-and-fixes` automatically triggers the Quartus CI synthesis build if code files changed).*
3. Summarize the completed merge and report the triggered GitHub Actions run if pushed.
