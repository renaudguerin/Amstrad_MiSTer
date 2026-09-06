# Codex Desktop Harness Adapter

Use this adapter when running inside **OpenAI Codex Desktop** (`create_thread`, `list_threads`, `send_message_to_thread`, `wait_threads`).

---

## 1. Pre-flight & Task Creation

1. **Resolve Saved Project**:
   - Call `list_projects` to resolve the saved project matching `/Users/renaudg/code/Amstrad_MiSTer`.
2. **Detect Active Owners**:
   - Call `list_threads` and, when needed, `read_thread` to detect an active task claiming a requested stream.
   - Cross-check with `git worktree list --porcelain`, the stream worktree's current branch, and whether that branch is already an ancestor of the integration tip.
   - Do not create a duplicate writer. Reuse an active task only when the user asked to continue it; otherwise report the ownership conflict before mutation.
3. **Spawn Stream Threads**:
   - Create one Codex Desktop task per requested stream using:
     ```json
     {
       "type": "project",
       "projectId": "<resolved-project-id>",
       "environment": {
         "type": "local"
       }
     }
     ```
   - Omit model/effort overrides so each task uses the user's configured defaults.
   - Tell each task to run every stream command with the fixed worktree as `workdir` or through `git -C`. **Do not request a generated Codex worktree**: it would bypass this repository's stream partition.
   - Initial prompt must include the brief, fixed worktree path, starting integration SHA, shared ownership split, prohibition on nested delegation, the writability probe, and the requirement to stop at READY.
4. **Resolve Task IDs & Exchange Peer Info**:
   - Task creation is asynchronous. Resolve a real task ID before messaging it; never pass a provisional client ID to task tools.
   - After all task IDs are known, call `send_message_to_thread` with the peer task ID, title, confirmed ownership split, and coordinator task ID. Report created task links/IDs to the user.

---

## 2. Writability Handshake

Tasks created against the main saved project may need managed approval to write a sibling stream worktree. Before `$stream-start` or any real edit, each child must prove both checkout and shared Git-metadata writes in its exact assigned path:

1. Confirm the stream worktree is clean (`git status --porcelain`).
2. Use `apply_patch` to add a uniquely named `.codex-stream-write-probe-<run-id>` marker in that worktree.
3. Run `git add -N` for that marker, then unstage it with a path-scoped reset.
4. Delete the marker with `apply_patch` and verify the worktree is clean again.

If any step is denied or leaves drift, the child reports **BLOCKED-WRITABILITY**, names the exact leftover path or Git state, and stops without changing branches. The coordinator does not proceed until every child passes this handshake.

---

## 3. Supervision & Messaging

1. **Multi-Task Wait**:
   - Supervise stream tasks using `wait_threads` with bounded timeouts for the requested task IDs rather than frequent status polling.
   - Call `read_thread` when a task completes, needs attention, or reports a coordination milestone (`WRITABLE`, `STARTED`, `DISCOVERY`, `READY`, `INTEGRATED`).
2. **Relayed Messaging**:
   - Use `send_message_to_thread` to relay cross-stream discovery or ownership changes between child threads.
   - When a child reports `READY`, proceed with the serialized integration lease protocol described in the main skill specification.
