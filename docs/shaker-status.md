# SHAKER status

Maintained page tally, 6 November 2026. Page judgments carry forward the
`c0939d8` milestone and qualified earlier builds; current trial `fdf348c`
adds scoped ASIC preservation, 44 fresh aligned CRTC1 D3 captures and the
ongoing current-build archive refresh. CRTC0 A–E acquisition is complete except
two missing PNGs; its 44 canonical D3 identities remain stale pending corrected
manual capture. Fresh unadjudicated images become unknown, without new passing
grades. A separate full-resolution C0 A4/AI check adds one passing and one
broken judgment to previously unknown coverage. [Archive progress](investigations/hardware-runs/shaker-milestone-archive-2026-11-06.md).
The per-page [results ledger](shaker-results.csv) owns these counts. Results
use mixed builds: older accepted photo comparisons are carried forward unless
superseded or invalidated; the ledger records the build and evidence for each
page. This is an approximation of known status, not a fresh milestone baseline.

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
| 0 | B | 0 | 0 | 221 | 0 | 221 |
| 0 | C | 1 | 0 | 21 | 0 | 22 |
| 0 | D | 0 | 0 | 67 | 44 | 111 |
| 0 | E | 2 | 1 | 8 | 0 | 11 |
| **0** | **Total** | **5** | **2** | **436** | **44** | **487** |
| 1 | A | 118 | 2 | 7 | 0 | 127 |
| 1 | B | 103 | 1 | 35 | 0 | 139 |
| 1 | C | 76 | 0 | 4 | 0 | 80 |
| 1 | D | 63 | 0 | 59 | 5 | 127 |
| 1 | E | 3 | 0 | 4 | 0 | 7 |
| **1** | **Total** | **363** | **3** | **109** | **5** | **480** |
| 3 | A | 0 | 0 | 9 | 115 | 124 |
| 3 | B | 0 | 0 | 8 | 113 | 121 |
| 3 | C | 0 | 0 | 0 | 23 | 23 |
| 3 | D | 0 | 0 | 0 | 150 | 150 |
| 3 | E | 0 | 0 | 1 | 4 | 5 |
| **3** | **Total** | **0** | **0** | **18** | **405** | **423** |
| **All** | **Total** | **368** | **5** | **563** | **454** | **1390** |

CRTC2/4 are not selectable in this core. There are no indexed module-E photos;
its passing/broken judgments use the qualified original-program checks.
CRTC3 has scoped successful checks on BI/AE/AR, but no retained complete-page
verdict: those15 pages remain unknown. Its old ungraded captures account for
most stale entries. This table does not classify those pages as failures.

## What the remaining counts contain

The focused current [CRTC0 A4/AI comparison](investigations/hardware-runs/shaker-crtc0-a4-ai-photo-check-2026-11-06.md)
adds an AI passing judgment and the A4 OUTI mismatch to previously unknown
coverage; this does not establish a regression. Current CPCSE26/27 match the
physical CRTC0 A4 result, unlike its CRTC1 result.

- **Confirmed broken:** C0 E6/page0409, cases27/28; C0/1 A/0037; C1 AI/008D;
  C1 B6/019D. E6's26/28 figure counts assertions across0407/0408/0409,
  separately from the page tally.
- **Repaired but not a whole-page pass:** B9's three3Fdelay rows are2740
  on MiSTer, while its MID glyph/reference binding remains open. S02D3
  has matching direct/history4E40, with photo4E30/program binding unresolved.
- **Unknown dynamic/display checks:** D6/D7 physical phase/settling, DI02E3
  (58/59; the program labels CRTC1 unreliable), and DT02F8/02FA's full
  phase-matched photo verdict. DT's reported missing band was an acquisition
  epoch mismatch; its early native trace contains both bands.
- **D3 acquisition repaired:** all 44 current headers match after correcting
  one extra Space in our private manual overlay. These pages are unknown
  pending full digital-content/blanking comparison; CRT deflection belongs to
  V1 separately. Five further D captures
  remain stale. [Count correction](investigations/hardware-runs/shaker-d3-count-correction-2026-11-06.md).
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
DSC4/Amazing preservation pass. Opus/promotion coverage and physical HDMI/CRT
acceptance remain open.
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
