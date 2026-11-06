# CRTC0 A4: qualified R0 continuation failure

The original cold CRTC0 A4 caller now reproduces a failure against French
ACCC §13.6.1 p124 independently of the photograph's scalar result. Its first
OUTI starts at software-relative C0vs3C and continues3F→40; the diagram
requires3F→0. Its second OUTI starts at3B and correctly continues3F→40.
Both observed scalar bytes are00, matching the native failure.

This is one production T80/GA/CRTC diagnostic with ideal RAM and a reset-ROM
module scaffold, rather than firmware/FDC boot. It executes the original
Module A initializer, type detection, French font/metadata, real HID key4,
seven preliminary measurements and both Type0-only preliminary calls before
the target pair. Original helpersA4D7/A4D8, immediate R0 restores, deadline
helpers, scalar reads and natural returns all qualify. No hidden counter,
beam release, runtime guest-memory patch or selected instruction phase is
forced. It completes442408568clocks in367.123seconds on its first run.

## Independent operational timing

French §7.2 p39 defines the ruler through the earliest successful drifting
IN's fourth microsecond. The frozen analyzer requires the previous failed
IN, actual19969µs drift versus19968µs frame periods, original ED/78 and ED/A3
identities,80/64clock prefix alignment and matching second-M1 GA phase.
Both calls satisfy those guards. The exact homologous second-opcode M1
intervals are25151/25150µs, giving `(interval−3) mod64` =3C/3B. Fractional
ED-prefix intervals are preserved and never rounded into a desired bin.

| Call | Required next C0, French §13.6.1 | Observed next C0 | Current storage/decision |
|---|---:|---:|---|
| OUTI3C | 00 | 40 | New7F is stored6master clocks before the3F decision |
| OUTI3B | 40 | 40 | New7F is stored70clocks before the3F decision |

The first write's selected-level onset is424499714; storage changes before
row424499715, the3F decision is424499721 and C0 becomes40 at424499722.
Its write interval ends424499754, after that decision. These are measured
production abstractions, not a measurement of a physical chip E aperture.
The shared register file currently publishes R0 on selected system-clock
edges and the later counter decision consumes that new value.

The pre-run analyzer passes33qualification checks and returns intentional
`QUALIFIED_RED` for the first continuation only. Root verifies all137artifact
hashes,36unchanged production/build source pairs and20unchanged frozen
execution items. No production RTL is changed by this diagnostic.

## Private repair evidence and limits

The asset-free production-T80 adaptation now qualifies all four independent
French §13.6.1 bins: ordinary OUT(C)3D→40 and3E→0, OUTI3B→40 and3C→0.
Current RTL misses only OUTI3C. All88 timing guards pass. OUT(C)3D and OUTI3C
have the same raw C0 at valid onset and the same write-end time relative to
the decision, so a broad old-terminal hold or write-end rule cannot separate
them. Actual motherboard nCLKEN falls between their valid onsets; it differs
from a synthetic half-character phase.

The narrowed candidate retains a true Type0 terminal predicate sampled at
that existing nCLKEN, ORed with the live comparator. Register storage remains
system-clocked, with same-edge data bypass and reset/SNA/live-type invalidation.
Sampled total1 stays on its existing path as a coverage boundary; matches that
arise after the sample also keep their live behavior. This does not assert
that every late lowering is unchanged, or that R0=1 ignores the timing rule. This is
an implementation inference: the four bins admit a range of sample positions,
not a uniquely identified silicon edge. No opcode, guest PC or fitted delay
selects the behavior.

The unchanged four-bin analyzer now passes, as does the independent R0=0
OUTI control; ordinary OUT and both Type1 controls remain compatible. The
unchanged original cold A4 caller then passes33/33 guards with continuations
00/40 and scalar01/00, matching the physical C0 result. It runs once for
443670136clocks in372.218seconds. An every-master-clock census finds only
one effective/raw difference interval during the original caller: seven
clocks following the critical late write. There are70 actual R0 write
onsets in that run.

A separately preregistered R0=1 late-OUTI case retains its baseline2/3/4
counts and C4=1 byte for byte. Its unchanged no-increment oracle remains
RED and unproved: review identifies a possible confusion between the chart
cell, model I/O onset and the preceding line's inherited counter decision.
The narrowed model preserves this domain rather than changing its scripted
phase0 compatibility assertion. French §13.7.2.2 p128 has no OUTI chronogram
for that phase; this is distinct from the directly diagrammed p124 repair.

