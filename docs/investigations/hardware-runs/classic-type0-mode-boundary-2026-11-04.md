# Type0 mixed-mode boundary: qualified failure — 4 November 2026

The retained Type0 Mode1→2 XFAIL is a real source discrepancy. A further
non-aliased control also exposes premature Mode0→1 decoding. No mode timing
change has been made from this investigation.

## Mode-qualified oracle

French ACCC v1.11 §9.3.4.3.3 p62 and p63 diagram(3) explicitly retain the last
half of old Mode1 pixel B at position4, followed by new Mode2 bits b5,b4,b3 at
positions5..7. Mode2's advance shortens the byte suffix; it does not remove B.
For source byte5B, the expected pen sequence is1,0,1,1. The independently
decoded byte origin places those slots at master ticks971,975,979,983.
Type0 instead emits new0,1,1 at971/975/979 and an extra bit at983. Type1 hides
the old slot and produces the correct three-bit suffix.

The actual-T80/READY private vector uses six distinct bytes and full fixed-mode
controls. Its controlled phase/release rule is frozen, not a natural caller
checkpoint; no new release phase or NOP count was searched. At native tick963,
Type0 already sees new Mode2 and overrides the old alternating hold/shift
controls. Type1 retains the old hold for that sample. That difference reaches
the decoder/serializer and then the exposed RGB slot.

## The aliased control was insufficient

The first Mode0→1 control appeared to preserve the old quarter-pixel required
by §9.3.4.3.2 pp60–61. Astra-high's focused read found that old Mode0 pen10
and wrongly decoded new Mode1 pen2 shared RGB13 in its modulo-four palette.
The result established suffix cadence, not old-mode preservation. The earlier
claim that the failure was specific to entering Mode2 is withdrawn.

A single new private build changes only pen10's ink to white31, leaving pen2
on ink6 and all CPU, VRAM and RTL timing unchanged. Full192-clock fixed-mode
controls pass. Type0's old slot should now be RGB62, but is RGB13: pen2 was
decoded instead of old pen10. Type1's old slot is blank; both types' new pen1/2
suffix cadence remains correct. The original aliased proof remains frozen.

## Architecture limit

The direct Mode2 override is present in Gerald's schematic. Its asynchronous
MODE_SYNC-clocked mode flops and native-clocked controls do not establish a
new publication delay, and the drawing itself is marked unverified against a
real40010. The missing observation is which native sampling edge first sees
the new mode at the colour mux and hold/shift inputs, relative to HSYNC fall
and LOAD. Neither a typed GA delay nor a common delayed event was implemented.
The Compendium's output rules remain the required behavior.

CPCSyntaxError's logical byte-position mode switch corroborates the output
oracle, but supplies no independent master-clock phase measurement. The one
focused Astra consultation confirmed the source oracle, caught the palette
alias and proposed the non-aliased discriminator; it made no implementation.
Persisted native metadata confirms gpt-6-astra/high.

Frozen evidence is under ignored
`local/overnight-20261103/classic/type0-mode-boundary/`, including primary
French renders, Gerald's inspector/fallback report and figures, source copies,
the actual-T80 traces, `ASTRA-BRIEF.md`, and `nonaliased-control/`.

## C-HSYNC discriminator

A focused Opus read found no literal stage/NBA translation defect in Gerald's
single native-clock video/control/color pipeline. Under the frozen traces, the
source0→1 old-quarter/suffix constraints require mode visibility after963 and
by967, while current Type0/1 visibility is961/965. This rules out a **simple
common constant delay** in that clock model; it does not identify a new gate.
The independent GA publication/CSYNC phase remains the issue, rather than a
justified typed GA workaround.

