# Independent Cross-Provider Review: Re-Review of ox-alpha Historical Items & Open Review Debt

Reviewed 2026-08-28 by **Claude Opus 5** (independent cross-provider pass over items previously
reviewed by ox-alpha under the 2026-08-22 locked decision recorded in `docs/review-debt.md` and
`docs/accuracy/f7-plus-followups-independent-review.md`).

- Repository branch: `accc-review-and-fixes`, reviewed tip `feb57d364efedebd006d0af32224a929fa40d889`.
- Working tree clean apart from this document.
- Primary sources consulted directly, not through digests: `docs/ACCC1.10-EN.pdf` pages 75-76,
  84-88, 92-93, 108, 124, 129 via `pdf-inspector` (`verdict: native_complete`, no OCR fallback
  pages), plus a rendered image of page 129 because that page is entirely chronogram figures and
  its text layer collapses three per-type blocks into one stream.

The historical commits under review predate the per-type split: `de71808`, `da79915`, `1a1233f`,
`c4c3e0f`, `90aed07` and `c9f4a4e` all edited `rtl/UM6845R.v`, which is now `rtl/CRTC.v` plus
`rtl/crtc_type0_engine.v` and `rtl/crtc_type1_engine.v`. This review judges the behaviour **as it
stands at the reviewed tip**, not the historical diffs in isolation, because a re-review whose
only question is "was the 2026-08-22 diff correct?" cannot see what six later findings did to it.
That framing is what produced finding N1.

---

## Executive Summary & Overall Verdict

**CLEAR WITH NON-BLOCKING FINDINGS.**

No blocking finding. All three gates pass at the reviewed tip (172 classic vectors, every Plus
suite, soak hash `0x48146d2b681268ab`, lint exit 0). Every item in scope holds up against the
ACCC v1.10 text I read directly, and the vectors I checked derive their expectations from the
source rather than echoing the simulator — several reproduce the Compendium's own worked examples
close to verbatim.

Five non-blocking findings and three observations follow. The one that matters most is **N1**: the
two CRTC-1 RFD trigger routes now disagree about the same documented condition, because finding
F17 (2026-08-27) applied the §11.6.1 p.88 "RFD triggered on the last line C9=R9 disables the
VMA-source state" rule only to the R5 route and left the older §13.7.1.2 R0-widening route
(2026-08-23) on its pre-F17 behaviour. `t13g` pins that pre-F17 behaviour on a line where C9==R9,
so the suite currently asserts both readings at once. This is exactly the stale-integration class
the review brief asked about, and it is invisible to a review scoped to either commit alone.

**N2** is the second-most consequential: the §11.3.2 p.85 clause "C4, however, continues to be
compared to R4 to process the change from C4 to 0" is neither implemented nor adjudicated, and the
digest paraphrase weakened it to "C4 still compares against R4 for its own logic". No vector
discriminates the two readings.

---

## Detailed Analysis

### 1. `de71808` / F4 — equality-only counter rollover

**Verdict: CLEAR.**

Verified against the source rather than the digest:

- p.75-76 (§10.3): "If R9 is modified with a value less than C9, then C9=C9+1 (overflow of C9) …
  it will count to its maximum value (31) before looping back". Matches `t07a`/`t07b`
  (type 1) and `t07e`/`t07f` (type 0) exactly, including the R9=0 case, which the source treats as
  an ordinary limit value with no short-circuit.
- p.92 (§12): "If the value of R4 is less than C4 (excluding vertical adjustment), then C4 counts
  up to its limit (127) and loops back." Matches `t07c`/`t07d`/`t07g`/`t07h`.
- p.92 (§12.2): "position R9 or R4 with the value 0 on C0<2 of the first line of the frame (when
  C4=C9=0 when (R4>0 and R9==0) or (R9>0 and R4==0))". This parenthesis is precisely the
  discrimination `t07i` (both limits effectively zero — arms) and `t07j` (only one zero — does not
  arm) encode. The loose "R4>0 or R9>0" misreading the `t07j` comment rejects is genuinely rejected
  by the source.
