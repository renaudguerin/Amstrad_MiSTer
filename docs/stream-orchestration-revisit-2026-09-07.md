# Stream orchestration: revisit the design before porting it further

**Status (2026-09-07): the Claude Code port of `$stream-orchestrate` is abandoned on purpose.
The skill remains supported on Codex Desktop, where it was written and is the only harness on
which it has actually run. Antigravity and OpenCode adapters exist but are unproven.**

This document exists so a future session can pick the question up without re-deriving what was
already established, and without repeating an attempt that was correctly stopped.

## The decision, and why

The skill's purpose is two **separately steerable conversations**, one per stream worktree, that
the user can read and redirect independently while a coordinator serializes their integration.
That is what Codex Desktop tasks provide.

Claude Code has two candidate mechanisms and neither delivers it:

- **`Agent` subagents** run in the background. The user sees progress and a final report but
  cannot converse with one, steer it mid-task, or read its reasoning as a thread. Porting onto
  subagents produces a coordinator driving two opaque workers — which is ordinary delegation,
  already available without any of this skill's machinery. The peer-relay protocol, the
  ownership handshake, and the visible task histories all lose their reason to exist.
- **`spawn_task`** does create a real, user-visible, steerable session, which is the right
  shape. It provisions a **fresh worktree** under the `cwd` it is given. The current design
  forbids that outright: three fixed worktrees partition the repository and the skill's
  prohibition on generated worktrees is what keeps one writer per stream.

Pointing `spawn_task` at the main project and having the session drive a sibling worktree
through `git -C` was considered and rejected in review: it evades the prohibition rather than
satisfying it, and leaves an unowned worktree behind.

## What a future session should actually reconsider

The user's stated direction is to **accept ad-hoc worktrees** rather than the fixed three. That
single change unblocks `spawn_task` and invalidates enough of the current design that the skill
should be redesigned, not patched. Open questions, in the order they matter:

1. If worktrees are created per task, what replaces the fixed partition as the single-writer
   guarantee? Branch ownership, a lease file, or something the harness enforces?
2. Does the accuracy/Plus split still need to be a *topology* at all, or is it just two branch
   prefixes and a review policy? The two streams share `rtl/CRTC.v` state and shared docs
   regardless of where they are checked out.
3. What is the cleanup contract for an abandoned ad-hoc worktree, and who runs it?
4. Is a coordinator still wanted, or is serialized integration better expressed as a queue the
   user drives by hand? The lease protocol is the most intricate part of the current skill and
   has been exercised least.
5. Should the multi-harness adapter structure survive? It costs four files to maintain and only
   one has ever run.

## Facts already established, do not re-derive

- **Claude Code does not discover this repository's skills.** They live in `.agents/skills/`;
  Claude Code reads project skills from `.claude/skills/`. There is no `.claude/` directory
  here, and `ListSkills` filtered on the three stream skills returns empty. A symlink
  (`.claude/skills -> ../.agents/skills`) would fix discovery; it was deliberately **not**
  created, because discovery is worthless while the mechanism above is unsuitable. Codex scans
  `.agents/skills` and follows symlinks, so adding one later is safe for it.
- **`SendMessage` is not guaranteed present** in a Claude Code session. It was available at the
  start of the 2026-09-07 session and gone later in the same session. Any design that relies on
  messaging a running child must detect the capability rather than assume it.
- **`Agent` accepts `isolation: "worktree"`**, which generates its own worktree. Under the
  current fixed topology that flag must never be passed.
- The `-w` flag on the `ask-claude`, `ask-gemini` and `ask-opencode` wrappers has no permission
  effect. Recorded in `docs/bridge-bug-backlog.md` in the `agents-roster` repository; the stream
  skills no longer reference those wrappers at all.

## What was changed and kept on 2026-09-07

The port was reverted, but three unrelated corrections found during it were kept:

- The skills no longer name any specific delegation system, so one can be retired without
  editing them. The external-export paragraph in `$stream-orchestrate` section 3 was rewritten
  on the user's instruction: it previously required that the user directly ask the coordinator
  to use a bridge provider before any cross-provider export. That gate is gone. Delegation
  routing is the host's business, the user does not want an extra approval hurdle for shipping
  code to third-party providers, and the old wording named a mechanism that may be retired.
  Only the duplicate-reviewer and single-writer constraints remain, because those are this
  skill's own. This is the one edit in this change set that alters behaviour for the Codex
  path.
- `peer-task` in `$stream-start` and `$stream-finish` is described as a harness task/session ID
  rather than specifically a Codex task ID.
- The OpenCode adapter no longer hardcodes `@builder` / `@builder-spark`; `builder-spark` had
  already been removed from the delegation roster.
