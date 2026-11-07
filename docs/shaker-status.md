# SHAKER status

Maintained page tally, 7 November 2026. The [results ledger](shaker-results.csv)
owns the counts and records each source build. Qualified historical comparisons
carry forward unless superseded; this is an approximation of known status,
not a fresh milestone baseline. A–E acquisition on CRTC0/1/3 has1,384images
and six explicit gaps; the three invalid DT0305 exit/menu grabs remain stale.
[Archive](investigations/hardware-runs/shaker-milestone-archive-2026-11-06.md).

Targeted493cdea fixes C0 A4's sole lower-field mismatch. Earlier seven-failure
claims misreadFEFF asFFFF. All photographed fields now match, but A4 stays
unknown because extraC9sweep lacks hardware reference. CRTC0/1 AI008F numeric
comparisons pass after reviewed reference readings. CRTC3 E0406 passes on5c1ab57;
DU03BF and03C0 pass on cb3a1a6 after the status corrections. See the
[A4 correction](investigations/hardware-runs/shaker-crtc0-a4-native-glyph-correction-2026-11-06.md).
The release candidate `354388d` now matches the user's real 6128 Plus
captures for BR01F0/01F1: all48 opcode/results and all18 complete timing records.
Both MRER pages also match128/128 phases; the preceding model misses two phase
boundaries. VS31/32/33 preserves0/0/64 interrupts in all three repetitions.
These two page judgments are now passing on the final release bitstream. The reviewed IRQ test contract passes47 selected benches. Exact354 full CI,
Quartus timing and six targeted native checks now pass, including fresh BR
comparisons on the final bitstream. This is not a full baseline.
[Physical comparison](investigations/hardware-runs/real-plus-irq-comparison-2026-11-07.md).
Both CRTC3 STATUS2 corrections pass original CPU RED/GREEN, 46 selected benches,
fresh Opus review, full fit and native acceptance on cb3a1a6. DU03C0 now shows
every check OK; all eleven Module E captures match4e45417, and scoped CRTC3
DSC4/Amazing samples remain intact. This moves one page from broken to passing.

Current cb3 S canonical prefix prints0640 WRONG versus4E40, with134commands,
65captures and121actualFull0SSM records. Exact-font decoding and CFG restore
pass. On the same passive observer fit, E→S gives0640 with C5=23/adjustment
active before S, while P→S gives4E40/inactive. E/P/S gives5060; fresh AmSpirit
controls give4E40 for all three sequences. These diagnostic subsets narrow the
next investigation to E's outgoing state without proving all key phases or an
original-chip rule. The page remains broken. See the [current S evidence](investigations/hardware-runs/shaker-s-current-2026-11-06.md).

AI0090 matches310/328 C0 and316/328 C1 matrix samples plus seven actual
footer comparisons per CRTC. Eighteen/twelve photo glyphs remain unresolved,
so both whole-page statuses stay unknown. Printed photo expected65 versus
native expected66 is retained separately; measured length66 matches.

Read the [reviewed static-reference text cache](shaker-reference-text.md) before reopening covered text-only photos; unresolved fields still require the source image.

## Counts by CRTC and module

A unit is one unique SSM page, not one screenshot or individual assertion.
A page with any confirmed failing check is broken. A page with only partial
validation remains unknown, even when a specific defect on it is repaired.
The source build belongs to each ledger row: a passing page on an explicitly
experimental build does not imply that build has passed its global gates.
CRTC/GA content and monitor response have separate acceptance boundaries: Full
and Raw pixels regularize acquisition and do not promise the photographed CRT
deflection. Missing curvature alone is neither a broken page nor a reason to
withhold functional acceptance. Unvalidated source content remains unknown.

- **Passing:** accepted original-CPC comparison or validated program self-checks.
  Documented chip variants and unrelated PPI upper-input differences are allowed.
- **Broken:** a confirmed current mismatch against a usable reference or self-check.
- **Unknown:** untested, no usable reference, unresolved phase/history, or only
  part of the page has been validated.
- **Stale:** older unadjudicated/obsolete captures, invalid page-state captures,
  or an earlier judgment explicitly requiring recheck. Stale does not mean broken.
- **Total:** known page inventory:1,350 indexed real-CPC photo pages plus40
  additional current author-script pages. It is not an exhaustive firmware
  enumeration; unlisted pages can expand it. Each row sums exactly to Total.

