# SHAKER status

Current maintained tally, 2026-11-05. Prioritize known SHAKER and DSC4/Amazing
residuals and their targeted fixes; run a broad fresh baseline after those fixes.
A completed script establishes execution, not a photograph match. Captures are
pages, not individual subtests; the estimates below describe the cited older
builds and are not an overall compliance score.

## Execution and approximate comparison

| CRTC | Execution coverage and source | Photo comparison / current boundary |
|---|---|---|
| 0 | On `cec641c`, 2.7 A completed 122 captures; B and D stopped part-way, C stalled at `029E` with no captures. 2.6 B and D fixed-wait walks yielded 2 and 110 captures respectively. The later B1 repair reaches `00FC`, without a full B0 rerun. On `7ac11a2`, updated-package E0 completed34 commands/11 captures; direct E6 on `e24cb50` completed4 captures and passes26/28 printed checks; only27/28 remain wrong (not a photo failure rate). No E photos are cached. | No aggregate original-photo failure denominator established. Current005 direct A4/AI completes28 commands/eight images:0091 is repaired6B/6B/38 with corrected VS bits; six preceding images are RGB-identical to42. 0037 top KO labels match hardware, separate OUTI fields remain open. Current005 direct C5 reaches `029E` (16 commands/three captures), still allO versus physical6E/6O/OEOEOE. Source208bacf corrects obsolete F16 table assumptions and reproduces all18 original probe relations in production-T80; full208 build passes and native parity groups match with all18rows visible/stable in Full and RawCRT. B1's pattern family/top band survives. [September A](investigations/shaker27-full-suite-2026-09-29.md), [October B–E](investigations/hardware-runs/shaker27-modules-b-e-2026-10-15.md), [B1 recheck](investigations/hardware-runs/shaker-followup-recheck-2026-10-27.md). |
| 1 | On `5fba168`, 2.7 A/B/C completed 127/139/80 captures with French B key and C marker workarounds; updated-package D/E completed 127/7. On `db4be80`, C completed 80. On `7ac11a2`, E1 completed25 commands/7 captures/zero failures/16SSM records. On `076eab1`, targeted canonical history through S completes64 captures/zero failures/121SSM; E0080/P26 are unchanged, S now4E40 on both entry paths. Diagnostic `c16e7ec` confirms the same S result and normalized entry state. Current DT completes14 captures without failures; diagnostic`c90ec8a` also qualifies both early active-black bands and preserves all14 production RGB images. The late one-band difference is an epoch mismatch, not a remaining DT RTL failure. On `5261414`, original O/E/P/direct-S complete 44 commands and 10 captures with zero failures; all 12 DA rows match, E/P/S retain pixel-identical accepted controls. | Older November 3 two-batch comparison: 350 pairs, 300 match, 30 phase and 20 different. Six of 350 (~1.7%) were content/timing flags; four (~1.1%) were acquisition questions. The raw20/350 (~5.7%) includes eight PPI input differences and two documented variants. The new targeted checks do not refresh this photo denominator. [Sweep](investigations/hardware-runs/shaker-crtc1-sweep-2026-11-03.md), [S controls](investigations/hardware-runs/shaker-s-history-2026-11-04.md), [passive observer](investigations/hardware-runs/shaker-s-passive-observer-2026-11-04.md). |
| 3 | On `cec641c`, 2.7 A/B completed 123/121 captures; C/D/E 2.7 stopped part-way, while 2.6 C/D/E produced 23/150/4 captures. On275af8a, A4/BI/AE-AR complete1/8/7 captures with zero failures; E3 remains unlaunched. | No aggregate original-photo failure denominator established. A4 top deadlines match, BI pulse widths and one-microsecond mode, AE marker and AR stripes have bounded acceptance; exact-photo geometry and other observables remain outside that acceptance. [Old execution](investigations/hardware-runs/shaker27-modules-b-e-2026-10-15.md), [current milestone](investigations/hardware-runs/asic-r0-terminal-latch-2026-11-04.md). |

