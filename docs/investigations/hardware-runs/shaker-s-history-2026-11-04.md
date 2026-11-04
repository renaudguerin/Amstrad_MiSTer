# SHAKER CRTC1 S: history-sensitive result — 4 November 2026

Test S (`02D3`) is not closed. The latest full C1 walk prints0640 instead of
the current program expectation4E40, while direct entry on the same RBF
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

## Current targeted milestones and photo qualification

On599ca80, the exact canonical prefix through S completes131commands,
64captures and121SSM records without capture errors: S remains0640, while
fresh direct S remains4E40. E now prints0080 in both paths and P remains26.
On e574a46, whose additional change shifts Type1 published VSYNC by one
Pixel-M2 without changing logical counters, the same prefix completes but
S prints9600 WRONG versus the same4E40 expectation. Direct S on that source
is pending. This isolates sensitivity to the changed PPI-visible phase/entry
path, not a proven C5 mechanism. [Exact artifacts and bounds](classic-targeted-milestones-2026-11-04.md).

The cached physical photograph CS_02D3_A.webp, SHA256
3738bf22d3a2e28c571705a10fec4262393176b781f5b038ff732bd93f9cb4c8,
appears to print4E30, rather than the current program's4E40. Parent inspected
the full original photograph; the precise last-but-one digit/version binding
still needs author confirmation. Keep the current program's explicit4E40
expectation and preserve the much larger0640/9600 failures. Do not claim the
CPCSE4E40 image exactly matches that physical photograph, or silently broaden
an assertion to mask the difference. A16us measurement quantum, variant or
program revision is an open question, not an established explanation.

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
