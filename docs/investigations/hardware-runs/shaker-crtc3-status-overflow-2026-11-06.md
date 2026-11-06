# CRTC3 status and C4-overflow failures

> Physical-reference correction: the ignored canonical real-CPC cache contains
> CRTC3 DU/03C0, showing STATUS2 bit0 andbit4 OK. Earlier no-photo assumptions
> were wrong. The available2.6/2.7checks are byte-identical; see the final
> section for the qualified adjusted-stage failure and reference provenance.

Native baseline `fdf348c` fails the original program checks on DU/03C0 and E/0406.
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
R4 race was excluded by the baseline `asic_video` implementation; the reviewed
branch candidate below now handles the source-program cases through existing
board-shaped ENABLE timing.
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

## First private E candidate and ROM write windows (superseded)

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
That first candidate remained private and its qualification failures stay
frozen. The second candidate below supersedes it. The page tally remains
broken until native acceptance.

## Reviewed selected-write candidate on the task branch

The branch now retains the sampled C4=R4 match only while the selected R4
write is active at the consuming CLKEN. Live equality, immediate register
storage, R9/R5 and literal status2 bit0 remain unchanged. Existing motherboard
READY/A13 shaping supplies the ordinary/ROM distinction; there is no added
ROM input or decode. `MODEL LIMIT (B26):` names the inferred active-write
rule, late false-to-true compatibility and unproved extension beyond R4→0.
A write-end-commit model fits the same original cases; their divergent late
false-to-true prediction remains an unsent author/hardware question.

Before this edit, a source-shaped64-clock-character discriminator records
two independent failures: baseline misses both late ordinary wraps at totals
63/59; the first retained-match candidate misses both fragmented-ROM overflows.
The final candidate passes all25 frozen controls and78 preservation checks.
All17 unchanged original production-CPU callers then match their programmed
witnesses: twelve ordinary rows form six1/0 pairs and the five ROM rows give
1/1/0/0/0. Original first-pair guards pass42/42, and startup timing/read logs
are unchanged. The two old single-onset guards remain25/26 UNQUALIFIED;
a separate source-bound operational qualifier passes238/238 checks across
all17, without changing original expectations or analyzers. Census525points/
nine CLKEN decisions and the seven-case ASIC R0 owner pass.

The asset-free public bench keeps six independent source-window cases,
including total59 controls; all75 existing entries are unchanged. The selected
command `python3 sim/select_tests.py --run` passes46 benches, ASIC81/81.
After the last functional/test edit, only an Opus-requested overview comment
changes; root records identical non-comment tokens. Exact-SHA CI follows the
push. No classic soak is re-minted for this ASIC-only behaviour change.

Fresh Opus5.5 review finds no RTL or test bug and approves experimental
synthesis. Full-effort zero-TNS timing is mandatory: the selected bus/READY
path now reaches vertical next-state logic. Native0406, DU, Plus demos and
owner checks remain required; a title with a qualifying late nonzero R4
rewrite should also be bound if available. This is a reviewed source candidate,
not a native whole-page pass, master integration or public release.

Evidence under the session root: `c3-e-rom-window-candidate-20261106/`
(753 artifact hashes verified; manifest707b28400595f25afd39a6a4f4ecf73b5f7b5525227d79c09a8077adb2f2b599),
`c3-e-public-integration-20261106/` (gate/source/hash receipts), and
`c3-e-final-opus-review-20261106/` (review, root acceptance and provider limits).

## E2 full build and targeted native acceptance

Source5c1ab5771e76eae0cff9cd2a76c73f6831551e5b passes the exact-SHA selected
simulation and production-T80 CI jobs. Full synthesis run
[42209634374](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42209634374),
artifactAmstrad-build-408-1-full, uses Quartus17.0.2 build602 SJLite, Standard
Fit at maximum effort. All seven clocks close: setup minimum0.550ns, hold
minimum0.241ns, zeroTNS. The3,149,156-byte RBF has SHA256
`0165785e2120f16a748f31543338d7faf8c5b4e4e4513fb2b22fd2a164f65d17`,
verified on MiSTer before loading. Packaging uses dry-run only; the existing
release draft and master remain unchanged.

Original native E entry retains its four preceding R5 pages. E0406 now clears
all12 ordinary witnesses and all five ROM-select checks, including0D/1D. Root
views every capture and the settled page against the failing fdf348c and matching
AmSpirit pages. There are11 PNGs and10 actual requested/appliedFull0 records;
hashes, dimensions, normal-wait CFG and CFG restoration verify. The four settled
03FA–03FD images are byte-identical to fdf348c. E0406 becomes passing by its
original-program checks; no indexed original-CPC E photograph is available.

