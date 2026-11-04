# SHAKER status

Current maintained tally, 2026-11-04. Prioritize known SHAKER and DSC4/Amazing
residuals and their targeted fixes; run a broad fresh baseline after those fixes.
A completed script establishes execution, not a photograph match. Captures are
pages, not individual subtests; the estimates below describe the cited older
builds and are not an overall compliance score.

## Execution and approximate comparison

| CRTC | Execution coverage and source | Photo comparison / current boundary |
|---|---|---|
| 0 | On `cec641c`, 2.7 A completed 122 captures; B and D stopped part-way, C stalled at `029E` with no captures. 2.6 B and D fixed-wait walks yielded 2 and 110 captures respectively. The later B1 repair reaches `00FC`, without a full B0 rerun. On `7ac11a2`, updated-package E0 completed34 commands/11 captures; direct E6 on `9a91920` completed4 captures; cases 5/14/21/24/25 match their printed expectations. Eleven of 28 unique printed checks remain wrong (not a photo failure rate). No E photos are cached. | No aggregate original-photo failure denominator established. Historical `0037` and `0091` differences and C0 `029E` starvation need reassessment on current builds. [September A](investigations/shaker27-full-suite-2026-09-29.md), [October B–E](investigations/hardware-runs/shaker27-modules-b-e-2026-10-15.md), [B1 recheck](investigations/hardware-runs/shaker-followup-recheck-2026-10-27.md). |
| 1 | On `5fba168`, 2.7 A/B/C completed 127/139/80 captures with French B key and C marker workarounds; updated-package D/E completed 127/7. On `db4be80`, C completed 80. On `7ac11a2`, E1 completed 25 commands, 7 captures, zero capture failures and 16 SSM records; photos are not yet judged. | Older November 3 two-batch comparison: 350 pairs, 300 match, 30 phase and 20 different. Six of 350 (~1.7%) were content/timing flags; four (~1.1%) were acquisition questions. The raw 20/350 (~5.7%) includes eight PPI input differences and two documented variants. E/P display and E's 0080 scalar now have targeted native acceptance; S remains history-sensitive. [Sweep](investigations/hardware-runs/shaker-crtc1-sweep-2026-11-03.md), [S controls](investigations/hardware-runs/shaker-s-history-2026-11-04.md). |
| 3 | On `cec641c`, 2.7 A/B completed 123/121 captures; C/D/E 2.7 stopped part-way, while 2.6 C/D/E produced 23/150/4 captures. On275af8a, A4/BI/AE-AR complete1/8/7 captures with zero failures; E3 remains unlaunched. | No aggregate original-photo failure denominator established. A4 top deadlines match, BI pulse widths and one-microsecond mode, AE marker and AR stripes have bounded acceptance; exact-photo geometry and other observables remain outside that acceptance. [Old execution](investigations/hardware-runs/shaker27-modules-b-e-2026-10-15.md), [current milestone](investigations/hardware-runs/asic-r0-terminal-latch-2026-11-04.md). |

The updated-package CRTC 1 D walk on `5fba168` separately classified 127
pairs: 71 match, 52 phase/acquisition and four content differences. Of the
52, all 44 D3 captures lack valid page-state alignment and are neither passes
nor failures. Do not combine these classifications with the earlier fraction into
a global rate. [D walk and page-state limits](investigations/hardware-runs/shaker-package-device-2026-11-03.md),
[D3 caller](investigations/hardware-runs/shaker-d3-held-space-2026-11-04.md).

## Known pages and next discriminators

- **CRTC 0 E6:** `9a91920` repairs 25=051B and retains 5/14=00C0/0D97
  and 21/24=0066/000E. Eleven of 28 unique printed checks still differ, down
  from 12 on 599 and 14 on ee9. Failures: 2/4/6/8/9/10/16/17/19/27/28.
  This is a program self-report, not a photo comparison. Case 19 changes
  0003→002B versus 00A3; case 25's previous value was 0003, correcting an earlier
  parent reading of 0037. The early forced-entry distinction now has a source-derived, failing-first
  candidate under review; case28 retains its separate post-entry rematch question.
  [Exact-source proof and native tally](investigations/hardware-runs/crtc0-r4-switch-vsync-2026-11-04.md),
  [previous targeted evidence](investigations/hardware-runs/classic-targeted-milestones-2026-11-04.md).

  Matched short original-program E6 reference: AmSpirit1.17.1 passes28/28
  printed checks; the preserved CPCSyntaxError0.12 batch passes9/28. These
  figures use exact glyph-decoded program results, not physical-photo scores.
  CPCSyntaxError passes case16 but fails17/19/20 and28, so use its source
  selectively rather than substituting it for AmSpirit on this module.

- **CRTC 1 A–C:** E `02C9` now gives `0080` on 599, direct and after canonical
  history; P `02CB` remains readable with `26`. S `02D3` gives 0640 after
  history versus4E40 direct on 599; publication build e574 gives 5060 after
  history, with direct control 4E40. Exact9a and the qualified passive diagnostic
  retain history0440/direct4E40. Seven native cuts locate inherited adjustment/
  C5 and an added line before the history measurement; source analysis continues
  ([passive evidence](investigations/hardware-runs/shaker-s-passive-observer-2026-11-04.md)). `02DA` shows `0050` versus `0060`; `009A` differs
  from the RAM-snow photograph; `019D` still shows fifth/fourth `1F/1F` on e574a46 versus fresh original-caller
  AmSpirit/photo `1E/1F`; it needs the physical comparator-visibility contract
  ([fresh caller evidence](investigations/hardware-runs/crtc1-outi-019d-2026-11-04.md)). The four yellow acquisition pages
  `00D5/00D6/00DD/00DE` remain unassigned, not proven correct. The older
  `01E7` and `0216` differences are documented variants; eight PPI pages
  reflect machine input differences. [Sweep and page details](investigations/hardware-runs/shaker-crtc1-sweep-2026-11-03.md),
  [E follow-up](investigations/hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md).
- **CRTC 1 D/E:** `02E3` still reads 58 on the e574a46 targeted check versus original-caller
  AmSpirit/photo 59 ([first-IRQ evidence](investigations/hardware-runs/shaker-di-first-irq-2026-11-04.md)); `02F8/02FA` lack a photographed
  black VSYNC region; `03B6` has a text-row difference. DT's caller phase must
  be reproduced before assigning timing or acquisition. E `0401/0402` have
  crop/blank uncertainty on the older updated-package run; new E0/E1 images
  await photo judgment. [DT evidence](investigations/hardware-runs/shaker-dt-blank-boundary-2026-11-04.md),
  [package run](investigations/hardware-runs/shaker-package-device-2026-11-03.md).
- **CRTC 3 A4:** top 3C/3D and OUTI deadlines are accepted. Native result bytes
  confirm the last four VSYNC-bit pairs are 0/0; the previous 1/1 reading was
  an E/F glyph error. First/third 0/1 versus the older photo's 1/1 remain open,
  subject to program/history binding. The observer preserves the printed page;
  the capture-processing deadline bug is fixed separately in `ccd49d4`.
  [Native result evidence](investigations/hardware-runs/asic-a4-native-results-2026-11-04.md).
- **Demo targets:** classic DSC4 credits are repaired, but Type 0 mixed-mode
  partial pixel remains open. ASIC DSC4 credit clipping is traced to HSYNC
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
