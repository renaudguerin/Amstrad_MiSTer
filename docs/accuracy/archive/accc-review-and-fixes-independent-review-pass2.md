# Independent pass-2 review — accc-review-and-fixes

## Reviewed range and tip

- Base: 2d4f880eefe1d992f31487bdfa8f4e3a0cd85271
- Tip: 0773ad47369f983094201c142122f6e2a1425d29
- Reviewed delta: git diff 2d4f880..accc-review-and-fixes
- Branch: accc-review-and-fixes
- Worktree: clean before and after review.
- Tip 0773ad4 differs from its tested parent 69da513 only by the pass-2 brief.
- The user-owned ACCC PDF remained untracked and unchanged, SHA-256 1bd6f0e3a06022d03fd40b51d4d622afef2675954a483780f0922cdf1e33a560. ACCC source checks used that local PDF: :codex-
  file-citation{path="/Users/renaudg/code/Amstrad_MiSTer/docs/ACCC1.10-EN.pdf" purpose="source"}

## Overall verdict

Blocking issues found.

The classic A3/F6 Stage 1 work and soak expansion are acceptable within their declared boundaries, but the branch cannot be approved because:

- Plus P0 has two load-bearing request/response failures.
- Plus P1 suppresses a documented live-R2 HSYNC collision.
- The GA40010 manifest still cannot elaborate cleanly.
- Canonical handoff documents have regressed after their remediation.
- The required git diff --check gate fails.
- Differential evidence overstates its sampled-state coverage.

## Verdict by area

 Area                        Verdict                                       Summary
━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Pass-1 remediation          Blocking                                      Issues 3, 4, and 6 closed; issues 1 and 5 remain open; issue 2 regressed.
──────────────────────────  ────────────────────────────────────────────  ───────────────────────────────────────────────────────────────────────────────────────────────────────────
 Classic A3/F6 Stage 1       Approved within declared approximation        t20i, comparator, injection point, type isolation, skew behavior, and first hash re-mint check out. Exact
                                                                            0.5 µs F13 behavior remains hardware-blocked.
──────────────────────────  ────────────────────────────────────────────  ───────────────────────────────────────────────────────────────────────────────────────────────────────────
 Plus A5/P0                  Blocking                                      Parser and atomic publication are sound, but timeout/cancellation behavior is unsafe at the MMU/service
                                                                            boundary.
──────────────────────────  ────────────────────────────────────────────  ───────────────────────────────────────────────────────────────────────────────────────────────────────────
 Soak expansion/re-mint      Approved                                      Only three sampled fields changed; seed, schedule, budget, phase, and RTL did not.
──────────────────────────  ────────────────────────────────────────────  ───────────────────────────────────────────────────────────────────────────────────────────────────────────
 F6 Stage 2/2b/F13           Approved as measurement/documentation-only    No production RTL change. F13 is honestly hardware-blocked. The recorded temporary measurement cannot be
                                                                            rerun from the checkout.
──────────────────────────  ────────────────────────────────────────────  ───────────────────────────────────────────────────────────────────────────────────────────────────────────
 Plus P1                     Blocking                                      Most named counter/pointer/DE/VSYNC checks pass source review, but dynamic R2 HSYNC collision behavior is
                                                                            wrong and one R0 test oracle is under-sourced.
──────────────────────────  ────────────────────────────────────────────  ───────────────────────────────────────────────────────────────────────────────────────────────────────────
 Differential evidence       Open                                          Frozen provenance and default run are correct; private-state coverage is overstated.
──────────────────────────  ────────────────────────────────────────────  ───────────────────────────────────────────────────────────────────────────────────────────────────────────
 Documentation               Blocking                                      Canonical current-state, queue, counts, hashes, and wrapper paths conflict with the tip.
──────────────────────────  ────────────────────────────────────────────  ───────────────────────────────────────────────────────────────────────────────────────────────────────────
 Test/assertion integrity    Open                                          XFAIL/XPASS behavior is correct and no assertion was weakened, but P0 integration coverage and P1 t01e do
                                                                            not support their current claims.

## Findings

### 1. Critical — legitimate CPR loading hits the MMU watchdog and returns open-bus data

Grounding:

- rtl/plus/plus_mmu.v:84 fixes the stall timeout at 1,024 clocks.
- rtl/plus/plus_mmu.v:179 drops the request and returns 8'hFF on timeout.
- rtl/plus/plus_cartridge_memory.v:21 clears 524,288 bytes before publication.
- rtl/plus/plus_cartridge_memory.v:251 gives clearing/loading priority over CPU requests.
- Amstrad.sv:672 does not include cpr_download in machine reset.
- docs/plus/architecture.md:165 instead claims reads during download stall until loading finishes.
- The P0 boot bench connects parser → service → SDRAM but omits plus_mmu, as shown by sim/plus/p0_boot_test_top.v:64.