All246 classic,45 scripted-phase and focused genuine-T80/historical/Plus
checks pass. Four asset-free additions to the existing T80 harness fail
before the fix where expected (late OUTI and zero-total departure), while
ordinary OUT and early OUTI controls pass. They separately check immediate
register publication, counter outcome and the following line's new total.
The public46-bench gate passes once; unchanged fixed-seed soak stimulus and
projection produce intended new hash31f3de4cf3288d9c. Fresh Opus review approves an experimental RBF. Its test-gap finding is
closed: the zero-total OUTI case now checks unchanged C4/C9 as well as C0.
A one-shot mutant retaining the old frozen-counter consumer fails that
assertion while the corrected RTL passes. After the last test edit the
selected gate again passes all46 benches. Source `8ba384e` is pushed and
exact-SHA full CI, production-T80 and synthesis pass. Native first-pair
acceptance passes; wider C0 R0 pages remain a hardware acceptance gate.

Type1's §13.6.2 expects40 at both3C and3B, and the earlier Type1 caller gives
that compatible continuation. This Type0 defect therefore does not explain
Type1's separate scalar/photo mismatch or justify transposing a wrap rule.

Private evidence:
`local/overnight-20261103/resumed-20261104/c0-a4-original-phase-20261106/`
(report, original bytes, pin/counter/fetch ledgers, frozen analyzer and parent
acceptance). The artifact manifest SHA256 is
`b7392a8fc8fe4531762b6d74328388d7c486794180730f3e5975000dd61c12ba`.

Additional private evidence is in
`c0-a4-r0-boundary-readiness-20261106/`,
`c0-r0-degenerate-proof-20261106/`, and
`c0-r0-nclken-candidate-20261106/` under the same parent directory.
The [new author-question draft](../../classic/accc-author-questions-2026-11-06-fr.md)
keeps the p128 interpretation separate from the proved p124 repair.

The final narrowed packet is `c0-r0-kept-equality-candidate-20261106/`;
root verifies all784 artifacts. Public gate/soak receipts and stable final
diff are in `c0-r0-public-integration-20261106/`.

Final review is retained in `c0-r0-public-integration-20261106/final-review-provider-run/`;
the frozen-counter mutant and final gate receipt are in
`c0-r0-postreview-counter-proof-20261106/` and
`c0-r0-public-integration-20261106/FINAL-GATE-RECEIPT.json`.

## Native A4 acceptance and remaining lower-section failures

Full CI run42196903156 produces verified RBF SHA256
`1391b7b8a2a732a70d9bfd98123dbff463fd5d0b28b87dd2485c1cdb2ce5938e`,
3,123,016bytes, Quartus17.0.2 build602 SJ Lite Edition, Standard Fit maximum
Fitter effort. Setup/hold minima are0.534/0.186ns across seven clocks, zero
TNS. Device hash/size match; the run uses Full and normal WAIT (bit6 clear).

The original direct native A4 walk now prints01/00. Marker and later images
are byte-identical. Compared with fdf348c, all changed pixels are confined to
the first OUTI scalar and the extra C9-sweep row; that sweep is now01×8,
matching current CPCSE26/27, but absent from the physical photo. All five
following C0 AI marker images are byte-identical to the milestone baseline.
This accepts the narrow R0 repair, not the complete page or every phase.

A complete lower-section recheck identifies **six existing VS-bit mismatches**
in the R0=1-origin pairs. The photograph and current CPCSE give
5E5F/5E5F/5F5F/5F5F/5E5F/5E5F/5E5F/5E5F. Both native builds giveFFFF
throughout. Removing only the documented unrelatedA0 upper-input difference
leaves5F5F throughout: six first-byte bit0 values still disagree. These are
not repaired by the p124 change. The exact photo binary remains unbound;
peer agreement supplies a useful discriminator, not a hardware timing rule.
A4 therefore stays **broken** in the maintained tally. The unreferenced C9
row is a separate unknown, not a reason to hide those confirmed differences.

Private native inputs/commands, quality receipts, full-page comparison and
preservation checks are in `c0-r0-native-20261106/`. The additive full-page
receipt corrects an initial pre-run grading note that overlooked those six
low-bit differences. No R0=1 repair is selected from this image.
