# shaker-ref

Reference-screenshot tooling for SHAKER comparison. Complements the live
oracles: [`scripts/amspirit/`](../amspirit/README.md) (local emulator runs)
and the `mister-capture` skill (device captures).

## scrape_logon.py

Downloads Logon System SHAKER reference screenshots for later comparison:

```sh
python3 scripts/shaker-ref/scrape_logon.py --arch amspirit_lite-1.15 \
  --crtc 0 --crtc 1 --crtc 3 --out-dir <dir>
```

- Source: `https://shaker.logonsystem.eu` (`/api/archs`, `/api/tests`,
  `/images/...webp`). Stdlib only, resumable (skips existing files).
- Layout: `<out>/<arch-key>/CRTC<c>/<test>_<hex>[_<subfolder>]_<sub>.webp`,
  plus `index.json` with test name, SSM hex code (the join key to
  `MISTER_<n>_<hex>.png` in hardware-loop manifests), source URL, sha256
  and attribution. Other `--arch/--crtc/--module/--test` filters available.
- Gotcha: the site returns HTTP 200 HTML for missing images, so payloads
  are magic-checked (`RIFF…WEBP`); misses are recorded in
  `index.json:missing`, not saved.
- Reference screenshots are (c) their authors; fetched for comparison only,
  never committed. Emulator agreement is diagnostic evidence; hardware
  photographs and the ACCC remain the authority (see `docs/backlog.md` B2).

## Local pull (2026-10-15)

`docs/specs/shaker-ref-logon/` (ignored): `amspirit_lite-1.15`, CRTC 0/1/3,
all 67 tests — 1296 images, 37 MB. 54 misses, all CRTC3 (DU/D8/DH, one A2):
the site has no AmSpirit shots there, so those subtests have no emulator
reference to diff against.

## compare.py

Diffs MiSTer captures against the pull, joined on `(SSM hex, CRTC)`:

```sh
python3 scripts/shaker-ref/compare.py --ref-index docs/specs/shaker-ref-logon/index.json \
  --mister <runs-dir> [--mister ...] --out-dir <out>
```

- Output: `results.json` per-pair metrics plus `diff_<module><crtc>_<hex>.png`
  composites (MiSTer | reference downscaled | amplified diff) for differing pairs.
- Geometry caveat: MiSTer captures are 768x273 async scaler frames, references
  768x542 settled emulator shots; the reference is LANCZOS-downscaled before
  diffing. Metrics are triage signals. Same-screen pairs (e.g. text result
  pages) compare well; pattern screens often mismatch on capture phase
  (instruction page vs pattern), which is a phase difference, not a CRTC verdict.
- First run (2026-10-15 snapshot, `docs/specs/shaker-compare-2026-10-15/`):
  673 pairs, 664 differ, 9 no-reference. Notable: B1 `01F4` shows MiSTer CRTC1
  `CALC=#34` against AmSpirit CRTC1 `CALC=#FF` (same screen phase, isolated
  text-row diff); C1 `02D3` agrees (`#4E40` both) modulo rendering.
- Full-set rerun (same dir, 1944 pairs over runs2 + Sept-30 captures):
  1803 differ, 141 no-reference. Codex 6.1 Sol triage
  (`codex-triage/triage.md`, +`inspection-ledger.json`,
  `composite-provenance.json`) reviewed all 292 low-diff records plus 89
  above-cutoff samples: top follow-ups all CRTC1 — BR/E `01F4`, DE/A `03AE`
  (spot-confirmed: WRONG vs OK on identical setup), B6/A `019D`, AI/A `008D`,
  DR/A `02EA`; modules A–D partial, E none (no references). Triage is
  provisional evidence collection, not a hardware verdict.

## compare_runs.py

Diffs two MiSTer capture trees against each other (same cells, joined on
`(cell, suite version, CRTC, SSM hex)`; one attempt dir per cell+version,
preferring a success manifest):

```sh
python3 scripts/shaker-ref/compare_runs.py --a <dir A> --b <dir B> \
  --a-label full --b-label raw-crt --out-dir <out>
```

