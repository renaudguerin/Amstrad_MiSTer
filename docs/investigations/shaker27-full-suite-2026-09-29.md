# SHAKER 2.7 suite execution, 2026-09-29

## Scope and immutable inputs

The author’s private 2.7 CSL corpus is under `local/test_media/shaker/CSL_27/`,
with space-containing `MODULE A` through `MODULE E` directories. The unchanged
2.6 fallback scripts are under `CSL_26/MODULE_A` through `MODULE_E`. Both
corpora contain 25 scripts. Original author files were neither edited nor
committed. The expected roughly 30% speedup is an author estimate, not a result
of this run; no duplicate baseline was requested solely to measure it.

The actual bitstream is `Amstrad_20260929_cec641c.rbf`, SHA-256
`40f7d66bfcfda37ce9f5053ff3e3af833713bdb4065fe4bcde9c311a3cb4dd2c`.
Task starting base is `d188607`; subsequent documentation and simulation-only
integration does not change that bitstream identity. SHAKER disks are pinned:

| Disk | SHA-256 |
|---|---|
| `shaker27.dsk` | `65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b` |
| `shaker26.dsk` | `f7082f8eab521d632c343a288f54038af6df090c59b372e0d2866269c2cc4d08` |

Original device state was MENU. An independent 16-byte CFG backup has SHA-256
`13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`.
The temporary classic base clears Plus bits 34:33 and CPU-fast bit 6, preserving
all unrelated bits; its hash is
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
The reused pinned MBC binary has hash
`0e99082bb8c9b8b2c2b6581c736dfc5d701a977a9c76fa548565cbc6642984db`.

## Matrix and execution contract

Every supplied script was dry-run audited. With `--no-follow-loads`, all twenty
classic 0/1 entry scripts across both versions pass validation. Default chaining
reaches unsupported CRTC2, so each module/type is run separately. CRTC2/4 are
unavailable and never substituted with classic0/1. The original runner also
rejects3 even though the core has a separate Plus ASIC CRTC3 implementation.

Runs use French keyboard translation, Full sync mode once per cell, SSM capture,
coherent observer startup, a 120-second per-wait bound, and disk/RBF hash pins.
A failed new-script cell triggers its corresponding 2.6 script and 2.6 disk;
fallback completion is execution coverage, not proof the 2.7 script passed.
Native Main captures are asynchronous and cannot establish exact marker-time
pixels or physical connector fidelity. Script completion and visual/numeric
acceptance are separate judgments.

## Transport interruption

The first 2.7 A0 attempt ran for 728.5 seconds and retained 107 PNG captures.
Its last completed command was `SHAKER-A-0.CSL` line 248, `wait_ssm 0x0097`,
released by a previously observed unconsumed matching record. SSH then failed
with `Could not resolve hostname mister`. The remaining batch launches,
including the matching 2.6 fallback, failed without useful execution. These
are transport failures, not established script typos or module failures.

That attempt’s cleanup could not verify CFG restoration because DNS remained
unavailable. Both `mister` and `mister.local` failed subsequent bounded checks.
The original CFG remained locally backed up. Restoration was subsequently
verified before the resumed session below.

Local evidence is retained under `docs/specs/shaker27-suite-2026-09-29/` with a
directory-local ignore file: corpus hashes and dry-run results, original and
classic CFGs, the batch command/timing ledger, per-attempt logs, manifests and
partial screenshots. Failed attempts remain distinct from any later retries.

## Partial A0 screen observations

Gemini inspected all 107 retained images; the main task checked the decisive
`MISTER_0_0091.png` and `0097` directly. Images are 768×273. The capture set
contains 78 distinct PNG hashes; 55 images belong to 26 duplicate groups.
Identical asynchronous captures do not prove the underlying video was static,
and the analysis’s proposed freeze/VRAM mechanisms are not accepted findings.
No original-CPC photograph comparison has been performed in this session.

The `0091` “CRTC 0 LOST & DEAD VSYNC” page shows VSYNC length `#38` in two
blocks whose printed expectation is `#68`; PPI.B0 is `#FF` in checks displaying
`Exp xE`. Other checks on that page match their printed values. These are
screen-level discrepancies requiring source/hardware follow-up, not grounds
for speculative RTL changes. The retained Gemini transcription reads `#6B`
for that expectation; direct image inspection corrects it to `#68`.

`0037` (R0 timing) displays `KO` at HCC `3E` and `3F`, and `00C3` includes
`xKOx` labels. Such labels depend on CRTC applicability (one explicitly says
“OK FOR CRT 3+4 ONLY”), so they are observations, not blanket failure verdicts
for this CRTC0 run. `0096`/`0097` contain speckled bands outside the main text
area and edge wrapping; their cause is unverified. The complete analysis log
is retained with the local evidence, including its unverified interpretations.

## Offline CRTC3 support and acceptance

The runner now supports explicit CRTC3 by selecting 6128+ (status34:33=2),
leaving classic bit2 uninterpreted, and loading an explicitly supplied,
SHA-pinned system CPR before the disk. The user selected `6128_FR.cpr`, which
boots directly into BASIC with the French keyboard layout. Its local hash is
`ab241580c9b6a9fa9aeae94ca6847ea70dca386d305e38fc2ac03fce603360cf`.
The `cec641c` bitstream includes the D5 BASIC mapping repair `f0af3d6`.
The resumed module A run below demonstrates the French-cartridge/disk startup
and CRTC3 script execution on that bitstream.

