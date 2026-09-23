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

- **Latest implementation:** B20-1 live PPR write handling (`5d12f56`), integrated
  with reviewed capture transport/metadata fixes and B20 acknowledge diagnostics.
  Combined exact-SHA CI and full Quartus synthesis pass at `cdcb3c3`.
  Original-hardware acceptance of the PPR change remains open.
- **Latest Classic implementation:** Classic F14 type-1 additional-line correction, source
  `5f72d66` (2026-09-22). The physical line no longer depends on R5; the added-line
  origin excludes simultaneous adjustment entry. Selected simulation gates and fresh
  review pass; integrated `95e6f56` also passes synthesis. Hardware confirms B9/type1
  page A; page B and C4 retain residuals, and type 0 remains inconclusive.
- **Plus baseline:** `b0e5bed` (2026-09-22) restores the three DMA
  behavior/test files exactly to `0e92c9c`, retaining the P8 test-selection
  dependency. The restore passes four selected benches and fresh Opus review;
  its integration build `0601050` passes all required CI jobs. DMA PAUSE candidate `190f4d3`, integrated at
  `a137d48`, **failed hardware acceptance as integrated**: it passed
  simulation, review and synthesis, but no longer reached Sonic gameplay in
  the matched input sequence. PRI and interrupt vector logic are unchanged.
- **Latest timing-clean artifact:** `cdcb3c3` (2026-09-22), including B20-1
  on the Classic F14/restored Plus baseline. Exact-SHA CI
  [35699931488](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/35699931488)
  passes simulation, production-T80, synthesis-policy, route, local Quartus and
  required-gate; hosted synthesis is intentionally skipped. Full-effort Quartus
  17.0.2 artifact `Amstrad-local-build-239-1-full` is delivered as
  `output_files/Amstrad_20260922_cdcb3c3.rbf`, SHA-256
  `f8bc3158826214d81ffeab09a311e78103f451add356247a70587592da6d63a1`.
  Setup/hold minima +0.622/+0.193 ns; zero TNS; 23,812 ALMs (57%),
  28,349 registers, 102 RAM blocks and 35 DSP blocks. Reports are in
  `output_files/reports-cdcb3c3/`. This artifact has not been hardware-tested.
  Candidate `a137d48` remains experimental; its progression failure and partial
  timing gain are preserved in the [rearm record](investigations/sonic/rearm-boundary-2026-09-22.md).
- **Latest hardware-tested builds:** `8b18ac0` (2026-09-23 user retest:
  Sonic GX confirmed fixed, Navy Seals / World of Sports sprite-line flicker
  appears fixed, Switchblade / original Eerie Forest CPR still not loading; see the
  [retest record](investigations/hardware-runs/plus-titles-8b18ac0-2026-09-23.md)).
  A subsequent same-build check confirms Eerie Forest boots after correcting only
  its malformed RIFF length ([finding](plus/eerie-forest-container-2026-09-23.md));
  `64702ac` (B20-7 DMA terminal-PAUSE resurrection;
  Sonic title screen 100% coherent without displaced bands; attract and player gameplay
  progression confirmed; six regression titles clean),
  `ef8da61` (master baseline with cartridge stall fix `03f4724`; tested in parallel with `64702ac`),
  `95e6f56` (Classic SHAKER partial acceptance),
  `0601050` (Classic B9/type1 first page only),
  `a137d48` (Sonic progression regression; experimental),
  `c59e03a` (Sonic baseline still corrupt but reaches gameplay),
  `88262b9` (Copter 271 title flash fixed) and `a0778b6` (PSG R7 reset;
  keyboard and joystick fixed). B18 SNA save was tested on branch RBF `0608653`.
- **Simulation baseline:** 232 required classic CRTC vectors with no expected failures, and
  canonical soak hash `0xe99ab434a5e1cdb3`, re-minted for the F14 type-1 additional-line correction. Since
  2026-09-15 the gate runs only the benches a change can break (`sim/select_tests.py`, index
  in `sim/TESTS.md`).

## Classic CRTC accuracy (types 0 and 1)

