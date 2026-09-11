# D1 and D6 parity repair

Accuracy work on `codex/accuracy/d1-d6-parity`, based on
`a248d99a937d129bcee5f2c0cc4e6f727fa8e16e`. The two repairs are sequential in
one checkout. The [first diagnosis](../hardware-diagnosis-2026-09-10.md) and
[second pass](../hardware-diagnosis-2026-09-10-second-pass.md) describe the
original source and hardware evidence; this document records the repair.

## D1: incoming parity owns origin VSYNC

French ACCC v1.11 §19.7.2 p.219 assigns MID-VSYNC to even ParityFrame on
CRTC 0 and 1, for R8=1 and R8=3. At R7=0, parity management precedes the
C4/R7 comparison. The French page was visually checked; the corresponding
English §19.7.2 agrees. Type 0 snapshots ParityR6 at the origin (§19.5.2
p.206); type 1 also changes parity through its R8 stage machine (§19.5.3
p.209). Neither an unconditional inversion in the wrapper nor the legacy
`field` flop represents both canonical transitions.

The wrapper now uses the selected engine's parity write/value on the edge
which consumes that transition. Type 0's additional line can end a frame
without the ordinary line-last comparison, so its canonical origin event
supplies the R7=0 comparison. The type-0 C0=2 qualification and partial-line
holdoff use the wrapper's count phase. A pulse already active retains its
count phase across frame origins and R8 writes. The existing field lifecycle
on exit from interlace is retained.

French §16.4 p.170 permits R7 updates through the last character before
C4=R7. An R7 write on the origin edge must participate in that comparison;
using the old register can miss an armed pulse or emit a cancelled one.

### Deterministic evidence

- Four origin vectors (both types, R7=0, R8=3, R9=7/8) failed on unchanged
  RTL, while all 192 existing classic vectors passed.
- The repaired suite checks one pulse per complete frame, odd seam/even
  midpoint, nonzero-R7 controls, R8=1, pulses crossing an origin, bus-driven
  IVM entry, and active pulses through R8=1/0/3 changes.
- R7 arm/cancel vectors cover both CLKEN and nCLKEN at the preceding
  character. The four exact-CLKEN cases failed before the origin comparison
  was made write-aware; all eight cases then passed.
- Existing snapshot/type-switch qualification vectors remain required.
  Their setups now enter an even frame, as §19.7.2 specifies. The t02i
  fixture has R6>R4, so §19.5.2 freezes even parity: the midpoint remains
  fixed across origins. Its before/on-count-edge end times are derived as
  1+16 and 16 characters, replacing expectations based on alternating FIELD.
- Current combined classic result: **225 required passes**. D1 adds 31
  cases and D6 adds two; the 192 existing cases remain required.
- Full `make -C sim` and lint pass, including the production GA/CPU-phase,
  motherboard FIELD, Plus and FDC gates (the existing payload-poll XFAIL
  remains). The combined D1/D6 soak is `0xb1cb70da95c2e44f`: intended VSYNC behavior
  changes and corrected RFD saves, plus `vsync_mid_arm` and `vsync_active_mid` added to the sampled
  state. The seed and randomized stimulus are unchanged. The 2,845,088-sample
  hash was reproduced with `SOAK_EXPECT=b1cb70da95c2e44f`.

Local red/green logs are retained under the ignored
`docs/references/d1-d6-parity-2026-09-11/` directory. These tests drive CRTC
register traffic; they do not execute SHAKER. The aggregate gate separately
exercises the production GA/CRTC bus-phase fixture.

### Review and hardware boundary

Opus 5 high completed source review (`20260910T233958Z-37136-35ad`),
confirming canonical parity, polarity, origin R7 priority and snapshot pulse
coherence. It did not run simulation and could not inspect the entire base diff
because its output was truncated. Its findings were handled as follows:

- An overlapping pulse could count on its retained midpoint phase while
  consuming the next pulse's seam predicate early. A failing type-0 vector
  reproduced this; both types now check the terminal tick and subsequent
  pulse phase. New starts require their own phase to match the count tick.
- A shortened additional line could block the origin comparison without
  consuming it, permitting a later equal-R7 write to fire. A failing vector
  shortens only that extra line to R0=1, then widens the origin line and
  writes R7=0. Origin generation and blocked consumption now share the match.
- The delayed half-count state remains for the legacy R8-exit lifecycle;
  its comment no longer claims it starts an ordinary odd IVM pulse.