- p.93 (§12.2.1): the RLAL worked example — "If, on the first line, we program R4=1 and R9=0 (when
  C0>1) … The test that took place on the second line (on C0=0) indicates that it was the last one
  (C4=R4=1 and C9=R9=0). C4 will therefore increase to 0 on the third line." `t07k`
  (`test_type0_rlal_first_line_delayed_arming`) reproduces this line for line.

RTL side, the shortcuts are genuinely gone at the tip. `crtc_type0_engine.v:311`
(`wire line_last = in_adj ? (line == crtc0_line_max) : type0_ivm_limit;`) and
`crtc_type1_engine.v:290` (`line_last_w = (line == crtc1_line_max)`) are pure equalities, and no
`!line_max` / `!R4` magnitude term survives in either engine or the wrapper. The type-0 C0=0 seam
comparator (`type0_c0_line_last`, `type0_c0_row_last`) is likewise equality-only.

The F4 rule survived the per-type split and six subsequent findings intact.

### 2. `da79915` + `1a1233f` / F12 — CRTC 0 last-line and vertical-adjustment entry arbitration

**Verdict: CLEAR.**

The §13.2.6 p.108 case study is reproduced correctly:

- "**C4's last hiccup** If C9=R9 on C0=0 (when R0 goes to 0) then C9 is no longer managed, but C4
  will however be incremented **regardless of the value of R4**." →
  `crtc_type0_engine.v:472` `frozen_row_advance = !CRTC_TYPE && r0_frozen && !in_adj &&
  !type0_r0_zero_entry_consumed && line == R9_v_max_line;` — no R4 term, correct.
