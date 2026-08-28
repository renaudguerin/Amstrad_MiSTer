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
   - If either worktree is dirty with uncommitted changes, pause and report them before touching branches.

3. **Branch Preparation in Stream Worktree**:
   - Cut/reset the new feature branch starting from the latest `accc-review-and-fixes` tip:
     ```sh
     git -C /Users/renaudg/code/Amstrad_MiSTer-<stream> checkout -B <stream>/<topic> accc-review-and-fixes
     ```
   - **Simulation rule**: Do **NOT** run `make -C sim` baseline at start. The `accc-review-and-fixes` tip is already validated.

4. **Handoff**:
   - Report the current commit SHA, branch name, and active workspace path to the user so work can proceed.