- Fixture comments distinguish source parity rules from model reset choices;
  the half-count fixture asserts canonical parity rather than FIELD.

**Fresh cross-provider review: CLEAR for source commit `988f5b9`.**
Gemini 3.8 Flash high completed the D1 follow-up and D6 review in run
`20260911T012004Z-97138-7451`, exiting normally with status 0. It found no
actionable defects in the scoped diff, including pulse phase qualification,
additional-line blocked consumption, D6 parity polarity, test expectations
and the repeat-per-frame ON/OFF interpretation. No further Opus call was used.

This is source review, not independent execution of the gates. The reviewer
read the French position-inspector extracts and reported no truncated input;
it relied on the parent's existing simulation/lint/soak results. Its broad
claim of complete correctness is not hardware evidence. One report statement
was corrected by the parent against RTL: `vsync_mid_arm` clears on reset or
snapshot load, not directly on a live type change. It is shared wrapper
state; type changes select the consuming engine, while seam/count events
update the arm. The report is not evidence of a type-change clear.
The raw report and brief are retained in the ignored evidence directory as
`gemini-review.md` and `gemini-review-brief.txt`.

The legacy R8=0 FIELD lifecycle and raw-R8 versus latched-IVM transition
window remain retained integration assumptions, not newly established
hardware rules. Source review and local gates are complete. Hardware acceptance remains
pending.

### Integration provenance

Accepted source tip `2d04812a30dbbeee3c8fa5aa83408c30dcef51ea` was rebased
onto master `b817977db73cbb51fd22d0368bbca202324b6851`. The intervening
master changes are D5 investigation documents only. The rebase conflict in
`docs/current-status.md` was resolved by preserving both status records.
Reviewed implementation `988f5b9` corresponds to rebased commit `7685325`;
the final RTL, simulation files and hash contract are byte-identical to the
reviewed source. Integration reconciles documentation only. The merged
checkout passes the aggregate simulation (225 classic vectors), lint and
expected-hash soak `b1cb70da95c2e44f`; the existing FDC XFAIL remains. CI and synthesis are intentionally skipped
for this integration push at the user’s request; further branches will be
integrated immediately afterward. The merge uses `[skip ci]`. A subsequent
integration must supply CI/RBF evidence before hardware retesting; no new
bitstream is claimed here.

No new RBF or hardware result is claimed. Retest SHAKER B (9), both screens,
on CRTC 1 and CRTC 0 in Full sync, preserving the nonzero-R7 controls and
comparing against the corresponding real-CPC reference photographs. DSC4
remains an independent hardware holdout.

## D6: RFD parity source

French §11.6.1 p.90 explicitly makes odd parity fail the C9=R9 test;
even parity permits the save. Section 19.5.3 pp.209-210 shares ParityC9
with IVM and describes the ON/OFF operation on even C9 with odd R9. The
engine now gates RFD saves with `~parity_c9`; the private `rfd_frame_parity`
flop and its reset/origin updates are removed. The RFD source and management
flags retain their existing lifecycles.

Two new vectors start from even and odd frames. Each repeats RFD followed by
R8=3 / R8=0 on C9=2, with R9=3, over four frames. The paper expectation is
VMA'=base+(row+1)*R1 at every row save and the same value at the next row's
MA reload. Both fail with the original private flop and pass with shared
parity. A subsequent frame omits ON/OFF and verifies that the odd frame
suppresses saves and consumes the preceding frame's retained VMA'.

This deliberately qualifies the requested “every later frame” expectation:
§19.5.3 still toggles parity at every origin. A single ON/OFF operation is
not a permanent even-parity lock. The repeat-per-frame vector tests the
source-supported normalization recipe; the no-ON/OFF control preserves the
free-running alternation. The prose around §11.6.2 is not treated as authority
for inventing a permanent lock or changing the RFD flag lifecycle.

`t13b` now advances one complete 32-character fixture frame before arming
RFD, asserting odd ParityC9 for case 1 and even ParityC9 for case 2. Three
other source-state fixtures likewise enter an odd frame. In `t13n`, this
preparatory even frame saves base+4*R1=0x1244; after source disarm the observed
MA at C0=1 is therefore 0x1245, rather than the former reset-state 0x0001.
These expectations are derived from the configured geometry and §11.6.1,
not copied from simulator output. Private-flop assertions were removed;
the canonical parity assertions and address consequences remain.

Hardware discriminator: SHAKER C (4), CRTC 1. No hardware result is claimed.