French §16.2.2 p166 specifies C-HSYNC deactivation1–2Pixel-M2 after CRTC HSYNC
end (or the4µs cap). Current `syncgen_sync.v` instead clears `hdelay` directly
through `hsync_n`; its synchronous event follows that modeled edge. Which
physical end phase should feed MODE_SYNC is not established by that literal
unverified drawing. The printed p166 chronogram and prose must be reconciled
before deriving an exact quantization phase; no universal delay follows here.

Parent also inspected higher-resolution primary pp56–57. Ordinary Type0
reactivation/mode switch are36/37 and Type1's37/37. In their R3.JIT rows both
are40/40. These are extra source output constraints; they are not evidence
that raw HSYNC end simply moved four dots or that one two-dot clock grid fits
all cases. A new publication model must explain ordinary and JIT together,
without fitting only the missing old-mode slot. Source render, crop and full
Opus reasoning are frozen in `resumed-20261104/type0-mode-order-review/`.
No new mode experiment or production edit followed this read.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).

## Old-pixel completion is insufficient

A second focused architecture read rejects a universal “finish the old pixel”
rule. At the observed ordinary handoff, Mode0→1 changes the colour mux while the
existing hold/shift decisions remain the same, so control-only deferral cannot
preserve old pen10. The mode-dependent control equations differ at other phases. Any common handoff
would need to govern both mux and control evaluation. Ordinary old-at963 /
new-at967 fits both transitions on paper, but does not select a circuit rule.

The independent R3.JIT rows on French pp56–57 switch at 40, inside the old
Mode1/Mode0 pixels ending at 40. Completing those pixels would switch at 41.
A LOAD exception could fit that position, but simultaneous serializer load
and colour consumption do not establish which mode owns the sample. Explicit
R3.JIT mixed-mode byte provenance is the next useful control; existing R2.JIT
blanking checks are a different stimulus. No mode handoff has been implemented.
Consultation and verified model metadata: ignored
`resumed-20261104/mode-completion-architecture/`.

## Genuine R3.JIT source-qualified mixed-mode control

A source-qualified actual-T80/READY private suite is complete in
`local/overnight-20261103/resumed-20261104/r3jit-mode-provenance/`
(`HANDOFF.md`, `SOURCE-PLAN.md`), with no timing or model changes. It qualifies
an actual write OUT(C),E R3 14→2 at C0=22 / phase 8 (C0 = R2+2 with R2=20),
hsc 2, old HS active, fulfilling French primary sources §9.3.4.4.2/.3 pp.66–67
and §14.5.4 pp.140–142. All six full 192-clock fixed-mode controls pass on both
CRTC types. Six distinct bytes B6, 6D, E3, 9C, 5B, A5 begin at line byte
38 = 2*(R2−1), with all 16 pens mapped to distinct RGBs and blank distinct from
every pen.

| Case | Type0 | Type1 |
|---|---|---|
| Actual write 14→2 at 22 / phase 8 / hsc 2 / HS active | Qualified | Qualified |
| Complete 192-clock fixed controls 0/1/2 | Pass | Pass |
| Source 40→47 A5 Mode2 bit sequence | Pass | Pass |
| Source 40 0→1 required 5B C pen 2 / RGB 13 | Fails: pen 1 / RGB 1 | Fails: pen 1 / RGB 1 |
| Required blank release 40 / tick 859 | Fails: 38 / tick 851 | Fails: 39 / tick 855 |

In 0→1, at tick 819 the serializer loaded 5B. At tick 851, simultaneous LOAD
consumes old SR = B6 (5B shifted once), pre-CIDX 13; the Mode1 mux writes pen 1
from 5B's (b2,b6), the ordinary B pair, rather than required JIT C (b1,b5).
At 855 colour is held, and RGB 859 reads stored ink 4. Thus the wrong pen 1
comes from **5B old-SR colour**, not an early A5 appearance (which also starts
with pen 1); the causal ledger resolves this alias.

