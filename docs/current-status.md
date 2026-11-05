# Current status

Fresh-session handover as of 2026-11-05. Work order and acceptance gates are in
[the roadmap](implementation-roadmap.md) and [backlog](backlog.md); review obligations
are in [review debt](review-debt.md); the maintained execution and photo tally is
in [SHAKER status](shaker-status.md). The prior handover is preserved as a
[historical snapshot](archive/current-status-history-2026-11-04.md), with older
history in the [September archive](archive/current-status-history-2026-09-21.md).
Dated reports contain the evidence and bounds behind the current claims below.

The [active unattended session](investigations/session-logs/overnight-resume-2026-11-04.md)
continues toward the user's approximately 90% weekly quota limit. Finish known
SHAKER and DSC4/Amazing defects before the broad baseline; preserve these
instructions across handoffs and context compaction.

## Branch and current artifacts

- Active experimental branch: `codex/classic-ga-boundary-native-20261105`;
  accepted functional reference remains `e24cb50`. Completed DT evidence is on
  `codex/overnight-validation-20261103` (`18b58ce`).
  It is distinct from integrated `master`; no integration or release is implied.
- Current experimental full artifact: `Amstrad_20261105_6cc52c3.rbf`, SHA256
  `666930cf70ce029901dbbbacfe0fef37f834dce70e227201713674a66bf632cf`.
  [Clean full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42008725418)
  passes all jobs, setup/hold minima 0.477/0.248 ns across seven clocks and zero TNS.
  It is installed hash-identically on MiSTer. D6 completes four captures with
  dynamic phase unqualified; all four D7 controls are RGB-identical to `5261414`.
  Static calibration verifies ordinary width4/5 LEFT8 with full paper width and
  stable return. All30 targeted DSC4/Amazing/BTF images across the named
  classic/Plus controls are inspected; sampled effects remain visible. BTF
  alternating-frame wobble remains unqualified by stills. This is a
  reviewed hardware trial, not general monitor or release acceptance.
  Last bounded native G1/MID controls remain on `5261414`: original O matches
  all 12 rows, E/P/S remain pixel-identical, and sampled C0/C1 DSC4/Amazing views
  are visible. Functional reference `e24cb50` retains C0 E6 26/28. Earlier
  artifacts and the resolved initializer-lint failure are in dated reports.
- ASIC acceptance remains bounded on `275af8a`: DSC4 colons/names and sampled
  DSC4/Amazing effects, BI widths/clipping and AE/AR patterns. Later changes
  target classic CRTC state; no new full Plus baseline is implied.
  [ASIC evidence](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md).

## Active hardware questions

The [next-discriminator matrix](investigations/hardware-runs/shaker-next-discriminators-2026-11-04.md)
separates observed faults from acquisition and state-observation questions.
Compare each relevant observable against original-CPC photographs; AmSpirit is
our closest software reference, not the hardware oracle.

### Classic CRTC 0/1

- C0 E6 on `e24cb50` passes 26/28 printed checks, with nine newly matching
  cases since `9a` and no previously passing regression. Remaining cases 27/28 require
  focused later R4-rematch work. First-line R9 restoration now passes case 4
  on the device as well as its source-defined counter proof.
  The early forced-entry distinction has failing-first proofs,240 CRTC/45 selected
  passes, independent review, full synthesis and targeted native acceptance.
  Matched AmSpirit passes 28/28; CPCSyntaxError passes 9/28 and supplies no
  validated replacement for every residual.
  [Last Line proof](investigations/hardware-runs/crtc0-last-line-cancellation-2026-11-04.md),
  [VSYNC proof and native result](investigations/hardware-runs/crtc0-r4-switch-vsync-2026-11-04.md),
  [forced-entry source distinction](investigations/hardware-runs/crtc0-forced-zero-adjustment-2026-11-04.md),
  [unsent C0 question](classic/accc-author-questions-crtc0-c0-1-2026-11-04-fr.md).
  A timing-preserving AmSpirit control removes exactly four lines from case 27
  while preserving the original OUT phases; it strengthens the coupled-rematch
  question without selecting a physical rule. [Stimulus qualification](investigations/hardware-runs/crtc0-e6-rematch-stimulus-2026-11-05.md).
- C1 E=`0080`/P=`26` and the S history residual have bounded native acceptance.
  The corrected history and direct paths now have matching R5/R7/exit state;
  changed raster captures match previously inspected views. The physical S
  photograph's 4E30 versus the current program's 4E40 still needs revision
  binding. Broader phase/lifetime closure is unproved.
  [E proof](investigations/hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md),
  [S state and reference limits](investigations/hardware-runs/shaker-s-history-2026-11-04.md).
