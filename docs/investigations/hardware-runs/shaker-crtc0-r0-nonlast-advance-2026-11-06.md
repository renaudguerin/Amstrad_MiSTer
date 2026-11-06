# Type 0 row advance and R6 consumers

The experimental source corrects two connected CRTC0 defects. Widening R0=1
on a raster-terminal, non-last frame row now advances C4 without entering
frame adjustment. Display border and ParityR6 consume that actual counter
commit rather than assuming C4 only changes at an ordinary row end.

The public suite passes **253/253 CRTC tests and 46 selected benches**. The
combined implementation preserves all three qualified original CPU trajectories;
its original B6 replay qualifies46/46 with the same six results and all ten
outputs byte-identical. Root verifies all271 combined artifacts and56 additive
R4 artifacts. Fresh Opus review permits an experimental build; historical T80
harness wiring and additional R6 route proofs now pass. The final selected
gate passes46 benches; supplemental Opus review also permits the experimental build. Targeted native A4/AI acceptance now preserves01/00 and corrects the sole
lower mismatch. The earlier seven-failure claim was an E/F transcription
error; [correction](shaker-crtc0-a4-native-glyph-correction-2026-11-06.md).
A4 is unknown because extraC9sweep remains ungraded; other native controls
are completing. No whole-page pass follows from source proof alone.

## Source-derived counter contract

Root reads the literal French ACCC1.11 §13.7.2.1 p127 figure and text. After
the specified59→1→63 sequence, C4 advances6→7 at C0=2 when C9=R9, even if
C4 differs from R4. C9 is retained. The non-last case then resumes ordinary
counting; §13.7.2.2 p128 separately owns continued adjustment on a true last
frame line. These are outcome constraints, not an identified silicon latch.

| Case, R4/R9 | Baseline at C0=2 | Corrected | Derived first widened end |
|---|---|---|---|
| True last6/7 |2/7/7|2/7/7|0/7/8|
| Non-last8/7 |2/6/7|2/7/7|0/8/0|
| C9-negative8/8 |2/6/7|2/6/7|0/6/8|
| Adjacent R4=7, R9=7 |2/6/7|2/7/7|0/8/0|

Tuples are C0/C4/C9. The figure directly specifies the first counter transition;
the later ordinary end is derived from the accompanying counting rule. The
three canonical production-GHDL-T80/GA/PPI/READY guests execute literal
ED51/NOP/ED59 with OUTs at7990/7995µs. All126 transport guards qualify. The
first corrected counter difference is exactly tick10704266; one added C4 step
persists over9,088 captured points, with C0/C9 unchanged. Both neighboring
controls' pins, events, I/O, fetch and counter ledgers are byte-identical.
The new row-only seam has no line/row-new collision or adjustment entry.

The additive R4=7 guest differs from the original non-last input by one operand
byte,8→7. Baseline and candidate each execute once, qualifying42 guards plus
explicit R4 binding. The corrected trajectory remains ordinary through the
first widened end. This does not settle later frame length or original-chip
behavior when the new row lands on R4.

The initial39/40 qualification stop remains preserved. Its additive
source-supported OUT-spacing qualifier neither changes that verdict nor fits
NOP/HALT M1 phases. Root verifies all174 baseline proof artifacts, all275
canonical candidate artifacts and their source bindings. The separate R4=7
packet is frozen independently.

## R6 consumer contract and implementation

Opus review found that a mid-line C4 change bypassed the R6 display and parity
consumers. Root verifies French §18.1/§18.2.2 p189: outside the first-line
conflict, reaching R6 asserts a definitive border, regardless of C0/C9, including
adjustment. French §19.5.2 p206 separately captures ParityFrame xor1 when C4
reaches R6, independently of R8.

Three retained tests protect distinct interactions:

- **t35b:** a non-last C4 advance and subsequent ordinary carry, with a
  C9-negative control. The qualified baseline has246 old passes and this new
  failure; the counter-only candidate passes247/247.
- **t35c:** a visible frame reaches C4=6/C9=7 and widens to produce the7 landing.
  Unreached R6=9 keeps DE high and ordinary R6=8 closes DE. R6=7 incorrectly
  left DE high on the next full line, after C4 had passed the match.
- **t35d:** the same natural setup observes the missing ParityR6 capture through
  its transfer at the next actual frame origin. Unreached/ordinary controls
  pass; the mid-line arm incorrectly retains odd parity instead of even.

The original two R6 tests fail before correction:247 passes and two failures
in that249-test cohort. The expanded route suite described below passes253/253. They share setup, use existing DE and parity
observations, and add no state injection or test-only production interface.

The wrapper now makes one combinational C4 write decision and commits it in
one flop assignment. Ordinary row_new retains priority over frozen/widening
increments; both engines already supply zero on frame_new. Branch-specific
management, field and origin side effects keep their old ordering. A shared
stored-R6 landing feeds DE and one internal Type0-engine input for parity.
There are no new state bits or top-level ports. Parity retains its pre-edge
!in_adj and ordinary-origin policy; Type1 and the existing R6=0 conflict retain
their consumers and priority. R4/R7 events are not synthesized from this commit.

Four further retained checks cover frozen R0=0 and true-last widening,
with separate DE/parity outcomes and higher-unreached R6 controls. On the
counter-only baseline all six consumer tests fail for the predicted reason;
on the candidate all253 tests pass. The frozen R8=1 case observes stale
ParityR6 inserting an extra line, independently linking the missed capture to
frame length. The non-last DE test also checks the immediate C0=2 result.
No setup guard failed and no oracle changed after execution. The historical
pre-B8 engine adapter now accepts the added unused input; its production-T80
negative control finds all four required B8 failures and exits0.

