# Current status

Fresh-session handover as of 2026-11-04. Work order and acceptance gates are in
[the roadmap](implementation-roadmap.md) and [backlog](backlog.md); review obligations
are in [review debt](review-debt.md); the maintained execution and photo tally is
in [SHAKER status](shaker-status.md). The prior handover is preserved as a
[historical snapshot](archive/current-status-history-2026-11-04.md), with older
history in the [September archive](archive/current-status-history-2026-09-21.md).
Dated reports contain the evidence and bounds behind the current claims below.

The [active unattended session](investigations/session-logs/overnight-resume-2026-11-04.md)
continues toward the user's approximately 80% weekly quota limit. Finish known
SHAKER and DSC4/Amazing defects before the broad baseline; preserve these
instructions across handoffs and context compaction.

## Branch and current artifacts

- Active branch: `codex/overnight-validation-20261103`, RTL tip `9a91920`, tooling tip `ccd49d4`, default-off observer `50b2402`.
  It is distinct from integrated `master`; no integration or release is implied.
- Latest targeted build: `Amstrad_20261104_9a91920.rbf`, SHA256
  `eadc739cb8809236df65866ad36f99b9b6ba1dd04c22e71d507318e53d0c5576`.
  [Full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41900154684)
  passes all jobs, setup/hold 0.585/0.242 ns. Native C0 E6 case 25 is now 051B;
  11/28 printed checks remain wrong. This artifact is installed on the MiSTer.
- Conservative hardware comparison: `Amstrad_20261104_599ca80.rbf`, SHA256
  `8c21cfeb24ed5cb33570e04a93add45aefb146f82e2dd169385b34059f02c87f`.
  [Full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41878239893)
  passes every required job, setup/hold 0.356/0.244 ns, seven clocks, zero TNS.
  C0 E6 cases 5/14 and 21/24 match; C1 E is 0080/P26; sampled DSC4 remains clear.
- Investigation build: `Amstrad_20261104_e574a46.rbf`, SHA256
  `f95bac8d60821cee62329d9314bd63811936a8102c892f47d484cfa74ae19df9`.
  [Full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41881983617)
  passes all jobs, setup/hold 0.575/0.243 ns. E/P is pixel-identical to 599 and DI/B6
  retain their known residuals; full-history S changes 0640→5060, still wrong.
  Direct S on this source is 4E40. Its ordinary fifth/sixth-pixel correction
  is source/reference/bench-proven, not a measured native-PNG endpoint.
  The isolated diagnostic branch `codex/s-passive-native-20261104` enables the
  reviewed observer at `ae9ccdc`; [full synthesis](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41910599920)
  is running. It has no native acceptance yet and must preserve exact-baseline
  S scalars before its state is interpreted.
  [Exact targeted results and limits](investigations/hardware-runs/classic-targeted-milestones-2026-11-04.md).
- ASIC acceptance remains bounded on `275af8a`: DSC4 colons/names and sampled
  DSC4/Amazing effects, BI widths/clipping and AE/AR patterns. Later changes
  target classic CRTC state; no new full Plus baseline is implied.
  [ASIC evidence](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md).
- Recent fixes retain failing-first tests, selected gates and fresh independent
  reviews. C5 controlled replay uses an inherited pre-9de T80 netlist; current
  CI/native checks cover the current CPU separately. Dated reports retain that
  provenance and the repaired historical-fixture failure.

## Active hardware questions

The [next-discriminator matrix](investigations/hardware-runs/shaker-next-discriminators-2026-11-04.md)
separates observed faults from acquisition and state-observation questions.
Compare each relevant observable against original-CPC photographs; AmSpirit is
our closest software reference, not the hardware oracle.

### Classic CRTC 0/1

- C0 E6 on `9a91920` fixes case 25 from 0003 to 051B, following the canonical
  row-commit VSYNC correction. Case 19 changes 0003→002B versus 00A3; 11/28
  printed checks still fail. The prior parent 0037 reading was wrong. Cases
  5/14/21/24 retain their corrected values. The zero-R5 early-write
  distinction now has a combined French-source counter proof in preparation;
  case 28's later R4 re-match remains a source question. The matched
  AmSpirit original E6 path passes all28 printed checks; CPCSyntaxError also
  fails case19, so its source supplies no validated replacement rule there. The canonical-row fix has its retained red proof, 240-case CRTC suite,
  selected gate, independent review, full synthesis and exact-source native
  acceptance; the next forced-entry correction remains in progress.
  [Last Line proof](investigations/hardware-runs/crtc0-last-line-cancellation-2026-11-04.md),
  [VSYNC proof and native result](investigations/hardware-runs/crtc0-r4-switch-vsync-2026-11-04.md),
  [forced-entry source distinction](investigations/hardware-runs/crtc0-forced-zero-adjustment-2026-11-04.md),
  [unsent C0 question](classic/accc-author-questions-crtc0-c0-1-2026-11-04-fr.md).
