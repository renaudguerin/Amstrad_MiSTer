# Sonic DMA candidate: component gains and remaining phase discrepancy

Investigation from `cadbb3775d0a83fec3ceb124ebd377cbc829bfc5`, 22 September 2026.
The terminal-PAUSE candidate `190f4d3` / `a137d48` merits retaining as an
experiment: it improves measured interrupt cadence and greatly reduces full-list
phase drift against AmSpirit. Its integrated hardware acceptance still failed.
Keep restored behavior `b0e5bed` / artifact `0601050` as the default pending a
separate repair and acceptance result. Neither disposition settles the general
CPC Plus PAUSE rule.

No production RTL, test expectation or cartridge was changed in this task.
The [earlier report](hardware-loop-2026-09-22.md) owns artifact identities,
matched device captures and the original fail-first candidate tests.

## What the matched hardware controls establish

The `device-latefire-c59e03a/run.py` and `device-latefire-a137d48/run.py`
scripts are byte-identical. Their saved input schedules are identical: F18,
200 ms left-Ctrl fire, then F20, with an early and later replay. Model/CFG,
media hash, temporary map and restoration checks agree. The first title captures
were requested 18.595 s and 18.597 s after their respective load commands.
These are matched acquisition procedures, not proof of identical game state.

The replay log proves submitted transitions and owned-key release. It does not
observe the CPC input matrix or the cartridge accepting fire. The screenshots
show baseline progression to Act 1/gameplay while the candidate remains on the
corrupt title. That is a real acceptance difference under the tested procedure;
it does not establish whether the cause is input recognition, title/attract
state, a failed exit path or another timing effect. A short no-input control and
scene-verified sustained-fire control remain useful when the device is available.

Visual inspection finds different corruption, not a demonstrated visual win.
The candidate has a conspicuous magenta palette error and displaced/repeated
bands; the baseline also has severely displaced bands. Their animated frames
are not aligned closely enough to score partial pixel or palette improvement.
Do not equate the baseline's greater progression with correct rendering, or the
candidate's continued corruption with absence of any timing improvement.

## Full-frame production-T80 discriminator

Two scratch builds use the current production-T80 D5 adapter, production
clocking and WAIT, the same unchanged original Sonic CPR, and the same
observation program. The baseline uses current `asic_dma.v`; the candidate
substitutes only the exact `a137d48:rtl/plus/asic_dma.v` in generated build inputs.
Neither build loads a snapshot. The initial ASIC-write sequences through the
first DMA enable match. Shared generated sources and production RTL remain
unchanged between builds.

Both runs have a 1,280,000,000-master-tick cap and stop 8,000,001 ticks after
the first interrupt acknowledge. They cover six CRTC frame wraps and 1,953
R1 comparison edges after that first acknowledge. The fixture uses 64 MHz
master ticks and 4,096 ticks per programmed 64-character scanline. A handler's
instruction-entry tick can jitter within a line; the table reports CRTC line
placement, not falsely exact CPU-entry periods.

| Observed quantity | Restored baseline | Terminal-PAUSE candidate |
| --- | --- | --- |
| Repeated `PAUSE 7; INT` acknowledgement spacing | 9 lines | 8 lines |
| `PAUSE 15; INT` acknowledgement spacing | 17 lines | 16 lines |
| First three A021 entries, raw CRTC line | 231, 255, 279 | 229, 230, 231 |
| Steady DMA-list recurrence in scanlines | 336 | 313 |
| Phase drift against a 312-line CRTC frame | +24 lines/list | +1 line/list |
| Completed interrupt acknowledgements | 163 | 172 |
| Empty-source acknowledge starts | 0 | 0 |
| Same-channel INT-set/clear collision ticks | 0 | 0 |
| Actual `split_latch_event` edges in captured window | 24 | 0 |

The candidate therefore has a concrete **partial timing improvement** beyond the
previous ten-ack trace. It still does not match the reference's one-frame loop.
A 313-versus-312 discrepancy is not by itself an ASIC defect: list execution,
CPU rearm and instruction/acknowledge phase all contribute to the period.

