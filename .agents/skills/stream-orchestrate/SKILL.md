---
name: stream-orchestrate
description: Creates and supervises parallel tasks for the accuracy and Plus streams across supported harnesses (Codex Desktop, Antigravity, Claude Code, OpenCode), coordinates shared-file ownership and discoveries, and serializes stream-finish integration. Use when the user explicitly asks one coordinator task to open or manage separate stream conversations; do not use for ordinary same-task subagents.
---

# stream-orchestrate

Turn one explicit user request into coordinated stream tasks for the dedicated accuracy and
Plus worktrees. The coordinator owns decomposition, cross-task messaging, integration order,
and final acceptance. Stream tasks own implementation only in their assigned worktree.

This skill may create tasks/subagents because invoking it or explicitly asking to open stream
conversations is authorization to do so. Do not create tasks from a merely related coding
request, and do not use separate tasks for routine same-task delegation.

## Inputs

- **`accuracy`**: accuracy-stream brief, or omitted.
- **`plus`**: Plus-stream brief, or omitted.
- **`integration-order`**: `auto` by default, otherwise `accuracy-first` or `plus-first`.
- **`finish`**: `auto` only when the user explicitly asks to finish, integrate, merge, or
  push unattended. This authorization belongs to the coordinator that received it directly;
  child tasks are not expected to inherit or reinterpret it. Otherwise default to
  `prepare-only` and stop at READY.

At least one stream brief is required. Preserve the user's wording and acceptance criteria;
do not move work between streams merely to balance workload.

## Fixed Topology

- Main project: `/Users/renaudg/code/Amstrad_MiSTer`, branch `accc-review-and-fixes`.
- Accuracy task: `/Users/renaudg/code/Amstrad_MiSTer-accuracy`, `accuracy/*`.
- Plus task: `/Users/renaudg/code/Amstrad_MiSTer-plus`, `plus/*`.

Every stream command must run with the fixed worktree as `workdir`/`Cwd` or through `git -C`.
**Do not request or generate ephemeral/isolated worktrees**: doing so would bypass this
repository's stream partition.

## Harness Dispatch

The core coordination invariants below apply across all hosts. Before spawning stream tasks,
inspect your available tools and read the matching harness adapter:

| Available Tool Primitives | Host Harness | Adapter Reference |
| :--- | :--- | :--- |
| `create_thread`, `wait_threads`, `send_message_to_thread` | OpenAI Codex Desktop | [references/codex.md](references/codex.md) |
| `invoke_subagent`, `manage_subagents`, `send_message` | Google Antigravity / AGY | [references/antigravity.md](references/antigravity.md) |
| `Agent` (background subagents only) | Claude Code | **Unsupported.** See `docs/stream-orchestration-revisit-2026-09-07.md` |
| `task` (`external_directory`) | OpenCode | [references/opencode.md](references/opencode.md) |

Only load the reference for your active host harness. If your harness is marked unsupported,
say so and stop; do not improvise a substitute mechanism.

## 1. Pre-flight and Task Creation

1. Read `AGENTS.md`, `docs/current-status.md`, and `docs/implementation-roadmap.md`. Check
   the main and requested stream worktrees for uncommitted changes or an in-progress Git
   operation. Preserve ignored/private local assets.
2. Detect active tasks claiming a requested stream using your harness adapter. Cross-check
   with `git worktree list --porcelain`, the stream worktree's current branch, and whether
   that branch is already an ancestor of the integration tip. Do not create a duplicate writer.
   Reuse an active task only when the user asked to continue it; otherwise report the ownership
   conflict before mutation. A clean branch with commits not yet integrated is unfinished
   ownership even when no task is active.
3. Record the exact starting `accc-review-and-fixes` SHA and choose the integration order.
   In `auto`, honor data dependencies first; otherwise finish accuracy before Plus so the
   Plus task can rebase onto the classic/shared baseline.
4. Before task creation, assign initial ownership for every shared area named or reasonably
   implied by the briefs. Embed that split in both initial prompts; do not defer the initial
   ownership decision until peer IDs exist.
5. Create one stream task/subagent per requested stream following your harness adapter.
   Omit model/effort overrides so each task uses the user's configured defaults. The initial
   prompt must include:
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
   - an instruction to stop at a committed, reviewed, gated **READY** handoff. The child
     must not push, dispatch CI, or invoke `$stream-finish`; the coordinator owns those
     actions when `finish: auto` is directly authorized.
6. Once task/subagent IDs are resolved, send each task the peer task ID, title, and confirmed
   ownership split. Include the coordinator task ID when available. State plainly when no peer
   exists. Tasks buffer early milestones until this follow-up. Report the created task links/IDs
   to the user.

### Writability Handshake

Tasks created against the main workspace may need managed approval to write a sibling stream
worktree. Before `$stream-start` or any real edit, each child must prove both checkout and
shared Git-metadata writes in its exact assigned path:

1. Confirm the stream worktree is clean (`git status --porcelain`).
2. Add a uniquely named `.stream-write-probe-<run-id>` marker in that worktree.
3. Run `git add -N` for that marker, then unstage it with a path-scoped reset (`git reset HEAD`).
4. Delete the marker and verify the worktree is clean again.

