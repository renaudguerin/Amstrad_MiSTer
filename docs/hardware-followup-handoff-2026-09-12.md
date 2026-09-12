# September 12 hardware follow-up: prepared branches

The requested documentation pass and coordinated B2/B6 work are complete at
the local READY boundary. **B2 is locally integrated from refreshed `cd08a1e`;
B6 integration and the combined push remain pending. No new RBF was built.** The user's hardware observations remain tied
to `5c16b17`; see the [hardware report](hardware-evidence-2026-09-12.md).

## Branches and owners

All task IDs below are on the local host. Integration base is
`013c7e5470187c795f7df2cb1aba01f577a4ab44`. Both implementation branches
include documentation prerequisite `cf1b30736325c00f286b24841e309221f7b796a5`.

| Task | Branch / accepted tip | Assigned checkout |
|---|---|---|
| Coordinator `01a0948e-97bf-7971-a22e-7fb3edaad675` | `codex/general/hardware-retest-20260912`; includes `cf1b307` and boot-summary reconciliation `1119987` | `/Users/renaudg/.codex/worktrees/936a/Amstrad_MiSTer` |
| B2 `01a094a4-1954-7913-a4b9-84357eda57e4` | `codex/general/b2-device-capture`, `5de8c5ccdd584ff7302cd0d51c7622d32d24cab3` | `/Users/renaudg/.codex/worktrees/7ed2/Amstrad_MiSTer` |
| B6 `01a08ecf-eca4-7143-afce-93b8457d45e2` | `codex/general/b6-video-boundary`, `c2fde66c5097d41456f26eb1becdcd05a724355a` | `/Users/renaudg/.codex/worktrees/5614/Amstrad_MiSTer` |

## B2 accepted outcome

The existing driver now has a real French-ROM SHAKER 2.7 B (9) launch case
and optional RBF/media SHA-256 pins checked before upload/load. On `root@mister`,
the exact device `5c16b17` RBF was verified; three independent loads produced
nine byte-identical, decoded 768 x 273 CRTC1 numeric screens. Coordinator
inspection confirmed the named screen and image hashes, inspected the diff
and independently reran all 26 focused tests successfully.

Full/Raw B (9) captures match. Native captures omit OSD, so mode selection is
supported by saved CFG evidence, not an independently observed active latch.
C (4) selection was confirmed by its menu; its disrupted raster is not a pass.
The device was left on stable B (9), original Amstrad configuration bytes were
restored, and temporary MBC/MGL/input processes were removed. Task screenshots
remain on the device. No further device work is pending in this batch.

Read the complete branch record with
`git show 5de8c5c:docs/b2-device-capture-2026-09-12.md`.
Private manifests/images are in the B2 checkout's ignored
`docs/references/b2-device-20260912/` directory.

## B6 accepted outcome

The production change only moves `video_mixer.sv` RGB wires to module scope,
preserving existing CRLF endings and expressions. The original source failed
the four-parameter-combination RGB test with 178 mismatches; corrected source
passes. This is a simulation elaboration finding, not proof of what the old
Quartus build synthesized or a cause of the user's hardware symptoms.

New fixtures cover CPU-driven malformed raster sequences and combined Plus
scrolling/sprites through the actual final output. All 18 existing B6 cases,
nine dynamic cases, five Plus layer/control cases and the complete simulation
gate pass; lint and canonical soak `0xb1cb70da95c2e44f` also pass. The
coordinator inspected the diff and logs and independently reran the mixer
regression and CRLF-aware whitespace check. Full-suite reruns were not duplicated.

The type-0 short-sync trace has visible Full/Raw differences with identical
acquisition; type 1 can legitimately remain identical. A sustained CPU-generated
stuck-high raw-sync recipe remains unestablished. Physical HDMI/CRT acceptance,
production-T80 timing and named title correctness are not closed.

Read the complete branch record with
`git show c2fde66:docs/b6-video-boundary-review-2026-09-11.md`.
Private logs are in the B6 checkout's ignored
`docs/references/b6-completion-2026-09-12/` directory.

## Review and integration boundary

B2's final implementation has completed Gemini cross-provider clearance.
B6's Gemini review and scoped correction review cover its final source.
Opus attempts failed authentication before reviewing; they are not clearances.
After authentication was refreshed, the user instructed the tasks not to
duplicate completed reviews. No redundant Opus pass was required. The
coordinator accepted both handoffs after inspecting their final changes.

Integration is now authorized. The coordinator documentation is merged at
`6dac98b`; B2 is locally integrated at `e391e137b7cc43107dc61a0c9a279cccf3cdffae`
from refreshed `cd08a1e`. B6 is refreshing against that destination, with its
source integration pending. The coordinator alone
integrates the accepted source branches sequentially, reconciling shared
status, backlog and review prose. B2 and B6 have no source-interface dependency.
Use `stream-finish` for destination gates, publication and exact-build RBF
delivery; B6's production `sys/` change requires the normal synthesis policy.
Preserve all private evidence before any worktree cleanup. The user invoked
`stream-finish` for these tasks, authorizing sequential integration and push.
See current status for subsequent integration and exact-build evidence; the
table above preserves the accepted source tips.
