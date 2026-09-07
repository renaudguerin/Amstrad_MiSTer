# Codex Desktop

Use loaded task tools, not guessed API fields. `list_projects` resolves the saved Git project;
`list_threads`/targeted `read_thread` identify existing task owners. Cross-check actual Git
worktrees, branch diffs and task briefs. Task titles are not authoritative scope records.

## New coordinated tasks

When the user requests new tasks, `create_thread` accepts a project target with
`environment: {"type": "worktree"}`. Use the returned project ID, not a hardcoded path.
Only set `startingState` when the user explicitly requested a particular Git starting state,
as required by the tool schema. Otherwise omit it, pass the intended integration SHA in the
initial brief, and have `stream-start` verify and prepare the fresh branch in the resulting
checkout before editing. Never assume the project's default branch equals integration.

Omit model/effort overrides unless requested. Creation may return a provisional
`clientThreadId`: do not pass that to task tools requiring a real `threadId`. Resolve setup
completion using the available task listing/status tools, then record actual checkout and
base from STARTED. Include the app's created-task directive in the final response.

Use bounded `wait_threads` calls and returned cursors; inspect full history only when needed.
Relay discoveries with `send_message_to_thread`, and retain direct publication authorization
in the coordinator. Adding tasks adopts the selected existing task without recreating it.

## Starting in the current conversation

An existing Codex-managed worktree is the task checkout; do not create a nested replacement.
If the current conversation runs directly in the shared integration checkout, prefer the
app's current-task worktree/handoff UI. With the currently exposed task tools,
`handoff_thread` can move another task but cannot move the calling task itself.
`stream-start` alone does not authorize `create_thread`, which creates a separate visible
conversation. Finish read-only task selection/preflight, then give the user the one required
app action to move this task to a worktree and resume. Do not create an orphan checkout,
pretend `git -C` moves the task's sandbox, or spawn a helper task to evade this boundary.
If the user explicitly requests a new task instead, use `create_thread` as above.

An explicitly user-assigned isolated checkout may be used directly. For sandboxed integration,
use a task that already has access to the destination; do not assume sibling writes work.

## Optional setup

Codex local-environment setup scripts can run at worktree creation. They may copy missing
reference PDFs from an accessible source checkout into the assigned worktree. They must not
select a roadmap item or switch branches. `stream-start` remains responsible for checking
and filling missing references, so correctness does not depend on a configured hook.

Source: [local environments](https://learn.chatgpt.com/docs/environments/local-environment).
The task API rules above come from loaded Codex Desktop tools; recheck their schema when it
changes. No new-task dispatch or current-task move was exercised by this workflow rewrite.
