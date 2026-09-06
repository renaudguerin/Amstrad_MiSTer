# OpenCode Harness Adapter

Use this adapter when running inside **OpenCode** (`task` subagent tool, child sessions).

---

## 1. Pre-flight & Permissions

1. **`external_directory` Permission**:
   - The active project root in OpenCode is `/Users/renaudg/code/Amstrad_MiSTer`.
   - Sibling worktrees (`/Users/renaudg/code/Amstrad_MiSTer-accuracy` and `/Users/renaudg/code/Amstrad_MiSTer-plus`) sit outside this directory.
   - In OpenCode, cross-directory operations are governed by the `external_directory` permission. Ensure `external_directory: allow` is configured (or approved upon prompt) so child subagents can read and write within the sibling worktrees without blocking headless runs.
2. **Detect Active Tasks**:
   - Inspect `git worktree list --porcelain` and stream worktree branch states before launching.
   - Do not launch a duplicate writer on an active stream branch.

---

## 2. Task Spawning via `task` Tool

1. **Subagent Invocation**:
   - Spawn stream tasks using OpenCode's `task` tool (using the general worker agent or `@builder` / `@builder-spark` per project roster).
   - If the model supports concurrent tool calls, dispatch both `task` calls in parallel in a single turn.
   - Explicitly pin the subagent to its target worktree via the `workdir` parameter or command flags:
     - Accuracy: `/Users/renaudg/code/Amstrad_MiSTer-accuracy`
     - Plus: `/Users/renaudg/code/Amstrad_MiSTer-plus`
2. **Task Prompt Composition**:
   The prompt passed to each subagent must include:
   - Complete stream-specific brief and acceptance gates;
   - Fixed worktree path and prohibition on touching the other stream;
   - Instruction to invoke `$stream-start <stream> [topic]` with starting base SHA;
   - Initial shared-file ownership split;
   - Prohibition on nested `task` delegation (`subagent_depth: 0`);
   - Requirement to execute the writability handshake before real work;
   - Requirement to stop at **READY** (do not push or run `$stream-finish`).

---

## 3. Writability Handshake

Before touching code or branches, each subagent must verify `external_directory` write permissions and git metadata integrity:

1. Confirm the stream worktree is clean (`git status --porcelain`).
2. Create a temporary marker `.opencode-stream-write-probe-<run-id>`.
3. Run `git add -N`, unstage with `git reset HEAD`, and delete the marker.
4. Verify the worktree remains completely clean.

If permissions block creation or unstaging, the subagent halts immediately and reports **BLOCKED-WRITABILITY**.

---

## 4. Supervision & Coordination

1. **Subagent Execution Model**:
   - OpenCode subagents run in child sessions linked to the parent coordinator.
   - Child sessions are navigable in the OpenCode TUI/UI using `session_child_first`, `session_child_cycle`, and `session_child_parent`.
   - Subagents report milestones (`WRITABLE`, `STARTED`, `DISCOVERY`, `READY`) directly in their output upon turn completion.
2. **Session Resumption & Discovery Relay**:
   - If a stream subagent reports a cross-stream `DISCOVERY` or question, the coordinator relays the updated context or decision by resuming that subagent's session via `task_id` (or calling `task` on the peer).
3. **Serialized Integration Lease**:
   - Once the first stream task returns **READY**:
     - Coordinator acquires the exclusive lease against the current integration SHA and runs `$stream-finish <stream>`.
     - After CI verifies the first integration, coordinator resumes the second stream task via `task(task_id: ...)` with the new integration SHA.
     - The second task rebases onto the new base, re-verifies its gates, and reports **READY**.
     - Coordinator acquires the lease for stream two and completes `$stream-finish`.
