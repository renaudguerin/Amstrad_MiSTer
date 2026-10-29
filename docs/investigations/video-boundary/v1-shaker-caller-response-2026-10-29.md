# V1: executing SHAKER T/Y and an independent monitor candidate

## Position and evidence boundary

The production motherboard/CRTC/GA with production GHDL-generated T80 now
executes the actual SHAKE27A T/A and Y/A callers, reaches their SSM emitters,
and exports native RGB, raw CRTC pins, GA monitor pins and Full/selected tuples.
The first native rasters contain the expected readable labels and black bands.
This removes the synthetic-program limitation of the B22 width probe for these
two source specimens. It does not establish original-monitor fidelity.

The diagnostic uses ideal physical RAM and a cold Module-A entry. A reset ROM
trampoline runs `DI; JP 4042`; the original lower-ROM font remains intact.
Production keyboard scanning selects T or Y from the real menu. Module startup,
bank-copy helpers, drawing, VSYNC waits and test loops execute normally. Earlier
Module-A navigation and production SDRAM arbitration are absent. Preserve this
distinction when comparing the photographs or requesting a hardware replay.

All harnesses, payloads, traces and rendered images remain ignored under
`local/investigations/sync-monitor-response-2026-10-29-f4da/continuation/`.
The active scratch directory is `output_files/v1-shaker-exec-2026-10-29/`.
The source/monitor probes remain ignored. V1-R1 below adds the narrow production
CRTC-to-GA restart contract and its retained integration regression; the monitor
filter and menu are unchanged.

## Caller and page identity

Input DSK SHA-256:
`65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b`.
SHAKE27A payload SHA-256:
`ab8a7cb64bbf7149dd71a3958216fa84274636bb7b13c1e9f6a15527706c8a8d`.
The 27,004-byte payload loads at `3D00`, ending at **A67C exclusive**; its
AMSDOS execution address is `4042`. Original extraction and disassembly are
retained alongside the source-verification addendum.

T dispatch at `4152` selects table `42A4` using keyboard line 6 bit 3.
Its handlers are `9835, 9842, 9849, 9850, 9857, 985E`, setting the intermediate
R2 value to `11..16`. Y dispatch at `415C` selects table `42B2` using line 5 bit 3.
Its handlers are `9973, 997C, 9980, 9984, 9988, 998C, 9990, 9994`, setting the
intermediate R3 value to `0..7`.

Startup copies `3D00..3FDD` to `B000..B2DD`. The first T and Y handlers arm
the page countdown at `B284` and `B2AE`. Routine `A636` decrements the count,
patches the SSM operands at `A658/A65A` and advances the table cursor. Therefore
T maps to `00BD..00C2`, and Y to `00C4..00CA, 03AC`, in handler order. The
leading word 3 is a frame countdown, not a record type. Page `00C3` has a
one-frame countdown and is outside these handler lists.

The first observed instruction-fetch prefix at `A657` identifies the emitter;
it is **not** the completion timestamp of the two SSM instructions. Execution
continues for a whole 1,277,952-tick field after that prefix.

| Cold source | First caller tick | First SSM prefix tick / code | End tick |
|---|---:|---|---:|
| T/A | 16,078,279 | 42,716,151 / `00BD` | 43,994,104 |
| Y/A | 16,079,047 | 41,438,135 / `00C4` | 42,716,088 |

Ticks are 64 MHz master ticks relative to the post-load/reset execution origin.
The applied mode is observed as Raw pixels (1), and SHIFT remains zero.
Raw native RGB is additionally plotted under the observed Full acquisition
rectangle; that illustrative raster is not the converter/mixer/scaler output.

Startup enables lower ROM and copies `3900..3BFF` into font buffer
`A67B..A97A`. The copied 768 bytes exactly match the supplied French lower ROM,
SHA-256 `2e180a06dd0abfc0dc3653271be7832e201a204f5c188a61cc6d9c38b0f17d4b`.
`BC00/BC18` are installed bank-copy stubs, not firmware services. An empty RAM
font or direct jump into a post-draw loop is not an equivalent source probe.

## Source waveform

T/A has 7,167 complete GA HS pulses of 256 ticks (4 µs). Y/A has 6,503 of
256 ticks and 352 of 142 ticks (2.21875 µs). Each contains 352 occurrences
of each 1,856/2,240-tick rising-edge interval (29/35 µs), summing to the normal
4,096-tick line. Those counts cover the entire retained caller window, including
setup, not just one stable image. The final active pulse is incomplete and is
excluded from widths.

For the last complete observed Full field, each specimen has an 88-line
two-pulse band. Native RGB under the Full rectangle has straight band edges;
the original CPC/CTM photographs show a curved entry and bent trailing hatch
rows. This is a suitable source signal for testing a monitor-response hypothesis,
without attributing the missing deflection to the CRTC or altering its pins.