Latest targeted CRTC1 validation closes O/`02DA` and preserves E/P/S.
Source `c0939d8` corrects B9's three `3F` rows to `2740` in qualified
original replay and native capture; its 46-bench gate, review, full build and
bounded native controls pass. Wider exit and coincidence bounds remain open.
[B9 acceptance](investigations/hardware-runs/shaker-b9-current-reference-2026-11-05.md). A fresh208 AI/008D check still fails bothR0=0 groups: CPCSE also
fails, while AmSpirit matches the photo. These raw-PPI discrepancies are
separate from filter response. Targeted runs do not reclassify the historical
350photo pairs. B6/`019D` and DI/`02E3` remain confirmed scalar discrepancies
on the accepted reference.
The ordinary
R3=4/5 Full-filter collapse has a deterministic rendered-border counterexample;
its reviewed correction passes the retained 12-caller proof and 46 selected
benches. Full experimental build `6cc52c3` passes and native static calibration
verifies ordinary LEFT8 without paper-width loss or return drift. D6 phase
acceptance remains open. See the
[G1/MID native controls](investigations/hardware-runs/classic-ga-display-edge-2026-11-05.md)
and [R3 investigation](investigations/hardware-runs/before-the-storm-r3-2026-11-05.md).
D7/R2 alternation is now isolated as a Full-filter phase defect with both
controlled alternating cases failing their source-derived interior bound;
the private prototype passes mean/coherent-acquisition controls and now admits
all ten source-qualified guest cohorts after correcting its cadence policy.
Physical oscillation/settling remain open. Strong review clears a labelled trial;
experimental `f4f2502` passes the failing-first public proof, all named simulation
gates and full synthesis/timing. All ten native D7 pages match the qualified
private replay at fixed pixel coordinates; static Type0/Plus controls remain
RGB-identical to `6cc52c3`. Named step/wave inputs execute, while D6 dynamic
phase remains unqualified. This does not clear physical oscillation/settling.
A new R2/R3 interaction probe exposes stale width4 RGB history after complete
width6 pulses. The reviewed independent-width metadata correction now passes
public17/R2/lint/46 selected benches. Exact-source `42befa3` full synthesis
and timing pass; native static C0/C1/Plus and all ten D7 pages preserve f4
output. All six targeted DSC4/Amazing groups retain the sampled scenes;
broader acceptance remains bounded. The historical photo denominator is unchanged.
These results are separate from page/photo compliance counts ([D7 proof](investigations/hardware-runs/shaker-d7-r2-phase-2026-11-05.md)).

The updated-package CRTC 1 D walk on `5fba168` separately classified 127
pairs: 71 match, 52 phase/acquisition and four content differences. Of the
52, all 44 D3 captures lack valid page-state alignment and are neither passes
nor failures. Do not combine these classifications with the earlier fraction into
a global rate. [D walk and page-state limits](investigations/hardware-runs/shaker-package-device-2026-11-03.md),
[D3 caller](investigations/hardware-runs/shaker-d3-held-space-2026-11-04.md).

## Known pages and next discriminators

- **CRTC 0 E6:** `e24cb50` passes26/28 unique printed checks, up from17/28
  on9a. Nine more checks match:2/4/6/8/9/10/16/17/19; all former passes remain.
  Failures are27=0055 versus0065 and28=02D6 versus000E.
  This is a program self-report, not a photo comparison. First-line R9
  restoration now fixes case4; later post-entry R4 rematch remains open.
  [Forced-adjustment fix and native tally](investigations/hardware-runs/crtc0-forced-zero-adjustment-2026-11-04.md),
  [earlier canonical-row proof](investigations/hardware-runs/crtc0-r4-switch-vsync-2026-11-04.md).

  Matched short original-program E6 reference: AmSpirit1.17.1 passes28/28
  printed checks; the preserved CPCSyntaxError0.12 batch passes9/28. These
  figures use exact glyph-decoded program results, not physical-photo scores.
  CPCSyntaxError passes case16 but fails17/19/20 and28, so use its source
  selectively rather than substituting it for AmSpirit on this module.

- **CRTC 1 A–C:** On `076eab1` and diagnostic `c16e7ec`, E `02C9`/`02CA`
  and P `02CB` retain E0080/P26. S `02D3` now gives the same 4E40 image directly
  and after canonical history, following the IVM adjustment-counting fix.
  The passive trace confirms C5=0/adjustment off at entry and matching R5/R7/exit
  states; all changed raster contents match previously inspected views.
  This is bounded state/result acceptance; physical photo 4E30 versus program
  4E40 still needs revision binding ([passive evidence](investigations/hardware-runs/shaker-s-passive-observer-2026-11-04.md)). `02DA` now matches all 12 rows on `5261414` through original direct O, including
  `0060` in all four formerly `0050` rows. Current-T80 replay and the MID correction
  explain the change with an identical recorded prefix; whole-module history
  remains unqualified ([proof/native result](investigations/hardware-runs/crtc1-02da-current-poll-boundary-2026-11-05.md)); `009A` differs
  from the RAM-snow photograph; `019D` still shows fifth/fourth `1F/1F` on current production `e24cb50` versus fresh original-caller
  AmSpirit, CPCSyntaxError0.12 and photo `1E/1F`; it needs the physical comparator-visibility contract
  ([fresh caller evidence](investigations/hardware-runs/crtc1-outi-019d-2026-11-04.md)). The four formerly yellow pages
  `00D5/00D6/00DD/00DE` now have structured, identical RGB on production e24
  and diagnostic c90. D5/DD were superseded before capture; D6/DE corroborate
  physical pattern families but geometry remains unjudged. This does not
  change the historical denominator ([bounded recheck](investigations/hardware-runs/shaker-b1-current-recheck-2026-11-05.md)). The older
  `01E7` and `0216` differences are documented variants; eight PPI pages
  reflect machine input differences. [Sweep and page details](investigations/hardware-runs/shaker-crtc1-sweep-2026-11-03.md),
  [E follow-up](investigations/hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md).
