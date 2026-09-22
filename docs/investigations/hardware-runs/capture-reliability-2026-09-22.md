# Capture reliability evidence — 2026-09-22

Dated follow-up to
[shaker-repaired-95e6f56-2026-09-22.md](shaker-repaired-95e6f56-2026-09-22.md):
same build, same media, bounded probes of capture reliability only. No RTL
changed, no physical display was observed, no blanket closure. Source
findings, host gate and hardware limits are reported separately below.

## Scope and identities

- Session base `8e449ac`; transport fix `210eca4`; metadata fix `b4e824d`.
- Same RBF build `95e6f56`; host and device SHA-256
  `a4f6a4f30758c2456a177a40a098167ab570214b311dc03149b72b72c499e0a7`.
- Same `shaker27.dsk` media SHA-256
  `65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b`.
- Private evidence:
  [`docs/screenshots/capture-reliability-2026-09-22/`](../../screenshots/capture-reliability-2026-09-22/)
  in the main checkout. Private images and logs are untracked and not committed.

## Transport preflight

- The first preflight failed before device load with an SSH multiplex socket
  permission error (`unix_listener ... Operation not permitted`, exit 255);
  see [`b9-type0/last-run.log`](../../screenshots/capture-reliability-2026-09-22/b9-type0/last-run.log).
- Fix: commit `210eca4` adds `-o ControlMaster=no -o ControlPath=none` to ssh
  and both scp directions. Opus review `20260922T064228Z-11311-4ee4` approved;
  27 driver tests OK. Record:
  [`review/opus-transport-review.log`](../../screenshots/capture-reliability-2026-09-22/review/opus-transport-review.log).

## Live probes (updated)

- **B9, type 0** (`b9-type0-no-mux/`): reset BASIC captured first, 768×273,
  SHA-256 `253ce13dd7670b6b6af8f731cc93521120b3d183ee0bc77377cae96d3205a970`.
  Native screenshot of live B9 type 0: no PNG after 20 s. Direct scaled
  screenshot of the same live state: also no PNG after 20 s, SSH healthy
  throughout
  ([`b9-type0-scaled-diagnostic.json`](../../screenshots/capture-reliability-2026-09-22/b9-type0-scaled-diagnostic.json),
  `result.status = no_png`, dispatch exit 0).
- **C4, type 1** (`c4-type1-fresh/`): fresh core reset; BASIC captured with
  the same hash; then C4 state A captured twice — both 768×546, both SHA-256
  `0124f82d351479a905c2b0574dd53141a8197f70873b5e0d9605915ab7c39808`
  (pixel-identical repeats).

## Image comparison

Gemini log
[`review/c4-image-comparison.log`](../../screenshots/capture-reliability-2026-09-22/review/c4-image-comparison.log),
fresh C4/A versus prior `repaired-c4-type1/C4_type1_page1.png` (main checkout):

- Same identified state: `CRTC 1 PARITY TEST 3 (C4.0=1). ODD FRAME`.
- New versus prior differs only on even rows y=274..504; all odd rows, and
  y=0..273 and y=505..545, are identical.
- Even rows y=506..544 in both retain the prior B9/type-1 page-B text — not
  the preceding B9 type-0 run. This supports a history-dependent capture
  concern but does **not** prove a capture-only artefact or physical-output
  correctness.
- The 768×273 BASIC capture does not prove the 546-line buffer was cleared;
  no cold boot and no DDR wipe were performed.

## Sync filter configuration

- Full was explicitly requested (`--sync-filter full`); CFG bits 35/36 = 0
  were written and the CFG hash checked on each run. The applied runtime latch
  was **not** independently observed: native PNGs omit the OSD, and the runner
  never reads `sync_filter_applied`.

## Source findings

Exact-source verification:
[`review/main-source-verification.log`](../../screenshots/capture-reliability-2026-09-22/review/main-source-verification.log).
The provisional filter audit (`review/filter-audit-provisional.log`) is
deliberately **not** used for causal conclusions.

