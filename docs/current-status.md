# Current status

Fresh-session handover, 5 November 2026. The [roadmap](implementation-roadmap.md)
sets work order and acceptance gates; the [backlog](backlog.md) and
[review debt](review-debt.md) track cross-cutting work. Dated investigations hold
proof and limits. The [detailed prior handover](archive/current-status-detail-2026-11-05.md)
retains the source milestones and remaining context.

The [active unattended session](investigations/session-logs/overnight-resume-2026-11-04.md)
may continue to weekly quota exhaustion; the user applies resets manually and
provides further instructions. Finish known SHAKER and DSC4/Amazing defects
before a broad baseline. Original-CPC hardware and photographs are the oracle;
AmSpirit and CPCSyntaxError are useful peers, not substitutes for hardware.

## Current branch and artifact

- Experimental branch `codex/classic-ga-boundary-native-20261105`, source
  `c0939d8`; comparison baseline `e24cb50`. This is not integrated
  into `master` and is not a release. Completed DT evidence is on
  `codex/overnight-validation-20261103` (`18b58ce`).
- Installed `Amstrad_20261105_c0939d8.rbf`, 3,151,920 bytes; SHA256
  `cb7fe0bd06c1d282413e501fd6017a34dba11ae62f4483e388a630c7ca1d883e`,
  matching the device. The 46-bench gate, CRTC 246/0, recorded soak
  `044411d1ebf3cb6d`, independent review and
  [full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42099745018)
  `Amstrad-build-399-1-full` passed. Minimum setup/hold slack was
  0.478/0.236 ns across seven clocks, zero TNS; warning counts unchanged from
  208. [Native B9 and preservation evidence](investigations/hardware-runs/shaker-b9-current-reference-2026-11-05.md).
- `c0939d8` restores B9's three photographed 3F=2740 rows; wider Type1 exit,
  even-R9, origin and independent R5 seam bounds remain open. ASIC acceptance
  on `275af8a` is scoped to the checks below, not a fresh Plus baseline.

## SHAKER

The [page tally](shaker-status.md) and [ledger](shaker-results.csv) are maintained
per unique page; passing results include qualified historical comparisons and
are not a fresh sweep. CRTC2/4 are not selectable in this core.

| CRTC | Passing | Broken | Unknown | Stale | Total |
|---:|---:|---:|---:|---:|---:|
| 0 | 4 | 1 | 256 | 226 | 487 |
| 1 | 363 | 3 | 65 | 49 | 480 |
| 3 | 0 | 0 | 18 | 405 | 423 |
| All | 367 | 4 | 339 | 680 | 1390 |