| CRTC | Module | Passing | Broken | Unknown | Stale | Total |
|---:|:---|---:|---:|---:|---:|---:|
| 0 | A | 4 | 0 | 118 | 0 | 122 |
| 0 | B | 0 | 1 | 220 | 0 | 221 |
| 0 | C | 1 | 0 | 21 | 0 | 22 |
| 0 | D | 0 | 0 | 110 | 1 | 111 |
| 0 | E | 2 | 1 | 8 | 0 | 11 |
| **0** | **Total** | **7** | **2** | **477** | **1** | **487** |
| 1 | A | 119 | 2 | 6 | 0 | 127 |
| 1 | B | 103 | 1 | 35 | 0 | 139 |
| 1 | C | 76 | 1 | 3 | 0 | 80 |
| 1 | D | 63 | 0 | 63 | 1 | 127 |
| 1 | E | 3 | 0 | 4 | 0 | 7 |
| **1** | **Total** | **364** | **4** | **111** | **1** | **480** |
| 3 | A | 0 | 0 | 124 | 0 | 124 |
| 3 | B | 3 | 0 | 118 | 0 | 121 |
| 3 | C | 0 | 0 | 23 | 0 | 23 |
| 3 | D | 2 | 0 | 147 | 1 | 150 |
| 3 | E | 1 | 0 | 4 | 0 | 5 |
| **3** | **Total** | **6** | **0** | **416** | **1** | **423** |
| **All** | **Total** | **377** | **6** | **1004** | **3** | **1390** |

CRTC2/4 are not selectable in this core. There are no indexed module-E photos;
its passing/broken judgments use the qualified original-program checks.
CRTC3 now has B6 and DU03BF/03C0 program-self-check passes. Scoped BI/AE/AR
checks retain15 unknown whole-page judgments. All modules have current acquisition evidence; only the three invalid DT0305
menu captures remain stale.
[Current CRTC3 numeric checks](investigations/hardware-runs/shaker-crtc3-b-current-2026-11-06.md).

## What the remaining counts contain

The focused current [CRTC0 A4/AI comparison](investigations/hardware-runs/shaker-crtc0-a4-ai-photo-check-2026-11-06.md)
adds an AI passing judgment and the A4 OUTI mismatch to previously unknown
coverage; this does not establish a regression. Current CPCSE26/27 match the
physical CRTC0 A4 result, unlike its CRTC1 result. C0 B6 now has two confirmed R7 differences on8ba384e; its four R0 rows
match the photo/self-checks after the narrow repair. The fresh peer matches all six.
[C0 B6 comparison](investigations/hardware-runs/shaker-crtc0-b6-current-2026-11-06.md).
The [original C0 A4 caller](investigations/hardware-runs/shaker-crtc0-a4-qualified-phase-2026-11-06.md)
now independently qualifies a3Ccounter-continuation failure; ordinaryOUT
boundary controls now qualify the OUTI-only mismatch. The zero-total OUTI control independently fails while ordinary OUT and C1
controls pass. Reviewed source8ba384e passes the qualified original caller, full CI and
native01/00 pair.493cdea now also corrects the sole lower mismatch. The
prior seven-failure claim misread nativeFEFF asFFFF: two blind readings and
exact pixel/glyph comparison establish baselineFEFF×8 and only row4 changing
toFFFF. All eight photographed pairs now match after the documented upperA0
allowance. A4 moves to unknown because the extraC9sweep remains ungraded.
[Correction and evidence](investigations/hardware-runs/shaker-crtc0-a4-native-glyph-correction-2026-11-06.md).

- **Confirmed broken:** C0 E6/page0409, cases27/28; C1 A/0037; C0/1 B6/019D;
  C1 AI/008D; C1 S/02D3 (current history0640 versus4E40). E6's26/28 figure counts assertions across0407/0408/0409,
  separately from the page tally.
- **Repaired but not a whole-page pass:** B9's three3Fdelay rows are2740
  on MiSTer, while its MID glyph/reference binding remains open. Earlier S
  direct/history4E40 passes are superseded by the current failing history
  capture. Same-RBF direct gives4E40, but exact source-font checking corrects
  the fdf prefix-repeat reading to0440. That repeat is not a passing control.
  [Current S evidence](investigations/hardware-runs/shaker-s-current-2026-11-06.md).
