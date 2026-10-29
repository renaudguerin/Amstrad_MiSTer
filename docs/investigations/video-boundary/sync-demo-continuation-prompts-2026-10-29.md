# Unattended continuation prompts

Choose **one of two alternatives**:

- **One session:** paste only the combined prompt below. It owns all three paths.
- **Separate sessions:** paste A, B and C into three fresh chats, each with its
  own worktree/branch. Do not also run the combined prompt against that work.

Separate sessions can run source analysis and simulation in parallel. They
must coordinate device access and overlapping semantic changes explicitly;
separate worktrees alone do not make those safe. There is no dependency on
finishing a filter redesign before investigating either demo defect.

For the separate-session option, designate A (V2/DSC4) as the initial coordinator
and sole MiSTer operator. B/C send capture requests and consume its artifacts.
All three may exchange task-relevant coordination messages; establish peer chat
IDs at startup using available chat tools. No reply or absent peer is permission
to take the device. Ownership can transfer only by explicit request/acknowledgment
between old and new operator after input release, CFG restoration and MENU.
Until that handoff, the old operator remains responsible. If coordination is
unavailable, continue offline work and defer device access. Use the same rule
for the shared AmSpirit instance. Agree before changing shared CRTC/GA semantics
or the video interface; reconcile and integrate sequentially.

SSH directly to `root@mister` from this Mac by default. Direct read-only access
was verified on 2026-10-29. Mac mini was the previous run's route, not a hardware
requirement. Use another host only for an actual tool/reachability limitation,
with the same single-operator ownership; changing hosts does not grant access.

These are prompts, not scheduled jobs or created chats. The files are prepared
in the d434 checkout; make them available to new worktrees before starting.
Do not assume they are already on master.

## One session, all three packages

```text
Run an unattended classic CPC video investigation. Read AGENTS.md,
unattended-overnight and its Codex reference, live quota, docs/backlog.md B1/B22,
docs/current-status.md, and
docs/investigations/video-boundary/sync-response-and-demo-plan-2026-10-29.md.
Use the current applicable roster, preserve existing work and reuse a suitable
checkout. Do not inherit an old overnight run's temporary model overrides.

Treat these as three independent paths with separate acceptance verdicts:
V2: DSC4's display defect reportedly does not vary with sync-filter setting.
Prioritize suspected CRTC/GA execution, write timing, addresses and raw pixels;
do not make repair depend on sync-response work.
V1: SHAKER AT/AY/D3/D4, B22 acquisition, and Amazing's upper effect. The user
reports the upper effect changes with filter setting and works better in Full
on HDMI. Investigate presentation independently; prototype CRT response offline.
V3: Amazing's lower flashing corruption, suspected CRTC/GA, independently of
its upper effect and DSC4. A shared cause is unproven.

DSC4: classic CPC 6128, Plus off, CRTC1, device media
/media/fat/games/Amstrad/dsk/dsc4-alt.dsk, RUN"-DSC4 then Return; wait at least
50 seconds after launch before the first capture and retain repeated frames.
Amazing: dsk/amazing2021_rev2.dsk, RUN"AMAZING, same initial model, capture at
30/45/60 seconds. Share Amazing captures between V1/V3 but judge regions
separately. Pin media/RBF and effective settings, preserve manual screenshots,
repeat from fresh boots and use scene/state-aligned AmSpirit replay/snapshots.

Choose bounded units independently; DSC4 may be tackled first. Do not require
all demo baselines before V1 or finish V1 before either suspected CRTC/GA repair.
Opus high advice is already recorded; read it rather than repeat the call.
Read mister-capture and amspirit-test before using them. MiSTer operations are
serial via direct SSH to root@mister; explicitly restore original CFG/MENU after each batch,
including failures. Preserve supplied media. Never infer analog behaviour from
HDMI PNGs or CRTC correctness from screenshot resemblance.

Every timing/state repair needs source-derived fail-first proof, appropriate
selection gate and soak handling, fresh cross-provider review and matched
hardware retest. Keep evidence ignored and write a durable handover separating
all three paths. Commit finished units on the task branch; no integration,
push, publication, reset redemption or paid overage without authorization.
```

## Separate session A: DSC4 repair (V2)