Explicit classic selection clears the Plus model bits. Classic `cpc_model`
state persists independently across Plus power-ons. Focused fail-first tests
exposed both the missing CRTC3 path and the Plus→classic configuration/model
round-trip hazards, then passed after their fixes. All ten CRTC3 entry scripts
across 2.6/2.7 pass complete isolated dry-runs with the pinned cartridge options.
CRTC2/4 remain rejected. Existing private-corpus tests now use the moved paths,
so a provisioned checkout executes them instead of silently skipping them.

Cross-provider review: Opus5.5 medium runs `20260929T051814Z-45787-5606`
and `20260929T052448Z-47797-21fa`; the second approves the corrected diff.
Review did not use the device or rerun the gate. Logs are retained in local
`opus-review-1.log` and `opus-review-2.log`. The pre-existing case-sensitive
programmatic SHA pin comparison remains a minor API limitation; CLI pins are
normalized. Scripts omitting `crtc_select` retain their original CFG model.

Final checks:
- `python3 -m unittest discover -s scripts/hardware-loop -p 'test_*.py'`:
  205 tests, OK, one skip. The first complete run exposed three exact CFG-mask
  expectations that needed to include the intentional clearing of bits33/34;
  those assertions were updated, retaining exact equality.
- `python3 sim/select_tests.py --run`: `select_tests: no simulation needed`.
- `git diff --check`: clean.

## Resumed module A and user-directed cutoff

After the user restored connectivity and rebooted MiSTer, the device reported
MENU with CFG hash `4d68df48…`. Before any new test load, the original CFG was
copied back and its complete `13ef32c7…` hash verified. MBC was reprovisioned
and hash-checked; RBF, both disks and the French cartridge matched their pins.
The clean task checkout advanced to integrated `a79a5a1`; no host-code edits
were made during this resumed run.

The user then requested finishing module A and stopping for the day to conserve
quota. The full-matrix supervisor was paused while A1 continued, then removed
after that child completed. A separate A-only continuation ran A3. Modules B–E
were not started in the resumed session. The earlier DNS-blocked launch attempts
remain transport evidence, not execution coverage.

| CSL27 cell | Result | Elapsed seconds | PNG captures | SSM records | CSL26 fallback |
|---|---|---:|---:|---:|---|
| A / CRTC0 | Complete | 807.720 | 122 | 237 | Not needed |
| A / CRTC1 | Complete | 828.406 | 127 | 240 | Not needed |
| A / CRTC3, 6128+ | Complete | 809.671 | 123 | 238 | Not needed |

A0/A3 times measure the runner subprocess wall time. A1 uses manifest start
through completion of the last cleanup command because its batch supervisor
was paused; it is not an independently measured subprocess duration. Together
these spans total about 40m46s. They include SSH, input and native screenshot
overhead and do not measure the author’s estimated speedup.

All three runs observed a zero-written SSM startup header in one attempt. Each
completed without a runner error and verified CFG restoration to the original
hash. The A3 first test page visibly identifies CRTC3, demonstrating the pinned
`6128_FR.cpr` + disk boot with French input. Its author script ends with a reset;
that second load is retained in its manifest. A0/A1 terminal `csl_load` commands
are recorded but not followed under `--no-follow-loads`.

The successful A0 run reproduces the earlier `0037` and `0091` PNGs byte-for-byte,
including the displayed VSYNC/PPI discrepancies described above. Successful
script execution does not make those observations pass. The 372 resumed PNGs
are preserved; exhaustive new image/reference comparison was deferred at the
user’s wrap-up request. No new RTL finding or original-CPC verdict is claimed.

## Final restoration, artifacts and handoff

After A3, an independent copy of the original CFG was applied and SHA-256
verified as `13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`.
`/media/fat/menu.rbf` was loaded and `CORENAME=MENU` read back. Task MBC
`/tmp/mbc-task5` was removed, along with the exact manifest-owned MGL/CFG temp
paths, including leftovers from the DNS-interrupted attempt. Final checks
returned exit0. The device slot was explicitly released to the coordinator.

The evidence directory’s `resumed/` contains each completed run, command logs,
`module-a-results.json`, recovery preflight and `restoration.log`. The original
failed attempt, original CFG backup, corpus hashes/dry-runs and offline gate/
review logs remain alongside them. The refreshed archive is
`shaker27-suite-2026-09-29-module-a.tar.gz`; its delivered hash is reported in
the coordinator handoff. Private author files remain unchanged and uncommitted.

This continuation changes documentation only; simulation and code review are
not rerun. The integrated host runner’s prior tests/review remain applicable.
The **full suite remains incomplete by user direction**: modules B–E for each
supported CRTC0/1/3 remain to run, with corresponding CSL26 fallback for module
execution problems (especially D), and separate visual/numeric comparison.
CRTC2/4 remain unavailable and must not be substituted. Resume with fresh output
directories, the same pinned artifacts, and a new independent CFG snapshot.