| Live question or gate | Current evidence and next boundary |
|---|---|
| C0 E6/0409 | Cases 27/28 remain wrong; 26/28 pass. Resolve R4 rematch against physical behaviour; AmSpirit passes 28/28. [Forced-entry evidence](investigations/hardware-runs/crtc0-forced-zero-adjustment-2026-11-04.md), [stimulus](investigations/hardware-runs/crtc0-e6-rematch-stimulus-2026-11-05.md). |
| C1 A/0037 and AI/008D | OUTI bit 0 is native/AmSpirit/CPCSE 0 against photo 1; exact program/history binding is a question. 008D direct 208 gives 1001 and canonical c093 gives 1000 versus photo 1111. Do not infer a repair from peer agreement. [A recheck](investigations/hardware-runs/shaker-crtc1-a-current-recheck-2026-11-05.md), [008D](investigations/hardware-runs/shaker-crtc1-008d-current-2026-11-05.md). |
| C1 AI/008F and 0090 | Fresh captures remain unknown: 008F physical V2 glyph is unresolved; 0090 needs complete cell adjudication. 008E, 0091 and direct/full-history 009A pass the bounded comparison. [A recheck](investigations/hardware-runs/shaker-crtc1-a-current-recheck-2026-11-05.md). |
| C1 B6/019D and DI/02E3 | B6 fifth/fourth deadlines are 1F/1F versus photo 1E/1F; DI is 58 versus 59 but the original test labels CRTC1 unreliable. Physical aperture and coherent R7.JIT remain open. [B6 evidence](investigations/hardware-runs/crtc1-b6-cpcse-ordering-2026-11-05.md). |
| C1 S/02D3 and B9 MID | Current direct/history S is 4E40 versus photo 4E30; program revision binding remains open. B9's repaired 3F rows pass, but MID glyph/reference binding still needs adjudication. [S](investigations/hardware-runs/shaker-s-history-2026-11-04.md), [B9](investigations/hardware-runs/shaker-b9-current-reference-2026-11-05.md). |
| C5 and RFD lifetimes | C0 C5 parity is restored to photographed 6E/6O/OEOEOE; C5 restart/count compatibility and RFD retire-on-save versus latch-at-arm still need physical discrimination. [C5](investigations/hardware-runs/crtc0-ivm-exit-table-correction-2026-11-05.md), [RFD](investigations/hardware-runs/rfd-frame-lifetime-2026-11-03.md). |
| D6/D7 and D3 | Physical phase, oscillation and settling remain unproved; 44 D3 pages have invalid page-state alignment and five other D captures need acquisition/parameter recheck (49 stale total). [D7](investigations/hardware-runs/shaker-d7-r2-phase-2026-11-05.md), [D3](investigations/hardware-runs/shaker-d3-held-space-2026-11-04.md). |
| Broader classic gates | Type1 inactive rearming; F13/F20 hardware gates; IA-5 simultaneous sync/interrupt; Q17 and D1 entry; wider phase/lifetime closure remain open. [Audit](classic/audit-findings.md), [discriminators](investigations/hardware-runs/shaker-next-discriminators-2026-11-04.md). |

DT's missing band was an acquisition-epoch mismatch; full phase-matched photo
acceptance remains open. B1's structured RGB is restored but analogue geometry,
display ownership, applied-mode observability and physical HDMI/CRT response
remain open. See [DT](investigations/hardware-runs/shaker-dt-blank-boundary-2026-11-04.md)
and [display evidence](investigations/video-boundary/v1-sync-response-evidence-2026-10-29.md).

## Demos and Plus

These are sampled native results, not full-demo or physical HDMI/CRT sign-off.

| CRTC | DSC4 | Amazing | Evidence and remaining bounds |
|---:|---|---|---|
| 0 | Credits clear; sampled main effects restored | Sampled effects preserved | [Classic display](investigations/hardware-runs/classic-ga-display-edge-2026-11-05.md); wider phases remain open. |
| 1 | Credits clear; sampled main effects restored | Sampled effects preserved | DSC4 green lines and Amazing coloured bands also appear in fresh AmSpirit; [comparison](investigations/hardware-runs/dsc4-c1-main-lines-reference-2026-11-05.md). Bottom row is outside AmSpirit's default viewport; [crop](investigations/hardware-runs/dsc4-bottom-row-crop-2026-11-04.md). |
| 3 / ASIC | Credits clear; sampled main effects restored | Logo and lower effects restored | [ASIC boundary](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md); full demo remains untested. |

Before the Storm: CRTC1 has native before/after acceptance; CRTC0 now has
63 eligible pairs without reversal. CRTC3 still reverses in 176 eligible
pairs; its failing-first RGB candidate passes focused tests and the 46-bench
gate. DeepSeek review permits an experimental build; synthesis/native acceptance
are pending. Opus review, live-owner and enabled-sprite coverage remain open.
Candidate source `fdf348c`; [full build](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42131293902) is running.
[ASIC candidate](investigations/hardware-runs/asic-r3-rendered-position-2026-11-05.md). Physical HDMI/CRT acceptance
remains open. [BTF evidence](investigations/hardware-runs/before-the-storm-r3-2026-11-05.md).
The GA/R3 compensation is shared by classic CRTC0/1 and has deterministic
coverage for both; ASIC display timing uses a separately tested correction.

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