```text
Before device or shared AmSpirit use, identify the other V1/V2/V3 chats and
exchange task-relevant coordination messages (authorized by this prompt).
Session A/V2 is initially coordinator and sole operator; B/C send it capture
requests. Do not take ownership without an explicit acknowledged handoff after
cleanup, or infer permission from silence. Continue offline if peers are not
reachable. Use separate worktrees; agree on shared semantic/interface changes
before editing them and integrate sequentially. Use direct root@mister SSH;
Mac mini is optional only if a demonstrated tool/connection limitation needs it.

Run V2 unattended from
docs/investigations/video-boundary/sync-response-and-demo-plan-2026-10-29.md.
Read AGENTS.md, unattended-overnight plus its Codex reference, live quota,
current-status, mister-capture and amspirit-test. Preserve existing work.
DSC4: classic 6128/Plus off/CRTC1, MiSTer dsk/dsc4-alt.dsk, RUN"-DSC4.
First capture at least 50 seconds after launch Return; retain repeated frames
and two independent boots. Pin identities/settings and preserve the latest
manual DSC4 image if available. Its defect reportedly does not vary with sync
filter setting; prioritize suspected CRTC/GA execution, write timing, addresses
and raw pixels. This task does not depend on V1 or Amazing. Use AmSpirit state
and replay diagnostically. Fix only a justified defect with source-derived
fail-first proof, required gates/review and matched device retest; otherwise
leave a precise evidence gap. MiSTer serial via direct SSH to root@mister, restore original
CFG/MENU after each batch. Commit finished units, no integration/push/publication
or paid quota actions. Preserve artifacts and a fresh-session handover.
```

## Separate session B: sync response / Amazing top (V1)

```text
Before device or shared AmSpirit use, identify the other V1/V2/V3 chats and
exchange task-relevant coordination messages (authorized by this prompt).
Session A/V2 is initially coordinator and sole operator; B/C send it capture
requests. Do not take ownership without an explicit acknowledged handoff after
cleanup, or infer permission from silence. Continue offline if peers are not
reachable. Use separate worktrees; agree on shared semantic/interface changes
before editing them and integrate sequentially. Use direct root@mister SSH;
Mac mini is optional only if a demonstrated tool/connection limitation needs it.

Run V1 unattended from
docs/investigations/video-boundary/sync-response-and-demo-plan-2026-10-29.md.
Read AGENTS.md, unattended-overnight plus Codex reference, live quota,
B1/B22 and the B6 design. Read the completed Opus high advice in the work plan.
Investigate SHAKER AT/AY/D3/D4, B22 acquisition, and Amazing's upper effect,
which the user reports works better in Full on HDMI. Amazing media is
dsk/amazing2021_rev2.dsk, RUN"AMAZING, initially classic 6128/Plus off/CRTC1,
captures at 30/45/60 seconds. Share captures with V3 if available; lower flashing
corruption is V3's separate suspected CRTC/GA issue, not this task's closure gate.
Do not wait for DSC4 or V3. Resolve exact SHAKER pages/history, separate raw
CRTC/GA accuracy from acquisition and physical monitor response, and prototype
any digital response model offline with held-out controls before RTL. No title-
specific tuning or speculative CRTC fixes. Own filter/output work and coordinate
overlapping writers. MiSTer serial via direct SSH to root@mister; restore original CFG/MENU.
Apply fail-first proof, gates, review and device retest to behaviour changes.
Commit finished units, no integration/push/publication or paid quota actions.
Leave a design decision, evidence limits and next concrete implementation slice.
```

## Separate session C: Amazing lower corruption (V3)

```text
Before device or shared AmSpirit use, identify the other V1/V2/V3 chats and
exchange task-relevant coordination messages (authorized by this prompt).
Session A/V2 is initially coordinator and sole operator; B/C send it capture
requests. Do not take ownership without an explicit acknowledged handoff after
cleanup, or infer permission from silence. Continue offline if peers are not
reachable. Use separate worktrees; agree on shared semantic/interface changes
before editing them and integrate sequentially. Use direct root@mister SSH;
Mac mini is optional only if a demonstrated tool/connection limitation needs it.

Run V3 unattended from
docs/investigations/video-boundary/sync-response-and-demo-plan-2026-10-29.md.
Read AGENTS.md, unattended-overnight plus Codex reference, live quota,
current-status, mister-capture and amspirit-test. Preserve existing work.
Use MiSTer dsk/amazing2021_rev2.dsk, RUN"AMAZING, initially classic 6128,
Plus off, CRTC1; capture at 30/45/60 seconds with repeated frames. Investigate
lower-screen flashing corruption as a separate suspected CRTC/GA defect:
continuous execution, write timing, RAM/buffer selection, fetch addresses and
pre-filter pixels. Use scene/state-aligned AmSpirit replay/snapshots to narrow
the problem. The upper effect belongs to V1 (Full currently works better on
HDMI); retain it as context but do not conflate its improvement with this fix.
Do not wait for V1 or assume a shared cause with DSC4. Reuse captures where
available. Fix only a justified defect with source-derived fail-first proof,
required gates/review and matched lower-screen device retest. MiSTer serial
via direct SSH to root@mister; restore original CFG/MENU after each batch. Commit finished
units, no integration/push/publication or paid quota actions. Leave a separate
V3 diagnosis, evidence bundle and fresh-session handover.
```
