# Antigravity Harness Adapter

Use this adapter when running inside **Google Antigravity / AGY** (`invoke_subagent`, `manage_subagents`, `send_message`).

---

## 1. Pre-flight & Task Creation

1. **Permissions & Sibling Worktrees**:
   - The primary workspace is `/Users/renaudg/code/Amstrad_MiSTer`.
   - Because stream worktrees are sibling directories (`/Users/renaudg/code/Amstrad_MiSTer-accuracy` and `/Users/renaudg/code/Amstrad_MiSTer-plus`), ensure the environment's `Non-Workspace File Access` policy allows accessing outside the workspace root without interactive prompt stalls.
2. **Detect Active Owners**:
   - Call `manage_subagents` with `Action: "list"` to check for existing active subagents.
   - Cross-check with `git worktree list --porcelain`, the stream worktree's current branch, and whether that branch is already an ancestor of the integration tip.
   - Do not launch a duplicate writer.
3. **Spawn Stream Subagents**:
   - Spawn both streams concurrently in a single `invoke_subagent` call:
     ```json
     {
       "Subagents": [
         {
           "TypeName": "self",
           "Role": "Accuracy Stream Implementer",
           "Prompt": "<stream prompt>",
           "Workspace": "inherit"
         },
         {
           "TypeName": "self",
           "Role": "Plus Stream Implementer",
           "Prompt": "<stream prompt>",
           "Workspace": "inherit"
         }
       ]
     }
     ```
   - **Notes**:
     - `TypeName: "self"` gives the subagent full write tools (`write_to_file`, `replace_file_content`, `run_command`).
     - `Workspace: "inherit"` keeps the underlying repository context. Instruct each subagent to run all terminal commands using its fixed sibling worktree path (`Cwd: "/Users/renaudg/code/Amstrad_MiSTer-<stream>"`) and use absolute paths for file edits. **Do not use `Workspace: "branch"`**, as that creates an ephemeral clone and bypasses the repository's permanent git worktrees.
   - Initial prompt must include the stream brief, fixed worktree path, starting integration SHA, initial shared-file ownership split, prohibition on nested subagent spawning, the writability probe instructions, and the requirement to stop at READY.
4. **User Visibility & Peer Info**:
   - `invoke_subagent` returns unique conversation IDs for each spawned subagent.
   - Present clickable navigation links in chat for the user:
     ```markdown
     - Accuracy stream: [Accuracy Task](conversation://<accuracy-id>)
     - Plus stream: [Plus Task](conversation://<plus-id>)
     ```
   - Send each subagent a follow-up message with the peer's conversation ID, title, and confirmed shared-file ownership split using `send_message`.

---

## 2. Writability Handshake

Before `$stream-start` or modifying code, each child subagent must verify checkout and Git-metadata writes in its assigned sibling worktree:

1. Verify the worktree is clean: `run_command` with `git status --porcelain` in `Cwd: "/Users/renaudg/code/Amstrad_MiSTer-<stream>"`.
2. Write a temporary marker file `.antigravity-stream-write-probe-<run-id>` in the stream worktree.
3. Stage intent: `git add -N .antigravity-stream-write-probe-<run-id>`.
4. Unstage: `git reset HEAD .antigravity-stream-write-probe-<run-id>`.
5. Remove the marker file and confirm `git status --porcelain` is clean again.

If any command is blocked by permissions or fails, the subagent sends a message reporting **BLOCKED-WRITABILITY** and halts.

---

## 3. Supervision & Messaging

1. **Reactive Wakeup (No Polling)**:
   - Antigravity automatically resumes coordinator execution when a subagent sends a message or completes.
   - **Do NOT poll `manage_subagents` in a loop.** Simply stop calling tools to yield the turn and await the system notification.
2. **Relayed Messaging**:
   - Use `send_message(Recipient: "<subagent-id>", Message: "...")` to relay discovery findings or shared-file ownership determinations between stream subagents.
3. **Serialized Integration Lease**:
   - When the first subagent reports `READY`, the coordinator executes `$stream-finish` directly in its turn, verifies CI, relays the new integration SHA to the second subagent via `send_message`, and finishes the second stream once it confirms readiness.
