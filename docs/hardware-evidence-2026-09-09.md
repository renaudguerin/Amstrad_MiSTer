# Hardware retest — 2026-09-09

## Build and evidence

Renaud tested `Amstrad_20260908_ce1d2da.rbf`. The original
[report](screenshots/testing_0909/README.txt) and 26 PNG captures are preserved
unchanged in [testing_0909](screenshots/testing_0909/).

- Source: `ce1d2da67c2598c0dd06208b9fc14c52ada01712`.
- Retained local RBF SHA-256, rechecked on September 9:
  `fef2c8553e85456a86bc6d28753cdbb43c386106b5cd719bbf808cea84dc6144`.
  The device copy was not independently hashed.
- Build record: full local Quartus 17.0.2, run `34228275825`, constrained
  setup +0.098 ns / hold +0.238 ns, zero TNS; 27 input and 90 output ports
  remain unconstrained. See [artifact provenance](current-status.md).
- Plus: **6128 Plus, Live blanking**, unless stated otherwise.
- Classic: **6128, Plus off, CRTC1**; DSC4 also sampled on CRTC0.
  Sync comparisons are recorded below. SHAKER version: **2.7**.
- MiSTer version, media hashes, output/scaler settings and complete reset/load
  order were not supplied. No Dandanator was tested.

These are user-reported hardware results. “OK”, “seems good”, “appears fixed”
and “could not reproduce” retain their limited scope; none closes an entire
subsystem. Flicker, audio pitch and crashes come from the report, not a still
image. RAM retention and shared-defect explanations below are hypotheses.

## Plus results

| Title / path | September 9 result |
|---|---|
| Arnold 5 | Still no keyboard. Loading cartridges such as Amstraddiag or Eerie Forest afterward crashes. |
| System / BASIC cartridges | Boot never completes; “disk missing” appears before any disk-access attempt. Disk reads could not be tested. Missing AMSDOS loading is the user's hypothesis, not an established cause. |
| Burnin' Rubber | Reported OK; retain September 2's right-edge-sprite pass. |
| Plotting | Fire still appears permanently pressed. |
| Pang | No change; possibly the same permanently pressed fire symptom. |
| Copter 271 | No change. |
| Navy Seals | Earlier black screen could not be reproduced. Almost perfect, but some sprite lines flicker within roughly the leftmost 1/16 of the screen. User explicitly corrects the old Dandanator association: no Dandanator was ever tested. |
| World of Sports / Amaury | User suspects the Navy Seals flicker is the same as World of Sports; reports it over Amaury's face (capture `015537`). Shared cause unconfirmed. |
| RoboCop 2 | Still mostly perfect; occasional flashing images during scrolling. |
| Switchblade | Black screen. |
| Enforcer / Tintin | “Seems good.” |
| Eerie Forest | Does not load. |
| Schnapps | Extreme slowdown and flickering in one scene. Report references a screenshot, but no supplied filename identifies Schnapps; capture association remains unresolved. |
| Xmas 17 | Display issue; capture `015858`. |
| Sonic | Extreme flickering; capture `020052`. |
| CRTC3 demo | Emulator warning, wrong-pitch DMA sound and midway crash persist. Right-edge sprite-column leak **appears fixed**; possible relation to Burnin' Rubber is unproved. Captures `022632`, `022652`, `022718`, `022747`, `022753`. |

## Classic and SHAKER results

