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

## Branch and accepted artifact

- The active `codex/overnight-validation-20261103` task branch contains the
  recent fixes and evidence. It is distinct from integrated `master` and no
  integration or release is implied. Published-artifact history is archived.
- Latest accepted device artifact: `Amstrad_20261104_ee9c131.rbf`, source
  `ee9c131`, SHA-256
  `b6bb116d91c34450128738fd4d3d118ca4fb27426f15af29dd8ba8f8e8a462fe`.
  MiSTer copy matches. [Full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41849887000)
  passes all required jobs; Quartus setup/hold 0.504/0.245 ns, seven clocks,
  zero TNS. CRTC239/focused30/final44 benches pass; fresh Opus accepts.
- Targeted Type0 acceptance: E6 cases21/24 now print0066/000E, matching their
  source-backed recipes; DSC4 credits and sampled effects remain clear/visible.
  [Comparator/build/device evidence](investigations/hardware-runs/crtc0-r9-r5-comparator-2026-11-04.md).
- Prior ASIC acceptance on `275af8a`: DSC4 colons/names and sampled DSC4/Amazing
  effects restored; BI widths/clipping, AE09/AR stripe families and A4 retained.
  Five Plus runs completed21captures. The later change is Type0-only; these
  checks were not repeated on its descendant. [ASIC evidence](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md).
- Current code tip is `e574a46`: Type0 Last Line/C0=1/IVM repair `c3e896d`,
  historical fixture adapter `28ba886`, Type1 positive-R5 C5 restart `599ca80`,
  and ordinary Type1 VSYNC publication `e574a46`. Each behavior change has a
  failing-first proof, passing selected gate and fresh independent review.
- Exact `599ca80` [push CI](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41874429612)
  passes all required jobs. Its [full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41878239893)
  is pending; let it finish before another synthesis. The prior `c3e896d` build
  met timing but failed its historical fixture, now repaired. It was preserved
  without device acceptance. `ee9c131` remains the accepted device artifact.
- C5's controlled E replay gives `0080`, seven other metrics and 515 zero-route /
  89 off-R4 controls. R7's replay preserves them while shifting all 1262 published
  VSYNC edges by four clocks. These use current CRTC/engines/GA with an inherited
  pre-`9de` T80 netlist, rather than a full original-disk motherboard boot.
  [E proof](investigations/hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md),
  [publication proof](investigations/hardware-runs/crtc1-r7-onset-controls-2026-11-04.md).
- The branch includes the ASIC mode repair `3113323`, T80 startup repair
  `9de75d3`, classic credit repair `db4be80`, and ASIC R0 repair `7ac11a2`.
  The rejected broader bus qualifier must not be treated as a verified fix.
  [Mode evidence](investigations/hardware-runs/asic-mode-latch-2026-11-04.md),
  [CPU evidence](investigations/hardware-runs/t80-first-fetch-iorq-2026-11-04.md),
  [classic credits](investigations/hardware-runs/classic-blank-alignment-2026-11-04.md).

## Active hardware questions

The [next-discriminator matrix](investigations/hardware-runs/shaker-next-discriminators-2026-11-04.md)
separates observed faults from acquisition and state-observation questions.
Compare each relevant observable against original-CPC photographs; AmSpirit is
our closest software reference, not the hardware oracle.

### Classic CRTC 0/1

- Type0 E6's R9→R5 handoff is fixed in `ee9c131`; cases21/24 have targeted
  device acceptance. Case5 cancellation, C0=1 establishment and current-line IVM
  rechecking are corrected in `c3e896d` (included in `599ca80`, build
  `41878239893` running), awaiting exact-source RBF and direct E6 check.
  Case16's zero-R5 early-entry lifetime remains a source question;
  [French draft](classic/accc-author-questions-crtc0-c0-1-2026-11-04-fr.md) is unsent.
  [Comparator](investigations/hardware-runs/crtc0-r9-r5-comparator-2026-11-04.md),
  [Last Line](investigations/hardware-runs/crtc0-last-line-cancellation-2026-11-04.md).
- C1 E/P is readable and P is 26; E prints `0080` in the controlled production-CRTC
  replay under `599ca80` (positive-R5 C5 restart, Gemini 4 Argon high reviewed),
  matching the original photograph. Hardware acceptance pending on build
  `41878239893`. On `db4be80` a full C1 walk gives S `0640`, while direct S
  entry gives `4E40` on both old and new artifacts. Preserve the history
  distinction before changing C5 further.
  [E investigation](investigations/hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md),
  [S history](investigations/hardware-runs/shaker-s-history-2026-11-04.md).
