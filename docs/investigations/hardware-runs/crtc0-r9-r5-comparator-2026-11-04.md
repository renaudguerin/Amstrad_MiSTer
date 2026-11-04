# Type0 R9-to-R5 comparator handoff — 4 November 2026

The Type0 counter switched to R5 as soon as adjustment management was armed,
before the pending C4 increment. French ACCC1.11 §11.2.2 p83 example3 instead
requires continued C9/R9 comparison while C4=R4, then C9/R5 after C4 increments.
A source-derived retained t12 extension fails before the fix. The candidate
reuses existing phase state; fresh Opus review accepts the corrected candidate.
Full synthesis and targeted E6/DSC4 device acceptance now pass.

## Independent counter proof

Parent and worker read the rendered French page, not a flattened chronogram.
With R0=63, C4=R4=38, C9=R9=7 and management armed, write R9=10 at C0=12.
The required subsequent (C4,C9) pairs are(38,8),(38,9),(38,10),(39,0).
Only after that C4 step may R5 counting begin. The pre-fix engine gives(38,11)
at the fourth boundary. Public RA expected0/actual11 is the first failure;
there is no invented expectation that management itself must stay unarmed.

The private source recipe uses R5=22. The retained existingt12 owner usesR5=16
and writes atC0=10, within the same2..R0−1 window; its follow-through checks
RA8,9,10, then C4=39/RA0, then16R5lines to frame origin. The original assertions
remain. This pins a shared counter/comparator interaction instead of adding a
duplicate owner for the first-line symptom.

Current SHAKER E0 case21 at62D5 programsR5=22 and R9=10 at the same source
phase, then prints expected66 at6330. The native7ac11a2 result is003A. The
counter trajectories differ by11lines×64µs; the author's16µs polling loop
gives44iterations, exactly66−3A. This is corroboration, not an original-CPC
photograph oracle: no moduleE reference photograph is cached.

## Scope and review boundary

The engine retains its existingR9 live-comparison latch until C4 advances,
keeps frame-end and VSYNC line-end consumers on the selected comparator, and
then begins R5 counting. No new flip-flop, shared-wrapper sequencing change,
Type1 change or ParityR6 rule is intended. LiveC9/R9 VMA saving remains active
even during the R5 counter phase. Exact-R0 R9 straddles retain their separate
precedence. Snapshot/type/reset clear behavior retains the existing ownership.

Fresh Opus review accepted the main source reading but found an R4-cancellation
interaction: clearing R9 phase alone left a LastLine latch sampled underR9.
When cancellation occurred onC9=R9 it could end the frame prematurely; on
C9=R5−1 it could miss the R5 match. A C0=1 R4 change followed by an R9 write
also retained R9 after C4 differed from R4. Three independently derived cases
were made red on the first candidate before correcting production again.
Their source anchors are §11.2.2 p83 examples2/3, §13.2.1 p106's unequal-write
history and §12.2 p95's early entry. The correction must select liveR5 for the
rest of the cancellation line and preserve that selection after an equal
restoring write. Existing R4-history/C0=1 owners carry these interactions.

Both C0=1 entry routes now use the same root qualification: after an R4 **or**
R9 break the current line is already the first additional line (§12.2 p95),
so a later R9 window write cannot store a pending last-frame-line phase. The
combinational current-line comparison is unchanged. The retained existingR9
owner also proves that sibling red before the final edit (RA0expected/2actual),
green afterward. This replaces the provisional R4-only inequality clear.

ParityR6 at an unusual R6=R4+1 delayed step remains a separate source question:
§19.5.2 p206 captures on reachingR6 while §19.6.1 p217 freezesR6>R4. This
candidate does not invent a parity rule for that boundary. The original E0
R8=0/R6=25 recipe does not depend on it.

## Evidence and next acceptance

Ignored `local/overnight-20261103/resumed-20261104/type0-r9-r5/` preserves the
French render, original payload/disassembly, frozen pre-fix RTL, original and
retained red logs, candidate diffs, review interaction red/green and final
handoff. Sibling `type0-r9-r5-review/` preserves cross-provider review rounds.
Final validation on the frozen corrected revision:

