# Classic CRTC unattended run, 2026-10-28

## Scope and starting position

This run starts from `5ade887` on the MacBook Air M5 in the task-owned
`codex/accuracy/crtc1-module-d-replay` checkout. The user's pasted handoff
predates the published `v2026.10.27` release and the integrated `019D`
production-T80 XFAIL. Current source is authoritative for task selection.

The user authorizes branch creation, integration and push. The run-specific
model roster supersedes the older generic routing: Sol 6.1 high handles the
authentic module-D timing investigation. Parent owns device operations and
acceptance. Existing worktrees, stashes and private inputs are preserved.

## Completed evidence unit

- Authentic production-T80/GA replay of module D `03AE`/`03B4`, retaining
  register-write phase and frame/address observations. A scripted pin replay
  already passes, so the integrated execution is the missing discriminator.
- Bounded D1 release-build retest, operated serially through the Mac mini.
  RBF `321f96a`, SHA-256
  `6bc437243db9ef5f9db2349f420520a46364dee4b7954ccc95e96f55ba36400d`.
  The verified temporary `mbc-fixed` helper was reprovisioned after `/tmp`
  lost its copy. Its SHA-256 is
  `f2b3a4f3cbd98c7a13f8770eb4c893bc352eae9d204f92f664f5eaad6c697330`.

## Evidence boundaries

`019D` remains blocked on the physical write-visibility rule; neither a
blanket latch delay nor an R7-specific exception is justified. `008D` still
needs original-hardware discrimination or an author answer. No inferred
counter clock or latch edge is to be promoted into RTL just to satisfy a
simulation.

The D1 `CSL_27` script actually loads `shaker26.dsk` / `SHAKE26D`.
The old snapshot's `A2FD` is delay-return padding, not the caller. Its stack
returns to `710B`, which repeats the interactive monitor-VSYNC-length sweep
through `7093`. The keyboard scan at `9FE0` controls the return at `7107`.
A snapshot in that loop alone cannot establish a CPU or CRTC wedge.

## Continuity and quota

Initial live Codex usage: five-hour 33% used; weekly 37% used. Two banked
resets are available and remain untouched. Work is bounded and stops before
the unattended reserve thresholds. Sleep/restart/quota-exhaustion recovery
is not verified; this file is the durable continuation point.

## Completion record

The authentic module-D replay completed without tracked code changes. Both
SHAKE26D and SHAKE27D A/C entries reach row 1 at `31B8`; three SHAKE26D
iterations retain the correct `31B8..31DF` addresses across all eight rasters.
Scratch reproduction sources and traces are retained under
`.roster-scratch/module-d/` and copied to
`docs/references/classic-overnight-2026-10-28/module-d/`. This uses production
T80/GA/CRTC but immediate RAM, direct PPI VSYNC and a BC00 RET stub; it is
not full-machine proof. No test, RTL, soak golden or production code changed;
no simulation gate or independent code review is required for this docs-only unit.

D1 on exact release `321f96a` again stopped at line 81, waiting for `0305`,
with 30 captures and zero capture failures. The runner restored the original
CFG. Snapshot `D1-release-1.sna` SHA-256
`e855e5092a13eaeb19d596247c5b6464e4bf1155457c50aa5e39786378120e75`
shows PC=A2F3, SP=405D, C0/C4/C9=51/1/1 and the same caller/sweep position as
the older sample (length `16`, outer count `10`). The first pull timed out;
a retry produced generation 1. Attempts to re-save did not advance the generation.

Static disassembly identifies an input deficit: the sweep at 702A starts
length 40, outer count 3A, decrements once per accepted advance and exits
when the count reaches 09 (49 advances). The supplied script sends 36+11=47
spaces after `0302`; both snapshots show only 42 accepted. That supports a
script pacing/count explanation but is not yet a proven fix. Additional input
after the snapshot did not visibly advance the screen; OSD/input/snapshot
state makes that control inconclusive. Do not claim a CRTC wedge from it.

Cold-boot direct entry E on the release reaches `03AE` and visibly says
"YOUR EMULATOR IS OK". Its manifest has 10 commands, one capture, zero capture
failures and one SSM record. Snapshot `E-03AE-release.sna` SHA-256
`c5cd9f998a56af521bef540f22927178714b1a5534f9f8af24e14ad40362f972`
is retained beside the PNG and manifests in
`docs/references/classic-overnight-2026-10-28/`. This supersedes the old WRONG
observation for this entry path only. The old capture used `cec641c` and
prior menu history; a controlled old/new or menu-history comparison remains.

Short-window usage reached 94% (weekly 47%), so the run checkpoints without
starting another unit. No banked reset was redeemed. A single heartbeat
`resume-classic-crtc-overnight-after-reset` targets this chat at 05:30 London
on 28 October, after the 05:25:06 reset. Its saved kind, target and schedule
were inspected. Actual wakeup/recovery is unverified and needs the Air awake,
Codex running and the workspace available. The Mac mini and MiSTer must also
be reachable for further device work.

## Resume next

1. Check live quota and current Git/device state; no substantial work above
   the unattended thresholds. Preserve shared account headroom.
2. Retain the completed direct E results below as the current cold-boot control.
3. Compare direct E on old `cec641c` with the release, then test prior menu
   history only if it changes the result. Do not patch a non-reproducing VMA rule.
4. For D1, try a scratch copy of the supplied script with at least 49 slower
   advances, without saving a snapshot first; preserve the source script.
   Re-test snapshot re-arming separately before relying on multiple samples.
5. Reconcile current-status and the older dated reports with these bounded
   results, then stream-finish the docs-only unit. No build is warranted.
6. `019D` and `008D` still need the independent evidence described above.

Device teardown must verify the original CFG SHA-256
`f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`
and `CORENAME=MENU`.

## Final device result before quota pause

The short direct-E walk also reaches `03AF` and `03B4`: 14 commands,
three captures, zero failures, five SSM records. `03B4` visibly says
"YOUR EMULATOR IS OK", matching the production replay. Capture SHA-256:

- `03AE`: `c91dea6b9b3b161b2cba377a1bea0d410552847eb3417ee52bf52beeaa8ef71b`
- `03B4`: `fcb90f46fa95fd8f3c16f1f4a2833ddb9c6b31fa4bd45c3d22a1451cb28c8d79`

Both tests therefore pass on this release/short-entry path. The older WRONG
captures remain evidence for their earlier build and menu history. No code
fix was made or inferred. The next useful discriminator is the controlled
old/new build comparison, followed by menu history if necessary.

The final device read verifies `CORENAME=MENU` and the original CFG hash
`f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`.
All provider/device commands have completed. Private media, snapshots,
replay sources and captures remain untracked; add only the named documentation
files when committing. Integration/push is deferred to the scheduled continuation
because the shared short window has exceeded the pause threshold.