Impact: a real CPU cartridge fetch during the guaranteed-long clear/load interval times out after 1,024 clocks and completes with FF, rather than waiting for atomic publication.
This can let the running CPU execute open-bus bytes during a .cpr load.

Resolution evidence: an integrated MMU + production-sized cartridge service + SDRAM test must exercise a read during the clear/load interval and demonstrate the accepted post-load
behavior without watchdog fail-open.

### 2. Critical — late backend acknowledgements can satisfy a later cartridge read

Grounding:

- The MMU abandons cart_valid without a cancellation or request-generation tag at rtl/plus/plus_mmu.v:179.
- The service rearms solely when cpu_valid drops and keeps a physical request until acknowledgement at rtl/plus/plus_cartridge_memory.v:124.
- CPU completions carry no identity at rtl/plus/plus_cartridge_memory.v:285.
- The watchdog test uses a mock backend and immediately supplies a fresh response after timeout; it does not inject a late old response: sim/plus/plus_mmu_test.cpp:350.

Impact: after timeout, the service may still own the original physical request. If its acknowledgement arrives as a new MMU read enters CART_WAIT, the old cart_ready pulse is
accepted and the following capture edge returns stale data for the new address. A permanently wedged request also means the watchdog releases the CPU without actually recovering the
service.

Resolution evidence must cover:

- Timeout followed by late acknowledgement and a new address.
- Bus-cycle abandonment followed by a new request.
- load_begin while a CPU physical request is pending.

Each must prove that an old completion cannot satisfy a newer read and that service ownership re-arms cleanly.

### 3. High — the GA40010 dependency-manifest remediation is still incomplete

rtl/GA40010/Makefile:8 now includes both CRTC engines, but still omits casgen_sync.v, which is instantiated unconditionally at rtl/GA40010/ga40010.sv:159.

A clean elaboration using the checked-in manifest files failed:

%Error-MODMISSING: rtl/GA40010/ga40010.sv:159:1:
Cannot find file containing module: 'casgen_sync'

Appending rtl/GA40010/casgen_sync.v to that same command produced successful elaboration with warnings only. Therefore pass-1 issue 1 was not closed by 90f0cda; it merely exposed
the next missing dependency.

Resolution: the checked-in dependency route must cleanly elaborate from scratch and reproduce its claimed lint/render gate without relying on stale obj_dir output.

### 4. High — P1 suppresses the documented dynamic R2 HSYNC collision

rtl/plus/asic_video.v:359 converts the type-1–4 collision rule into a static W mod (R0+1)==0 relation. At rtl/plus/asic_video.v:392, an active HSYNC remains asserted on an end/start
collision only if that static relation is true.

ACCC §15.3.5 p.151 specifically demonstrates CRTC3/4 live R2 rewrites causing C3 to continue/overflow when the rewritten start position coincides with the active pulse’s natural
end. That can occur when the static modulo relation is false.

Impact: a live R2 rewrite at the documented collision point incorrectly deasserts HSYNC. asic_video is not yet production-instantiated, so current RBF behavior is unaffected, but
the P1 foundation is not ready for integration.

Resolution evidence: a source-derived vector mirroring the p.151 type-3 chronogram must show the live end/start collision, C3 continuation, and correct HSYNC pin behavior. Existing
t04d covers only the static R0=0,R2=0,R3=1 case at sim/plus/asic_video_test.cpp:403.

### 5. High — canonical handoff state regressed after pass-1 remediation

Examples:

- docs/implementation-roadmap.md:21 presents 87 tests, old hash 0x5b5004ff70148443, tied-off parser/P-1 state, and no Plus boot as current.
- docs/implementation-roadmap.md:313 still schedules the already-landed P1 foundation.
- docs/current-status.md:23 calls 4c78603 the newest synthesized milestone, despite later P0/P1 CI evidence.
- docs/current-status.md:63 says everything after 4c78603 is behavior-preserving and cites the old hash, although F6 Stage 1 intentionally changed behavior.
- docs/current-status.md:167 says the cartridge path is tied off, then docs/current-status.md:184 says it is live.
- docs/current-status.md:302 again schedules the completed P1 foundation.

Impact: a fresh session can use the wrong soak contract, rerun completed work, or select obsolete hardware evidence. This reopens pass-1 issue 2.

Resolution: reconcile the canonical baseline, CI milestone, current test/hash state, live P0 state, completed P1 foundation, and next P1-remainder milestone at the reviewed tip.

### 6. High — required git diff --check gate fails

The command reports 201 trailing-whitespace errors, all in Amstrad.sv:57. Representative added regions begin at lines 57, 228, and 617; blame assigns them to P0 commits 09487a16 and
eb390812.

