# SHAKER D1/D6 retest — 2026-09-22

The restored timing-clean build still disagrees with the real-CPC reference on
**SHAKER 2.7 B (9), CRTC 1, first numeric page**. Two captures reproduce the
September 12 image exactly. This is a confirmed remaining acceptance mismatch,
not evidence that the D1 repair should be reverted or that a new RTL cause is known.
The device became unreachable before page 2; CRTC 0 and C (4) were not run.

## Identity and evidence

- Source checkout: `cadbb3775d0a83fec3ceb124ebd377cbc829bfc5`.
- RBF: `Amstrad_20260922_0601050.rbf`, SHA-256
  `66b8d72fa1b67a6535ba07a4d9118c9998f833a4bbe89a1339d02eb4d25fa7fd`.
  No intervening changes in `rtl/`, `Amstrad.sv` or `files.qip` between that
  source and the task base. The coordinator supplied exact CI run `35686740255`
  as passing; this task did not rerun synthesis or simulation.
- Device media: `/media/fat/games/Amstrad/dsk/shaker27.dsk`, SHA-256
  `65eb43e1f99ea232a6cc1494e799880488130ba1876bfe9d67b347a924e7721b`.
- Applied settings: Classic CPC 6128, CRTC 1, Full. Saved CFG bytes
  `00004000000000000000000000000000`; SHA-256
  `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
  This equals the original backup. The captured heading confirms CRTC 1;
  native screenshots do not show the OSD or verify the active filter mode.
- Keyboard tool: pinned MBC commit `3873450d413c30e6b0339e6b3dbf2373e0e5a74a`,
  rebuilt using the existing recipe. Host and device SHA-256 both
  `0e99082bb8c9b8b2c2b6581c736dfc5d701a977a9c76fa548565cbc6642984db`.

Private evidence is under
[`docs/screenshots/shaker-d1-d6-2026-09-22/`](../../screenshots/shaker-d1-d6-2026-09-22/):
`b9-type1-run1/manifest.json`, `last-run.log`, both PNGs, the original CFG,
task-authored CSL cases, reference photo/API snapshot, and `device-state.json`.
The failed manifest remains failed even though its first two captures completed.

Both 768×273 captures, `B9_type1_page1.png` and
`B9_type1_page1_repeat.png`, have SHA-256
`68cf044c423d607f74ec9c642de644f1d3cbf1f33388f24e9e50103ac22ef468`.
This is exactly the [September 12 capture](b2-device-capture-2026-09-12.md) hash.
It establishes repeated captured content, not physical display timing or all fields.

## Numeric comparison

The oracle is Logon System's [real-CPC B9 CRTC1 A photograph](https://shaker.logonsystem.eu/images/cpc/CPC/B9_CRTC1_A.webp),
not an emulator image. The downloaded WEBP SHA-256 is
`b836670809878b6b5385b911537d85aac87167124e91611be8ea9c298d9419aa`.
The [portal inventory](https://shaker.logonsystem.eu/api/tests) identifies B9 A
as SSM `01C7`, B as `01C8`; CRTC 0 additionally has C/D (`01C9`/`01CA`).
No SSM event was observed in this timed capture run.

Values below are hexadecimal microseconds as printed by SHAKER. Line lists are
in screen order 0 through 4. These are observed values, not new test expectations.

| Screen block | Real-CPC reference | Restored MiSTer |
|---|---|---|
| R6=19, R7=0, R8=3 on lines 0–4 | 2740, 2760, 2780, 27A0, 27C0 | 2700, 2760, 2740, 27A0, 2780 |
| R6=7F, R7=0, R8=3 on lines 0–4 | 2740, 2760, 2780, 27A0, 27C0 | 2700, 2760, 2740, 27A0, 2780 |
| R7=18 before R6=19, lines 0–4 | 1820, 1840, 1860, 1880, 18A0 | 1820, 1840, 1860, 1880, 18A0 |
| R8=3 on raster line 2 / R8=0 on line 43, first two blocks | 43C0 | 43C0 |
| Same ON/OFF control, R7=18 block | 25C0 | 25C0 |

The doubled-frame `4E00…` values in the September 9 evidence are absent here.
The nonzero-R7 block matches completely. The remaining difference is exactly
64 µs (one line) on the even-numbered entry lines in the R7=0 blocks.
Nearest-neighbour enlargement resolved ambiguous small glyphs: this transcription
supersedes the mistaken `2700/2700/2740/2780/2780` and final `1880` values in the
September 12 prose and the first unzoomed reading in this task. The PNG hash was
correct throughout. See private `b9-numeric-3x.png`.

## C (4) and automation boundary

C (4) is **not a clean numeric-table acceptance test**. The real-CPC
[initial state A](https://shaker.logonsystem.eu/images/cpc/CPC/C4_CRTC1_A.webp)
and [state B](https://shaker.logonsystem.eu/images/cpc/CPC/C4_CRTC1_B.webp)
contain repeated text, patterned regions and coloured stripes. A garbled-looking
capture is insufficient to classify this test as failed. Match the state label
and raster structure; paired states also need a temporal observation.

The locally supplied CSL scripts name SHAKER **2.6**. Both 2.6 and 2.7 disks were
present on the device. This run used a short task-authored CSL v1.5 script,
French keyboard translation, timed waits and explicit screenshots against 2.7.
It neither substitutes a 2.7 filename into the author's complete 2.6 walk nor
claims live `wait_ssm` validation. The prepared C (4) script was not executed.

## Device interruption and recovery obligation

The last successful input was a completed SPACE tap at approximately
04:54:50 UTC, advancing toward B9 page 2. MBC returned zero; no key hold was
left by that command. At 04:55:10 UTC the next screenshot preflight timed out
connecting to SSH. CFG restoration and temporary-file cleanup then failed.
Subsequent bounded checks could not resolve `mister` or `mister.local`.

**MENU and restoration were not verified.** The applied CFG happened to equal
the backup, but this does not turn a failed cleanup into a successful one.
The coordinator and Sonic task were notified; no competing device writes were
authorized. Once connectivity returns, the device owner must restore the retained
CFG, check its hash, return to MENU, and remove only these task temporaries:

- `/media/fat/csl_cfg_1790052829_1713b1.bin` (may already be absent)
- `/media/fat/csl_1790052829_1713b1_0.mgl`
- `/tmp/mbc-shaker-20260922`

The copied RBF is a retained named build, not a temporary to remove. Hardware
work still pending: B9 type 1 page 2; B9 type 0; C4 type 1 with explicit state
identification. No D1/D6 hardware closure is claimed.

## Source diagnosis

Opus diagnosis `20260922T050207Z-60925-2faa` identified a narrower source
conflict than the original D1 origin-pulse fault. The parent re-extracted French
ACCC v1.11 pp.85, 200, 209–210, 217–218 through pdf-inspector and viewed pp.85, 209,
217–218. Section **19.6.2 p.217** adds a physical line after the R5 lines, if any,
whenever R8 is 1 or 3 and ParityFrame is even. Its following paragraph concerns
C4 accounting, not a condition on whether that physical line exists.
Section **19.5.3 p.209** separately pairs the even-frame additional line with
MID-VSYNC. The special first-line activation bug on p.218 belongs to **CRTC 2**
(§19.6.3), not type 1.

The pre-repair engine incorrectly required positive R5 and `(R9+1) % R5 == 0`
for the physical line. Its F14 comment and `t28b` repeated this reading, partly
justified by retaining the existing R5=0 IVM walks. D1's pulse-count/phase
vectors did not assert complete frame duration, so they did not discriminate
this missing line. The shared wrapper consumes the engine's row/frame events
for C4/C9 reset, parity, adjustment state and VSYNC; fixing only a private timer
would not repair that boundary.

The observed B9 difference has the predicted signature: one missing 64 µs line
on the even-entry cases, unchanged odd-entry/MID-VSYNC cases, and unchanged
measurements ending at nonzero R7 or after IVM has been switched off. This is a
causal candidate supported by a source contradiction. B9's full executed write
sequence was not traced here; its R4/R5 setup was not inferred into a test oracle.

Gemini image comparison `20260922T050417Z-64106-4a46` independently confirmed
all B9 rows. Its C4 discussion is accepted only as a visual description of the
reference states; a still image cannot establish the claimed recovery mechanism
or temporal stability. The earlier Gemini run `20260922T045031Z-53155-1d75`
downloaded references but ended with a provider network error and no comparison;
its zero wrapper exit is not a completed review. Logs are retained in the private
`review/` directory. No documentation-only review was requested.

## Repair and changed expectations

The type-1 engine intercepts both the ordinary R5=0 end and the positive-R5
adjustment end on an even interlace frame, then completes the added line through
the existing canonical frame-origin path. The shared wrapper is unchanged.
For the newly covered nondivisible adjustment path, C9/C4 continue ordinary
adjustment counting into that line. The established divisible-R5 counter path
is retained: the p.217 multiple wording versus pp.84–85 carry examples remains
an explicit internal-counter residual. Physical line duration is independently
specified and no longer depends on resolving that ambiguity.

Four new cases `t28d`–`t28g` use R8=1/3 × R5=0/3 with R9=7, R4=7 and
R0=63. On paper, the ordinary portion is 64 plain or 32 IVM lines plus R5;
the even physical frame adds one line and the odd frame does not. The tests
observe six origins and check origin-to-origin duration and VSYNC spacing,
including the existing discrete C0=31 midpoint convention. Their pulse length
is shorter than the frame so overlap cannot conceal the interval.

`make -C sim crtc-test` before the RTL edit: **226 passed, 4 failed**, all
four new cases exactly 64 characters short (`implementation/red.log`).
After the repair and source-based fixture corrections: **230 passed, 0 failed**
(`implementation/green.log`). The intermediate failures are retained rather
than presented as unrelated errors or silently dropped assertions.

| Existing vector | Old premise → corrected expectation; preserved purpose |
|---|---|
| `t02p` | Two 17-line type-1 frames → 18-line even plus 17-line odd; still tests even MID-VSYNC phase. |
| `t02q`, `t02r` | Type-1 setup now 2+1 / 4+3 lines. R7=127 remains unreachable when the added line reaches R4+1. Snapshot/type-switch pulse-history assertions are retained. |
| `t23a`, `t23b` | Insert C4=2/C9=0 additional line before origin; retain row parity and snapshot IVM-entry walks. |
| `t24a`, `t24b`, `t24c` | The even frame includes physical line 32 before the odd frame. Within-frame gap/no-gap and midpoint assertions remain. |
| `t28b` | Nondivisible R5=3 no longer suppresses the line: C4=3/C9=3 precedes origin. This corrects the disputed rule itself, rather than weakening a control. |
| `t32a` | IVM entry moves to even row 2; the test explicitly proves PF=0/PC9=1 on the added line before origin. It still distinguishes origin realignment from stale toggle-both logic. |
| `d1_type1_overlap` | Initial R7 moves 1→2 for 10+9 rather than 9+9 lines; the fixed 16-line pulse still ends at the final odd-frame midpoint. Active-pulse phase ownership remains the assertion. |

The expected changes follow French §19.6.2 p.217 with the pre-existing parity
and VSYNC rules, not new simulator measurements. English §19.6.2 p.217 was
also extracted and agrees on the physical-line gate; French remains the oracle.

### Review boundaries

Fresh Opus review `20260922T051530Z-70701-c53b` cleared the main diff and
rederived the changed fixtures without rerunning suites. It recommended pinning
the newly reachable R7=R4+1 VSYNC path. It also recorded two remaining limits:

- Clearing the private additional-line latch by snapshot load or a live type
  switch while C4=R4+1 can leave the shared counters beyond R4 for one long
  frame. The R5>0 adjustment path has C5 to bound that state; the R5=0 path
  does not. No hardware equivalence for that transition is claimed.
- R5=0's added line is not marked `in_adj`. The R4=0/C4=1 video-address
  reload and the added-line VMA-save behavior are not fully adjudicated.
  The ordinary IVM restart supplies the C9/ParityC9 values asserted in
  the fixture walks; their extra-line internal state is model inference,
  not a separately measured hardware rule.

The parent additionally found a concrete engine/wrapper inconsistency if R4
and R5 are changed during that new R5=0 line: the engine's unconditional origin
could coincide with the wrapper's adjustment-entry branch. This is distinct from
the documented hardware residuals; the chosen origin path must at least be
internally consistent. Its focused correction and final review are recorded below.

For those counter/address residuals, the next discriminator is an instruction-
level trace positioned on the added line, observing C4/C9, parity, MA/VMA and
raw VSYNC through the selected transition. Separate a snapshot's software
contract from live CRTC-type behavior; do not infer either from a still screen.
The R4=0 case needs an R12/R13 write on the added line and the following memory
fetch to distinguish ordinary-row and adjustment-only reload behavior. These
paths are newly exposed by the R5=0 line; neither a hardware regression nor a
hardware-correct transition has been established.

`t28h` reproduces that hybrid origin with early R4/R5 writes during an R5=0
added line. The meaningful fail-first run is `followup-red-verified.log`:
**231 passed, 1 failed**, at “origin cannot also enter adjustment”. An earlier
`followup-red.log` failed fixture setup and is not the bug reproduction.
The engine now prevents adjustment entry while the additional-line latch is
active. This enforces the already-chosen unconditional origin; live-write
hardware semantics remain unverified.

`t28i` pins the newly reachable R7=R4+1 raw-VSYNC path using the ordinary
C4/R7 comparison (French §16.1 p.160) and the added physical line. Across six
frames it requires three even-frame starts, no odd-frame starts, and three
fixed 16-line pulse completions across origins. The final focused run is
`followup-green.log`: **232 passed, 0 failed**, with no subsequent code edits.

Fresh final Opus follow-up `20260922T052326Z-74427-8b31` returned **CLEAR**
on the exact final code diff, preserving the earlier fixture acceptance. It
confirmed that the guard changes only the wrapper's adjustment-entry decision
while the additional line is active, and checked both new regressions against
their RED/GREEN evidence. It reran no suites. The code was rebased onto
`c0e80792c735` (the coordinator's independent Plus documentation and B20 queue),
with no code conflict or subsequent code edit. Review logs are
`review/opus-first-review.log` and `review/opus-final-review.log`.


## Final source acceptance

The last code editor ran `python3 sim/select_tests.py --run` once on the final
code: `select_tests: PASS 3 benches: crtc-test, crtc-cpu-phase-test, ga40010-test`.
The unrelated slow snapshot bench was listed but not run. Exact command and
output are preserved in `implementation/selected-gate.log`.

One intentional behavior mint, `make -C sim soak`, produced
**`0xe99ab434a5e1cdb3`** from seed `0xaccc5eed20260822`, with 2,845,088
characters and CLKEN samples (`implementation/soak.log`). The previous hash
was `0xb1cb70da95c2e44f`. Seed, stimulus and sampled projection are unchanged;
the additional physical line and its adjustment-entry guard change behavior.
Subsequent edits only finalize documentation and this recorded hash.

This is reviewed and simulation-gated source, ready for coordinator integration.
No repaired RBF was built or tested here. B9 on both types, C4 on type 1, and
device restoration remain open; the partial pre-repair capture does not close
those hardware gates. No further device connectivity attempts were made after
the coordinator stopped retries.