- **C3 BR page matches on release candidate:** all 66 measured records match
  the real Plus captures on `354388d`; accepted `cb3a1a6` still has the
  five incorrect bytes and three thresholds. Exact354 full CI, timing and targeted native acceptance now pass; a full
  corpus refresh remains separate. The bounded normal-count/VS split and
  unresolved dynamic PRI aperture remain documented model limits.
  [Hardware comparison](investigations/hardware-runs/real-plus-irq-comparison-2026-11-07.md).
- **Closed CRTC3 status/overflow:** DU03C0 and all E0406 ordinary/ROM checks
  pass on cb3a1a6. Source-bound CPU replay and native captures support the
  sampled STATUS2 corrections despite the literal French bit4 table conflict;
  wider R5/interlace/live-write topology remains inferred.
  [Evidence](investigations/hardware-runs/shaker-crtc3-status-overflow-2026-11-06.md).
- **Unknown dynamic/display checks:** D6/D7 physical phase/settling, DI02E3
  (58/59; the program labels CRTC1 unreliable), and DT02F8/02FA's full
  phase-matched photo verdict. DT's reported missing band was an acquisition
  epoch mismatch; its early native trace contains both bands.
- **D3 acquisition repaired:** all132 current headers across CRTC0/1/3 match after correcting
  one extra Space in our private manual overlay. These pages are unknown
  pending full digital-content/blanking comparison; CRT deflection belongs to
  V1 separately. Four other formerly stale C1 D pages now become unknown;0305 remains stale
  because its current exit grab is the menu, not the reference result. [Count correction](investigations/hardware-runs/shaker-d3-count-correction-2026-11-06.md).
- **Current AI recheck:** 008E and 0091 pass the photographed VS fields;
  008F now passes30/30 numeric fields on C0/C1 after blind vision and root
  adjudication; 0090 awaits full cell adjudication. The C1 0037 OUTI bit0 mismatch remains; C0
  photographed fields are repaired on493cdea.
- **009A passes:** original direct and full A1-history native paths both show
  the dynamic full-screen RAM-noise family seen in the photograph. The old
  black defect did not reproduce; this result introduces no new RTL rule.
  See the [current A recheck](investigations/hardware-runs/shaker-crtc1-a-current-recheck-2026-11-05.md).

Recent accepted fixes include C0 0091 and C5 parity, C1 E/P, O's twelve
result rows and Module E0401/0402 warning removal. Current B9 preservation
covers O/E/P/S, Module E, all ten D7 pages and D9→E. See the
[B9 acceptance](investigations/hardware-runs/shaker-b9-current-reference-2026-11-05.md),
[Type0 table correction](investigations/hardware-runs/crtc0-ivm-exit-table-correction-2026-11-05.md),
[AI residual](investigations/hardware-runs/shaker-crtc1-008d-current-2026-11-05.md),
and [next discriminators](investigations/hardware-runs/shaker-next-discriminators-2026-11-04.md).

## Demo acceptance

Renaud confirms manual MiSTer testing of **DSC4, Amazing and Before the Storm,
all working on CRTC0/1/3**, closing the reported demo defects. The tested
filename/SHA was not supplied; do not silently attribute this to an exact build.
[Manual acceptance](investigations/hardware-runs/demos-manual-acceptance-2026-11-06.md).

The source-bound native samples and BTF displacement measurements remain in
[classic display](investigations/hardware-runs/classic-ga-display-edge-2026-11-05.md),
[ASIC display](investigations/hardware-runs/asic-r3-rendered-position-2026-11-05.md)
and [BTF](investigations/hardware-runs/before-the-storm-r3-2026-11-05.md).
This does not change the SHAKER page tally or validate a CRT-response model.

## Maintaining the tally

Update the row in `shaker-results.csv` when a targeted run changes its known
status; do not add another count for a duplicate screenshot or repeated check.
Recompute this table from the ledger, retaining each row's source build and
reason. A capture's existence or successful script execution is not a passing
verdict. Keep partial acceptance in the detail field until the remaining page
observables are adjudicated. A broad fresh baseline follows known open fixes.

The historical350-pair summary mixes modules and overlaps the later D walk;
it must not be added to127 again. Its raw totals are300match/30phase/20different,
with later parent corrections and fixes applied per page here. Historical
execution detail is [archived](archive/shaker-status-before-numeric-tally-2026-11-05.md).
Primary comparison reports: [November3 sweep](investigations/hardware-runs/shaker-crtc1-sweep-2026-11-03.md),
[D walk](investigations/hardware-runs/shaker-package-device-2026-11-03.md),
[older execution](investigations/hardware-runs/shaker27-modules-b-e-2026-10-15.md).