| Test | September 9 result |
|---|---|
| Classic 6128, Live blanking / Off | Bottom screen line not fully drawn; report references a screenshot without uniquely identifying it. |
| DSC4 | Completely garbled and flickering on CRTC1 regardless of sync mode; CRTC0 garbled but not flickering. Six `dsc4` captures (`020425` through `020808`); individual capture settings not supplied. |
| Amazing Demo | Live blanking seems to perform the HSYNC effect correctly; Off is insufficient. Bottom half reportedly flickers between valid and garbled buffers. This qualifies the September 2 apparent pass: overall display remains defective. Captures `024615` (Full or Live blanking) and `024632` (Off); filename does not distinguish Full from Live for the first image. |
| SHAKER A (T) | Full: not garbled, but black shape is incorrectly square. Live blanking / Off: broken. Live reportedly shows flickering remnants of DSC4 at the bottom even after reset. Retained RAM versus stale displayed data is unresolved; this does not establish a reset bug. |
| SHAKER A (4), A (U) | Captured, user unsure how to interpret; **unassessed**, not passes or failures. |
| SHAKER B (1) | Live blanking corrupted, Off worse. Full capture supplied without an explicit pass verdict. |
| SHAKER B (9), C (1) | Captured, user unsure; **unassessed**. |

### SHAKER capture index

Labels use the existing [menu transcriptions](accuracy/shaker/menu-transcriptions.md).
The five otherwise unlabeled result screens were visually inspected for their
headings; identification does not validate their numerical results. No Logon
reference-photo comparison was performed in this documentation pass.

| Capture filename in `testing_0909/` | Entry / identification evidence |
|---|---|
| `20260909_021359-shaker_module_A_T_full sync.png` | A (T), Full; filename and report. |
| `20260909_021444-shaker_module_A_T_liveblanking_or_off.png`, `20260909_021454-shaker_module_A_T_liveblanking_or_off.png` | A (T); exact Live/Off assignment unresolved. |
| `20260909_021830-shaker.png` | A (4), UPDATE CRTC R0 TIMING; visible R0 updates and OUT/OUTI timing. |
| `20260909_021901-shaker.png` | A (U), R4 & R9 CHECKING; visible “RESULT OF CRT-R4 & R9 CHECK”. |
| `20260909_022017-shaker_sync_full.png`, `20260909_022025-shaker_sync_live_blanking.png`, `20260909_022033-shaker_sync_off.png` | B (1), INTERLACE C4/C9 COUNTERS; association inferred from report order and capture sequence, no visible menu label in inspected Full image. |
| `20260909_022251-shaker.png` | B (9), INTERLACE VM; visible “CRTC 1 INTERLACE VIDEO MODE” and frame-size measurements. |
| `20260909_022434-shaker.png` | C (1), RFD & PARITY STORY; visible “CRTC 1 R8 PARITY WITH RFD”. |

## Acceptance and next investigation

B1 and P10 remain open after this retest. B8's local regressions and synthesis
remain valid within their recorded scope, but do not establish title-level
closure. Untested items (including Pulpo, IA-5/Q17, disk operations and snapshot
restore) retain their own pending gates. The Dandanator ownership fix is a
separate source/simulation result; it cannot explain the reported Navy Seals
incident given the corrected load history.

A dedicated diagnostic session should first compare the identified SHAKER
captures with matching CRTC1 reference photos and interpret A (4)/(U), B (9)
and C (1) before assigning defects. Keep Classic and Plus investigations
separate. Choose one reproducible symptom per stream, pin media/settings and
reset sequence, then trace the relevant production boundary. Distinguish raw
CRTC/GA timing, selected blanking and scaler output for Classic; distinguish
sprite composition from later video stages for the left-edge Plus flicker.
Use a short video or repeated captures when temporal behaviour matters. Only
propose timing/state RTL changes after a source-grounded failing regression;
screenshot similarity alone does not establish a common cause.

**Subsequent investigation:** [September 10 code diagnosis](hardware-diagnosis-2026-09-10.md)
compares the B (9) numerical result with the reference and reproduces an
origin-VSYNC defect, while retaining the distinction between a code finding
and hardware closure. It also records the user's clarification that the
right-edge sprite result is a nonregression, and that System/BASIC boot blocks
disk-based software specifically in Plus/CRTC3 mode.
The [same-day second pass](hardware-diagnosis-2026-09-10-second-pass.md)
transcribes every SHAKER capture against the reference photographs: A (U) and
C (1) match, B (9) shows the origin-VSYNC defect on both CRTC types, and the A (4)
`FEFF` versus `5E5F` difference is PPI port B configuration bits, not CRTC timing.