**Position.** Findings F1-F20 are implemented within their recorded scope at the
deterministic-model level; see [audit-findings.md](classic/audit-findings.md). The latest RTL
change before the September 22 retest was the D1/D6 parity repair (source `2d04812`,
2026-09-11). That retest exposed F14's missing type-1 additional line for R5=0 and
nondivisible R5; its correction is described in the linked evidence record. Further
classic RTL work should start from a hardware discrepancy or a rule the RTL is predicted
to violate, not from blanket coverage.

**Open hardware and validation questions:**

- Repaired build `95e6f56`: SHAKER B (9), type 1 page A matches all numeric
  reference rows, closing the observed F14 even-entry one-line deficit. Page B
  retains C0=3F (`2780` vs `2740`) and MID FRAME SIZE (`4E40` vs `4F40`)
  discrepancies. Type 0 produced no PNG and remains inconclusive. C (4), type 1
  states A–E, has capture/reference structural differences with cause unassigned.
  See the [repaired hardware record](investigations/hardware-runs/shaker-repaired-95e6f56-2026-09-22.md).
  Original CFG, MENU and owned-temporary cleanup are verified; no full D1/D6 closure.
  [Capture investigation](investigations/hardware-runs/capture-reliability-2026-09-22.md)
  identifies field-history evidence in C4 images, not a proven CRTC fault. Full
  was requested; the active Sync latch was not independently observed.
  A scratch production-T80 replay reproduces the B9 C0=3F stage-A/line-end
  collision and extra 64 µs raw VSYNC interval. The September 23 result-buffer
  follow-up now reproduces all five first-block SHAKER values (`2740, 2740,
  2780, 2780, 2760`), closing the displayed-value correlation gap.
  French ACCC v1.11 §19.5.3
  pp.210–212 and §19.8.2 p.226 do not order that collision, so the RTL is not
  predicted to violate a documented rule and remains unchanged. Counting the
  coincident line end in the new IVM mode is the only candidate ordering that fits all
  five photographed rows. Its hardware/author discriminators, and the setup,
  measurement and Sync-path limits, are in the
  [page-B discriminator brief](investigations/hardware-runs/shaker-b9-page-b-discriminator-2026-09-22.md).
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

**B20-1 integrated:** live PPR writes now advance the active PAUSE iteration,
including same-value writes, through registered CPU write events. The fail-first
regs-to-DMA matrix and nine selected benches pass; see the
[source and validation record](plus/live-ppr-2026-09-22.md). This is a source-model
repair with hardware acceptance still open. On device, `cdcb3c3` keeps the `c59e03a`
Sonic no-input progression (corrupt title, then playfield); see the
[device record](investigations/hardware-runs/device-acceptance-cdcb3c3-2026-09-22.md). Exact write/HSYNC phase remains
a model convention; no Sonic outcome is claimed.

**Position.** The P-2 to P9 functional milestones and the P10 compatibility repairs are
integrated. Remaining work is title-driven and evidence-gated: roadmap rows P10f/P10g list the
capture each screenshot family needs before any RTL change, and the named model assumptions
(sprite `+3` mirror, coordinate formula, PRI offset, lowered R0, R3-low collision, pixel phase)
stay assumptions until a source or hardware discriminator settles them. B19's residual
fix is hardware-confirmed at `88262b9`; the subsequent Sonic DMA PAUSE candidate
failed hardware acceptance and its behavior was restored in `b0e5bed`. Its
measured partial cadence improvement remains worth isolating, as described below.

**Hardware-confirmed:** 6128 Plus BASIC boot (`5c16b17`); Copter 271 logo palette (PRI line
compare, `b5c3014`) and title flash (`88262b9`); Pang, Plotting and `arn5diag` input
(`a0778b6`); the Burnin' Rubber and CRTC3-demo right-edge sprite leaks.

**Open:**

- B20-2/3: [integrated acknowledge diagnostics](plus/references/b20-ack-discriminator-2026-09-22.md)
  reproduce the synthetic empty-vector mismatch, but the production CPU matrix
  does not reproduce the second acknowledge. Physical bus timing remains unproven.