Impact: a required acceptance gate is red even though behavioral tests pass.

Resolution: the exact range must produce no git diff --check diagnostics.

### 7. Medium — differential evidence omits a type-1 private sequential latch

The Sample structure and new/reference extraction paths at tools/split-differential/diff_main.cpp:21, tools/split-differential/diff_main.cpp:166, and tools/split-differential/
diff_main.cpp:201 sample status_bit5_r but not the private r6_border_condition latch at rtl/crtc_type1_engine.v:182.

This contradicts the “both engines’ private latches” and “none of the sequential state is known to be unsampled” claims in tools/split-differential/README.md:18.

Impact: the successful 45.5M-sample run proves equality only for the actual sampled projection; a split error isolated to that latch could escape.

Resolution: either compare that private state on both frozen sides and preserve a new run log, or narrow the documented claim.

### 8. Medium — wrapper rename/current-path sweep remains incomplete

Current architecture text still calls the production classic path UM6845R at docs/plus/architecture.md:12, docs/plus/architecture.md:35, and docs/plus/architecture.md:69. The
session plan also retains current-looking UM6845R co-simulation and ownership statements at docs/plans/2026-08-22-accc-review-plan.md:122.

Impact: integration guidance points future work at a deleted file/module name. Pass-1 issue 5 remains open.

Resolution: use rtl/CRTC.v plus the two engines, or explicitly mark each passage as historical.

### 9. Medium — P1 t01e is not supported by its cited oracle

sim/plus/asic_video_test.cpp:313 asserts an exact 20→…→255→0→10 C0 sequence after shrinking R0 below current C0. ACCC §13.5 says CRTC3/4 accept all R0 values and §13.1 describes the
normal C0/R0 count, but the cited §28.1.1 is specifically about C4/C9 overflow, not C0.

Impact: the result may be correct, but the directed expectation is not independently established by the cited source, contrary to the repository’s vector discipline.

Resolution evidence: a direct CRTC3 rule/chronogram, Logon/hardware observation, or explicit unverified classification. §28.1.1 should not be presented as support.

### 10. Low — F6 point-of-use comments blur the full-character approximation with the 0.5 µs rule

rtl/crtc_type0_engine.v:362 and sim/sim_main.cpp:3649 describe the source’s 0.5 µs border byte while the implementation and character-granular assertions hold DE low for a full 1 µs
character.

The decision gate correctly discloses this as Stage 1 approximation, so this is not an unrecorded RTL defect.

Resolution: point-of-use comments and vector descriptions should explicitly say that the assertion pins the accepted full-character approximation, not exact ACCC pin timing.

### 11. Low — residual documentation counts and formatting remain stale

- sim/README.md:48 says 87 vectors; current result is 93.
- docs/accuracy/testbench-spec.md:145 retains the earlier 85-pass definition-of-done state.
- docs/accuracy/type-split-review-guide.md:116 still lists A3 and post-split streams as open/out of scope.
- docs/review-debt.md:155 duplicates its table header.

These are non-behavioral but weaken the durable handoff.

## Priority-check results

1. Pass-1 remediation — refuted overall.
    - Issue 1, GA manifest: open; engines are present, casgen_sync.v is still missing.
    - Issue 2, canonical handoff: regressed/open.
    - Issue 3, F6 premise: closed; the old GA-boundary assumption is superseded and F13 owns the unresolved phase.
    - Issue 4, soak claim/fields: closed.
    - Issue 5, wrapper paths: open/incomplete.
    - Issue 6, guide/Q15/Q16 numbering: closed.

2. Classic A3/F6 Stage 1 — confirmed for the declared approximation.
   t20i follows §§13.2.6/13.8.3/20.3.1. The F6 predicate is live unsigned R1>R0, substitutes C0=R0, is type-0-only, enters before SKEW-DISPTMG, and has vectors for type isolation,
   skew 0/1/2, and mode 3. The first behavior-driven hash re-mint to 0x326ea81358e7d88f is recorded consistently. Exact 0.5 µs timing remains unverified under F13.

3. Plus A5/P0 — refuted.
   Parser abort/backpressure, commit gating, service publication atomicity, classic-mode mux isolation, WAIT wiring, and same-commit files.qip inclusion are confirmed. The MMU/
   service timeout and stale-response failures block acceptance.

4. Soak expansion — confirmed.
   Commit 750ea32 adds only type0_vsync_wait_line_start, r6_border_condition, and status_bit5_r to the mix. It changes no RTL, seed, stimulus schedule, event budget, or sample
   phase. The current 0xf5f8ae01ffdf928d hash and both re-mint rationales agree in AGENTS.md, the plan, sim/README.md, and the split guide.