If any step is denied or leaves drift, the child reports **BLOCKED-WRITABILITY**, names the
exact leftover path or Git state, and stops without changing branches. The coordinator does
not treat task creation as successful until every child passes this handshake.

## 2. Shared-file and Discovery Protocol

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

The coordinator relays milestone and ownership messages between tasks by default. The receiver
acknowledges the ownership decision or raises a concrete conflict. A task may continue disjoint
work while waiting, but must not guess ownership. Keep relayed messages concise, human-readable,
and useful in visible task histories.

Each task reports these milestones when applicable:

1. **WRITABLE** — the reversible checkout/Git-metadata probe passed.
2. **STARTED** — branch, base SHA, worktree, scope, anticipated shared files.
3. **DISCOVERY** — cross-stream evidence or ownership change.
4. **READY** — tip SHA, changed shared files, gates, review/debt state, residuals.
5. **INTEGRATED** — integration SHA, CI run/jobs, artifact, and remaining hardware gates.

## 3. Supervision

Supervise tasks according to your harness adapter (bounded waits or reactive message wakeups).
Read a task when it completes, needs attention, or reports a coordination milestone. Resolve
scope/ownership questions from the user's original request; do not broaden authority.

If one task fails, preserve the other task's valid work. Retry or redirect only the failed
task. Do not let a provider/reviewer failure silently weaken repository review policy: obtain
another valid review or record review debt as the repository requires.

### Direct-Authorization Ownership

Keep external mutations in the coordinator that received the user's authorization. Do not
relay phrases such as "the user authorized push" and ask a child task to treat them as direct
permission: task-local approval layers may correctly refuse inherited authority, creating an
unnecessary dead end.

When `finish: auto` is authorized, the coordinator may push an exact committed stream tip for
a pre-merge build, dispatch CI for that tip, and perform serialized integration under the
lease protocol below. Verify the local tip and remote ref exactly before dispatch. The child
continues to own implementation and fixes; after a coordinator-run build or review, send it
the evidence or blocking findings and let it produce a new READY tip. If the tip changes,
the coordinator repeats the exact-ref push/build as needed.

If the coordinator itself lacks direct authorization or its host requires user approval,
surface that prompt once. Do not bounce the action through a child, broaden permission, or
interrupt valid running work merely to reduce elapsed time.

External review or analysis of a stream tip follows the host's own delegation routing. This
skill adds no approval step of its own to it. Two constraints do belong here: do not start a
second reviewer on a tip that one is already reviewing, and an external worker permitted to
write needs an explicit file boundary and a confirmed single-writer window in the stream
worktree.

## 4. Serialized Integration Lease

Only one task may mutate or push `accc-review-and-fixes` at a time.
Execute this section only when `finish: auto` was explicitly authorized by the user.
Otherwise `finish` is `prepare-only`: stop at READY, report the stream tips, and do not grant
an integration lease.

1. Wait until the selected first task reports READY. Fetch the current integration ref and
   verify that the task's original base is its ancestor. Acquire the coordinator's exclusive
   integration lease against the **current** exact integration SHA, tell the peer not to
   mutate integration, and invoke `$stream-finish <stream>` from the coordinator with that
   SHA as `integration-base`.
2. Verify the first exact integration SHA and the required CI jobs. A queued or merely
   aggregate-green run is insufficient. The coordinator records the INTEGRATED milestone.
3. Send the verified SHA and changed shared-file list to the second task. After it confirms
   its READY tip and any conflict-sensitive assumptions against that SHA, acquire a new
   coordinator lease and invoke `$stream-finish <stream>` with the verified current SHA as
   `integration-base`. The finish procedure performs the topology-preserving rebase and
   required post-rebase gates.
4. Verify the second exact integration SHA and CI jobs in the same way, then record its
   INTEGRATED milestone.
5. A non-fast-forward push rejection or a moved integration SHA means the lease is stale.
   Stop, fetch, rebase/reconcile, and rerun the affected gate; never force-push.

When only one stream task exists, the same READY -> coordinator lease -> coordinator-run
`$stream-finish` protocol applies without peer messaging, provided `finish: auto` was
explicitly authorized.

## 5. Final Report

Report the task titles/IDs, stream tip SHAs, serialized integration SHA(s), exact CI job
results, artifacts, hardware-only residuals, and any review debt. If both streams integrate,
label the first stream's RBF as an intermediate artifact and the final stream's exact-SHA RBF
as the combined hardware-test artifact. Distinguish implemented and simulated behavior from
hardware-confirmed behavior.

After the final exact-SHA synthesis succeeds, download its artifact, independently verify
the reported timing/resource evidence and RBF SHA-256, and copy the combined hardware-test
RBF into `/Users/renaudg/code/Amstrad_MiSTer/output_files/`. Preserve the artifact's
SHA-bearing filename, confirm the copied file has the same hash, and give the user a clickable
local path. Do not consider the hardware-build handoff complete while the final RBF exists
only in GitHub Actions or a temporary download directory.
