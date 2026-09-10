---
name: stream-start
description: Explicitly start or resume a repository task in an ad-hoc worktree. Select roadmap/backlog work, assess overlap, prepare a named branch, and provision references. Supports accuracy, plus, general, or auto scope.
---

# stream-start

This is the manual task-start shortcut. Ordinary project conversations and automatic worktree
setup do not select implementation work. Read [the common workflow](../../../docs/task-workflow.md)
for host routing, reference provisioning, and cleanup.

## Inputs

- **Scope**: `accuracy`, `plus`, `general`, or `auto` (default).
- **Topic/brief**: optional slug or natural-language request. If the first argument is not a
  scope keyword, interpret the whole argument as the brief and infer scope.
- **starting-base**: optional requested integration SHA. An explicitly requested dependency
  branch may instead provide the base; record that distinction.
- **coordinator**: optional task ID and coordination brief.

Examples: `$stream-start accuracy`, `$stream-start general improve-ci-diagnostics`,
`$stream-start investigate disk loading`, or `$stream-start`.
`general` covers shared infrastructure, peripherals, docs and tooling; it does not permit
combining accuracy and Plus behavior in one change. `auto` classifies a supplied brief, or
selects suitable unclaimed work across the roadmap and backlog when no brief is supplied.

## Procedure

1. Read `AGENTS.md`, `CLAUDE.md`, backlog, current status and relevant roadmap sections.
   Auto-select an actionable item with available prerequisites and acceptance evidence;
   state the choice and rationale. Preserve the user's explicit brief.
2. Inspect the current checkout, `git worktree list --porcelain`, visible tasks, relevant
   branch diffs and `.stream-task.md` briefs. Inspect likely code seams before judging
   overlap. Avoid duplicate work; agree shared interfaces or sequence incompatible redesigns.
   Ordinary textual overlap need not block work. The agent owns this assessment; do not ask
   the user to identify conflicting modules. Report gaps in cross-harness visibility.
3. If this task already owns the same brief and branch, resume and preserve its base/edits.
   A different brief must not repurpose unfinished work. To add parallel work, use
   `stream-orchestrate` to adopt the existing task and launch the addition.
4. Adopt the environment's task-owned checkout, or follow the common workflow's host routing.
   Check its status and any merge/rebase operation before switching branches. Preserve dirty
   work and existing commits. Unrelated dirt elsewhere is not a global blocker. Inspect
   named stashes for relevant WIP and report overlaps; never auto-apply or drop a stash.
5. Fetch origin; compare local and remote `master`. Select the descendant tip
   when one is ahead; stop branch preparation on divergence. Do not mutate another task's
   integration checkout just to start work. Record the exact selected SHA. If `starting-base`
   was supplied, require it to be an ancestor of this base, unless the user requested a
   different dependency base.
6. In a fresh clean checkout, create a unique branch with
   `git switch -c <branch> <base-sha>`. Use `accuracy/<topic>`, `plus/<topic>`, or
   `general/<topic>`; follow a host-required prefix (Codex: `codex/<scope>/<topic>`).
   Check branch/worktree ownership first. Never use `checkout -B` or reset an existing
   branch. Preserve host-carried commits. The environment's default base may differ from
   the intended integration base; verify it before starting implementation.
7. **Provision references here** by copying missing PDFs as described in the common workflow.
   Subsequent bridge workers use this same prepared checkout, with no extra provisioning
   stage, worktree, or clone. Missing inputs block evidence-dependent work; unrelated work
   can continue with the gap reported. Do not run baseline simulation at start.
8. Write/update an ignored `.stream-task.md`: task/coordinator IDs when available, scope,
   brief, branch, actual base SHA, anticipated modules/interfaces, dependencies, acceptance
   gates and reference status. This is discoverable context, not a lock or inactivity proof.
   Report **STARTED** with the selected task, branch, base, checkout and overlap decisions,
   then continue the requested work. Coordinated tasks stop at committed, reviewed/gated
   **READY** with residuals; their integrator owns finish. Start alone does not authorize
   publication or create an extra user-visible conversation.
