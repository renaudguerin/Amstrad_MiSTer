# Current status

Fresh-session handover as of 2026-11-05. Work order and acceptance gates are in
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

- Active branch: `codex/overnight-validation-20261103`, functional code tip `e24cb50`.
  It is distinct from integrated `master`; no integration or release is implied.
- Current full artifact: `Amstrad_20261104_e24cb50.rbf`, SHA256
  `c65d7e60ac4ff95263e2bf425da248c31b6c838bfadb90f2e653df533605cc46`.
  [Full synthesis](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41929217942)
  passes all jobs, setup/hold0.560/0.182ns. Native C0 E6 passes26/28 printed
  checks: case4 now0032, all other results unchanged. Four bounded DSC4 views
  retain clear credits and visible effects. [C0 evidence](investigations/hardware-runs/crtc0-forced-zero-adjustment-2026-11-04.md).
- C1 interlace-adjustment fix `ccf2be7` has targeted native acceptance on
  production `076eab1` and diagnostic `c16e7ec`: direct and canonical-history S
  both give 4E40; E0080/P26 remain pixel-identical to their controls. The passive
  trace confirms history arrives without inherited adjustment or C5. Sampled
  DSC4/Amazing screens remain visible. [C1 evidence](investigations/hardware-runs/crtc1-interlace-adjustment-counting-2026-11-04.md).
  Earlier artifacts belong in [dated milestone reports](investigations/hardware-runs/classic-targeted-milestones-2026-11-04.md).
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

- C0 E6 on `e24cb50` passes26/28 printed checks, with nine newly matching
  cases since9a and no previously passing regression. Remaining27/28 require
  focused later R4-rematch work. First-line R9 restoration now passes case4
  on the device as well as its source-defined counter proof.
  The early forced-entry distinction has failing-first proofs,240CRTC/45selected
  passes, independent review, full synthesis and targeted native acceptance.
  Matched AmSpirit passes28/28; CPCSyntaxError passes9/28 and supplies no
  validated replacement for every residual.
  [Last Line proof](investigations/hardware-runs/crtc0-last-line-cancellation-2026-11-04.md),
  [VSYNC proof and native result](investigations/hardware-runs/crtc0-r4-switch-vsync-2026-11-04.md),
  [forced-entry source distinction](investigations/hardware-runs/crtc0-forced-zero-adjustment-2026-11-04.md),
  [unsent C0 question](classic/accc-author-questions-crtc0-c0-1-2026-11-04-fr.md).
  A timing-preserving AmSpirit control removes exactly four lines from case27
  while preserving the original OUT phases; it strengthens the coupled-rematch
  question without selecting a physical rule. [Stimulus qualification](investigations/hardware-runs/crtc0-e6-rematch-stimulus-2026-11-05.md).
- C1 E0080/P26 and the S history residual have bounded native acceptance.
  The corrected history and direct paths now have matching R5/R7/exit state;
  changed raster captures match previously inspected views. The physical S
  photograph's 4E30 versus the current program's 4E40 still needs revision
  binding. Broader phase/lifetime closure is unproved.
  [E proof](investigations/hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md),
  [S state and reference limits](investigations/hardware-runs/shaker-s-history-2026-11-04.md).
- DSC4 credit colons are visible on CRTC 0/1 after `db4be80`; the unchanged
  bottom row matches all 768 pixels of the uncropped AmSpirit reference, outside
  its default viewport. Monitor bounds remain B1 work. Type 0's old-mode partial
  pixel at a mixed-mode boundary remains a failure. A genuine R3.JIT control
  also finds wrong first-pixel bits on 0→1 for both types; 1→2 suffixes pass,
  while blank release is early on both. JIT 2→0 additionally fails the source-required
  old-byte pen on both types, while its non-aliased successor and fixed controls pass.
  An asset-free current-T80 baseline has 12 source-derived failures among 56
  controls; the frozen candidate passes all 56, including Mode3, and the existing
  fast GA controls. Removing proof-only stored metadata preserves all captured
  fields. Eight genuine same-numeric JIT controls also pass. Focused snapshot checks
  exposed mode restoration and stale prefetch defects in the private candidate;
  corrections pass focused red/green checks and all 64 pixel controls.
  The asset-free retained-test draft also passes64 public RGB/blank controls;
  strict pin qualification removes its broad missing-pin waiver. Stronger
  independent review and tracked test integration remain pending; production
  GA timing is unchanged.
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
- DT's late one-band screenshot is an acquisition-epoch question, not a
  confirmed core/filter defect. The original guest removes the second band
  about339ms after F8; the same AmSpirit run also shows one band after F7.
  Native F8/F7 were already in one polled batch before the F8-labelled PNG.
  A default-off passive trace on `c90ec8a` is in full diagnostic synthesis to qualify
  the short early window. Its selected gate passes46 benches; the host also
  flags superseded captures. No filter repair is selected.
  [DT evidence](investigations/hardware-runs/shaker-dt-blank-boundary-2026-11-04.md).
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