In 1→2, tick 851 loads A5 while colour consumes old SR. Refresh 855 consumes A5
bit 7; RGB 859 reads that result, as required. The following 7-bit suffix
(source 40..47) agrees on both types. However, blank release fails on both
(Type0 releases at 38 / tick 851, Type1 at 39 / tick 855, versus required 40 /
tick 859). The 1→2 result provides an informative pixel-ownership control, but
is not a whole-case pass.

Preliminary invalid composites were caused by a port list shift when the frozen
wrapper was initially paired with a newer Type0 engine; coherent frozen engines
were restored, giving zero missing-pin warnings under `-Werror-PINMISSING`.
Prior universal old-pixel completion is rejected, and no rule or timing fix has
been chosen.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).

## Causal barrier after the R3.JIT control

A focused Gemini4Argon read identifies the third Mode1 shift decision at839,
before the actual R3 write840. No later MODE_SYNC delay alone can make old5B
advance from B to required C by the colour sample851. The passing1→2 suffix
starts after LOAD ofA5, so it is not a mixed-byte handoff proof. A late-half
shift schedule and a byte-phase cursor remain unimplemented alternatives;
fixed-mode equivalence is a proposed constraint, not a new passed simulation.

Parent reread Frenchp166: its plotted CSYNC is the combined signal during
the second HSYNC of VSYNC, not an isolated MODE_SYNC/C-HSYNC trace for both
CRTC types. It cannot simply be renamed as the required clock edge. The
1–2Pixel-M2 prose and chip tolerance do not independently choose a fitted
quantizer. No new timing candidate follows. Full source/read-only report is
under ignored resumed-20261104/r3jit-next-discriminator/.

## Non-aliased 2→1 controls and abstract-model boundary

A further paired ordinary/JIT proof uses 5D followed by A5, retaining each
family's actual-T80/READY schedule. All 12 complete fixed-mode controls pass.
Ordinary Type0 loses the old partial pixel and has wrong zero-fill provenance;
Type1 passes. JIT Type0's visible pen is correct, but Type1 emits pen1 instead
of the source-required zero-filled pen 0. Its causal ledger attributes that
value to old 5D shifted seven times, not the equal first A5 pen. Both JIT blank
releases remain early. No behavioral candidate followed.

One focused Astra-medium assessment corrects an assumption: the new-mode token
already occupies masked source 39 before visible 40. Visible release therefore
does not establish the internal mode edge. A universal abstract token model is
allowed; identifying literal silicon stages is unnecessary. A general causal
rule for residual-byte ownership/publication has not yet been derived. French
§9.3.4.4.1 p65 additionally requires JIT2→0 residual 5 (pen 5 for 5D), versus
JIT2→1 residual 6 (pen 0). Any proposed shared model must satisfy both destination
cadences without recipe-specific phase constants. This is a paper acceptance
constraint, not a newly executed 2→0 test.

Frozen paired proof and assessment: resumed-20261104/r3-2to1-zero-fill-proof/
and r3-stateful-model/. The consultation made no production change.

## JIT 2→0 destination-cadence proof

One further private build/run qualifies eight cases: six complete192-clock
fixed-mode0/1/2 controls and two genuineJIT2→0 transitions. Source-plan
expectations precede execution. The old5D byte must emit Mode0 pen5 at
source40; successorA6 deliberately starts pen13 to avoid the oldA5 alias.
Type0 instead emits pen0 (old5D shifted6); Type1 emits pen1 (shifted7).
Both following A6 pixels have the correct four-slot pens13/2, and both blank
releases retain their known early38/39 rather than40 failure.

The causal ledger follows the source40 RGB sample back to old5D's colour
refresh on the same edge that loads A6. Complete fixed controls, unique inks,
unchanged accepted R3 write840/C0=22/phase8/hsc2 and frozen coherent source
hashes all qualify. No phase/NOP/reset search or behavioral candidate follows.
Compared with the existing2→1 proof, the independent Frenchp65 output requires
residual5 for destinationMode0 and residual6 forMode1. A universal model must
account for destination cadence as well as preserved byte history; selecting
a single frozen residual at visible release is insufficient.