- `make -C sim crtc-test`: **239passed, zero failed**.
- Final focused neighbors: **30cases, zero failed**, including R4 unequal and
  restoring writes, early R4/R9 entry, exact-R0 straddle, VMA, snapshot and
  live-type controls. Retained test ownership remains unchanged.
- `make -C sim soak`: **0x924e7251a6323f25**, seed0xaccc5eed20260822,
  2845088characters/CLKEN samples. Intended behavior change; stimulus and
  sampled projection are unchanged. The history top row replaces unpublished
  intermediate candidate hashes.
- `python3 sim/select_tests.py --run`: **PASS44benches**, once after the final
  code edit; complete final line is in `review3/FINAL-REPORT.json` and its raw
  log. Earlier candidates' green gates are superseded.
- Fresh guarded Claude Opus5.5/high review run20261104T142918Z-95243-1105:
  **ACCEPT**, no blocking findings. Reviewer read the full engine, wrapper
  ownership, source render and retained interactions without rerunning gates.
  Parent verified all three hashes match the final report: engineba67268f…ebe3f,
  sim9c027a6b…8c784, historyfea29c63…92614. No code edit followed the gate.

Pending-R5-zero counting and parity at the newly reachable delayed C4 step are
named unpinned source boundaries; they are not declared hardware-proven by the
main recipe. Any unequal R4 write during the pending phase cancels it; precise
outside-window hardware behavior still needs a discriminator. No wider rule
is inferred from the added state lifetime.

Parent's direct E6/CRTC0 before-run on7ac11a2 completed16commands/fourcaptures,
zero failures/nineSSM records. Parent inspected0407..040A; case21 on0408/0409
remains003A against printed0066. The last page warns R4/R9 without flash has
issues after switching. Full/French/current disk6b9 are pinned in its manifest;
private output is `resumed-20261104/device/e6-0-before-7ac11a2/`. Use the same
entry for targeted after-captures; do not substitute a broad baseline.

## Full artifact and targeted hardware result

Committed sourceee9c131e3bc59dc5ec06b644c0c0931cf75ff4a9 has a successful full
[build41849887000](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41849887000):
all simulation/lint, production T80, synthesis and required-gate jobs pass.
Quartus17.0.2.602 closes setup0.504ns/hold0.245ns across seven clocks, zero
TNS/errors/critical/implicit-net warnings. Artifact13335232700,
`Amstrad-build-352-1-full`, contains `Amstrad_20261104_ee9c131.rbf`,3121976bytes,
SHA256b6bb116d91c34450128738fd4d3d118ca4fb27426f15af29dd8ba8f8e8a462fe.
The MiSTer copy matches; provenance is in `resumed-20261104/type0-comparator-build/`.

Same direct E6/French/Full input and disk6b9 complete16commands/fourcaptures,
zero failures/nineSSM records. Parent inspected every image:

- Case21 changes003A→0066 on0408 and0409, matching the printed0066 and the
  independently established pending-R9 counter trajectory.
- Case24 changes06EE→000E, matching its printed000E withR7=39 at the delayed
  C4 increment. These are two distinct recipes; duplicated21 is counted once.
- 0407 is byte-identical to the before-run. Other numeric recipes stay at their
  previous values. Fourteen of28 still differ from program expectations, down
  from16; this is a **program self-report**, not a physical-photo failure rate.
  The new Last Line case5 and early-entry case16 are separate investigations.
- 040A changes display phase; its source warns about first-line flashing/R12/13
  after the switch. No additional numeric pass or exact-photo geometry is inferred.

A matched-script CRTC0 DSC4 regression run completes19commands/fourcaptures,
zero failures. Parent inspected all: four colons/names clear, static picture and
sampled main/rotating/fire-band region visible. This is bounded title acceptance,
not full demo completion. Evidence is `device/e6-0-after-ee9c131/` and
`device/dsc4-c0-ee9c131/` below `resumed-20261104/`. Prior ASIC acceptance remains
on275af8a; this change touches only classic Type0 rules and their tests.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
