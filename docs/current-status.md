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

- Active V1 integration branch `codex/v1-production-integration-20261106`
  starts at the preserved investigation branch tip `ef0c109`; its receiver
  profile is explicitly enabled for an experimental full build; native
  acceptance remains pending.
  Latest device-verified functional source is still `cb3a1a6`, not integrated
  into `master`.
  [Exact-SHA full CI](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42273216780),
  including production-T80, passes. Quartus 17.0.2 full fit closes seven
  setup/hold domains, minima 0.645/0.197 ns and zero TNS. Device-verified RBF
  `Amstrad_20261106_cb3a1a6.rbf`, 3,098,272 bytes, SHA256
  `0e27d4c89f03e1bf609a055586e69abd9a4096e4f17a7a23aa0810c3051cd56a`.
  Four targeted CRTC3 runs capture 26 PNGs: DU03C0 now passes every check; all eleven
  Module E images match the accepted build; scoped DSC4/Amazing samples are
  preserved. DU/E supply sixteen actual Full0 records. Normal WAIT and every
  CFG restore pass. Classic RTL is unchanged from the accepted 493cdea proof.
- C0's p127 row advance and shared R6 consumers pass253/253 CRTC tests,
  46 selected benches,126 p127 CPU guards,46 B6 guards and33 original A4 guards.
  Opus full/supplemental reviews permit experimental use. Native A4 now matches
  all photographed fields: earlier seven-lower-mismatch claims misreadFEFF asFFFF;
  onlyrow4 was wrong and493cdea corrects it. A4 becomes unknown because extra
  C9sweep lacks a hardware reference. R4/R7 landings, late admission and competing
  parity readings remain explicit B26 limits. Scoped CRTC0 DSC4/Amazing preservation also passes; user manual BTF acceptance
  remains carried forward.
  [Correction](investigations/hardware-runs/shaker-crtc0-a4-native-glyph-correction-2026-11-06.md),
  [candidate proof](investigations/hardware-runs/shaker-crtc0-r0-nonlast-advance-2026-11-06.md),
  [model audit](investigations/rtl-model-confidence-2026-11-06.md).
- Plus DU03C0 and E0406 now pass natively on cb3a1a6. Both status corrections
  have original CPU RED/GREEN proofs, selected gates and fresh Opus reviews.
  The register-discovery page now reports25/08/19/1E for R4–R7, matching its
  source setup; its complete photo verdict remains open. Wider R5/interlace,
  live-write and decoder-topology claims remain B26 model limits.
  [Plus proof](investigations/hardware-runs/shaker-crtc3-status-overflow-2026-11-06.md).