Evidence: resumed-20261104/r3-jit-2to0-proof/. Parent verified all frozen
source/artifact hashes and accepted the worker's completed command/result,
without repeating the build or run. Final line:
`JIT_2TO0_DESTINATION_CADENCE: MISMATCH`; all eight setup guards pass.

## Fixed Mode3 control qualified

The missing fixedMode3 control now passes on both CRTC types over all192
clocks of the frozen actual-T80 fixture. French§9.1p47 and§9.3.4.3.4p63 give
two four-colour, four-M2 pixels per byte; the six distinct markers yield pens
1,2 /2,3 /1,1 /3,2 /2,3 /1,2, with every pen checked for sixteen master clocks.
The private observer now distinguishes requested11/effective decode flags000
from Mode0, and verifies the actual CPU8F GA write. An old-SR/earlier-LOAD
ledger independently binds each byte; matching RGB alone is insufficient.

One coherent build/run passes both192-clock controls; existing fixed0/1/2
results are reused. Parent verifies all102 source/artifact hashes and unchanged
production hashes, without rerunning the proof. The absolute703..895 window
is a bound of the frozen fixture, not a new measurement of physical silicon
pipeline latency. No mixed-mode model or production change follows from this
fixed-mode result. Evidence: `resumed-20261104/r3-fixed-mode3-proof/`, including
`PARENT-ACCEPTANCE.md`. The source-plan gloss attributing a one-M2 lead to p47
alone is qualified there; its pixel oracle does not depend on that gloss.

## Ordinary 2→0 source-family guard

French§9.3.4.3.1p59 gives the GA40010 ordinary2→0 suffix: after the old
Mode2b3 slot at source36, residual5 supplies [0,b0,0,b2] across source37..40.
The adjacent p60 describes GA40007/8 and its one-filled missing bits. It does
not justify advancing GA40010 to residual6 at source39..40. Parent rendered
and read p59 directly while qualifying a provisional model opinion; that
opinion's ordinary-only extra increment and universal-candidate claim are
rejected. No serializer or blank-validity rule was implemented from it.
Corrected paper assessment: `resumed-20261104/
r3-universal-cadence-assessment/PARENT-QUALIFICATION.md`.

## Frozen retained-byte/token hypothesis

A private causal model now passes56 selected actual-T80 controls:40 complete
192-clock fixed-mode windows (including genuine Mode3) and16 ordinary/JIT
mixed-mode cases on both types. It retains the original byte and a saturating
cursor, queues the existing mode event to an endogenous phase edge, and
replays destination cadence from the retained four-M2 anchor. Token epoch
validity preserves the ordinary old quarter while delaying premature JIT
blank release. This is an abstract implementation hypothesis, not a unique
physical-stage claim. No production GA code has changed.

The event/tag/validity policy and all model sources were frozen before one
build and one56-case execution. All8,352 visible RGB-clock constraints,
original-byte/cursor/mode/pen/ink provenance and mixed blank intervals pass.
Forty-six prior CPU/READY/CRTC/sync/IRQ/fetch/VRAM streams are byte-identical;
the ten new ordinary5D/A6 cases have focused baseline/source qualification.
For the new ordinary2→0 baseline, Type0 loses the old quarter while Type1
already passes; neither result is manufactured to fit the candidate.

One fixed-JIT analysis guard initially compared R2 with the R3 payload14
instead of the preserved R2=46. Original analysis/logs are retained; the
read-only correction uses the same56 completed traces without rebuilding,
rerunning or changing model/oracles. Parent read the full policy, source/interface
delta and analyzer, and verified all initial source and238 artifact hashes.

Current-core asset-free proof is the next bounded step. Independent review,
same-numeric-mode JIT, reset-free SNA lifecycle, adjacent phases and native
acceptance remain open. Fixed controls use an unblanked recipe and do not
cover the same-mode JIT validity question. Evidence: `resumed-20261104/
r3-retained-token-prototype/` and `r3-production-integration-plan/`.

