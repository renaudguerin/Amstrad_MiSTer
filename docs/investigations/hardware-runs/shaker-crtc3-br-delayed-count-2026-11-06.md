# CRTC3 BR: private delayed-count candidate

The original resident thirteen-row replay now produces twelve CC controls and
CPI C4, with the unchanged analyzer qualifying source, loader, copy, CPU/PPI,
stack and complete RAM-write reconstruction. The same candidate on the current
source composition produces byte-identical trace, stack, store, RAM and loader
ledgers. This is a private candidate, not a deployed fix or hardware verdict.

## One delay, different ownership

The accepted model counts raw HSYNC and delays compatible delivery. The
candidate instead uses the existing PRI one-character HSYNC history for normal
R52 counting and prompts ordinary52-wrap delivery. It preserves raw monitor/VS
shaping, CPU/IC116, held source-qualified ACK clearing and DMA retention.
Non-normal VS/MRER delivery remains unchanged: that bounded split is explicitly
marked MODEL LIMIT(B26), not a proven shared physical phase law. No fitted
numerical delay or instruction classification was introduced.

The alternative was independently open in PA7 V5 and DMA H1 constraints before
this BR analysis. Local ordering predicts all twelve controls and CPI: the
delayed count falls within held clear for eight controls, before ACK for four,
and after ACK release for CPI. Clear stays active through the gap between
split shaped pulses; treating only one strobe would misclassify the eight.

## Actual causal receipts

One candidate run on the qualified older composition passes all13 stores.
CPI's delayed line event at55481290 sees31 with no held clear and yields32.
The next ordinary wrap at55563210 follows20 line events /81920master ticks;
51 becomes0. Actual stacked PCs are4A9B/C4ED, compared with the accepted
before4A9B/CCED. The original program and oracle were not changed.

An earlier measured-slot anchor is4096ticks (one64-character line) earlier,
with C9=0 instead of1; other compared architectural/geometry fields match there.
This is a material earlier phase effect, not unchanged-prehistory validation.
The measured post-ACK count and20-line interval support the proposed local
mechanism despite that earlier change.

A separate current-composition compile/replay closes the old classic/status
source gap: all thirteen stores and transport pass, and its complete trace,
frames, stores, initial/final RAM and loader receipts equal the first candidate.
The two runs have the same55,300,783 post-origin ticks. No baseline was rerun.

## Remaining gates

Focused existing controls have completed without changing assertions.
All six original V5 screens30–35 match the physical endpoint/count table, and
all68 photographed DMA H1 records match. These constrain delivered behaviour;
they do not locate the internal count stage. PRI pr01–13 pass; pr14 stops on
its old raw-relative delivery bound. GA
differential d02/r02 stop on raw classic-lockstep phase, and snapshot B3=51
stops on its old next-raw-line deadline. Those are recorded failures, not waived
gates: source/model expectations and actual external outcomes must be reviewed
separately. The latter aborts leave later subcases unexecuted, including VS/MRER
and snapshot pending/ACK combinations.

Fresh cross-provider semantic review, current-tip final gates, enabled full fit
and native original BR/IRQ/DMA preservation remain before adoption. The
original-Plus MRER-between-edges discriminator and counter-clock alternatives
remain in the [ACK investigation](shaker-crtc3-br-ack-boundary-2026-11-06.md).

Private packets under`local/overnight-20261103/resumed-20261104/`:
`c3-br-ack-constraint-judgment-20261106/`,
`c3-br-delayed-count-proof-20261106/` (132 final artifact hashes, original
unchanged analyzer and supplemental receipts), and
`c3-br-current-composition-gates-20261106/`. The current native SHAKER tally
stays unchanged; accepted cb3a1a6 still has BR01F0/01F1 broken.
