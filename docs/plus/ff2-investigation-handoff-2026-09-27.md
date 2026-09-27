# Fresh-session brief: FF2 PRI investigation, then Eerie Forest

## Integration and evidence preservation

The successor is active in `/Users/renaudg/.codex/worktrees/603a/Amstrad_MiSTer`
on `codex/plus/ff2-pri-investigation`, starting from `7d80ddf`. The original
handoff documents were copied there before integration. The source session is
integrating its completed documentation and diagnostic into master; it is not
changing the successor's branch or running emulator tests.

Private evidence and generated outputs have been preserved in the main checkout
under `local/task-archives/crtc3-2026-09-27/`: `evidence/` contains the original
`docs/specs/crtc3-2026-09-25/` tree, and `output_files/` contains the generated
outputs. Use this durable location when the original 04d3 worktree is archived.
The original paths below describe the investigation checkpoint.

Resume this authorized investigation using `stream-start`. The previous chat
has stopped work and yields ownership; do not launch further coordinating
chats. Bounded subagent work follows AGENTS.md. Investigate, implement only
when evidence supports a fix, and stop at READY with a human-test handoff.

## Checkout and current position

Source checkout: `/Users/renaudg/.codex/worktrees/04d3/Amstrad_MiSTer`, branch
`codex/plus/crtc3-demo`, committed tip `7d80ddf`. Preserve its private evidence
and uncommitted documentation. If the app assigns a different checkout, inspect
the source branch and transfer its uncommitted docs deliberately before work;
do not start from master and lose this context. Do not reset either checkout.
Main checkout is `/Users/renaudg/code/Amstrad_MiSTer`; at handoff its master
was `ace9842`, with later investigation commits still on the task branch.

Uncommitted handoff changes are this file, the new emulator survey, and its
link from `docs/plus/remaining-title-regressions-2026-09-27.md`. No RTL changed
in the source-survey turn. `git diff --check` passed for that survey.

Read first:

1. `docs/plus/remaining-title-regressions-2026-09-27.md` — bisect, CPCEC,
   Opus opinion, alias probe and AmSpirit results.
2. `docs/plus/references/emulator-pri-scroll-survey-2026-09-27.md` — completed
   delegated Caprice32, CPCSyntaxError and konCePCja source reviews. Do not
   repeat this survey.
3. `docs/plus/references/amspirit-pri-phase-2026-09-26.md` — ordinary PRI
   phase/source disagreement and why blanket retiming is not established.
4. `docs/plus/ack-provenance-regression-2026-09-26.md` — accepted ACK fix to
   protect; `docs/current-status.md` for wider state.

## Accepted behavior and remaining defects

User confirms CRTC3, Copter 271 and World of Sports BMX fixed on `cf62d5f`.
Current accepted RBF is main-checkout
`output_files/Amstrad_20260926_cf62d5f.rbf`, SHA256
`ae4d5b7881f3a6740d40a02fcea78762cef3ae49c943329fc800c9714d3f27a7`.
Preserve the ACK provenance repair and five CRTC3 repairs.

- FF2: sky flicker and music slowdown in attract mode without input, as well
  as gameplay. Hardware bisect: `84e6969` good, `bee92a6` good, `c595031` bad,
  `05cb9fd` bad, `88262b9` bad. Only production RTL change between bee92a6
  and c595031 is `b5c3014`, changing PRI comparison from low-eight-bit alias
  to full nine-bit comparison with bit 8 clear. This is the trigger, not
  proof the correct repair is to revert it: it fixed Copter on hardware.
- Eerie Forest: left-edge screen-plane spill remains; reveal strips fixed.
- Prehistorik II: one corrupt line just above bottom HUD remains, absent on
  edaa15b. Do not start with its cumbersome gameplay navigation.

Hardware settings: 6128Plus, Full sync. User owns visual/device acceptance.
Targeted AmSpirit API diagnostics are authorized; save and restore the user's
session. User has original Plus/GX4000 and can run CPRs if needed, but prefers
AmSpirit first and can do a human check when requested.

## Next investigation

Locate FF2's runtime PRI-writing code and interrupt handler, then trace old/new
PRI, line and write phase, request/acknowledge ordering. Distinguish:

- Changed PRI becomes current line during raw HSYNC after an ordinary event:
  missing write-trigger hypothesis.
- Write falls outside raw HSYNC because our ordinary IRQ is later: phase
  hypothesis.
- Game intentionally requires another line event: revisit assumptions rather
  than forcing the preferred hypothesis.

