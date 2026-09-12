# Current implementation status

**B4 CSL/SSM phases 0 and 1 implemented, 2026-09-12, branch `general/b4-csl-ssm`:**
`scripts/hardware-loop/csl_runner.py` drives a Logon System CSL v1.4 script on the
device, applying CRTC/model by CFG bit and restoring the file afterwards, and
`rtl/ssm_marker.v` recognises SHAKER's `ED LL ED HH` markers on the opcode fetch
stream and publishes them to a DDR3 ring behind OSD status bit 37, off by default.
`--ssm` gives the runner `wait_ssm0000` and `#FFFE`-driven captures named the way the
SSM standard suggests. `make -C sim` passes with 19 new SSM vectors; 97 host tests
pass. **Nothing has run on the device.** Two limits matter before it does: the DDR3
base `0x30000000` is the MiSTer convention rather than a measurement, so the first
`--ssm` run confirms it or `--ssm-base` finds the right one; and no synthesis has run
on this branch, which now drives previously constant DDRAM pins.

Phase 2 is now unblocked and not started: the author answered that `#FFFE` should
capture at the opcode from a framebuffer that is never cleared, which withdraws the
plan's earlier default, and separately confirmed that every SSM marker sits in a
stable zone, so every candidate capture semantics yields the same image for this
corpus. AMSpiriT images therefore remain a usable pixel-diff partner. What phase 2 is
actually needed for is the tests where SHAKER emits two markers a few frames apart to
record both phases of a flashing display: Main's asynchronous grab cannot serve those,
and the runner now marks such captures `state_uncertain` instead of presenting two
PNGs of the same phase. One `--ssm` device walk measures the real gap between a pair.
See [the plan](csl-ssm-implementation-plan.md), the
[driver guide](mister-hardware-loop-driver.md#the-csl-runner) and the two unreviewed
rows in [review debt](review-debt.md).

**Coordinated follow-ups integrated, 2026-09-12:** B2's accepted `5de8c5c`
establishes real SHAKER B (9) navigation and nine identical captures across
three independent loads; B6's accepted `c2fde66` closes the final-mixer RGB
simulation gap and adds dynamic raster/Plus layer gates. Sources were refreshed
without implementation changes to `cd08a1e` and `9849b9a`, respectively, and
merged sequentially into `master`. B2's 26 focused destination tests pass;
the complete merged simulation and lint gates pass, and soak matches
`0xb1cb70da95c2e44f`. Both tasks are published at `9ee710ce969aa3db7fb1540f7e96443bfd7c274c`.
[Exact-SHA CI](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34684600995)
is running: policy and routing passed; simulation and automatically selected
hosted full synthesis are pending. The local synthesis leg is correctly skipped. No new RBF exists. The user made a new build optional; no separate
build will be dispatched. The normal push workflow selects its required jobs.
See the [branch/evidence handoff](hardware-followup-handoff-2026-09-12.md)
for reviewed tips, device restoration and remaining hardware limits.

**B2 device capture, 2026-09-12:** French-ROM SHAKER 2.7 B (9) boots through
MGL/MBC on the hash-verified `5c16b17` RBF. Three independent loads produce
nine identical decoded numeric-screen PNGs with CRTC 1 visible. The reusable
case pins RBF/media identity; 26 focused tests and fresh Gemini review pass.
Native screenshots omit OSD, so saved Full configuration is not visual proof
of the active mode. See [device evidence and limits](b2-device-capture-2026-09-12.md).

**Latest hardware report, 2026-09-12, `5c16b17`:** the user confirms **6128 Plus
BASIC boot fixed**. Corrupt sprite lines at the left edge are **much improved,
perhaps fixed**, with definitive closure still open. **Pang/Plotting fire
always pressed and the Copter 271 logo remain NOT fixed.** B6 Full versus Raw
pixels shows **no visible difference so far** in Amazing Demo, DSC4 or SHAKER
A (T). Output configuration and Classic model/CRTC details are unrecorded.
See the [dated report](hardware-evidence-2026-09-12.md). The accepted B2/B6
follow-ups above supply capture and simulation evidence without broadening
these user-reported hardware verdicts.

**B6 rendering follow-up, 2026-09-12 (integrated source):** the production mixer
RGB scope defect has a failing-before/passing-after parameter regression.
The motherboard fixture now reaches the complete production output chain;
new CPU-written malformed-raster and combined Plus scroll/opaque-sprite
checks score final RGB. Fresh Gemini review and its correction check are
CLEAR; Opus did not review because authentication expired, and the user
subsequently requested no duplicate review. Full simulation, lint and the
unchanged canonical soak pass. No new bitstream exists. A sustained
CPU-generated stuck-high raw-sync recipe, physical connector acceptance and
title-specific defects remain open. See the
[completion record](b6-video-boundary-review-2026-09-11.md#rendering-completion-follow-up-2026-09-12).

**B6 video boundary integrated, 2026-09-11:** Full and Raw pixels now share
the complete Full acquisition tuple. Raw pixels uses native VRAM byte order
and aligned raw vertical blank in RGB; Raw CRT sends raw geometry through the
shared output with native cadence and core effects/crop disabled. The applied
mode commits during Full VBLANK at a CPU-owned byte boundary, with reset as
the escape path. Reviewed source `843cd5b` from
`codex/general/b6-video-boundary`, based on `20ed4d3`, is integrated into
`master`. Full simulation,
lint, canonical soak `0xb1cb70da95c2e44f`, fresh Sol/Gemini review and focused
negative controls pass. No `sys/` changes. Published merge `5c16b17` passes
all required CI and full-fit timing (setup +0.320 ns, hold +0.247 ns, zero
TNS). The delivered RBF is `output_files/Amstrad_20260911_5c16b17.rbf`.
The original artifact retains the then-open rendering limitations; the local
follow-up above adds that simulation evidence without changing this RBF.
Physical HDMI/CRT acceptance remains open; see the [design](b6-video-boundary.md) and
[validation record](b6-video-boundary-review-2026-09-11.md).

**Combined Plus integration timing, 2026-09-11:** D5 and D3/D4 are integrated
and published at `f8e9372`. All destination simulation, lint, canonical soak
and unchanged-CPR boot checks pass. The full Quartus fit failed setup at
−0.217 ns on the sprite render path. A behavior-preserving selector rewrite
passes exhaustive combinational equivalence, full simulation, lint and fresh
Gemini review. Follow-up `6a06ec9` passes all required CI and full-fit timing
(setup +0.368 ns, hold +0.228 ns, zero TNS). The exact artifact is delivered as
`output_files/Amstrad_20260911_6a06ec9.rbf`; hardware retests remain pending.
See [timing evidence](plus/d3-render-timing-2026-09-11.md).

**Accuracy repair integrated, 2026-09-11:** D1 uses the selected engine’s incoming
parity for origin VSYNC and retains active-pulse count phase; D6 uses shared
ParityC9 for RFD saves. The full simulation gate passes with 225 classic vectors, as do lint and
soak `0xb1cb70da95c2e44f`. Gemini high re-reviewed `988f5b9`, including the
post-Opus fixes and D6, and returned CLEAR with no actionable findings.
The rebased source tip is `2d04812`. The D1/D6 push skipped CI and synthesis at the user’s request to allow
subsequent Plus integration. That push produced no new RBF. SHAKER B (9) on both types and C (4) on
type 1 still await a hardware retest. See [repair evidence](accuracy/d1-d6-parity-repair-2026-09-11.md).

## 2026-09-11 D5 BASIC boot input repair

CPC Plus now supplies low `/EXP` for BASIC boot; MMU decoder polarity is
unchanged. The production-T80 regression requires actual firmware `Ready` from
both unchanged CPRs on 6128 Plus and 464 Plus and retains model/ROM controls.
See [D5 validation](plus/d5-basic-boot-input-2026-09-11.md) for results, review
and limits. Reviewed source `f0af3d6`, refreshed as `8e2f280`, is integrated
into `master` ahead of D3/D4. The September 12 user report confirms BASIC boot
fixed on 6128 Plus with `5c16b17`. Cartridge/empty-drive details and 464 Plus
hardware coverage remain unrecorded; disk I/O acceptance is separate.

**Earlier hardware retest, 2026-09-09:** `Amstrad_20260908_ce1d2da.rbf` was tested
on 6128 Plus / Live blanking and classic 6128 / CRTC1 (DSC4 also CRTC0).
Most defects persist. Burnin' Rubber is reported OK; CRTC3's earlier right-edge
sprite fix has not regressed, but warning/audio/crash defects remain. Navy Seals black
screen was not reproduced; left-edge sprite flicker remains, and the user
confirms no Dandanator was ever tested. System/BASIC fails before disk access.
DSC4 and SHAKER still fail; Amazing Demo has improved HSYNC behaviour with
Live blanking but remaining lower-screen corruption. See the
[full results and SHAKER capture index](hardware-evidence-2026-09-09.md).
B1/P10 remain open. The subsequent diagnostic assessment is recorded below;
unassessed entries do not become passes by association.

**Code diagnosis, 2026-09-10:** the [retained investigation](hardware-diagnosis-2026-09-10.md)
reproduces a type-1 frame-origin VSYNC parity bug matching SHAKER B (9)'s
`#4E00` interval, ASCAL geometry changes under split DE, a Plus sprite first-row
refill failure, and Plus PPI control-readback differences. Production RTL was
unchanged by that diagnostic pass. Local diagnostic sources, reference images and rerun logs are
preserved; the full existing simulation suite passes with its known FDC XFAIL.
The D1/D6 source repair is integrated above; the next Classic hardware work
is its SHAKER retest plus a first repeatable B2 capture. D5 resolves the
System/BASIC boot input in firmware simulation; the September 12 report also
confirms boot fixed on 6128 Plus. Disk-based testing and other model/cartridge
coverage remain separate, as described above.

**Plus D3/D4 repair, 2026-09-11:** the sprite predictor now stages row zero on
the preceding compare line under the [bounded first-row service contract](hardware-diagnosis-2026-09-10.md#d3-repair-contract).
The required production-cadence regression checks all 16 sprites enabled with
sprite 15 opaque at X=0/16/32/64/256, including distinct first/second-row colours.
Plus PPI mode-word control reads follow Thacker's `00`/`FF` bit-4 pattern;
DMA direction retention is checked through Port-A behavior before setup writes.
See the [D4 scope and BSR residual](hardware-diagnosis-2026-09-10.md#d4-repair-boundary).
Reviewed feature tip `5c58fe7`, refreshed as `cc9d704`, is integrated after
D5 merge `24b2520`. Both refreshes preserved reviewed behavior and resolved
only status wording; the Makefile retained both test targets. Destination
full simulation (225 classic vectors), lint and canonical soak
`0xb1cb70da95c2e44f` pass. The private D5 gate also passes with D4's PPI
change present: both unchanged BASIC CPRs emit `Ready` on both CPC Plus
models, and ROM0/ROM7/all direct-page controls pass on all three Plus models.
Fresh Opus 5 high feature review is CLEAR on correctness; no new review was
needed for documentation-only resolutions. Title/input/flicker symptoms,
post-BSR control readback and hardware acceptance remain open.

The batch makes one final integration push for full-fit CI, including the
previously integrated D1/D6 changes. Exact-SHA job results, reports and the
copied RBF hash are retained in the local ignored
`docs/references/d3-d5-integration-2026-09-11/` record. Source/test success
does not close the September 9 hardware findings.

**Hardware-tested baseline, built 2026-09-08:** `ce1d2da67c2598c0dd06208b9fc14c52ada01712`
passed simulation, policy, required gate and full local Quartus 17.0.2 in
[run 34228275825](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34228275825).
This includes the completed B8-5 snapshot restore plus production-T80 validation,
Plus FIELD ownership, SDRAM coherence, preserved B3/P10j work, palette ownership
and tape backpressure. Artifact `Amstrad-local-build-199-1-full`
(ID `10057418042`, `build_mode=clean_full`) is downloaded as
`output_files/Amstrad_20260908_ce1d2da.rbf`; SHA256
`fef2c8553e85456a86bc6d28753cdbb43c386106b5cd719bbf808cea84dc6144`.
Fit: 22,945/41,910 ALMs (55%); minimum setup +0.098 ns, hold +0.238 ns,
zero setup/hold TNS and no unconstrained clocks. Some I/O paths remain
unconstrained (27 input and 90 output ports). Reports are retained under ignored
`docs/references/snapshot-artifact-34228275825/`. Hardware retest is now recorded
in the September 9 report above; snapshot-specific acceptance remains untested.

B8-5 source `f2300be6ae2075dff41c6a96ec59f65286feb67f` restores mapped DMA,
selected video/GA, MMU and palette state after storage drain, holding CPU
execution through apply. All twelve focused cases, full simulation, lint and
the unchanged soak pass in both source and integration checkouts; fresh Astra
medium review is CLEAR. See the
[restore contract and acceptance evidence](plus/b8-5-snapshot-apply-2026-09-08.md),
including the documented failure-first chronology gap for the initial video/GA
implementation. First-frame pixels remain approximate where SNA omits internal
address/phase history; no hardware closure is claimed.

The prior `3acc8e6` RBF from successful run `34194119970` remains available as a
comparison baseline; it predates snapshot apply. Its hash is
`d847e542f49f545be739f4adacceb491f6f8ccd698bc349bb6eac04e2a4a8216`.

The preceding `b945c1b` build
[34192429871](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34192429871)
failed HDMI setup by 0.030 ns and packaged no RBF. Its retained reports under
`docs/references/b945-timing-failure-34192429871/` remain diagnostic evidence;
the successful descendant above establishes current timing closure.
The earlier `9052a08` RBF and reports remain available as a comparison baseline.

The completed T80, memory/tape, palette, preserved Plus and snapshot worktrees have been
removed after successful CI, idle/clean checks and verification that all private
non-build files and symlinks were preserved under `docs/references/retired-*/`.
Their branches and recovery stashes are retained. The snapshot cleanup preserved
and verified all 48 private non-build files/links. Only the integration checkout
remains. The requested task batch is complete; no further tasks were started.

**Earlier test/tooling integration, 2026-09-08:** `9d238dafc3a9950c276f91a3e928d5cfeb013bff`
passed simulation/lint, synthesis policy and the required gate in
[CI run 34188386354](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34188386354).
Its preceding preserved-work merge `5c201a8` passed the same required jobs in
[CI run 34187708364](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34187708364).
Both correctly skipped Quartus and reuse the `dba49d5` RBF below. The completed
FDC/test and host-tooling worktrees have been removed; their integrated branches,
review/build evidence and host tools remain available. Integrated branches and private recovery stashes are retained; snapshot cleanup
is recorded above.

**B2 original host tooling, 2026-09-08 (device result above):** accepted source `7e39204` adds the
[SSH/MGL capture driver](mister-hardware-loop-driver.md) and
[pinned ARM MBC cross-build](mister-mbc-cross-build.md). The integration checkout
passed all 24 focused tests and an offline dry-run; Opus 5 high returned CLEAR
in closure run `20260908T044131Z-35882-569d`. The ARM binary and cross compiler
are preserved under ignored `docs/references/b2-host-tools/`; the binary hash
is recorded in the build recipe. That original pass did not access MiSTer. The September 12 device record above
supplies real French-ROM navigation and repeated captures, with active-mode
observation limits preserved.

**Preserved FDC/test consolidation integrated, 2026-09-08:** refreshed source
`5fcf223` retains the separate histories of `c1a8ff9` and `c12c264`. The held-read
test checks payload bytes 0/1 and one-byte consumption across a three-CE strobe;
the B6 fixture now retains RAM-capability coverage while the duplicate model
test is removed. P1 remains a functional model, and the real-motherboard P1
fixture's constant-zero VRAM return does not establish physical return timing.
See [integration evidence](preserved-work-integration-2026-09-08.md).
Full-sector result/ST1, classic AMSDOS and hardware acceptance remain open.

**CI toolchain aligned, 2026-09-08:** CI and local testing now use Verilator
5.052 for consistent diagnostics. The installer and matching Ansible
provisioning variables pin official upstream commit
`ea338be98e1e838d3518809ce8899f85a009963c` (peeled `v5.052` release tag).
[CI run 34181169103](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34181169103)
passed the compiler build/check, full behavioral tests, lint and required gate
on exact pin-update commit `9483fb8269c5a9666688649486ef50c415d696fa`.
The installer-hash cache key selects the new compiler cache automatically;
source-marker and reported-version checks remain unchanged. Synthesis was
correctly skipped: this toolchain-only change does not alter the synthesized
inputs or the delivered B8 RBF below. The older-version waiver guard is retained.
Muse Spark/xhigh returned scoped CLEAR in guarded run
`20260908T024401Z-24341-f068` (clean exit and complete handoff), checking the
release ref, matching pins, cache/check contract and shell/YAML syntax. This
weaker-than-Opus source review did not install or execute the new CI toolchain.

**B8 integration artifact, 2026-09-08:** Accuracy merge `35ae03b` and Plus
merge `dba49d5` are pushed. Full-effort Quartus 17.0.2 on exact source
`dba49d57ab2e3e2f024b070bd6efa023073d902f` passed in
[run 34179032281](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34179032281),
job `synthesis-local / quartus`; artifact `Amstrad-local-build-190-1-full`.
ALMs: 22,306/41,910 (53%); constrained setup/hold minima +0.063/+0.241 ns
across seven clocks, zero TNS. External I/O remains partly unconstrained
(27 input and 90 output ports); this is not full board-interface timing proof.
The downloaded `output_files/Amstrad_20260908_dba49d5.rbf` has SHA-256
`64a13ce721b5d5323e984e5fd4e30b78a204e9253c25252acec9f4c53b6c7870`.
Reports and `clean_full` provenance are retained under ignored
`docs/references/b8-integration-2026-09-08/quartus-dba49d5/`.

The first run's behavioral tests passed on pinned Verilator 5.050, but its lint
and aggregate gate failed on unknown `SIMILARNAME` control-file rules. Reviewed
build-only correction `9cfe743` preserves the CI pin and loads the five waivers
only on 5.052 or newer. Its corrected
[CI run 34180289243](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34180289243)
passed simulation, lint, synthesis-policy and the required gate on exact
`9cfe743a79fb896ddf8b80b7b8750759a9e46452`, using pinned Verilator 5.050.
Changed-path classification from `dba49d5` to `9cfe743` is false;
the RBF above is reused with its original source identity, not relabelled.
Local combined simulation/lint and soak `0x6e8258198d6e6137` pass on 5.052.
No DSC4/SHAKER, named-title or real-hardware closure is claimed. Task worktrees,
private evidence, older READY branches and both stashes are retained.

**Accuracy B8-1 integrated, 2026-09-08, from `c1d2add`:** the repair
retains qualified R5/R0 events from system-clock register capture to the CRTC
character decision. Source `8c29248` has scoped Gemini/high and requested
Muse Spark/xhigh clearance (both weaker than Opus); the separately reviewed
build-only lint delta is `d52152a`. Full simulation, aggregate Verilator 5.052
lint, and soak `0x6e8258198d6e6137` pass. Its production-GA fixture fails 20 cases on unchanged
`d46609d` and passes all 45 after repair. R6 RTL remains unchanged: sticky
C4=R6 and temporary R6=0 border are distinct controls. See the
[timing contract, gates, and review status](accuracy/b8-1-cpu-write-timing-2026-09-08.md).
Executed production T80 validation is integrated from `84f3106`: four OUT(C)/OUTI
cases pass through the real GA/CRTC boundary and fail their named side effects
with historical rule engines. Fresh optional GHDL checks, full simulation, lint
and the unchanged soak pass on integration. Review is scoped clear (Opus plus
Astra closure of the Gemini-authored timing remediation). See the
[bounded CPU evidence](accuracy/b8-production-t80-2026-09-08.md). Dynamic-wait
native-VHDL equivalence and full motherboard execution remain open.
September 9 hardware captures show DSC4/SHAKER still failing; acceptance
remains open. CI and artifact delivery are tracked separately.

**Shared B8-7 integrated, 2026-09-08, from `e7d73ba`:** tape download
writes retain their address and payload through synchronous completion.
Backpressure, early write acknowledgement and drain-held playback reset
prevent lost or duplicate writes; Fn[2] clears metadata without redirecting a
queued write, and machine reset dominates a simultaneous strobe. Physical-DQ
regressions fail before repair and pass afterward; Astra independently cleared
the repair and its two review corrections. See
[tape evidence](b8-7-tape-write-lifetime-2026-09-08.md). Actual HPS cadence,
player reset and real-CDT playback remain hardware/source boundaries.
The merged checkout passes full simulation, lint and unchanged soak
`0x6e8258198d6e6137`.

**Plus B8-3 integrated, 2026-09-08, from `807f081`:** each accepted legacy
palette write now reaches the 12-bit palette, including repeated same-value
pen and border writes after direct ASIC-page edits. One-time reset import
preserves retained GA colours without replaying idle shadows over later page
writes. The failure-first motherboard regression includes ASIC-only reset;
Gemini independently cleared the frozen Muse-authored change. See
[palette evidence](plus/b8-3-palette-events-2026-09-08.md). Scripted-bus evidence
does not establish Copter causality, executed-title or hardware acceptance.
The merged checkout passes full simulation, aggregate lint and unchanged soak
`0x6e8258198d6e6137`.

**Preserved B3 capture integrated, 2026-09-08, from `bb77075`:** the bounded
P10 frame-capture CLI and P10j contract notes are restored after the SDRAM
repair. Original Gemini implementation review plus fresh merge-compatibility
review are clear. Capture proves repeatability of synthetic frames with the
reduced TV80 and fixture clocking; it is not a hardware/title oracle. See
[preserved-work evidence](preserved-work-integration-2026-09-08.md).

**Shared B8-4 integrated, 2026-09-08, from `7a58f88`:** retained SDRAM
video words are keyed by physical word address and bank, invalidated by accepted
writes and initialization, and refetched without requiring a raster-address
change. The physical-DQ fixture and a production motherboard byte-to-pixel
witness fail on original RTL and pass after repair. Astra medium cleared the
foreign-authored repair and the corrected pre/post-edge assertions. See
[coherence evidence](b8-4-video-coherence-2026-09-08.md). The merged checkout passes full simulation, lint and soak
`0x6e8258198d6e6137`. Hardware/title acceptance and dedicated cartridge/tape
invalidation vectors remain separate.

**Plus B8-2 integrated, 2026-09-08, from `55151a0`:** the selected ASIC now
owns Plus FIELD; classic FIELD remains selected in classic mode. The production
interlace-history consumer is shared with a motherboard fixture. Eleven cases
pass, including R8=1/3, R7=0 association, entry/exit and witnessed filtered-VSYNC
negative controls; the historical replay exposes the original leakage. Opus
cleared the functional repair and Gemini cleared its corrected test/evidence
scope. See [FIELD evidence](plus/b8-2-field-ownership-2026-09-08.md).
The merged checkout passes full simulation, lint and soak
`0x6e8258198d6e6137`. Full ASCAL, Quartus and hardware acceptance remain
separate gates.

**Plus B8-6 integrated, 2026-09-08, from refreshed `2bb75b5`:** branch `codex/plus/b8-6-colour-alignment`,
originally accepted at `023d020` from base `d46609d`, contains the reviewed
colour/CE extraction (`f08a3ca`) and Plus RGB/metadata alignment fix (`51b61e3`).
It is refreshed onto Accuracy integration `35ae03b` with only shared manifest/doc
reconciliation. The original Plus Verilator 5.052 sim/lint and soak passed;
combined integration simulation, lint and soak pass with Accuracy's expected
hash `0x6e8258198d6e6137`. Opus/high source review is CLEAR; the separately
reusable build-only lint compatibility commit `f12f998`, already present as
`d52152a`, has supplementary Gemini/high source/log review with lower-confidence scope recorded.
[Evidence and residuals](plus/b8-6-colour-boundary-2026-09-08.md) distinguish
the CI-pin compatibility correction, full vendor HQ2x/freeze/top/T80 coverage,
and hardware/title retests. CI and artifact evidence are separate from local
acceptance; no hardware verdict is claimed.

**Architecture/methodology review, 2026-09-08:** the user-authorized B8 first pass
is complete against source `65364ee`; see
[findings and repair order](b8-architecture-methodology-review-2026-09-08.md).
The strongest accuracy finding is that CRTC old-value side effects, including
type-1 RFD, miss production CPU write phases while direct-edge tests pass.
Additional Plus video/palette/restore and shared-memory defects have local
reproductions. These justify focused failure-first repair work; they do not
establish the cause of a named hardware symptom. The clean baseline suite passes.
The [SSH hardware-loop plan](mister-hardware-loop-plan.md) is ready for a first
stable-screen capture once device access is supplied. No production RTL, pending
branch, RBF or hardware verdict changed during this review.

**Workflow update, 2026-09-07:** manual start/orchestrate/finish now use ad-hoc task worktrees,
including general/auto scope and adding tasks to an existing run. Start provisions local
reference PDFs; finish retains push-by-default and CI/artifact gates. See
[task workflow](task-workflow.md). Existing fixed checkouts and pending hardware work below
are preserved. Host dispatch/adoption/cleanup smoke tests remain open; this workflow update
adds no RTL, synthesis artifact or hardware evidence.

**Continuation, 2026-09-03:** local testing-policy candidate `c12c264` and shared FDC
held-read test `c1a8ff9` are committed on isolated branches with simulation/lint passing;
Plus READY remains `bb77075`. The FDC/test candidates are now integrated through
the September 8 refresh above; B3 remains pending. The uncommitted CPU-model candidate
passes its focused test and lint but makes P10 fail at an obsolete XFAIL because all 512
payload bytes now match; seven result bytes still need phase-verified acceptance.
Claude hit its session limit and the single authorized retry failed, so implementation
stopped as instructed. B10 has no code changes. Exact branches, recovery files, evidence,
review debt, reference preservation, and repair backlog are in
[the continuation handoff](session-continuation-2026-09-03.md).

**CPU-candidate recovery check, 2026-09-08:** the old temporary worktree contained
only empty directories, with no staged source or additional commit in its retained
Git metadata. Its uncommitted source and documented recovery patch were not found
in the checked repository and temporary paths. The empty temporary worktrees have
been cleaned up; branches and stashes remain. The review-debt branch's patch was
already integrated as `22ad766` (matching patch ID). The CPU branch still names
its original base, so the September 3 candidate results are historical, not a
currently runnable checkout. The FDC stash and main `.fdc-scratch/` evidence remain
present. Check retained records/backups before attempting to resume the candidate.

**September 3 handoff source tip: `a8286bd` (docs-only B6/B10 wording/comments). This handoff
is docs-only and adds no new validated source.** The Plus READY tip below is
**not** on main: serial integration, CI/gates, and artifact handoff remain
pending separate authorization.

**Earlier hardware report, 2026-09-02 (superseded for current results by September 9 above):** on `Amstrad_20260901_84e6969.rbf`
with Live blanking selected, the user reports that
`amazingdemo_sync_live_blanking` **appears fixed** and
`burnin_rubber_sprite_on_the_right_should_be_hidden` **is fixed**. Other
Plus defects remain **TBD**. DSC4 and SHAKER are still failing, possibly with
changed failure shapes; captures were unavailable then (September 9 captures are now indexed above). These are symptom-specific hardware observations, not
B1/P10 closure or proof of the individual cause. The
[dated report](hardware-evidence-2026-09-02.md) records artifact identity,
evidence limits and the authorized FDC/Plus/review scope. Accuracy RTL work is
deferred pending discriminating captures. No new screenshots, RBF, CI, or
hardware closure in that session. B8 was subsequently authorized and reviewed
on September 8; see the current review above.

Shared FDC state is the accepted observe-only diagnostics at `d3aabbc`
(`docs/fdc-recovery-2026-09-03.md`, independently reviewed CLEAR in
`docs/fdc-diagnostics-review-2026-09-03.md`): passive taps plus a print-only
DIAG block, XFAIL byte-identical in strength. Boundaries retained: the
**classic AMSDOS full command/data regression is required and unmet** (the P10
harness runs a synthetic unrolled sequence, not an AMSDOS ROM boot); the
reduced-TV80 surrogate never executes `JR`/`JP cc` conditionally
(`jump_e` unconnected, `Jump_r` forced), so the old polling fixture proves
nothing about the controller; the original `fdc-payload-poll` XFAIL stays.
Stash `0fe18a4513a47e4f21e0f504f002673a853388c3` is intact; B3 capture is
recovered only on the Plus branch; the six stashed u765 pre-edge tests remain
unaccepted and unapplied. The failed FDC experiment is preserved under
`.fdc-scratch/` (untracked scratch, not committed evidence).

**Independent-review pass recorded 2026-09-03, all at committed `a98590a`
(NOT concurrent FDC work).** Round 2 consequence/B1 hybrid blanking, OSD
sync-filter wiring, B9 archive/bookkeeping, B3 frame-harness foundation, B6
menu visibility, B7 dark-silicon audit, and the Plus hardware-defect triage
are review-CLEAR on source/test with hardware, full-T80/top, and
model-oracle residuals retained as validation, not debt. Prior Opus CLEARs
for B6/B10 (2026-09-02) are recorded without re-review, with B6-1/B6-2 and
B10-1/B10-2/B10-3 doc follow-ups retained. **P10j is independently CLEARED
on the prepared Plus branch but that commit is not on main:** `3db81d0`
(comment-only sprite-RAM/asic_regs invariant notes, Gemini 3.8 Flash high
CLEAR, source-verified SNA-drain/CPU-reset chain) awaits serial integration
with the B3 capture commit `bb77075` (see READY paragraph below), so the
pending-integration row remains until integration lands; zero unresolved
review findings. IA rows already
cleared retain their scoped evidence; the B8
architecture audit was outside that September 3 review. A CLEAR verdict is not
hardware closure, and comments not yet integrated are distinct from the many
retained validation residuals. Records: `accuracy/classic-review-2026-09-03.md`,
`plus/plus-review-2026-09-03.md`,
`plus/b3-frame-harness-review-2026-09-03.md`,
`b6-b10-review-2026-09-02.md`; ledger: `review-debt.md`. No simulation was
run for this docs-only pass. Published ACCC v1.11 remains unchanged; the
author message is dated clarification only.

**Plus READY (pending integration, NOT on main): `bb77075` on
`plus/b3-capture-recovery`, based exactly on main `a8286bd`.**
Range-diff proves the two reviewed patches unchanged since review
(`d52df41`=`3db81d0` P10j comments, `1e2fb3e`=`bb77075` B3 capture); final
parent `make -C sim` and `make -C sim lint` on exact `bb77075` both exit 0
(2026-09-03). B3 adds a bounded steady-state frame capture over the real P10
cartridge path with fail-closed `--capture-cpr` CLI, atomic-exclusive output,
self-equality (not hardware-oracle) evidence, and the reduced-TV80 opcode
boundary honestly recorded. Independent Gemini 3.8 Flash high review is CLEAR
with one non-blocking test-tightening suggestion (assert the dangling-symlink
target/link state on the filesystem; refusal itself already passes). Inspect
without checking out Plus paths, e.g.
`git show bb77075 --stat` from a clone containing the Plus branch. No
integration, merge, push, CI dispatch, or new RBF this session. A B10-1
proper wrapper coupled test remains a residual without source-scraping; the
symlink postcondition suggestion is non-blocking and recorded in the dated
handoff, not a gate.

**Next session: read `docs/backlog.md` first.** A 2026-08-31 methodology review concluded the
project's bottleneck is observability rather than implementation quality, and opened a
prioritized cross-cutting backlog (B1-B12). Its top item identified why classic CRTC sync work
could produce no visible hardware change: the original `rtl/crt_filter.v` Live route reduced
HBLANK to the shaped sync pulse. A simulation-gated hybrid now preserves raw horizontal phase
for Live HBLANK while retaining regenerated scaler sync; the first hardware A/B rejected that
candidate as sufficient.

The earlier 2026-09-01 hardware A/B rejected that Live route as sufficient: the supplied
Amazing Demo and Pulpo captures are materially narrower than Full, and DSC4/SHAKER remain
incorrect. The exact-expiry and watchdog repairs below harden fallback behavior but do not
establish a steady-state hardware improvement by themselves. The 2026-09-02 report above
supersedes the Amazing Demo verdict for the named build; Pulpo awaits retest.
DSC4/SHAKER still fail, but the changed failure shapes are not yet characterized.
B1 therefore remains open as an ownership/observability problem.

B6's architecture pass is complete in `docs/b6-architecture-decision.md`. Runtime clock/write
gating is rejected for now because it cannot reduce fitted resources and is unsafe against the
current non-atomic model/reset transition. The first bounded slice conditionally hides Plus-only,
classic-only, FDC, and tape menu entries from the selected model's capabilities while preserving
all status encodings and hidden values. Its focused four-model mask fixture, full simulation, and
lint pass. `Reset & Detach Cartridge` is also restored to its original Dandanator-only scope;
the Plus CPR image can now be replaced only through its atomic loader/reset lifecycle. MiSTer
OSD rendering remains a hardware/UI confirmation rather than local proof.

B3's first whole-core-frame-harness foundation is also live in the existing P10
fixture. The previously disconnected SDRAM video client now receives the real
motherboard VRAM address, a failure-first cartridge program pins changing
cross-module requests through the physical SDRAM ACTIVE/READ commands, and raw
motherboard timing, selected monitor timing, and shared filter-dependent payload
taps are available for the next capture slice. The fixture still has no runtime
CPR-to-frame writer and is not an image oracle.

B10's safe production ROM-loader destination decoder prerequisite is complete
in `rtl/rom_loader_route.v`. It extracts destination decoding from `Amstrad.sv`
without changing accepted-write behavior or invalid-chunk state retention: the
index-zero ten-chunk boot bundle mapping (6128 OS/BASIC/AMSDOS/MF2, 664
OS/BASIC/AMSDOS/MF2, 464 OS/BASIC),
index 7 forced bank 2 route, generic nonzero index bank selection including
inferred 0x40/0x80/0xC0 auto routes, and separate bank-zero to bank-one
second-write promotion. Its dedicated deterministic simulation suite in
`sim/plus/` checks named policy-significant cases and exhaustively compares all
indices/chunks over representative pages with a frozen model of the former
inline decoder. No locale assets or visible menu
states are introduced, preserving the documented asset/policy blocker.

The exact B10 source/QIP tip `44eb1b782a21312c3447b3170eb0b168e7e23713`
also passes GitHub Actions run `33482930072`: the required simulation gate and
full-effort Quartus 17.0.2 Build 602 synthesis both succeeded, with setup
+0.511 ns, hold +0.248 ns and zero TNS. Fit is 22,513/41,910 ALMs (54%),
26,417 registers, 701,596/5,662,720 block-memory bits (12%), 102/553 RAM
blocks (18%), and 35/112 DSPs (31%). Artifact
`Amstrad_20260901_44eb1b7.rbf` has SHA-256
`d73f355a5ff3843cc4e4485921a1154e136ecf353e439ccfbb5816ccaf65a235`
and is retained under `output_files/Amstrad-local-build-186-1-full/`. This is
exact synthesis, timing and packaging evidence, not a ROM-locale decision or
hardware confirmation.

## 2026-09-01 combined accuracy/Plus integration artifact

Source-bearing integration SHA `ea0e0bd4a0c2557f6cce2c0e1e60b84d389bf101` combines the
Round 2 author consequences and hybrid-blanking diagnostics with the Plus hardware-defect
triage. Full simulation, lint, manifest checks, and soak `0x2263c9fc44af4ee7` pass. Local
full-effort Quartus run `33474427903` (Quartus 17.0.2 Build 602) closes setup at +0.451 ns and
hold at +0.242 ns with zero TNS. Fit is 22,517/41,910 ALMs (54%), 26,238 registers,
701,596/5,662,720 block-memory bits (12%), 102/553 M10Ks (18%), and 35/112 DSPs (31%).

Artifact `Amstrad_20260901_ea0e0bd.rbf` has SHA-256
`02665b2ae907591aa858e26970da6218a2b822c834af9595811085880309fef7` and is retained with
reports under `output_files/Amstrad-local-build-11-1-full/`. This is simulation, synthesis,
timing, and packaging evidence—not hardware closure. Priority hardware retests are Live
blanking/DSC4/SHAKER, BASIC/System CPR FDC payload, Dandanator-to-Plus isolation, and the
signed/offscreen sprite plus display-origin/SSCR seams.

The later B6 source-bearing tip `7618e39a17739963968cc77206421eb59e1eb029`
(conditional menu visibility, Dandanator-only detach, and the standalone
CI-portable menu helper) also has an exact full-effort artifact. Local-runner
workflow `33479031612` passed simulation, lint, synthesis policy and Quartus
17.0.2 Build 602 with setup +0.601 ns, hold +0.247 ns and zero TNS. Fit is
22,375/41,910 ALMs (53%), 26,440 registers, 701,596/5,662,720 block-memory
bits (12%), 102/553 RAM blocks (18%), and 35/112 DSPs (31%). Artifact
`Amstrad_20260901_7618e39.rbf` has SHA-256
`31b03baeae09d45d72096a8d3e07bb736cd427c36cb208dc850eab323e15097d`
and is retained under `output_files/Amstrad-local-build-184-1-full/`. This
supersedes the earlier RBF for B6 hardware/UI checks; it does not change the
recorded Live-blanking hardware verdict.

This is the handoff for the next development and hardware-test session. The newest partial
hardware observations were reported on 2026-09-02 above; older milestone narratives below retain their
own dates and are not evidence that later work was hardware-confirmed. The
`accc-review-and-fixes` branch now contains the ACCC review/corrections, per-type classic CRTC
split, F6 Stage 1 full-character approximation, sampled-field soak expansion, production Plus
P0 cartridge wiring, the simulation-only P1 CRTC3 foundation, the implemented F7/A1/A2
classic work, F14/F15 classic closures, and the F16/F17/F18 closures (2026-08-26). On 2026-08-28,
Longshot released **ACCC v1.11** incorporating feedback from our Round 1 audit
(`docs/accuracy/accc-author-feedback-round1-2026-08-27.md`). Active Round 2 feedback is tracked in
`docs/accuracy/accc-author-feedback.md`. The author's 2026-08-31 response describes intended
future corrections; the published v1.11 PDFs remain unchanged and no corrected full edition
exists yet.

Q12 is resolved by French v1.11's repeated-activation qualifier and becomes an English
clarification. The author confirmed Q20's row-only C4 reset while R5=0 adjustment and C5 remain
active; that behavior is now implemented and independently checked locally, while hardware
confirmation and the separate ParityC9 residual remain open. Neither is new hardware evidence.
A complete mechanical and multimodal comparison across all 295 pages (278 word-identical, 17 updated)
resolved the author questions/errata, confirmed that our F15–F18 implementations match the corrected
rules, and evaluated Finding **F19** (clarified as CRTC-2 specific §12.4.1 p.95, while CRTC 0 is confirmed to evaluate same-edge writes per §12.2 pp.92-94).
Full diff and impact reports are in `docs/accuracy/accc-1.11-differences.md` and
`docs/accuracy/f19-type0-c0-timing-todos.md`, with the repeatable process in
`docs/accuracy/accc-update-procedure.md`.

Settled pass-by-pass review records are indexed in the
[accuracy archive](accuracy/archive/README.md) and
[Plus archive](plus/archive/README.md); their cleared verdicts remain summarized in
`docs/review-debt.md`. Those archives preserve provenance, not current status or hardware
evidence. The 2026-08-31 IA-1 through IA-6 source/model audit and 2026-09-01 Round 2
consequence/B1 work are also integrated: 192 classic vectors and all nine focused hybrid-
blanking vectors pass, and the current soak hash is `0x2263c9fc44af4ee7`. The author-response
work corrects Q20, type-0 short-line VSYNC qualification, type-1 interlace isolation, and
snapshot/live-type lifecycle reconstruction. `t02o` remains explicitly a model inference and
hardware discriminator. IA-5 retains a real-hardware U.S.-ROM phase capture rather than a
circular synthetic oracle; the hybrid blanking path also remains hardware-test pending.

Plus P10j moved the sprite-pixel array from soft registers to exactly two M10Ks. Exact
feature build `c047a7d` (run `33392854459`) reports 22,057 ALMs (53%), 16,384 sprite-RAM
bits, setup/hold +0.323/+0.251 ns, zero TNS, and a packaged RBF. One guarded Gemini 3.7
Flash high lane reviewed the architecture and CI work through `f16020e`. A later guarded
Claude Opus 5 high review at integrated SHA `bf1e785` found no defect in the final two
Quartus-compatibility corrections; three low model/invariant documentation and primitive-
stub coverage notes keep the narrow row open. This fit is synthesis evidence, not hardware
confirmation. The detailed behavioral rules remain in `accuracy/`; the long-term ordering
remains in `implementation-roadmap.md`.

## How hardware testing fits the loop

SHAKER is **not** part of the automated loop. The automated loop is the Verilator suite
(`make -C sim`) plus GitHub Actions synthesis. SHAKER sessions are manual, user-run, and
happen only at significant milestones, against a named target list recorded before the
session. A green simulation gate is never evidence of hardware accuracy; a manual session
never gates a commit.

## 2026-08-30 Plus hardware round three — integrated source-backed repairs

Build 165 contained the round-two Plus RTL and produced no obvious Plus improvements; build
166 is functionally equivalent for Plus. Those observations keep every reported title symptom
open. The integrated `plus/p10-hardware-round3` work adds two independently bisectable,
simulation-verified repairs:

- CRTC3 R8=1 now follows ACCC v1.11 sections 19.6.4 and 19.7.3: the outgoing even-parity field
  gets the documented additional line and midpoint VSYNC, while the alternating field has no
  additional line and starts VSYNC at the seam. The focused fixture first observed seam=2,
  midpoint=0 instead of one of each, then pinned C4/C9/VMA/RA/DE, both field lengths, R4=0
  entry, R9=0 exit, and R9=0/R5-nonzero adjustment sequencing.
- Production DMA/PPI/PSG concurrency now preserves an already-accepted CPU transaction and
  its read result, blocks new PPI transactions during DMA ownership, classifies physical PSG
  writes, and implements bounded 8/9/10-CCLK LOAD duration. The full motherboard fixture
  first failed the PSG discriminator at 9 rather than 10 CCLKs, then also caught duplicate
  acceptance, a corrupted pre-owner AY R14 keyboard read, and an early-frozen uncontended
  Port B read. It now passes 24 LOADs, 80 PPI transactions, and 103 CPU operations; the
  maximum observed wait is five CCLKs.

These fixes make Pang/CRTC3, Arnold 5 keyboard, Plotting held Fire, and DMA sample pitch
direct hardware retests; they do not close those titles. Sprite top-row/colour/positioning,
Switchblade and other cartridge crashes, CPC+ SNA/reset/reload recovery, and undocumented
odd-R5 CRTC3 behavior remain evidence-gated. The motherboard WAIT/PPI timing change requires
an exact full-effort synthesis before hardware testing. Full evidence, gates, and residuals
are in `docs/plus/hardware-test-round3-2026-08-30.md`.

Accuracy tip `683fcaf3afab672c9bec85c43066292eb9f6bf75` adds a real production-timed u765
READ DATA/EDSK seam and closes the demonstrated late-ACK/reset-reload alias in simulation,
not hardware. Its tracked track-0/head-0/R=`&41`/N=2 case resets while sector LBA1 is
outstanding, retains cancelled ownership through the old ACK rise/fall, gates reset/cancel
buffer writes, reloads metadata, and verifies all 512 bytes. No `Amstrad.sv` or Plus decoder
change is indicated; BASIC hardware closure remains open.

## 2026-08-30 Plus hardware round two — focused simulation follow-up

The second hardware report shows substantial CPR-loading progress but keeps P10 open. BASIC
cartridges still report `disc missing`/`read fail`; Switchblade remains black; Pang reaches
level select then crashes; Plotting reads Fire as held; Arnold 5 has no keyboard; Copter 271
and other titles show sprite top-row/colour defects; Burnin' Rubber exposes columns of an
initially hidden sprite; and the CRTC3 demo still detects an emulator, has sample-pitch and
picture corruption, then crashes. Some reload failures require reloading the whole core.
The tested RBF and media/model metadata were not recorded, so these remain hardware symptoms,
not commit-specific causality. The full symptom mapping and retest order are in
`docs/plus/hardware-test-round2-2026-08-30.md`.

The rebased `plus/p10-hardware-test-round2` follow-up provides these simulation-verified
changes and discriminators:

- CRTC3 R8=3 now implements IVM `+2` counting, adjustment parity, the even-frame additional
  line, MID-VSYNC, odd-frame delay, R7=0 priority, live writes, reset, exit, and the DE/ADJ/VMA
  consumers. R8=1 sync-only interlace and odd-frame R5 recurrence remain explicit residuals.
- DMA skips inactive channel fetch and execute slots; all eight channel masks are pinned for
  both phases. This is a direct retest candidate for the reported sample pitch, not hardware
  confirmation.
- The production-shaped PPI/YM2149/hid/joydb fixture passes PS/2 A, SNAC Fire 1, and USB Fire 1
  under real ASIC phase/READY timing. It excludes DMA ownership, so Plotting and Arnold 5 are
  not attributed or closed.
- The SNA FIFO reserves the checked two-byte post-wait producer tail. The local seam injects
  that bound but still does not elaborate `hps_io` and `Amstrad.sv` together.
- Sprite regressions now overlap all 16 live-updated sprites at one early window and connect
  real `asic_regs` storage to delayed sprite arbitration, stale-ACK rejection, re-demand, and
  modeled access blanking. These validate the implementation model, not undocumented hardware
  bandwidth, sprite coordinates, or access-hole duration.
- The production boot harness pins a 4,096-tick sustained cartridge window: 38 M1 fetch phases,
  73 physical reads, 803 WAIT-low ticks, and an 11-tick maximum stall. Its hardwired no-wait
  ordinary-RAM side cannot provide a like-for-like speed comparison, so no cache redesign is
  justified yet.

Shared integration `074c182` also adds reviewed u765 reset/ACK quarantine, mount-retry
retention, and selected-write aliases. Accuracy follow-up tip `457a3b4` adds a production-timed
public-command READ DATA transaction against a copyright-free two-track EDSK, including exact
LBA/payload checks and reset during an active sector request. It retains cancelled ownership
until the old ACK rises and falls, quarantines that response's buffer writes, and prevents
metadata reload from falsely completing behind the cancelled sector owner. This is a direct
BASIC/System retest candidate, not title-level or hardware closure. A host that never ACKs now
requires a power-cycle; safe bounded recovery needs an epoch/tag, while cross-drive sector
arbitration, sector-info reset, WRITE DATA reset, and automatic-EOT C/R remain focused fixture
gaps. None of these post-hardware-report changes has been hardware-tested.

## 2026-08-29 Plus cartridge checkpoint — historical P10 baseline

P0-P9 and HF-1/HF-2/HF-3 are implemented and simulation-verified, but the first broad
post-P9 cartridge sample does **not** support calling the Plus implementation complete in a
hardware or full-system sense. Approximately half the tested cartridges loaded. Panza Kick
Boxing stopped at a grey active area with blue border; RoboCop 2 started with garbled
sprites; Arnold 5 loaded with an inoperable keyboard; BASIC/System cartridges displayed the
copyright banner and later reported `Drive A: read fail`. The tested RBF identity, Plus
model, and mounted-media state were not recorded, so preserve these as checkpoint symptoms,
not commit-specific proof.

The exact findings, confidence boundaries, required discriminators, and checked todo list are
in `docs/plus/hardware-checkpoint-findings.md`. P10 implemented the first PPI, model/FDC,
DMA-arbitration, SNA-parser, and sprite-write remediations, but the 2026-08-29 independent
review found that several claimed production tests were leaf tests or copies of production
equations. The review is preserved verbatim in `docs/plus/p10-independent-review.md`; its
debt remains open. The hardware observations themselves, future model/media/ROM UX work, and
simulation-versus-hardware automation options are preserved separately in
`docs/plans/2026-08-29-real-hardware-session.md`,
`docs/plans/2026-08-29-plus-ui-ux-architecture.md`, and
`docs/plans/2026-08-29-verification-automation-research.md`.

The integrated Plus remediation checkpoint (feature tip `1d1795b4`) contains focused
simulation fixes for five hardware-correlated defects:

- one shared classic/Plus FDC decoder restores the upstream classic A10/A8/A7 partial decode
  while retaining 6128+ `&FADD`/`&FBDF` aliases and model gating;
- the CPC+ SNA path captures each accepted payload byte with a one-cycle strobe, keeps the
  parser and ASIC register loader live while the ordinary machine is reset, drains the FIFO
  and final registered tail before apply, retains RMR2/unlock through that apply, and clears
  retained state plus any residual prior FIFO/write tail when a later snapshot starts;
- the authoritative ASIC lock state now separates locked `101xxxxx` MRER writes from
  unlocked RMR2 writes in both the cartridge MMU and Gate Array path, and the classic
  onboard-ROM path is suppressed in Plus mode; this is the confirmed source-level mechanism
  for AmstradDiag content appearing during Burnin' Rubber relocation;
- sprite fetches remember a same-sprite access for the whole delayed request, and SSCR
  vertical wrap advances the video row base before the wrapped RA=0 line;
- the status-2 16-frame timer now shares the selected C4/C9/C0 frame-origin condition with
  the video-pointer reload it mirrors; ACCC v1.11 §20.3.4 p.243 also gives a later C4/C0-only
  form, so the source conflict and hardware confirmation remain open.

These changes pass the complete Verilator suite, lint, and the classic golden soak. A second
independent review found the intended four remediation paths functionally correct but returned
NOT CLEAR on three small integrity blockers and the frame-timer mismatch; the blockers and
timer now have focused fixes. A later final scan then found and locally remediated three HIGH
production SNA lifecycle defects: writes discarded by ASIC-register reset, registered strobe/
data misalignment or repetition, and apply racing the undrained FIFO/tail. The new `p8_04`
test elaborates the production parser, `asic_regs`, and MMU and covers backpressure, the final
post-download strobe, a rapid new snapshot before the prior FIFO drains, register/palette/
sprite restore, shadow apply, and ordinary reset. It is still not a full `Amstrad.sv`
elaboration, so the top-level reset/strobe equations remain a
Quartus and hardware boundary. This expanded post-review delta is not independently cleared,
synthesized, or hardware-confirmed. The report and remediation status are in
`docs/plus/p10-hardware-remediation-independent-review.md`. RoboCop and Navy Seals causality
therefore remain retest hypotheses. A bounded real-u765 bench now covers CPC
write aliases, EDSK mount/status, and reset during an active host transfer with
delayed ACK/buffer traffic; BASIC/System CPR remains open because no bench runs
the full CPU boot and READ DATA path against that controller. DMA/PPI concurrency, a
cartridge-versus-RAM/title timing comparison, PRI monitor-HSYNC timing, CRTC3 R8=1 sync-only
interlace and odd-frame R5 recurrence, SSCR raw-bit horizontal shifting, and the
first adjustment-line split also remain open.

The only current local RBF is the smoke-fit `7c46b8d` artifact
`Amstrad_20260829_7c46b8d.rbf` (SHA-256
`895bd0491e9ff249295bab27b091870ddf5728961b97b07ee1129f1316faad86`): 82% ALMs,
main setup slack **-0.796 ns**, TNS **-10.576 ns**. Smoke builds are not hardware-build
evidence under `docs/ci-testing-policy.md`. If this was the tested RBF, timing is a plausible
cross-title failure source but does not identify a particular RTL rule. P10a delivered a
deterministic TV80-under-T80pa-contract production-motherboard harness, not the required
exact-tip full-effort timing-clean baseline and not a faithful title oracle. P10d now has a
sustained-cartridge WAIT baseline but no valid ordinary-RAM/title comparison; P10g title-level
Pang/CRTC3 first-divergence work has not started.

Status vocabulary from this checkpoint onward is: **implemented**,
**simulation-verified**, **full-fit/timing-clean**, and **hardware-confirmed**. Promote an
item only when its own gate is recorded. The P10 remediation checkpoint is integrated to
produce a hardware-testable build, but remains simulation-verified only until fresh review,
exact-tip synthesis/timing, and the open production/hardware gates are recorded.

## Hardware-test milestone

`27cb993` is the newest successfully synthesized code milestone (GitHub Actions run
`32970234749`, 2026-08-26, workflow_dispatch, Quartus 17.0.2, full effort): the merge of
`accuracy/f14-f15-interlace` -- the F14 additional interlace line on both classic types and
the F15 type-0 odd-R9 IVM counting with its VSYNC delay correction, fixtures `t27a`-`t27d`,
`t28a`-`t28c`, `t29a`-`t29e`, independently reviewed by Codex with all four findings
remediated (`accuracy/archive/f14-f15-independent-review.md`) -- on top of the Plus P4 mobo-bench tip.
**It has not been hardware-tested.**

- Simulation at this hardware milestone: 175 required CRTC passes / 0 failed, all Plus
  leaf/integration suites green; lint clean. Its soak hash was `0x48146d2b681268ab`;
  even-R9 and non-IVM behavior bit-identical, t21-t24 untouched.
- Logic utilization 28,533 / 41,910 ALMs (68 %); 37,685 registers; 685,217 / 5,662,720
  block-memory bits (12 %); 100 / 553 RAM blocks; 34 / 112 DSP blocks; 145 / 314 pins.
  Versus the previous milestone `c762d36` (29,971 ALMs, 37,442 registers) the classic
  CRTC delta alone enters the fit: -1,438 ALMs (fitter re-placement around the new
  frame-end intercept logic), +243 registers, memory/DSP unchanged.
- Worst-case setup slack +0.635 ns, hold +0.151 ns -- positive; setup improved +0.375 ns
  versus `c762d36`. No regression signal.
- RBF retained as `output_files/hardware-milestones/Amstrad_20260826_27cb993.rbf`
  (SHA-256 `2a94701c30c922f94e3f3bb8050f0e2a5bfa437d90930688330c7ae3d053e79a`). Use a
  current-tip RBF for the next SHAKER session; interlace-aware tests (SHAKER 22C/3
  toggles, Module A `(O)`) are the ones this change should move.

The P4 work merged three times (`0dabcb8`, `85b0eaa`, then `143a213`
on 2026-08-26 carrying review-pass 3-5 remediations with the thread
closed CLEAR — see the cleared review-debt row for the full record).
Exact synthesis ran green at the engine tip `e3dd848`/`69e5d91`
(run `32902476483`: 71 % ALMs / +0.436 ns setup), at merge tips
`b533a93`/`dcbc6ad` (run `32914211795`: 72 % ALMs, +0.260 ns setup),
and a fresh dispatch re-confirmed at the docs tip `abfcadc`: GitHub
Actions run `32918709419` (workflow_dispatch, Quartus 17.0.2) passed
simulation, policy, exact synthesis and the required gate — 29,971 /
41,910 ALMs (72 %); 37,442 registers; worst-case setup slack +0.260 ns,
hold +0.241 ns; RBF artifact `Amstrad_20260826_abfcadc.rbf`. Identical
fitter figures to `dcbc6ad`: the fq_row/fq_acc validation flops and
their compare mux fit inside existing slack; no regression signal.

The classic stream's `accuracy/d1-followups` branch then merged onto
this tip as `c762d36` (2026-08-26; t25 type-0 adjustment addressing
pins, t26 section 17.5 R1=0 deadline pins, the Q10/Q11/Q12/Q19
re-adjudication with findings F14/F15, and the A4/N-6
housekeeping; reviewed CLEAR pre-merge by ask-claude review at Opus 5
high with gates and bite-tests independently reproduced, seven
non-blocking findings remediated at `6c7c905`). Dispatched CI run
`32921230624` (2026-08-26, Quartus 17.0.2) passed simulation, policy,
exact synthesis and the required gate with fitter figures identical to
`abfcadc` (29,971 / 41,910 ALMs, 37,442 registers, +0.260 ns setup) —
the branch's RTL delta is a comment change, so the fit is neutral; RBF
artifact `Amstrad_20260826_c762d36.rbf` (SHA-256
`981fd9dcd7f81ed5d1c9f53447b722381b13169f086a23adcf414e55f40ec858`).
The branch's own intermediate milestone (comment-only RTL, sim/docs
delta) is run `32917100161` at `d9abf35`: 16,208 / 41,910 ALMs,
21,112 registers, +0.623/+0.231 ns. Soak unchanged at
`0x63d9de100ac9f6f2` across the whole session; nothing hardware-tested.

`b533a93` was the first merge's synthesized milestone: GitHub Actions run `32905405634`
(2026-08-25, Quartus 17.0.2, workflow_dispatch at the merge tip) passed
simulation, policy, exact synthesis and the required gate. Fitter:
29,893 / 41,910 ALMs (71 %); 37,506 registers; 685,217 block-memory bits
(12 %); 34 / 112 DSP blocks; worst-case setup slack +0.436 ns, hold
+0.179 ns — positive; no regression signal. Versus `5d6d342` (16,198
ALMs / 39 %) the growth is the sprite engine's flop-based staging arrays
and datapath entering the fit for the first time; memory bits are
unchanged by design. RBF retained as artifact of run `32905405634`
(`Amstrad_20260825_b533a93.rbf`). Not hardware-tested. The push-run
`32905376888` is identically green. Historical paragraph:

NOTE superseded — see above.



`5d6d342` is the newest successfully synthesized code milestone (GitHub Actions run
`32852900420`, 2026-08-25, Quartus 17.0.2): the merge of `plus/p2-asic-regs`
into this branch - P1 review follow-ups, the calibrated p1_video bench, the P2 ASIC
register page (asic_regs + motherboard integration + 4-bit RGB widening) and P3
interrupts (PRI merged into asic_ga_timing, DCSR fields, IVR vector supply), on top
of the F11h/t24 classic work. Both streams' independent reviews are complete and
remediated (`docs/plus/archive/p2p3-independent-review.md`, plus the F10/F11h records).
**It has not been hardware-tested.**

- Logic utilization 16,198 / 41,910 ALMs (39 %); 21,030 registers; 701,601 /
  5,662,720 block-memory bits (12 %); 34 / 112 DSP blocks.
- Worst-case setup slack +0.665 ns, hold +0.250 ns - positive; no regression signal.
- RBF retained as `output_files/hardware-milestones/Amstrad_20260825_5d6d342.rbf`
  (SHA-256 `94b426a49f612895ff287072409badd452cddfa685f3bd7c27f130d3a5e75af5`).

## Hardware-test milestone

`bf1e785` is the final combined Accuracy/Plus integration milestone (GitHub
Actions full-effort run `33396320914`, synthesis job `99501803549`, artifact
`9760139200`, Quartus 17.0.2). It includes the IA-1 through IA-6 accuracy audit
closures and the P10j two-M10K sprite-pixel store. **It has not been
hardware-tested.**

- Logic utilization 22,120 / 41,910 ALMs (53%); 26,045 registers; 701,596 /
  5,662,720 block-memory bits (12%); 102 / 553 RAM blocks (18%).
  `plus_sprite_ram` uses exactly 16,384 bits and 2 M10Ks, with no soft mirror or
  duplicate bank copy.
- Worst-case setup slack +0.613 ns and hold slack +0.187 ns across seven clocks;
  setup and hold TNS are both zero.
- RBF retained as `output_files/Amstrad_20260831_bf1e785.rbf`, SHA-256
  `6dc8fddcb44098eafee485830323070f0421de68c13e273e39489a6c53c3ef19`.

## Hardware-test milestone

`c047a7d` is the accepted P10j feature-build milestone (local-VM run
`33392854459`, job `99490354763`, Quartus 17.0.2). It replaces the 4Kx4
register-backed sprite pixel store with one 2,048x4 true-dual-port M10K per
even/odd bank and adds fail-closed setup/hold timing checks. **It has not been
hardware-tested and predates the final combined Accuracy/Plus integration
build.**

- Logic utilization 22,057 / 41,910 ALMs (53%); 26,101 registers;
  701,596 block-memory bits; 102 RAM blocks. `plus_sprite_ram` itself uses
  exactly 16,384 bits, 2 M10Ks, 20 ALUTs, and 3 registers, with no soft mirror
  or duplicate bank copies.
- Worst-case setup slack +0.323 ns and hold slack +0.251 ns; setup and hold TNS
  are both 0.000.
- RBF `Amstrad_20260831_c047a7d.rbf`, SHA-256
  `b0ccf327bcc5054466ef19828af50b21fbce9b3079a6d92655ba582968030945`.
  Exact reports, rejected intermediates, and the reviewer limitation are in
  `plus/p10-review-debt-status-2026-08-31.md`.

## Hardware-test milestone

`df6c0f7` is the newest successfully synthesized code milestone of the classic stream
(GitHub Actions run `32845921357`, 2026-08-25, Quartus 17.0.2): the merge of
`accuracy/f11h-and-ivm-vsync-coverage` (F11h same-edge R12/R13 closure, t24 type-1 IVM VSYNC
positions + MID-VSYNC, checkout v7 bump; reviewed and remediated) on top of the F10 merge
`e78e0ab`. Recorded retroactively: the original recording commit (`b806502`) landed empty,
so this section was re-inserted from its message and the session record. **It has not been
hardware-tested.**

- Logic utilization 15,745 / 41,910 ALMs (38 %); 20,651 registers; 685,217 / 5,662,720
  block-memory bits (12 %); 100 / 553 RAM blocks; 34 / 112 DSP blocks. Versus the previous
  classic-stream milestone `e78e0ab` (15,718 ALMs, 20,717 registers) the delta is the classic
  CRTC changes alone (no Plus file moved); memory and DSP are unchanged.
- Worst-case setup slack +0.662 ns, hold +0.228 ns — positive; no regression signal.

## Hardware-test milestone

`3d7a178` is the newest successfully synthesized code milestone (GitHub Actions run
`32810340518`, 2026-08-25, Quartus 17.0.2): the Plus branch `plus/p2-asic-regs` with
the P1 review follow-ups, the calibrated p1_video bench in the gate, the P2 ASIC
register page (`asic_regs` + motherboard integration + 4-bit RGB widening), and P3
interrupts (PRI merged into asic_ga_timing, DCSR fields, IVR vector supply on
acknowledge). **It has not been hardware-tested.**

- Logic utilization 15,868 / 41,910 ALMs (38 %); 20,551 registers; 145 / 314 pins
  (46 %); 689,313 / 5,662,720 block-memory bits (12 %); 34 / 112 DSP blocks. Versus
  `e78e0ab` (15,718 ALMs, 20,717 registers) the delta is `asic_regs` plus the PRI
  merger entering the fit; memory +4,096 bits (sprite RAM); DSP unchanged.
- Worst-case setup slack +0.657 ns, hold +0.243 ns — positive; no regression signal.
- RBF retained as `output_files/hardware-milestones/Amstrad_20260825_3d7a178.rbf`
  (SHA-256 `1d47554fca5d082170855c637c224c67d85cc854da91c18a958c61f47a291505`).

The prior milestone paragraph is kept below for bisection history.

`e78e0ab` was the newest synthesized code milestone before that (GitHub Actions run
`32789356344`, 2026-08-25, Quartus 17.0.2): the merge of `accuracy/f10-fixtures` (F10
interlace parity machinery, both types, reviewed and remediated) on top of the Plus P1
motherboard integration (merge `4cab4ec`). **It has not been hardware-tested.**

- Logic utilization 15,718 / 41,910 ALMs (38 %); 20,717 registers; 685,217 / 5,662,720
  block-memory bits (12 %); 100 / 553 RAM blocks; 34 / 112 DSP blocks. Versus the
  pre-both-streams `de30faf` (15,378 ALMs, 20,168 registers) the delta covers the Plus P1
  motherboard integration and the F10 machinery together; memory and DSP are unchanged.
- Worst-case setup slack +0.734 ns, hold +0.246 ns — positive; no regression signal.
- Every synthesis is now a clean compile and `reports/quartus-cache.txt` records
  `build_mode=clean` (queue item D2, closed 2026-08-24: the Quartus database cache was
  removed after the investigation showed it saved zero time; evidence in
  `docs/ci-testing-policy.md` and the roadmap).
- Queue item D2 closed 2026-08-24: the Quartus database cache was removed after the
  investigation showed it saved zero time (no design partitions → `--flow compile` never
  reuses the restored databases; evidence in `docs/ci-testing-policy.md` and the roadmap).
  Every synthesis is now a clean compile.
- The synthesizable delta over the previously synthesized `f6f09f5` is the CRTC change alone:
  the Plus P1 pixel path lives in `rtl/plus/asic_video.v`, which no QIP compiles, so it is
  simulation-only and contributes nothing to this bitstream. **When `asic_video` is wired into
  the motherboard it must be added to `files.qip`**, or no CI run will ever tell you whether it
  fits or meets timing. `5ddddef` remains the
hardware-*tested* milestone as of August 19, covering the
deterministic-complete F12/F4 counter work and the CPR parser. `1a1233f` is the previous one; GitHub Actions run
`31661330994` passed the complete Verilator gate, Quartus 17.0.2 compilation, fitter,
TimeQuest, RBF packaging, and artifact upload for it, and it carries the independently
reviewed C0>=2 F12 arbitration slice on top of the earlier Dandanator/SDRAM milestone.

The retained CI builds have been downloaded locally under the ignored
`output_files/hardware-milestones/` directory:

| Milestone | Commit | RBF SHA-256 |
|---|---|---|
| F2 status readback | `9c16729` | `40992b8e41ead9a9441734aedd171bd0e0942412fbadb089d62e26e4d3e8ba0c` |
| F3 complete, before F5 | `9956d83` | `9634938072dec7a9c82676a3a5b7192ef6927df50e77d164b84963d0d6c554d2` |
| F5 plus tied-off SDRAM foundation | `4ffa853` | `67efe7c7d07f49b31edb6dfca0c19ccf99237bd62df5eefdb0c780a86c95f0a9` |
| Final milestone with Dandanator isolation | `ba5b629` | `7ede21c7449868764f576c114f1697ffd5e6ce4a9b98a38679861d2d52dd3249` |
| F12 C0>=2 arbitration | `1a1233f` | `fd9705732ae20cb45f1807d4c980b893e974392c3b8f48bdb69ff57794f93319` |
| F12 complete | `365c132` | `f44e16cc8c815a5d34c4a807feadbb54a25d358175fb4d542dfbcfddbd20f231` |
| F4 complete plus CPR parser | `5ddddef` | `e1ba1728435f33fc4fe8e1886b0b7b4021f12a4dff861767440b9e2b60a65ff6` |
| F8 type-1 C5 counter | `4c78603` | `8caa9a9f4db825e6fe0d375554a6810e053ec4839409944139e18b29c8bc8e0b` |

The `1a1233f` fitter used 14,947 / 41,910 ALMs (36%), 685,217 block-memory bits (12%),
and 3 / 6 PLLs. Worst setup and hold slacks were positive at +0.541 ns and +0.192 ns.
The later `365c132` F12-complete build also passed synthesis, with worst setup and hold
slacks of +0.472 ns and +0.253 ns respectively; it has not yet been hardware-tested.
GitHub Actions run `32251491936` synthesized the F4-complete/CPR-parser state at `5ddddef`.
Its fitter used 14,899 / 41,910 ALMs (36%), 685,217 block-memory bits (12%), and 3 / 6
PLLs. Worst setup and hold slacks were +0.606 ns and +0.254 ns. TimeQuest still reports
the repository's existing unconstrained external I/O paths, so these positive internal
slacks are not full timing closure. This RBF is retained under
`output_files/hardware-milestones/f4-plus-cpr-5ddddef/` and is the one hardware-tested on
2026-08-19. The intermediate `365c132` build remains untested and is kept only as a bisection
point.

GitHub Actions run `32289023249` then synthesized `4c78603`, the first build containing F8 on
top of that F12/F4 and CPR-parser state. Its fitter used 14,947 / 41,910 ALMs (36%), 685,217
block-memory bits (12%), and 3 / 6 PLLs, with worst setup and hold slacks of +0.516 ns and
+0.246 ns. The artifact is retained under
`output_files/hardware-milestones/Amstrad-build-17-1/Amstrad_20260819_4c78603.rbf`. It has not
been hardware-tested. It was the proposed next SHAKER build at that milestone;
current testing uses `ce1d2da` (September 9 report above). `5ddddef`
predates F8 and cannot produce evidence for it. The later per-type split and rename were
behaviour-preserving within the directed, soak, and frozen differential projections, but F6
Stage 1 intentionally changed classic DE behaviour and re-minted the soak. F7 RFD, the A1
VSYNC correction, and the A2 reload caveat then re-minted it again with unchanged
seed/schedule/projection; the F10 work re-minted it four more times (fixture-stage field
expansion, one intended behavior mint per type, and the review remediation) and the
current canonical hash is `0xa9e5026de83d287c` (full chain in AGENTS.md). Use a current-tip RBF for F6/F7 work; retain `4c78603` as the clean F8-era
bisection milestone.

Hardware testing on 2026-08-19 covered two milestones and returned the same result for
both. `1a1233f` showed no regression against the stock core and no CRTC-0 compatibility
improvement in the SHAKER Module A tests that were run. `5ddddef`, which adds the
deterministic-complete F12/F4 counter work and the CPR parser, was then tested and also
showed no regression and no Module A progress.

Treat that as a signal about coverage, not only about correctness. The completed F12/F4
counter arbitration work moved nothing that the attempted Module A subtests measure, so
further counter-internal work is unlikely to change those tests. Both sessions tested only a
few subtests and did not record them individually, so no failure can currently be mapped to a
named finding.

Before the next classic RTL change, close that data gap:

- Record every Module A subtest by name and result, for both supported CRTC selections, with
  the stock core tested side by side in the same session.
- Confirm that SHAKER's own CRTC identification agrees with the OSD CRTC selection. If it
  does not, every Module A comparison so far is uninterpretable and that is the first bug to
  chase.
- Map each persistent difference to an implemented finding or to a named gap. The current
  leading hypothesis is that Module A leans on behaviour that was still unimplemented —
  F10 interlace parity and, before 2026-08-30, F13's missing half-character F6 seam — rather
  than on the counter internals already fixed. F13 is now implemented but still requires
  that named hardware retest. F20 now also implements CRTC-1's normal/R2.JIT sub-character
  HSYNC start through the integrated GA path; DSC4 and SHAKER `(TAB)` are its hardware gates.
  F8 (type-1 C5) and F7's type-1 R5-route RFD
  are now implemented, so they are no longer candidate explanations; F6 Stage 1's
  presence/type/skew approximation landed 2026-08-23.

Do not infer hardware accuracy from the green counter-level simulation gate alone.

The build is suitable for classic CPC regression testing. It contains the F1, F2, F3, and
main F5 CRTC accuracy work. Plus P0 is now production-wired: `.cpr` parsing, atomic cartridge
publication, SDRAM service, MMU windows, and CPU WAIT are connected and covered by an
integrated production-sized simulation. A real `.cpr` boot on MiSTer hardware is still
unverified, and Plus video remains the uninstantiated P1 foundation; selecting a Plus model is
therefore a manual P0 checkpoint, not evidence that Plus support is complete.

For a first MiSTer pass:

1. Keep `Plus model = Off`. Boot each classic model that you normally use and test both
   supported CRTC selections.
2. Confirm BASIC boot, keyboard/joystick, reset, video stability, tape if available, and
   `cat` plus a program load from a known-good disk.
3. Exercise SHAKER's status/readback and dynamic VSYNC tests for F2/F3.
4. Exercise its type-0 R0=0/stall tests for F5. Watch sync stability and Gate Array
   interrupt behavior as well as the diagnostic result.
5. Load a normal 512 KiB Dandanator image. The build rejects bytes at `0x080000` and
   above so malformed uploads cannot overwrite the reserved Plus cartridge region.
6. Record the RBF commit, MiSTer version, classic model, CRTC type, diagnostic/image name,
   and observed result. If a regression appears, compare the F2, pre-F5, and final milestone
   RBFs retained from CI before narrowing it further.

## Completed classic accuracy work

- A deterministic Verilator harness and GitHub simulation gate now cover the CRTC at pin
  level. Unexpected passes of named divergences fail the suite.
- F1 register readback is protected across R0-R31 for both supported CRTC types.
- F2 implements type-1 status bit 5 sampling at the required C0=R0 point and excludes the
  dynamic R6=0 false case.
- F3 implements model-specific live R7/VSYNC timing, including blocked type-0 early writes,
  partial-line duration, interlace boundaries, live type changes, and snapshot loading.
- F5 implements the type-0 R0=0 freeze, R2-dependent HSYNC behavior, live entry and
  recovery, MA/RA behavior, type switching, odd-field VSYNC freeze, and the single C4
  increment when C9 already equals R9 at freeze entry.
- ACCC v1.10 F12 counter arbitration is deterministic-complete for the documented entry
  paths. `t16a`-`t16z` cover R5 arbitration at C0=2, the R4/R9 last-line write windows,
  same-edge C0=0 writes, the C0=1/R5=0 equality-break route, exact-C0=R0 switching,
  `R0=0` and `R0=1` default adjustment, active-adjustment freeze, exact short-line latch
  consumption, bus phases, completion, retained-state lifecycle, and IA-4's p.106
  two-history case where R4 becomes unequal and is restored before line end. Exact sub-character MA/DE/VSYNC behavior still requires hardware or SHAKER
  traces; the deterministic assertions use C4/C9/RA and adjustment state.
- F4 removes the non-equality C9/C4 zero-limit shortcuts for both CRTC types and preserves
  type-0's live-versus-latched Last Line/RLAL behavior. Live R9 writes now also feed the
  independent VMA capture comparison, while active type-0 adjustment reuses C9 against
  R5 rather than R9.
- F8 gives the CRTC 1 type-1 vertical adjustment its own 5-bit C5 counter. C9 now keeps
  cycling 0..R9 during adjustment so the row address keeps cycling; C4 increments at each
  C9==R9 wrap; C5 counts the adjustment lines and the adjustment ends when C5+1 equals R5
  by equality, so R5=0 never ends it — the documented ACCC 11.3.2 hardware bug, reproduced
  deliberately. The comparison is widened to six bits so C5=31 (32) cannot alias R5=0.
  The two former F8 expected-failure cases (`t08f`, `t08g`) are now required passes.
- The September 1 gate recorded 192 required CRTC passes, zero expected failures, no
  unexpected passes, and no failures (Verilator 5.050), plus the
  nine-case hybrid blanking seam, integrated GA R2.JIT, and u765 transaction benches. The
  randomized equivalence soak then reproduced `0x2263c9fc44af4ee7`; the current
  B8-1 golden is recorded at the top of this file and in AGENTS.md. IA-1's `t33a`
  confirms that unchanged RTL already preserved the p.150-151 C3l overflow sequence;
  `t33b` independently failed its first post-write pin sample because the live comparator
  restarted HSYNC after one master tick. The corrected type-0-only path models the p.151
  earliest approximately 3.5-pixel restart as 14 ticks for that controlled bus phase,
  without claiming a universal hardware constant. `t33c` pins lifecycle and R3l=0 guards.
  IA-2's `t32a`
  discriminator first failed the stale type-1 frame-origin toggle-both model from an
  even-R9 unequal-parity state. French ACCC v1.11 section 19.5.3 p.209 now governs the
  origin: the new ParityFrame seeds ParityC9 and, in IVM, C9. This is source-model
  evidence; hardware confirmation remains open. IA-4's `t16z` discriminator likewise
  first failed unchanged RTL at C4=3 instead of the
  source-derived C4=2/C9=4 transient result. Its sticky history correction did not move the
  then-current soak, so the directed vector carries that proof. IA-3's `t34a` first failed
  unchanged RTL at the frame-origin first half: French v1.11 section 18.3.2 p.191 expects
  DE high, but the ordinary row-next/R6 priority forced it low. The corrected type-0 path
  preserves the documented high-then-low half-character alternation and uses live R6 at a
  reachable C0=R1: R6 still zero makes border definitive, while an earlier R6 0-to-nonzero
  write cancels the conflict. The new lifecycle latch joins the soak projection; this is
  source-model evidence and real type-0 hardware confirmation remains open. IA-6's `t35a`
  closes the only uncovered bilingual-premise route: its safe C0=0 R0 widening control
  passes unchanged, while unchanged RTL fails the unsafe C0=1 widening arm because the old
  R0=1 comparator resets C0 instead of continuing to the new total. The type-0-only
  correction holds the accepting edge, enters C0=2 with C4=R4+1/C9 retained, and persists
  through R5=0's effective target 31. The private pending action joins the soak projection.
  French section 13.7.2.1's non-last-line overflow and positive-R5/interlace variants remain
  outside this source-backed slice, and hardware confirmation remains open. The
  §13.7.1.2 trigger leaves the hash unchanged because random traffic does not reach that
  window; per review finding F-9 the soak is measurably insensitive to this region, so the
  directed vectors — not the soak — carry the behavioral proof here.
  IA-5 closes without an RTL or vector change: French §4.2 p.18 supplies the U.S.-ROM
  R5=6 versus R5=4 frame arithmetic, but the repository has neither the physical U.S. ROM
  initialization path nor an independent phase trace. The integrated CRTC/GA harness can
  only replay `syncgen.v`'s own answer. A simultaneous real-hardware capture of raw CRTC
  HSYNC/VSYNC and GA `INT_N`, with exact ROM/register/type metadata, is the retained
  discriminator for the French same-scanline-before-VSYNC claim.
  The Plus leaf, SDRAM, and boot integration suites are also green.
- The core is split into a shared-state wrapper (`rtl/CRTC.v`) plus two per-type rule engines
  (`rtl/crtc_type0_engine.v`, `rtl/crtc_type1_engine.v`); live `CRTC_TYPE` round-trips stay
  pinned by t02j/t06d/t09f/t16l, and bit-identity with the pre-split core is pinned by the
  reproduced lockstep differential run.
- F9 closure is merged into this branch: the documented `t12` worked-example pair — R9 write
  at exact C0==R0 → C4=39/C9=8, and its windowed companion in C0∈[2,R0−1] → C4=38/C9=8
  (ACCC p.82) — is encoded as `t12a`/`t12b` (`aea80b5`, merged via `d5cab8f`).
- F13 implements the ACCC pp.186/195 no-skew type-0 half-character DE pulse: `t31a` pins
  first-half display, second-half border, and next-character recovery; t10a-t10e retain
  presence/type/skew controls and SKEW 1/2's rounded full-character displacement. This is
  an ACCC-model correction pending SHAKER Module A `(O)` and, if possible, DE-pin validation;
  it is not yet hardware evidence.
- F20 implements ACCC v1.11 §9.3.4.1/§9.3.4.3/§14.6.1 R2.JIT timing through
  the integrated production CRTC+GA clock path. Real Z80-phased `OUT (C),r8`
  controls require type-0/type-1 starts +4/+3 Mode-2 pixels later, raw widths
  shorter by 4/3, fixed type-specific display-reactivation edges, and a
  same-value rewrite on the normal path. Every new phase/deferred-edge latch
  joins the soak projection. DSC4/SHAKER `(TAB)` hardware confirmation, OUTI,
  active-pulse R2 updates, and the independent
  RFD×IVM compound case remain open.
- F7 RFD is implemented for the type-1 R5 route (`t13a`-`t13d`): same-edge `R5 0→nonzero`
  arming at C0=R0, VMA-from-R12/R13 on every row, parity-gated VMA' saves with odd-R9
  frame-parity alternation, successful-save disarm, and the B6 R1>R0 bare-C9 disarm.
  A1 closes the adjustment-ending VSYNC corner (`t08m`, corrected `t08g`) and A2 implements
  the §11.2.4 exact-C0==R0 caveat pair (`t08n`/`t08o`). The §13.7.1.2 R0-widening trigger —
  the second route — landed 2026-08-23 (`t13e`-`t13m` after cross-provider review): a strictly
  widening R0 write on the C0==R0 edge of the frame's last line defers that line end (wrapper
  `hcc_end`), and a last-line condition cancelled by R9/R4 rewrites before the extended end
  arms the same two flags there. The review's blocking findings are remediated in-branch:
  the vestigial display-end guard is removed (`t13m` pins DE blanking at `C0==R1` on the
  extended line) and both off-last-line precondition halves now have load-bearing vectors
  (`t13j` retimed, `t13l` added). RFD#10's "1-B" variant and the scope notes in
  `audit-findings.md` remain as documented there.

- F10 interlace parity machinery is implemented for the unblocked scope on
  `accuracy/f10-fixtures` (2026-08-24): type-1 two-stage R8-toggle parity update and
  §19.8.2 counting (`t21a`-`t21p`, the 16 pp.210-211 panels), type-0 split C9/C9.VMA with
  the asymmetric entry/exit limit tests and §19.5.2 parity rules (`t22a`-`t22o`, the
  pp.221-224 tables). All 31 vectors are required passes; the old stepping/halving
  approximation is removed and non-IVM behavior is bit-identical. The former question gates
  are adjudicated: F14 owns the additional interlace line, F15 owns odd-R9 alternation and
  its VSYNC correction, and F16 owns the post-exit frozen-C9.VMA behavior. Q12's source
  question is resolved by the French v1.11 repeated-activation qualifier; local odd-C4
  transition fixtures and post-toggle timing validation remain separate. Residuals are in
  `accuracy/f10-implementation-notes.md`. The stack was independently reviewed 2026-08-25
  (`accuracy/archive/f10-independent-review.md`): NOT CLEAR on two blockings, both fixed with new
  vectors (`t23a`-`t23c`, `t22p`-`t22s`, RA column in `t22`); review-debt row cleared.
  All three remaining non-gated classic items named for this session are done (2026-08-25):
  the F11h closure, the t24 IVM VSYNC fixture family, and the CI-only `actions/checkout`
  bump — see the three bullets below.

- F11h is closed by implementation (2026-08-25, this branch): the p.242 render shows the
  second CRTC-1 chronogram catching an R12 write that lands on the row-0 line-boundary edge
  itself (OFFSET=#30xx from C0=0) where the paired CRTC-0 chronogram keeps the old offset —
  so the §20.3.2 reload samples the post-edge register file. `t20j` (fixture commit XFAIL,
  behavior commit required pass) pins the catch at a mid-row-0 boundary and the frame
  origin; `t20k` pins the type-0 miss. Soak re-minted `0x801a59096c192d26` (chain in
  AGENTS.md). Unpinned residuals are recorded in the F11h entry of `audit-findings.md`.
  The t24 IVM VSYNC fixture and CI-only `actions/checkout` bump also landed;
  their dated evidence follows.

- t24 is closed (2026-08-25, this branch): the p.208 table (with the §19.8.2 p.225
  alternation, which needs an odd C4 count — the fixture uses R4=6) pins the type-1 IVM
  VSYNC start at the first line of C4=R7 on both frame parities, with no delay correction —
  the documented permanent 1-line gap for odd R7 (`t24a`, required pass) and the no-gap
  contrast for even R7 (`t24b`, fixture XFAIL `e0f5b6a`, behavior commit required). The fix:
  `vsync_line_fire` uses the IVM-aware row-structure test, and during type-1 IVM the legacy
  field=1 MID-VSYNC arm no longer hijacks fire or count tick. Soak re-minted
  `0xd620fce8b1c05b25`. The type-0 IVM VSYNC rule (the §19.5.2 delay) was
  subsequently implemented under F15. The CI-only
  `actions/checkout` bump landed as `4e776f1` (v4 → v7, standalone).

- Independent review of the F11h+t24 work (Claude Opus 5 xhigh via the ask-claude bridge,
  fresh session; record `accuracy/archive/f11h-t24-independent-review.md`, review-debt row
  `accuracy/f11h-and-ivm-vsync-coverage`) returned NOT CLEAR on one blocking finding,
  **B-1**: the t24 fix had silently removed the type-1 IVM MID-VSYNC (p.208 schedules it on
  the ParityFrame-even frame) and no vector sampled the half-line phase. Remediated on this
  branch: the wrapper now keys the type-1 IVM VSYNC on ParityFrame directly — even-parity
  frames start and end the pulse at the half-line tick via a seam-latched fire decision
  (`e1_vsync_line_fire` is hcc-independent, so consuming it mid-line needed the latch),
  odd-parity frames keep the seam start/end — pinned by `t24c` (fixture XFAIL `acbc51a`,
  behavior commit required). The reviewer's sandbox could not execute gates or bite-tests;
  the gates and all three of the reviewer's bite-tests were reproduced by the parent
  session against the remediated tip: (a) forcing `vsync_type1_ivm` to 0 (legacy field mux)
  fails exactly `t24b`+`t24c`; (b) reverting the engine row-end test to plain `C9==R9`
  fails exactly `t24a`+`t24b`+`t24c`; (c) deleting the `e1_row0_reload` override fails
  exactly `t20j`; each restore re-greens the suite. Soak re-minted `0x63d9de100ac9f6f2`.
  Non-blockings: N-3/N-4/N-5 fixed; N-1 recorded as a named residual comment at the gate
  (raw R8 mode vs latched engine IVM, 1-2 character window at toggles); N-2 folded into
  action item A4; N-7 covered by synthesis-on-merge. Review-debt row cleared 2026-08-25.

- D1 follow-up, p.81 type-0 adjustment addressing (2026-08-25, this branch): the D1
  correction's RTL premise did not survive verification. The p.81 LINE column is the
  C9-driven segment of the *composed* VRAM address (§20.2 p.241 takes bits 13:11 from
  C9[2:0]; `Amstrad_motherboard.v` forms `{MA[13:12], RA[2:0], MA[9:0]}`), not the CRTC
  module's raw MA port — that port carries the video pointer, which must scan per character
  because the p.83 prose's memorized value is line start + R1. The wrapper already produced
  the documented behavior (C9 counts to R5 at the seam limit, RA=C9 feeds the composition,
  save/restore gives the pointer steps), so Item A of the session brief landed as
  **required-pass pins, not a fix**: `t25a` (period-8 segment cycle, wrap at C9=8, DE-off
  caveat pinned), `t25b` (constant pointer between crossings, single +R1 step at the C9==R9
  crossing, within-line scan), `t25c` (exit resets C4/C9 and reloads R12/R13). Bite-tested
  (mutation: `type0_c0_adjust_line_max`, the C0=0 seam limit, retargeted to R9): fails
  exactly the t25 family plus the nine existing t16/t08k adjustment guards; a broader
  mutation that also retargets the live in-adjustment limit adds t12a to the failure set. Soak unchanged at `0x63d9de100ac9f6f2` (no behavior change —
  the brief's expected re-mint does not apply). Recorded NOT-PINNED boundary: the tables
  normalize PTR-VRAM to 0 at adjustment entry, so the absolute entry pointer value
  (last-row base vs base+R1, i.e. whether the entry line's own capture applies) is not
  source-adjudicated; this core keeps the plain-rule entry capture and t25 asserts only
  source-supported deltas. Full adjudication in `compendium-01-counters.md` §4.1.

- D1 follow-ups session, 2026-08-26 (branch `accuracy/d1-followups`, base `6030b4c`):
  - **Item A** (p.81 adjustment addressing): landed as required-pass pins, not a fix —
    see the dedicated bullet above. Soak unchanged; the brief's expected re-mint does not
    apply.
  - **Item B** (§17.5 R1=0 write deadline, p.185): the RTL already matches the documented
    deadline via the seam-time `hcc_next==R1` check and the R1 write-hit term (`hcc==DI`);
    `t26a` (type 0) / `t26b` (type 1) pin it as required passes, bite-tested (disabling the
    write-hit term fails exactly t26a+t26b). Derived and pinned beyond the chronograms: a
    too-late write still updates the register, so the live `C0=R1` comparison targets 0 and
    the too-late line displays past the old R1's end; R1=0 is honored from the next line.
    Digest §17.5 updated. Soak unchanged.
  - **Item C** (Q19/Q10/Q12 re-adjudication, fresh renders pp.198-199/205-206/216/219-220/
    223-224): Q10 RESOLVED — the additional interlace line is generated at the end of the
    ParityFrame-even frame (type 1 gate ParityFrame even; type 0 gate ParityR6 odd with the
    R6>R4 freeze) and duration-counted in the following odd frame; labelling is
    ParityFrame-relative. Q11 RESOLVED (p.205 states even explicitly). Q12 RESOLVED by the
    2026-08-28 edition comparison: French v1.11 p.208 specifies activation on every frame;
    English p.206 omits the qualifier. This does not verify post-toggle pin timing.
    Q19 main token + (a) RESOLVED (`R9.0=0` is a typo for `R9.0=1`; the
    three-phase comparison form is pinned by p.220 and already implemented). At merge time
    Q19(b) remained open: the pp.223-224 exit tables implied a frozen-C9.VMA line-end test
    after a non-matching R8=0 write, while this core resumes a live plain `C9==R9` test on
    post-write lines. The subsequent 2026-08-26 visual pass resolved that discriminator:
    seven non-match windows run through C9=7 and the frozen-6 control resets; one of the
    seven run-ons also has an isolated anomalous C4=2 cell. Actionable rules are findings **F14** (additional
    interlace line, both types), **F15** (type-0 odd-R9 IVM counting incl. the §19.5.2 VSYNC
    delay correction), and **F16** (post-IVM-exit frozen C9.VMA) — fixtures before any RTL.
  - **Subsequent author-question closure pass** (2026-08-26): Q4's p.88 rule exposes **F17**
    because the current F7 arm and required `t13d` retain the R12/R13-source flag on C9=R9;
    Q13 confirms real UM6845R R16/R17 light-pen readback and exposes the absent LPSTB path as
    **F18**. Q17 is not closed: detailed adjustment arithmetic/current sim predict R7=39
    silence, while §28.1.1 explicitly predicts a pulse; hardware must discriminate.
  - **Item D**: A4 closed (the `expect_known_*` helpers renamed `expect_xfail_*` with the
    house rule in the comment; review-debt row done); N-6 closed (`actions/upload-artifact`
    v4 → v7, standalone; first run resolves it and uploads green).
  - **CI evidence**: dispatched run `32917100161` (2026-08-26, Quartus 17.0.2) fully green
    — simulation, policy, synthesis, required gate; the v7 artifact upload verified online.
    Fitter: 16,208 / 41,910 ALMs (39 %); 21,112 registers; 701,601 / 5,662,720
    block-memory bits (12 %); 34 / 112 DSP blocks. Worst-case setup slack +0.623 ns, hold
    +0.231 ns — positive; versus the previous milestone `5d6d342` (16,198 ALMs, 21,030
    registers) the delta is tool noise on a comment-only RTL change. RBF retained as
    `output_files/hardware-milestones/Amstrad_20260826_d9abf35.rbf` (SHA-256
    `26f415d4d9d2723c98168539cbec91fd421b7b79da05c12a602f0f9a89dde259`). Not
    hardware-tested.
  - **Soak**: unchanged at `0x63d9de100ac9f6f2` throughout the session — no classic
    behavior change landed (pins and docs only), so no re-mint is due.
  - **Review status**: end-of-session cross-provider review of the branch diff happens
    before merging; outcome recorded in the merge/branch notes.

D1 is complete (2026-08-24): every remaining ⚠ VERIFY flag in the three digests was
re-verified against the PDF (pdf-inspector Markdown primary, figures judged from rendered
pages). Outcomes: most flags retired as confirmed; four genuine digest errors corrected —
p.81 type-0 adjustment addressing is period-8 through 8 distinct addresses (never propagated
to RTL/vectors), §17.5's R1=0 deadline boundary was inverted-ish and is now derived (type
0/1/2 accept writes through C0=0, type 3/4 close two characters earlier), p.183's worked
example uses R1=40/&28 not 64, and the pp.221-224 IVM tables were re-adjudicated after an
initial misread: they use R9=6 (even, per p.220) and corroborate the §19.8.1 pseudocode; the
surviving source-internal conflict is only the p.219 gate token `If R9.0=0` against its own
odd-R9 gloss (author question Q19, narrowed). The re-verification was itself cross-reviewed:
a GPT reviewer-cross pass returned five blockings, two of which contained evidence errors
that an Opus adjudication settled against the renders (both disputes sided with the
re-verification's corrected readings); all other blockings are remediated in this diff. Page
anchors were corrected against the real TOC (§13.2.x, pp.210-211, p.247, §21.4), and the
separate stale-reference sweep over docs/ found ten more fixes; rtl/ and sim/ citations were
all clean. F10 has since been implemented, reviewed, and merged (see the completed-work
section); F13's ACCC-model correction is now implemented and waits for hardware validation.

## Historical Plus milestone record (P-2 through P9)

This section preserves the implementation evidence and assumption boundaries at each dated
milestone. Any “next” wording inside it is historical; the active Plus queue is P10 in the
2026-08-29 checkpoint above and the next-session order below.

- A separate default-off `Plus model` selector decodes GX4000, 6128+, and 464+ capabilities
  without reinterpreting the classic model field or selecting Plus hardware.
- The ASIC lock/unlock state machine is implemented and exhaustively unit-tested as a leaf.
- An atomic 512 KiB cartridge memory service is implemented and tested for clear, load,
  commit, abort/detach/reset, invalid addresses, and CPU reads.
- `sdram.v` now has a held cartridge request/acknowledge client with tested byte lanes,
  addressing, arbitration, back-to-back transfers, classic main/tape writes, and refresh
  fairness. P0 production wiring connects it to the cartridge service when Plus mode owns a
  cartridge window; classic mode leaves it inactive.
- A real service-to-real-SDRAM simulation proves exact clear/load transaction counts,
  publication, and CPU readback without duplicate held requests.
- A bounded, streaming RIFF/CPR parser now validates the `RIFF`/`AMS!` envelope, accepts
  ordered `cbNN` cartridge-bank chunks, handles RIFF padding, streams payload bytes into
  the atomic cartridge service, and fails closed on malformed or aborted downloads.
  Oversized `cbNN` chunks abort instead of truncating (A5 decision,
  `docs/plus/architecture.md` "CPR parser policy (P0)").
- P0 wiring is complete on `plus/p0-parser-wiring`: `plus_mmu` implements the Plus
  cartridge windows (high window from the ROM-select port incl. the GX4000 page-1 rule
  and the /EXP-dependent value-0 rule; low window position/page from unlock-gated RMR2;
  ASIC-page-enable captured but unbacked until P2). `/EXP` is a defined dynamic input,
  tied high at the top level for P0 (= no expansion connected). The cartridge memory
  service is production-connected to the reserved SDRAM port, and Z80 reads in cartridge
  windows are bridged to the service with CPU WAIT insertion. The watchdog pauses while the
  cartridge service is clearing/loading, and retains fail-open behavior only for a quiescent
  backend that does not respond.
  The CPR stream is live on ioctl index 8 (OSD "F8,CPR"), and the historically named P0
  boot integration bench runs parser + service + real SDRAM through the memory path,
  including reset-mid-load cleanup. It does not instantiate the real CPU/top level or prove
  reset-vector execution; P10a owns that missing coverage.
- Dandanator uploads are bounded below the Plus cartridge reservation: bank 3
  `0x000000..0x07ffff` remains Dandanator, while `0x080000..0x0fffff` is reserved for Plus.
- Plus P1 counter/timing foundation is implemented on `plus/p1-crtc3-foundation`:
  `rtl/plus/asic_video.v` carries the type-3 register file, C0/C9/C4 counters with the
  type-3 R9-forced-reset and R4-overflow rules (ACCC §10.3.4/§12.5), R5 vertical
  adjustment that freezes C4 at R4 (§11.2.6/§11.3.3), the two-stage video pointer with
  the C4=0 ∧ C0=0 reload condition (§20.3.4), DE with line-start-only R6 semantics
  (§18.2.4) and SKEW-DISPTMG (§19.2), and HSYNC/VSYNC generation including bounded
  R3=0 widths and the live §15.3 end/start collision. 28 deterministic vectors
  (t01a-t04i) cover them, including the p.151 live-R2 chronogram; every sourced rule cites
  its ACCC section at the point of implementation. `t03c` also pins the simultaneous C0=R1=R0
  row-end save/reload so MA
  advances to the captured row base rather than restoring stale VMA'. Interlace is
  stored-but-inert; the status registers/read map are implemented by P5 below.
  Follow-up vector `t04i` makes the R3l=0 end/start-collision exception explicit: the
  current model keeps the documented 16-character pulse bounded, but that boundary is an
  unverified model assumption pending a direct rule, Logon observation, or hardware capture.
- Still open in P1 before the milestone is complete: the CPU/WAIT
  timing-contract decision that lands with the first motherboard instantiation
  of `asic_video` (architecture §5 Risk 1), plus the intra-character pixel
  phase validation against the ga40010 cadence named below. `files.qip` is
  untouched until that instantiation commit. The locked-ASIC title boot-point
  check remains the manual P0/P1 hardware checkpoint described above.
- P1 remainder, locked-ASIC pixel path, is implemented on
  `plus/p1-pixel-path` as a leaf extension of `asic_video`: a pen pipeline
  decoding two video bytes per CRTC character (MA is word-addressed;
  ga40010 latches VIDEO_BUF twice per character) at the documented
  mode-dependent rates, border substitution outside DE, HSYNC forced blank,
  screen mode latched on HSYNC assertion, and a registered 32-entry
  legacy-colour ROM translating hardware colour numbers to 4-bit-per-channel
  RGB. Sources: [KT] palette table (web.archive.org capture 20230923001014),
  cross-checked entry by entry during extraction against this repo's own
  ga40010 DAC equations — all 32 agree; byte/pixel layouts from the Grimware
  Gate Array page (already cited by color_mix.sv), corroborated by the
  netlist cidx taps ({r1,r5,r3,r7}/{r3,r7}/{r7}). Vectors t05a-t05h pin the
  ROM sweep, all four mode layouts, border/sync blanking, the
  after-next-HSYNC mode latch, and the per-character byte-latch phase. Explicit unverified P1 model assumption
  (t04i discipline): the first pixel of a character's even byte is presented
  on dot 0 with one-dot registered output latency; the real GA's pipeline
  latencies relative to its load/DISPEN cadence (Plus INKR effects ~1/4
  character late, 40010's one-pixel mode-2 early start) are deferred to the
  motherboard-integration differential check. Interlace stays stored-but-
  inert; the status registers/read map are implemented by P5 below.
- CI evidence for the pre-rebase P1 foundation branch (run `32632492492`, original tip
  `0be8a60`):
  simulation and synthesis both green. Fitter: 15,295 / 41,910 ALMs (36%), 685,217
  block-memory bits (12%), 3 / 6 PLLs; worst setup slack +0.342 ns — numerically
  identical to the P0 merged-tip build, and `asic_video` appears nowhere in the fit
  report because nothing instantiates it yet. No regression signal; the first
  meaningful synthesis delta arrives with the P1-remainder motherboard integration.
- ASIC register page backing, palette, interrupts, sprites, split/scroll,
  and DMA are not implemented. FDC/tape presence gating for GX4000/464+ is also still
  inert (P8 polish scope).

Historical P1 handoff (completed by later milestones): the next work at this point was the
P1 motherboard-integration commit per
`docs/plus/architecture.md` §4/§5/§7: instantiate `asic_video` (deciding the
CPU/WAIT timing contract, Risk 1: replicate ga40010 timing behaviorally vs
keep it as clock generator), wire the video word and legacy GA-config inputs,
add `files.qip`, record fitter utilisation, and validate the intra-character
pixel phase assumption against the production cadence; then P2. The P0
hardware checkpoint is manual:
with a Plus model selected, load a real `.cpr` (e.g. the local untracked `crtc3_v2fix.cpr`
fixture) and confirm the firmware/game reaches its first screen; classic mode must be
re-checked side by side in the same session. Do not start Plus video by extending
`ga40010`; the planned path is the parallel behavioral `asic_video` module.

### P1 motherboard integration — landed on `plus/p1-motherboard-integration` (2026-08-24)

Risk 1 was decided in favour of option (a): `rtl/plus/asic_ga_timing.v`
reproduces the ga40010 timing contract behaviourally (sequencer, CCLK/PHI/
READY/RAS/CAS/CPU_N, CAS refresh masking, monitor sync shaping, 52-line
interrupt counter, legacy GA register file). Cycle-exact equivalence with the
synthesised ga40010 composition is pinned by
`sim/plus/asic_ga_timing_diff_tests`, which compiles the reference with
`-UVERILATOR` (its simulation-only shadow domain double-drives the sync
outputs under plain Verilator) and drives both with identical randomised
bus/reset/sync traffic; register payloads ga40010 does not export are pinned
by directed vectors r01-r03. Deliberate deltas are documented in the module
header (no SNA preload — no Plus snapshots; defined INKR power-up values,
named unverified assumption).

`Amstrad_motherboard.v` instantiates both Plus subsystems unconditionally and
muxes at the consumption points (house style); classic mode is untouched
(soak re-verified). ga40010 stays the ROM-enable source in both modes because
its register decode watches the same bus; plus_mmu overlays cartridge
windows. The motherboard assembles VIDEOD on the reference VIDEO_BUF latch
phases (e0 → even byte, 03 → odd byte). RGB reaches the existing 2-bit+OE
path through a temporary lossless adapter for the legacy {0,6,15} levels;
true 4-bit widening is P2's first commit. `files.qip` gained both new files.

Open P1 follow-up, tracked here so it is not lost: `sim/plus/p1_video_tests`
(`make -C sim/plus p1-video-bench`) hosts an integration bench with a classic
CRTC+ga40010 oracle slice intended to close the t05h pixel-phase note by
requiring the Plus PEN stream to match the classic pipeline byte-for-byte.
Its stimulus/sampling calibration is unfinished (the first run showed a
slot-grid/border-sampling mismatch that behaved like a bench-side phase
error, not a production defect), so it builds but sits outside the default
gate until calibrated; the t05h caveat therefore remains open.

**Update 2026-08-25: calibrated and moved into the default gate** (commit
`cee64cd`). Three bench-side defects explained every earlier symptom — the
RTL was correct throughout: a wrong C++ mirror of the fake-VRAM tag pattern,
a CLKEN level probe landing one cen_16 edge early, and tag-pattern aliasing
across 128-byte boundaries (video base moved to &0080). Final coverage: p1a
pins the word-granular pointer stream with dots 0-7 = even byte / dots 8-15
= odd byte through the production VIDBUF assembly (t05h assumption closed at
integration level), tolerating the mixed word across each line-start MA
reload; p1b pins border-flag interiors only — the PEN flag's de_hold capture
skews the colour-class switch at region boundaries by up to one character,
which is the documented GA pipeline latency question and stays deliberately
open with the motherboard timing contract; p1c checks classic VIDEO_BUF
provenance over the same window (byte order is pinned by p1a; netlist buffer
latency is GADIFF territory). Bite-tested against an assembly-order swap.

An Opus-5-high independent review of this P1 delta returned NOT CLEAR on
2026-08-24 with two BLOCKING findings, both confirmed real and fixed in the
same pass: the asic_ga_timing bus pins were wired to uppercase implicit nets
(`MREQ_N` etc.) that synthesis tied to constants — dead GA-register decode,
stuck irqack, no Plus interrupts — and `plus_vidword`'s reset arm had
inverted polarity (active-high `reset`). The review also corrected the
VIDBUF comment (byte order is assumed pending p1_video calibration, not
validated) and flagged that the soak scopes only to `rtl/CRTC.v`, so
'classic untouched' claims must cite the mux inspection, not the soak.
Post-fix CI (run `32777625616`, both blocking fixes in) is fully green:
fitter 15,716 / 41,910 ALMs (37 %), 19,966 registers, worst setup +0.519 ns,
hold +0.252 ns. The +187-ALM delta versus the pruned 15,529 build confirms
the reviewer's constant-bus inference; the earlier paragraph's figures are
superseded by these.

Queued from the review: a Verilator lint pass over Amstrad_motherboard.v
(would have caught finding 1; needs stub modules for YM2149/hid), an explicit
decision on the dead plus_phi_en_* wires vs driving T80pa/crt_filter from
the ASIC enables in Plus mode, re-measuring the fitter delta after these
fixes (previous numbers were taken on the constant-bus-pruned build),
directed U204-restart and randomised-fast lockstep coverage, implementing or
re-documenting the INKR power-up constants so r03 pins RTL rather than
Verilator zero-init, and a minimal plus_mode=1 motherboard bench before P2.

**All queued items implemented 2026-08-25 on `plus/p2-asic-regs`** (commits
`5730b66`, `ea69d68`, `e9f2bca`; CI evidence below):

- `make -C sim/plus motherboard-lint` (in the default lint chain) elaborates
  the whole motherboard hierarchy under `--language 1364-2001 -UVERILATOR`
  with full-port-list stubs for T80pa (VHDL), ga40010/YM2149/hid
  (SystemVerilog; the `.do(` pin name rules out default-SV mode). IMPLICIT
  and UNDRIVEN stay fatal — finding 1's exact bug class. Waived classes are
  triaged and documented in `sim/plus/Makefile`.
- The plus_phi_en_* decision: T80pa, crt_filter CE and the expansion phi
  pins now take ASIC enables under plus_mode via explicit ownership muxes.
  Cycle-neutral today (GADIFF-proven equivalence); makes asic_ga_timing the
  Plus owner so deliberate deltas land everywhere at once later.
- Differential bench: d04 drives an intack bus state across reset (U204's
  reset term — previously uncovered); d01 randomises the no-wait input
  inside lockstep traffic. r03 now proves INKR/ink-select power-up clears
  are explicit RTL resets in `asic_ga_timing.v` (bite-tested), not simulator
  zero-init; real ASIC power-up contents remain a named assumption.
- Motherboard bench m1-m4 (`make -C sim` runs it): plus_mode=1 boot with a
  scripted fake Z80; GA RMR/INKR/border writes reach asic_video through the
  production muxes; the 52-line interrupt fires into the CPU pin and clears
  on acknowledge. Uses --public-flat-rw taps; ga40010/YM2149/hid join as
  stubs (same language constraint).

The gate after these changes reports 256 PASS lines (147 CRTC vectors, all
Plus leaf/integration suites including p1_video and mobo benches); lint green
including the new hierarchy pass; soak unchanged at `0xa9e5026de83d287c`.

CI synthesis of the instantiation is green on the dispatched exact build
(run `32771020608` — simulation, policy, Quartus
17.0.2 compile/fitter/TimeQuest all pass; NOTE fitter figures below were
measured before the two blocking fixes landed and must be re-recorded).
Fitter: 15,529 / 41,910 ALMs
(37 %), 20,483 registers, 145 / 314 pins (46 %), 685,217 block-memory bits
(12 %), 34 / 112 DSP blocks; worst-case setup slack +0.410 ns, hold
+0.246 ns. Versus the pre-integration milestone `de30faf` (15,378 ALMs,
20,168 registers, +0.581/+0.246 ns) the ~150-ALM / ~300-register growth is
the two Plus subsystems entering the fit for the first time (`asic_video`
was previously uninstantiated); the setup-slack shift stays comfortably
positive — no regression signal. Three integration defects were caught by
this CI loop and fixed en route: `plus_vidword` wire-vs-reg (Verilator
tolerated it, Quartus did not), a double drive of `plus_gamode` from both
`MODE` and `GAMODE_O` aliases, and two stale lint/policy expectations
(`-UVERILATOR` on the wrapper lint line; `asic_video.v` now legitimately on
the synthesized manifest).

Plus P0 wiring is merged onto `accc-review-and-fixes` (merge `daf1d6f`) and has a green
GitHub Actions build (simulation + synthesis) on the merged tip. Fitter: 15,295 / 41,910
ALMs (36%), 685,217 block-memory bits (12%), 3 / 6 PLLs; worst setup slack +0.342 ns,
worst hold slack +0.244 ns (TimeQuest still reports the repo's unconstrained external I/O
paths, so internal slacks are not full closure). Versus the pre-P0 build (`4c78603`:
14,947 ALMs, +0.516/+0.246 ns), the ~350-ALM growth and small setup-slack shift match the
added cartridge decode/bridge logic; no regression signal. It has not been hardware-tested.

### P2 ASIC register page — landed on `plus/p2-asic-regs` (2026-08-25)

`rtl/plus/asic_regs.v` backs the &4000-&7FFF page per `asic-reference.md`
§2-§6: the 4K×4 sprite pixel RAM with its low-nibble mask, sprite X/Y/mag
storage with the documented read rules (&FF for all-ones high bytes) and
+4..+7 read mirrors, the 32×12 palette in the documented {G,R,B} word
layout with split-byte writes and a free-running video port, legacy
PENR/INKR translation into entries 0-16 through the [KT] table, PRI/SPLT/
SSA/SSCR/IVR and DMA SAR/PPR byte storage for later phases, DCSR readable
across &6C00-&6C0F but writable only at &6C0F, and wired-AND-neutral open
bus over every unmapped/write-only region. Seven exhaustive vector groups
(a01-a07) run in the gate.

Integration: `Amstrad_motherboard` instantiates it (chip-select from the new
`plus_aspage_on`, legacy GA shadow straight from `asic_ga_timing`); `Amstrad.sv`
captures `plus_mmu`'s RMR2 page-enable and suppresses main-memory read AND
write cycles across the whole window while it is on (no read/write-through,
reference §2, cartridge-owned-cycle pattern), with an answering page read
taking priority on the CPU data bus. The motherboard bench gained cycle-type
awareness (I/O vs memory, like a real Z80's pin behaviour) and m5: scripted
page writes land in sprite RAM, sprite registers and palette with correct
masks/layout; unused-region writes are ignored. Byte order through the
production VIDBUF assembly — the t05h caveat — was closed by the calibrated
p1_video bench (p1a).

RGB widening (P2's second focused commit): motherboard red/green/blue ports
are now 4-bit. Plus mode carries ASIC palette nibbles natively to a new
expansion stage before the video mixer; classic mode keeps the netlist
{level, OE_N} pair unchanged in the low two bits feeding color_mix exactly
as before, so classic video output is bit-identical by construction. The P1
lvl4_to_ga lossy adapter is gone.

Open P2 items: the phase exit "static Plus palettes display correctly
(Burnin' Rubber title)" needs the manual hardware checkpoint (this RBF plus
a real .cpr). The magnification write-mirror on offset +3 remains the ⚠
ASIC-REF §4 conflict note (+3 stores Y-high here pending hardware
verification). ADC/DMA behaviour stays unmapped-rule until their phases.
No-write-through into real SDRAM is enforced by the suppression terms and
verified by construction/mux inspection; no bench drives the full top-level
memory path yet.

The branch was merged into `accc-review-and-fixes` on 2026-08-25 at merge
commit `0dabcb8`; see the open review-debt row for the outstanding second
pass over the post-review fix delta and the skipped-vector residual.

### P4 sprites — engine implemented on `plus/p4-sprites` (2026-08-25)

`rtl/plus/asic_sprites.v` implements the [KT] coordinate model literally
(asic-reference §5): vertical compare on `{LINE, ROW&7}` (not gated by R6),
horizontal window on a free-running 10-bit dot counter cleared at the CRTC
character wrap — so the documented "R0>64 repeats horizontally" falls out of
the scale wrapping at exactly 64 characters — magnification codes
01/10/11, transparency at nibble zero, lowest-index priority, colour c ->
palette entry 16+c. Rows stage in DUAL banks: at each line seam a
row-tag-matched inactive bank is promoted (zero-latency swap) while the
background walker speculatively fills it with predicted row+1; mismatches
(Y/mag rewrites, first frame) fall back to urgent refill. X rewrites cut
the live window via a shadow; CPU pixel-data accesses blank that sprite only
and flush its staged banks through `asic_regs`' new access indicator.
`asic_video` gained HWRAP plus the final border > sprite > screen mux under
HSYNC force-blank; the motherboard instantiates the engine; `files.qip`
carries it with instantiation.

Vectors: s01-s10 PASS (disabled codes, placement/transparency, Y-formula
masking incl. the ROW&7 pin, x/y/quad magnification bounds and row
duplication, priority chain + 16-stack, palette mapping/order, X extremes +
wrap-through + negative alias, R0>64 repeat). asic_video t06a-c pin the
precedence mux; asic_regs a09/a10 pin the fetch-port handshake/preemption
and access-indicator decode.

**VECTOR STATUS (all 14 green, 2026-08-25):** s01-s14 all pass with zero
skips; the runner counts skips separately and exits 65 if any vector is
skipped, so gates cannot silently accept a disabled vector.

Two independent review passes shaped this phase. Pass 1 (Codex/GPT-5.6
Sol high, NOT CLEAR) found: suppressed port completions stranded sreq
bits forever (fixed — the handshake always completes; only the payload
write is conditional, scoped to words whose sprite matches ACC_IDX);
the disabled-sprite block jump advanced walk[7:4], crossing bank+sprite
and starving odd sprites; and an out-of-window s11 assertion.

Pass 2 (same route, also NOT CLEAR on the first remediation delta)
found the deeper truth and closed the phase: (a) the pass-1 walk-jump fix
had overcorrected — preserving walk[7] trapped the walker inside one
bank half whenever sprite 15 was disabled, which is the common case;
the skip now advances the {bank,sprite} block number with carry across
the half boundary. (b) The "exit 65" skip accounting existed only in
prose; it is now real code. (c) The long-standing "post-flush cross-seam
refill incompleteness" residual was never an RTL defect at all: s11 had
been configuring its target sprite with mag code 0x5 = X1/Y1, whose
window is ONE character, so the char5 recovery assertions sampled where
the sprite correctly never appears. With mag 0xA (X2/Y2) and correct
per-char expectations, plus the s12 cut bound moved to dot 6 (the
rewrite lands after dot 5 is sampled), every vector passes honestly.

One measured behaviour is pinned as a documented model choice rather
than an S5 rule: a pixel-data access flush landing mid-walker-lap leaves
the accessed sprite invisible for up to roughly four further characters,
because the single continuous fetch server finishes its current sweep
(including speculative work) before revisiting the sprite's active
block; recovery is complete and byte-correct by the same source row's
window on the next display line (s11 pins exactly this). Reference §5
fixes only THAT-sprite-only scope and image integrity, not hole shape.
Future optimisation if hardware ever needs it: urgent-first scheduling
of active-bank misses ahead of the speculative sweep.

The review thread ran five passes total and closed CLEAR. Pass 3
confirmed the walker block-carry sound but exposed a pre-existing race:
an issue on the seam edge captured pre-edge bank state while seam
maintenance retagged it, so a delayed ACK could land row-N data into a
bank retagged N+1. Fixed by completion-time validation: each request
carries its source row and is accepted only while the target bank still
holds that tag. Pass 4 cleared that mechanism but found its own pair: a
pixel-data WRITE during an in-flight fetch to the same sprite returned
the pre-write byte after ACC_EN dropped (the port serves grants through
CPU writes), fixed by poisoning the request for its whole life via
fq_acc; plus an s11 oracle bug (source row under Y2 is (12-8)>>1 = 2,
not 4) and an s12 early-arm coverage gap. Pass 5 reviewed exactly that
delta and returned CLEAR with no findings, clearing the review-debt
row.

mobo bench m8 (2026-08-26, merged `2a221e1`) closed the phase's last
build item: the first end-to-end sprite vector through the production
chain. The scripted fake CPU programs sprite 0 over the ASIC page
(X=0x166 via the x_hi byte, Y=16, MAG x1/x1), a bench-CPU auto-fill
phase writes the whole 16x16 image with the low-nibble mask exercised
on every write, and pal[21]/pal[26] carry distinct payloads. The scan
derives expectations on paper — [KT] compare formulas, reference S5/S6,
the engine's {G,R,B}->{R,G,B} emission swap, and asic_video's
registered RGB output lagging the engine plane by one dot — and
requires exactly three 16-dot SPR_EN windows on each compare line
16..31 per frame (and nowhere else) with alternating palette payloads
on the top-level RGB pins. Two review passes over the scan harness
ended CLEAR. The finding en route was fixture-side, not RTL: rows 1-15
unwritten meant transparent pixels, and the engine had been correct all
along.

Still open from this phase: the INKR-effects ~1/2-us-late GA pipeline
question noted in P1 remains deferred; P4 hardware checks land with the
next board milestone.

The synthesis-cost audit's Plus-track finding was probed 2026-08-26 and
resolved as accepted cost: the decode-split walker increment
(`c09534c`, review CLEAR, cycle-exact) did not lift the fitter cliff —
fit stayed at 15:40 in the post-cliff band — falsifying the carry-chain
hypothesis. The cost tracks the walk half bit gaining any next-state
logic; the pipelining alternative is rejected on vector-pinned-latency
risk. Full record in `docs/plans/2026-08-26-synthesis-cost-audit.md`
(remediation-outcome section).

Milestone CI (workflow_dispatch run `32892544906`, Quartus 17.0.2,
commit `e3dd848`): simulation, policy, exact synthesis all green.
Fitter: 29,893 / 41,910 ALMs (71 %); 37,506 registers; 685,217
block-memory bits (12 %); 34 / 112 DSP blocks; worst-case setup slack
+0.436 ns, hold +0.179 ns — positive; no regression signal. Versus the
P2/P3 merge milestone (`5d6d342`: 16,198 ALMs / 39 %) the ~13,700-ALM
growth is the sprite staging arrays (dual 8-byte banks x 16 sprites with
request/delivery bitmaps) plus the engine datapath landing in fabric for
the first time; memory bits unchanged (staging is flop-based by design).
RBF retained as artifact `Amstrad-build-104-1`
(`Amstrad_20260825_e3dd848.rbf`). Not hardware-tested.

### P5 CRTC-3 bus semantics — implemented on `plus/p5-crtc3-bus` (2026-08-26)

Two code commits close the deterministic P5 scope. `3891213` implements the
ACCC v1.10 §21.2.3 modulo-8 map
`{R16,R17,STATUS1,STATUS2,R12,R13,R14,R15}`, full-byte R12 storage/readback,
stored R14/R15, and the §21.3.4 live status groups. `8523136` connects the
Plus CRTC output to the CPU wired-AND path only under `plus_mode`, keeps the
live CPU byte on CRTC DI during IN cycles, and propagates read transactions to
the unlock/RMR2 observers. The MMU converts held Z80 I/O levels to one strobe
per transaction before those one-shot side effects.

Vectors t07a-t07g cover the map, storage widths, both `&BE`/`&BF` read ports,
horizontal/vertical status boundaries, VMA preview and the 16-frame timer.
MMU tests hold read/write cycles for four system clocks and prove a read byte
inside the unlock stream plus an IN-carried RMR2 payload. Motherboard m9 proves
the production mux and CRTC data/select plus GA traps; m7 proves classic mode
retains its existing type-0 `&BE`/`&BF` behavior. Full simulation and lint pass;
classic soak remains `0x85b3f8e847430495`. Hardware SHAKER/CRTC3 evidence and
the shared Plus title checkpoint remain pending.

### P3 interrupts — implemented on `plus/p2-asic-regs` (2026-08-25)

The programmable raster interrupt lives inside `asic_ga_timing`, where the
classic 52-line counter it modulates lives: with PRI=0 the new block is
inert and the GADIFF lockstep equivalence is untouched; with PRI!=0 the
counter keeps running but its assertion is suppressed, and an interrupt
fires at the trailing edge of the shaped monitor HSYNC when
{VC5..VC0,RC2..RC0}=={0,PRI}. The comparison's bit-8 don't-care produces
the documented n / n+256 aliasing. Vertical adjustment gates firing. A
raster fire pokes counter bit 5 (as an acknowledge would), so a later
re-enabled CPC-compatible interrupt cannot occur within 32 lines.
Clearing is shared with the classic path: CPU acknowledge or MRER bit 4.

DCSR became field-wise: bit 7 is a merger-driven read-only level ("last
INT ack was raster"), bits 6:4 are write-1-to-clear DMA flag storage
(set-paths arrive with P7's INT instruction), bits 2:0 are plain R/W
enables. IVR/vector supply: on every INT acknowledge the ASIC drives
(IVR & &F8) | source; with DMA absent the source field is raster (%110)
while a raster interrupt pends, else 0 — the no-pending behaviour is
unspecified on hardware (named assumption). The motherboard detects the
acknowledge cycle and Amstrad.sv gives the vector top priority on the
CPU data mux. The A13 vectored-interrupt bug stays deliberately not
emulated (architecture §5.4 decision).

Vectors: `pr01`–`pr04` (exact 52-line cadence at PRI=0; suppression plus
aliased fires at identical intra-line offsets; adjustment gate; MRER
clearing a pending PRI interrupt), `a08` (DCSR bit 7 mirrors the merger
level), mobo bench `m6` (ack-cycle vector byte 0xDE after a scripted
IVR write). P3's remaining exit item is title-level stability (Pang,
RoboCop 2) at the next hardware checkpoint.

Open scope note: the monitor-trailing-edge trigger uses this model's
fixed four-character shaping microsequence, so [ARNOLD-REV]'s "clamp at
HSYNC_start+6µs" is covered by construction here; [KT]'s conflicting
"~10µs" measurement stays recorded as ⚠ ASIC-REF §7.

Independent review (two passes, 2026-08-25, record in
`docs/plus/archive/p2p3-independent-review.md`): Claude Opus 5 xhigh on
invariance/PRI/seams returned five blockers — all real, headline being
undeclared top-level wires that corrupted every ASIC-page read while
Quartus warning 10236 sat inside green synthesis, and an intack-polarity/
sampling pair that inverted DCSR bit 7 and collapsed the vector source.
GPT-5.6 Sol high on asic_regs conformance returned two blockers: a reset-
dominance regression in the page-write branch and unobservable w1c flags
(now settable via the new dma_int_set lines ahead of P7). Everything is
remediated at the tip with new vectors pr05, strengthened m6/m7 and
extended a02-a06; both passes' residual items are recorded in the review
document.

Tooling policy: hosted CI and the Ansible VM provisioning configuration both pin
**Verilator 5.052** from the exact official upstream release matching local
development. The earlier hosted 5.020 package rejected the YM2149 unpacked
array initializer and the P10 input fixture's mixed scheduling, which led to
temporary compatibility edits. Those edits were removed when the toolchain
pin landed; new RTL and benches target the repository-pinned simulator rather
than whichever older package a Linux distribution happens to carry.

## Build and tooling state

- `.github/workflows/build.yml` runs local-style Verilator tests/lint before a pinned Quartus
  17.0.2 synthesis job and uploads the RBF plus fitter/timing reports.
- Since 2026-08-26 that synthesis job has two effort tiers: routine default/integration-branch
  pushes compile at smoke fitter effort (`scripts/ci/apply-quartus-effort.sh` appends
  FAST FIT / physical-synthesis-off overrides; a log guard fails the leg on any
  `Ignored assignment:`), while PRs, tags, and manual dispatches keep full effort and are the
  only hardware-build evidence. Simulation and synthesis run in parallel behind one gate.
  Trigger rules, measured figures, and the dispatch `both` benchmark mode are documented in
  `docs/ci-testing-policy.md`; the cost audit that motivated it (the P4 pass-2 fitter cliff)
  is `plans/2026-08-26-synthesis-cost-audit.md`.
- `actions/checkout` was bumped v4 → v7 (2026-08-25, standalone CI-only commit `4e776f1`)
  to clear the per-run Node 20 deprecation warning; the first CI run on this branch is the
  online check that v7 resolves. `actions/upload-artifact@v4` also sits on the deprecated
  runtime (review N-6) and is the next standalone CI-only bump.
- `ansible/` provisions the Debian 13 arm64 UTM guest (reached as `quartus-vm.local` over
  mDNS), restores the build user's supplementary groups, mounts Rosetta, registers amd64
  binfmt, and validates
  a real amd64 binary. It also creates a private installer staging directory and provides a
  read-only checksum preflight for the exact Altera 17.0.2 payloads. Quartus itself still
  requires a human download and interactive EULA step; then run `ansible/post-install.yml`
  and `ansible/validate.yml -e quartus_required=true`.
- ACCC v1.11 French is now the primary written baseline; English v1.11 is a working
  translation. The section-complete language audit is
  `accuracy/accc-1.11-fr-en-differences.md`. Digests are navigation aids: consult the French
  PDF whenever a rule claim matters and render tables/figures when layout carries meaning.
- The local `docs/references/ACCC1.11-FR.pdf` and `ACCC1.11-EN.pdf` are user-owned source
  material and must remain outside commits. The reproducible v1.11 extraction snapshot is
  versioned under `accuracy/extract/`; retained v1.10/v1.9 editions and unselected generated
  intermediates stay untracked.
- The local example cartridge `docs/plus/references/cartridges/crtc3_v2fix.cpr` is likewise
  deliberately untracked. Use it as a real RIFF/CPR parsing fixture when P0 starts; do not
  make it a build dependency or redistribute it from this repository.
- The complete deterministic F12/F4 counter milestone and CPR parser have a synthesized CI
  build at `5ddddef`, which was hardware-tested on 2026-08-19 as described above. The F8 work
  on top of it has a synthesized, not yet hardware-tested, CI build at `4c78603`.
- Independent review: the six per-commit `review-debt.md` rows were repaid on 2026-08-22 by
  same-model independent review under the 2026-08-22 locked decision (cross-provider review
  unavailable on this harness); findings became action items A1-A5. The whole-diff review of
  this branch plus the type-split branch then ran on 2026-08-23
  (`accuracy/archive/accc-review-and-fixes-independent-review.md`): split RTL accepted as sound; its
  blocking findings (broken GA40010 co-sim manifest, stale handoff/F6 premises) and
  non-blocking ones (soak claim bounds, sweep leftovers) are addressed on this branch. New
  work here still deliberately takes no per-commit debt rows — stream branches cut from this
  tip rebase onto these fixes.

## Next-session order

Use the [current roadmap queue](implementation-roadmap.md#8-immediate-execution-queue),
which incorporates the September 8 production-boundary findings. Older dated
hardware and implementation records above are evidence for their named source;
their old branch names, hashes and proposed tasks are not the current launch plan.

- B8-1 through B8-7, artifact delivery and worktree cleanup are complete as
  recorded above. The September 9 hardware retest still exposes defects; use
  its capture index for a dedicated diagnostic session before choosing repairs.
- Extend the bounded executed-T80 evidence only where a specific missing
  interaction warrants it. Hardware DSC4/SHAKER, IA-5/Q17 and Plus title
  acceptance remain open.
- Use the integrated bounded B3 capture CLI within its documented CPU/clock
  limits. Preserve private evidence and stashes until their distinct remaining
  acceptance questions are resolved.
- Use the prepared B2 host driver when device access returns; real capture
  repeatability and selected-machine identity require the MiSTer. CSL/SSM
  execution and exact event capture remain separate work.

F10 and F14–F18 are implemented within their recorded scope. Physical light-pen
capture is optional and is not the completed F18 readable-register validation.
The latest accepted CI/artifact identities are at the top of this document.