The first transport attempt stopped at AmSpirit's unsupported wait_frames
instruction after03FA. Its inputs/manifest remain preserved. The native variant
uses a conservative1s settling wait instead and completes; guest code and DUT
are unchanged. This is not classified as a failed hardware result. Private
packet: `c3-e-native-20261106/`, including PARENT-E-ACCEPTANCE.json.

DU preservation also completes: all ten captures are byte-identical to9c11daf,
including passing03BF and the retained03C0 status2 failures at4DFF/0000.
Six actualFull0 records and CFG restoration verify. PARENT-DU-ACCEPTANCE.json
binds every PNG. DSC4/Amazing and AE/AR preservation also pass in the scoped native samples:12
PNGs with verified hashes/dimensions and CFG restoration. Credits and the first
otter are byte-identical to9c11daf; later otter and both Amazing samples retain
their visible effects with animation-phase differences. All seven AE/AR frames
have exact accepted fdf348c/c093 phase counterparts. AE/AR has12 actualFull0
records; demo runs supply requestedFull/CFG evidence only. No whole-demo or
physical HDMI/CRT sign-off follows. PARENT-PRESERVATION-ACCEPTANCE.json and its
comparison bind the images.

A separate pinned CPCSyntaxError0.12.0 DSC4 trace contains527,529 register events
and1,349 R4 writes. It reaches the intended credits/otter scenes but supplies no
changed nonzero R4 write qualifying the coarse late-terminal discriminator.
The initial zero-event report was an offline parser whitespace bug, preserved
and corrected without replay. UK peer firmware and character-rounded writes
could not establish MiSTer bus/CLKEN phase anyway. Broader late nonzero R4 title
coverage therefore stays open, as do false-to-true behavior, the rival write-end
model and the split consumer. Native acceptance does not establish silicon
latch topology or complete R4 behavior.

A second pinned peer trace covers the unchanged Amazing script:202,907 register
events and1,590 R4 writes, with both requested logo/lower-text captures viewed.
Its527 repeated0→17 rewrites occur atC0=11/R0=63, so oldC4/R4 equality there
is ordinary rewrite coverage rather than a late terminal overlap. No changed
nonzero near-terminal candidate is found in this sampled script. The specific
late-nonzero title acceptance remains open; neither trace is a MiSTer aperture
oracle. Private packet: `c3-e-amazing-r4-coverage-20261106/`.

The next DU status2 unit is an unchanged original bit0 collector proof, before
attempting bit4's contradictory adjustment predicate. A Gemini static source
map is retained with parent corrections in `c3-status2-proof-readiness-20261106/`.
It is not executed evidence: sampler timing, IN-to-banked-RAM/remap provenance
and resident dependencies must qualify before any timing or predicate change.

## Qualified normal-frame status2 bit0 boundary

A frozen5c1 resident architectural-loader proof now completes the original
normal-frame bit0 capture/analyzer:19,968 genuine IN/store/analyzer reads,
83-byte relocation, two12,288-byte bank6 copies,124,462 replayed writes and
542,147 architectural T2 entries all qualify. Original RET938D reaches933C,
HL93A7/SP9DE4/carry clear. First/last samples areB7/96 at counters0/0/0 and
63/38/7; all source32/33us intervals and24576us synchronization offset match.
There is no phase fit or post-release host state change. Parent verifies all44
final artifacts, manifest8cad68a24558781b1b66e6119d53e4a8257aaf058787286e0c56ed0b48c29915.

This is the **normal first stage only**, not a full DU bit0 pass or native
warm-history reproduction. The printed4DFF offset alone cannot identify which
stage failed: normal9388 reports it for a HIGH final sample; adjusted9368 also
reports it for a LOW final sample. The original second capture programsR4=37,
R5=8 and expects LOW at4BFF, then HIGH through4DFF. It remains unexecuted in
this proof. Preparation now follows that unchanged original continuation
before attributing the native result to a read aperture or firmware history.
No comparator change or bit4 repair is selected.

Private evidence: `c3-status2-original-bit0-proof-20261106/runtime-v3/`.
The unexecuted v1 guard review caught SP/DE reads before T1 deferred commits;
v2/v3 observe them at T2. A v2 lexical compiler error and a source-path comment
identity-helper error are preserved; only one v3 DUT ran. All original guest,
phase, oracle and36 production inputs remain frozen.

## Qualified adjusted-stage bit0 failure and physical reference

The unchanged original continuation now completes both captures under the
frozen5c1 production composition. Normal geometryR4=38/R5=0 passes. With
R4=37/R5=8, the reconstructed buffer has bit0LOW at4BFF and4DFF. Original
code requires only4BFFLOW;9368 branches to938E on the final adjustment byte,
HL5DFF/SP9DE4/F54. This is a qualified original-software failure; the actual
native branch remains unobserved.