The CSV is a pre-edge 64 MHz value-change stream. RGB/sync/SHIFT/blanking values
hold until the next row. PC, counters and `ce16_at_row` are point observations
at that row, not independently compressed continuous signals. Fetch records
are also pre-edge: the byte register may hold the preceding fetch. Do not infer
a failed RAM transaction from a same-row word/byte comparison without its
production pipeline delay.

## CPCSyntaxError comparison

Read-only source pin: `ba477b84496e7399ff7c001bbf86863e49cf3eca` in the
user-supplied CPCSyntaxError checkout. `src/core/monitor_renderer.{h,cpp}`
implements a concrete candidate with one composite-sync input, deferred windows
straddling flyback, nearest-tip line selection, per-tip width, asymmetric
early/late response, slow AFC, width settling and separate sync-train behavior.
`monitor_model.{h,cpp}` distinguishes set identities while sharing horizontal
behavior where its source has no characterization.

Its comments explicitly fit many constants to SHAKER photographs and sometimes
AmSpirit: D3/D4 displacement, taper bounds, early/late gain, settling, drift and
train decay. AT and AY already informed its calibration. These pages therefore
cannot be claimed as held-out validation of this candidate, even though this
replay supplies an independent MiSTer source stream. Some comments describe
unmeasured monitor families sharing a value; that is an assumption, not evidence
that the sets respond identically.

The cited `tools/monitor_response.py` and `docs/LA7800.pdf` are absent from this
checkout. The measurement procedure remains unavailable. An independent
[primary circuit check](v1-ctm-afc-circuit-check-2026-10-29.md) recovers the Sanyo
datasheet and CTM644 service drawing: the sample has 4.7 kΩ, the CTM has 8.2 kΩ,
and R·C is not the closed-loop settling time. The candidate's 73-line constant
remains an empirical hypothesis; do not replace it with 128 lines merely from
the CTM component product.

An ignored adapter compiles the supplied `monitor_renderer.cpp` **in place**,
with no modified constants. It feeds active composite sync as GA HS XOR GA VS,
matching that implementation's `GateArray::csyncActive()`. The 64 MHz source
is sampled at each of four possible 16 MHz phases; both pulse edges are retained
in the input. Model state begins at the first caller, not at machine reset.

For each last complete field, all four sample phases yield 312 decisions:
219 ordinary line decisions, five vertical decisions and 88 two-tip discriminator
decisions. T/A pulls 10 Pixel-M2 on every band decision. Y/A pulls 10 on the
first two band lines, then 5 on the remaining 86. Both hold a 1,024 Pixel-M2
line, with zero slow term and no re-lock snap. The sampled Y width is 35 or 36
Pixel-M2 according to phase; the branch and pull results are unchanged.

The adapter's illustrative raster uses model flyback/line offset with unchanged
native RGB and a fixed crop. It produces the expected direction of band-edge
curvature and trailing hatch distortion. It does not reproduce CPCSyntaxError's
complete renderer, CRT transform, clipping or snap/refile path, and this visual
agreement is not a quantitative acceptance result.

Two implementation details matter before reuse:

- The deferred decision can precede a late pulse's falling edge. The reported
  per-tip width is zero on all 88 T/A decisions and the first two Y/A decisions;
  `decide()` then substitutes `lastPulseWidth`. Thus the response is causal, but
  does not always use the current interloper's completed width. Preserve that
  distinction in mixed-width and narrow-extra-pulse tests.
- The comment saying that three tips stay without a pull disagrees with the
  current `pendN >= 2` path, which sums interloper pulls. There is no explicit
  three-tip exclusion. The current header's far-early pull function explicitly
  cites AT/F and AmSpirit, so this is also a documented fitted hypothesis, not
  simply an accidental count predicate. Judge source traces against the
  implementation, not the older three-tip comment.

## Within-group traversal and the T/E source defect

Continuous cold T/A–F and Y/A–B runs use the production HID Space event two
fields after each SSM prefix, releasing it at the next real handler. Every
expected SSM is observed, ending at T/F `00C2` tick 225,463,287 and Y/B `00C5`
tick 79,776,695. This preserves earlier pages within the selected group;
it still does not reproduce the supplied full Module-A walk.

T/A–E retain the same two-tip GA pattern. T/F adds a third pulse: within its
effect band the rising intervals are 1,856/704/1,536 ticks, summing to 4,096.
The frozen candidate produces 87 three-tip decisions with pull −13 Pixel-M2,
plus boundary decisions +10 and −23, identically at all four sampling phases.
Y/B returns to 256-tick pulse widths and +10 pull throughout the band.

