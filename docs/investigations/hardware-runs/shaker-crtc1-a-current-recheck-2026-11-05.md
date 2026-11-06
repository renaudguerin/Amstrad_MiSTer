# SHAKER CRTC1 Module A: current targeted recheck

Original direct entry and the original A1 sequence through `009A` both show
the changing RAM-pattern display on experimental build `c0939d8`. The old
black-screen flag no longer reproduces. Rechecking the neighboring AI pages
also exposes an OUTI result on `0037` that the older broad PPI classification
had incorrectly accepted. No RTL change follows from these observations.

## Inputs and coverage

The installed artifact is `Amstrad_20261105_c0939d8.rbf`, SHA256
`cb7fe0bd06c1d282413e501fd6017a34dba11ae62f4483e388a630c7ca1d883e`.
Runs use SHAKER 2.7 disk SHA256
`6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`,
normal boot, CRTC1, French layout and requested Full filtering. Requested
filter settings do not establish the applied OSD state or physical monitor
response. The [B9 report](shaker-b9-current-reference-2026-11-05.md) owns the
unchanged source, gate and synthesis acceptance.

The direct O run completes 29 commands, ten PNGs and 17 SSM records without
failure. The original A1 prefix through `009A` completes 220 commands,
111 PNGs and 206 SSM records without failure. Root visually screens all
111 images in eight contact sheets and inspects the named target pages at
full size. Incidental captures are not a new graded Module A baseline.

## Current page judgments

| Page | Judgment | Evidence and limit |
|---|---|---|
| A4/`0037` | Broken | First OUTI result has bit0=0 on MiSTer; the hardware photograph has bit0=1. |
| AI/`008D` | Broken | Current canonical zero-length groups give1000 against photographed1111; the earlier direct native run gave1001. |
| AI/`008E` | Passing | All four photographed and current native VS fields are high. A focused photo reading resolves the disputed final glyphs as F. |
| AI/`008F` | Unknown | Fresh capture exists; physical V2 exception glyphs remain insufficiently resolved for a whole-page verdict. |
| AI/`0090` | Unknown | Six footer low bits001111 and length66 match; the328 matrix cells are not fully readable/adjudicated. |
| AI/`0091` | Passing | All four photographed and current native VS fields are1111. |
| AO/`009A` | Passing | Both native entry paths produce the photographed RAM-pattern family, with continuing changes in later captures. |

The [results ledger](../../shaker-results.csv) counts pages once. Accepted
unrelated upper-PPI input differences remain distinct from bit0 and from the
OUTI scalar result. An unresolved glyph is not evidence for a timing rule.

## 009A: history control closes the old black flag

The direct native target and later capture differ in 68,231 pixels. The
canonical-history target and later capture differ in 70,281 pixels below
the header, bounding box `(0,48,768,241)`. Both cover the display with a
changing RAM pattern rather than the old black body/displaced text.
Root compares the original-CPC `AO_009A_I.webp` photograph: its white-bar
and RAM-pattern family agree. Exact RAM pixels depend on preceding tests;
analogue geometry and temporal behavior cannot be proved by a still photo.

Fresh AmSpirit 1.17.1/core 2491682 direct O entry also gives the full changing
pattern. Isolated CPCSyntaxError0.12.0 gives it on direct entry and after
the original canonical prefix. Its canonical run takes155.572 emulated
seconds and10.725 host seconds; the two target captures differ in235,710
pixels below the header. This corroborates the display family without
making either emulator the hardware oracle.

The AmSpirit canonical-prefix attempt reaches a150-second host deadline
before `009A`, on PLEASE WAIT. Its checkpoint/state/snapshot are preserved;
it supplies no canonical `009A` result and is not classified as a CRTC
failure. Cleanup pauses the instance and clears scripts/breakpoints.

No current-T80 VMA trace attributes the earlier black image to a particular
rule. A provisional peer-source explanation treated R1 as fixed64 although
the guest alternates63/64; it was rejected. The passing recheck does not
establish that explanation or require another RBF.

## A4/0037: a hardware-photo mismatch shared by both peers

The physical photo's first `OUTI ON C0vs=#3C` result has a clearly readable
last digit1; its complete byte is tentatively01. The second `#3B` result
is00. Current canonical native, canonical CPCSyntaxError and fresh direct
AmSpirit all print00/00. Root inspects each actual rendered page. The
focused independent photo transcription confirms the first low digit1
without treating the uncertain upper glyph as a measured byte.