- C1 E is 0080 on native direct and canonical-history paths; all seven numeric
  controls and P26 remain. S remains history-sensitive: 599 full 0640/direct 4E40,
  e574 full 5060/direct 4E40; exact9a full0440/direct4E40. The e574 word
  corrects an earlier9600 transcription. The cached CS photograph appears 4E30, so exact
  photo/program revision binding also needs clarification. Preserve the current
  program's 4E40 expectation. Matched original-caller AmSpirit direct/history
  runs both give 4E40/1,188 polls despite three versus six failed acquisition
  retries; every checkpoint's CRTC register array matches. Retry count alone
  does not explain the native failure. A bounded default-off passive FPGA
  observer is reviewed/gated and undergoing isolated synthesis; no S counter
  model is selected. [E proof](investigations/hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md),
  [S evidence](investigations/hardware-runs/shaker-s-history-2026-11-04.md).
- DSC4 credit colons are visible on CRTC 0/1 after `db4be80`; the unchanged
  bottom row matches all 768 pixels of the uncropped AmSpirit reference, outside
  its default viewport. Monitor bounds remain B1 work. Type 0's old-mode partial
  pixel at a mixed-mode boundary remains a failure. A genuine R3.JIT control
  also finds wrong first-pixel bits on 0→1 for both types; 1→2 suffixes pass,
  while blank release is early on both. JIT2→0 additionally fails the source-required old-byte pen on both types,
  while its non-aliased successor and fixed controls pass. No mode timing fix
  is selected.
  [Crop comparison](investigations/hardware-runs/dsc4-bottom-row-crop-2026-11-04.md),
  [mode boundary](investigations/hardware-runs/classic-type0-mode-boundary-2026-11-04.md).
- B6/019D remains 1F/1F versus 1E/1F; DI remains 58 versus 59. Ordinary R7
  publication is a separate correction. R7.JIT remains 8/4 versus 9/5.
  Address-tagged AmSpirit pixels qualify a nonterminal raster window but its
  later PPI sample is still 0; no admission model is selected from that result.
  [Onset controls](investigations/hardware-runs/crtc1-r7-onset-controls-2026-11-04.md),
  [optical qualification](investigations/hardware-runs/crtc1-r7-optical-address-qualification-2026-11-04.md),
  [unsent questions](classic/accc-author-questions-r7-s-phase-2026-11-04-fr.md).
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
- DT's snapshot replay has a different vertical phase after bypassing its one-time
  synchronizer. Full reproduces one band; Raw CRT crops the picture without
  settling the cause. Reproduce the caller phase before assigning the difference
  to functional timing or display acquisition. [DT evidence](investigations/hardware-runs/shaker-dt-blank-boundary-2026-11-04.md).
- B1 display ownership and applied-mode observability remain open; native
  screenshots omit the OSD. B6 still needs physical HDMI/CRT acceptance and a
  CPU-generated stuck-high raw-sync recipe. The available trace and format-2
  device observations do not establish original-monitor fidelity or a complete
  monitor response model. [B1/B22 evidence](investigations/video-boundary/b22-acquisition-trace-2026-09-29.md),
  [monitor study](investigations/video-boundary/v1-sync-response-evidence-2026-10-29.md).
- AmSpirit Lite 1.17.1 media/render/script operations work through direct HTTP;
  CPCSyntaxError v0.12.0 and matching source are instrumentable. The MiSTer
  hardware loop, CSL/SSM and native capture are working tools, subject to the
  page-state and display limits above. One operator owns shared MiSTer and
  AmSpirit access. Device use remains authorized. Sandboxed SSH signing/authentication failed;
  an approved read-only check outside the sandbox succeeded. Captures now use
  that execution path; no user SSH-agent action is needed. [AmSpirit tooling](investigations/hardware-runs/amspirit-lite-1.17-tooling.md),
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
