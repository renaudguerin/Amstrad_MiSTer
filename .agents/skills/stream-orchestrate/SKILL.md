---
name: stream-orchestrate
description: Explicitly create or extend coordinated steerable repository tasks in ad-hoc worktrees. Assess compatibility, adopt existing tasks, launch additions, relay discoveries, and integrate sequentially when requested. Not for ordinary same-task delegation.
---

# stream-orchestrate

Use when asked to launch or coordinate separate conversations. Invocation can authorize the
requested task creation; discussing this skill cannot. Read
[the common workflow](../../../docs/task-workflow.md). There is no fixed count, permanent
worktree mapping, one-task-per-stream rule, or custom integration lease service.

## Inputs and extending a run

- **Tasks**: requested accuracy, Plus, general or auto briefs. If items are omitted, choose
  suitable roadmap/backlog work within the requested count/scope. If count is unspecified,
  propose a bounded pair rather than multiplying concurrency.
- **Existing tasks / add**: adopt the current started task or named IDs, then launch additions.
  Example: `$stream-orchestrate add one compatible task to this accuracy task`.
  Another example: `$stream-orchestrate accuracy plus general: improve CI diagnostics`.
- **Finish**: prepare-only by default. Explicit finish/integrate/merge/push authorizes
  coordinator-run `stream-finish`, which pushes by default; honor an explicit no-push.

Adoption preserves checkout, branch, commits, working changes, base and owner. Do not restart
branch preparation or spawn a replacement. The current task can coordinate while retaining
its own implementation responsibility; never become a competing writer in an adopted checkout.

## Assess compatibility

Read backlog/status/roadmap, visible task and worktree briefs, relevant diffs, and the code
seams implied by the proposed work. The agent owns overlap assessment, not the user.
Text conflicts in shared docs or independent small fixes can be reconciled later. Changes
to the same counter semantics, bus handshake, memory arbitration or interface need agreement,
a common prerequisite, or sequencing. Assign a shared redesign's owner and describe the
contract other tasks may rely on. Do not require exclusive file ownership across isolated
checkouts for incidental overlap. Keep one writer per checkout.

For additions, compare against existing tasks' actual work, not just their original briefs.
Relay consequential discoveries/scope changes, leaving disjoint work running. State visibility
gaps across harnesses. Idle tasks and clean branches may still own unfinished work; absent
messages do not authorize reassignment.

## Dispatch and supervise

Read only the active adapter:
- [Codex](references/codex.md)
- [Claude Code Desktop](references/claude-code.md)
- [OpenCode](references/opencode.md)
- [Antigravity](references/antigravity.md)

Use host-native steerable conversations and their actual assigned worktrees. Detect loaded
capabilities; do not fabricate parameters or substitute opaque workers for conversations.
If creation is unavailable, provide prepared launch briefs and coordinate accessible existing
tasks. Manual start/finish remain available.

Each new task gets its full brief, scope, exact intended base, instruction to invoke
`stream-start`, acceptance gates, initial interface/dependency decisions, known coordinator/
peer IDs and stop-at-READY instruction. Omit model/effort overrides unless requested.
Children must not launch further coordinating tasks; bounded implementation/review delegation
follows repository and host policy without multiplying requested task fan-out. `stream-start`
provisions references; bridges run in that same checkout without another worktree or clone.

Record resolved task IDs and actual checkout paths, and report links. Use bounded waits or
reactive notifications, not repeated full transcript polling. Relay useful **STARTED**,
**DISCOVERY**, **READY** information. No sibling-write probe is required. Preserve other
valid work when one task fails; review failures do not waive repository review requirements.

## Integration

Prepare-only stops at READY tips, gates, review/debt and dependencies. Otherwise choose
integration order by dependencies (accuracy before Plus only as a tie-breaker), then run
`stream-finish` one at a time from a task able to access the integration checkout. Ordinary
Git checks and a single integrator suffice; no lease exchange or force-push.

After each merge, send affected tasks its SHA and changed interfaces. Their owner reconciles
and refreshes READY evidence before their turn. A moved integration ref calls for inspection
and affected gates, not a blind retry. Keep publication in the task holding direct user
authorization. If sandbox access prevents integration, deliver the exact READY branch/tip
and remaining finish command for the integration task/user; do not evade denial via a child.

Report integration SHAs, required CI jobs, artifact provenance, review debt and hardware
residuals. `stream-finish` owns local RBF delivery when synthesis is needed. Identify any
intermediate RBF separately from the final combined artifact. Follow common cleanup policy.
