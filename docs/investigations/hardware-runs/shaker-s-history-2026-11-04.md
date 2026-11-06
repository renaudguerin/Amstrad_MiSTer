# SHAKER CRTC1 S: history-sensitive result — 4 November 2026

> Current correction,6November: this report preserves the named4November cohorts.
> Later fdf history fails0840/0440; the physical S word is reread4E40,
> superseding historical4E30 mentions below. Use the
> [current S report](shaker-s-current-2026-11-06.md) for the live position.

The current canonical S residual has two native passing fits: default production
build `076eab1` (macro OFF) and parent-verified diagnostic build `c16e7ec` (macro ON),
both incorporating the C1 interlace adjustment counting correction `ccf2be7`.
Test S (`02D3`) prints the current program expectation 4E40 in both fresh direct
entry and the full canonical history walk. Direct and history full RGB captures
are identical to each other and match the qualified `9a91920` direct 4E40 baseline
image (SHA256 `b03c5e572b9d72e2a0cfb56b5c687f2e089b52d5cf38150f97fcee2dd50f4e2d`).
Pre-fix builds exhibited history-sensitive numerical failures (0640 on
`db4be80`/`599ca80`, 5060 on `e574a46`, and 0440 on `9a91920`/`ae9ccdc`), which
are preserved below as historical evidence. Comprehensive lifetime policy,
phase robustness, and physical photo compliance (CS_02D3_A.webp photo 4E30 vs
current program 4E40) are not claimed closed; scope is a targeted history walk,
not a broad baseline rejudgment.

## Exact observations

| Source | Entry | Result |
|---|---|---|
| `076eab1` | Canonical history through S | 4E40 (matches direct baseline) |
| `076eab1` | Fresh SHAKE27C boot, directly S | 4E40 |
| `c16e7ec` (diagnostic) | Canonical history through S | 4E40 (matches direct baseline) |
| `c16e7ec` (diagnostic) | Fresh SHAKE27C boot, directly S | 4E40 |
| `8dde1e7` | Canonical full C1 history | 4E40 |
| `bedb1c5` | Canonical full C1 history | 4E40 |
| `db4be80` | Canonical full C1 history | 0640 WRONG (pre-fix), expected 4E40 |
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
[E/P investigation](shaker-ep-display-2026-11-03.md). Prior to the adjustment
counting fix, direct controls showed that those RBFs could reproduce the prior
exact result while full-history entry failed. That was a computed numerical
difference, not merely an asynchronously sampled field image. Historical
failures (0640, 5060, 0440) were retained rather than dismissed as screenshot
jitter or fit by ad-hoc scalar alterations.

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
source guards, timestamps and passivity limits. The subsequent source investigation
resolved interlace counting during adjustment against French §11.8 p93 while
preserving E0080/P26 and the zero-R5 author contract; see
[CRTC1 interlace counting during adjustment](crtc1-interlace-adjustment-counting-2026-11-04.md).

## Bounded native resolution on 076eab1

Full build `076eab1` (functional change `ccf2be7` in `rtl/crtc_type1_engine.v`)
was verified on hardware following clean full Quartus synthesis:

- **Build verification**: Source `076eab11a948e91e44279f5ef128c924a26b92ea`, CI
  workflow 41925025171 (all required jobs pass, artifact 13370705502, build 368).
  Bitstream `Amstrad_20261104_076eab1.rbf` (3,139,692 bytes, SHA256
  `7d7fc3f6f1cc2ea6197a05cfc273ca4bc308e7e6a2b14bfc3774ffac1b6cb4b6`). Built with
  clean_full Quartus 17.0.2.602 Standard Fit / High Performance Effort: 7 setup and
  7 hold clock groups, TNS 0, min slack +0.539 ns setup / +0.242 ns hold, zero errors,
  zero critical warnings, observer macro OFF. Device bitstream matches installed hash.
- **Native execution**:
  - Direct S: 10 commands, 1 capture, 0 failures, 1 SSM record, prints 4E40.
  - Canonical history through S: 131 commands, 64 captures, 0 failures, 121 SSM
    records, prints 4E40.
  - Direct S and history S full RGB captures are identical to each other and
    identical to the qualified `9a91920` 4E40 direct baseline (SHA256
    `b03c5e572b9d72e2a0cfb56b5c687f2e089b52d5cf38150f97fcee2dd50f4e2d`).
  - Preceding checkpoints E (`02C9`/`02CA`, 0080) and P (`02CB`, 26) full RGB
    are identical to baseline 9a; root inspected full S, E, and P images.
- **History comparison and limits**:
  - Across all 64 captures from the canonical history prefix against the exact 9a
    baseline, 53 images are RGB identical and 11 changed (including S). The other
    10 changed images contain raster patterns. Parent inspected all64 contact views
    and all11 enlarged paired panels; no physical-photo verdict is refreshed.
  - Regression controls: C1 DSC4 completed 4 views root-inspected (clear colons
    and names, visible otter and rotated fire effects); Amazing completed 2 views
    (visible logo, text, and QR code, showing bounded regression only).
  - In production build `076eab1`, the observer macro was OFF; the earlier
    native C5=24 reading belongs strictly to pre-fix traces. Live hidden state was
    subsequently qualified on the dedicated macro-enabled diagnostic build `c16e7ec`
    detailed below.
  - Physical photograph CS_02D3_A.webp (4E30) versus current program (4E40)
    revision binding remains open. Scope is a targeted history walk, not a broad
    baseline or physical photo rejudgment. Neither comprehensive lifetime policy,
    phase robustness, nor photo compliance is claimed closed.

