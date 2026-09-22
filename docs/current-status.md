# Current status

The handoff for a fresh session: where each stream stands, what is integrated, and which
hardware or validation questions are still open. It states the current position only. Commit,
review-run and RBF identities for past milestones, and the order in which they landed, are in
the [status history](archive/current-status-history-2026-09-21.md). Work order lives in
[implementation-roadmap.md](implementation-roadmap.md) (section 8 is the queue) and
[backlog.md](backlog.md); open review obligations are in [review-debt.md](review-debt.md).

Update this file when a stream's position changes. Put the dated evidence (hashes, run IDs,
reviewer verdicts) in the task's own record or a dated hardware report, and link it from here.

## Latest integration and artifact

- **Latest implementation integration:** `37ccfc3` (2026-09-22), merging B16 load-time
  Plus selection and B18 classic snapshot capture/round-trip acceptance. Exact-SHA CI
  [35673603710](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35673603710)
  passed simulation/lint, production-T80, full hosted synthesis and the required gate.
  Delivered RBF: `output_files/Amstrad_20260922_37ccfc3.rbf` (ignored), SHA-256
  `a3c9fb8eba80a64df2928bc55d2cca7d68a9a302121b6678dad07ded04a3780c`.
  Full fit uses 23,675 ALMs (56%); worst setup/hold +0.553/+0.241 ns, zero TNS.
  This RBF has not been device-tested. Source/review/artifact details are in
  [the integration record](investigations/session-logs/b16-b18-integration-2026-09-22.md).
- **Latest hardware-tested builds:** `88262b9` (2026-09-14, B19 residual; Copter 271 title
  flash fixed) and `a0778b6` (2026-09-13, PSG R7 reset; keyboard and joystick fixed). B18 SNA
  save was device-tested on branch RBF `0608653`.
- **Simulation baseline:** 225 required classic CRTC vectors with no expected failures, and
  canonical soak hash `0xb1cb70da95c2e44f`, unchanged since the D1/D6 repair. Since
  2026-09-15 the gate runs only the benches a change can break (`sim/select_tests.py`, index
  in `sim/TESTS.md`).

## Classic CRTC accuracy (types 0 and 1)

**Position.** Findings F1-F20 are implemented within their recorded scope at the
deterministic-model level; see [audit-findings.md](classic/audit-findings.md). The latest RTL
change is the D1/D6 parity repair (source `2d04812`, 2026-09-11). The bottleneck is hardware
observability, not implementation: further classic RTL work should start from a hardware
discrepancy or a rule the RTL is predicted to violate, not from blanket coverage.

**Open hardware and validation questions:**

- SHAKER B (9) on both types and C (4) on type 1 await a hardware retest of the D1/D6 repair.
- DSC4 and SHAKER still fail on hardware (2026-09-09 retest); the changed failure shapes are
  not yet characterized. Amazing Demo keeps lower-screen corruption.
- F13 (type-0 half-character DE) and F20 (CRTC-1 R2.JIT HSYNC start) are implemented but need
  their named hardware gates: DSC4 and SHAKER `(TAB)`. See roadmap section 4.
- IA-2, IA-3 and IA-6 rest on source-model evidence only. IA-5 needs a simultaneous capture of
  raw CRTC HSYNC/VSYNC and GA `INT_N`.
- Q17 is not closed: the adjustment arithmetic and the simulation predict R7=39 silence, while
  ACCC §28.1.1 predicts a pulse. Hardware must discriminate.
- The absolute adjustment-entry pointer in the D1 p.82 addressing pins is not
  source-adjudicated.
- Production T80: four executed OUT(C)/OUTI cases pass, but native dynamic-wait equivalence
  and full motherboard execution remain open. Do not infer production instruction behaviour
  from the TV80 substitute.

## Plus / GX4000 ASIC

**Position.** The P-2 to P9 functional milestones and the P10 compatibility repairs are
integrated. Remaining work is title-driven and evidence-gated: roadmap rows P10f/P10g list the
capture each screenshot family needs before any RTL change, and the named model assumptions
(sprite `+3` mirror, coordinate formula, PRI offset, lowered R0, R3-low collision, pixel phase)
stay assumptions until a source or hardware discriminator settles them. The latest RTL change
is the B19 residual fix, integrated at `88262b9` (2026-09-14).

**Hardware-confirmed:** 6128 Plus BASIC boot (`5c16b17`); Copter 271 logo palette (PRI line
compare, `b5c3014`) and title flash (`88262b9`); Pang, Plotting and `arn5diag` input
(`a0778b6`); the Burnin' Rubber and CRTC3-demo right-edge sprite leaks.

