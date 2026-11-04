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
- The latest accepted device artifact is `Amstrad_20261104_7ac11a2.rbf`, from
  source `7ac11a2`, SHA-256
  `4dc1e7628248a30092719ff026c63997bae6d95810c49bc7ad981c10b7a501ec`.
  The MiSTer copy matches. Its [full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41775641712)
  passes simulation, lint, production T80 and the required selection gate;
  Quartus setup/hold slack is 0.279/0.204 ns with zero TNS. The R0 change was
  independently reviewed; seven production-CPU cases, default B18, all 75
  ASIC leaf tests and 44 selected benches pass. [Evidence and reviews](investigations/hardware-runs/asic-r0-terminal-latch-2026-11-04.md).
- Bounded device acceptance covers A4's 3C/3D and OUTI deadlines, eight BI
  and seven AE/AR images unchanged from the accepted ASIC-mode artifact, and
  restored sampled DSC4 and Amazing main effects on CRTC3. It does **not**
  establish full SHAKER, demo or ASIC compliance. Target known SHAKER and demo
  residuals first; a broad fresh baseline follows the fixes. Capture completion
  is execution coverage, not a pass rate.
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

- C1 E/P is readable and P is 26; E remains `0060` versus reference `0080`.
  A positive-R5 candidate reaches `0080` but does not identify the mechanism;
  production C5 remains unchanged. On `db4be80` a full C1 walk gives S `0640`,
  while direct S entry gives `4E40` on both old and new artifacts. Preserve the
  history distinction and obtain a discriminator before changing C5.
  [E investigation](investigations/hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md),
  [S history](investigations/hardware-runs/shaker-s-history-2026-11-04.md).
- DSC4 credit colons are visible on CRTC0/1 after `db4be80`; the unchanged
  bottom row matches all 768 pixels of the uncropped AmSpirit reference, outside
  its default viewport. Monitor bounds remain B1 work. Type 0's old-mode partial
  pixel at a mixed-mode boundary remains a failure; no publication-delay fix
  has been selected. [Crop comparison](investigations/hardware-runs/dsc4-bottom-row-crop-2026-11-04.md),
  [mode boundary](investigations/hardware-runs/classic-type0-mode-boundary-2026-11-04.md).
- `019D` has a failing production-T80 regression; physical write visibility and
  the adjacent B8 expectation need hardware evidence. The updated-package full D1 walk passed the
  prior input stops, and `03AE`/`03B4` matched after that history.
  [Classic follow-up](investigations/hardware-runs/shaker-followup-recheck-2026-10-27.md),
  [OUTI replay](investigations/hardware-runs/crtc1-outi-019d-2026-10-27.md).
- B9 page B's collision ordering and MID FRAME glyph remain unresolved; original
  machine evidence is needed. F13 and F20 still need their named hardware gates;
  IA-2/3/6 are source-model evidence, IA-5 needs simultaneous sync/interrupt
  capture, and Q17 needs hardware discrimination. The D1 adjustment-entry
  pointer is not source-adjudicated. [B9 discriminator](investigations/hardware-runs/shaker-b9-page-b-discriminator-2026-09-22.md),
  [audit](classic/audit-findings.md).
- CRTC1 RFD parity has bounded device acceptance; retire-on-save versus
  latch-at-arm remains an inference. [RFD lifetime](investigations/hardware-runs/rfd-frame-lifetime-2026-11-03.md).

### Plus / GX4000 ASIC

- DSC4 credit clipping is caused by HSYNC suppression of intact pixels. A private
  delay candidate recovers 132 missing samples but adds 16 colon-tail pixels
  and has startup implications; it is not accepted. The separate 16-dot
  mode/blank alignment residual also remains open.
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
