# Current status

Fresh-session handover as of 2026-11-05. Work order and acceptance gates are in
[the roadmap](implementation-roadmap.md) and [backlog](backlog.md); review obligations
are in [review debt](review-debt.md); the maintained execution and photo tally is
in [SHAKER status](shaker-status.md). The prior handover is preserved as a
[historical snapshot](archive/current-status-history-2026-11-04.md), with older
history in the [September archive](archive/current-status-history-2026-09-21.md).
Dated reports contain the evidence and bounds behind the current claims below.

The [active unattended session](investigations/session-logs/overnight-resume-2026-11-04.md)
continues until weekly quota exhaustion, as authorized on 5 November. The user
will apply any reset manually and provide the next instructions. Finish known
SHAKER and DSC4/Amazing defects before the broad baseline; preserve these
instructions across handoffs and context compaction.

## Branch and current artifacts

- Active experimental branch: `codex/classic-ga-boundary-native-20261105`;
  accepted functional reference remains `e24cb50`. Completed DT evidence is on
  `codex/overnight-validation-20261103` (`18b58ce`).
  It is distinct from integrated `master`; no integration or release is implied.
- Source `c0939d8` corrects Type1 IVM counting when stage A coincides with
  line end. Qualified production-T80 replay and native B9 both give all three
  photographed `3F=2740` rows. The 46-bench gate, CRTC 246/0, recorded
  soak `044411d1ebf3cb6d`, independent review and full build pass. Wider exit,
  even-R9, origin and independent R5 seam bounds remain open.
- Installed experimental artifact: `Amstrad_20261105_c0939d8.rbf`, SHA256
  `cb7fe0bd06c1d282413e501fd6017a34dba11ae62f4483e388a630c7ca1d883e`
  (3,151,920 bytes; device hash matches). [Full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42099745018)
  artifact `Amstrad-build-399-1-full` passes all jobs; minimum setup/hold
  slack is 0.478/0.236 ns across seven clocks with zero TNS and warning counts
  unchanged from 208. Nine native cohorts completed 185 commands and 60 viewed
  PNGs; targeted controls are described in the [B9 acceptance report](investigations/hardware-runs/shaker-b9-current-reference-2026-11-05.md).
  Earlier 208 C5 parity and 005 acceptance are in the
  [C5 report](investigations/hardware-runs/crtc0-ivm-exit-table-correction-2026-11-05.md)
  and [terminal-fixes report](investigations/hardware-runs/shaker-terminal-fixes-2026-11-05.md).
  Prior filter/R3 controls and paired title replay remain bounded evidence in
  the [R3 investigation](investigations/hardware-runs/before-the-storm-r3-2026-11-05.md).
  Physical oscillation/settling and BTF alternating-frame acceptance remain open;
  a paired original BTF snapshot replay verifies width4 right8/width5 unchanged
  with byte-identical guest/raw-GA/acquisition inputs.
  Prior G1/MID acceptance on `5261414` retains all 12 original O rows and E/P/S;
  functional reference `e24cb50` retains C0 E6 26/28. Dated reports hold earlier
  artifacts, capture receipts and limits.
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
  Timing-preserving AmSpirit controls isolate the interaction: R9=3/7 gives
  the same result with R4=38, but four/eight additional lines with R4=39.
  All OUT phases match; the matrix sharpens the unsent question without
  selecting a physical rule. [Stimulus qualification](investigations/hardware-runs/crtc0-e6-rematch-stimulus-2026-11-05.md).