## Current-source baseline and candidate equivalence — 5 November

A separate private fixture `r3-current-source-baseline` independently qualifies the current-core
T80 environment, including the reviewed `9de75d3` reset sequence and synthetic owned RAM/guest.
The baseline yields 44 control passes and 12 source-derived failing expectations; all 40 fixed
192-clock cases pass, including Mode 3. Against this qualified baseline, the candidate frozen
policy from `r3-retained-token-prototype` executes all 56/56 PASS, satisfying 8,352 visible RGB
clocks along with byte, cursor, mode, and pen provenance and full blanking intervals.

Strict candidate-vs-current-baseline verification compares 150,472 rows across 43 machine and
bus fields, producing 6,470,296 equal comparisons; the unchanged-video shadow output is also
strictly equal. The earlier frozen CPU prototype's early IORQ, VRAM, and IRQ differences remain
preserved for diagnostic reference only; no full-stream equality with the legacy CPU model is
claimed. Root reviewed handoffs and verified 49 source and 353 baseline artifact hashes, alongside
49 source and 312 candidate artifact hashes.

## Fast GA regression qualification and clean candidate scope

In the existing fast GA suite `r3-current-fast-ga-regression`, the Type 0 old-quarter French
required expectation is red before and passes with the candidate, completing 27 PASS receipts.
A previously unqualified initial Mode 2 premise in the narrow R3 restart fixture was addressed
by `r3-fast-qualified-mode-prefix`: this strengthens only the setup sequence via public bus
wide R3 writes and authentic `MODE_SYNC`, restoring the narrow R3 stimulus and waiting two fresh line seams to retire
the wide pulse while leaving the target body unchanged.

Original baseline and candidate each ran once with byte-identical logs, producing the 17 remaining
PASS receipts across mode transitions, restart, non-JIT controls, and context. Absolute target
phase, master tick, and IRQ match on both. No failure assertion was weakened, and no model fitting
was performed. Root verified 29 source / 58 artifact hashes for fast GA, and 28 source / 56
artifact hashes for the prefix suite; the first 27 PASS receipts were reused.

The completed private `r3-production-clean-candidate/` removes eight trace-only
registers (74 declared bits), moves the causal monitor and unchanged-video
shadow into the simulation top, and restores original copyright/ACCC attribution.
Functional policy is unchanged. One execution preserves every captured field:
150,472 rows × 78 columns = 11,736,816 equal comparisons, with all 56 unchanged
French controls passing. Parent read the proposed video/GA deltas and verified
52 source and 305 artifact hashes. Fast receipts remain valid through exact
functional projection and unchanged observation interfaces; no rerun was needed.
Completed Gemini 3.8 review is supplementary; parent rejects unsupported
area/timing/reset guarantees and does not accept it as hard sign-off. No production GA edit
or integration is accepted; reset-free SNA/lifecycle and adjacent phases remain
open.

## Same numeric mode under genuine R3.JIT

The fixed-mode controls above use an unblanked recipe. Eight additional controls
in `r3-same-mode-jit-controls/` instead execute the genuine width-shortening
caller with old and requested mode equal: Modes0–3 on both CRTC types. The
actual T80 writes R3=2 during active HS/C3=2, from initial R3=14/R2=20, with the
same accepted-write, READY and six marker-LOAD guards. The frozen candidate and
seed remain unchanged; no old trace is claimed equal for this new stimulus.

French §9.3.4 pp56–57 gives the R3.JIT colourless interval and fixed-mode pixel
cadence independently of a numeric mode change. Parent inspected the rendered
French p57. Public RGB expectations combine the independently qualified fixed
192-clock byte sequence with the JIT black interval [715,859) on Type0 and
[719,859) on Type1. These absolute bounds belong to the qualified fixture;
they are not new measurements of physical pipeline latency. No cursor/tag or
replay implementation state supplies expected output.

