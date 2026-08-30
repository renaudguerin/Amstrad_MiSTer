---
name: stream-orchestrate
description: Creates and supervises parallel Codex Desktop tasks for the accuracy and Plus streams, coordinates shared-file ownership and discoveries, and serializes stream-finish integration. Use when the user explicitly asks one task to open or manage separate stream conversations; do not use for ordinary same-task subagents.
---

# stream-orchestrate

Turn one explicit user request into coordinated, user-visible Codex Desktop tasks for the
dedicated accuracy and Plus worktrees. The coordinator owns decomposition, cross-task
messaging, integration order, and final acceptance. Stream tasks own implementation only in
their assigned worktree.

This skill may create Desktop tasks because invoking it or explicitly asking to open stream
conversations is authorization to do so. Do not create tasks from a merely related coding
request, and do not use separate Desktop tasks for routine same-task delegation.

## Inputs

- **`accuracy`**: accuracy-stream brief, or omitted.
- **`plus`**: Plus-stream brief, or omitted.
- **`integration-order`**: `auto` by default, otherwise `accuracy-first` or `plus-first`.
- **`finish`**: `auto` only when the user explicitly asks to finish, integrate, merge, or
  push unattended. Otherwise default to `prepare-only` and stop at READY.

At least one stream brief is required. Preserve the user's wording and acceptance criteria;
do not move work between streams merely to balance workload.

## Fixed topology

- Main project: `/Users/renaudg/code/Amstrad_MiSTer`, branch
  `accc-review-and-fixes`.
- Accuracy task: `/Users/renaudg/code/Amstrad_MiSTer-accuracy`, `accuracy/*`.
- Plus task: `/Users/renaudg/code/Amstrad_MiSTer-plus`, `plus/*`.

Use the Codex project whose saved path is exactly the main project. Create each task with the
saved project's **local** environment, then tell it to run every stream command with the
fixed worktree as `workdir` or through `git -C`. Do not request a generated Codex worktree:
it would bypass this repository's stream partition.

The coordinator uses the Codex Desktop task capabilities directly: `list_projects` to
resolve the saved project, `list_threads` and `read_thread` to detect existing owners,
`create_thread` with
`{type: "project", projectId, environment: {type: "local"}}`,
`send_message_to_thread` for relayed peer/lease messages, and `wait_threads` for bounded
supervision.

## 1. Pre-flight and task creation

1. Read `AGENTS.md`, `docs/current-status.md`, and `docs/implementation-roadmap.md`. Check
   the main and requested stream worktrees for uncommitted changes or an in-progress Git
   operation. Preserve ignored/private local assets.
2. Use `list_threads` and, when needed, `read_thread` to detect an active task claiming a
   requested stream. Cross-check with `git worktree list --porcelain`, the stream worktree's
   current branch, and whether that branch is already an ancestor of the integration tip.
   Do not create a duplicate writer. Reuse an active task only when the user asked to
   continue it; otherwise report the ownership conflict before mutation. A clean branch with
   commits not yet integrated is unfinished ownership even when no task is active.
3. Record the exact starting `accc-review-and-fixes` SHA and choose the integration order.
   In `auto`, honor data dependencies first; otherwise finish accuracy before Plus so the
   Plus task can rebase onto the classic/shared baseline.
4. Before task creation, assign initial ownership for every shared area named or reasonably
   implied by the briefs. Embed that split in both initial prompts; do not defer the initial
   ownership decision until peer IDs exist.
5. Create one Codex Desktop task per requested stream. Omit model/effort overrides so each
   task uses the user's configured defaults. The initial prompt must include:
   - the complete stream-specific brief and acceptance gates;
   - the exact fixed worktree path and a prohibition on editing the other stream worktree;
   - an instruction to invoke `$stream-start <stream> [topic]` with the pre-flight SHA as
     `starting-base` before implementation;
   - the pre-flight integration SHA and planned integration order;
   - the initial shared-file ownership split and an instruction to hold any unassigned
     shared edit until the coordinator relays a decision;
   - a prohibition on nested delegation by child agents unless the user requested it;
   - an instruction to perform the writability handshake below before real work, and to
     report BLOCKED rather than relying on unattended approval prompts;
   - an instruction to stop at a committed, reviewed, gated **READY** handoff without
     invoking `$stream-finish` until the coordinator grants the integration lease.
6. Task creation is asynchronous. Resolve a real task ID before messaging it; never pass a
   provisional client ID to task tools.
7. After all task IDs are known, send each task the peer task ID, title, and confirmed
   ownership split. Include the coordinator task ID when it is available; otherwise the
   coordinator collects milestones through bounded waits/reads. State plainly when no peer
   exists. Tasks buffer early milestones until this follow-up and do not negotiate shared
   ownership directly before it. Report the created task links/IDs to the user.

