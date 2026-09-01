# F10 interlace (IVM) parity work — independent review

**Reviewer:** Claude Opus 5 (fresh session, no part in authoring the diff)
**Date:** 2026-08-25
**Branch:** `accuracy/f10-fixtures`
**Range reviewed:** `d74cd10..3a2293a` (commits `20eb6d5`, `657ccde`, `3a2293a`), plus the
docs commits `ea079d1` / `c11f4cb` and the CI commit `62db6c1` as secondary scope.
**Files:** `rtl/CRTC.v`, `rtl/crtc_type0_engine.v`, `rtl/crtc_type1_engine.v`,
`sim/sim_main.cpp`, `.github/workflows/build.yml`, `docs/ci-testing-policy.md`.

**Source used:** `docs/ACCC1.10-EN.pdf`, SHA-256 verified against
`docs/accuracy/extract/README.md` (`1bd6f0e3…f1e33a560`). Every rule below was derived from
the PDF first and only then compared with the RTL and the author's notes. Table and
chronogram pages were judged from the 200 dpi renders in
`docs/accuracy/extract/pages/`, per the extraction protocol; the pdf2md text layer was used
only to cross-check digits already read from a render.

---

## Verdict: **NOT CLEAR**

Two blocking findings, both inside the newly added F10 behaviour. Nothing pre-existing
regresses: the non-IVM reduction of the type-1 row-end split is behaviour-preserving apart
from one dead-state detail (N-4), the type-0 `t09g` RA re-derivation is correct, and all
three gates pass exactly as the commit messages claim.

The type-0 half of the work (commit `3a2293a`) verified cleanly against every rule I could
check. The type-1 half (commit `657ccde`) has one rule inverted against the pp.210-211
panels and one internally inconsistent parity update.

| ID | Severity | Summary |
|---|---|---|
| **B-1** | blocking | The type-1 *leaving* stage A must write C9.0; the RTL holds it for one character, and four `t21` vectors assert the hold. All four panels that can distinguish show the write. |
| **B-2** | blocking | During type-1 IVM, C9 restarts at a *toggled* ParityC9 at a frame boundary while the ParityC9 flop is *not* toggled. Internally inconsistent under either reading of the source; no vector covers it. |
| N-1 | non-blocking | The leaving stage B writes ParityC9 with a value §19.5.3 does not specify. Currently masked by every consumer path; unasserted. |
| N-2 | non-blocking | No deterministic vector pins the type-0 C9.VMA path to `RA`. Deleting the whole mux leaves 140/140 green. |
| N-3 | non-blocking | `t22` asserts the internal C9 only, never the C9-VMA column the tables are actually about. |
| N-4 | non-blocking | Type-1 `line_last` changed meaning inside vertical adjustment; the "outside IVM all three reduce to the previous behaviour" claim is very slightly overstated. |
| N-5 | non-blocking | `t22_configure`'s "R6=63 = frozen even" comment is wrong about the mechanism (R6 == R4, so C4 does reach R6). |
| N-6 | non-blocking | Type-0: `OUT R8,3` followed by `OUT R8,0` in one line leaves that line flagged as an *entering* switch line. |
| N-7 | non-blocking | Type-0: under the R0=0 freeze the seam fires every CLKEN, wiping a toggle line's status. |
| N-8 | non-blocking | Exit tables 1 and 2 on each of pp.223-224 (exit at C9=0 and C9=1) are not covered by any vector. |
| N-9 | non-blocking | `RA` no longer ORs `field` for R8=1 (INTERLACE SYNC). Correct per the source, but an un-flagged change outside IVM with no vector pinning it. |
| N-10 | non-blocking | The new hidden state `tog_enter` / `tog_enter_line` is outside the soak projection. |
| N-11 | non-blocking | `REF_TYPE` is now a dead env var in `build.yml`. |
| N-12 | non-blocking | The CI commit's per-stage "identical to clean runs" claim is not substantiated by the policy doc, which compares totals only. |
| N-13 | non-blocking | The blanket "the panels' drawn Parity rows are internally inconsistent" dismissal discards a check that in fact validates 14 of the 16 panels. |

