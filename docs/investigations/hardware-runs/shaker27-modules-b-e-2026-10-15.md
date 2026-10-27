# SHAKER 2.7 suite + sync-filter capture comparison — 2026-10-15/16

Tracked copy of the session report. Its run artifacts (captures, manifests, logs, scripts,
`codex-verify.md`) stay ignored under `docs/specs/shaker-overnight-2026-10-15/` and
`docs/specs/shaker-compare-2026-10-15/` on the macmini checkout; relative paths such as
`runs2/` below are relative to the first directory. The runner fixes in §3 and §6 were
committed as `2732443` and the triage tools as `de4ae90`.

Dated evidence report for the overnight MiSTer session. Objective: run the
SHAKER modules B–E across CRTC types 0/1/3 on the current RBF, capture the
scalers' screenshots, then re-run the capture most likely affected by the
OSD **Sync filter** setting (Full / Raw pixels / Raw CRT) to separate
capture-pipeline effects from core behaviour before judging the AmSpirit
reference comparison of `docs/specs/shaker-compare-2026-10-15/`.

Emulator agreement is diagnostic evidence; hardware photographs and the ACCC
remain the authority. All composites and metrics below are triage signals,
not verdicts.

## 1. Identities

- MiSTer `root@mister`; CFG
  `f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`
  verified after every stage. The runner restores the CFG at the end of each
  cell but leaves the core loaded; each cell reloads the RBF and re-inserts
  its own media, and the core returns to MENU only when asked to (verified at
  the end of the session).
- RBF `Amstrad_20260929_cec641c.rbf` `40f7d66b…`; `shaker27.dsk`
  `65eb43e1…`; `shaker26.dsk` `f7082f8e…`; Plus cartridge `6128_FR.cpr`
  `ab241580…`. Runner MBC `/tmp/mbc-task-shaker` `0e99082b…` (re-provisioned
  after the 2026-10-16 reboot).
- Runner: `scripts/hardware-loop/csl_runner.py` via `phase2.sh`
  (cells/`SKIP27`/`SYNC_FILTER`/`RUNS`/`RESULTS` env overrides). Media pins
  are derived from each script's own `disk_insert` line (see §3, D1).
- Host: `/tmp/shaker-venv` (Pillow 12.3, numpy 2.5) for image tooling.

## 2. Suite execution (Full mode)

Per-cell outcome, `results.jsonl` / `results-raw-crt.jsonl` /
`results-raw-pixels.jsonl`. "caps" = completed screenshots.

| Cell | Full | Raw CRT | Notes |
|---|---|---|---|
| A0/A1/A3-27 | PASS 122/127/123 | PASS 122/127/123 | |
| B0-27 | FAIL | FAIL | `00FB` grab times out; tolerate rerun records it but the walk still dies at `wait_ssm 00FC` (CRTC0 marker wedge, §3) |
| B0-26 | FAIL (phase 1) / PASS (tolerate a7, a8) | not run | phase 1 aborted at `00FB` with 2 caps; tolerate walks all 344 cmds (`00FA` + tolerated `00FB`); `00FA` pixel-identical a7-vs-a8 |
| B1-27 | FAIL | FAIL | SSM `00E5` never arrives; 26 partial caps |
| B1-26 | PASS 137 | PASS 137 | |
| B3-27 | PASS 121 | PASS 121 | |
| C0-26 | "PASS" 0 caps | "PASS" 0 caps | **false pass** — see §3 |
| C0-27 | FAIL | not run | SSM `029E` never arrives |
| C1-26 / C1-27 | PASS 80 / FAIL | PASS 80 / — | 27 stalls at `02F1`; 75 partial caps |
| C3-26 / C3-27 | PASS 23 / FAIL | PASS 23 / — | 27 stalls at `03AD`; 23 partial caps |
| D0-27 | FAIL | not run | `02EA` grab times out (8 caps phase 1); tolerate a7: 29 caps `02E3`–`0302`, then dies at `wait_ssm #0303`; a8/a9 aborted at the `02EA` preflight on a7's late grab (rc=1, §3) |
| D0-26 | FAIL (phase 1) / PASS (tolerate a7, a8) | not run | phase 1 aborted at `02EA` with 8 caps; tolerate: 110 caps `02E3`–`0392` per attempt; 105/111 pixel-identical a7-vs-a8 (4 diffs are scroll/phase races on animated pages, 2 are ≤6 px) |
| D1-26 | PASS 127 (phase 1; tolerate a5 PASS 127) | not run | a5 same-mode rerun confirms the phase-1 PASS |
| D1-27 | FAIL | not run | pin fixed (§3); stalls at `wait_ssm 0x0305` with 30 caps (`02E3`–`0304`), identical across attempts a4–a7 — deterministic |
| D3-26 / D3-27 | PASS 150 / FAIL | not run | 27 stalls at `02FB`; 24 partial caps |
| E0-26 / E0-27 | PASS 7 / FAIL | not run | 27 stalls at `0407`; 7 caps |
| E1-27 | PASS 7 | not run | |
| E3-26 / E3-27 | PASS 4 / FAIL | not run | 27 stalls at `0406`; 4 caps |