One build and eight-case execution pass all 1,536 public RGB constraints and
complete black intervals, including true Mode3, with zero mismatch. Commands:
`bash local/overnight-20261103/resumed-20261104/r3-same-mode-jit-controls/build.sh`,
then its `candidate-obj/proof` with `CREDIT_VECTOR_TRACE_DIR` pointing to its
`traces/`, then `python3 local/overnight-20261103/resumed-20261104/r3-same-mode-jit-controls/analyze.py`.
Final line: `SAME_NUMERIC_GENUINE_R3_JIT: PASS 8 cases;1536 publicRGB clocks;candidate unchanged`.
Sources match their pre-execution freeze. This resolves the same-numeric concern
at the tested phase; adjacent phases and lifecycle transport are still unproved.

## Private snapshot lifecycle red/green

Public-GA-pin tests expose two normalization defects before production adoption.
Without HSYNC, restored Modes1–3 still render as Mode0 after128µs (6/8 cases).
A private internal reset-mode input uses the settled SNA header on apply and
zero otherwise, following the engineering convention already used by Plus.
This normalizes unrepresented state; it is not a physical reset/hidden-mode claim.

A separate real-GA fetch/serializer test fails7/64 apply phases: clearing only
the inner byte leaves a pre-apply VIDEO_BUF available for the next LOAD, leaking
28 old RGB samples each. A functional prefetch-valid flag clears on reset/apply,
sets on an actual later fetch, and qualifies the inner LOAD. A fetch coincident
with apply is discarded; a partial initial byte may remain black while fresh
ownership is acquired, but fresh-data recovery is required.

All64 unchanged stale-byte controls and all8 restored-mode controls pass on
final private sources. Red expectations existed before each correction; no
phase/palette/output rule was fit. Original queue/cursor/epoch policy is unchanged.
Evidence: resumed-20261104/r3-sna-mode-contract/ and r3-sna-stale-byte/. Separate
64-case red/green traces are preserved; initial8mode-redCSVs were overwritten
by first mode-only green, with red.log/sourcefreeze retained.

The final SNA-corrected source passes all56 original controls and all8 genuine
same-mode JIT controls:9,888 public RGB clocks/full blank. All171,968 captured
rows and78 fields (13,413,504 comparisons) are byte-identical to the original
controls; source hashes remain frozen. Evidence is r3-sna-normalized-matrix/
and r3-sna-normalized-jit8/. Stronger review and retained production tests remain
required. No production GA code, selected gate or RBF has changed.


## Retained-test draft qualification

The asset-free test draft now wraps the existing B18 motherboard/T80 fixture
and generates its own synthetic RAM/header/guest. Its unchanged original56
public RGB/blank controls produce11 baseline reds; the older12th provenance
failure is a same-ink palette alias and is not a public-pixel red. Eight added
same-numeric R3.JIT controls fail on production; all64 pass on the final private
candidate with actual CPU/RMR/CRTC/READY/setup guards. Matching error counts
alone do not establish full machine-input trace equality.

The initial file-wide missing-pin waiver is rejected for retention. A private
extension of the existing generated-motherboard adapter explicitly opens five
verified unused outputs (CPU BUSAK, GA ROM, PSG ACTIVE/IOA/IOB). Strict fatal
PINMISSING lint passes across the same elaborated test hierarchy without any
waiver; no runtime repeat was needed for empty output connections. Parent read
the delta, verified16 input/artifact hashes and independently confirmed cached
T80 token equality after removing comments. The proposed patch text needs a
comment-format repair before application; neither that patch nor the test has
been integrated. This is routine interface qualification, not strong approval
of the serializer. Evidence: r3-retained-final-candidate/ and
r3-retained-pin-qualification/. Stronger GA review remains pending.