---

## Gates

Run at reviewed tip `62db6c1` with a clean working tree, after every bite-test was reverted.

```
$ make -C sim
Summary: 140 passed, 0 xfailed, 0 xpassed, 0 failed

$ make -C sim lint
- Verilator: Built from 0.292 MB sources in 8 modules, into 0.115 MB in 3 C++ files
[exit 0, no warnings]

$ make -C sim soak SOAK_EXPECT=83e80134f7705b46
soak: seed 0xaccc5eed20260822, 2845088 characters, 2845088 CLKEN samples
soak hash: 0x83e80134f7705b46
soak hash matches expected
```

All three match what the commit messages claim.

---

## Blocking findings

### B-1 — the type-1 leaving stage A does write C9.0; the RTL holds it

**Where:** `rtl/crtc_type1_engine.v:341` (`line_poke`), its comment at lines 155-160, and the
assertion at `sim/sim_main.cpp:4640-4641`.

**What the source says.** p.209 gives the 3rd-µs rule once, for both directions of the
toggle: *"On the 3rd µsecond when R8 changes from 3 to 0 **or vice versa**: … ParityC9 = C9.0;
ParityC9 = ParityC9 xor (C4.0 and not (R9.0))"*. The 4th-µs rules are then split into an
entering case and a leaving case, and the leaving case contains **only**
`ParityFrame = ParityC9` — no C9 assignment at all.

**What the panels show.** Reading p.210 and p.211 from the renders (including the
`p210_test17_zoom`, `p210_test2_zoom`, `p211_test1_zoom`, `p211_test27_zoom` crops), each
`OUT R8,x` occupies four C0 cells: the two-cell label box (µs 1-2), one dark-green cell
(µs 3), and the `on` / `off` cell (µs 4). Measured against the C0 header, for every panel
the entering write sits at C0 = 3,4 / 5 / 6 and the leaving write at C0 = 7,8 / 9 / 10.

Four panels have `X = C4.0 and not R9.0 = 1` *and* a stage-A value that differs from the
value already in C9.0, so only these four can distinguish a leaving-stage-A poke from a
leaving-stage-B poke. All four show C9.0 changing in the **dark-green (3rd µs) column**,
one character before the `off` cell:

| Panel | page | C9 row, C0 = 7 8 **9** 10 11 | leaving stage A |
|---|---|---|---|
| Test 4(D), 8(H) | 210 | 1 1 **0** 0 0 | C9.0 1 → 0 at µs 3 |
| Test 2(B), 6(F) | 210 | 1 1 **0** 0 0 | C9.0 1 → 0 at µs 3 |
| Test 12(L), 14(N) | 211 | 1 1 **0** 0 0 | C9.0 1 → 0 at µs 3 |
| Test 1(A), 10(J2) | 211 | 0 0 **1** 1 1 | C9.0 0 → 1 at µs 3 |

In each case the new value equals the stage-A formula `C9.0 xor X`, and it equals the
panel's own `ParC9=` callout for that same column. The entering side of the RTL matches the
panels cell-for-cell (Test 2(B): C9.0 = 0 at µs 3 from stage A, then 1 at µs 4 from stage B),
which pins the fixture's timing anchor and rules out a whole-sequence offset — the
divergence is specifically on the leaving write.

**What the RTL does.** `line_poke = (stage_a_edge && tog_enter) || stage_b_edge` suppresses
the poke on a leaving stage A, so C9.0 takes the new value one character late, at stage B.
The final value is right; the transition is one character late. Since C9 is `RA`, that is
one character cell of visibly wrong raster address on screen — exactly the observable the
SHAKER 22C/3 panels exist to capture.

