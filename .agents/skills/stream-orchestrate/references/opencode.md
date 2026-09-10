# OpenCode

Manual `stream-start` and `stream-finish` use the common workflow in the current assigned
checkout. The `.claude/skills` compatibility path exposes the same skill source to hosts that
support Claude-compatible project skill discovery; verify discovery in the installed version.

Automatic orchestration is capability-gated and unverified. Inspect the available session
creation, workspace selection, follow-up and visibility tools before launch. Require a
user-visible steerable session bound to its own checkout. Do not assume the `task` tool has
`workdir` or worktree-creation parameters, or that a navigable child is independently steerable.
If the available tools cannot meet this contract, provide launch briefs for user-created
sessions and keep coordination manual. Do not widen `external_directory` permissions globally.

Adopt an existing session/check-out when adding work; never repurpose its branch. Provision
references during `stream-start` with an ordinary copy of missing files. There is no mandatory automatic
hook. Integrate one READY branch at a time from a checkout the session can access.

The former fixed-sibling adapter was never proven. These capability requirements deliberately
replace its speculative tool calls; do not claim OpenCode end-to-end support without a smoke.