### Writability handshake

Tasks created against the main saved project may need managed approval to write a sibling
stream worktree. Before `$stream-start` or any real edit, each child must prove both checkout
and shared Git-metadata writes in its exact assigned path:

1. Confirm the stream worktree is clean.
2. Use `apply_patch` to add a uniquely named `.codex-stream-write-probe-<run-id>` marker in
   that worktree.
3. Run `git add -N` for that marker, then unstage it with a path-scoped reset.
4. Delete the marker with `apply_patch` and verify the worktree is clean again.

Use normal scoped managed escalation when required. If any step is denied or leaves drift,
the child reports **BLOCKED-WRITABILITY**, names the exact leftover path or Git state, and
stops without changing branches. The coordinator does not treat task creation as successful
until every child passes this handshake. This fail-closed probe is required because the
saved project root and fixed stream worktrees are siblings.

## 2. Shared-file and discovery protocol

Stream-local RTL, tests, and stream-specific documents belong to their stream. Treat these
as shared unless the initial decomposition explicitly assigns one owner:

- top-level/core wiring and common peripherals, including `Amstrad.sv`,
  `rtl/Amstrad_motherboard.v`, `rtl/u765/`, and common GA/CPU/memory seams;
- root or cross-suite build/CI files;
- `AGENTS.md`, `CLAUDE.md`, `docs/current-status.md`,
  `docs/implementation-roadmap.md`, and `docs/review-debt.md`.

For a shared area, assign one writer before edits. The other task may inspect it but must not
write overlapping files. A task that discovers cross-stream evidence must promptly message
its peer with:

- the concrete finding and file/area;
- whether it changes the peer's assumptions or tests;
- the proposed owner;
- any commit SHA the peer must consume.

The coordinator relays milestone and ownership messages between tasks by default. Direct
child-to-child messaging is only an optimization after the coordinator confirms both
children expose that capability. The receiver acknowledges the ownership decision or raises
a concrete conflict. A task may continue disjoint work while waiting, but must not guess
ownership. Keep relayed messages concise, human-readable, and useful in both visible task
histories.

Each task reports these milestones when applicable:

1. **WRITABLE** — the reversible checkout/Git-metadata probe passed.
2. **STARTED** — branch, base SHA, worktree, scope, anticipated shared files.
3. **DISCOVERY** — cross-stream evidence or ownership change.
4. **READY** — tip SHA, changed shared files, gates, review/debt state, residuals.
5. **INTEGRATED** — integration SHA, CI run/jobs, artifact, and remaining hardware gates.

## 3. Supervision

Use bounded multi-task waits for the requested task IDs rather than frequent status polling.
Read a task when it completes, needs attention, or reports a coordination milestone. Resolve
scope/ownership questions from the user's original request; do not broaden authority.

If one task fails, preserve the other task's valid work. Retry or redirect only the failed
task. Do not let a provider/reviewer failure silently weaken repository review policy: obtain
another valid review or record review debt as the repository requires.

## 4. Serialized integration lease

Only one task may mutate or push `accc-review-and-fixes` at a time.
Execute this section only when `finish: auto` was explicitly authorized by the user.
Otherwise `finish` is `prepare-only`: stop at READY, report the stream tips, and do not grant
an integration lease.

1. Wait until the selected first task reports READY. Fetch the current integration ref and
   verify that the task's original base is its ancestor. Send the task an explicit
   integration lease containing the **current** exact integration SHA as `integration-base`,
   then instruct it to invoke `$stream-finish <stream>`. Tell the peer not to finish yet.
2. Wait for the first task's INTEGRATED report and verify the exact integration SHA and the
   required CI jobs. A queued or merely aggregate-green run is insufficient.
3. Send the verified SHA and changed shared-file list to the second task. Grant the second
   integration lease only after the first push/CI is settled, using that current verified
   SHA as its `integration-base`. Instruct it to rebase with merge topology preserved, rerun
   any conflict-sensitive checks, and invoke `$stream-finish` only under that lease.
4. Verify the second task's final exact SHA and CI jobs in the same way.
5. A non-fast-forward push rejection or a moved integration SHA means the lease is stale.
   Stop, fetch, rebase/reconcile, and rerun the affected gate; never force-push.

When only one stream task exists, the same READY -> lease -> `$stream-finish` protocol applies
without peer messaging, provided `finish: auto` was explicitly authorized.

## 5. Final report

Report the task titles/IDs, stream tip SHAs, serialized integration SHA(s), exact CI job
results, artifacts, hardware-only residuals, and any review debt. If both streams integrate,
label the first stream's RBF as an intermediate artifact and the final stream's exact-SHA RBF
as the combined hardware-test artifact. Distinguish implemented and simulated behavior from
hardware-confirmed behavior.