- The [unpublished milestone draft](https://github.com/renaudguerin/Amstrad_MiSTer/releases/tag/untagged-992c9d8a468de299083c)
  still targets `fdf348c`, with its coherent 1,384-image SHAKER archive and six
  explicit gaps. The CRTC3 DSC4/Amazing samples are also preserved on5c1ab57; wider demo/
  display acceptance below is carried from fdf348c, without a fresh corpus repeat. [Milestone archive](investigations/hardware-runs/shaker-milestone-archive-2026-11-06.md),
  [ASIC display acceptance](investigations/hardware-runs/asic-r3-rendered-position-2026-11-05.md).
  Type1 B9's repaired3Frows remain accepted; wider exit/even-R9/origin/R5
  lifetime bounds remain open. Completed DT evidence remains on
  `codex/overnight-validation-20261103` (`18b58ce`).

## SHAKER

The [page tally](shaker-status.md) and [ledger](shaker-results.csv) are maintained
per unique page; passing results include qualified historical comparisons and
are not a fresh sweep. CRTC2/4 are not selectable in this core.

| CRTC | Passing | Broken | Unknown | Stale | Total |
|---:|---:|---:|---:|---:|---:|
| 0 | 7 | 2 | 477 | 1 | 487 |
| 1 | 364 | 4 | 111 | 1 | 480 |
| 3 | 4 | 2 | 416 | 1 | 423 |
| All | 375 | 8 | 1004 | 3 | 1390 |

| Live question or gate | Current evidence and next boundary |
|---|---|
| C0 E6/0409 | Cases 27/28 remain wrong; 26/28 pass. Resolve R4 rematch against physical behaviour; AmSpirit passes 28/28. [Forced-entry evidence](investigations/hardware-runs/crtc0-forced-zero-adjustment-2026-11-04.md), [stimulus](investigations/hardware-runs/crtc0-e6-rematch-stimulus-2026-11-05.md). |
| A4/0037, CRTC0/1 | C0 upper01/00 remains correct and all8 photographed lower pairs now match on493cdea. Prior seven-lower-failure claim was an E/F transcription error: baselineFEFF×8 had only row4 wrong, nowFFFF. Page becomes unknown because extraC9sweep lacks hardware reference. C1 first pair00/00 remains broken. [Correction](investigations/hardware-runs/shaker-crtc0-a4-native-glyph-correction-2026-11-06.md). |
| C1 AI/008D | S2/S3 remain low versus physical1111 on both direct/canonical paths. Fresh normal-boot AmSpirit has raw1111 at64ED before cleanup, excluding its formatter/restoration as the cause. The width-clock rule still needs physical/author evidence. C0 retains its16 photographed VS-bit passes. [C1 residual](investigations/hardware-runs/shaker-crtc1-008d-current-2026-11-05.md). |
| C1 AI/008F and 0090 | 008F now passes30/30 numeric fields after independent vision/source adjudication, also on C0. 0090 has310/328 C0 and316/328 C1 matrix samples plus7 actual footer comparisons matched per CRTC;18/12 glyphs remain unresolved. 008E, 0091 and direct/full-history 009A retain acceptance. [A recheck](investigations/hardware-runs/shaker-crtc1-a-current-recheck-2026-11-05.md). |
| B6/019D, CRTC0/1 | C0 now matches all four R0 rows on8ba384e; both R7 rows remain1F versus1E. Current CPCSE matches all six physical values. C1 fifth/fourth R7 deadlines remain1F/1F versus1E/1F. Astra finds the lateR7 ordering underdetermined; a physical R7=32/33 paired discriminator is prepared. No aperture or RTL fix selected. [C0](investigations/hardware-runs/shaker-crtc0-b6-current-2026-11-06.md), [C1](investigations/hardware-runs/crtc1-b6-cpcse-ordering-2026-11-05.md). |
| C3 B6 and BR | B6 passes all six current checks; BR01F0/01F1 remain broken on accepted cb3. A private delayed-count candidate passes all13 original rows and transport on current composition, including CPI C4 and its predicted20-line next interrupt. All six V5 screens and68 DMA H1 records also match physical references. Earlier one-line phase shift and broader raw-phase/restore failures remain explicit; fresh semantic review and native acceptance are pending. [Candidate](investigations/hardware-runs/shaker-crtc3-br-delayed-count-2026-11-06.md). |
| C1 DI/02E3 | Native58 versus photo59; original program labels CRTC1 unreliable, so this remains excluded from confirmed broken counts. |
| C1 S/02D3 and B9 MID | Current cb3 history gives0640 versus direct/physical/AmSpirit4E40. Passive0c9 captures both passing4E40 and failing0440 histories with valid seven-cut rings: good arrives inactive, bad with C5=15/adjustmenton. Original P cleanup and a short E/P/S subset are next; no clear-C5 fix is selected. B9 repaired3Frows pass; MID binding remains open. [S](investigations/hardware-runs/shaker-s-current-2026-11-06.md), [B9](investigations/hardware-runs/shaker-b9-current-reference-2026-11-05.md). |
| C5 and RFD lifetimes | C0 C5 parity is restored to photographed 6E/6O/OEOEOE; C5 restart/count compatibility and RFD retire-on-save versus latch-at-arm still need physical discrimination. [C5](investigations/hardware-runs/crtc0-ivm-exit-table-correction-2026-11-05.md), [RFD](investigations/hardware-runs/rfd-frame-lifetime-2026-11-03.md). |
| D6/D7 and D3 | Physical phase, oscillation and settling remain unproved; All132 D3 captures across CRTC0/1/3 now have correct parameters after a private Space-count fix; digital-content comparison remains pending; CRT deflection is separate V1 monitor-response work. DT0305 remains stale on CRTC0/1/3 because the exit grab contains the menu. [D7](investigations/hardware-runs/shaker-d7-r2-phase-2026-11-05.md), [D3](investigations/hardware-runs/shaker-d3-count-correction-2026-11-06.md). |
| Broader classic gates | Type1 inactive rearming; F13/F20 hardware gates; IA-5 simultaneous sync/interrupt; Q17 and D1 entry; wider phase/lifetime closure remain open. [Audit](classic/audit-findings.md), [discriminators](investigations/hardware-runs/shaker-next-discriminators-2026-11-04.md). |

DT's missing band was an acquisition-epoch mismatch; full phase-matched photo
acceptance remains open. B1's structured RGB is restored but analogue geometry,
display ownership, applied-mode observability and physical HDMI/CRT response
remain open. Full/Raw pixels do not model the photographed CRT deflection;
its absence is not a CRTC/GA failure. V1 has a reviewed private RTL candidate;
production video behaviour is unchanged. See [DT](investigations/hardware-runs/shaker-dt-blank-boundary-2026-11-04.md)
and [display evidence](investigations/video-boundary/v1-sync-response-evidence-2026-10-29.md).

## Demos and Plus

Renaud confirms manual MiSTer acceptance of **DSC4, Amazing and Before the
Storm on CRTC0/1/3**, closing the reported demo defects. The tested filename/SHA
was not specified; exact-build sampled evidence remains separate.
[Manual acceptance](investigations/hardware-runs/demos-manual-acceptance-2026-11-06.md).

| CRTC | DSC4 | Amazing | Before the Storm |
|---:|---|---|---|
| 0 | Manually confirmed working | Manually confirmed working | Manually confirmed working |
| 1 | Manually confirmed working | Manually confirmed working | Manually confirmed working |
| 3 / ASIC | Manually confirmed working | Manually confirmed working | Manually confirmed working |

Reproducible earlier bounds remain in the [classic display](investigations/hardware-runs/classic-ga-display-edge-2026-11-05.md),
[ASIC display](investigations/hardware-runs/asic-r3-rendered-position-2026-11-05.md)
and [BTF before/after](investigations/hardware-runs/before-the-storm-r3-2026-11-05.md)
reports. Native samples do not independently validate physical HDMI/CRT response.

ASIC static LEFT8, eight exact BI controls and scoped DSC4/Amazing/AE/AR
preservation pass. AR colour variants reproduce the original INK10 cycle.
A single mixed-colour sprite passes; live-owner RGB isolation is accepted by
inspection. Other sprite configurations and physical HDMI/CRT remain unproved.
The classic correction is shared by CRTC0/1; ASIC uses its separate tested
correction. [ASIC candidate](investigations/hardware-runs/asic-r3-rendered-position-2026-11-05.md).
The fdf348c SHAKER acquisition is complete: 1,384 images and six explicit gaps. A small [upstream comparison](investigations/hardware-runs/shaker-upstream-comparison-2026-11-06.md) is also complete; the candidate is restored on MiSTer.
The [V1 design](investigations/video-boundary/v1-qualitative-monitor-design-2026-11-06.md)
now has an isolated receiver/renderer on `codex/general/v1-crt-response-20261106`
(`b291826`, READY). Eighteen frozen streams match over5,432,672 output clocks;
47 selected benches, unchanged CRTC soak, fresh Opus review and
[exact-SHA CI](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42273531215)
pass. The active integration explicitly enables it for the experimental full build. Its
production-boundary proof covers motherboard/compositor/lifecycle wiring,
but fresh Opus review found a one-dot RGB/metadata alignment defect. Its
combined consumer repair and fresh Opus supplement now pass. Owned import passes
48 selected benches and strict disabled/enabled motherboard lint. Fresh test
review is reconciled: RGB/metadata/FIELD share the output consumer, while
FRAME_FIELD is checked separately at the motherboard export. Enabled-profile
Quartus, full-top/ASCAL and native acceptance remain. [Integration state](investigations/video-boundary/v1-production-integration-2026-11-06.md).
Full and both Raw profiles retain their behaviour. Original T/Y and first D3
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

The main [model-confidence inventory](investigations/rtl-model-confidence-2026-11-06.md)
records bounded inferred policies. The separate B26 audit is READY on
`codex/accuracy/b26-terminal-sampling-20261106` (`425be96`):32 classified policies
and one C0 pending-step consolidation, with65,536 four-state equivalence cases,
253 CRTC passes, unchanged soak, Opus review and exact-SHA CI. A further focused C1 audit has two private algebraic reductions with bounded
full-wrapper preservation proof; a possible state-bit reduction still needs
the production bus contract. Both parallel
branches retain frozen base1e95fa0. Neither is integrated; reconcile the shared
inventory and preserve current Plus evidence during integration.

The [private reference-text cache](shaker-reference-text.md) has eighteen pages
with reviewed result fields and explicit unresolved cells. Exact source-font
matching now verifies native text independently, correcting A4 E/F and S result
misreadings; it does not automatically grade the core against hardware.

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
