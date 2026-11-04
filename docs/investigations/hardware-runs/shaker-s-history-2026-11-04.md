# SHAKER CRTC1 S: history-sensitive result — 4 November 2026

Test S (`02D3`) is not closed. The 599 full-history path prints 0640 and
e574 prints 5060, while direct entry on either artifact prints the current
program expectation 4E40. Keep the numerical failure; its cause is not established by the
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
S prints5060 WRONG versus the same4E40 expectation. Fresh direct S on that source gives 4E40 (10 commands, one capture, zero
failures, one SSM record). This records a different failing result under the same author path, not a
proven C5 mechanism or isolated publication-phase cause. [Exact artifacts and bounds](classic-targeted-milestones-2026-11-04.md).

The cached physical photograph CS_02D3_A.webp, SHA256
3738bf22d3a2e28c571705a10fec4262393176b781f5b038ff732bd93f9cb4c8,
appears to print4E30, rather than the current program's4E40. Parent inspected
the full original photograph; the precise last-but-one digit/version binding
still needs author confirmation. Keep the current program's explicit4E40
expectation and preserve the much larger0640/5060 failures. Do not claim the
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

## Qualified caller facts and next observation

Original BIN SHA256 3647e120…682f was byte-checked. Acquisition A067/A069
writes R8=3 then 0 using the inherited live R9; the S path programs R9=7 only
later at 5BE7. A prior source audit's steady-IVM timeline is rejected. Including
CALL/helper/branch overhead, equal retry probes are 19,969 nominal CPC µs
apart, giving +1 µs drift against a 19,968 µs frame. An extra retry does not
automatically determine later parity because the acquisition pulse and fresh
IVM request also affect it.

The loop computes 16*(64+N) modulo 65,536. Thus 0640 means N=36 modulo 4096, 5060 means N=1222, and0440 means N=4, not an observed exact frame-length timeline.
A passive first A4CF→successful A4DE timestamp pair gives 10+19969F µs for F
failed retries; first 5B0A→first 5B17 gives 4+16N µs, cross-checked with raw HL,
provided the API's instruction-clock semantics qualify. Current v2 SNA counter
fields remain unavailable; no zeros may be substituted.

## Matched original-caller AmSpirit discriminator

A qualified original-normal-boot direct/history pair now completes on AmSpirit
Lite1.17.1/core2491682, CPC6128/CRTC1A/128K/French. Both paths use the installed
author disk and unchanged BIN3647e120…682f. The history input is the canonical
142-line prefix through S, without splitting P/S into separate host jobs. Source,
code-bank, CALL operand, stack-return and instruction guards all pass. Root
inspected all ten live checkpoint images and the final02D3 image in each path.

| Observation | Direct | Canonical history |
|---|---:|---:|
|Failed acquisition retries|3|6|
|First A4CF→successful A4DE|59,917 µs|119,824 µs|
|A067→A06B, R8=3→0|8 µs|8 µs|
|Inherited R9 for that pulse|7|7|
|5B0A→5B17|19,012 µs|19,012 µs|
|Raw HL before doubling|04E4|04E4|
|Poll count / wrap|1,188 / none|1,188 / none|
|Computed and printed result|4E40|4E40|

All ten checkpoint CRTC register arrays match between paths. These measured
instruction-clock deltas satisfy10+19969F and4+16N respectively. Different
retry counts alone do not flip this software-reference result. The R8 pulse
and fresh IVM request make a simple inherited-parity argument insufficient.
Classic v2 SNA counter fields remain unavailable; they are not zeros or an
observed C0/C4/C9/parity tuple. Live screenshots do not supply that hidden state.

The positive-R5 C5 restart guard has not been observed in the native caller
and is not yet implicated. The next useful observation is passive live FPGA
state at the original fetch checkpoints. Existing SSM/SNAP wiring has most
counters but lacks parity/effective-IVM state and a lossless checkpoint queue;
a bounded default-off diagnostic is being prepared. No guest-state patch,
CPU pause, counter model change or altered printed expectation is authorized
by the matched pair. AmSpirit is left paused after the completed history S;
its temporary breakpoints/scripts are cleared.

Private evidence: `resumed-20261104/c1-s-caller-qualified/` and
`c1-s-original-capture/runs/run-direct-002/`, `runs/run-history-001/`, plus
`direct-history-comparison.json`. The initial capture helper had a constructor
keyword error before its first HTTP call; it was corrected and statically
bound before the accepted runs. It contributes no guest-state evidence.

A focused live [portal index](https://shaker.logonsystem.eu/api/tests) recheck
on4November finds67test entries and no moduleE. CE_B/02CA explicitly lists
only CRTC3/4; there is no listed CRTC1 reference for that second E page. This
qualifies the photo gap rather than treating a missing cache file as evidence.
Private response: resumed-20261104/reference-index-recheck/.

## Exact9a comparison and transcription correction

The same targeted scripts on exact9a91920 complete direct10commands/onePNG/
zero failures/oneSSM and canonicalhistory131commands/64PNGs/zero failures/
121SSM. Direct remains4E40; history prints0440 WRONG. Root viewed the final
scalar. C1 functional RTL is unchanged frome574, so a different scalar alone
does not isolate a source-code cause; entry phase remains a necessary control.

Independent immutable-label glyph matching corrects the earlier e574 reading:
the preserved PNG says5060, not9600. Its derived poll count is1222mod4096,
not2336. The599 control0640 and both direct4E40 readings are confirmed. Active
reports and unsent question are corrected; original PNGs/logs are unchanged.
Ledgers/panel: s-passive-fpga-trace/{historical-s-glyph-ledger.json,
baseline-s-glyph-ledger.json,baseline-s-words.png}. This correction changes no
pass/fail verdict and supplies no hidden counter trajectory.

## Passive native state now qualified

The isolatedae9ccdc observer retains exact9a direct4E40/history0440 and clean
full timing. Both complete seven ordered cuts with no errors/drops and stable
raw reads. History already enters acquisition with adjustment active/C5=31;
C5=24 remains at the original R5-write fetch, while direct has adjustment off/
C5=0. At R7, direct C4=4 versus history C4=2 with an added line active.
This supplies the missing native state discriminator. It does not authorize
clearing inherited state to fit the final word. See the [observer qualification
and cut table](shaker-s-passive-observer-2026-11-04.md) for exact artifact,
source guards, timestamps and passivity limits. The next source investigation
checks interlace counting during adjustment against French§11.8p93 while
preserving E0080/P26 and the zero-R5 author contract.
