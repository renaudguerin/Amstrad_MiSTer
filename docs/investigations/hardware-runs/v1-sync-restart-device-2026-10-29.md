# V1 sync-restart test artifact and bounded device receipt — 2026-10-29

The hosted test build passes its exact-SHA CI and timing gates. Both BR/E
numeric counter guards pass on MiSTer. Cold T-group Full captures retain
readable headers and page progress. Raw CRT reaches the requested page with
mode 2 applied, but its native images are distorted. This is bounded regression
and acquisition evidence; direct GA pulse delivery and analog fidelity remain
unobserved. No master integration or release was performed.

## Build identity

- [Hosted run 40942767030](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/40942767030), exact SHA `cfd0016e00ddc5527364d28b02641f6d1db7cdf3`; compiled change `b144db18a524651d679fbed1f79724b3ea95bf62`.
- All selected simulation/lint, production-T80, full synthesis, effort and timing steps succeed. No duplicate dispatch or local gate run was made.
- Artifact `Amstrad-build-325-1-full`; Quartus 17.0.2 Build 602, `build_mode=clean_full`, High Performance Effort. Minimum setup slack 0.567 ns, hold 0.222 ns; all reported TNS zero.
- `Amstrad_20261029_cfd0016.rbf`, SHA256 `7e4a905dcc0c749e0790a5d50fec741f5e04ec58e51d00216de5751a78352c83`, matches the distinct device copy under `/media/fat/_Computer/`.

Reports, GitHub metadata and acceptance JSON are retained under
`local/investigations/classic-video-v2-2026-10-29-b157/v1-build/`.
The earlier VM DNS failure did not trigger host changes; the user selected
GitHub and the coordinator dispatched this single hosted build.

## Device cases and observed limits

A remained sole operator of `root@mister`. Cases use classic 6128, Plus Off,
French ROM/keyboard, SSM On, with CRTC selected by script and mode selected
through CFG/reset. The model/Plus settings are write evidence; format 2 SSM
records independently report acquisition mode requested/applied. The generic
manifest `applied_b6_config` remains labelled requested-only: use each event's
`sync_observation`, not that placeholder, for the observations below.

Media is unchanged `shaker27.dsk`, SHA256
`65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b`.
The packet preserves original boot prefixes and within-group progression;
these bounded scripts do **not** establish full Module-A/B prehistory.

| Case | Progress / captures | Observed result |
|---|---|---|
| BR CRTC1 Full | 01F0–01F4; six captures, no failures | BR/E shows R0=1/R3=1 CALC#34 and R0=0/R3=1 CALC#FF: numeric guard passes. Mode 0/0. |
| BR CRTC0 Full | 01F0–01F4; six captures, no failures | BR/E shows #34 for both rows: negative control passes. Mode 0/0. |
| Cold T CRTC1 Full | 00BD–00C2; nine captures, no failures | A/E/F explicit captures and intervening pages have readable titles/footers. E has the wide black effect band; F has separated bands. Mode 0/0 at A/E/F. |
| Cold T CRTC1 Raw CRT | 00BD–00C1; six captures, no failures | Mode 2/2 at A/E. Native images show displaced/coloured bands, wrapped content and a noisy lower region. Progress/capture succeeded; visual/analog acceptance is not established. |

All 27 PNGs were inspected, including automatic SSM-triggered captures and
explicit line captures. Full's legible output is not a quantitative old/new
pixel comparison or proof of CTM curvature. The Raw CRT observation has no
matched older-RBF Raw CRT capture in this batch, so it does not establish a
regression caused by this change. A returned PNG is not proof of fresh scaler
content; buffer age and the physical connector's displayed output are unknown.
Format2 SSM does not expose GA HS pulse counts or HSYNC_RESTART. Neither its
mode observation nor these PNGs prove the intended pulse waveform.

The bounded packet, script hashes, commands, manifests, SSM records, all native
PNGs and contact sheets are retained under the same bundle's `v1-hardware/`.
`run.py` records the four cases; `run.log` records their successful completion.
P0 captures after derived line 24; Full T captures lines 16/24/26 (A/E/F), and
Raw CRT stops/captures at line 24 (E). No Raw-pixels, full-prefix, D3/D4 or demo
retest was added. The stored `MiSTer.ini.preflight` reports direct_video=0,
vga_scaler=0, forced_scandoubler=0 and composite_sync=0; these are configuration
values, not runtime connector measurements.

Each case restores the original CFG and returns to Menu. Final readback is
`MENU`, CFG SHA256
`f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`.
No device operation remains in flight. A retains ownership pending explicit
handoff. DSC4/Amazing repair and monitor-response fitting remain separate gates.