T/E is a source-validity discriminator: its original CRTC1 photograph curves
the black band's entry rightward, while this candidate on our source curves
leftward. The governing French ACCC §15.3.4 **p.152**, read in position-aware
Markdown and in the rendered chronogram, supplies a concrete missing event.
For R2=11 followed by R2=21 with R3l=10, C3l overflows, but type 1 emits an
invisible HSYNC off/on transition that restarts the GA and produces another
monitor pulse at C0vs=23. The diagram distinguishes type 2, which has no such
transition. This is a boundary claim supported by the French source, rather
than a monitor parameter inferred from the photograph.

The actual T/E bus writes R2=11 at C0=6, then 21 at C0=15 and restores 46 at
C0=37. Raw HS stays high from C0=11 through 37, and GA HS rises only at C0=13
and 48. Adjacent T/F writes 22 at the same phase and produces rises at 13, 24
and 48. `rtl/CRTC.v` already labels the invisible transition as an unmodelled
residual next to `type1_hsync_overflow`; masking `hsync_off` preserves the long
raw pulse without notifying the GA.

The pinned CPCSyntaxError implementation explicitly handles this same boundary:
`crtc.cpp` emits `onHsyncStart()` under `hsyncRestartFromOverflow`, and
`emulator.cpp` first invokes `GateArray::endCHsync()` before the restart. Its
stated rationale agrees with the French rule; this is not an exception to it.
It demonstrates an event representation, without establishing a physical pulse
duration to copy into RTL.

One-shot source-derived witness:

```text
python3 output_files/v1-shaker-exec-2026-10-29/assert_type1_restart.py
T/E R2=21 observed GA-rise C0vs: [13, 48]
T/F R2=22 observed GA-rise C0vs: [13, 24, 48]
adjacent R2=22 completed-restart control: PASS
type1 invisible restart from §15.3.4 p152: FAIL missing GA C0vs23
```

Remediation **V1-R1** now has a focused deterministic production CRTC/GA failure
and a narrow implementation, with the event contract acknowledged by the
coordinator and V2/V3 before behavior edits. Do not fit the monitor to compensate
for the missing input pulse in the original source trace.

## V1-R1 implementation and focused proof

`CRTC.HSYNC_RESTART` is a registered one-CLOCK event, emitted once at the
type-1 terminal-equality overflow and delayed through the same stored phase
convention as an ordinary trailing edge. Raw HSYNC remains high and C3l keeps
counting. Reset, snapshot load and departure from type 1 discard the pending
event. The motherboard routes it to the classic GA only. Paired CRTC/GA fixtures
carry the event; independent GA references tie it low.

`syncgen_sync` consumes the event only in both H06/hdelay reset terms, including
the high terminal bit. The raw falling-edge detector, R52/V26 counters and
RGB FORCE_BLANK remain on raw HSYNC. This distinction is supported by original
CPC SHAKER BR/E `01F4`: R0=R2=0/R3l=1 gives `#FF`, meaning no R52 overflow in
the 3,328 µs observation window. A full raw-pin low pulse would incorrectly
qualify a counter end and potentially release blacking.

The primary retained regression extends `rtl/GA40010/r2jit_tb.cpp`, using the
existing real-I/O CRTC/GA fixture. Before the fix, the existing R2.JIT tests,
type-0 R2=21 control and both types' R2=22 neighbors pass; only type-1 R2=21
fails with monitor rises `[13,48]` instead of `[13,23,48]`. After the fix:

- T/E produces `[13,23,48]`; raw HSYNC, RGB blacking and end counters remain
  continuous across the invisible event, observed every master clock.
- R3l=1,2,6 controls exercise restart before, during and after the first
  monitor pulse, with 1,2,2 real mode-sync edges and a verified pending-mode
  latch. The width-1 R2 write occurs at C0=11, after its start and before its
  terminal character; this excludes the R2.JIT mixture noted in review.
- The BR/E infinite-HSYNC control observes 208 restart events across 3,328 µs
  while raw HSYNC and both GA end counters remain unchanged. Setup retires the
  prior vertical pulse and prevents independent raw-VSYNC-start resets from
  contaminating the V26 invariant.

Fresh cross-provider review by Claude Opus 5.5 medium, run
`20261029T191005Z-3543-2a54`, completed **CLEAR, no blocking defects**. It checked
phase, duplicate suppression, lifecycle, one-CLOCK mode qualification, H06's
terminal bit, counter/blacking separation, Plus masking and caller compatibility.
Its width-1 fixture concern was corrected as above. Mid-character R2/R3 writes
which create the terminal condition remain an unmeasured timing residual: the
event follows the existing ordinary-off scheduling convention, not a newly
claimed hardware phase. Do not infer full hardware acceptance from the review.