CPCEC `/Users/renaudg/code/cpcec/cpcec.c`, commit
`c025aab961a796b918cc99bc3e16216ea65bb5d1`, uses our nine-bit VC/RC line formula.
Its ordinary event is raw HSYNC assertion (`874–877`). Its changed PRI write
path (`2106–2114`) asserts if new PRI matches current line during raw HSYNC,
explicitly commenting that FF2 needs it. A separate branch clears pending
raster and mentions Eerie. Do not bundle that disputed clear behavior into a
write-trigger fix. Same-value writes bypass the branch. The old good core also
lacked the write trigger: alias IRQs may have masked another missing behavior.

Opus5.5-high already assessed this lead conditionally; its CPCEC read was
directory-denied, whereas native Astra directly inspected the source. Opinion
run `20260927T004637Z-50401-f7ec` is preserved in ignored
`docs/specs/crtc3-2026-09-25/ff2/opus-opinion/`.

The standalone `scripts/diagnostics/pri_alias_probe.py` generates four 32-frame
counts. AmSpirit gave exactly 32 in every case: PRI100/R6=25, PRI10/R6=25,
PRI10/R6=34, PRI255/R6=25. No alias even with taller display, and off-display
PRI works. No original-hardware result yet. Delivered CPR SHA256
`c41a332ba467c0830f894a689f7f525a0794b275b7f83e116218bdb6b368a0b9`.
Do not repeat this test merely to recover context.

New emulator reviews provide no decisive replacement rule. CPCSyntaxError
has the same line formula with delayed IRQ; the other two use elapsed scanlines.
konCePCja's IPC write watchpoints could locate PRI writers, but first establish
it executes the relevant FF2 scene. It has no raw IRQ/HSYNC timeline; debugger
SL is truncated to eight bits and PRI pending is omitted. Avoid tool-building
detours when existing AmSpirit breakpoints can answer the question.

## Tools and evidence

FF2 CPR: main checkout `local/test_media/cartridges/01_PlusGames/Fire And Forget II.cpr`,
SHA256 `ce72fcf911b4b403a5012f8dedabd567c80a8af2e43e1b0646fd55468e48e794`.

AmSpirit API: `http://127.0.0.1:6128`, previously Lite1.15.1/core2491682.
Read `scripts/amspirit/README.md`, helper `scripts/amspirit/amspirit.py`, and
`docs/investigations/hardware-runs/amspirit-oracle-design-2026-09-13.md`.
The existing save/restore diagnostic is in ignored
`docs/specs/crtc3-2026-09-25/ff2/alias-probe-amspirit/run.py` (also previously
`/tmp/ff2-pri-smoke.py`). Previous runs restored snapshot/config/render/pause.

CPR loading needs explicit reset. `/api/z80_bp` accepts CPU addresses or
physical-bank addresses; `/api/step` executes one instruction. RAM views and
SNA can recover code/ASIC registers. API state does not expose live PRI/DCSR,
and CRTC API does not expose raw HSYNC or full C4/C9: do not present screen
beam coordinates or instruction-stop timestamps as equivalent bus events.
Avoid long Lua evaluations that continue blocking after caller timeout.

Other source checkouts: `/Users/renaudg/code/caprice32`, `CPCSyntaxError`,
`konCePCja`; exact revisions/anchors are in the survey. Do not modify those
checkouts or CPCEC user configuration as part of source inspection.

Eerie existing replay: see
`docs/investigations/hardware-runs/eerie-forest-graphics-2026-09-23.md`.
Residual SSCR AC→8C at C0=2 dot0 as 16-dot mask ends; RA6 data emerges through
12-dot horizontal delay after RA becomes4. No basis for flushing/widening yet.
Ordinary PRI width mismatch is revised Arnold versus Thacker/AmSpirit, not an
established ACCC disagreement. Source rationale is already documented.

## Implementation gates and stopping point

Timing/state RTL changes require a focused deterministic failing test before
implementation. Preserve nine-bit comparison and ACK provenance unless new
evidence explicitly overturns them. Use selected tests once after final edit,
fresh cross-provider review, and record exact commands/results. Existing macOS
test compiler setup, if still needed:

```
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++'
CXX=/opt/homebrew/opt/llvm/bin/clang++
GHDL=/tmp/eerie-ghdl/ghdl-llvm-6.0.0-macos15-aarch64/bin/ghdl
```

Use Astra high for deep troubleshooting, Astra medium for tricky implementation,
and guarded Opus5.5 medium for independent review, subject to current roster
instructions. No provider bridges were left running. Preserve private ignored
evidence and user-owned ACCC PDFs. Stop at READY with concrete findings/fix,
remaining uncertainty and a short human checklist; do not claim hardware
acceptance from simulator or emulator agreement.
