# CRTC3 status and C4-overflow failures

Current `fdf348c` fails the original program checks on DU/03C0 and E/0406.
Fresh AmSpirit Lite 1.17.1 completes both pages with every check clear. These
are confirmed failures against the current program, separate from a matched
physical-Plus photograph or a selected RTL mechanism.

| Check | MiSTer fdf348c | CPCSyntaxError v0.12.0 | AmSpirit1.17.1 |
|---|---|---|---|
| DU status1 bit5, zero VSYNC-width nibble | WRONG | OK | OK |
| DU status2 bit0, final captured sample | WRONG at4DFF | Same failure | OK |
| DU status2 bit4, adjustment-context capture | WRONG at0000 | Same failure | OK |
| E0406 ordinary R4/R9/R0 variants | Several failures | Checks clear | Checks clear |
| E0406 final ROM-select variants | Multiple failures | B0D/B1D fail | All clear |

MiSTer and AmSpirit use the same French Plus cartridge and current 2.7 disk:
CPR SHA256 `ab241580c9b6a9fa9aeae94ca6847ea70dca386d305e38fc2ac03fce603360cf`,
DSK SHA256 `6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
AmSpirit reports frontendLite 1.17.1/core 2491682, 6128+, CRTC3, 128 KB, FR;
monitor Off. CPCSE uses matching source 22de765 and its packaged English Plus
cartridge, with no chaining or built-in errata. Root viewed the full target
images and all other spot-capture images; their other checks are not regraded.

## Source boundaries

Re-extracted current D/E payloads and original Type3 scripts explicitly enable
these pages. DU rejects types below3; E distinguishes Type4 expectations in
its final rows. No Type4 photograph or result is substituted for Type3.

DU's printed offsets describe software sampling, not raw counter registers.
S1 bit5's FFFF+15 comes from a line-loop countdown. The status2 collector
reconstructs its frame map from624 samples at 32 strided phases. S2 bit4 first
checks a normal frame, then programs R4=25hex/R5=8 and expects a different
predicate in that adjustment-context capture. French§21.3.4.2 p249 instead
lists bit4 as always 1. The capture/interpretation and this apparent table
conflict need qualification before changing the constant; CF-9's bit3timer
question is a different issue.

French §21.3.4.1 p248 gives opposite event polarity for the zero-width
case: bit5 is HIGH on fifteen VS lines. Applying the table's event-value
convention implies LOW outside that band, also required by the original
297-LOW/15-HIGH caller. Matching CPCSE source explicitly switches polarity.
Fresh Opus review supports this isolated repair without relying on the peers
as hardware evidence. A private candidate fails-before/passes-after the
outside-band check and passes all 75 ASIC-video tests. The original CPU caller now finishes all 64 drifting sampling frames: 19,968
status reads, exact per-frame grid and natural carry-clear success. The
baseline reproduces FFFF at the second LOW read. Source commit9c11daf contains the isolated repair with corrected comments and
first-line/before/after-band assertions. The selected46-bench gate passes;
native RBF validation passes for the repaired bit5 check. Live-R3 changes remain a named
assumption, not a newly tested effect. No whole03C0 page pass is claimed.

E0406 reads PPIportB bit0 after its register sequence as an overflow witness;
it does not read C4 directly. French§12.5 p103's final simultaneous ROM-select/
R4 race is explicitly excluded by the current `asic_video` implementation.
The interface has no explicit B5 flag, but the motherboard already shapes
`board_iorq_n` using READY and A13 before generating ASIC ENABLE
(`rtl/Amstrad_motherboard.v`, board decode and `asic_vid` wiring). The
earlier inference that a missing port prevents the ROM distinction was
wrong. Existing enable timing must be evaluated before adding any interface.
The ordinary-row failures and the ROM race still need separate proof.

## AmSpirit acquisition qualification

The first marker-only DU adapter stopped on the first page: its early Space
could not replace the author's long waits. That attempt is retained without
a final-page verdict. A fresh direct-U run preserves the authored 10/10/10/15/
45-second waits. The final45-second screenshot still says PLEASEWAIT, so the
already-running measurement was resumed for 1,000 frames without another guest
input or state edit. The later complete 03C0 page prints every check OK.
E follows the original module order through 0406 and clears every row.

Private evidence is under
`local/overnight-20261103/resumed-20261104/`:
`milestone-release-20261106/{captures,inspection}/`,
`c3-status-overflow-cpcse-20261106/`,
`c3-status-overflow-amspirit-20261106/`, and the exact-byte/French-source
`c3-status-overflow-source-readiness-20261106/` brief. Original failures,
incomplete captures, snapshots and the completed continuations remain intact.

The production-T80 caller proof is retained separately in
`c3-status1-original-caller-proof-20261106/`. It uses the original register
setter/table and PPI synchronizer/delay helpers, architectural SNA loading
and ideal RAM; it does not establish firmware/FDC boot equivalence. Actual
IN latch edges and subsequent architectural A values qualify read timing,
including the first VSline15/C0=0 sample and final frame C0=63 endpoints.

Public gate: `python3 sim/select_tests.py --run` → `select_tests: PASS 46 benches`.
The unchanged source/log hashes are retained in
`c3-status1-public-integration-20261106/`. No whole-page pass or status2
correction follows from this isolated bit5 result.

## Qualified first ordinary overflow pair

The original E7128 caller now independently reproduces witnesses1/1 against
the program's1/0 expectation, matching native captures. Both cases pass 21
execution/transport guards. Their captions3C/3D correspond to raw valid-write
C0=62/63; the original synchronizer and all register restores/compare execute.
Both R4=0 values are stored before the same terminal decision. The live
R4/C4 match disappears, C4 becomes2 and R7=2 starts VSYNC, which the real
PPI read measures. This is a functional counter boundary, separate from
monitor response and the ROM-select exception.

The paired calibration history is identical and target write/read times
differ by exactly 64 master clocks. Root verifies all 142 artifacts in
`c3-e-first-pair-proof-20261106/`. This locates the current RTL path; a
retained-comparison candidate now passes the qualified ordinary cohort,
with the limits below. No physical sampling aperture is claimed.

## Native acceptance on 9c11daf

Exact-SHA full CI and production-T80 jobs pass. The verified 3,125,268-byte
RBF has SHA256 `a03ecb37e4a4a290968b7adb8b16b07309cf9e4bd0448a7d9e82880801f6e314`;
setup/hold minima 0.368/0.246 ns across seven clocks, zero TNS.
The targeted original DU walk completes with **status1 bit5 OK**, including
its subsequent positive-width cases. Status1 bit7 is also OK. Status2 bit0
WRONG4DFF and bit4 WRONG0000 are unchanged, so 03C0 stays broken.

All twelve condition-true/false checks on 03BF are clear, matching AmSpirit;
that page gains a qualified original-program self-check passing grade.
This does not claim a physical-Plus photo comparison. Root verifies all ten
manual/SSM native images and their hashes/dimensions, six actual Full0 records
and CFG restoration. The final manual wait is 90 seconds; both automatic and
manual 03C0 images show the complete result. Native captures omit OSD and
physical display response. Evidence: `c3-status1-native-20261106/`.

The same RBF preserves the sampled CRTC3 DSC4/Amazing scenes. DSC4 credits
(all four colons/names) and first otter frame are byte-identical to fdf348c;
the later effect and Amazing samples remain visible at different animation
phases. All five images are viewed and hashes/CFG restoration verified.
This is bounded preservation, not complete-demo or physical-display acceptance.

## Private E candidate and ROM write windows

One CLKEN-sampled retained R4 match, qualified at the terminal character and
ORed with the live comparator, changes the focused failure to77/77 passing
leaf checks. All12 original ordinary rows give their expected1/0 pairs and
pass312/312 transport guards. The exact first-pair guards pass21/21 each;
the unchanged ASIC R0 owner passes7/7. Register storage, R9/R5 and live
status2 bit0 are unchanged. These results support the ordinary deadline
outcome, not a unique silicon latch. The two R0=59 caption pairs are duplicate
stimuli: the original payload does not apply the advertised R9-reset patch.

Whole0406 remains broken. ROM0D/1D give0 against expected1. They also fail
the frozen ordinary single-write-onset guard, so their old qualification
remains **UNQUALIFIED**, rather than being rewritten after the observation.
One raw CPU write `[13108690,13108778)` becomes two genuine board ENABLE
windows, `[13108690,13108706)` and `[13108750,13108770)`, for A13=0.
The other three ROM-family variants have continuous ENABLE across that raw
write. Actual selected READY is observed independently; the line-end CLKEN
is13108745, during the gap between the first two windows. This explains why
the existing interface can distinguish them, without proving the correct
consumer timing rule. No pulse-fall commit or additional decode is inferred.

The immutable candidate packet is `c3-e-retention-candidate-20261106/`:
737 artifacts, manifest SHA256
`84ec05c1824de2d3194f1a51f90c4e35fb78780b67f81cbdbbeb0b48f31ce3d0`.
The report includes raw CPU/ENABLE/READY/clock intervals and source wiring.
A focused independent review of the next contract is pending; no E change
is in public RTL, no public gate or RBF is claimed for it, and the tally
stays unchanged.
