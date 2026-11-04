# SHAKER CRTC1 S: history-sensitive result — 4 November 2026

Test S (`02D3`) is not closed. The latest full C1 walk prints0640 instead of
the original-machine expectation4E40, while direct entry on the same RBF
prints4E40. Keep the numerical failure; its cause is not established by the
changed screenshot alone.

## Exact observations

| Source | Entry | Result |
|---|---|---|
| `8dde1e7` | Canonical full C1 history | 4E40 |
| `bedb1c5` | Canonical full C1 history | 4E40 |
| `db4be80` | Canonical full C1 history | 0640 WRONG, expected4E40 |
| `db4be80` | Fresh SHAKE27C boot, directly S | 4E40 |
| `8dde1e7` | Same fresh-boot/direct-S script | 4E40 |

Both direct-entry PNGs are byte-identical to the earlier passing full-history
image, SHA256
`b03c5e572b9d72e2a0cfb56b5c687f2e089b52d5cf38150f97fcee2dd50f4e2d`.
Each direct control completes10commands/one capture/no failures/one SSM record.
The input script copies the canonical boot prefix and S command; preceding
tests are explicitly absent. It is a history discriminator, not a replacement
for the failed full walk.

The full `db4be80` walk completes165commands/80captures/no capture failures/
152SSM records. All152 report requested/applied Full0. Its72 unconsumed0000
separators match the previous run. Media is the installed author package,
SHA256 `6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
The [classic milestone](classic-blank-alignment-2026-11-04.md) pins the RBF.

## Corpus comparison and interpretation

Against the prior80-page `8dde1e7` walk,66 PNGs are byte- and pixel-identical,
including E02C9/02CA and P02CB. E's known0060 versus0080 residual remains;
P still prints26. Thirteen other changed images contain row/raster patterns;
several exactly match alternate phases already captured previously. Their
remaining causes are unassigned without a same-RBF phase control. Parent
inspected the complete contact sheets and the enlarged numerical S comparison.

The provisional Gemini artifact report correctly reads0640, but overstates
causality when it attributes that value directly to the new blank/JIT RTL.
Its description of `bedb1c5` as pre-E/P is also wrong: that source includes
the E/P repair. The original report is preserved with a parent assessment.

S was already qualified as history/entry-sensitive in the
[E/P investigation](shaker-ep-display-2026-11-03.md). The new direct controls
show that the new RBF can reproduce the prior exact result; they do not rule
out a change in which full-history entry conditions fail. This is a computed
numerical difference, not merely an asynchronously sampled field image.
Do not claim that S is fixed, dismiss0640 as screenshot jitter, or alter C5
to fit that sample without a faithful counter/entry discriminator.

Evidence is under ignored `local/overnight-20261103/milestone-next/`:
`c1-device-db4be80/`, `c1-comparison/`, `s-only.CSL`, `s-only-db4be80/` and
`s-only-8dde1e7/`. The source/rule questions in the unsent C5 follow-up remain
open; this result does not choose an adjustment-counter lifetime model.