**Open:**

- Copter 271: vertical-scrolling issues during gameplay (possibly pre-existing).
- Sonic GX: display severely broken by video/split timing defects (Hazard 2); the B19 check was
  inconclusive. See `investigations/sonic/`.
- Left-edge sprite corruption is much improved, perhaps fixed; closure is open. Navy Seals
  left-edge sprite flicker remains; its black-screen report was not reproduced and has no
  assigned cause.
- CRTC3 demo: warning, audio and crash defects remain. Switchblade and other cartridge
  crashes, CPC+ SNA/reset/reload recovery and odd-R5 CRTC3 behaviour remain evidence-gated.
- Disk path: 464 Plus boot, cartridge/empty-drive details and disk I/O are unrecorded. The
  System CPR disk read is held at `XFAIL fdc-payload-poll`; see "Open Plus evidence
  boundaries" in the roadmap.
- D3/D4: post-BSR control readback and title/flicker hardware acceptance.
- B8-2, B8-3, B8-5 and B8-6 are integrated without hardware acceptance. B8-5 snapshot apply
  approximates first-frame pixels where the SNA omits address/phase history.
- Status-2 16-frame timer: the source conflict and hardware confirmation remain open.
- **B16 implemented:** valid CPR loads select 6128+ when Plus is Off; SNA v3
  model headers 4/5/6 select the matching Plus model before restore. Existing Plus
  selection is preserved for CPR. Independent review and selected simulation pass;
  Main/OSD echo, boot and restore still need device acceptance. See
  [the load contract](plus/b16-load-model-2026-09-22.md).
- The PRI line-compare change is unreviewed (review debt).

## General: video path, peripherals, harnesses and tooling

- **B1 video ownership** is open as an ownership/observability problem. Full versus Raw pixels
  shows no visible difference so far (Amazing Demo, DSC4, SHAKER A); Pulpo awaits retest.
- **B6 video boundary** is integrated (`5c16b17`) with its rendering follow-up. Physical
  HDMI/CRT acceptance and a CPU-generated stuck-high raw-sync recipe remain open.
- **Capture and oracles:** the B2 device driver captures repeatably (native screenshots omit the
  OSD, so they cannot prove the active mode). B4 CSL/SSM Phase 1 is verified on hardware for
  Module A on both CRTC types and Gate 1 (Module B); the workbook's 712 rows remain coverage
  targets. Source support is updated to CSL v1.5 and SSM v1.2: `wait_ssm 0xHHLL` is available,
  legacy `wait_ssm0000` and SHAKER 2.6 remain compatible, and the existing detector already
  enforces four consecutive opcode fetches. A bounded live `wait_ssm` check remains before
  claiming device acceptance, but it can use an existing Phase 1 SSM-capable RBF because the
  refresh changes no hardware logic. The B3 frame-capture CLI works within
  its CPU and clock limits. The AmSpirit oracle
  (`scripts/amspirit/amspirit.py`) captures reference checkpoints. See the
  [mister-capture skill](../.agents/skills/mister-capture/SKILL.md) and
  [driver guide](investigations/hardware-runs/mister-hardware-loop-driver.md).
- **B18 SNA save** is integrated as a development aid only: the core cannot write to SD, so
  shipping it needs a Main_MiSTer change. The reviewed production-T80 capture/round-trip fixture now covers six classic
  model/CRTC combinations and host publication races (acceptance 2 and 3). Open:
  a 464/664 device save, SD transport, and the existing slice 3-4c review debt. See [b18-sna-save.md](b18-sna-save.md).
- **Peripherals:** FDC full-sector result/ST1, classic AMSDOS and hardware acceptance are open.
  B8-7 tape real-CDT playback and HPS cadence are open. B8-4 video-word coherence has no
  hardware acceptance.
- **Tooling:** the task-workflow host smoke tests (B14) are open. B17 input record/replay is a
  proposal, not started. CI and synthesis routing are in
  [ci-testing-policy.md](ci-testing-policy.md); local setup is in [building.md](building.md).

## How hardware testing fits the loop

SHAKER is **not** part of the automated loop. The automated loop is the selected Verilator
benches plus GitHub Actions synthesis. SHAKER and title sessions are manual, user-run, and
happen at significant milestones against a target list recorded before the session (roadmap
section 4 lists SHAKER targets per checkpoint; [plus/hardware-test-checklist.md](plus/hardware-test-checklist.md)
is the Plus title matrix). A green simulation gate is never evidence of hardware accuracy, and
a manual session never gates a commit. Dated results go under
[investigations/hardware-runs/](investigations/hardware-runs/).
