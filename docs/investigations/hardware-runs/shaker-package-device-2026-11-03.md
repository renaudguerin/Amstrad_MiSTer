# Updated SHAKER package on MiSTer — 3 November 2026

## Setup

Renaud explicitly released MiSTer after his manual session. The parent is the
sole device operator. These runs test the [updated package](shaker-package-audit-2026-11-03.md)
on existing RTL, not the separate unbuilt E/P display repair.

- RBF: `/media/fat/_Computer/Amstrad_20261103_5fba168.rbf`, SHA-256
  `b5511c228adcf35f2cbbb78dec3e90c37d71e480ce471732a1fe4584977de43e`.
- New canonical disk: `/media/fat/games/Amstrad/dsk/shaker27.dsk`, SHA-256
  `6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
  Former disk retained beside it as `shaker27-pre-CSLSSM-20261103.dsk`
  (old SHA-256 `65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b`).
- Classic CRTC1, Plus off, French keyboard/ROM selection, requested Full
  sync filter. The script applies CFG; active filter/mode latch is unobserved.
  `boot_fr.rom` was rehashed as
  `1ba46e0f28388dd0d45b759f92468ffdccae8d40b973af736d7cc7ed0739eb4a`.
- Repaired MBC copied to `/tmp/mbc-overnight-20261103`, SHA-256
  `f2b3a4f3cbd98c7a13f8770eb4c893bc352eae9d204f92f664f5eaad6c697330`;
  bounded release-only smoke returned zero. The binary remains temporary.
- SSM enabled, `--no-follow-loads --max-wait 120 --tolerate-capture-timeout`.

All cases and complete manifests/captures/logs are ignored under
`local/overnight-20261103/package/` in the task checkout.

## E-1 result

Canonical `CSL_27/MODULE E/SHAKER-E-1.CSL` completed:

```text
status=success commands=25 captures=7 capture_failures=0 ssm_records=16
```

Evidence directory: `e1-device-5fba168/`, log `e1-device-5fba168.log`.
All seven PNGs were inspected through a contact sheet and the two warning
pages additionally compared with the old originals.

| Pages | Comparison with prior 3 November E-1 run |
|---|---|
| `03FA`–`03FD`, `0400` | All five PNGs byte-identical to the prior disk/run. |
| `0401` | Warning text remains visible: the old capture also shows it. New geometry is 768×250 versus old 768×273; the new crop loses the header. |
| `0402` | New capture is blank, 768×266. The old 768×273 capture shows warning text. Capture phase/geometry and test behavior are not separated by this run. |

Thus the new E-1 script executes and yields seven captures, but this is **not
a seven-page correctness pass**. The warning/blank behavior predates the
updated disk; neither improved correctness nor a new disk regression is
established for `0401`/`0402`. The changed module-E binary still merits
expectation review on each supported CRTC in a broader acceptance matrix.

The E-1 manifest records CFG restoration matching its original:
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
This is configuration-file evidence, not proof of an active runtime latch.

## D-1 completed walk and photo comparison

The new fixed-delay canonical D-1 completes on the same pinned RBF/disk:

```text
status=success commands=200 captures=127 capture_failures=0 ssm_records=186
```

Evidence directory `d1-device-5fba168/`. It progresses beyond both earlier
keyboard stops (`0305` and `0311`). This is the first complete D-1 walk in
this investigation, now using SHAKE27D rather than the old script's SHAKE26D.
CFG restoration matches the same original hash recorded for E-1.

`compare.py` joined all 127 captures to the cached Logon System real-CPC
photographs. Two Gemini visual batches provisionally classified 71 matches,
51 phase/acquisition cases and five content differences. Parent inspection
reclassified `0305` (the menu captured after leaving the timing test) as
acquisition, giving **71 matches, 52 phase/acquisition cases, four visible
content differences**. These are image classifications, not 127 hardware-rule
verdicts. Reports, pair records and composites: `d1-vs-cpc/`.

| Visible residual | Observation; no causal RTL claim |
|---|---|
| `02E3` (DI/A) | DEC DE interrupt timing prints `#58` instead of the real-CPC photograph's `#59` for a classic machine. |
| `02F8`, `02FA` (DT/G2/H2) | The photographs show an additional black VSYNC region absent from these native captures. A signal/phase discriminator is still needed before assigning a GA cause. |
| `03B6` (DE/E) | One LINE A appears where the photograph shows A/A/B. Printed `FF` versus `5F` is the separately known PPI input difference and does not explain the missing text rows. |

All four were spot-checked by the parent against the originals/crops. D9 G/H
show the expected single copies, and `03AE`/`03AF`/`03B4`/`03B5` compare
correctly after the full walk's history. The parent additionally inspected
the OK text at `03AE` and `03B4`; those are no longer only isolated-entry
observations.

**D3 is not valid page-by-page evidence from this full walk.** All 44 markers
`0314`–`033F` were observed, but the fixed-delay script sends five bulk runs
of 50 spaces. Host-native PNG retrievals fall behind those transitions, so
many filenames contain later parameter states or the menu. The remaining
eight phase/acquisition cases comprise `02EE`/`02EF`, `0303`–`0305` and three
flashing D9 pages. Do not report these 52 cases to the authors as core defects.

The first focused paced D3 run completed `100 commands, 44 captures,
0 capture failures, 49 SSM records`, but every captured parameter state was
one increment ahead, with the final capture at the menu. Removing the
initial space and waiting for 0314 then timed out before any capture:
**the marker is emitted on page advance, rather than stable initial entry**.
The first key is therefore not proven to be an author-script error; native
host retrieval sees the state after the marker-producing transition.

A manual overlay instead captures the stable page before sending each key
sequence, with SSM autocapture disabled. Its first 44-capture run completed
but 70 ms press/release timing lost spaces: 0314 is correctly 35/06, 0315 is 39
instead of 3A, and later pages accumulate offsets. This run also cannot
support page-by-page core verdicts. A short three-page probe with symmetric
200 ms press/release instead repeated
keys (35→3C→3E), while 100 ms reached the expected 35→3A→3B. The full
100 ms overlay walk is now running; acceptance still requires all 44 headers.
The MBC adapter uses the larger of requested press and release delays for
both, so asymmetric CSL values do not provide independently controlled phases.

The provisional Gemini `d3-paced-vs-cpc/visual-triage.md` is not accepted as
an RTL-fidelity verdict; its module-E media identification is also wrong.
The adjacent `parent-acceptance.md` records those limits. Canonical author
scripts remain unchanged. These host-specific capture/input limitations
must be resolved before recommending an overlay or reporting D3 failures.