Both builds reach the final DMA2 interrupt, identified by SAR2 `83D0`, with
last fetched instruction `FBF8: JP (HL)`, the intended wait loop. Both disable
and re-enable DMA2 on every observed list. Thus neither a main-work overrun at
that interrupt nor a lost rearm is reproduced in this window. This does not
exclude a later or device-only throughput problem.

The rearm boundary is concrete. In the candidate's first completed list,
SAR2 low/high stores at F922 are observed at line 165, HCC 53/54; STOP clears
the enable between them. The DCSR store at F927 re-enables DMA2 at HCC 62.
The next loops rearm at lines 166, 167, 168 and 169. The baseline makes the
corresponding stores at line 188, HCC 53/54/62, then lines 212, 236 and 260.
R2 is 51 in this title: the stores are after that line's HSYNC start.
Current RTL takes the active-channel snapshot at HSYNC, selects SAR in
`ST_DEAD`, then fetches and executes the instruction. With only DMA2 active,
the old STOP has already been selected before the CPU's SAR reload. DCSR
subsequently re-enables the channel after that line's scheduling opportunity.
This explains the modeled extra restart line; whether CPU/acknowledge latency
or the DMA slot placement differs from hardware remains unresolved. It is not
evidence that an enable write was lost or that clear priority needs changing.
The trace reports the beginning of a multicycle write window; any sub-line
margin calculation must retain its end and the actual SAR sampling edge.

The split observation uses the **pre-edge** `split_latch_event` and its actual
SPLT/SSA/SSCR inputs at HCC=R1. With R1=49 in this title, the first candidate
SPLT=7 store occurs on line 229 and the following SPLT=15 store on line 237.
The intended comparison lines have already passed; the registers change again
before their later eight-bit aliases are reached. No split capture occurs in
the bounded candidate window. The baseline's rapidly drifting chain happens
to intersect some comparison lines. Its 24 captures do not mean 24 correct
splits. This explains why better relative cadence alone is insufficient to
establish correct rendering; it does not assign the phase error's cause.

## Fresh AmSpirit control and software boundary

AmSpirit Lite 1.15.1, core 2491682, was booted from the same original CPR in
6128 Plus/type 3. A fresh normal-boot run recorded all 22 handler entries
across two consecutive frames, checking the PC against each requested
breakpoint. A021 is at beam Y=70 in both frames; FBF9 is at Y=318 in both.
The intermediate repeated intervals are 8 lines, with the list's longer
intervals 16, 48 and 40 lines. The recorded CRTC setup is non-interlaced,
R0=63, R4=38, R5=0 and R9=7: its frame is `(38+1)*(7+1)=312` lines.
Beam coordinates are emulator coordinates, not a direct raw-CRTC mapping.

The runtime list at 835A starts with PAUSE 42, then fourteen non-PAUSE,
non-INT slots (including the 4000 command), then 22 positive PAUSE/INT pairs,
and STOP at 83D0. Under the candidate's PAUSE scheduling, arithmetic places
the final INT 311 scanline intervals after the first PAUSE fetch, and STOP on
the next HSYNC. That describes list slots, **not the CPU-controlled restart
period**. Mapped CPU bytes independently identify F922 as `LD (6C08),HL` and F927 as
`LD (6C0F),A`; the path also writes IVR=0 and resets the JP chain to A021.
Counting STOP as a necessary extra gap would assume that software
cannot rewrite SAR/rearm before that fetch. The fresh AmSpirit recurrence
shows the actual program can repeat in one frame on that implementation.
Adjudicate the restart boundary before changing another DMA rule.

A separate rearm probe reaches FBF9 at beam (144,318), F922 before the SAR2
store at (800,318), F927 before the DCSR store at (912,318), then A021 at
Y=70 in the next frame. Its subsequent continuation fails, so these are a
single boundary witness, not a three-iteration validation. The successful
44-handler normal-boot run is the accepted recurrence reference.

Imported-snapshot debugger runs also became black after six expected handler
entries. They are excluded from phase/progression conclusions. Adding rearm
breakpoints similarly disrupted later continuation even without intermediate
snapshot/CPU-memory dumps; the responsible debugger operation is not isolated.
Do not attribute these failures to the cartridge or ASIC. The final restoration
applies saved configuration/render settings **before** loading the saved full
SNA, clears breakpoints and returns to running BASIC Ready; applying settings
after the SNA had disturbed the restored display. No emulator input was injected.