## Diagnostic native observation on c16e7ec

Parent-verified diagnostic branch `codex/s-passive-ivm-native-20261104` at commit
`c16e7eca6c574410d0f5d2b2cfee9bd30c19d898` is exact `076eab11a948e91e44279f5ef128c924a26b92ea`
plus ONE QSF enable `SSM_S_PASSIVE_TRACE=1`. Full workflow 41933969717 passed all
required jobs (artifact 13375070713, build 372). Bitstream `Amstrad_20261104_c16e7ec.rbf`,
3,169,908 bytes, SHA256 `e6e6b848fff15d3e37c1bf5948aeb1c36654b856409955eb7d8ee01201f75f5e`.
Clean_full Quartus 17.0.2.602 Standard Fit / High Performance Effort: 7 setup and
7 hold clock groups, TNS 0, min slack +0.398 ns setup / +0.246 ns hold, zero errors,
zero critical warnings, installed hash matches on target device. Production observer
macro OFF remains unchanged.

Native execution:
- Original direct S: 10 commands, 1 capture, 0 failures, 16 SSM records.
- Canonical history through S: 131 commands, 64 captures, 0 failures, 136 SSM records.
- BOTH direct S and canonical history S full RGB captures yield exact SHA256
  `b03c5e572b9d72e2a0cfb56b5c687f2e089b52d5cf38150f97fcee2dd50f4e2d`, printing
  the program expectation 4E40 identically to default `076eab1`.
- Preceding checkpoints E (`02C9`/`02CA`, 0080) and P (`02CB`, 26) full RGB are
  identical to default `076eab1` and baseline 9a.
- Two stable 1040-byte ring reads per path, 7 ordered valid cuts, error 0, dropped 0,
  all sequence numbers match header. Direct written 16 / history written 136.
- Timings: Acquisition direct 39,948 µs (2 failed retries); history 10 µs (0 failed retries);
  R8 pulse 8 µs for both; measurement 19,012 µs (1,188 polls) for both.

New hidden state is now actually observed:
- In canonical history, the caller already arrives at A4CF with C5=0 and adjustment OFF
  (before acquisition). A067 and A06B are all 0 / off.
- Before R5 at 5BFB: C0=58, C4/C9/C5 = 0/0/0, adjustment OFF, effective IVM=1.
- At R7 fetch 5B0A: C0=13, C4/C9/C5 = 4/0/0, adjustment OFF, added line 0, effective IVM=1.
- At exit 5B17: C0=17, C4/C9/C5 = 0/1/0, adjustment OFF, parities=1.
- Direct entry observes the same R5, R7, and exit states.

This replaces the old pre-fix history state (A4CF C5=31 / adj ON, 5BFB C5=24 / adj ON,
and 5B0A C4=2 / adj ON / added line 1). Do NOT say new code clears C5 by R8 write or adds a
reset policy; it corrects IVM row counting during adjustment, and the full canonical prefix
now naturally arrives at S without inherited adjustment. The source fix remains independently
derived from French ACCC §11.8 p93; this is observed bounded state normalization, not scalar
coincidence or comprehensive C5 author adjudication.

Comparing all 64 diagnostic `c16e7ec` canonical history images against default `076eab1`:
51 are RGB identical, and 13 raster pattern views changed (`MISTER_1_022B.png`, `022C`,
`0230`, `0231`, `0233`, `0236`, `0239`, `023A`, `023B`, `0287`, `0288`, `0298`, `0299`),
while S, E, and P are identical. Parent inspected all 64 contact views; each of the 13 changed RGB contents
exactly matches a previously inspected view in the 076/9a/ae9/599 corpus. No
novel content was found and no physical-photo verdict is refreshed.

Physical photograph CS_02D3_A.webp (4E30) versus current program (4E40) revision binding
remains open; scope is a targeted history walk, not a broad baseline or physical photo
rejudgment. See the [passive observer qualification](shaker-s-passive-observer-2026-11-04.md)
and [interlace counting investigation](crtc1-interlace-adjustment-counting-2026-11-04.md).


## Available2.6/2.7 program check — 5 November

Local SHAKER2.6 disk SHA256f7082f8eab521d632c343a288f54038af6df090c59b372e0d2866269c2cc4d08
contains C body137c51511f9580639c6179eb4ee846145388e84a40e164270386466f22b66498.
The current2.7 C body is3647e120e282412d5afc31bed0e4b6673beed850ba94ec7a4d5374ce04a4682f.
Root verified identical expected-value setup5A91..5ABC and polling/arithmetic
5B01..5B2A/5B2E..5B3B: both select4E40 for CRTC1, seed HL0040, and scale the
16µs polling count by16. Relocated formatter call operands differ, including
5A90; the earlier provider claim that5A90..5ABC was wholly byte-identical is
corrected. A direct changed expectation/formula does not explain4E30 between
these available versions. Earlier caller/history changes and the photograph's
actual program binding remain unproved; do not rule out all version effects.

One printed0x10 step is one polling quantum, not proof the physical VSYNC edge
itself was16µs earlier. No runtime or hardware recheck was performed. Evidence:
resumed-20261104/s-photo-program-binding/{HANDOFF.md,PARENT-ASSESSMENT.md,PARENT-CHECKS.json}.
The physical-photo boundary remains open; current bounded S acceptance is unchanged.