- Copter 271: vertical-scrolling issues during gameplay (possibly pre-existing).
- Sonic GX: title screen corruption is **hardware-accepted fixed on `64702ac`**
  ([device acceptance record](investigations/sonic/b20-7-dma-pause-acceptance-2026-09-22.md)).
  With the cartridge stall fixed (`03f4724`), the resurrected DMA terminal-PAUSE
  rule (`64702ac`, B20-7) eliminates the extra scanline wait on expiry, restoring
  frame-locked 312-line list recurrence and 8-line handler cadence matching AmSpirit.
  On real MiSTer hardware, the Sonic title renders with complete coherence (no displaced
  bands or torn copper rasters); no-input control reaches Green Hill Zone attract playfield;
  sustained fire input transitions cleanly through the Act 1 title card into live player
  gameplay. Zero regressions observed across Copter 271, Burnin' Rubber, Pang, Plotting,
  Navy Seals, and the CRTC3 demo.
  Title corruption is confirmed on the integrated build `8b18ac0` (2026-09-23
  user retest, title symptom only; see the
  [retest record](investigations/hardware-runs/plus-titles-8b18ac0-2026-09-23.md)).
  General PAUSE 0/1, PPR and REPEAT boundaries remain open for independent investigation.
- Left-edge sprite corruption is much improved, perhaps fixed; closure is open. Navy Seals
  / World of Sports left-edge sprite-line flicker is reported fixed on `8b18ac0`
  (2026-09-23 user retest; no assigned RTL cause, closure pending a repeatable
  capture); its black-screen report was not reproduced and has no
  assigned cause.
- CRTC3 demo: warning, audio and crash defects remain. Switchblade's original
  CPR produces an all-black capture on `8b18ac0`; its redundant-zero ASIC unlock
  repair is integrated, and the production-T80 trace now reaches the expected
  early execution point ([finding](plus/switchblade-unlock-2026-09-23.md)). Eerie
  Forest's original CPR overstates its outer RIFF length; a header-only corrected
  copy boots on the old build ([finding](plus/eerie-forest-container-2026-09-23.md)),
  and the integrated parser now accepts complete-chunk EOF. Both **unchanged**
  cartridges need an exact new-build hardware retest; full title/demo progression
  remains untested. Other cartridge crashes, CPC+ SNA/reset/reload recovery and
  odd-R5 CRTC3 behaviour remain evidence-gated.
- Disk path: on `4027f5e` the System Cartridge boots 6128+ (AMSDOS) and 464+ (tape), and
  6128+ reads a disk directory and loads Space Gun to its title
  ([device record](investigations/hardware-runs/device-acceptance-cdcb3c3-2026-09-22.md));
  GX4000 shows `Ready / 14592` (explained: see [gx4000-system-cartridge-2026-09-22.md](plus/gx4000-system-cartridge-2026-09-22.md)). Empty-drive details are unrecorded. The
  System CPR disk read is held at `XFAIL fdc-payload-poll`; see "Open Plus evidence
  boundaries" in the roadmap.
- D3/D4: post-BSR control readback and title/flicker hardware acceptance.
- B8-2, B8-3, B8-5 and B8-6 are integrated without hardware acceptance. B8-5 snapshot apply
  approximates first-frame pixels where the SNA omits address/phase history.
- Status-2 16-frame timer: the source conflict and hardware confirmation remain open.
- **Sonic reference preparation:** the new ASIC/interrupt scrapes are compared with current
  RTL in [the source findings](plus/references/scrapes-interrupt-findings-2026-09-22.md).
  PRI no-wrap remains the accepted Copter-backed policy; stale B19 and SPLT assumptions
  are corrected. Empty-vector and live-prescaler differences are investigation leads,
  not confirmed Sonic causes. See [the corpus inventory](reference-ingestion/scrapes-2026-09-22.md)
  for reviewed and deferred sources, including the synchronization/filter comparison.
- **B16 implemented:** valid CPR loads select 6128+ when Plus is Off; SNA v3
  model headers 4/5/6 select the matching Plus model before restore. Existing Plus
  selection is preserved for CPR. Independent review and selected simulation pass;
  Main/OSD echo, boot and restore still need device acceptance. See
  [the load contract](plus/b16-load-model-2026-09-22.md). **Device run on `cdcb3c3`:
  the CPR path is unreachable** because the OSD disabled `Load Plus cartridge` with Plus
  Off and Main drops an MGL at that item. Branch `general/device-acceptance-cdcb3c3`
  ungates the entry; its RBF `4027f5e` boots Sonic from Plus Off on device. The SNA path works: a header-4
  snapshot with Plus Off matches the explicit-6128+ result (types 5/6 and OSD echo unobserved). See the
  [device record](investigations/hardware-runs/device-acceptance-cdcb3c3-2026-09-22.md).
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
  model/CRTC combinations and host publication races (acceptance 2 and 3). 464 and 664 saves round-trip on
  device (`cdcb3c3`, 2026-09-22; see the
  [device record](investigations/hardware-runs/device-acceptance-cdcb3c3-2026-09-22.md)). Open:
  SD transport and the existing slice 3-4c review debt. See [b18-sna-save.md](b18-sna-save.md).