- C0 AI/`0091`: fix `338d111` retains inactive protection through same-line
  R7 toggles. Original production-T80 replay restores the photographed low
  post-toggle bits; lengths become6B/6B/38. The source-derived corrected
  rearm test fails before and passes after, with246 CRTC checks passing.
  Full build and native005 acceptance pass:6B/6B/38, with the six preceding
  captures pixel-identical to42. Type1 inactive rearming remains unverified.
  A4/`0037`’s top KO labels match hardware; separate OUTI fields remain open.
  [Current source and native evidence](investigations/hardware-runs/shaker-crtc0-0091-protection-2026-11-05.md).
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
- Before the Storm (CRTC1, RUN"-BTF): the ordinary R3=4/5 post-GA
  correction passes the twelve-caller proof, strict CI and full synthesis on
  `6cc52c3`. Native calibration verifies LEFT8 with complete paper width and
  stable return. Thirteen normal-boot landscape-scroller captures are visible;
  asynchronous stills cannot judge the reported alternating-frame wobble.
  D6 per-line settling and nonordinary/JIT width behaviour remain open.
  [R3 evidence and limits](investigations/hardware-runs/before-the-storm-r3-2026-11-05.md).
- D7 R2 alternation: experimental source `f4f2502` corrects the source-qualified
  endpoint lock and passes the public failing-first R2
  proof, all ten source-qualified guest cohorts, twelve R3 callers,18 strict
  B6 boots,nine dynamic cases and the final46-bench selection. Complete public
  RGB exports are inspected; strong review clears only a labelled trial.
  Full synthesis/timing passes and all ten native pages match the private replay.
  Static Type0/Plus controls are unchanged; named dynamic/demo checks complete.
  The measured VS reanchor stretch is inside both blank windows in its qualified
  case. Physical oscillation and settling remain unproved. A separate qualified
  R2/R3 interaction probe finds stale width4 RGB history after complete width6
  pulses. The reviewed metadata correction separates width measurement from
  acquisition phase; public17, R2, lint and46 selected benches pass. Full
  synthesis and targeted native static/D7/demo preservation pass for this
  transport. Physical/native relock closure remains bounded. [D7 proof and bounds](investigations/hardware-runs/shaker-d7-r2-phase-2026-11-05.md).
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
- C1 AI/008D remains a raw-PPI discrepancy on208: both R0=0 blocks give
  1/0/0/1 versus the photograph/AmSpirit1/1/1/1. Fresh CPCSE also fails.
  Q22's fixed-width premise is corrected to16lines (French§14.2p133);
  genuine replay qualifies premature16-tick expiry but gives1000, leaving
  native fourth-sample provenance and physical clock/retrigger rules open. [Current evidence](investigations/hardware-runs/shaker-crtc1-008d-current-2026-11-05.md).
- C1 O/02DA is corrected on `5261414`: all 12 original-O rows match on MiSTer,
  with `0060` in all four formerly `0050` R7=0 cases. The source MID-VSYNC correction
  has failing-first proof,246 CRTC/46 selected passes and strong Opus clearance.
  Current-T80 replay changes `0050`/HL5/one poll to `0060`/HL6/two polls with an identical
  recorded prefix through R7 acceptance. AmSpirit and fresh CPCSyntaxError
  agree on the expected rows. Whole-module history remains a separate gate.
  [Proof and native result](investigations/hardware-runs/crtc1-02da-current-poll-boundary-2026-11-05.md).
- Standalone module E Option2: fix `6e3f14f` retains terminal adjustment
  eligibility across held R4/R9 publication while keeping live counter values.
  Original first-edge replay changes1PASS/2FAIL to3PASS/0FAIL, removes raw-DE
  warning fetches and preserves0400. Combined selection46PASS; source review
  accepts the bounded fix and the first-C4=1 reload correction. Full build and
  native005 acceptance pass:0401/0402 warnings gone;0400 remains pixel-identical.
  C5 restart/count compatibility remains open (four observed lines for R5=3).
  [Current discrepancy](investigations/hardware-runs/shaker-module-e-vma-2026-11-05.md).
- C0 C5/`029E` now completes on42, but prints all odd against the hardware
  photo's6E/6O/OEOEOE. Fresh CPCSyntaxError0.12.0 matches those parity groups;
  the latest French exit tables expose an obsolete persistent comparator in
  F16. The private source-corrected successor passes246owners and restores the
  original even caller's unequal PPI pair. Gated/reviewed source208bacf also
  completes the original initializer's6E/6O/OEOEOE without forced parity.
  Full208 build passes. Native C5 gives6E/6O/OEOEOE with all18rows visible
  and stable through5seconds in Full and RawCRT modes. B1 retains its
  structured pattern/top band; exact analogue geometry is unjudged.
  Q21 is withdrawn, not awaiting an author reply. This is a raw PPI result, separate
  from filter acquisition, and does not add18 independent tests to the tally.
  [Parity evidence](investigations/hardware-runs/shaker-crtc0-c5-parity-2026-11-05.md).
  [Primary table correction](investigations/hardware-runs/crtc0-ivm-exit-table-correction-2026-11-05.md).
- C1 DE/03B6's old missing-row symptom no longer reproduces on e24cb50:
  direct E and D9→E both show A/A/B, with all six E PNGs pixel-identical.
  FF/5F remains a separate machine-input difference. [Current recheck](investigations/hardware-runs/shaker-03b6-current-recheck-2026-11-05.md).
- B9 Page B's three `3F=2780` rows on 005 now read photographed `2740`
  on c0939d8 in both qualified original replay and native capture; the other
  rows remain intact. MID `4E40` is unchanged and remains separate. Wider
  exit and coincidence bounds remain open.
  [Current B9 reference and acceptance](investigations/hardware-runs/shaker-b9-current-reference-2026-11-05.md). F13 and F20 still need their named hardware gates;
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
