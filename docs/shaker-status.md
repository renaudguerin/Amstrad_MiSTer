# SHAKER status

Maintained page tally, 6 November 2026. Page judgments carry forward the
`c0939d8` milestone and qualified earlier builds; current trial `fdf348c`
adds scoped ASIC preservation, correctly aligned D3 captures on all three
selectable CRTCs, and the completed archive refresh. A–E acquisition on CRTC0/1/3
is complete: 1,384 images and six explicit gaps; DT0305 on all three remains stale
because its exit grab contains the menu. Fresh unadjudicated images become
unknown. A separate full-resolution C0 A4/AI check adds one passing and one
broken judgment to previously unknown coverage. [Archive progress](investigations/hardware-runs/shaker-milestone-archive-2026-11-06.md).
The targeted9c11daf CRTC3 DU check repairs status1 bit5;03BF passes its
complete program checks while03C0 remains broken on status2 bits0/4.
The per-page [results ledger](shaker-results.csv) owns these counts. Results
use mixed builds: older accepted photo comparisons are carried forward unless
superseded or invalidated; the ledger records the build and evidence for each
page. This is an approximation of known status, not a fresh milestone baseline.

Read the [reviewed static-reference text cache](shaker-reference-text.md) before reopening covered text-only photos; unresolved fields still require the source image.

## Counts by CRTC and module

A unit is one unique SSM page, not one screenshot or individual assertion.
A page with any confirmed failing check is broken. A page with only partial
validation remains unknown, even when a specific defect on it is repaired.
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
| 0 | A | 2 | 1 | 119 | 0 | 122 |
| 0 | B | 0 | 1 | 220 | 0 | 221 |
| 0 | C | 1 | 0 | 21 | 0 | 22 |
| 0 | D | 0 | 0 | 110 | 1 | 111 |
| 0 | E | 2 | 1 | 8 | 0 | 11 |
| **0** | **Total** | **5** | **3** | **478** | **1** | **487** |
| 1 | A | 118 | 2 | 7 | 0 | 127 |
| 1 | B | 103 | 1 | 35 | 0 | 139 |
| 1 | C | 76 | 1 | 3 | 0 | 80 |
| 1 | D | 63 | 0 | 63 | 1 | 127 |
| 1 | E | 3 | 0 | 4 | 0 | 7 |
| **1** | **Total** | **363** | **4** | **112** | **1** | **480** |
| 3 | A | 0 | 0 | 124 | 0 | 124 |
| 3 | B | 1 | 2 | 118 | 0 | 121 |
| 3 | C | 0 | 0 | 23 | 0 | 23 |
| 3 | D | 1 | 1 | 147 | 1 | 150 |
| 3 | E | 0 | 1 | 4 | 0 | 5 |
| **3** | **Total** | **2** | **4** | **416** | **1** | **423** |
| **All** | **Total** | **370** | **11** | **1006** | **3** | **1390** |

CRTC2/4 are not selectable in this core. There are no indexed module-E photos;
its passing/broken judgments use the qualified original-program checks.
CRTC3 now has current B6 and DU03BF program-self-check passes. Scoped BI/AE/AR
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
native01/00 pair. A4 stays broken: six existing lower R0=1 first-byte VS
bits still disagree after the upper-PPI allowance. The changed extra C9
sweep matches current CPCSE but is absent from the photo and remains unknown.

- **Confirmed broken:** C0 E6/page0409, cases27/28; C0/1 A/0037; C0/1 B6/019D;
  C1 AI/008D; current C1 S/02D3 (0840 versus4E40); C3 BR/01F0 and01F1, DU/03C0 and E/0406. E6's26/28 figure counts assertions across0407/0408/0409,
  separately from the page tally.
- **Repaired but not a whole-page pass:** B9's three3Fdelay rows are2740
  on MiSTer, while its MID glyph/reference binding remains open. Earlier S
  direct/history4E40 passes are superseded by the current failing history
  capture; same-RBF direct and an unchanged history repeat give4E40.
  [Current S evidence](investigations/hardware-runs/shaker-s-current-2026-11-06.md).
- **Current CRTC3 numeric failures:** original Type3 callers/tables qualify
  BR01F0 and01F1 as broken against the current program. The fresh peer
  passes; an original ACK/R52 trace must locate the responsible boundary.
- **Current CRTC3 status/overflow:** DU03C0 and E0406 fail their original
  checks; completed AmSpirit1.17.1 passes all. Status2bit4 has an apparent
  literal-table/collector tension, and E's final ROM-select race is already
  excluded from the implementation. [Evidence](investigations/hardware-runs/shaker-crtc3-status-overflow-2026-11-06.md).
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
  008F remains unknown because its physical V2 exception glyph is unresolved,
  and 0090 awaits full cell adjudication. The 0037 OUTI bit 0 mismatch cannot
  be masked as an unrelated upper-PPI input difference.
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

## DSC4 and Amazing acceptance

| CRTC | DSC4 | Amazing | Last targeted native evidence |
|---:|---|---|---|
| 0 | Credits clear; sampled main effects restored | Sampled effects preserved | 208bacf preservation |
| 1 | Credits clear; sampled main effects restored | Sampled effects preserved | c0939d8 preservation |
| 3 / ASIC | Credits clear; sampled main effects restored | Logo/lower effects restored | fdf348c targeted preservation |

These are sampled hardware results, not full-demo sign-off. DSC4's CRTC1
bottom row also exists in AmSpirit's uncropped buffer; desired visible bounds
remain a viewport question. Fresh CRTC1 AmSpirit runs also show DSC4's long
green lines and Amazing's later coloured bands; their physical-CPC status
remains unverified. [Reference comparison](investigations/hardware-runs/dsc4-c1-main-lines-reference-2026-11-05.md),
[classic evidence](investigations/hardware-runs/classic-ga-display-edge-2026-11-05.md),
[ASIC evidence](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md),
[bottom-row comparison](investigations/hardware-runs/dsc4-bottom-row-crop-2026-11-04.md).

The R3 compensation is shared by classic CRTC0/1 and has deterministic
coverage for both. ASIC display timing uses its separate fix. BTF's reported
reversing wobble now has native before/after acceptance on **CRTC0/1/3**.
For CRTC0, old e24 has 86 eligible pairs with−10/+6 reversals; current fdf has
43 holds/43 left-two steps in 86 pairs. CRTC1 retains 82/176 eligible pairs
before/after, and CRTC3 retains 176/168. All current middle-band steps are 0/−2.
All new CRTC0 frames are viewed; six rejected acquisition attempts remain
recorded across its old/new runs. Static LEFT8, exact BI and scoped AE/AR/
DSC4/Amazing preservation pass. Fresh Opus review and the enabled mixed-colour sprite proof pass; physical
HDMI/CRT acceptance remains open.
[BTF evidence](investigations/hardware-runs/before-the-storm-r3-2026-11-05.md).

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