5. F6 Stage 2/2b/F13 — confirmed in scope; measurement reproduction unverified.
   750ea32..fb3205c changes no production RTL/top-level/manifest. The checked-in GA and motherboard paths support the boundary analysis. F13 is evidentiary and explicitly hardware-
   blocked. The temporary render/VCD harness is absent, so the recorded 199/199 seam counts and transitions could not be rerun.

6. Plus P1 — refuted.
   Attribution, point citations, C0=R1=R0 save/reload, R9 >= completion versus equality-only capture, R5 adjustment, C4=0/C0=0 reload, basic HSYNC widths/start, VSYNC re-fire, scope
   cuts, and deferred files.qip inclusion are confirmed. Dynamic R2 collision behavior is wrong; t01e is under-sourced.

7. Differential evidence — refuted in full.
   Defaults 418aa68/2d4f880, frozen extraction, explicit overrides, run totals, and exclusion of later intentional changes are confirmed. The “all private/sequential state” claim is
   false because r6_border_condition is unsampled.

8. Test/assertion integrity — refuted overall.
   No assertion weakening was found. XFAIL remains non-failing, XPASS remains failing, and the current suite has neither. However, P1 t01e lacks a valid direct oracle, F6 comments
   overstate exactness, and the P0 integration bench omits the MMU seam where both blocking failures occur.

## Mechanical verification

 Command                                            Outcome
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 git diff --check 2d4f880..accc-review-and-fixes    Exit 2 — 201 trailing-whitespace errors, all in Amstrad.sv; first at line 57.
─────────────────────────────────────────────────  ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 make -C sim                                        Exit 0 — classic: 93 passed, 0 xfailed, 0 xpassed, 0 failed; all Plus groups passed, including 26 P1 vectors and 5 P0 boot
                                                     tests.
─────────────────────────────────────────────────  ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 make -C sim lint                                   Exit 0 — warnings only, no lint errors.
─────────────────────────────────────────────────  ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 make -C sim soak SOAK_EXPECT=0xf5f8ae01ffdf928d    Exit 0 — seed 0xaccc5eed20260822; 2,845,088 characters/CLKEN samples; hash matched.
─────────────────────────────────────────────────  ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 tools/split-differential/run.sh                    Exit 0 — extracted 418aa68 and 2d4f880; type 0: 22,627,500 samples; cumulative after type 1: 45,498,863; no divergence in
                                                     sampled fields.

Additional GA manifest check failed on missing casgen_sync; adding that source explicitly made elaboration pass with only existing missing-pin warnings.

### GitHub Actions

There is no literal exact-tip run for 0773ad47369f983094201c142122f6e2a1425d29; that commit adds only the pass-2 documentation brief and is skipped by the workflow’s docs-only
behavior.

The code-equivalent parent has successful Build core run 32641514600 (https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/32641514600) at
69da51323bfa61f7c6ae3b05e4adbe3241911ad8:

- Simulation job 97199209183: success; behavioral tests and lint passed.
- Synthesis job 97199394596: success; Quartus compilation, reports, RBF packaging, and artifact upload passed.

Simulation supports the checked behaviors. Synthesis proves buildability of the production manifest, not CRTC/Plus hardware accuracy, and asic_video.v is deliberately absent from
that production build.

## Could not verify

- A real .cpr boot on MiSTer hardware.
- Classic mode side-by-side hardware regression after P0.
- SHAKER Module A (O) or DE-pin capture for F13.
- The exact 0.5 µs type-0 F6 event; Stage 1 remains a 1 µs approximation.
- The Stage 2 temporary-harness 199/199 render counts and VCD transitions.
- CRTC3 live-R2 and R0-shrink behavior against real hardware; ACCC remains the working oracle, not final authority.
- Unsampled phases/wires in either soak or differential comparator.
- Literal-tip CI, because no workflow run exists for the docs-only dispatch commit.
- Lower-risk ACCC claims outside the named priority edges were read and exercised by the suite but not all independently re-derived from the PDF.

## Review-debt disposition

- accc-review-and-fixes: remain open — GA manifest and canonical handoff remediation are not closed.
- accuracy/crtc-type-split: previous split-RTL review remains valid within its bounded evidence.
- accuracy/a3-f6-stage1: may clear for Stage 1/A3, with F13 remaining separately hardware-blocked and the point-of-use wording logged as low-priority cleanup.
- plus/p0-parser-wiring: remain open — both MMU/service protocol findings are blocking.
- docs/split-differential-evidence: remain open — provenance pinning is correct, but sampled-state coverage is overstated.
- plus/p1-crtc3-foundation: remain open — dynamic R2 collision and t01e oracle must be resolved.
- A1/A2 remain open as already recorded and were not expanded here.
- A5 parser observations can clear independently; they do not clear the broader P0 wiring row.
- Mechanical gates and successful synthesis do not discharge these review seams.
