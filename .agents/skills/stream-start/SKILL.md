---
name: stream-start
description: Prepares a stream worktree (accuracy or plus) for a new task by bringing it up to date with the latest accc-review-and-fixes tip and cutting a new feature branch. If topic is omitted, autofills it from the stream's next pending roadmap item.
---

# stream-start

Prepares the dedicated stream worktree for a new task and branches from the integration tip.

## Inputs
- **`stream`** (required): `accuracy` or `plus`
- **`topic`** (optional): Short descriptive branch topic (e.g. `f20-type0-c0-timing`, `p10-dma-audio`).
  - **Autofill rule**: If `<topic>` is omitted, read [`docs/current-status.md`](file:///Users/renaudg/code/Amstrad_MiSTer/docs/current-status.md) and [`docs/implementation-roadmap.md`](file:///Users/renaudg/code/Amstrad_MiSTer/docs/implementation-roadmap.md) (or `docs/accuracy/audit-findings.md` / `docs/plus/`) to determine the next pending task for that stream, format a concise slug, and state the chosen topic to the user.
- **`peer-task`** (optional): Codex task ID for the concurrently running other stream. Omit
  for an ordinary single-stream task.
- **`integration-order`** (optional): Integration order assigned by `$stream-orchestrate`.
- **`starting-base`** (optional): Coordinator's pre-flight integration SHA. Verify it remains
  an ancestor after refreshing integration; the actual branch-base SHA reported by this
  skill becomes the canonical `starting-base` carried into `$stream-finish`.

## Worktree Mapping
- `accuracy` -> `/Users/renaudg/code/Amstrad_MiSTer-accuracy` (Branch prefix: `accuracy/`)
- `plus`     -> `/Users/renaudg/code/Amstrad_MiSTer-plus`     (Branch prefix: `plus/`)
- Main       -> `/Users/renaudg/code/Amstrad_MiSTer`          (Branch: `accc-review-and-fixes`)

## Procedure

1. **Resolve Topic**:
   - If provided by user: format branch name as `<stream>/<topic>` (strip duplicate `<stream>/` if already prefixed).
   - If omitted: inspect the stream's next todo in [`docs/current-status.md`](file:///Users/renaudg/code/Amstrad_MiSTer/docs/current-status.md) / [`docs/implementation-roadmap.md`](file:///Users/renaudg/code/Amstrad_MiSTer/docs/implementation-roadmap.md) and generate `<stream>/<topic>`.

2. **Pre-flight Check**:
   - Check `git status --porcelain` in the main worktree (`/Users/renaudg/code/Amstrad_MiSTer`).
   - Check `git status --porcelain` in the stream worktree (`/Users/renaudg/code/Amstrad_MiSTer-<stream>`).
   - If either worktree is dirty with uncommitted changes, pause and report them before touching branches. Ignored/private local assets are not dirt.
   - Inspect `git stash list` for named WIP from an earlier session. Report any stash whose
     message or paths overlap the selected stream before resetting its branch. Never apply a
     stash automatically: a stash may span streams or shared files, so recover only the
     explicitly owned paths after branch preparation and keep the original stash until the
     recovered work is committed and verified.
   - Fetch origin. Fast-forward a behind local `accc-review-and-fixes`; stop if local and
     remote diverged.
   - Inspect the stream worktree's current branch and the requested target branch. If either
     contains commits not yet ancestral to `accc-review-and-fixes`, or the target is checked
     out in another worktree, do not reset it. Resume/finish that ownership or choose a new
     topic with the user/coordinator.
   - Record the exact `accc-review-and-fixes` SHA. If a peer task was supplied, confirm the
     peer owns the other stream and record any shared-file ownership assigned by the
     coordinator.
   - If `starting-base` was supplied, require it to be an ancestor of the refreshed
     `accc-review-and-fixes` tip. Stop on unrelated history.

3. **Branch Preparation in Stream Worktree**:
   - Cut/reset the new feature branch starting from the latest `accc-review-and-fixes` tip:
     ```sh
     git -C /Users/renaudg/code/Amstrad_MiSTer-<stream> checkout -B <stream>/<topic> accc-review-and-fixes
     ```
   - **Simulation rule**: Do **NOT** run `make -C sim` baseline at start. The `accc-review-and-fixes` tip is already validated.

4. **Handoff**:
   - Report the current commit SHA, branch name, and active workspace path to the user so work can proceed.
   - In a coordinated run, report a **STARTED** message to the coordinator containing the
     branch, canonical `starting-base` SHA, worktree, scope, and anticipated shared files.
     If direct peer messaging was explicitly confirmed, send the peer a copy. Do not edit
     an unassigned shared file until the coordinator relays one writer. Shared areas include
     top-level/common peripheral RTL, root build/CI files, `AGENTS.md`, `CLAUDE.md`,
     `docs/current-status.md`, `docs/implementation-roadmap.md`, and
     `docs/review-debt.md`.
   - A coordinated task stops at a committed, reviewed, gated **READY** handoff. It does not
     invoke `$stream-finish` until the coordinator grants the integration lease. With no
     peer/coordinator, the ordinary single-stream lifecycle is unchanged.
