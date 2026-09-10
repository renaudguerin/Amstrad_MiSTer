# Ad-hoc task worktrees

Accuracy and Plus remain separate behavior streams, not permanent directories. General work
covers shared infrastructure, peripherals, documentation and tools. Tasks use short-lived,
environment-owned worktrees and integrate into `master` unless instructed
otherwise. Existing fixed worktrees remain valid unfinished-work locations; never delete or
reset them merely because the topology is deprecated.

## Manual commands

- `$stream-start [accuracy|plus|general|auto] [brief]`: select actionable work, assess overlap,
  prepare/resume a named branch, provision references, then start. Omitted scope means auto;
  a free-text brief is classified rather than mistaken for an invalid stream.
- `$stream-orchestrate ...`: launch compatible tasks or adopt existing ones and add more.
  Example: `add two compatible tasks alongside this one`. Existing branches and owners stay
  intact. Repeated start in one conversation resumes one task; it does not create parallelism.
- `$stream-finish [source] [--no-push]`: merge locally, reconcile shared docs, validate, and
  push by default. An explicit invocation authorizes that push without another confirmation.
  Ordinary discussion/editing of these skills does not invoke them.

Use the host's skill invocation syntax. Canonical skills live in `.agents/skills/`;
`.claude/skills` is a relative symlink to that source, not a second copy.

## Workspace and scope ownership

Discover paths with `git worktree list --porcelain` and host task metadata. Adopt the task's
current isolated checkout; do not create a second checkout merely because it has a generated
name. Keep one writer per checkout. Host-managed task creation/cleanup owns its worktrees.
For a plain CLI where Git worktrees are supported and accessible, create a unique worktree
with `git worktree add -b <branch> <new-path> <base-sha>` and continue in that checkout.
Changing shell directory does not change a managed task's sandbox or app workspace.

For Codex new conversations, use `create_thread` with project environment `type: worktree`
when the user requested a new task. For the current conversation, adopt its worktree; if it
is still in shared integration, use the app's move-to-worktree UI. The loaded `handoff_thread`
tool cannot move its own calling task. Start can finish read-only selection before that app
step; it must not silently create a new conversation or an orphan checkout. See the
[Codex adapter](../.agents/skills/stream-orchestrate/references/codex.md).

A short ignored `.stream-task.md` in each task checkout records task ID, scope/brief, branch,
base SHA, expected interfaces/modules, dependencies, gates and reference status. It helps
other tasks discover intentions; it is not a lock, global registry, or guarantee of visibility.
The agent compares actual work and interfaces before launching overlapping tasks. Tolerate
ordinary merge conflicts; coordinate incompatible redesigns before implementation. The user
is not responsible for knowing which modules collide. Branch names do not prove inactivity.

Default branch names are `accuracy/<topic>`, `plus/<topic>`, and `general/<topic>`; use the
host's required prefix where applicable (`codex/<scope>/<topic>`). Preserve an explicitly
requested name. Never reset an existing branch to implement this naming convention.

## Reference provisioning belongs to stream-start

Tracked references arrive through Git. During start, copy any missing ACCC PDFs from an
existing checkout into the assigned worktree's `docs/references/` directory using ordinary
`mkdir -p` and `cp`. The two filenames are `ACCC1.11-FR.pdf` and `ACCC1.11-EN.pdf`.
Leave existing copies alone. Use real copies so sandboxed workers can read them without
access to another checkout. The originals remain ignored and must not be committed.

This is routine local setup: no copy-time hash checks, manifest, custom helper or dedicated
test suite. If a copy fails, report the error and retry or recopy when the source is available.
Missing references block only work needing those documents. Existing extraction provenance
and hardware-evidence requirements remain separate from preparing a worktree.

Bridge delegation is performed by the task's main chat using that same prepared checkout as
`-C`. Workers do not create another clone/worktree or repeat setup. Copy other private
reference files only when the task needs them.

## Optional environment setup

The root `.worktreeinclude` lists the two PDFs for Claude-native copying where supported.
Other hosts can do the same simple copy during environment setup; this is optional because
`stream-start` fills missing files. No Codex setup configuration or OpenCode hook is assumed
installed by this repository. Automatic setup must not select roadmap work or change branches
just because a conversation opened.

## Finish and cleanup

Use one integration task at a time. Refresh the destination, reconcile conflicts and shared
documents, and run relevant gates on the merged result. No lease files or bespoke queue are
needed. Use a task with destination access; sandboxed children deliver a READY handoff.
`stream-finish` retains exact-commit CI checks, synthesis classification/ancestor-RBF reuse,
local artifact delivery, and the distinction between simulation and hardware confirmation.

Successful integration does not automatically authorize destructive cleanup. Prefer the
owning environment's cleanup action. Before removal, verify commits are integrated or
otherwise deliberately retained, inspect tracked/untracked/ignored files and in-progress
operations, and preserve unique private evidence or outputs. Copied references are reproducible
while the originals remain in a retained checkout; never delete the sole originals.
A READY or abandoned task can still contain valuable work. Do not force-remove worktrees or
prune branches/stashes simply because no task appears active. Report cleanup left to the host.

## Verification boundary

Skill schema/link checks and scenario walkthroughs do not prove host operations. Actual task
creation, adoption and cleanup remain untested in this rewrite; verify capabilities against
loaded tools when using a host. The historical
[revisit note](stream-orchestration-revisit-2026-09-07.md) records the rejected fixed-topology
port; it is not the current operating procedure.
