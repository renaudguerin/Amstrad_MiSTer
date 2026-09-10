# Claude Code Desktop

Use the session's loaded capabilities. The recorded Desktop `spawn_task` mechanism creates
a steerable conversation and its own worktree: accept that checkout and run `stream-start`
there. Read the current tool schema for base/project arguments; do not fabricate parameters
or redirect the child to an old sibling checkout. Report its actual path/branch/base.

A background `Agent` worker is not a substitute for a separately steerable conversation.
If `spawn_task` or an equivalent conversation mechanism is unavailable, provide the prepared
brief for a user-created session; manual start/finish still work. Do not label automatic
orchestration supported just because a background subagent tool exists.

Discover messaging/wait capabilities rather than assuming `SendMessage` persists. When
messaging is absent, use a completed READY handoff and the user-visible session mechanism;
do not claim to supervise or steer a running task you cannot reach. Adopt existing sessions
without recreating them when extending a run.

Project skills are exposed through `.claude/skills -> ../.agents/skills`. The checked-in
`.worktreeinclude` lists the two ignored ACCC PDFs for native copying where supported.
`stream-start` copies any missing PDFs from an existing checkout; this
also covers hosts that do not honor `.worktreeinclude`. A `WorktreeCreate` hook replaces Git
worktree creation, so do not introduce one merely to copy PDFs.

An isolated session may not redirect Git into the main checkout. Run finish in an integration
session with access to that checkout, or hand off the exact READY branch/tip. No sandbox bypass.

Sources: [worktrees](https://code.claude.com/docs/en/worktrees),
[hooks](https://code.claude.com/docs/en/hooks), and the dated repository
[Desktop investigation](../../../../docs/stream-orchestration-revisit-2026-09-07.md).
The generated-worktree prohibition is removed; Desktop dispatch, copying and messaging still
need a live smoke test. Documented CLI capabilities are not proof of Desktop behavior.