The canonical soak now includes the restart output and its seen/pending/phase
state, producing `0x0874d4b8ecc08793` over 2,845,088 characters. The golden history
records that intentional projection expansion. A separate ignored build with
the old projection, on the repaired RTL, reproduces `0xde0a16d0ececc573` exactly;
this checks that the existing CRTC pin/address/state projection was preserved.
Its retained command is `legacy_obj/legacy_soak --soak 0xde0a16d0ececc573` under
the private execution bundle. The first soak build collided with a concurrent
gate build; the serial rerun and isolated legacy build are the accepted results.

Final selected gate after the width-1 test correction:

```text
python3 sim/select_tests.py --run
select_tests: PASS 10 benches: crtc-test, crtc-cpu-phase-test, ssm-marker-test, ga40010-test, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/p10_boot_tests, run/p10_dma_mobo_tests, run/b8_palette_tests
```

The additional focused output-chain run `make -C sim/plus b6-dynamic` passes
all nine machine/mode cases. No unchanged gate is repeated after documentation
edits. Other slow benches were not run: the primary GA boundary vector proves
the affected shaper/mode/counter interaction, B6 protects output delivery, and
the executing production-T80 source comparison below supplies actual caller
phasing. No synthesis or hardware result is implied by those local checks.

The repaired production-T80/motherboard replays complete the same T/A–F and
Y/A–B SSMs at identical ticks, with byte-identical CRTC write streams. Full-tick
comparison of one complete field per page finds **zero changes** in native RGB,
raw CRTC HS/VS, Full tuple or non-HS GA tuple bits across all eight pages. Only
T/E's GA HS changes: 22,528 ticks = 88 additional 256-tick monitor pulses.
T/E's band intervals become 1,856/640/1,600 ticks; T/F and the other pages retain
their prior patterns. This distinguishes the corrected output event from a
pixel or software-history change.

The frozen candidate now gives T/E the same three-tip decision family as T/F:
87 decisions of −13 Pixel-M2, with +10/−23 boundary decisions, identically at all
four sampling phases. Its illustrative T/E raster curves rightward like the
original photograph. This is a useful integration result, still not held-out
or quantitative monitor validation. Production Full HDMI uses the unchanged
raw-CRTC acquisition/filter path and native pixels; it must not be claimed to
have acquired CTM deflection merely because this monitor-input pulse is fixed.
MiSTer hardware validation of event delivery and the relevant corpus remains
pending with the sole device operator.

## Decision and next discriminators

A pin-only causal audit of the frozen monitor candidate supplies an independent
implementation discriminator. After steady warm-up, a three-sample extra pulse
starting one sample before its deferred decision is initially treated like a
long pulse. Its later rejection restores admission bookkeeping but leaves a
six-sample change in the next flyback. Moving the same short pulse earlier so
it ends before the decision restores the no-extra result. Similarly, widths
36 and 37 ending exactly at / one sample after the deadline produce pulls 11
and 23 because the latter uses the previous-width fallback. These are scheduling
and rollback properties of this implementation, not physical-monitor verdicts.

The private `v1-monitor-astra-2026-10-29/` bundle retains the complete pulse
chronology, runnable unchanged-source probe and state report. A separate
three-tip ablation with all widths known before the deadline produces −23+11=−12,
confirming summation without the width-fallback confound. The host renderer can
also reposition captured previous/current raster records after a snap; emitted
FPGA pixels cannot be revised that way.

Before an FPGA response model, specify pulse identity and qualification/completion
times, window ownership/finalization, each contributor's effective width and
fallback provenance, and the last output interval still mutable. Either a
decision acts on future sweep events only, or an explicitly buffered source
interval remains unpublished until every affecting decision is final. A single
line buffer is not yet a proven sufficient contract. Preserve the frozen model
as a comparison rather than silently correcting these policies while fitting it.

This is a materially stronger offline candidate than the deliberately
under-specified width-only grid. Retain it as a frozen comparison implementation,
not a set of proven CTM constants or a design ready for RTL.

The within-group T/E–F and Y/B source discriminators are captured. Repair and
revalidate the T/E source boundary before using it for response acceptance.
Compare mixed widths and three-tip handling without retuning the candidate.
Full supplied Module-A history remains a separate source-validity gate. Quantitative original-photo
measurement needs a declared ruler, perspective treatment and held-out family;
the candidate's existing AT/AY/D3/D4 fitting must remain visible in that choice.
Only after those checks should displacement, buffering, latency and physical-CRT
bypass contracts be specified. No production filter or menu change is justified
by this continuation alone.
