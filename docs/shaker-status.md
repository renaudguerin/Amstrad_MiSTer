# SHAKER status

Maintained page tally, 5 November 2026, against experimental source `c0939d8`.
The per-page [results ledger](shaker-results.csv) owns these counts. Results
use mixed builds: older accepted photo comparisons are carried forward unless
superseded or invalidated; the ledger records the build and evidence for each
page. This is an approximation of known status, not a fresh milestone baseline.

## Counts by CRTC and module

A unit is one unique SSM page, not one screenshot or individual assertion.
A page with any confirmed failing check is broken. A page with only partial
validation remains unknown, even when a specific defect on it is repaired.

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
| 0 | A | 1 | 0 | 5 | 116 | 122 |
| 0 | B | 0 | 0 | 221 | 0 | 221 |
| 0 | C | 1 | 0 | 21 | 0 | 22 |
| 0 | D | 0 | 0 | 1 | 110 | 111 |
| 0 | E | 2 | 1 | 8 | 0 | 11 |
| **0** | **Total** | **4** | **1** | **256** | **226** | **487** |
| 1 | A | 116 | 1 | 6 | 4 | 127 |
| 1 | B | 103 | 1 | 35 | 0 | 139 |
| 1 | C | 76 | 0 | 4 | 0 | 80 |
| 1 | D | 63 | 0 | 15 | 49 | 127 |
| 1 | E | 3 | 0 | 4 | 0 | 7 |
| **1** | **Total** | **361** | **2** | **64** | **53** | **480** |
| 3 | A | 0 | 0 | 9 | 115 | 124 |
| 3 | B | 0 | 0 | 8 | 113 | 121 |
| 3 | C | 0 | 0 | 0 | 23 | 23 |
| 3 | D | 0 | 0 | 0 | 150 | 150 |
| 3 | E | 0 | 0 | 1 | 4 | 5 |
| **3** | **Total** | **0** | **0** | **18** | **405** | **423** |
| **All** | **Total** | **365** | **3** | **338** | **684** | **1390** |

CRTC2/4 are not selectable in this core. There are no indexed module-E photos;
its passing/broken judgments use the qualified original-program checks.
CRTC3 has scoped successful checks on BI/AE/AR, but no retained complete-page
verdict: those15 pages remain unknown. Its old ungraded captures account for
most stale entries. This table does not classify those pages as failures.

## What the remaining counts contain

- **Confirmed broken:** C0 E6/page0409, cases27/28; C1 AI/008D;
  C1 B6/019D. E6's26/28 figure counts assertions across0407/0408/0409,
  separately from the page tally.
- **Repaired but not a whole-page pass:** B9's three3Fdelay rows are2740
  on MiSTer, while its MID glyph/reference binding remains open. S02D3
  has matching direct/history4E40, with photo4E30/program binding unresolved.
- **Unknown dynamic/display checks:** D6/D7 physical phase/settling, DI02E3
  (58/59; the program labels CRTC1 unreliable), and DT02F8/02FA's full
  phase-matched photo verdict. DT's reported missing band was an acquisition
  epoch mismatch; its early native trace contains both bands.
- **Stale CRTC1 captures:**44 D3 pages have invalid page-state alignment;
  five further D captures need acquisition/parameter recheck. Four old AI
  PPI-mask judgments need individual bit checks: the earlier broad masking
  wrongly hid008D's VSYNC discrepancy.
- **009A remains unknown:** fresh original direct-O MiSTer, CPCSyntaxError
  and AmSpirit runs show the noisy RAM pattern family. The older canonical
  walk was black; equivalent history is not yet qualified. The bounded
  AmSpirit prefix stopped on PLEASE WAIT and is preserved, not counted as
  either a pass or failure. No new R1/VMA rule follows from that experiment.

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
| 3 / ASIC | Credits clear; sampled main effects restored | Logo/lower effects restored | 275af8a acceptance; later005 preservation |

These are sampled hardware results, not full-demo sign-off. DSC4's CRTC1
bottom row also exists in AmSpirit's uncropped buffer; desired visible bounds
remain a viewport question. [Classic evidence](investigations/hardware-runs/classic-ga-display-edge-2026-11-05.md),
[ASIC evidence](investigations/hardware-runs/asic-credit-blank-boundary-2026-11-04.md),
[bottom-row comparison](investigations/hardware-runs/dsc4-bottom-row-crop-2026-11-04.md).

The R3 compensation is shared by classic CRTC0/1 and has deterministic
coverage for both. ASIC display timing uses its separate fix. BTF's reported
reversing wobble now has native before/after acceptance on **CRTC1**:
82eligible reference pairs include−10/+6reversals;176current pairs contain
only0/−2steps. CRTC0/3 BTF acceptance remains untested.
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