All39,936 real statusIN→DI_Reg→A→store→original remap→analyzerRAM→A receipts
qualify, along with49,318 copied bytes and248,928 replayed writes. The second
C7→C7 command is observed separately from actual map changes:10commands and
9changes. Both sample generations and final128KB match the independent write
replay. The one DUT and analyzer each exit0; execution takes161,520,447clocks
and146.578host seconds. No input, phase or expectation was fitted or retried.

The body and adjustment LOWs both haveC0/C4/C9=63/37/7, with in_adj0 and1
respectively,512µs apart in the same strided phase. These counter observations
are diagnostic, not qualification gates. Parent reads the complete observer
delta, accepts Gemini’s independent review with no functional finding, and
verifies226input,30compiled and46final-artifact hashes. Private packet:
`c3-status2-adjusted-bit0-proof-20261106/runtime/`.

The [canonical physical CRTC3 photograph](https://shaker.logonsystem.eu/images/cpc/CPC/DU_CRTC3_D.webp)
shows STATUS2 bit0OK andbit4OK. Root and Astra independently read those labels.
Its cached SHA256 isbc2837b3df66b3b46471ec603d627823e6eb9aa53bd43040a777bde8143377e5;
the portal index identifies it as Real Hardware, not an emulator. It was
already available in the main clone’s ignored cache; the earlier missing-photo
claim was an evidence-discovery mistake. Photograph identity does not identify
its exact disk binary. Available2.6/2.7 predicate comparison is the next binding
check before using that OK to settle the adjustment rule.

French§21.3.4.2p249 gives raw counter equalities, while§11.2.6pp85–86 assigns
C9’s adjustment termination toR5. This does not by itself establish the status
comparator’s domain. Astra’s focused judgment keeps the source interpretations
separate and revises its recommendation after viewing the physical photo:
a matching two-stage hardware verdict permits a scoped fail-first correction,
without proving comparator topology or transposing it to other status bits.
No production comparator or bit4 change is selected in this checkpoint.

## Scoped bit0 candidate: original two-stage PASS, hardware pending

The candidate adds only `!in_adj` to the strict live STATUS2bit0 equality.
It leaves R9 equality, vertical counters, timing and other status bits intact.
With normalR4=38/R5=0, the body-endLOW remains4DFF; withR4=37/R5=8 it remains
4BFF and the512adjustment samples stayHIGH. A focused counter/status vector
fails at adjusted4DFF before the RTL edit and passes unchanged afterward.
All46selected benches pass, including82ASIC-video tests.

Available original26D/27D collectors, status predicates, geometry helpers and
synchronizers are byte-identical at the same addresses. The complete payload
difference is accounted for by the version digit, relocated font workspace
and10bytes of SSM stubs. This supports the canonical physical03C0OK verdict
without claiming the photograph’s exact binarySHA is known. See private
`c3-du-photo-program-binding-20261106/`.

One candidate production-CPU replay with the unchanged observer, SNA, original
guest and software oracle now reports QUALIFIED_PASS. Both19,968-sample captures qualify through IN/store/analyzer transport
(39,936total samples), and originalRET9373 reaches9159 naturally,
HL93A7/SP9DE6/F44. No source timing, phase or expectation changed. It retains
one diagnostic difference at adjusted4DFF: actual counters still63/37/7 with
in_adj1, but bit0 isHIGH instead of the former literal-comparatorLOW. The
diagnostic never owns qualification. Private candidate packet:
`c3-status2-bit0-body-candidate-20261106/`.

Fresh Opus5.5 source review finds no correctness/regression bug and gives
conditional experimentalGO. It reads the changed actual source/test regions;
its sandbox cannot read the separate proof packet outside the worktree. Parent
checks the exact two-file diff/hashes and independently binds the reported
gates, physical reference and original CPU evidence. That scope limit is
retained in the review receipt. Full fit with zeroTNS and native bit0 repair
without anotherDUregression remain required before hardware acceptance.

MODEL LIMITB26: the observable pair is supported by the canonical physical
reference; status-comparator topology and generalR5/live-write behavior remain
inferred. Suppression also affects adjustment in R8=1/3frames; it is not
noninterlace-only. The separate extra-interlace-line behavior is unchanged.
R5=16 would separate the ordinary-domain interpretation from a last-adjustment-
line-only rival; no invented expectation for that unmeasured corner is added.
Other status bits, especially bit4, remain separate. No whole03C0pass, native
acceptance, master integration or release approval follows from this checkpoint.
