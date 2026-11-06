# V1 private receiver/renderer candidate

An isolated fixed-point receiver/renderer is implemented on
`codex/general/v1-crt-response-20261106`, explicitly based on
`1e95fa0a802ea0cf04bb7e06d50dff9370e204b8`. It implements the
[accepted qualitative design](v1-qualitative-monitor-design-2026-11-06.md).
Public mode activation and physical CTM calibration remain separate. Full stays default;
reserved mode 3 continues to select Full in production until integration is validated.

## Hardware boundary

A new isolated `crt_response` consumes these signals on the 64 MHz video clock.
The first slice requires the existing one-in-four native-dot enable; it does
not derive a new pixel clock or change producer timing.

| Input | Contract and intended production source |
|---|---|
| `clk`, `ce_16` | Master clock and selected native enable; one dot per four master edges |
| `reset`, `context_clear`, `owner` | Held reset, SNA-load/committed presentation change, and selected classic/Plus owner; invalidate receiver and RGB history before accepting new samples |
| `ga_hs`, `ga_vs` | Already selected `hsync_ga`/`vsync_ga` in the motherboard; monitor stimulus independent of acquisition |
| `source_missing` | Explicit acquisition absence (`crt_filter.no_hsync`, currently also observation bit 11); re-prime estimator while timing transport continues |
| `rgb12`, `raw_vblank` | Selected post-compositor RGB12 and its aligned vertical-blank tag, including Plus sprites and DAC horizontal near-black |
| `hs`, `vs`, `hblank`, `vblank` | Complete regularized filter tuple, independently delayed at fixed latency |
| `field`, `frame_field` | Both owner-selected field tags, delayed with the acquisition tuple |

Outputs are displaced RGB12 plus its blank tag/availability, and the complete
fixed-latency tuple and field tags. No output feeds CRTC/GA counters, IRQ, CPU,
VRAM arbitration, sprite fetch/production, or the acquisition filter. Model
state depends only on monitor tips, acquisition line starts and lifecycle.

Source inspection at this dependency base confirms `selected_source_rgb` is
after the classic/Plus mux, and the Plus branch retains all twelve sprite/palette
bits. Full's eight-dot RGB history is a separate later mux. Future activation
must select native byte order and bypass that Full history as Raw pixels does.
`frame_field` and `field` currently leave the motherboard independently; both
must pass through this slice together when activated. The filter's old
HBLANK_LIVE comment calls its raw CRTC HS input a GA event; correct that comment
with the eventual wiring change. This branch will not alter frozen production
inputs merely to correct a comment.

The initial module is instantiated only in simulation. Production integration
must add it to `files.qip` when first instantiated there and explicitly export
source absence; observation bit decoding is not the preferred production API.
Mode entry/exit must use both live and delayed filtered VBL plus the existing
CPU-phase/reset policy. A dedicated native-cadence control must disable adaptive
pixel rate and HQ2x without borrowing Raw CRT's geometry/crop bypass.

## Fixed candidate and acceptance

The accepted private Python reference is SHA256
`299b3b205ddbe327af43bcea6b3f1106e1c04d823f00ffd836b25925c61b9ea7`.
It adds the lifecycle/capacity closure below to the reviewed predecessor
`401e70ce750f3d736df66e60b78b263b5d14ae5e5a41aa3601aa7d3bdb55b989`;
the private `offline/MANIFEST.json` binds both copies and the minimal patch.
Preset SHA256 is `c343be8623db7bd154de9f6ca087996c97842240861277a101795cf88fd79ac0`:
Q8 phase, 256-master-clock width reference/cap, minimum width 8, relaxation
1/8, early extra pull 16 dots, late extra pull 8 dots, bound ±48 dots,
64-dot presentation delay, 128 history entries, eight consistent priming lines.
No fitting against exposed T/Y/D3 or title/row-specific behavior is permitted.
Two pulse-window records admit only nominal periods greater than 640 master
clocks with cumulative raster coordinates limited to ±192 clocks. General
shorter acquisition falls back to zero response; it must never overwrite a
pending line offset or read stale RGB.

Before RTL behavior was written, the focused cross-boundary test showed RED
against an identity transport. The completed receiver moves a stationary RGB
edge over multiple lines and recovers, while steady rows retain registration
and acquisition/field latency stays fixed. Natural zero-response parity,
owner/sprite transport, reset/mode refill and short-line admission have separate
transport/lifecycle checks; they do not calibrate physical amplitude.

## Acceptance and remaining gates

**READY as an isolated receiver/renderer candidate.** Fresh Opus 5.5 review
found a medium off-HS invalidation defect; the repaired code passes focused/model
comparisons and the final 47-bench selection. Follow-up review
`20261106T183706Z-13036-6ada` clears the code. Its requested one-shot exact-HS
anchor proof also passes, with an independent old-binary failure; the reviewer
explicitly requires no further code change, gate run or review for that proof.
The unchanged CRTC soak matches
`0x974e5c048eb7f877` over 2,845,088 CLKEN samples.
Quartus fit, M10K packing/read latency, timing, production activation and exact
build hardware comparison remain coordinator/integration gates. No synthesis
is dispatched by this task and no emulator/device slot is assumed.