- DSC4 credit colons are visible on CRTC0/1; its bottom-row difference is
  outside AmSpirit's default viewport. The remaining Type0 mixed-mode quarter
  and R3.JIT pixel/blank defects have an experimental GA candidate with retained
  64 production-T80 pixel controls, 80 snapshot/display-edge controls and 46 selected
  benches passing. Strong Opus source review clears the corrected candidate;
  the replacement full build and bounded native checks pass. Experimental
  adoption does not establish all phases or final release acceptance.
  [G1 correction and bounds](investigations/hardware-runs/classic-ga-display-edge-2026-11-05.md).
  [Crop comparison](investigations/hardware-runs/dsc4-bottom-row-crop-2026-11-04.md),
  [mode-boundary source/proof](investigations/hardware-runs/classic-type0-mode-boundary-2026-11-04.md).
- Before the Storm (CRTC1, RUN"-BTF) has original normal-boot native and
  AmSpirit scene captures. Fresh CPCSyntaxError confirms the guest alternates
  R3=04/05. The production motherboard proves Full collapses 4/5 onto
  one byte position. The pre-GA extension is rejected for edge clipping; the
  post-GA correction now has a retained 12-caller public-output proof, 46
  selected benches passing and fresh strong transport review. Its initializer
  syntax passes strict lint and fresh equivalence review; the corrected full
  build passes, with targeted native acceptance still in progress. D6 per-line behavior and R2 relocking
  remain acceptance gates before any general monitor-model claim.
  D7 now has a qualified source discriminator: raw R2 alternation is correct,
  but Full acquisition remains endpoint-locked in both cases. A private phase
  candidate passes seven input patterns and preserves rectangular acquisition
  and vertical line identity. Strong review conditionally permits a bounded trial,
  but the source-qualified ten-page guest replay exposes cadence rejection
  on the last four pages. A private cohort-consistency correction precedes
  any tracked adoption. [D7 proof](investigations/hardware-runs/shaker-d7-r2-phase-2026-11-05.md). Current BTF stills
  cannot judge alternating-frame wobble. [Evidence and limits](investigations/hardware-runs/before-the-storm-r3-2026-11-05.md).
- B6/019D remains 1F/1F versus 1E/1F; DI remains 58 versus 59. Ordinary R7
  publication is a separate correction. Fresh CPCSyntaxError B6 matches hardware
  and supplies a carry-ordering clue, without a physical aperture. Coherent
  current-T80 R7.JIT remains 8/4 versus 9/5 on both old and new GA.
  [Peer evidence and rejected deferral](investigations/hardware-runs/crtc1-b6-cpcse-ordering-2026-11-05.md).
  Address-tagged AmSpirit pixels qualify a nonterminal raster window but its
  later PPI sample is still 0; no admission model is selected from that result.
  [Onset controls](investigations/hardware-runs/crtc1-r7-onset-controls-2026-11-04.md),
  [optical qualification](investigations/hardware-runs/crtc1-r7-optical-address-qualification-2026-11-04.md),
  [unsent questions](classic/accc-author-questions-r7-s-phase-2026-11-04-fr.md).
- C1 O/02DA is corrected on `5261414`: all 12 original-O rows match on MiSTer,
  with `0060` in all four formerly `0050` R7=0 cases. The source MID-VSYNC correction
  has failing-first proof,246 CRTC/46 selected passes and strong Opus clearance.
  Current-T80 replay changes `0050`/HL5/one poll to `0060`/HL6/two polls with an identical
  recorded prefix through R7 acceptance. AmSpirit and fresh CPCSyntaxError
  agree on the expected rows. Whole-module history remains a separate gate.
  [Proof and native result](investigations/hardware-runs/crtc1-02da-current-poll-boundary-2026-11-05.md).
- C1 DE/03B6's old missing-row symptom no longer reproduces on e24cb50:
  direct E and D9→E both show A/A/B, with all six E PNGs pixel-identical.
  FF/5F remains a separate machine-input difference. [Current recheck](investigations/hardware-runs/shaker-03b6-current-recheck-2026-11-05.md).
- B9 page B's collision ordering and MID FRAME glyph remain unresolved; original
  machine evidence is needed. F13 and F20 still need their named hardware gates;
  IA-2/3/6 are source-model evidence, IA-5 needs simultaneous sync/interrupt
  capture, and Q17 needs hardware discrimination. The D1 adjustment-entry
  pointer is not source-adjudicated. [B9 discriminator](investigations/hardware-runs/shaker-b9-page-b-discriminator-2026-09-22.md),
  [audit](classic/audit-findings.md).
- CRTC 1 RFD parity has bounded device acceptance; retire-on-save versus
  latch-at-arm remains an inference. [RFD lifetime](investigations/hardware-runs/rfd-frame-lifetime-2026-11-03.md).

### Plus / GX4000 ASIC

- ASIC credit fix `275af8a` has bounded device acceptance: all colons/names
  visible, BI clipping corroborated by original Plus photos. The terminal-mask
  alignment is measured, not a literal silicon-stage claim. Wider mixed-mode
  and full-demo acceptance remain outside this result.
  [Boundary evidence](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md).
- A4 native result observation corrects the earlier E/F glyph reading: the
  last four rows already have VSYNC bits 0/0, and the current original page is
  pixel-identical to 7ac11a2. The first/third rows remain 0/1 versus the older
  physical photo's 1/1, with exact program/history binding unproved. The safe
  observer's complete marker burst and identical page qualify the raw bits;
  its runner false-timeout is fixed separately in `ccd49d4`.
  [Native A4 evidence](investigations/hardware-runs/asic-a4-native-results-2026-11-04.md).
- General DMA PAUSE/PPR/REPEAT boundaries, exact interrupt phase, PA3 early-arm
  and last-adjustment controls, and named model/disk/reset acceptance remain
  open. Sonic's title and bounded gameplay are hardware accepted; general DMA
  boundary closure does not follow. [Source divergence ledger](plus/source-divergences.md),
  [Sonic acceptance](investigations/sonic/b20-7-dma-pause-acceptance-2026-09-22.md).
- Left-edge sprite flicker needs a repeatable capture for closure; full CRTC 3
  demo completion, Eerie full-demo completion and Switchblade gameplay remain
  untested. Other cartridge crashes, Plus SNA/reset/reload recovery, odd-R5
  CRTC 3 behaviour and low-A13 FDC aliases remain evidence-gated.
  [CRTC 3 investigation](investigations/hardware-runs/crtc3-demo-2026-09-25.md).
- D3/D4 post-BSR readback and title/flicker acceptance, the status-2 16-frame
  timer source conflict, B8-2/3/5/6 hardware acceptance and B16 cartridge-backed
  SNA restoration remain open. The system-cartridge disk poll remains XFAIL.
  [B16 contract](plus/b16-load-model-2026-09-22.md), [roadmap](implementation-roadmap.md).

## Capture, display and platform

- The updated SHAKER package is installed; the French B1 key override is retained
  separately, and old A/B/C baseline captures remain valid. D1 executes 127
  captures, but its 44 D3 pages lack accepted native page-state alignment; held
  Space can advance across handler entry. Avoid blind full-walk timing retries.
  [Package audit](investigations/hardware-runs/shaker-package-audit-2026-11-03.md),
  [D3 caller](investigations/hardware-runs/shaker-d3-held-space-2026-11-04.md).
- DT's reported band difference is resolved as an acquisition-epoch mismatch.
  Native diagnostic `c90ec8a` exactly matches the authentic replay's early
  timestamps/state and active-black counts 79,664/79,664/0. All 14 images retain
  the production RGB; the guest removes the second band about 339 ms after F8,
  before the late F8-labelled PNG, as matched late AmSpirit does. Full synthesis
  and two stable native ring reads pass. No DT timing/filter repair is selected;
  exact marker-frame HDMI capture remains separate tooling work.
  [DT evidence](investigations/hardware-runs/shaker-dt-blank-boundary-2026-11-04.md).
- B1's four formerly flat-yellow captures now show identical structured RGB
  on production e24 and diagnostic c90. D5/DD were superseded before capture;
  D6/DE corroborate photo pattern families, with geometry unjudged.
  A faster opt-in SSH retry still misses the photographed intro text; absence
  of the superseded flag does not establish alignment.
  [Bounded recheck](investigations/hardware-runs/shaker-b1-current-recheck-2026-11-05.md).
  Display ownership and applied-mode observability remain open; native
  screenshots omit the OSD. B6 still needs physical HDMI/CRT acceptance and a
  CPU-generated stuck-high raw-sync recipe. The available trace and format-2
  device observations do not establish original-monitor fidelity or a complete
  monitor response model. [B1/B22 evidence](investigations/video-boundary/b22-acquisition-trace-2026-09-29.md),
  [monitor study](investigations/video-boundary/v1-sync-response-evidence-2026-10-29.md).
- AmSpirit Lite 1.17.1 media/render/script operations work through direct HTTP;
  CPCSyntaxError v0.12.0 and matching source are instrumentable. The MiSTer
  hardware loop, CSL/SSM and native capture are working tools, subject to the
  page-state and display limits above. One operator owns shared MiSTer and
  AmSpirit access. Device use remains authorized; the approved capture execution
  path works, and no user SSH-agent action is needed. [AmSpirit tooling](investigations/hardware-runs/amspirit-lite-1.17-tooling.md),
  [CPCSyntaxError guide](cpcsyntaxerror-testing.md),
  [hardware loop](investigations/hardware-runs/mister-hardware-loop-driver.md).
- FDC full-sector result/ST1, classic AMSDOS and hardware acceptance remain open;
  B8-7 HPS cadence and B8-4 video-word hardware acceptance also remain open.
  B18 SNA save is a development aid: SD transport needs a Main_MiSTer change,
  and Plus saves are refused. [B18](b18-sna-save.md).
- B25 still needs original-CPC and MiSTer table captures; B14 host smoke tests
  remain open. Production T80 native dynamic-wait equivalence remains open.
  The Compendium lookup is navigation only.
  [B25 diagnostic](investigations/b25-diagnostic.md), [backlog](backlog.md).

Simulation selection and synthesis gates do not establish hardware accuracy;
record bounded device results in dated [hardware reports](investigations/hardware-runs/).
