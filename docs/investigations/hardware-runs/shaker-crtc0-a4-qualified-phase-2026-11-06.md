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

## Boundary before a repair

French §4.4.3 p26 explicitly distinguishes write positions within the same
microsecond. The same §13.6.1 figure requires ordinary OUT(C)3D→40 and3E→0,
as well as OUTI3B→40 and3C→0. Those ordinary-OUT neighbors must constrain
any candidate; a broad rule retaining an old terminal equality could fix the
OUTI symptom while breaking ordinary OUT.

The existing asset-free ASIC R0 terminal bench already provides a CPU-run
IN calibration and OUT/OUTI guests through the B18/PPI/T80 composition.
A private Type0 adaptation is the next bounded proof, with independently
qualified instruction anchors. Its existing Plus oracle is not reused as a
classic oracle. Neither a global E-fall register change nor a guessed delay
is authorized by the current result. The real motherboard nCLKEN phase is
also distinct from a synthetic half-character testbench phase.

Type1's §13.6.2 expects40 at both3C and3B, and the earlier Type1 caller gives
that compatible continuation. This Type0 defect therefore does not explain
Type1's separate scalar/photo mismatch or justify transposing a wrap rule.

Private evidence:
`local/overnight-20261103/resumed-20261104/c0-a4-original-phase-20261106/`
(report, original bytes, pin/counter/fetch ledgers, frozen analyzer and parent
acceptance). The artifact manifest SHA256 is
`b7392a8fc8fe4531762b6d74328388d7c486794180730f3e5975000dd61c12ba`.