- DSC4 credit colons are visible on CRTC0/1 after `db4be80`; the unchanged
  bottom row matches all 768 pixels of the uncropped AmSpirit reference, outside
  its default viewport. Monitor bounds remain B1 work. Type 0's old-mode partial
  pixel at a mixed-mode boundary remains a failure. A genuine R3.JIT control
  also finds wrong first-pixel bits on 0→1 for both types; 1→2 suffixes pass,
  while blank release is early on both. No mode timing fix is selected.
  [Crop comparison](investigations/hardware-runs/dsc4-bottom-row-crop-2026-11-04.md),
  [mode boundary](investigations/hardware-runs/classic-type0-mode-boundary-2026-11-04.md).
- `019D` remains fifth/fourth `1F/1F` versus original-caller AmSpirit/photo
  `1E/1F`. Ordinary NJIT source/reference Type0=5 / Type1=6 is corrected by
  `e574a46`'s reviewed abstract pin transport; device acceptance is pending.
  R7.JIT remains 8/4 versus 9/5, and terminal-row admission is still open.
  A new reference pair returned low for both controls, so its assumed hidden
  counter phase is being qualified before any admission change. B8's synthetic
  OUTI margin is distinct from the authentic OUT(C),C caller.
  [OUTI caller](investigations/hardware-runs/crtc1-outi-019d-2026-11-04.md),
  [onset controls](investigations/hardware-runs/crtc1-r7-onset-controls-2026-11-04.md).
- B9 page B's collision ordering and MID FRAME glyph remain unresolved; original
  machine evidence is needed. F13 and F20 still need their named hardware gates;
  IA-2/3/6 are source-model evidence, IA-5 needs simultaneous sync/interrupt
  capture, and Q17 needs hardware discrimination. The D1 adjustment-entry
  pointer is not source-adjudicated. [B9 discriminator](investigations/hardware-runs/shaker-b9-page-b-discriminator-2026-09-22.md),
  [audit](classic/audit-findings.md).
- CRTC1 RFD parity has bounded device acceptance; retire-on-save versus
  latch-at-arm remains an inference. [RFD lifetime](investigations/hardware-runs/rfd-frame-lifetime-2026-11-03.md).

### Plus / GX4000 ASIC

- ASIC credit fix `275af8a` has bounded device acceptance: all colons/names
  visible, BI clipping corroborated by original Plus photos. The terminal-mask
  alignment is measured, not a literal silicon-stage claim. Wider mixed-mode
  and full-demo acceptance remain outside this result.
  [Boundary evidence](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md).
- A4 lower VSYNC-bit words differ from the original Plus; native result RAM is
  unknown. A saved-menu replay does not reproduce the fresh-device result. A
  disk-only observation probe was **rejected** before device use because its
  proposed emitter overlaps nested stack writes. Obtain safe native observation
  before inferring an admission rule. The R0 model's live-equality creation
  control and late-R0/split overlap also remain unverified.
  [A4 and R0 evidence](investigations/hardware-runs/asic-r0-terminal-latch-2026-11-04.md).
- General DMA PAUSE/PPR/REPEAT boundaries, exact interrupt phase, PA3 early-arm
  and last-adjustment controls, and named model/disk/reset acceptance remain
  open. Sonic's title and bounded gameplay are hardware accepted; general DMA
  boundary closure does not follow. [Source divergence ledger](plus/source-divergences.md),
  [Sonic acceptance](investigations/sonic/b20-7-dma-pause-acceptance-2026-09-22.md).
- Left-edge sprite flicker needs a repeatable capture for closure; full CRTC3
  demo completion, Eerie full-demo completion and Switchblade gameplay remain
  untested. Other cartridge crashes, Plus SNA/reset/reload recovery, odd-R5
  CRTC3 behaviour and low-A13 FDC aliases remain evidence-gated.
  [CRTC3 investigation](investigations/hardware-runs/crtc3-demo-2026-09-25.md).
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
  AmSpirit access. [AmSpirit tooling](investigations/hardware-runs/amspirit-lite-1.17-tooling.md),
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