- Output: `results.json` plus `runpair_<cell>-<ver>_<crtc>_<hex>.png`
  composites (left A, middle B, right amplified diff).

## analyze_runpair.py

Classifies those pairs: flat (scaler lock loss), shift-only, or
content-change, by searching the integer (dx, dy) that best aligns B onto A:
`flat` = one side is a near-uniform field; `shift-only` = aligned residual
below threshold; `content-change` = the visible content really differs.

```sh
python3 scripts/shaker-ref/analyze_runpair.py --a <dir A> --b <dir B> \
  --out <analysis.json>
```

- Sync-filter modes change the acquisition window (see
  `docs/investigations/hardware-runs/capture-reliability-2026-09-22.md`), so
  raw pixel diffs across modes are dominated by global offsets; the aligned
  residual is the content signal. Phase noise (async grabs of animated
  screens) still shows up as content-change for scrolling/flashing tests —
  same-mode reruns are the control.

## compare_modes.py

Paired mode-vs-reference comparison: joins two `compare.py` results files on
`(cell, CRTC, hex)`, keeps the best (minimum) mean per side, and reports
per-cell distributions of `B - A` (negative = B closer to the AmSpirit
reference):

```sh
python3 scripts/shaker-ref/compare_modes.py --a <results A> --b <results B> \
  --a-tag runs2/ --b-tag runs-raw-crt/ --out <modes.json>
```

## Operational gotchas

- Do not edit `phase2.sh` while a run is in flight: bash reads scripts
  incrementally, and an edit shifts the byte offsets of the remainder (one
  queue's tail — final CFG check and MENU reload — was skipped this way).
- `PASS` in `results*.jsonl` means the runner exited 0, not that captures
  exist. C0 completed with 0 captures and 0 SSM records in both modes and was
  still logged PASS; check `captures`/`ssm_records` in the manifest when
  integrating results.
- The 2.7 suite is not uniformly on the 27 disk: `SHAKER-D-1.CSL` inserts
  `shaker26.dsk`. Derive media pins from each script's `disk_insert` line.
- Same-mode reruns are the capture-phase control: B0 captures repeat
  pixel-identically, while animated tests (e.g. B1-27) differ between
  attempts with mean abs diff up to ~112, so cross-mode pixel diffs on
  animated screens are noise-dominated.
- A page whose test breaks video sync starves the scaler, so Main's grab times
  out (B0 `00FB`, D0 `02EA`, the C0 C5 page) and the runner aborts the whole
  cell unless `--tolerate-capture-timeout` is given: the flag records the
  failure in `capture_failures` (and a `failed` entry in `captures`) and
  continues, so one frameless page does not cost the rest of the module. The
  tolerated condition is the `CaptureTimeoutError` *type*: when the deadline
  expires during the final `test -f` poll, the recorded text is the
  transport's `SSH command timed out after Ns`, so classifying on the
  canonical `Capture … timed out after Ns` text silently misses the flag's
  own case (B0-27 attempts a4–a6, 2026-10-16). Output the grab produced but
  the pipeline could never use (failed download, invalid PNG, broken file
  check) raises `CaptureFailedError` instead and is never tolerated.
- On CRTC0 a sync-breaking test costs more than the page: after B0 `00FB` the
  machine stops emitting SSM markers altogether, so the walk dies at the next
  `wait_ssm` (line 18 `00FC`) even with the capture failure tolerated. Marker
  waits are deliberately not tolerated — a missing marker can mean a wedged
  machine. 2.7 B0/D0 therefore cannot complete on CRTC0; the 2.6 scripts use
  fixed waits and walk the whole module.
- Raw pixels keeps Full's acquisition tuple and geometry: over the A cells 280
  of 372 pairs have aligned-mean zero and 37 more differ by ≤3/255, while all
  44 large pairs are sync/register-update tests (A3 CRTC RVMB, R13 UPDATE, R1/R6
  STORIES) where the grab is a race. It is the control that separates the
  byte/pixel-mask effect from raw-crt's window/sync path.