## Validation and preserved evidence

The exact final command is `python3 sim/select_tests.py --run`; its final line
reports PASS46 benches. The final `c0-r6-extra-routes-20261106/PARENT-GATE.json`, source hashes
and complete log bind253/253 and PASS46; the earlier immutable
`PUBLIC-GATES.json` separately binds the original249-test composition. No gate is rerun by the reviewer.

The fresh fixed-seed soak gives974e5c048eb7f877 with unchanged seed, stimulus,
sampled projection and2,845,088 CLKEN samples. Private controls isolate the
changes: restoring only old admission returns the original31f3de4cf3288d9c in
the counter-only composition; restoring only old R6 consumers returns its
intermediate9d080f4f4bfda9ff even while retaining the new common counter mux.
This attributes the observed changes to intended behavior under the fixed seed,
without claiming universal equivalence.

The unchanged original C0 B6 baseline qualifies46/46 and gives
27/14/26/27/1F/1F. The counter-only candidate preserves all ten full output
artifacts byte-for-byte; the combined consumer replay also preserves all ten
outputs with46/46 guards and the same clocks and values. B6 never stores R0=1, so it
protects shared owners, zero propagation and IRQ-driven results rather than
the new admission itself. Its normal architectural CRTC values are seeded and
survive CPU/counter reset; this is a cold original-module diagnostic, not
firmware/FDC boot.

Two transport mistakes remain recorded separately. The first leaf input crossed
C0=1 while waiting for nCLKEN; it stopped before the outcome. Correct scheduling
follows the bench's phase convention and now checks C0=1 after the write; no
expected value changed. An initial public selection reused a generated baseline
model after a file copy retained older timestamps. Its metadata/log/soak stay
unqualified. Touching only RTL timestamps with unchanged hashes forced fresh
Verilator generation. The final combined build uses normal edits and fresh
private model directories. An optional trace-symbol identity helper also failed
because those private builds omit tracing; frozen inputs and generated metadata
qualify identity without a DUT retry.

## Model limits and hardware gate

Fresh Opus review of the combined diff permits an experimental full build and
finds no RTL composition defect. It catches historical negative-control port
wiring and missing focused frozen/true-last R6 route coverage. Those follow-ups
now pass; supplemental independent review also permits synthesis. No master/release or whole-page promotion
is authorized; original-hardware R4/R7 questions remain open.

The widening admission is still restricted to !in_adj and R5=R8=0. Applying the
literal early-OUT result to other pre-CLKEN writes, including late OUTI, remains
an inferred extension. Direct first-CLKEN non-last admission and the old-one
horizontal deadline retain their previous model. An off-seam numeric C4=C9=0
landing preserves the legacy display/parity policy pending evidence; it does
not manufacture a frame origin. Ordinary zero/IVM behavior is unchanged.

Landing on R7 has unresolved VSYNC/validation consequences. Landing on R4 now
has a qualified local counter probe but no complete original-hardware frame
verdict. These limits are named in MODEL LIMIT(B26) comments. Before interpreting
native A4 changes, bind R4/R6/R7 at each row-only firing. No onset or frame
rule is guessed to make a screenshot pass. Full synthesis/timing, targeted
native controls and physical HDMI/CRT acceptance remain separate gates.

Private packets under the active session evidence root:
`c0-r0-p127-readiness-20261106/`, `c0-r0-p127-candidate-20261106/`,
`c0-r0-p127-r4-adjacent-20261106/`, `c0-r6-offseam-readiness-20261106/`,
`c0-r6-commit-integration-20261106/` and the two Opus review packets.

Supplemental review records two competing French readings, verified by parent
from the rendered pages. At R6=R4+1, §19.5.2 p206's actual-C4-event reading
permits parity capture, while §19.6.1 p217's literal R6>R4 freeze forbids it.
The true-last test pins the former as an experimental model choice, not a
physical verdict. At frozen R0=0, §13.2.6 p110's bullet says C4 increments
regardless of R4, but its summary literally gives R4+1. The existing counter
and new consumer tests retain C4+1; no counter rule changed in this consumer
repair. Both questions are recorded in the unsent French list and B26 inventory.

The493cdea original A4 resident replay also qualifies33/33 unchanged guards,
retains the repaired upper01/00 pair and binds all16lower widening onsets.
None of those baseline/candidate onsets has C4+1 equal to R4/R6/R7. Private
lower PB0 pairs become01/01/01/11/01/00/01/00, changing only row4. The previous description of native eightFFFF pairs was a transcription
error. Correct native baseline is FEFF×8, candidate changes onlyrow4 toFFFF.
Private/native PB0 differences remain only in second bytes of rows6/8; this
is not hidden by an upper-PPI allowance. The resident diagnostic is not
firmware/native history reproduction, so lower values remain classification
and prediction only. Exact493 native comparison remains required. Parent
verifies all85 packet artifacts against manifest13ea6f48b871c355d26a4690754c17feb7bb24a6477833a297d8dbf6df8cb744.

Targeted493 hardware preservation completes with35viewed PNGs and41actual
Full0 SSM records across A4/AI,B6/B9,C5,A2 onC0 and A4/AI,B6/B9 onC1.
All14 C1 images are byte-identical to8ba384e; stable C0 B6/C5/B9 markers are
also identical. Three asynchronous A2/C5/B9 samples vary in acquisition and
remain ungraded. All12writtenCFGs have normalWAIT; allsixrestores match.
Scoped C0 DSC4 credits/first/later main are byte-identical; the otter and both
Amazing logo/text samples remain visible with animation-phase variation.
Demo Full evidence is requested/CFG only, separate from the41observed SHAKER
records. No whole-demo or physicalCRT model claim follows.