The engine comment at lines 155-160 asserts the opposite of the panels ("the panels hold
C9.0 through the `off` column"), and it also contradicts the fixture's own header comment at
`sim/sim_main.cpp:4534-4535`, which states the rule correctly and unconditionally
(`3rd us … ParityC9 := C9.0 xor (C4.0 and not R9.0); C9.0 := ParityC9`). The fixture's
header is right; the RTL and the assertion are wrong.

**Fix.** Drop `&& tog_enter` from `line_poke`, and change
`sim/sim_main.cpp:4640` from `expect_line_parity(… " off stage A C9.0 held", stage_b_pc9)` to
`expect_line_parity(… " off stage A C9.0", off_a)`. `../f10-implementation-notes.md:44-46` needs
the same correction.

**Evidence it is the assertion, not the RTL semantics, that pins this:** bite-test A below.

### B-2 — ParityC9 and C9's restart parity desynchronise at a type-1 IVM frame boundary

**Where:** `rtl/crtc_type1_engine.v:334` (`c4_increment_toggle`) against `:299` (`line_next`).

§19.8.2 p.225 gives one match branch, and it does two things together:

```
C4 management (C4++ or C4 = 0 if C4 == R4)
ParityC9 = ParityC9 xor (not r9.0)
C9 = ParityC9
```

The RTL implements the `C9 = ParityC9` half unconditionally —
`line_next = ivm_row_end ? pc9_toggled : c9_ivm_step` with
`pc9_toggled = R9.0 ? parity_c9 : ~parity_c9` — but gates the `ParityC9 = ParityC9 xor …`
half on `!frame_new_w`, i.e. suppresses it precisely on the `C4 = 0 if C4 == R4` arm of the
same branch.

The result at a frame boundary with even R9 and R5 = 0: `line[0]` is loaded with
`~parity_c9` while the `parity_c9` flop keeps its old value. The two disagree from that
edge on. Because `c9_ivm_step` preserves bit 0 through the row, the desync survives the whole
row; the next (ordinary) row end then toggles `parity_c9` to the value C9 already carries, so
**rows 0 and 1 of every frame run with the same C9 parity** and the documented per-C4
alternation slips one row at each frame boundary.

This is a defect under either reading of the source, which is what makes it decisive:

- If p.225's pseudocode governs, the flop should toggle too and the fix is to drop the
  `!frame_new_w` gate.
- If p.209's prose ("ParityC9 is reversed with each C4 **increasing** when R9 is peer")
  governs and a frame wrap is not an increase, then `line_next` must use the *un*toggled
  `parity_c9` at `frame_new_w`.

Either way the current code must not use the toggled value for C9 and the untoggled value
for the flop. The engine's own comment at lines 207-210 cites p.225 and describes the
match branch as "restart C9 from the toggled ParityC9", so p.225 is the rule the author
intended to implement.

No vector covers it (bite-test D). `t21` never runs IVM across a frame boundary — every
panel fixture sets R4 = 63 and stays inside one row of one frame — and `t22` is type 0.

---

## Non-blocking findings

**N-1 — leaving stage B writes an unspecified ParityC9.** `pc9_write` fires on
`stage_b_edge` regardless of direction, with
`pc9_value = stage_b_pc9_value = parity_frame ? parity_c9 : stage_b_x`. §19.5.3's leaving
4th-µs rule is only `ParityFrame = ParityC9`. When ParityFrame is even and `X = 1` and the
stage-A result differs from `X`, the RTL overwrites ParityC9 with `X`. Worked case (panel
2(B), 6(F) shape): stage A leaves ParityC9 = 0, the RTL then writes 1. `pf_value` reads the
pre-edge flop so ParityFrame is still correct. I could not find a path that makes the wrong
value observable — the next entering stage A always overwrites it from C9.0, and a live type
switch re-seeds the type-0 copy at the first `type0_ivm_turn_on` seam — but the flop's
documented meaning is wrong between those points and nothing asserts it (bite-test E).
Recommended: gate the stage-B ParityC9 write on `tog_enter`.

**N-2 — the type-0 C9.VMA → RA path is unpinned by the deterministic suite.** Replacing
`assign RA = CRTC_TYPE ? line : (e0_ivm_disp ? e0_line_vma : line)` with `assign RA = line`
in `rtl/CRTC.v:90` — deleting the entire §19.8.1 address-visible value — leaves
140 passed / 0 failed. Only the soak notices. The C9.VMA value survives in the limit test
(`type0_limit_value`), which is why `t22` still fails a broken `line_vma`; but the *output*
mux itself rests on a hand-minted hash.

**N-3 — `t22` asserts C9, never C9-VMA.** `t22_walk` calls `expect_c4` and `expect_line`
only. The pp.221-224 tables have three columns and the C9-VMA column is the reliable one
(the C9 column carries the flagged drawing quirk in the settled `C4 >= 1` blocks). Adding one
`expect_ra` per step would both close N-2 and assert the column the tables are actually
about. This is the single highest-value follow-up in this list.

**N-4 — type-1 `line_last` changed inside adjustment.** Old: `assign line_last = line_last_w`
unconditionally. New: `assign line_last = line_limit_match`, which is `crtc1_adj_end` when
`in_adj`. I checked the other two members of the split algebraically and they *are* exact:
`line_row_event` reproduces the old `row_new` term in both branches, and
`line_row_structure_last` reduces to `line_last_w` everywhere with IVM off, so the reload /
save / VSYNC / adjustment-entry consumers are untouched — `t08i`/`t08j`/`t08l` and
`t13a`-`t13d` are green for the right reason, not by luck. `line_last` itself feeds only
`line_last_r` / `frame_adj_r` in the wrapper, and `frame_adj_r` masks it with `~in_adj`, so
the change is visible only through `line_last_r`, which only the *type-0* engine reads.
Observable only on a live `CRTC_TYPE` 1 → 0 switch during type-1 vertical adjustment. Not a
bug, but "outside IVM all three reduce to the previous behavior" (commit `657ccde`) is
marginally stronger than what holds.

**N-5 — `t22_configure`'s R6 comment.** The comment says R6 = 63 gives a "frozen even"
ParityFrame. With R4 = 63 that is R6 == R4, not R6 > R4, so `pr6_write` does fire when
`row_next == 63` and the freeze rule of §19.5.2 does not apply. The even fixtures are correct
only because they never cross a frame boundary (R4 = 63, R9 = 6 ⇒ 448 lines; the longest
walk is ~25). Either use R6 > R4 or fix the comment.

**N-6 — type-0 double write inside one line.** `r8_toggle_write_t0` compares `DI[1:0] == 3`
against `ivm_disp_r`, the *line's* latched mode. `OUT R8,3` then `OUT R8,0` in one line
therefore leaves `tog_line = 1, tog_enter_line = 1`, so that line's limit target keeps the
`R9 or ParityFrame` entering form even though R8 ended at 0. Unpinned in the source and
undocumented in the engine; the type-1 engine documents its equivalent case (back-to-back
toggle writes) but the type-0 one is silent.

**N-7 — the type-0 seam under an R0=0 freeze.** `type0_seam = CLKEN && (hcc == 0)` fires on
every CLKEN while C0 is pinned at 0, so `tog_line` is cleared immediately and a toggle write
landing during the freeze loses its switch-line status. Edge case on top of an already
special-cased configuration; worth one line of comment.

**Same-edge seam race (asked for explicitly).** A write landing exactly on the seam edge:
`ivm_disp_r` takes the *pre-edge* R8 (nonblocking), so the doubling starts one line later;
`tog_line` is set to 1 by the later assignment in the same block, so the new line is treated
as the switch line. That is self-consistent with p.219 ("the calculation … will be performed
on the next C0=0, after the C9/R9 test of the line") and with p.220 ("the state set with R8 is
considered when C0 returns to 0"). The one asymmetry is that the seam *capture*
(`type0_seam_target_parity`) reads the pre-edge `tog_line`, i.e. the previous line's status,
while the live comparison during the new line uses the new write's — but the captured value
only governs the frame's last line (`type0_last_line_armed`), so the two never contradict on a
line that consumes both. **I judge the implemented behaviour defensible**; the case is not
pinned by the source and the code comment says so.

**N-8 — uncovered exit tables.** Of the eight tables on pp.223-224, the four vectors cover
exit at C9 = 2 and C9 = 3 on both parities. Exit at C9 = 0 (tables 1) and C9 = 1 (tables 2)
are uncovered on both pages.

**N-9 — RA and INTERLACE SYNC.** Removing `line | (field & interlace[0])` also removes the
field OR for R8 = 1, not just R8 = 3. That is *correct* — INTERLACE SYNC offsets VSYNC by half
a line and does not touch the raster address — but it is a behaviour change outside IVM that
neither commit message calls out, and no vector configures R8 = 1 to pin it.

**N-10 — unsampled new hidden state.** `tog_enter` and `tog_enter_line` are outside
`soak_mix_sample`. Both are always written together with a state that *is* sampled
(`tog_stage`, `tog_line`), so neither can go stale unobserved, but the projection comment
claims the stage machinery joined the sample and only two thirds of it did.

**N-11 / N-12 — CI commit `62db6c1`.** The workflow is coherent: `cache_enabled` is gone from
the classifier outputs and from both consumers, the restore/save steps are gone, and the
`build_mode` plumbing (`synthesis.outputs.build_mode` → gate summary) is intact, so the report
shape claim holds. `REF_TYPE` at line 50 is now dead — its only consumer was the deleted
`clean_quartus_build` branch. The policy-doc rewrite matches the commit message on the
substance (no design partitions, `Netlist Type Used: Source File`, fitter-dominated 10:06
flow, reintroduce only with a partitioned incremental policy). One imprecision: the commit
says the per-stage split is "identical to clean runs", while the doc substantiates only equal
*totals* ("the same ~12 minutes either way") and presents the per-stage split from the
restored run alone. I cannot verify Actions run `32657783842` from this worktree, so the
measurement itself is taken on trust.

**N-13 — the Parity-row dismissal is broader than the evidence.** The fixture header and the
`657ccde` message dismiss the panels' drawn `Parity` rows wholesale as "internally
inconsistent by one character on the ODD page". They are not, mostly. Reading them as
*effective* parity — `IVM ? ParityC9 : ParityFrame` — reproduces the drawn row exactly in
14 of the 16 panels, including every panel on p.210 and six of eight on p.211 (verified
cell-by-cell on Tests 1(A), 12(L), 16(P), 26(Z), 28(ZB), 11(K2)). Only Test 27(ZA)/29(ZC) and
Test (M)/15(O) drop one cell early, at the on-write's µs 3 instead of µs 4 — a genuine but
local drawing slip. The dismissal is therefore true in the narrow sense and misleading in the
broad one: the Parity rows are a working third consistency check that was set aside.

---

## What I checked and found correct

Recording these so a later pass does not redo them.

**Type-1 stage formulas (p.209).** `stage_a_pc9 = line[0] ^ (row[0] & ~R9[0])` is the 3rd-µs
rule verbatim. Stage B is exact in both directions, including the non-obvious reduction: with
ParityFrame odd, `ParityFrame := ParityFrame and (ParityC9 xor X)` with the stage-A ParityC9
substituted gives `(C9.0 ^ X) ^ X = C9.0`, so `stage_b_pf_value` reproducing the old C9.0 is
right, and it is what Tests 1(A) (ParFrame stays 1, old C9.0 = 1) and 12(L) (ParFrame → 0, old
C9.0 = 0) show. `pf_value` on leaving is `parity_c9` — `ParityFrame = ParityC9` — correct.
ParityFrame's stage-A hold is correct ("the current parity of the frame is not modified").

**All 16 panel configurations.** I derived every panel from the two p.209 formulas
independently and matched them against the drawn C9 rows and the 64 callouts. Every callout
in the RTL's model is right. The *only* divergence between the source and the implementation
is B-1's poke timing on four panels.

**Type-1 §19.8.2 counting (p.225).** `c9_pre`, `ivm_row_end`, `c9_ivm_step`, `pc9_toggled`
reproduce the pseudocode exactly, including the `C9 = C9+1+(R9.0)` else-branch giving `C9+2`
for both R9 parities and the bit-0-masked comparison.

**Type-0 counting (pp.219-220) and all twelve worked tables.** The two-bit decomposition —
value doubled from `ivm_disp_r`, target parity from the line-scoped toggle status — is a
faithful and, I think, well-chosen factorisation of the three documented forms. I re-derived
all eight entry tables (pp.221-223 top) and all four exit tables under review from
p.219/p.220 alone and every row matches: the switch line testing raw C9 against
`R9 or ParityFrame` (odd-frame entry at C9 = 6 giving the 16-line overflow run to C9 = 19,
VMA = 39 mod 32 = 7); steady lines testing `(C9×2 + ParityC9) mod 32` against
`R9 or ParityC9`; and the exit line testing the doubled value against plain R9
(p.223 bottom-right resetting at VMA = 6, p.224 bottom-right missing at VMA = 7 = R9+1). The
`line_vma = {line[3:0], parity_c9}` truncation is the documented "the more significant bit is
lost".

**`t22` expectations are table-derived, not simulator-derived** — the question explicitly
asked. All four exit walks correspond row-for-row to a specific table, the entry walks match
the tables' overflow runs, and the one place where the tables are surprising (six of eight
run C9 to 7 with R9 = 6 without the C4 increment plain counting predicts) is *not* asserted
and is routed to author question Q19(b) with the reasoning written down. I independently hit
the same puzzle before reading the note. This is the right call and the right way to record
it. The exit fixtures' shape — enter on row 0 line 0, complete that row, exit on a row-1 line
at C4 = 1 — matches pp.223-224 exactly.

**The Q19 correction on the p.219 row-end ParityC9 update.** The pseudocode's guard reads
`If R9.0=0` while its own parenthetical reads "(C9 parity switched if R9 is odd)". p.205 is
decisive and the author's reading is the correct one: for even R9 "the parity is identical
regardless of the value of C4", and the C4-dependent alternation is the odd-R9 balancing
scheme. Leaving odd-R9 unimplemented and named as a residual is the right scope call. Note
that this means the p.219 row-end update is currently absent altogether, so type-0 IVM with
odd R9 is wrong by construction — correctly flagged, but worth keeping visible.

**`t09g`'s RA expectation change 1 → 0.** Re-derived independently and I **agree**. That
fixture runs R4 = 0, R6 = 1, R9 = 0, R8 = 3. R6 > R4 means C4 never reaches R6, so per §19.5.2
p.205 ParityR6 stops updating and ParityFrame stays frozen at its reset value 0. Mechanically
in the RTL: with R9 = 0 and IVM active every line is a row end and every row end is a frame
end, so `pr6_write` (gated on `!frame_new_w`) never fires and `pf_write` writes `parity_r6` = 0
each frame. ParityC9 is seeded from ParityFrame = 0. C9 is frozen at 0, so
RA = C9.VMA = (0×2 + 0) = 0. The old expectation of 1 came from the field OR that §19.8.1
replaces. The assertion's freeze intent is preserved.

**Positional port hazards.** I walked both instantiations against both module port lists
term by term. `crtc_type0_engine` (`rtl/CRTC.v:239-261`) and `crtc_type1_engine`
(`:276-296`) both match in order and width, including the type-1 list's removal of
`R8_interlace` and insertion of `parity_frame, parity_c9` at the end of the input block.
Verilator would catch an arity change but not a same-width swap; there is none.

**XFAIL/XPASS triage.** The staging is coherent across the three commits: `20eb6d5` lands the
`t22` fixtures as 15 xfails against the pre-F10 type-0 core, `657ccde` reports
125 passed / 15 xfailed (type-0 `t22` only) while promoting the 16 `t21` panels to required,
and `3a2293a` promotes the `t22` set, ending at 140/0/0/0. No vector is xfailed at tip, and no
assertion was weakened along the way.

**Fixture setup paths.** `run_to_line_mid` and `run_to_frame_start` are both bounded and both
followed by an `expect_` that converts a bounded failure into a test failure rather than a
silent pass; their ignored return values are therefore harmless. `run_to_c0` is unbounded but
every call site uses a target below the configured R0 (4, 20, 27 against R0 = 63), so it
cannot spin. The `t21` R9 := 0 mid-line setup write is a real ACCC-legal way to reach a state
("C9.0 = 1 with R9.0 = 0" is unreachable under R9 = 0 alone) and the resulting register state
is asserted before the toggle, not assumed.

---

## Bite-tests

Five mechanical reverts, each run against the full suite and each reverted immediately. The
working tree is clean of all of them; the three gates above were re-run afterwards to confirm.

**(A) Remove the `tog_enter` qualification on the type-1 stage-A poke** —
`line_poke = stage_a_edge || stage_b_edge`.

```
FAIL  t21c_type1_ivm_toggle_4D_8H    … off stage A C9.0 held == 0x01, actual 0x00
FAIL  t21d_type1_ivm_toggle_2B_6F    … off stage A C9.0 held == 0x01, actual 0x00
FAIL  t21k_type1_ivm_toggle_12L_14N  … off stage A C9.0 held == 0x01, actual 0x00
FAIL  t21l_type1_ivm_toggle_1A_10J2  … off stage A C9.0 held == 0x00, actual 0x01
Summary: 136 passed, 0 xfailed, 0 xpassed, 4 failed
```

Exactly the predicted four X=1 panels, and no others — the asymmetry is pinned by these four
assertions and nothing else. Decisive for B-1: every "actual" value under the patched RTL
(0, 0, 0, 1) is the value the corresponding panel's C9 row draws in the leaving write's 3rd-µs
column. The patched RTL agrees with the source; the shipped RTL does not.

**(B) Force the type-0 limit target parity to zero** —
`type0_limit_target = R9_v_max_line | {4'b0000, 1'b0}`.

```
FAIL  t22h_type0_ivm_entry_odd_c9_0
FAIL  t22i_type0_ivm_entry_odd_c9_1
FAIL  t22j_type0_ivm_entry_odd_c9_3
FAIL  t22k_type0_ivm_entry_odd_c9_6
FAIL  t22m_type0_ivm_exit_odd_at_r9_plus_1
FAIL  t22n_type0_ivm_exit_odd_below_limit
Summary: 134 passed, 0 xfailed, 0 xpassed, 6 failed
```

The six odd-frame type-0 vectors fail; every even-frame vector and every non-IVM vector stays
green. Correct and expected — on an even frame ParityFrame = ParityC9 = 0, so the target
parity bit is already zero and cannot be distinguished. Confirms both that the mechanism is
covered and that only the odd-frame half of `t22` covers it.

**(C) Delete the type-0 C9.VMA output mux** — `assign RA = line`.

```
Summary: 140 passed, 0 xfailed, 0 xpassed, 0 failed
soak hash MISMATCH: expected 0x83e80134f7705b46, got 0x8e0a737bb4bfd352
```

Removing the entire address-visible value of §19.8.1 breaks no deterministic vector. Evidence
for N-2/N-3.

**(D) Toggle ParityC9 at frame ends too** —
`c4_increment_toggle = row_new && !R9_v_max_line[0]` (drop `!frame_new_w`).

```
Summary: 140 passed, 0 xfailed, 0 xpassed, 0 failed
soak hash MISMATCH: expected 0x83e80134f7705b46, got 0x199fe90537751b85
```

The B-2 frame-boundary behaviour is pinned by no deterministic vector at all. (The soak
samples `parity_c9` directly, so its mismatch demonstrates only that the flop changed, not
that an output did.)

**(E) Stop the leaving stage B from writing ParityC9** —
`pc9_write = stage_a_edge || (stage_b_edge && tog_enter) || c4_increment_toggle`.

```
Summary: 140 passed, 0 xfailed, 0 xpassed, 0 failed
soak hash MISMATCH: expected 0x83e80134f7705b46, got [differs]
```

Evidence for N-1: nothing deterministic asserts ParityC9 after a leaving stage B. Same soak
caveat as (D).

---

## Recommended order of work

1. **B-1** — one-token RTL fix plus one assertion re-derivation; the four `t21` vectors then
   assert the panels rather than the implementation.
2. **B-2** — decide between the p.225 pseudocode and the p.209 prose, make `line_next` and
   `pc9_write` agree either way, and add a type-1 IVM vector that crosses a frame boundary
   with even R9 and R5 = 0 (there is currently none).
3. **N-3 / N-2** — add `expect_ra` to the `t22` walk. One change closes the largest coverage
   gap in the type-0 half and pins the output mux.
4. **N-1**, then the documentation corrections (`../f10-implementation-notes.md:44-46`,
   `crtc_type1_engine.v:155-160`, N-5, N-13) and the CI tidy-ups (N-11, N-12).

The `docs/review-debt.md` row for this branch is accurate and names four of the five areas I
was asked to press hardest, including the one that turned out to be wrong. That row should
stay open until B-1 and B-2 are resolved.

---

## Remediation (2026-08-25, author's thread)

- **B-1 fixed.** `line_poke` lost its `tog_enter` qualification (stage A pokes C9.0 in both
  directions); the engine comment and the `t21` off-stage-A assertion now assert the panels'
  dark-column write. The four X=1 panels are the discriminating vectors.
- **B-2 fixed.** The `!frame_new_w` gate is removed from `c4_increment_toggle`: the p.225
  match branch toggles ParityC9 and restarts C9 as one step at every row end including the
  frame-boundary arm. New vector `t23a` walks a type-1 IVM frame boundary (R9=2, R4=1) and
  asserts C4/C9/RA/ParityC9 continuity across it; it failed against the pre-fix RTL for the
  predicted reason (plus a latent 5-bit width bug the vector exposed: a bare `~parity_c9`
  ternary arm was widened before the invert, producing 5'b11111 — fixed with an explicit
  width-forcing concat).
- **N-1 fixed.** The leaving stage-B ParityC9 write is gated to the entering case; the
  leaving 4th-µs rule is only `ParityFrame := ParityC9`. Bite-test E's scenario is now
  structurally absent.
- **N-2/N-3 fixed.** Every `t22` step now asserts RA (the tables' C9-VMA column, plain C9
  off IVM display); the RA output mux is pinned by 19 vectors.
- **N-5 fixed.** Comment corrected: R6=63 with R4=63 is R6==R4; the even fixtures are safe
  because they never cross a frame boundary, not because of the freeze rule.
- **N-6/N-7 documented.** Named residuals with comments in `crtc_type0_engine.v`.
- **N-8 fixed.** Four new exit vectors `t22p`-`t22s` (exit at C9=0 and C9=1, both parities)
  cover all eight pp.223-224 exit tables.
- **N-9 pinned.** New vector `t23c` asserts RA == C9 under R8=1 (INTERLACE SYNC) on both types.
- **N-10 fixed.** `tog_enter`/`tog_enter_line` joined the soak projection.
- **N-11 fixed.** Dead `REF_TYPE` env removed from `build.yml`.
- **N-12 erratum.** The commit message's "identical to clean runs" over-claimed: the
  substantiated comparison is equal totals (~12.2 min); the per-stage split is recorded from
  the restored run alone. Recorded here rather than rewriting the pushed commit.
- **N-13 accepted.** The notes' Parity-row dismissal is rewritten: read as effective parity
  (`IVM ? ParityC9 : ParityFrame`) the drawn rows validate 14 of 16 panels; only
  27(ZA)/29(ZC) and (M)/15(O) drop one cell early. The rows are a consistency check, not a
  discarded oracle.
- **N-4 noted.** The notes' "bit-identical outside IVM" claim is scoped: `line_last` inside
  type-1 adjustment differs (observable only through `line_last_r` after a live 1→0 type
  switch during adjustment).
- **Additional (self-found during B-2 remediation).** Type-1 `ivm` never engaged when R8=3
  was already programmed at reset or arrived via snapshot load — there is no toggle write to
  run the stage machine. The engine now tracks the register whenever no toggle stage is in
  flight; new vector `t23b` snapshot-loads R8=3 and asserts the §19.8.2 sequence.

Gates after remediation: 147 required passes, 0 xfailed/xpassed/failed; lint exit 0; soak
re-minted `0x83e80134f7705b46` → `0xa9e5026de83d287c` (B-1/B-2/N-1 behavior + two new
sampled fields).

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot (CC BY-NC-ND).