This is a failed hardware comparison, not a successful peer reference and
not a difference that can be masked as unrelated PPI upper inputs. Exact
program/revision and chip-variant binding still warrant checking before
choosing a repair. The photograph lacks 2.7's additional C9=0..7 row; this
version/machine question is already recorded in
[author feedback §4](../../classic/accc-author-feedback.md#4-shaker-module-a-test-4--outi-on-c0vs3c-and-photograph-version).
Fresh extraction finds the same main OUTI sequence at7190 and first caption
at75B4 in the available2.6 and2.7 binaries, with relocated helper calls.
This does not bind either binary to the older photographed program or
prove helper timing equivalence. AmSpirit agreement with MiSTer does not
overturn the photograph. The two KO labels higher on the page are not
this discrepancy.

The next source discriminator is R0 publication relative to the real OUTI
bus cycle and C0 rollover. French ACCCv1.11 §13.3 p115 and §13.7.1.1 p126
describe its instruction-dependent ordering; the separate last-frame-line
OUT(C) extension in §13.7.1.2 must not be substituted for that contract.
Root reads the primary position-aware Markdown and fresh renders of
French pages115/124/126; §13.6.2's diagram also distinguishes the OUTI and
OUT(C) deadline positions.
No failing-first production-T80 proof or new physical timing rule is yet
selected for this observation.

## A4 R0 contract and clean resident input — 6 November

A focused primary-source audit reads French §13.3 p115, the rendered §13.6.2
chronogram p124 and §13.7.1.1 p126. Its Type1 OUTI start 3C extends old 3F
through 40 under new 7F; the start 3D control wraps 0. Current R0 storage before
the counter edge already makes that edge compare the new value. Simultaneous
first storage/carry instead uses old R0 outside the separate last-frame RFD
exception. The original caller has not yet been bound to that internal edge;
no numeric aperture or inferred correction follows.

The original critical sequence includes OUTI 7F at 7190 and immediate OUT(C),C
restoration of 3F at 7192. Its first/second helper targets are A4D7/A4D8:
715E increments HL before installing the call operand. Omitting restoration,
using the untouched literal A4D6, or restarting the later result dump would
change the stimulus. The retained later RAM already has modified wait/helper
operands and cannot establish first-entry timing.

Root captures a fresh normal-boot AmSpirit 1.17.1/core 2491682 input through real
key 4 dispatch, breaking before the first original A4 instruction at 71AA.
PC71AA/SPBFFE/return40D1, BF03=1 and RMR8E are verified. Critical 715A–71B5,
resident BC00 support and B000 metadata bytes match the untouched original
module; all 128 KiB RAM is retained. AmSpirit’s uninterrupted continuation reaches
0037 and still prints 00/00; both entry-menu and result images are inspected.
Scripts/breakpoints are cleared and the instance is left paused there.

The original v2 SNA includes a trailing `SPRT` chunk. A separately hashed
loader-compatible copy drops only that metadata, leaving the standard 256-byte
header and 128 KiB RAM byte-identical. The first attempt stopped at an overly
strict container-length guard; its original capture remains preserved. The
accepted input is `c1-a4-preinit-amspirit-20261106/run-002/`, with parent guards
and both hashes. This is a clean resident-caller input, not a firmware boot
or physical timing proof on FPGA.

A production replay must still qualify consumed loader state, original PPI
synchronization and instruction-start C0vs. C0vs is software-relative to the
original PPI origin, not automatically rawhcc. Hidden CRTC/GA/CPU-divider phase
is not transferred by this classic snapshot. Preserve and observe both R0
writes and the control case before interpreting the later PPI scalar. No new
failing-first proof or RTL repair is established by the snapshot alone.

### Current production resident replay

The private replay now executes the untouched 71AA body and every preliminary
measurement before the original pair. The production loader restores all 29
architectural fields and 131,072 RAM bytes; the first M1 fetch is 71AA. Both
OUTIs execute once with helpers A4D7/A4D8 and return 00/00. No ROM or keyboard
input dependency is observed on the executed path.

First R0=7F storage is visible six sampled master clocks before the 3F→40
counter decision; the second has 70 clocks of margin. Both counters continue
past 3F. The conditional same-edge storage/carry gap therefore is not exercised
by this original pair and supplies no repair for its scalar discrepancy.

The original synchronizer and nominal 64-clock counter history are retained.
Exact source-relative instruction-start convention remains unresolved: first
M1 onset is 59+59/64 µs from the relevant published-PPI origin, whereas T80’s
T2 INSN_START is 60+3/64 µs; the second is one µs earlier. These are kept as
fractional observations, without rounding into a captioned C0vs bin or selecting
a comparator aperture. This is not a strict French-diagram pass/fail claim.

The parent reads the C++/passive-top changes and report, verifies 36 current
source bindings, 48 artifact hashes and the immutable input, without rerunning
DUT or gates. A pre-guest ownership-guard failure is retained separately;
the corrected attempt executes the guest once. Evidence is
`c1-a4-r0-contract-20261106/resident-replay/`, including parent acceptance.
The entry gap is closed; physical-photo agreement and source-relative phase
remain open. No public RTL or test change follows.

## AI/008D: entry-path provenance remains relevant

The current original canonical sequence gives1000 in both R0=0 blocks,
matching the qualified replay's four samples. The older direct native
208 run gave1001. Both disagree with photographed1111 at S2/S3; the new
history result narrows the fourth-sample provenance issue without proving
which omitted state or phase caused the difference. See the
[AI investigation](shaker-crtc1-008d-current-2026-11-05.md) and the
[unsent clock question](../../classic/accc-author-question-vsync-r0-zero-2026-11-05-fr.md).

## Evidence location

Private artifacts are under
`local/overnight-20261103/resumed-20261104/c1-009a-native-current/`:
`device-c093/`, `canonical-device-c093/`, `amspirit-001/`,
`amspirit-a4-001/`, the incomplete `amspirit-canonical-prefix-001/`, and
`cpcse-canonical-prefix-001/`. Direct CPCSyntaxError artifacts and the
rejected causal explanation remain in sibling `c1-009a-current/`.
Photo inventories and the focused glyph readings are in sibling
`shaker-tally/`. Source media, photographs, snapshots and captures remain
ignored and are not distributed with this report.

Technical information sourced from the Amstrad CPC CRTC Compendium by
Longshot (CC BY-NC-ND).