- Main pinned
  [`f8dc68e`](https://github.com/MiSTer-devel/Main_MiSTer/commit/f8dc68e3dcf4694f5593e6552aea56cd852982af):
  native and scaled screenshots read the **same** DDR buffer at `0x20000000`;
  scaled is an Imlib2 software resize inside Main
  ([`scaler.cpp#L509`](https://github.com/MiSTer-devel/Main_MiSTer/blob/f8dc68e3dcf4694f5593e6552aea56cd852982af/scaler.cpp#L509),
  [`scaler.cpp#L465`](https://github.com/MiSTer-devel/Main_MiSTer/blob/f8dc68e3dcf4694f5593e6552aea56cd852982af/scaler.cpp#L465)),
  not an independent HDMI capture; zero freshness validation in the examined
  capture path; no independent HDMI observation was made.
- Mode seam verified against source:
  [`rtl/Amstrad_motherboard.v`](../../../rtl/Amstrad_motherboard.v) lines
  960–1015 and [`rtl/crt_filter.v`](../../../rtl/crt_filter.v) lines 288–318.
  Full and Raw pixels feed the same regenerated acquisition H/VSYNC/blank
  tuple; Raw pixels only changes the byte/pixel mask; Raw CRT uses GA-shaped
  H/VSYNC, raw CRTC HSYNC as HBLANK, and GA VBLANK.
- Modes commit only at filtered VBLANK while `~cpu_n`, or on reset.
  Recommendation: fresh core loads with matched states for comparisons; no
  live sync-filter grant now.

## Field and buffer inspection

The [field inspection log](../../screenshots/capture-reliability-2026-09-22/review/field-geometry-inspection.log)
traces the shared framework scaler in `sys/ascal.vhd`: header construction at
1186–1201, odd/even row starts at 1236–1251, interlaced extra line pitch at
1564–1575, and independent field-buffer advancement at 1920–1931. The examined
Main `scaler.cpp` maps buffer zero and reads `header + y * line_pitch`; it does
not use header bytes 4–5 to establish field freshness. Main is a separate HPS
binary; `sys/ascal.vhd` is the repository's vendored framework source. Neither
was changed. The pinned Main source is a contract reference, not proof of the
running binary's exact source revision.

Progressive BASIC does not establish that the taller interlaced storage was
cleared. The residual even rows 506–544 correspond to field lines 253–272;
field truncation or buffer history are candidates, not established causes.
The initial claim that Full never writes any rows below 274 was rejected:
it conflicts with the observed odd-row C4 data and changed even rows.
A bounded next discriminator is repeated read-only 16-byte scaler headers,
with matched Full/Raw CRT cases and field/DE observations as needed. No live
header or field-pointer values were collected here, and no DDR scrub is proposed.

## Host gate and metadata fix

- Metadata fix landed as commit `b4e824d`: unobserved
  `applied_b6_config` values are now `null`; requested aliases
  (`sync_filter_requested`, `raw_crt_requested`) and provenance (`evidence`)
  are explicit and never derived from on-disk CFG bytes. The driver guide records the same distinction.
- Validation: 59 CSL tests OK (6 skipped); 77 driver/ring tests OK;
  `select_tests:` reported no simulation needed for the selected change set.

## Device restoration and release

- MENU restored; original CFG
  `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`
  verified; the runner's own temporary MBC removed; per-run temporary files
  cleaned up; balanced key operations completed with none left held.
- Recorded in [`device-release.json`](../../screenshots/capture-reliability-2026-09-22/device-release.json);
  release accepted; exclusive MiSTer ownership transferred to the B9 task.

## Hardware limits

- No RTL change, no physical display observation, and no blanket closure of
  the capture question.
- Native Astra review approved the Muse/MiMo metadata and help changes; the
  unsupported zero-overhead/hazard prose was removed as requested. See
  [`review/native-metadata-review.txt`](../../screenshots/capture-reliability-2026-09-22/review/native-metadata-review.txt).
- Source findings, host gate and hardware limits above are intentionally kept
  separate; capture-path behaviour remains open pending matched-state probes.

## Evidence paths

- Probes:
  [`b9-type0/`](../../screenshots/capture-reliability-2026-09-22/b9-type0/),
  [`b9-type0-no-mux/`](../../screenshots/capture-reliability-2026-09-22/b9-type0-no-mux/),
  [`c4-type1-fresh/`](../../screenshots/capture-reliability-2026-09-22/c4-type1-fresh/),
  `B9-type0-probe.csl`, `C4-type1-probe.csl`,
  [`b9-type0-scaled-diagnostic.json`](../../screenshots/capture-reliability-2026-09-22/b9-type0-scaled-diagnostic.json),
  [`device-release.json`](../../screenshots/capture-reliability-2026-09-22/device-release.json).
- Reviews:
  [`review/c4-image-comparison.log`](../../screenshots/capture-reliability-2026-09-22/review/c4-image-comparison.log),
  [`review/main-source-verification.log`](../../screenshots/capture-reliability-2026-09-22/review/main-source-verification.log),
  [`review/opus-transport-review.log`](../../screenshots/capture-reliability-2026-09-22/review/opus-transport-review.log).