- **CRTC 1 D/E:** `02E3` still reads 58 on current production e24cb50 versus original-caller
  AmSpirit/photo 59 ([first-IRQ evidence](investigations/hardware-runs/shaker-di-first-irq-2026-11-04.md)); `03B6` now shows the expected A/A/B rows on e24cb50 both directly and
  after D9, with all six E images pixel-identical between those entry paths.
  Full earlier D-history equivalence is unproved; FF/5F remains a PPI input
  difference ([bounded recheck](investigations/hardware-runs/shaker-03b6-current-recheck-2026-11-05.md)).
  DT `02F8/02FA` is not currently a confirmed core/filter failure: the guest
  intentionally removes the second band about339ms after F8, and continuing
  the same AmSpirit run past F7 also gives one band. The native F8-labelled
  PNG was captured after F7 had already been published. A passive first-window
  native trace on`c90ec8a` now matches the authentic replay exactly in relative
  timestamps/summary: three rawVS/GA-black starts, active-black79,664/79,664/0.
  Both early bands reach acquisition; all14 late images retain production RGB.
  The reported DT discrepancy is removed from the timing/filter fix queue.
  Standalone moduleE `0401/0402` show both warning and success in a current42
  targeted Option2 run (23commands/six images), versus only success in matched
  AmSpirit1.17.1. Fix6e3f14f preserves terminal adjustment eligibility across
  held R4/R9 writes: original first-edge1/2RED→3/0GREEN, warnings removed
  before the filter, combined46benchesPASS. Native005 Option2 completes
  23commands/six inspected images:0401/0402 warningsgone and0400 RGB-exact.
  C5 restart/count compatibility remains open; no new photo verdict ([current evidence](investigations/hardware-runs/shaker-module-e-vma-2026-11-05.md)). [DT evidence](investigations/hardware-runs/shaker-dt-blank-boundary-2026-11-04.md),
  [package run](investigations/hardware-runs/shaker-package-device-2026-11-03.md).
- **CRTC 3 A4:** top 3C/3D and OUTI deadlines are accepted. Native result bytes
  confirm the last four VSYNC-bit pairs are 0/0; the previous 1/1 reading was
  an E/F glyph error. First/third 0/1 versus the older photo's 1/1 remain open,
  subject to program/history binding. The observer preserves the printed page;
  the capture-processing deadline bug is fixed separately in `ccd49d4`.
  [Native result evidence](investigations/hardware-runs/asic-a4-native-results-2026-11-04.md).
- **Demo targets:** classic DSC4 credits are repaired, but Type 0 mixed-mode
  partial pixel has an experimental retained-test correction,
  with clean full synthesis and bounded C0/C1 DSC4/Amazing views. Stronger
  source review, full synthesis and bounded native checks now pass on5261414;
  all-phase acceptance is open
  ([corrected native bounds](investigations/hardware-runs/classic-ga-display-edge-2026-11-05.md)). ASIC DSC4 credit clipping is traced to HSYNC
  suppression of intact pixels; the 15-dot terminal-mask fix 275af8a has full synthesis and targeted device
  acceptance: credit colons/names clear, BI photo clipping corroborated,
  AE/AR controls preserved. Its private replay recovered 132 samples with zero
  new title pixels. CRTC 3 DSC4/Amazing sampled main effects
  are restored, not fully accepted. [Classic boundary](investigations/hardware-runs/classic-type0-mode-boundary-2026-11-04.md),
  [ASIC credits](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md).

The [next-discriminator matrix](investigations/hardware-runs/shaker-next-discriminators-2026-11-04.md)
contains the evidence-led order. Original-CPC photographs are the oracle; compare
matched state and viewport before attributing a visual discrepancy to RTL.
The [short E0/E1 checks](investigations/hardware-runs/shaker-short-e-checks-2026-11-04.md)
record their input identities, observed failures and absence of cached E photographs.
