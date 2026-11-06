# Current status

Fresh-session handover, 6 November 2026. The [roadmap](implementation-roadmap.md)
sets work order and acceptance gates; the [backlog](backlog.md) and
[review debt](review-debt.md) track cross-cutting work. Dated investigations hold
proof and limits. The [detailed prior handover](archive/current-status-detail-2026-11-05.md)
retains the source milestones and remaining context.

The [active unattended session](investigations/session-logs/overnight-resume-2026-11-04.md)
may continue to weekly quota exhaustion; the user applies resets manually and
provides further instructions. Prioritize justified fixes for known SHAKER and
DSC4/Amazing defects. The user authorizes V1 monitor-response design and review in parallel with a
milestone release draft and current-build SHAKER archive refresh. Original-CPC
hardware and photographs remain the oracle; AmSpirit and CPCSyntaxError are peers.

## Current branch and artifact

- Experimental branch `codex/classic-ga-boundary-native-20261105`, trial source
  `fdf348c`; prior native milestone `c0939d8`, comparison baseline `e24cb50`.
  This is not integrated into `master`. The [milestone draft](https://github.com/renaudguerin/Amstrad_MiSTer/releases/tag/untagged-c15fe46775a05244113b)
  targets exact `fdf348c` and remains unpublished; its current-build SHAKER archive
  is being refreshed. Completed DT evidence remains on
  `codex/overnight-validation-20261103` (`18b58ce`).
- Installed `Amstrad_20261106_fdf348c.rbf`, 3,138,276 bytes; SHA256
  `17466675860ee115418e712c55250a866c1b2f45bc9a796fd7f314a86d1cbf0d`,
  matching the device. Exact-SHA selected simulation/lint, production-T80
  (including the new slow ASIC fixture) and
  [full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42131293902)
  pass; `Amstrad-build-401-1-full`. Setup/hold minimum is 0.507/0.201 ns
  across seven clocks, zero TNS; warning counts match c0939d8.
  Fresh Opus 5.5 review gives GO; the qualified private mixed-colour sprite
  proof passes. Physical-display and wider-corpus limits remain explicit.
  [ASIC proof and current acceptance](investigations/hardware-runs/asic-r3-rendered-position-2026-11-05.md).
  A later test-only change requires fresh Full-return acquisition/READY
  receipts; focused/46-bench gates and Gemini review pass. FPGA RTL is unchanged.
- `c0939d8` restores B9's three photographed 3F=2740 rows; wider Type1 exit,
  even-R9, origin and independent R5 seam bounds remain open. ASIC acceptance
  on `fdf348c` preserves the scoped BI/AE/AR and demo checks below, not a fresh
  Plus baseline.

## SHAKER

The [page tally](shaker-status.md) and [ledger](shaker-results.csv) are maintained
per unique page; passing results include qualified historical comparisons and
are not a fresh sweep. CRTC2/4 are not selectable in this core.

| CRTC | Passing | Broken | Unknown | Stale | Total |
|---:|---:|---:|---:|---:|---:|
| 0 | 5 | 3 | 434 | 45 | 487 |
| 1 | 363 | 4 | 112 | 1 | 480 |
| 3 | 0 | 0 | 18 | 405 | 423 |
| All | 368 | 7 | 564 | 451 | 1390 |

| Live question or gate | Current evidence and next boundary |
|---|---|
| C0 E6/0409 | Cases 27/28 remain wrong; 26/28 pass. Resolve R4 rematch against physical behaviour; AmSpirit passes 28/28. [Forced-entry evidence](investigations/hardware-runs/crtc0-forced-zero-adjustment-2026-11-04.md), [stimulus](investigations/hardware-runs/crtc0-e6-rematch-stimulus-2026-11-05.md). |
| A4/0037, CRTC0/1 | Native first OUTI bit0=0 versus physical1 on both. Current CPCSE matches the CRTC0 photo01/00 on available26/27, but gives00/00 on CRTC1. Bind the original C0 caller's successful IN/M1 phase before selecting a rule; photo-version identity remains open. [Current C0 discriminator](investigations/hardware-runs/shaker-crtc0-a4-ai-photo-check-2026-11-06.md). |
| C1 AI/008D | Direct208 gives1001 and canonicalc093 gives1000 versus physical1111. C0 AI now passes all16 photographed VS bits under the upper-PPI allowance. [C1 residual](investigations/hardware-runs/shaker-crtc1-008d-current-2026-11-05.md). |
| C1 AI/008F and 0090 | Fresh captures remain unknown: 008F physical V2 glyph is unresolved; 0090 needs complete cell adjudication. 008E, 0091 and direct/full-history 009A pass the bounded comparison. [A recheck](investigations/hardware-runs/shaker-crtc1-a-current-recheck-2026-11-05.md). |
| B6/019D, CRTC0/1 | C0 has four differing rows; current CPCSE matches all six physical values. C1 fifth/fourth R7 deadlines remain1F/1F versus1E/1F. Continue source-qualified original caller ordering after the C0 A4 phase diagnostic; no aperture selected. [C0](investigations/hardware-runs/shaker-crtc0-b6-current-2026-11-06.md), [C1](investigations/hardware-runs/crtc1-b6-cpcse-ordering-2026-11-05.md). |
| C1 DI/02E3 | Native58 versus photo59; original program labels CRTC1 unreliable, so this remains excluded from confirmed broken counts. |
| C1 S/02D3 and B9 MID | Current full-history S is0840 WRONG versus program4E40; fresh CPCSE history gives4E40. Same-RBF direct gives4E40; qualify the canonical-prefix repeat before selecting the inherited-state cause. Older photo4E30 binding remains separate. B9's repaired 3F rows pass, but MID glyph/reference binding still needs adjudication. [S](investigations/hardware-runs/shaker-s-current-2026-11-06.md), [B9](investigations/hardware-runs/shaker-b9-current-reference-2026-11-05.md). |
| C5 and RFD lifetimes | C0 C5 parity is restored to photographed 6E/6O/OEOEOE; C5 restart/count compatibility and RFD retire-on-save versus latch-at-arm still need physical discrimination. [C5](investigations/hardware-runs/crtc0-ivm-exit-table-correction-2026-11-05.md), [RFD](investigations/hardware-runs/rfd-frame-lifetime-2026-11-03.md). |
| D6/D7 and D3 | Physical phase, oscillation and settling remain unproved; All 44 CRTC1 D3 captures now have correct parameters after a private Space-count fix; digital-content comparison remains pending; CRT deflection is separate V1 monitor-response work. DT0305 remains stale on CRTC0/1 because the exit grab contains the menu. [D7](investigations/hardware-runs/shaker-d7-r2-phase-2026-11-05.md), [D3](investigations/hardware-runs/shaker-d3-count-correction-2026-11-06.md). |
| Broader classic gates | Type1 inactive rearming; F13/F20 hardware gates; IA-5 simultaneous sync/interrupt; Q17 and D1 entry; wider phase/lifetime closure remain open. [Audit](classic/audit-findings.md), [discriminators](investigations/hardware-runs/shaker-next-discriminators-2026-11-04.md). |

DT's missing band was an acquisition-epoch mismatch; full phase-matched photo
acceptance remains open. B1's structured RGB is restored but analogue geometry,
display ownership, applied-mode observability and physical HDMI/CRT response
remain open. Full/Raw pixels do not model the photographed CRT deflection;
its absence is not a CRTC/GA failure. V1 has offline response candidates,
with no validated dynamic monitor model in production. See [DT](investigations/hardware-runs/shaker-dt-blank-boundary-2026-11-04.md)
and [display evidence](investigations/video-boundary/v1-sync-response-evidence-2026-10-29.md).

## Demos and Plus

These are sampled native results, not full-demo or physical HDMI/CRT sign-off.

| CRTC | DSC4 | Amazing | Evidence and remaining bounds |
|---:|---|---|---|
| 0 | Credits clear; sampled main effects restored | Sampled effects preserved | [Classic display](investigations/hardware-runs/classic-ga-display-edge-2026-11-05.md); wider phases remain open. |
| 1 | Credits clear; sampled main effects restored | Sampled effects preserved | DSC4 green lines and Amazing coloured bands also appear in fresh AmSpirit; [comparison](investigations/hardware-runs/dsc4-c1-main-lines-reference-2026-11-05.md). Bottom row is outside AmSpirit's default viewport; [crop](investigations/hardware-runs/dsc4-bottom-row-crop-2026-11-04.md). |
| 3 / ASIC | Credits clear; sampled main effects restored | Logo and lower effects restored | fdf348c preservation; [ASIC boundary](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md); full demo remains untested. |

Before the Storm now has bounded native before/after acceptance on **CRTC0/1/3**.
The new CRTC0 comparison has 86 eligible pairs per build: old e24 reverses by
−10/+6; fdf348c has 43 holds/43 left-two steps. CRTC1 has 176 current eligible
pairs and CRTC3 has 168, also with only holds/left-two steps. All new frames
are viewed; acquisition limits are unchanged. [BTF evidence](investigations/hardware-runs/before-the-storm-r3-2026-11-05.md).

ASIC static LEFT8, eight exact BI controls and scoped DSC4/Amazing/AE/AR
preservation pass. AR colour variants reproduce the original INK10 cycle.
A single mixed-colour sprite passes; live-owner RGB isolation is accepted by
inspection. Other sprite configurations and physical HDMI/CRT remain unproved.
The classic correction is shared by CRTC0/1; ASIC uses its separate tested
correction. [ASIC candidate](investigations/hardware-runs/asic-r3-rendered-position-2026-11-05.md).
MiSTer is running the serial current-build SHAKER capture refresh under root's sole control.
[V1 design and Opus review](investigations/video-boundary/v1-qualitative-monitor-design-2026-11-06.md)
recommend a separate fourth CRT-response profile; the offline prototype is under
validation and changes no FPGA behaviour. Current original T/Y and first D3
[source transport](investigations/video-boundary/v1-current-original-callers-2026-11-06.md)
is qualified separately from monitor acceptance.

Plus retains bounded BI/AE/AR and credit acceptance. A4 first/third rows are
0/1 versus the older physical photo's 1/1, with program/history binding open;
the last four rows read 0/0. [A4 evidence](investigations/hardware-runs/asic-a4-native-results-2026-11-04.md).
DMA PAUSE/PPR/REPEAT, interrupt phase, PA3 early-arm/last-adjustment, D3/D4
post-BSR readback, status-2 timer conflict, B8-2/3/5/6 and B16 hardware gates,
model/disk/reset acceptance, and cartridge-backed SNA remain open. Sonic title
and bounded gameplay pass without closing general DMA. Sprite flicker needs a
repeatable capture; Eerie full demo, Switchblade gameplay, other cartridge
crashes, Plus SNA/reset/reload, odd-R5 CRTC3 and low-A13 FDC aliases need
further evidence. [Plus divergences](plus/source-divergences.md),
[Sonic](investigations/sonic/b20-7-dma-pause-acceptance-2026-09-22.md),
[roadmap](implementation-roadmap.md).

## Tools and remaining platform gates

Updated SHAKER package and French B1 key override are installed; D1 page-state
alignment needs care. [Package audit](investigations/hardware-runs/shaker-package-audit-2026-11-03.md).
AmSpirit Lite 1.17.1 and CPCSyntaxError 0.12.0 are usable, as is the MiSTer
hardware loop; one operator owns shared devices. [AmSpirit](investigations/hardware-runs/amspirit-lite-1.17-tooling.md),
[CPCSyntaxError](cpcsyntaxerror-testing.md), [hardware loop](investigations/hardware-runs/mister-hardware-loop-driver.md).

FDC full-sector/ST1, classic AMSDOS and hardware acceptance; B8-7 HPS cadence,
B8-4 video-word acceptance, B25 original/MiSTer captures, B14 host smoke and
production T80 dynamic-wait equivalence remain open. B18 save needs a
Main_MiSTer SD transport change and refuses Plus saves. [B18](b18-sna-save.md),
[B25](investigations/b25-diagnostic.md), [backlog](backlog.md). Simulation and
synthesis do not establish hardware accuracy.