Failure determinism: every failing cell failed identically on all three
attempts (three marker classes: Main capture timeouts `00FB`/`02EA`; SSM
waits listed above; D1 media preflight). Raw-crt reruns of the failing
B0/B1-27 cells failed identically in raw-crt, so the stalls are not
filter-dependent.

Residual coverage notes: B0-26 now walks the whole module (2 caps — the module
emits only 2 markers on CRTC0) and D0-26 captures 110 pages per attempt; E
(4–7 caps) cells still only partially capture before their stalls; E has no
AmSpirit references at all; C0 has none until the diagnostic below is
explained.

## 3. Execution defects found and fixed

- **D1-27 media pin**: phase 1 passed `--expect-media-sha256 <shaker27>` for
  every 27-suite cell, but `SHAKER-D-1.CSL` (2.7) actually inserts
  `shaker26.dsk` (only its `wait`→`wait_ssm` stepping differs from 2.6). The
  runner correctly rejected the preflight all three times; this was a
  harness bug, not a device fault. `phase2.sh` now derives the expected pin
  from the script's own `disk_insert` line. Redone: see the D1-27 item in §6.
- **C0 zero-capture success (diagnosed)**: `SHAKE26C-0.CSL` (and the 27-suite
  `SHAKER-C-0.CSL`) emit no SSM records on CRTC0 — both suites stall at the
  first expected marker `029E` (C5/A parity page, "CRTC 0,2 : PARITY CHECK
  SELECT") — while the same tests work on CRTC1/3 and the reference has 19
  CRTC0 module-C pages. The runner reports `status=success` when all commands
  complete despite zero captures, so phase 1 recorded a false PASS.
  `c0-probe.sh` then captured four points — the module menu (line 13), the C5
  parity page (17), the menu again after SPACE (19) and the C7 page (23) — in
  both the native and the scaled screenshot route
  (`runs-diag/C0-probe-native/`, `runs-diag/C0-probe-scaled/`):
  - line 13 (menu): **captured** in both routes — the machine is alive and
    displaying a normal lockable mode-1 menu, so the capture machinery works;
  - lines 17, 19 and 23: **all three time out**, in both routes.

  The parity test therefore leaves the machine producing no capturable frames
  through the observed walk in either route: the line-19 capture follows a
  SPACE keypress and still fails, and the silence holds to the end of the
  walk. With zero markers over the whole module, the C5 parity test on CRTC0
  either hangs the test program before its marker or leaves the display in a
  state nothing downstream can lock. That is a core-behaviour candidate, not
  an establishment — both probe routes share the scaler DDR source
  (Codex C5 correction), so joint starvation cannot exclude a persistent
  scaler/capture-path failure and does not prove "no lockable frames" or
  "never recovers"; only failure through the observed walk is established.
- **Frameless pages aborted the walk**: a Main grab that times out on a page
  whose test breaks video sync was fatal, so B0 lost everything after its
  second page (`00FB`) and D0 everything after its ninth (`02EA`). The runner
  now takes `--tolerate-capture-timeout` (default off): the failure is
  recorded in `captures` and in a new `capture_failures` manifest list, and
  the walk continues. Four offline tests in
  `scripts/hardware-loop/test_csl_runner.py` pin the default abort, the
  tolerant host-capture path, the SSM-marker path (the one B0/D0 use), and the
  SSH-timeout-at-deadline shape. `b0d0-retry.sh` re-runs both cells, 2.7 and
  2.6, in Full mode. Cross-provider review (`codex-verify.md`, trimmed pass)
  found three defects in the first diff; all three are fixed (§6, `2732443`).
- **Tolerated-timeout classification, corrected on hardware**: the flag's
  first device run (B0-27 attempts a4–a6, 2026-10-16 05:01Z) still aborted,
  because the classification matched the canonical message
  `Capture … timed out after 20.0s` while the text actually recorded was the
  transport's own `SSH command timed out after 0.14s: test -f …` — the
  deadline had expired *during* the final poll. Both capture paths now raise a
  typed `CaptureTimeoutError` (`driver.py`) and the runner classifies on the
  type; a fourth test pins the SSH-timeout-at-deadline shape. `b0d0-retry.sh`
  re-runs from a7.

### The capture-timeout family

Three pages are known to starve the scaler on this RBF: `00FB` (B1/A1
INTERLACE C4/C9 COUNTERS), `02EA` (DR/A VSYNC TORTURE (LOCK MECHANISM)) and
the C5/A parity page. All three are tests that deliberately manipulate video
sync. On B0/D0 the marker itself arrives and the *next* capture dies; on C0 no
marker ever arrives at all, and the probe shows the starvation persisting
through the end of the walk rather than page-local (line 19, back at the
menu, still starves). Whether the frameless state is core behaviour or a
scaler-lock consequence of it is not settled by host-side evidence — it needs
the real-hardware path. The salvaged D0-27 `02EA` late grab (readable
VSYNC-torture text page, `runs2/D0-27-full-a7/`) shows the D0 member of the
family is transient lock loss, unlike C0's walk-persistent starvation.

The B0-27 retry (attempt 7, flag on) separates the two failure layers on CRTC0.
The `00FB` capture failure is now recorded and tolerated (`capture_failures=1`,
canonical `Capture … timed out after 20.0s` message), and the walk then dies at
the *next* marker — line 18 `wait_ssm 0x00FC`, "no SSM #00FC arrived within
120s". So after a sync-breaking test on CRTC0 the machine stops emitting SSM
markers altogether, the same shape as C0's parity page. Marker waits are
deliberately not tolerated (a missing marker can mean a wedged machine), so the
2.7 B0/D0 cells cannot complete on CRTC0 under any capture policy; the 2.6
scripts use fixed waits and do walk the whole module.

## 4. Sync-filter comparison (phase 2)

Modes: Full = regenerated acquisition tuple; Raw pixels = same tuple,
byte/pixel mask only; Raw CRT = GA-shaped H/VSYNC, raw CRTC HSYNC as HBLANK,
GA VBLANK (see `docs/investigations/hardware-runs/capture-reliability-2026-09-22.md`).

Method (all reusable, `scripts/shaker-ref/README.md`):

- `compare_runs.py` — pair diff of the same (cell, version, CRTC, hex)
  capture in two run trees.
- `analyze_runpair.py` — integer alignment search + flat/shift/content
  classification.
- `compare_modes.py` — paired mode-vs-reference deltas over `compare.py`
  results (best mean per key per side).

Capture-geometry caveat: Full captures are 768×273; Raw CRT captures vary
(800×287, 928×287, …) because the mode feeds the scaler uncropped windows.
Raw-pixels keeps Full's tuple (768×273) and changes only the byte/pixel mask,
so it is the control for geometry effects. Cross-mode diffs are therefore
dominated by window/phase offsets; the aligned residual is the content signal.

### 4.1 Paired MiSTer-vs-MiSTer comparison (Full vs Raw CRT)

Every paired capture differs; after the alignment search the residual splits
into three classes, verified by inspecting the composites:

- **Window/phase offset** (the bulk): same screen content at a different
  capture-window origin and/or a different asynchronous grab phase.
- **Phase noise on animated pages**: tests that alternate two graphics show a
  different half of the alternation per mode. The same-mode control run
  (B1-27 raw-crt a1 vs a2) already differs up to mean 112 on such pages, so
  cross-mode diffs there carry no signal.
- **Lock-loss artifacts, in both directions** — both modes fail, differently,
  on the tests that manipulate sync timing:
  - Full: A3 RVM pages `000F`–`0026` capture as scrambled noise.
  - Raw CRT: the same pages capture as flat yellow fields; and on A6/D
    `007F`–`0082` (R13 update in 2 µs), AT/F `00C2` (R2 update during HSYNC)
    and AR/A1–A2 `00AF`/`00B4` (mode update > HSYNC delay) raw-crt shows
    torn/wrapped or wrong-window pages where Full stays readable (shifted).

  On those pages cross-mode acquisition is unreliable, so pixel
  metrics against the reference are not verdicts there. (The stronger claim
  that *neither* capture is the screen state at the marker exceeds the sampled
  evidence — Codex C2 correction, see `codex-verify.md`: the composites show
  disrupted/wrapped pages in both directions but do not prove scaler lock loss
  as the cause, nor cover whole page ranges.)

### 4.2 Mode vs AmSpirit reference

Per-cell medians put raw-crt worse than Full nearly everywhere; the exception
is the A3-27 cell, where raw-crt scores closer. That is a metric artifact, not
content agreement: the A3 reference pages are mostly-yellow fields, and raw's
lock-loss capture is an all-yellow field, so it matches numerically (A3 `0010`:
raw mean 63.6 vs Full 129.9 against a reference the Full capture misses by
scrambling) while reproducing none of the stripes or text. Conversely the A6
`007F`–`0082` regressions are raw-crt tearing where Full was close.

### 4.3 Content signal

Where both modes lock, content is mode-independent: B1 `01F4`'s `CALC=#34`
line is text-identical in Full and raw-crt, and A0 `00C2` shows the same text,
boxes and values in Full (shifted) with raw-crt torn. So the §5 triage
candidates that are text/verdict disagreements survive the filter change; the
pattern/geometry candidates on sync-timing pages remain capture-limited and
need hardware or a lockable capture route.

### 4.4 Raw pixels (control)

Raw-pixels keeps Full's geometry (768×273) and acquisition tuple and changes
only the byte/pixel mask, so it isolates the mask from the window effect. Over
the three A cells (372 pairs, `compare/runs-full-vs-raw-pixels*`,
`analysis-full-vs-raw-pixels.txt`):

- 280 pairs have *aligned*-mean 0.0 and 37 more differ by at most 3/255 —
  i.e. 317/372 = 85.2% are equal or rounding-level **on the aligned metric**
  (Codex §4.4 check: unaligned, 288 records are zero and 35 more are ≤3, i.e.
  86.8%; two aligned-zero records, A1 `0096`/`0097`, have nonzero unaligned
  means — so "pixel-identical" without the aligned qualifier overstates it).
  Per-cell aligned-mean: A0 median 0.0, p90 ≈0.005, max 0.396; A1 median 0.0,
  p90 0.0.
- 44 pairs are classified `content-change`, and every one is a
  sync/register-update test: 43 in A3 (A3/CRTC RVMB `000F`–`0036`, R13 UPDATE
  IN 1/2/4 USEC SCREENS `0038`–`0083`, R1/R6 STORIES, SKEWDISP ON R0 RUPTURE)
  and one in A1 (AO/I R1 STORIES `009A`). Inspected composites show both
  captures carrying the skewed or scrambled display the test produces, at a
  different rupture point — the grab is a race there, not a mask effect.
- The mode-vs-reference arithmetic agrees: A0 and A1 are 122/122 identical
  verdicts (median delta 0.0) and A3 is 99 same, 11 better, 10 worse — all of
  the movers on those same families (`modes-full-vs-raw-pixels.txt`; these
  counts come from `compare_modes.py` over the `compare.py` per-mode results,
  not from the alignment JSON, and were outside the Codex check boundary).

So the mask is content-neutral wherever the capture locks, and raw-crt's large
deltas are attributable to its window/sync path, not to the byte/pixel mask.
The residual caveat is that the A-cell runs have a single attempt each, so a
same-mode control (like the B1-27 raw-crt a1-vs-a2 pair) does not exist for
these pages; the classification rests on the test families involved.

## 5. AmSpirit reference comparison status

Full-set triage (2026-10-15, `docs/specs/shaker-compare-2026-10-15/`):
top same-screen candidates BR/E `01F4` (CRTC1 `CALC=#34` vs `#FF`), DE/A
`03AE`+DE/C `03B4` (WRONG vs OK verdict), B6/A `019D`, AI/A `008D`, DR/A
`02EA`; the `FE/FF` vs `1E/1F` offsets are upper PPI input bits, not parity.
Mode sensitivity: `01F4` text is identical in Full and raw-crt (checked), so
that divergence survives the filter change; `019D` in raw-crt shows only the
page title at the same marker (capture settle phase differs by mode), so
2.6-path candidates remain provisional pending hardware or a 2.7-path rerun.

## 6. Open items

- C0: the probe maps the starvation region (menu OK → C5 parity page starves →
  menu still starves → C7 starves; both capture routes, which share the scaler
  DDR source). The underlying question — does a real CRTC0 display the C5
  parity page and continue the module? — needs the hardware path: the AmSpirit
  reference has the page, so either this core diverges on CRTC0 or the page
  only exists in that emulation.
- B0/D0: tolerate retry done (`b0d0-retry.sh`, `results-tolerate.jsonl`).
  B0-26 walks the whole module on fixed waits (PASS, 2 caps: `00FA` + tolerated
  `00FB`; listener records markers `00FA`/`00FB`, then silence). D0-27 a7
  captured 29 pages (`02E3`–`0302`, `02EA` tolerated) before dying at
  `wait_ssm #0303`; a8/a9 aborted at the `02EA` preflight on the stale
  late-grab file (rc=1, not independent retries). **D0-26 a7 PASS with 110
  captures** (`02E3`–`0392`, `02EA` tolerated with the canonical message this
  time, all 82 fixed waits, CFG restored) — full-module coverage where phase 1
  had 8 pages. E module still lacks AmSpirit references.
  (Note: `results-tolerate.jsonl` will also carry spurious FAIL rows for
  B0-26/D0-26 — the script's second pass re-attempted the same out-dirs and
  the runner correctly refused to overwrite; the PASS rows + manifests are
  ground truth.)
- D1-27: determinism confirmed — pin-fixed attempts a4–a7 all stall at
  `wait_ssm 0x0305` with the same 30 captures (`02E3`–`0304`); D1-26 a5 PASS
  127 caps (same-mode rerun of the phase-1 PASS).
- Hardware oracle for the §5 candidates (SHAKER photographs / real CRTC).
- Harness diff items from Codex Part 1 (`codex-verify.md`) — **fixed
  2026-10-16, 8 new tests (116 green), fail-before proven by temporary revert**:
  1. over-tolerance — terminal classification is now by evidence: a remote
     file observed but never downloaded/verified in time, or a broken file
     check, raises `CaptureFailedError` (never tolerated); only a file never
     observed, or a valid PNG past its deadline, is a `CaptureTimeoutError`.
     Residual: transport errors that *raise* (vs bad exit codes) still retry
     to the deadline as timeouts — the transport does not type them;
  2. stale remote files — remote names carry the out-dir leaf (cell + attempt)
     while manifest/local names stay logical; best-effort `rm` on the failure
     path; failed entries record `remote_path`/`local_path` for salvage;
  3. stale SSM neighbour metadata — a tolerated failure clears
     `last_capture_record`, so a later paired success blames nobody stale.
  Committed as `2732443`.
- Runner recommendation (not implemented tonight): a run that ends with zero
  captures should say so in the manifest, so a C0-style false PASS cannot
  recur silently.

## 7. Restoration

Session end 2026-10-16 ~08:00 UTC, all device work complete, no runner
processes remain on host or device:

- MiSTer core: MENU (`/tmp/CORENAME` = `MENU`, loaded via Main's FIFO
  `load_core /media/fat/menu.rbf` by the fixed `phase2.sh` final step — the
  old `mbc-task-shaker load_core menu` form was invalid and never unloaded
  the core).
- CFG: `/media/fat/config/Amstrad.CFG` sha256
  `f8c6879a0e1fc3370de80ba3f04d30bea05cec5abc31c74db40e71c52867cf3e`,
  identical to the session baseline; verified by `ensure_cfg` after every
  cell.
- Screenshots: no `MISTER_0*` files left on the device (three tolerated-timeout
  late grabs were salvaged to `runs2/D0-27-full-a7/` and
  `runs2/D0-26-full-a7|a8/` with notes, then deleted).
- RBF/media on the device untouched (`Amstrad_20260929_cec641c.rbf`,
  `shaker27.dsk`, `shaker26.dsk`, `6128_FR.cpr` pins in §1).

Artifact index (all under `docs/specs/shaker-overnight-2026-10-15/` on the macmini, ignored
by git): `REPORT.md` (the original of this file), `RESUME.md` (run record),
`codex-verify.md` (independent review), `phase2.sh` / `b0d0-retry.sh` /
`c0-probe.sh` / `finalize.sh` / `make-review-composites.sh`,
`codex-phase2-brief.md`, `phase2.log`, `results.jsonl` /
`results-raw-crt.jsonl` / `results-raw-pixels.jsonl` /
`results-tolerate.jsonl`, `runs2/` (Full incl. tolerate reruns),
`runs-raw-crt/`, `runs-raw-pixels/`, `runs-diag/` (C0 probes),
`compare/` (finalize outputs + `review/` panels).