## Proof checkpoint

The identity-transport baseline passes 106,427 RGB12/tag samples and 425,728
master-clock tuple/both-field comparisons, then fails specifically at
`late extra tips must move stationary RGB edge left` (clock 163,840). Late
extra tips in rows 24–47 rise at phase 2368 and fall at 2624, crossing the
rise-owned deadline 2560. The baseline edge stays at dot 400 everywhere.
The expected left direction, multiple intermediate positions and recovery
criteria were written before receiver behavior. A separate PRNG transport
check passes 4,733 samples through held reset, held context clear, owner change
and 128-clock acquisition lines; the same movement RED remains. These checks
exercise data addressing and lifecycle, independent of response arithmetic.

The minimal enabled-sprite fixture reuses the production `asic_sprites` leaf
and its registered fetch contract with a simulation compositor. That establishes
sprite RGB/tag/owner transport; the real `asic_video` compositor, motherboard
native-byte selection and output colour chain remain integration gates.

Private inputs and proof logs are retained in the task-owned real directory
`.roster-scratch/v1-crt-response/`. Coordinator packets stay at their original
absolute evidence paths. Temporary worker logs are copied here at acceptance.

The focused RTL bench passes with the edge at dot 400 above the band,
399/398/396/393/392 through it, then 394/397/400 during recovery. Opposite
early pull, missing-tip phase hold, rejected glitch/overlong/VS pulses,
cumulative ruler correction, common translation, reset/mode/owner/source-loss
after displacement, exact-deadline strict-prior finalization and timer rollover
pass. The real sprite leaf transports 192 enabled palette pixels across
Plus→classic→Plus owner changes with independent blank-tag coincidence and
refill checks. It uses a test compositor, so production compositor integration
is still unproved.

Parent review found two port defects with independent REDs before repair:
an open tip belonging to the preceding window leaked into the first primed
window, and a physical period 65536+4096 aliased to an ordinary 4096-clock line.
The former now derives ownership from the frozen half-open window; a
receiver-only watchdog invalidates at 16384 clocks before the latter can wrap.
These fixes have retained regressions because ownership and timestamp state
interact with the active receiver implementation.

All 653,360 master clocks of three synthetic streams agree bit-for-bit with
the accepted Python reference on public RGB/tag/availability/tuple/both fields.
The streams cover late crossing tips, asymmetric opposing tips, cumulative
common translation, recovery and the tight L641 two-record boundary, with rich
RGB and field edges between native enables. `oracle/MANIFEST.json` binds the
generator and inputs; `rtl/oracle-L*.log` records the passing comparisons.
This proves the port against the accepted model, not the model against a CTM.
The real T/Y/D3 inputs and prior replays remain untouched; no new holdout is claimed.

A further one-shot seeded comparison passes 2,977,072 clocks and 743,588
available native samples across ten frozen streams. It covers native enable
phases 0–3, periods 641 through 16191 (observed maximum 16383), exact primary
ties, widths 7/8–256/257, asymmetric tips, VS straddles and bounded raster
coordinate changes. Rich RGB and blank tags remain independently addressed.
`stress/FROZEN-MANIFEST.json` and `PRE-COMPARE-HASHES.json` precede execution;
`stress/COMPARE-RESULTS.json` records all ten passing commands. Together with
the three focused streams, this is 3,630,432 accepted-model comparison clocks.
These streams were repeated after the review repair below.

Opus review (`20261106T182128Z-6372-d0e4`) found that receiver-only invalidation
zeroed the emitted line's offset immediately. A mid-line missing-source flag
therefore skipped source dots after the fixed delay. Both a retained edge
vector and an independently generated public-output stream fail before repair:
the latter first diverges exactly 256 clocks after the off-HS flag assertion.
The repaired receiver holds the current line's offset until its next HS;
reset/context/owner invalidation still clears pixels and offset immediately.
A coincident clear/HS also commits native fallback and seeds the fresh anchor.
The new edge checks cover off-native source loss, an exact-HS clear and
off-native watchdog expiry without invalidating valid same-owner RGB history.

Four further frozen streams close the review's priming and transport gaps:
O700>L/2, O−100 with a tip crossing HS, mid-line source loss with a tip in flight,
unsupported +193-step re-priming, and a 128-clock acquisition interval after
nonzero displacement. The two short-line delayed HS edges remain pending
together and receive their own offsets in order. `oracle2/REPORT.md` and its
manifests/RED/GREEN receipts bind these 1,310,720 clocks. All 17 prior and new
streams pass again after the repair: **4,941,152 total master clocks**.
The sprite dependency is now in the new bench's selection coverage; a dry-run
selection on `rtl/plus/asic_sprites.v` explicitly selects it.

The final one-shot anchor proof asserts source absence on HS50 and releases it
on HS52. The preserved old binary first diverges at clock 250132, the first
displaced native sample after re-priming on row61; neither clear edge causes
the first mismatch. The current binary passes all 491,520 clocks. Exact one-shot commands are:

- `.roster-scratch/v1-crt-response/rtl/review-f1-before/crt_response_tests --oracle .roster-scratch/v1-crt-response/oracle3/F5-HS-coincident-missing.bin`: exit1.
- `sim/obj_dir/crt_response/crt_response_tests --oracle .roster-scratch/v1-crt-response/oracle3/F5-HS-coincident-missing.bin`: exit0.

 `oracle3/RED.json`, `GREEN.json` and `REPORT.md` bind the complete
commands, binary hashes and first-counterexample chronology. This remains a
one-shot proof rather than another maintained vector. The 18 synthetic streams
total **5,432,672 model-comparison clocks** on the final implementation.

Offline lifecycle controls fail against the frozen predecessor: reset writes
one history sample, a mode-change event retains old-epoch RGB/offset, and a
512-clock acquisition period primes an unbounded receiver. The repaired copy
holds history empty during reset, explicitly invalidates on a committed
`mode_change` event, and rejects periods at/below 640 clocks. Coefficients and
preset remain unchanged. RTL `context_clear` is a held clear with write
suppression; the Python mode event may accept the new-context sample on that
same feed, so their refill origin differs by one native sample. Both prohibit
old-epoch reads; the hardware boundary uses the stricter held-clear contract.

Ordinary-width characterization repeats measured GA widths synthetically at
period 4096; the original two captured lines alone cannot prime the receiver.
Widths 133/197/256 clocks settle at offsets −15/−7/0 native dots. Because the
read address is x + offset, the 4→5 change moves a stationary edge left by
eight dots; 5→6 moves it left by seven. The latter is a result of this fixed
midpoint model, independent of Full's retained absolute positioning. Neither
result is a new amplitude calibration or real-monitor settling measurement.
A CPCSyntaxError rerender would not resolve lifecycle, RAM or ownership
uncertainties and is not used as a calibration target.

The RTL slice additionally limits nominal periods to 641–16383 master clocks
and invalidates when any observed acquisition interval reaches 16384 clocks.
An otherwise allowed positive ruler step near the upper bound can therefore
re-prime the receiver; the observed-interval bound still applies.
Its 16-bit modular timestamps then keep live two-window differences inside a
signed half range, including wrap. Longer periods use native position and
re-prime; this is an implementation capacity bound, absent from the unbounded
Python timestamp model. Ordinary 4096-clock lines fit without qualification.
This restriction is labelled at admission and must be reconsidered with a
wider timestamp before broad production-mode activation.

Pixel history stores 128 × 13 = 1664 bits without clearing RAM. Tuple transport
uses 257 × 6 = 1542 register bits; the per-master-clock offset pipeline uses
256 × 7 = 1792 register bits. The latter deliberately avoids a single pending
offset that a short line could overwrite. These are source-level storage
counts, not Quartus utilization or M10K packing results. The renderer performs
one synchronous read on each native enable from the pre-write pointer at age
64 − offset (16–112 dots), concurrently writing the current source sample.
The fill guard uses the pre-write count, so unavailable samples stay tagged
blank. Source and acquisition have exactly 256 master clocks (64 native dots,
4 microseconds) of presentation delay at the specified one-in-four cadence.

## Verification record

The final code edit precedes all final focused/oracle reports and the selection:

- `make -C sim crt-response-test JOBS=1`: PASS.
- Seventeen `sim/obj_dir/crt_response/crt_response_tests --oracle <file>` runs:
  PASS, 4,941,152 total master clocks; packets `oracle/`, `stress/` and `oracle2/`
  name and hash each file. Final batch: `rtl/review-f1-oracle-batch.log`.
- `python3 sim/select_tests.py --check`: index OK, 74 rows.
- `PYTHON_CPU_COUNT=1 JOBS=1 python3 sim/select_tests.py --base 1e95fa0a802ea0cf04bb7e06d50dff9370e204b8 --run`:
  `select_tests: PASS 47 benches` (full final line in `rtl/selection-final.log`).
- `make -C sim soak JOBS=1 SOAK_EXPECT=0x974e5c048eb7f877`:
  unchanged hash matches; `rtl/soak.log`.

The explicit dependency base scopes this gate to the candidate rather than
reselecting work already proved on the coordinator branch. Existing Full/Raw
colour/output proofs pass in the selection. Actual production-path activation
and its slow motherboard/ASCAL checks are still integration work.

Quartus must assess the ordered 32-bit receiver arithmetic and repeated
`finish_windows` calls on the 64 MHz path, as well as RAM inference and the
3334 tuple/offset register bits. Source-level scheduling does not establish
physical M10K latency or timing closure. Reset release on an exactly coincident
fresh GA rise can prime one line earlier than the Python feed's stricter
wait-low policy. Conversely, a pixel lifecycle clear exactly on HS uses the
next HS anchor in RTL, while Python keeps that clear-tick anchor. Both safely
invalidate pixels; exact lifecycle priming latency can differ by one line.
Exercise these coincidences with the real owner/mode/reset sources during
production wiring. With no HS, receiver-only loss holds the last bounded offset
indefinitely, matching Python; verify the production filter's missing-HS output
at that same integration boundary. The native 1:4 enable cadence is an interface
requirement, without a runtime cadence checker in this slice.