- "**Additional management** If C9=R9 and C4=R4 (when R0 goes to 0) … an additional management is
  activated" → the wrapper's `if(e0_frozen_row_advance) begin row <= row + 1; if(row ==
  R4_v_total) in_adj <= 1; end`. Correct, and the one-shot `type0_r0_zero_entry_consumed` latch
  correctly makes the hiccup fire exactly once.
- p.75-76 (§10.3.1): "If the 'last line' state is true at position C0==0 … and R9 or R4 is updated
  at position C0==1 with a value different from C9 or C4, then vertical adjustment becomes active
  and the current line becomes the 'first' adjustment line." → `type0_c0_1_break_write`
  (`crtc_type0_engine.v:294`) matches the predicate exactly, including the "different from C9 or
  C4" discrimination, and `type0_rollover_line_last` forces the current line not-last so it becomes
  the first adjustment line. Pinned by `t16m`/`t16n`.
- §11.2.2 R4-write-switches-C9/R5 window: `type0_r4_window_write` requires `hcc >= 2 && hcc <=
  R0_h_total`, matching "from C0=2 through C0=R0". `t16a`/`t16i`/`t16g`/`t16j` cover the plain,
  at-R0, and mid-character variants.

The `hcc == 2` re-evaluation in the wrapper (`frame_adj_r <= frame_adj_r & e0_hcc2_adj_keep`) is
ungated on `CRTC_TYPE`, but `frame_adj_r` is routed only into the type-0 engine, so the type-1
value it holds is never consumed. Intentional under the shared-flop model; not a defect.

`t16k`/`t16l` correctly pin that the arbitration latches clear on snapshot load and on a live type
round-trip — the engine's reset term is `~nRESET | SNA_LOAD | CRTC_TYPE`, which matches.

### 3. `cd47d7d` / P0 CPR parser — untrusted RIFF/CPR stream

**Verdict: CLEAR, with two test-coverage findings (N3, N4).**

I traced every arithmetic bound and every fail path. The bounds are sound:

- `riff_len_valid` (`plus_cpr_parser.v:104`) constrains `full_riff_len` to `[4, 0x01FFFFF7]`, so
  `riff_limit = full_riff_len[24:0] + 8` maxes at `0x01FFFFFF = 2^25 - 1` and cannot wrap the
  25-bit register. Correct by construction, not by luck.
- `chunk_extent_exceeds_riff` is computed in 33 bits (`file_pos_after_chunk`,
  `chunk_total_span` including the pad byte, `riff_limit_ext`), so a `0xFFFFFFFF` declared chunk
  length cannot overflow into a false pass.
- The 8-byte chunk-header headroom check `{8'd0, file_pos} + 33'd9 > {8'd0, riff_limit}` is
  arithmetically right: after consuming the byte at `file_pos` the position is `file_pos + 1`, and
  a header needs 8 more, so the abort condition is `file_pos + 9 > riff_limit`. Applied
  consistently at all four sites (form-type end, zero-length chunk, chunk-data end, chunk-pad end).
- `decoded_page` cannot exceed 31 on any accepted ID, and with `full_chunk_len <= 16384` enforced
  for block chunks, `load_offset <= chunk_bytes_read[14:0]` is an exact truncation, not a wrap.
  The residual `chunk_bytes_read < 32'd16384` guard in `STATE_CHUNK_DATA` is now redundant but
  harmless defence in depth.

Fail-closed behaviour holds on every race I could construct:

- `load_error` is tested before the `cpr_download` processing branch, so a byte arriving on the
  error cycle is dropped rather than forwarded.
- On `cpr_download` falling, commit requires `state == STATE_DONE && has_block && !load_valid &&
  !load_error`; a pending un-acked write forces the `load_abort` path instead. Conservative and
  correct.
- Extra bytes after `STATE_DONE` abort; a truncated file aborts because `state != STATE_DONE` when
  download falls; a non-sequential `ioctl_addr` aborts.
- `load_begin`/`load_commit`/`load_abort` are cleared unconditionally at the top of every non-reset
  cycle and set at most once, so all three are genuinely single-cycle.
- The parser and `plus_cartridge_memory` share the same `reset`/`cold_reset` net in `Amstrad.sv`
  (lines 1145 and 1167), so a mid-download reset cannot leave the memory service holding orphaned
  partial state with no abort notification.

See N3 and N4 for the two untested corners.

### 4. `c4c3e0f` — test-only F4 counter equality vectors

**Verdict: CLEAR.**

Covered by the item-1 analysis: these vectors are the ones I checked line-by-line against pp.75-76
and pp.92-93. Every expectation is derived from a quoted rule, the ACCC section is cited beside it,
and `t07k`'s expectation reproduces the Compendium's own worked example rather than the
simulator's output. No echo-the-simulator pattern anywhere in `t07a`-`t07l`.

### 5. `90aed07` — t20a-t20h R12/R13 video-pointer reload vectors

**Verdict: CLEAR, with one labelling observation (O1).**

The load-bearing check here is page 129 (§13.8.3, "1 µsec FRAMES (R0=0)"), which is three
chronogram blocks and nothing else. The extracted text layer interleaves the CRTC 0, CRTC 1 and
CRTC 3/4 blocks into a single stream and, read that way, appears to show a CRTC-0 case where
`OUT R13,4` takes effect under the R0=0 freeze — which would have been a real divergence.
Rendering the page shows the opposite and confirms the implementation:

- **CRTC 0 block**: C4 runs `0 0 0 0` then sticks at `1` (the last hiccup), C9 stays 0, and the
  offset row stays 0 across the whole trace under both `OUT R0,0` and the later `OUT R0,30`. The
  annotation "R12 / R13 cannot be considered until C4 and C9 both go back to 0" belongs to this
  block. `t20g` and `t20i` pin exactly this.
- **CRTC 1 block**: C4 stays 0 throughout — no freeze, each character is a complete 1 µs frame —
  and `OUT R13,4` *does* take effect, in three sub-traces that vary the write timing. `t20h`
  reproduces this precisely, reloading the updated R12/R13 on every 1 µs line.

The RTL matches: `r0_frozen` is `!CRTC_TYPE`-gated (`crtc_type0_engine.v:299`), so type 1 never
freezes C0, `line_new = hcc_end` fires every character, and `crtc1_row0_reload` reloads at every
`row == 0` line start.

This is a concrete instance of the working rule in `CLAUDE.md`: judging page 129 from its text
layer would have produced a wrong finding against `t20g`.

The `t20g` (cold reset) / `t20i` (live entry) pair asked about in the brief is correctly separated,
and `t20i` carries the ACCC-derived behaviour with the §20.3.1 wrap-edge reload. See O1 for the
labelling nuance on `t20g`.

### 6. `c9f4a4e` / F8 — CRTC 1 separate C5 counter

**Verdict: CLEAR, with one unadjudicated source clause (N2).**

Against §11.1/§11.2.1 p.81 and §11.3.2 p.85:

- The separate C5 with C9 cycling 0..R9 and C4 incrementing at each wrap is implemented at
  `crtc_type1_engine.v:344-355` (the `c5_next` block) and `:366` (`row_new = line_new &
  line_row_event`, with `line_row_event = in_adj ? (line_last_w | crtc1_adj_end_eff) : …`).
  `t08i` reproduces the R4=10/R5=16/R9=3 worked example's full 16-line C4/C9/C5 table.
- The R5=0 hardware bug: "if R5 becomes zero during additional management, the state is not
  deactivated, C4 does not return to 0 and C5 loops … Thus, if C5+1 reaches an R5>0, then the
  additional management changes C4 to 0 before deactivating its state." Implemented as
  `crtc1_adj_end = … & (|crtc1_rollover_r5)` (`:261`) — equality against C5+1 that R5=0 can never
  satisfy — with `t08j` covering the C5 free-run, the C5 31→0 wrap, and the exit via a reachable
  R5=8.
- §11.2.4 p.84 reload ("If C4=0 before the additional management, then VMA is updated with R12/R13
  and not VMA', and this as long as C4=1"): `crtc1_adj_entry_from_row0` / `crtc1_adj_row1_reload`
  (`:452-456`), pinned by `t08l` with a discriminating fixture (VMA'=0x30A0 vs R12/R13=0x0111 on
  the C4=2 line).
- The VSYNC comparator substitution (`vsync_line_fire`, `:513`) correctly excludes the
  adjustment-ending line, where C4 goes directly to 0.

`t08k` is a genuine type-0 control: C4 frozen at R4+1 for the whole adjustment while C9 counts to
R5-1, which is the documented type-0 behaviour and the right negative for the F8 claim.

### 7. `accuracy/f7-rfd` — type-1 F7 R5-trigger RFD, A1, A2

**Verdict: CLEAR for A1 and A2; the RFD area carries N1.**

**A1 (adjustment-ending VSYNC).** Derived on paper against the identification program R4=36, R9=7,
R5=16 and confirmed in the RTL: adjustment entry advances C4 to 37; the C9==R9 wrap at C5=7
advances it to 38; the C5=15 end takes C4 from 38 directly to 0 via `row_frame_last`. C4 never
reaches 39. `vsync_line_fire` substitutes `row + 1` only while `in_adj && !crtc1_adj_end`, so the
substitution can only ever have mattered on the ending line — the exclusion is exactly right, not
merely sufficient. `t08g` (R7=39 silent) and `t08h` (R7=40 silent) are correct, and the `t08g` flip
is an oracle correction, not a weakened assertion. The §28.1.1 p.292 tension is properly logged as
author question 17.

**A2 (§11.2.4 exact-edge caveat pair).** The p.84 note is implemented as
`r4_positive_write_at_adj_entry` gating `crtc1_adj_entry_from_row0` (`:448-453`), with the correct
asymmetry: an R4 write with `|DI[6:0]` on the exact `hcc_last` entry edge suppresses the C4=1
R12/R13 reload; an R9 write on the same edge does not. `t08n`/`t08o` share one fixture and
discriminate on MA (0x1238 vs 0x2050), which is the right shape for an exact-edge pair.

**F7 R5-route RFD.** The two-flag model (`rfd_vma_flag` for the VMA source, `rfd_parity_flag` for
IVM parity management) matches the two statuses §11.6 p.87 enumerates. The same-edge arming using
stored R5 plus live DI (`rfd_arm`, `:257`) correctly makes the newly armed state participate in its
own rollover. `t13a` is a proper directed never-triggered proof. The R1>R0 bare-C9 disarm
(`rfd_r1_gt_r0_disarm`) matches p.87's explicit "if the condition C0=R1 is not met (because R1>R0),
then the condition C9=R9 is enough to deactivate the update of VMA with R12/R13".

**The gap is N1**, below: the §13.7.1.2 R0-widening route bypasses F17's C9=R9 disable.

### 8. `plus/p1-followups` — GA40010 tooling (Q1) and R3l=0 collision (t04i)

**Verdict: CLEAR.**

Empirically re-verified rather than taken from the prior record:

- `make -C rtl/GA40010` builds clean, exit 0, with only the pre-existing MULTIDRIVEN warning on
  `assign SYNC_N = ~(VSYNC_O ^ HSYNC_O)`.
- `VERILATOR_BIN` override works in both forms — `make VERILATOR_BIN=/tmp/fake-verilator` and
  `VERILATOR_BIN=/tmp/env-verilator make` both put the override in the recipe. The `unexport
  VERILATOR_BIN` line is load-bearing and correctly commented: without it the variable leaks into
  the invoked Verilator wrapper, where it is also an internal name, and can resolve recursively.
- `-Wno-fatal` is the intended non-fatal policy for a frozen netlist recreation, and this target
  is deliberately outside the `make -C sim` gate chain, so it cannot mask a production lint
  regression. The five GA40010 sources are enumerated in the synthesis-path classifier (F-C
  remediation from the prior pass), so an edit to them still triggers Tier B.

`t04i` is correctly disciplined. Its comment states plainly that "§14.5 p.141 establishes that type
3's R3l=0 encoding produces a 16-character HSYNC, but does not state whether the §15.3 end/start
collision extends that pulse", and labels the choice "an explicitly unverified model assumption,
not an ACCC-derived expectation". The labelling is consistent across `asic_video.v`,
`docs/plus/architecture.md`, `docs/current-status.md` and `docs/review-debt.md`. This is the right
pattern, and O1 recommends applying it to `t20g`.

### 9. Open review-debt row — `hotfix/implicit-rgb-net`

**Verdict: the wiring fix is CORRECT. The root cause remains unguarded (N5).**

The three things the debt row asked a reviewer to check hardest, each verified:

1. **`b4`/`g4`/`r4` driven only by the motherboard ports.** Declared once as `wire [3:0] b4, g4,
   r4;` at `Amstrad.sv:1388`; the only driver is the `Amstrad_motherboard` instance's
   `.red(r4)`/`.green(g4)`/`.blue(b4)` at lines 1267-1269. No second driver anywhere.
2. **Consumed only by `color_mix`'s `[1:0]` inputs and the nibble expansion.** `color_mix` takes
   `r4[1:0]`/`g4[1:0]`/`b4[1:0]` (lines 1401-1403); `R_plus_raw`/`G_plus_raw`/`B_plus_raw` take the
   full nibble (lines 1418-1420). No other consumer.
3. **No stale reference to the old names.** A regex sweep for bare `r`, `g`, `b` net declarations
   and uses in `Amstrad.sv` returns nothing. The rename is complete.

The classic path is bit-for-bit preserved: `Amstrad_motherboard.v:649-651` drives
`red = plus_mode ? plus_rgb_r : {2'b00, ga_red}`, and `ga_red`/`ga_green`/`ga_blue` are the 2-bit
`{level, OE_N}` netlist pairs, so `r4[1:0]` reaching `color_mix` is exactly the old `r`. The Plus
path takes the 4-bit ASIC level through the `{nibble, nibble}` ×17 expansion, matching the stated
P2 intent, and `video_mixer` selects between them on the same `plus_mode` signal the motherboard
uses. The fix is right.

What is **not** closed is why the bug shipped. See N5.

---

## Findings by severity

### BLOCKING

None.

### NON-BLOCKING

#### N1 — The two CRTC-1 RFD trigger routes disagree about §11.6.1 p.88

`rtl/crtc_type1_engine.v:416-419`, vector `t13g`.

ACCC v1.10 §11.6.1 p.88: "**A RFD triggered on the last line C9=R9 disables the state allowing VMA
to be updated with R12/R13.**" Finding F17 implements this — but only for the R5 route:

```verilog
wire rfd_vma_disarm_hit = rfd_arm & (line == crtc1_line_max);
wire rfd_vma_arm        = (rfd_arm & (line != crtc1_line_max)) | rfd_r0_arm;
```

`rfd_r0_arm` — the §13.7.1.2 R0-widening route — arms the VMA-source flag unconditionally, with no
C9==R9 test. That route can fire on a line where C9==R9: `rfd_r0_cancelled = (line !=
crtc1_line_max) | (row != R4_v_total)`, so cancelling the last-line condition by rewriting **R4
alone** leaves C9 still equal to R9 at the extended line's end, and the RFD arms there.

`t13g` (`test_type1_rfd_r0_widen_r4_cancel_arms_and_advances_c4`) is precisely that case — its own
comment says "At the extended end C9==R9 still holds" — and it asserts `expect_type1_rfd_state(…,
true, true, false)`, i.e. the VMA-source flag **armed**, plus the resulting R12/R13 reload
(`expect_ma(…, 0x1234)`). `t13n` asserts the opposite outcome for the same C9==R9 condition on the
R5 route. The suite therefore currently pins both readings.

This is a stale integration assumption, and the history shows it directly: the R0-widening route
landed in `95fff05` (2026-08-23) and F17 in `0718247` (2026-08-27), which touched only `rfd_arm`.
`docs/accuracy/audit-findings.md:374` states F17's rule as scoped to "$R_5$ written 0 $\to$ nonzero
at $C_0=R_0$", narrower than the p.88 sentence it cites — which is how the R0 route escaped the
sweep (see O2).

There is no hardware evidence either way, and §13.7.1.2's own wording ("this paradox will generate
an RFD state, which will be triggered at the end of the line") does not say which C9 the disable
test sees. But the model should not hold two contradictory readings without a recorded rationale.

*Recommended:* either extend `rfd_vma_disarm_hit` to `(rfd_arm | rfd_r0_arm) & (line ==
crtc1_line_max)` and re-derive `t13g`'s VMA expectation, or record an explicit adjudication (author
question + named residual + `t04i`-style relabelling of `t13g`) explaining why the R0 route is
exempt. Either way the divergence should stop being silent.

#### N2 — §11.3.2 p.85's "C4 continues to be compared to R4" clause is unimplemented and unadjudicated

`rtl/crtc_type1_engine.v:261/359/361`, vector `t08j`, digest
`docs/accuracy/compendium-01-counters.md:218`.

The source, verbatim from p.85:

> But if R5 becomes zero during additional management, the state is not deactivated, C4 does not
> return to 0 and C5 loops. **C4, however, continues to be compared to R4 to process the change
> from C4 to 0.** **The additional management, however, remains activated.**

Two readings are possible. The literal one is that the ordinary C4/R4 comparison stays live during
the stuck adjustment, so C4 returns to 0 when it next reaches R4 — while additional management
stays active and C5 keeps looping. The weaker one is that the sentence merely restates which
comparator would normally schedule the reset.

The RTL implements neither explicitly; it implements the weaker one by omission. During `in_adj`
with R5=0, `frame_adj_CRTC1` is gated on `~in_adj`, `crtc1_adj_end` is gated on
`(|crtc1_rollover_r5)`, and `crtc1_row_frame_last` is therefore permanently 0, so `row_next = row +
1` free-runs C4 through 127 and wraps by 7-bit overflow. Under the literal reading, C4 would
instead cycle with period R4+1. That is observable through R6 and R7 comparisons.

Two problems compound here. The digest at `compendium-01-counters.md:218` paraphrases the clause as
"C4 still compares against R4 for its own logic, but adjustment-active persists", which drops the
"to process the change from C4 to 0" that carries the whole content. And `t08j` cannot discriminate:
it runs R4=10 with C4 from 11 to 19, never approaching a second C4==R4, so both readings pass.

*Recommended:* raise an author question on the clause, sharpen the digest line to quote it, record a
named residual in the RTL at `crtc1_row_frame_last`, and — if the literal reading is adopted or the
ambiguity is left open — extend `t08j` with the C4-wraps-to-R4 case so whichever choice is made is
pinned rather than accidental.

#### N3 — CPR parser: the upper RIFF-length bound has no vector

`rtl/plus/plus_cpr_parser.v:104`, `sim/plus/plus_cpr_parser_test.cpp`.

`riff_len_valid` rejects `full_riff_len > 32'h01FFFFF7`. That bound is what keeps `riff_limit =
full_riff_len[24:0] + 8` from wrapping the 25-bit register, and a wrap would make
`chunk_extent_exceeds_riff` pass for chunks that run off the end of the addressable window. The
suite tests the lower bound (`riff_len = 2` aborts) but never the upper one — `build_cpr_image`'s
`override_riff_len` is only ever called with 2, 10 and 100.

This is a bounds guard on untrusted external input with zero coverage. Nothing suggests it is
wrong; it is simply the one arithmetic guard in the parser that no test would notice losing.

*Recommended:* add a case with `override_riff_len = 0x01FFFFF8` asserting abort, and optionally one
at `0x01FFFFF7` asserting the boundary value is accepted.

#### N4 — CPR parser: a zero-length `cbNN` chunk commits an empty cartridge

`rtl/plus/plus_cpr_parser.v:349` and `:393`.

`has_block` is set at `STATE_CHUNK_ID` as soon as a valid `cbNN` ID is decoded, before the length is
known. A chunk with `full_chunk_len == 0` then skips `STATE_CHUNK_DATA` entirely. A file consisting
of a valid RIFF/Ams! header plus one zero-length `cb00` therefore reaches `STATE_DONE` with
`has_block` set and no writes at all, and commits.

The downstream effect is bounded — the service's clear sweep zero-fills, so the cartridge presents
as an all-`00` ROM 0 and the CPU runs a NOP sled — but it is a fail-open corner in a parser whose
stated policy is fail-closed, and it is untested.

*Recommended:* set `has_block` only once a block chunk has actually delivered a byte (or gate
commit on a nonzero write count), and add the zero-length-`cbNN` case to
`test_short_and_oversized_blocks`.

#### N5 — The implicit-net CI guard the hotfix's own postmortem queued is still not implemented

`docs/ci-testing-policy.md:20-33`, `.github/workflows/build.yml`.

The policy document records, under Tier A "Queued hardening (2026-08-26 hardware session)", that
the black-screen bug shipped because no gate elaborates `Amstrad.sv` and Quartus reports implicit
nets only as Warning 10236. It names two guards, "cheapest first", the first being: fail the
synthesis job when the compile log contains `Implicit Net warning`.

Neither workflow implements it. `build.yml:254` greps the compile log for `Ignored assignment:` —
so the post-compile log-guard pattern exists and works — but there is no 10236 check, and
`local-build.yml` has neither. The full option (Verilator elaboration of `Amstrad.sv` with
IMPLICIT/UNDRIVEN fatal) is also absent, as expected given its stated stub prerequisites.

The wiring fix is correct, but the class of bug it fixed is still undetectable by any gate. Since
`Amstrad.sv` is the top level, the next port rename has the same failure mode and the same
detection latency: a hardware session.

*Recommended:* implement guard 1 in both `build.yml` and `local-build.yml` alongside the existing
`Ignored assignment:` check. It is a one-line grep, and it is the concrete deliverable the debt row
should close against — not just the three-line wiring fix, which reviews clean on its own.

### OBSERVATION

#### O1 — `t20g`'s cold-reset expectations are model-derived, not ACCC-derived

`sim/sim_main.cpp:4627`.

`t20g` cites §13.8.3 p.129, §13.2.6 p.108 and §20.3.1 p.242 for a cold-reset-into-R0=0 fixture. The
p.129 CRTC-0 chronogram and the p.108 table both describe the **live-entry** transition ("when R0
goes to 0"), and the p.108 table's first row explicitly reloads VMA from R12/R13 at that transition.
The cold-reset case — where no rollover edge has ever run, so the §20.3.1 reload never fires and MA
holds its reset value — is a model consequence, not a documented rule. `t20i` (added as review
action A3) carries the sourced behaviour and does pin the reload.

The test's internal comment already reasons correctly about this. The issue is only that the
section citations in the body read as though the cold-reset expectations were derived from them.

*Recommended:* relabel `t20g` in the `t04i` style — explicitly an unverified model choice for the
cold-reset path, with `t20i` named as the sourced twin. No behavioural change.

#### O2 — F17's recorded rule is narrower than the source sentence it cites

`docs/accuracy/audit-findings.md:374`.

F17's rule is written as "On Type 1 (UM6845R), triggering an RFD ($R_5$ written 0 $\to$ nonzero at
$C_0=R_0$) on the last character line of a row where $C_9==R_9$ disables the state…". The p.88
sentence it cites says only "A RFD triggered on the last line C9=R9", with no reference to how the
RFD was triggered — and §11.6 p.87 explicitly notes the R0-widening alternative route in its very
first paragraph. Baking the R5 trigger into the rule statement is what let N1 through the F16/F17
review.

*Recommended:* restate F17's rule in the trigger-agnostic form the source uses, then resolve N1
against it.

#### O3 — `line_last_r` / `row_last_r` / `frame_adj_r` are outside the reset branch

`rtl/CRTC.v:245-320`.

The counters block's `if(~nRESET)` branch clears `hcc`, `line`, `row`, `c5`, `in_adj`, `field`,
`crtc1_adj_from_row0` and the three parity flops, but not the three arbitration latches. They
self-correct on the first CLKEN after reset release, because `hcc == 0` holds there and the load
condition `hcc == 0 && !r0_frozen` fires. Noted only so a future reviewer does not re-derive it as a
finding; no action needed.

---

## Verification Evidence

All three gates rerun independently at the reviewed tip, plus the standalone GA40010 target.

| Check | Result |
|---|---|
| `make -C sim` | **Pass.** Classic suite: `Summary: 172 passed, 0 xfailed, 0 xpassed, 0 failed`. All Plus suites green: `plus_mmu`, `plus_cpr_parser`, `plus_cartridge_memory`, `sdram_cartridge`, `dandanator_loader_bounds`, `p0_boot`, `p1_mobo_bench`, `p1_pixel_phase`, `asic_unlock`, `asic_regs`, `asic_video` (28), `asic_ga_timing_diff`, `asic_pri` (5), `asic_sprites` (14), `asic_dma` (10), `plus_p8` (3), `plus_model_select`. Exit code 0. |
| `make -C sim soak` | **Pass.** `soak: seed 0xaccc5eed20260822, 2845088 characters, 2845088 CLKEN samples` → `soak hash: 0x48146d2b681268ab`. Exit code 0. |
| `make -C sim lint` | **Pass.** Exit code 0. Known warnings only: `UNUSEDSIGNAL` on `GA40010/syncgen_sync.v:30` (`MREQ_N`) and `GA40010/video.sv:28` (`S[7,4:0]`), `SYNCASYNCNET` on the `p1_video_test_top.v` bench `RESET_N`. No new warning class; the CRTC hierarchy and the `Amstrad_motherboard` hierarchy pass both lint. |
| `make -C rtl/GA40010` | **Pass**, exit code 0. Pre-existing MULTIDRIVEN warning on `ga40010.sv:156` only. |
| `VERILATOR_BIN` override (CLI and env) | **Verified working.** `make VERILATOR_BIN=/tmp/fake-verilator -n` and `VERILATOR_BIN=/tmp/env-verilator make -n` both emit the override in the recipe. |
| `git status --short` | Clean apart from this document. |

**ACCC v1.10 pages read directly** (via `pdf-inspector`, `verdict: native_complete`, `pages_needing_ocr: []`, `fallback_pages: []`): 75, 76, 84, 85, 86, 87, 88, 92, 93, 108, 124, 129.

**Pages rendered for figure judgement**: 129, at 160 dpi. Required, not optional — the page is
three per-type chronogram blocks and its text layer collapses them into one stream, which produces
a false CRTC-0 reading. This is recorded because it is a live instance of the `docs/accuracy/extract/README.md`
protocol earning its keep.

---

## Standing caution

Green gates verify the vectors, and the vectors verify against the ACCC. The ACCC is the working
oracle, not the final authority. N1 and N2 both turn on readings of source sentences that only a
SHAKER session or a hardware capture can settle; what this review asserts is that the model
currently holds *inconsistent* readings (N1) and an *unrecorded* one (N2), which is a defect
regardless of which reading hardware eventually confirms.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot (CC BY-NC-ND).