## Other ASIC leads and next bounded experiment

Sonic-specific priority does not determine whether an independent accuracy
finding deserves implementation. The coordinator's B20 backlog, introduced in
`5d26524`, separately owns those source discrepancies and discriminators.

- **I1/I2:** no empty acknowledge start occurs in either bounded trace. The
  interrupted instruction/board-level A13 bug remains a separate diagnostic;
  handler/table placement does not establish immunity.
- **I4:** PRI is zero during the captured title loop, so nonzero-PRI phase
  changes cannot explain this interval. Preserve accepted PRI no-wrap and
  the existing B19 retention/provenance behavior.
- **I5:** all observed PPR writes are zero with channels disabled. No live
  mid-pause PPR update is exercised here; B20-1 remains independently actionable.
- **I6:** DCSR reads were not instrumented. No conclusion about read polarity
  or frozen readback follows from this experiment.
- **Same-channel set/clear:** zero collisions in the sampled pre-edge mask.
  STOP occurs near SAR rearm, but both builds successfully re-enable. This
  does not settle general STOP/write ordering or the full DCSR write window.

The next useful discriminator is the **final INT → CPU acknowledge → FBF9 →
SAR2/DCSR writes → next first-PAUSE fetch** boundary. Capture raw HSYNC and
exact fetch/execute slots, `/M1`, `/IORQ`, `/WAIT`, T80 SP and bus data. Derive
CPU instruction timing from the actual mapped FBF9/F922 code; compare equivalent
AmSpirit events without converting beam X to raw HCC by assumption. Separate
CPU/ack latency from DMA slot placement and initial VSYNC/enable phase. A minimal
cross-module fail-first vector is justified only after the boundary rule is
supported; the current evidence does not authorize another speculative RTL fix.

When MiSTer is available and its owner explicitly releases it, add paired
no-input and sustained-fire controls at visually verified scenes. Keep RBF/hash,
settings, key release and MENU restoration explicit. MiSTer became unreachable
during the Classic owner's window; this task never contacted it and cannot
claim new hardware acceptance or its restoration.

## Reproduction and evidence

Ignored evidence: `docs/screenshots/sonic-phase-2026-09-22/`. Preserve it in the
main checkout before removing this task worktree. `provenance.json` records
source, binary, trace and analysis hashes; `baseline/` and `candidate/` contain
the build logs, traces and timing. Builds use each directory's scratch Makefile
from `sim/plus`; each binary runs with the original CPR and cap `1280000000`.
`prepare.py`, `trace.cpp` and `analyze.py` preserve the diagnostic setup and
`summary.json` the extracted events. Runtime was 621.33/614.81 wall seconds.

The external SDRAM model and stubbed unused Classic GA remain fixture limits;
this is production-T80 integration evidence, not physical CPC Plus timing.
The scratch RAM dumps use the wrong physical RAM base and are **not CPU-memory
evidence**; the same limitation applies to the earlier ten-ack dumps. Runtime
handler execution is evidenced by the bus trace; program list inspection uses
AmSpirit's correctly identified base RAM. No conclusion relies on those dumps.

`amspirit-normal-boot/handlers.json` contains the accepted 44-entry control.
Failed or partial acquisitions remain separately labelled. `restored-final/`
contains the verified BASIC Ready screenshot and final settings/state.
Opus high's initial and follow-up causal judgments are retained alongside the
traces. They are independent hypotheses checked against the measurements,
not substitutes for the observations. Parent checking corrected the follow-up
claim that all baseline split hits were in one frame: they span frames 1–4.
The fresh 44-handler control also supersedes its concern that A10B was not
reached in the failed snapshot probe. Documentation-only changes require no
simulation suite or synthesis; the two diagnostic executions are reported
separately from those gates.

Follow-up: the [rearm-boundary investigation](rearm-boundary-2026-09-22.md)
adds paired no-input and sustained-fire hardware controls, retaining the timing
gain and the progression failure as separate findings. Baseline reaches a
playfield presentation without input; candidate remains on the corrupt title in
both tested input conditions. The follow-up records acquisition timing and active
Sync verification limits explicitly.
