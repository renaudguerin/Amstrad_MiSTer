# Claude Code Harness Adapter

Use this adapter when running inside **Claude Code** (`Agent` tool or bridge CLI).

---

## 1. Pre-flight & Task Creation

1. **Detect Active Tasks**:
   - Inspect active sessions or `git worktree list --porcelain` and stream worktree branch states before launching.
   - Do not launch a duplicate writer on an active stream branch.
2. **Spawn Stream Subagents**:
   - When using native Claude subagents (`Agent` tool):
     - Launch tasks with the stream-specific prompt.
     - Specify the fixed worktree path explicitly (`/Users/renaudg/code/Amstrad_MiSTer-<stream>`).
     - Subagents must run commands using `cd <worktree>` or `git -C <worktree>`.
   - When using CLI bridges (`~/.agents/bin/ask-*`):
     - Pass the `-C /Users/renaudg/code/Amstrad_MiSTer-<stream>` parameter to target the specific worktree.
     - Pass `-w` to permit modifications in that worktree.
     - Never run nested subagents within the worker session.
   - Initial prompt must include the brief, fixed worktree path, starting integration SHA, shared ownership split, prohibition on nested delegation, writability probe, and requirement to stop at READY.

---

## 2. Writability Handshake

1. Verify the stream worktree is clean (`git -C <worktree> status --porcelain`).
2. Create marker `.claude-stream-write-probe-<run-id>`.
3. Run `git add -N`, unstage with `git reset HEAD`, and remove the marker.
4. Verify the worktree remains clean before continuing.

---

## 3. Supervision & Integration

1. **Supervision**:
   - Collect milestone reports from the subagent execution logs or return outputs.
   - Relay any cross-stream discovery or shared-file ownership assignments.
2. **Serialized Integration**:
   - When a stream reaches `READY`, the coordinator invokes `$stream-finish <stream>`.
   - Rebase and gating checks must complete before the second stream acquires the integration lease.