- **TV80 bench CPU** now takes the Z80's automatic I/O wait, so its I/O windows match the
  production T80pa. The reported no_wait Gate Array write drop was this fixture artifact, not an
  RTL defect ([record](investigations/no-wait-ga-write-latch-2026-09-22.md)). Consequence: the
  slow `b6-dynamic` bench fails for the type-1 machine until
  [B22](backlog.md#b22-b6-dynamic-type-1-short-hsync-stage-loses-its-shifted-fetches-after-the-tv80-io-fix)
  is resolved. [B23](backlog.md#b23-tv80-bench-cpu-bus-timing-parity-with-production-t80pa)
  proposes a TV80-vs-T80pa parity bench.
- **Peripherals:** FDC full-sector result/ST1, classic AMSDOS and hardware acceptance are open.
  B8-7 HPS cadence is open; real-CDT playback is device-accepted on 464 and 464+.
  **CDT on a CPC 464 overwriting the 464 OS ROM is fixed and device-accepted on
  `8b18ac0`** (ROM then CDT loads AmstradDiag fully; inherited from upstream: tape SDRAM
  bank 2 was also the 464 model bank; see the
  [device record](investigations/hardware-runs/device-acceptance-cdcb3c3-2026-09-22.md)).
  The fix (`859fd24`, integrated 2026-09-22) moves the tape image to bank 3 at `0x100000`
  upward — the region no model ROM/RAM, Dandanator or Plus cartridge uses — with a 7 MB
  maximum tape length, proven fail-first against the map and the B8-7 fixture. **464+ CDT
  playback losing block 2 is resolved on hardware** by the cartridge execution-rate fix
  `63fcf23`/`94b18f0`: on 2026-09-23 the same System Cartridge + `AmstradDiag.cdt` sequence
  loaded fully on RBFs `ef8da61` and `64702ac` and still lost block 2 on `4027f5e`
  ([investigation](investigations/b8-7-464plus-cdt-block2-2026-09-23.md)). The in-sim
  `cdt-boot` fixture reaches the PLAY prompt but its tape motor never starts (fixture
  divergence, recorded there, not committed as a bench). B8-4 video-word coherence has no
  hardware acceptance.
- **Tooling:** the task-workflow host smoke tests (B14) are open. `scripts/accc/lookup.py`
  finds candidate Compendium sections (BM25 plus a TypeSafe Jev skim and rerank; BM25 only,
  with a notice, when Jev is unreachable) as a navigation aid, not verification. On the two labelled
  query sets in `scripts/accc/eval/` an accepted section lands in the top 3 for 23/30 and
  27/29, and for 40/49 of the queries that share no word with the gold section's title. `--check-claim` (EN, FR or both)
  is experimental: it misreads same-topic "silent" sections as contradictions. B17 has a device-tested
  [timed keyboard replay slice](../scripts/hardware-loop/INPUT-REPLAY.md), including Sonic fire
  through Main keyboard joystick mode. Recording remains deferred because Main grabs evdev
  inputs; gameplay timing is not frame-deterministic. CI and synthesis routing are in
  [ci-testing-policy.md](ci-testing-policy.md); local setup is in [building.md](building.md).

## How hardware testing fits the loop

SHAKER is **not** part of the automated loop. The automated loop is the selected Verilator
benches plus GitHub Actions synthesis. SHAKER and title sessions are manual, user-run, and
happen at significant milestones against a target list recorded before the session (roadmap
section 4 lists SHAKER targets per checkpoint; [plus/hardware-test-checklist.md](plus/hardware-test-checklist.md)
is the Plus title matrix). A green simulation gate is never evidence of hardware accuracy, and
a manual session never gates a commit. Dated results go under
[investigations/hardware-runs/](investigations/hardware-runs/).
