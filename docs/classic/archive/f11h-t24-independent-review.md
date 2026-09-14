# Independent review — F11h and t24 (branch `accuracy/f11h-and-ivm-vsync-coverage`)

Reviewer: Claude Opus 5, fresh session, cross-provider worker.
Date: 2026-08-25.
Range reviewed: `c3efcff..4e776f1` (five commits).
Files read as authority: `docs/ACCC1.10-EN.pdf` renders at
`docs/accuracy/extract/pages/` (p.206, p.207, p.208, p.225, p.242) plus the
pdf-inspector Markdown layer at `docs/accuracy/extract/pdf2md/accc-v1.10-paged.md`.

---

## Verdict

**NOT CLEAR — one blocking finding (B-1).**

Everything the two behaviour commits set out to fix is correct and correctly
sourced. The p.242 chronogram reading, the p.208 table reading, the p.225
alternation arithmetic, and the R4=6 / R9=8 fixture geometry all hold against
the rendered pages, cell by cell. No existing vector was weakened (the sim diff
removes exactly one line, a relocated `}  // namespace`), both XFAIL→required
promotions landed in their behaviour commits, and both soak re-mints are
justified by a real behaviour change.

The blocking finding is a second, undeclared behaviour change that rode along
with the t24 fix: during type-1 IVM the VSYNC no longer starts at the half-line
position on either frame parity, so the MID-VSYNC that ACCC §19.5.3 p.208
schedules for the even ParityFrame is now absent altogether. No vector covers
it, and `docs/accuracy/f10-implementation-notes.md` now asserts the opposite —
that the MID-VSYNC residual *narrowed*.

### Gate status — NOT RUN in this environment

**This reviewer could not execute the gates.** Every command outside a small
read-only allowlist (`ls`, `grep`, `git log/diff/show/status`, file reads) was
refused by the harness permission layer in this non-interactive session:
`make -C sim`, `/usr/bin/make -C sim`, and even running the prebuilt
`sim/obj_dir/crtc_tests` binary all returned "This command requires approval".

Consequently:

- `make -C sim` (expected: 151 passed, 0 xfailed, 0 xpassed, 0 failed) — **not run**.
- `make -C sim soak SOAK_EXPECT=0xd620fce8b1c05b25` — **not run**.
- The three mechanistic bite-tests — **not run**.

The verdict above is therefore based on static verification only, and is
provisional on the parent reproducing the gates. Static corroboration of the
count claim: the test registry at `sim/sim_main.cpp:5972` holds exactly 151
entries (`grep -c '^        {"t' sim/sim_main.cpp` → 151) and none of them carries
`true` in the `known_divergence` slot, which is consistent with "151 required
passes, 0 xfailed".

The bite-test section below states the exact edits and the predicted failure sets
so the parent can run them mechanically. One of those predictions is itself
evidence for B-1 and is worth running first.

---

## PDF-first verification of the rule claims

### Claim 1 — F11h, §20.3.2 p.242, second CRTC-1 chronogram: CONFIRMED

Judged from the render `docs/accuracy/extract/pages/p242.png`; the pdf2md text
layer (`../extract/pdf2md/accc-v1.10-paged.md:7789-7823`) was used only to confirm the prose and
to prove that the layer flattens the geometry, which is why the render decides.

The page carries four chronograms in two pairs, all with the identical cell
strip `C0: 55 56 57 58 59 60 61 62 63 | 0 1 2 3 4` and the identical
`R12=#10` box over cells 55-57.

| Pair | Chronogram | `OUT R12,#30` box spans | Shaded write cell | OFFSET bar from C0=0 |
|---|---|---|---|---|
| §20.3.1 CRTC 0 | 1 | 61 → 0 | 63 (blue) | `#30xx` |
| §20.3.1 CRTC 0 | 2 | 62 → 1 | 0 (orange) | `#10xx` |
| §20.3.2 CRTC 1 | 1 | 61 → 0 | 63 (blue) | `#30xx` |
| §20.3.2 CRTC 1 | 2 | 62 → 1 | 0 (green) | `#30xx` |

The second chronogram of each pair is the same drawing shifted one character
right, so the register update lands on the C0 63→0 boundary edge itself. The
CRTC-0 pair fixes the semantics of the shaded cell: when the shaded cell is 63
the write is caught (`#30xx`), when it is 0 the CRTC-0 frame load misses it
(`#10xx`). With that semantics fixed, the CRTC-1 second chronogram showing
`#30xx` from C0=0 with the shaded cell at 0 can only mean the type-1 row-0
reload samples the register file as of *after* the boundary edge.

The prose corroborates and cannot by itself discriminate the two chronograms:
§20.3.1 says "The updates of R12 and R13 are considered immediately", §20.3.2
says "Note that the update of both R12 and R13 are immediate" and "On CRTC 1,
**VMA is loaded with R12/R13 while C4=0**". Both prose lines are identical in
force; only the drawn geometry separates the types. The commits' reading is the
correct one.

The commit messages and `../audit-findings.md` describe the box as spanning
"C0=62..1", which matches the render.

### Claim 2 — t24, §19.5.3 p.208 table: CONFIRMED, cell by cell

Judged from `docs/accuracy/extract/pages/p208.png`; the pdf-inspector layer
happens to reconstruct this table as a real Markdown table
(`../extract/pdf2md/accc-v1.10-paged.md:6436-6461`), and it agrees with the render exactly, which
is unusually strong corroboration for a figure.

Column order is `+32 | even C4 | even C9 | left VSYNC | right VSYNC | odd C4 | odd C9`.
R9=8 (even), so each character row is 5 even lines or 4 odd lines.

| Frame line | Even (C4,C9) | Left VSYNC | Right VSYNC | Odd (C4,C9) |
|---|---|---|---|---|
| 0 | (0,0) | ■ | ■ | (0,1) |
| 4 | (0,8) | | ■ | (1,0) |
| 5 | (1,1) | ■ | | (1,2) |
| 9 | (2,0) | ■ | ■ | (2,1) |
| 13 | (2,8) | | ■ | (3,0) |
| 14 | (3,1) | ■ | | (3,2) |
| 18 | (4,0) | ■ | ■ | (4,1) |
| 22 | (4,8) | | ■ | (5,0) |
| 23 | (5,1) | ■ | | (5,2) |

Every left-column box sits on the first line of C4=R7 in the even frame; every
right-column box sits on the first line of C4=R7 in the odd frame. There is no
box anywhere else, and no box is displaced by a line relative to its own frame's
C4 start — that is exactly "no delay correction". For odd R7 (1, 3, 5) the two
boxes sit one physical row apart (the permanent 1-line gap); for even R7 (0, 2,
4) they coincide.

The `+32` markers appear only at even-frame C4 starts (rows 0, 5, 9, 14, 18, 23),
which independently confirms the even-frame row segmentation used above.

The prose above the table is the rule statement and is unambiguous:
"contrary to what was done on CRTC's 0, 3 and 4, there is no specific management
of the **VSYNC** in IVM mode when the number of lines of a character is odd. The
**VSYNC** is not delayed from a line on odd C4s when R9 is even."

Both fixtures pin their table rows correctly:

- `t24a` (R7=1): even frame line 5 asserting C4=1, C9=1 — table box (1,1) ✔;
  odd frame line 4 asserting C4=1, C9=0 — table box (1,0) ✔.
- `t24b` (R7=2): both frames line 9, asserting (2,0) then (2,1) — table row 9 ✔.

### Claim 3 — §19.8.2 p.225 alternation and the R4=6 fixture: CONFIRMED

The p.225 pseudocode (render read directly) is:

```
When C0 goes to 0:
    C9 = C9 + not(R9.0)
    If (C9 and %11110) == (R9 and %11110)
    Then
        C4 management (C4++ or C4 = 0 if C4 == R4)
        ParityC9 = ParityC9 xor (not r9.0)
        C9 = ParityC9
    Else
        C9 = C9 + 1 + (R9.0)
    End if
```

The ParityC9 toggle sits inside the same `Then` branch as the C4 management,
and that C4 management explicitly includes the `C4 = 0 if C4 == R4` frame wrap.
So the toggle fires at every C9/R9 match including the frame boundary — the
claim as stated. This is the B-2 reading the F10 review already forced, and the
RTL implements it at `rtl/crtc_type1_engine.v:352-354`
(`c4_increment_toggle = row_new && !R9_v_max_line[0]`, with
`row_new = line_new & line_row_event`).

Fixture arithmetic re-derived on paper with R9=8, R4=6 (seven C4 values):

- `ivm_row_end` (engine line 225) with R9=8 is true for C9=7 and C9=8, false
  elsewhere — exactly the 4-line/5-line row structure the table draws.
- Even frame: C4 ∈ {0,2,4,6} run 5 lines, C4 ∈ {1,3,5} run 4 → 4×5 + 3×4 = **32**.
- Odd frame: the parities swap → 4×4 + 3×5 = **31**.
- Seven rows ⇒ seven ParityC9 toggles per frame ⇒ odd count ⇒ the frame-start
  C9 parity flips every frame. That is what makes consecutive frames alternate;
  an even C4 count would not.
- R7=1: even-frame C4=1 starts at frame line 0+5 = **5** with C9=1; odd-frame
  C4=1 starts at 0+4 = **4** with C9=0. Gap = 1 line. ✔
- R7=2: even-frame C4=2 starts at 5+4 = **9** with C9=0; odd-frame C4=2 starts
  at 4+5 = **9** with C9=1. No gap. ✔

All four numbers in the brief's claim 3 reproduce.

### Claim 4 — scope claims: CONFIRMED, with one wording note

**Type-0 delay correction is documented for R9 odd.** §19.5.2 p.205 states it
directly: "When R9 is **odd** in IVM (unlike CRTC 1), this implies a difference
between the number of even and odd lines for a character between 2 frames".
p.206 then gives the correction inside that scheme, with an R9=7 worked example:
"if R7 is scheduled on an odd C4, then the VSYNC is delayed by 1 line. It occurs
when C4=R7 and C9.VMA=2 on the odd C4s." So the `../f10-implementation-notes.md`
characterisation ("part of the odd-R9 balancing scheme — Q19 territory") is
correct and it stays unimplemented. The p.207 table's four `R7=n` pairs show the
one-row displacement in the PARITYFRAME=ODD column, and its Note ("On an even
frame, the **VSYNC** occurs in the middle of the line on C0 = R0/2") is the
type-0 MID-VSYNC statement. Confirmed as documented-but-unimplemented, Q19-gated.

Note on symmetry, since it is easy to misread: the CRTC-0 scheme is keyed on
R9 **odd** and the CRTC-1 scheme on R9 **even**, because type 0 carries a
separate C9.VMA. In both cases the *character* has an odd line count. The commits
never confuse the two.

**The wrapper gate is type-1-IVM-only.** `rtl/CRTC.v:475` reads
`CRTC_TYPE && interlace[0]`, where `interlace[0]` is `&R8_interlace[1:0]`
(`rtl/CRTC.v:182`), i.e. R8 mode 3 only. Type 0 keeps the `field` arm on both
the fire and the count tick. The two vectors that pin that arm — `t02i`
(`test_type0_interlace_count_boundaries`, `sim_main.cpp:955`) and `t09g`
(`test_type0_interlace_r0_zero_freezes_vsync_count`, `sim_main.cpp:1457`) —
both set `set_crtc_type(0)` with R8=3, so they still pin the type-0 field arm
and would still fail if it moved. Confirmed.

---

## RTL review

### F11h — `rtl/CRTC.v:397-426`, `rtl/crtc_type1_engine.v:94,405`

The mechanism is: the engine exports the §20.3.2 row-0 arm separately
(`row0_reload = crtc1_row0_reload`), and the wrapper adds a *later* assignment in
the same `always @(posedge CLOCK)` block that overrides the stored-register load
with `{r12_effective, r13_effective}`. Because `e1_row0_reload ⊆ e1_reload`, the
override always wins where it applies and the other reload arms are untouched.
That is a clean way to express the scoping.

`r12_effective` / `r13_effective` mirror the register block at `rtl/CRTC.v:137-180`
correctly: `SNA_LOAD` first, then `ENABLE & ~nCS & ~R_nW & RS & (addr == 12/13)`,
then the stored register. The NBA reasoning in the commit message is sound —
both blocks assign non-blocking on the same `posedge CLOCK`, so reading
`R12_start_addr_h` yields the pre-edge value while `r12_effective` yields the
value the register file is about to take, which is precisely "samples the
register file as of after the current edge".

Correctly *not* changed: `row_addr` (VMA') is still never loaded from R12/R13 on
type 1, matching "On CRTC 1, **VMA** is loaded with R12/R13 while C4=0"; the
§11.2.4 adjustment arm and the §11.6 RFD arm keep the stored registers and are
named as unpinned residuals in `../audit-findings.md`.

### t24 — `rtl/CRTC.v:469-482`, `rtl/crtc_type1_engine.v:475-476`

The engine change (`line_last_w` → `line_row_structure_last` in
`vsync_line_fire`) reduces to the old term everywhere except type-1 IVM outside
adjustment, exactly as the commit claims. Verified by expansion of
`rtl/crtc_type1_engine.v:269-273`:

- inside adjustment: `line_row_structure_last = line_last_w` (unchanged);
- outside adjustment, `ivm == 0`: `line_limit_match = line_last_w` (unchanged);
- outside adjustment, `ivm == 1`: `ivm_row_end` (the change).

`ivm` is forced to 0 whenever `CRTC_TYPE == 0` (`rtl/crtc_type1_engine.v:208-214`),
and this engine output is consumed only under `CRTC_TYPE` in the wrapper mux, so
type 0 is untouched by construction.

The wrapper change is correct for the *fire line*. It is the *count tick* half
that carries B-1 below.

---

## B-1 (BLOCKING) — the type-1 IVM MID-VSYNC is removed, unpinned, and the docs claim the opposite

**Where:** `rtl/CRTC.v:476-478` (`vsync_count_tick`), with
`docs/accuracy/f10-implementation-notes.md:137-140`.

**Rule:** ACCC §19.5.3 p.208, in the prose immediately above the table this
whole vector family is derived from: "If **ParityFrame** is even, then an
**additional line** and a **MID-VSYNC** are scheduled. If **ParityFrame** is
odd, then **no additional line** and **no MID-VSYNC**." §19.5.1 p.205 lists
"The generation of a **MID-VSYNC** when C4=R7 on the even frame" as one of the
three roles of frame parity in interlace mode. The type-0 equivalent has an
explicit Note on p.207: "On an even frame, the **VSYNC** occurs in the middle of
the line on C0 = R0/2."

**What changed.** Before this branch, `vsync_count_tick` was
`field ? e1_field_count_tick : line_new`, and `e1_field_count_tick` is
`hcc_next == {1'b0, R0_h_total[7:1]}` — the half-line position. With R8=3 the
`field` flop toggles every frame (`rtl/CRTC.v:371`), so one of the two frames
counted (and therefore started and ended) its VSYNC at mid-line. After the
change, `vsync_type1_ivm` bypasses that arm for *both* parities and the tick is
`line_new` on both, so during type-1 IVM the VSYNC now always starts and ends on
a line boundary and no MID-VSYNC is emitted on any frame.

The old behaviour was partly broken — on the frame whose C4=R7 row had odd C9
values the field arm's `row == R7 && !line` fire never matched, so that frame
produced no pulse at all. That is the real bug `t24b` caught, and fixing it is
right. But the fix discards the half-line offset along with the wrong fire line,
and the half-line offset is separately documented.

**Why it is not merely a scoping choice that could be waved through.** The two
sub-changes cannot be separated in the current shape, which is exactly why this
needs to be recorded rather than left implicit. `e1_vsync_line_fire` is
`hcc`-independent (`row_next` at `rtl/crtc_type1_engine.v:340` and
`line_row_structure_last` both depend only on counters, not on C0), so it is
already true throughout the *whole* last line of the row before C4=R7. Consuming
it at a half-line tick would start the pulse half a line *before* the documented
line, not after it. A correct MID-VSYNC therefore needs a latch, not a mux
tweak — a real design decision that the branch made implicitly.

**Why nothing caught it.** `t24a`/`t24b` sample VSYNC mid-line: after the
snapshot load the bench walks 30 then 34 characters and thereafter 32+32 per
line, so every sample lands at C0 ≈ 33-35, past the C0=31 half-line point
(`R0=63`, `R0>>1 = 31`). A pulse starting at C0=31 of line L and one starting at
C0=0 of line L read identically at C0=35, on the first line, on the last line,
and everywhere between. No other vector in the suite asserts VSYNC with
`CRTC_TYPE=1` and R8=3 — the only two MID-VSYNC vectors, `t02i` and `t09g`, are
type 0. The soak re-mint does move, but it bundles both sub-changes and cannot
discriminate them, so it is not evidence for this one.

**Why the documentation is wrong, not merely silent.**
`docs/accuracy/f10-implementation-notes.md:137-140` now says the MID-VSYNC
residual was *narrowed* — "during type-1 IVM the field arm no longer hijacks
fire/count tick at all … so the residual now covers type-0 IVM and post-episode
divergence only". The residual was widened, not narrowed: type-1 IVM now has no
MID-VSYNC under any parity. A future session reading that bullet would conclude
type-1 IVM VSYNC placement is settled. The neighbouring bullet at lines 141-146
compounds it: "Type 1's documented *lack* of the correction (p.208) is now
modeled and pinned by `t24a`/`t24b`" is true for the delay correction and reads
as if p.208's VSYNC rules for type 1 are now fully modelled.

**Remediation — either of these clears B-1.**

*(a) Record it (minimum bar).* Add a named residual to
`docs/accuracy/f10-implementation-notes.md` stating that during type-1 IVM the
VSYNC starts at the line boundary on both frame parities and the §19.5.3 p.208
MID-VSYNC for the even ParityFrame is not emitted; cite p.208 and note that
`e1_vsync_line_fire` is `hcc`-independent so implementing it requires a latched
fire decision. Correct the "residual now covers type-0 IVM and post-episode
divergence only" sentence. Add the same residual to the t24 closure bullet in
`docs/current-status.md:241-250` and, if t24 gets an `../audit-findings.md` entry,
there too.

*(b) Implement it.* Latch, at the `line_new` CLKEN edge, the current value of
`e1_vsync_line_fire` ("the line starting now is the first line of C4=R7"), and
consume that latch at `e1_field_count_tick` while `vsync_type1_ivm && field`,
keeping `line_new` on the other parity. Land it with a new required vector
`t24c`: the `t24a` register set with R7=2, sampling VSYNC at C0=20 and at C0=40
on the pulse's first line of each frame, expecting low-then-high on the
ParityFrame-even frame and high-then-high on the odd one, and the mirrored
low/high pair on the line where the pulse ends. That vector is what makes the
half-line offset observable at all; the existing mid-line sampling never can be.

---

## Non-blocking findings

**N-1 — the IVM gate reads the raw R8 register, not the engine's latched `ivm`.**
`rtl/CRTC.v:475` uses `interlace[0]`, which follows `R8_interlace` combinationally
from the write edge. The engine's `ivm` (`rtl/crtc_type1_engine.v:152,186-206`)
lags it by one CLKEN on a snapshot/reset seeding and by a full stage-A edge on a
toggle write, and leads it on the way out (R8→0 drops `interlace[0]` at once
while `ivm` holds until stage A). For a one-to-two character window around an R8
toggle the wrapper and the engine therefore disagree about whether IVM is
active, so a VSYNC decision landing exactly on that window takes a mixed model.
Narrow and unpinned, but new with this branch. Suggest exporting the engine's
`ivm` as `e1_ivm` and gating on `CRTC_TYPE && e1_ivm`, or a one-line comment at
`rtl/CRTC.v:475` naming the window as a deliberate residual.

**N-2 — three dead helpers left behind.** `expect_known_ma`,
`expect_known_vsync_high`, `expect_known_vsync_low` (`sim/sim_main.cpp:574-592`)
were added by the two fixture commits and orphaned by the two behaviour commits;
no test calls them at HEAD. Harmless (public members, no unused warning) and
consistent with the pre-existing `expect_known_*` family, but it extends the
already-recorded action item A4 in `docs/review-debt.md`.

**N-3 — whitespace regression in `docs/current-status.md:224-229`.** The
continuation lines of the "Remaining non-gated classic items" bullet gained a
one-space indent in the t24 commit's edit, so the bullet's body no longer aligns
with the two-space convention used by every other bullet in the file. The repo
runs an exact-range whitespace gate elsewhere; worth a one-line fix.

**N-4 — misleading comment in `t20j`.** `sim/sim_main.cpp:4368-4369` says "R12 is
6 bits, so the written `0x15` truncates `DI[5:0]`". `0x15` is 21, which fits in
six bits; nothing truncates. The value is fine, the explanation is not.

**N-5 — the frame-origin extension of the F11h catch is an inference, not a
drawing.** `crtc1_row0_reload` is `frame_new_w | (~line_row_structure_last & !row
& !hcc_next)` (`rtl/crtc_type1_engine.v:402`), so applying `r12_effective` to
that arm extends the same-edge catch to the frame origin as well as the mid-row-0
seam that p.242 actually draws. It is a defensible reading of "VMA is loaded
with R12/R13 while C4=0" — the frame origin is one such event — and `t20j` pins
both forms deliberately. But the `../audit-findings.md` F11h entry presents both as
p.242-derived. Suggest one clause naming the frame-origin case as the inference
it is, so a later hardware test knows which half is drawn and which is deduced.

**N-6 — CI bump partially achieves its stated motive, and is unverifiable
offline.** The workflow's triggers are `push`, `pull_request`, `workflow_dispatch`
only (`.github/workflows/build.yml:3-11`) — no `pull_request_target` and no
`workflow_run` — so the commit's risk assessment about the v7 fork-PR breaking
change is correct, and all three `actions/checkout` uses were bumped together.
However `actions/upload-artifact@v4` at line 217 was left alone; if the motive
was the Node-20 deprecation warning "in every run", that action will keep
emitting it. Separately, this reviewer has no network access and cannot confirm
that `actions/checkout@v7` exists or that v7.0.1 is the current latest major;
the first CI run on this branch is the check.

**N-7 — new combinational path into the video pointer.** `r12_effective` /
`r13_effective` put `DI`, `ENABLE`, `nCS`, `R_nW`, `RS` and `addr` directly into
the data mux of the 14-bit `row_addr_r`. At this clock rate the added depth is
immaterial, but it is new logic on a path Quartus has not compiled on this
branch. Per `docs/ci-testing-policy.md` synthesis runs on merge, which covers it;
noted only so it is not a surprise.

---

## Bite-test log — NOT RUN, with exact edits and predictions

The harness refused every build and execute command in this session (see "Gate
status" above), so no bite-test was performed. The following are specified
precisely enough to run mechanically. Run (d) first: its outcome is the direct
evidence for B-1.

**(a) Wrapper IVM bypass.** In `rtl/CRTC.v`, change both occurrences of
`(field && !vsync_type1_ivm)` (lines 477 and 481) back to bare `field`.
Predicted: exactly `t24b` fails, `t24a` still passes. Reasoning: `t24a`'s odd
frame fires at (1,0), where `line == 0`, so the reverted field arm's
`row == R7 && !line` still matches; `t24b`'s odd frame box is (2,1), where
`line == 1`, so the field arm misses it. Restore with
`git checkout -- rtl/CRTC.v`, rerun `make -C sim`.

**(b) Engine row-end test.** In `rtl/crtc_type1_engine.v:476`, change
`line_row_structure_last` back to `line_last_w` inside `vsync_line_fire`.
Predicted: exactly `t24b` fails. Reasoning: `t24b`'s even-frame fire crosses the
(1,7) wrap, where `line_last_w` (`line == R9 == 8`) is false while `ivm_row_end`
is true; `t24a`'s even-frame fire crosses the (0,8) wrap, where both are true.
Restore with `git checkout -- rtl/crtc_type1_engine.v`, rerun `make -C sim`.

**(c) F11h override.** In `rtl/CRTC.v`, delete the
`if(e1_row0_reload) row_addr_r <= {r12_effective, r13_effective};` block
(lines 424-426) inside the address `always` block. Predicted: exactly `t20j`
fails, at its first same-edge assertion (`expect_ma` == `0x3034`), because
`crtc1_reload` then loads the pre-edge stored `R12 = 0x12`. `t20k` (type 0) is
unaffected. Restore with `git checkout -- rtl/CRTC.v`, rerun `make -C sim`.

**(d) B-1 evidence — count tick only.** In `rtl/CRTC.v:477`, revert *only* the
`vsync_count_tick` condition to bare `field`, leaving `vsync_fire` on line 481
as committed. Predicted: **`t24a` and `t24b` both fail** (on the field=1 frame
the fire term is only true at the line-end edge, which the half-line tick never
coincides with, so no pulse starts on that frame at all). That failure mode is
the point: it demonstrates the two sub-changes are structurally inseparable in
the current shape, which is why the MID-VSYNC removal is a design decision that
needs recording rather than an incidental consequence. If instead the parent
wants direct evidence that the *committed* count-tick change is unpinned, run
(a) and observe that `t24a` passes — a test that samples at C0≈35 cannot tell a
C0=31 pulse start from a C0=0 one.

After any bite-test, `git status` must show a clean tree apart from this
document. This session made no RTL or sim edits at all, so the tree is clean now.

---

## Other surfaces reviewed

**Harness phase arithmetic — correct in all four vectors.** `bus_write` performs
exactly one `clock_tick` at the current phase, and `write_selected_register_at_clken`
first walks to `tick_in_character_ == 0` (the CLKEN phase), so the register write
posedge is the CLKEN edge. Re-derived:

- `t20j`: after `reset()` the phase is 0; `run_characters(312*64 + 63)` lands at
  C0=63 of line 0 of the second frame, so the write edge is the 63→0 row-0
  boundary — the p.242 chronogram-2 geometry exactly. The bench then sits at
  phase 1, where `select_register(13)` correctly consumes no CLKEN edge (the
  comment at `sim_main.cpp:4357-4359` is right), and `run_characters(9)` +
  the write edge lands the R13 write on the edge entering C0=10. The final
  `run_characters(309*64 - 1)` plus the write edge sums to exactly 312×64 = 19968
  edges from the frame origin, so the last write lands on the frame-origin edge
  as claimed.
- `t20k`: `run_characters(311*64 + 63)` lands at C0=63 of line 311, the frame's
  last line (C4=R4=38, C9=R9=7), so the write edge is the frame origin. The
  follow-up `run_characters(312*64)` reaches the next origin. Correct.
- `t24a`/`t24b`: sampling lands at C0 ≈ 33-35 every line, mid-line and past the
  half-line point. The line-index walk (32 lines then 31) is self-checking
  because the pulse-start assertions pin C4 and C9, so a frame-phase
  misalignment would fail rather than pass silently. Correct — with the caveat
  in B-1 that mid-line sampling is precisely what makes the count-tick change
  invisible.

One soft point, non-blocking: which parity the first post-snapshot frame runs
(the comment at `sim_main.cpp:5222-5223` asserts it is the table's EVEN FRAME
column) is a model-state property, not an ACCC-derived one. It is acceptable
here because both parities are asserted with explicit (C4,C9) expectations, so
the pair is pinned as a set even if the labels were swapped.

**No vector was weakened.** `git diff c3efcff..HEAD -- sim/sim_main.cpp` is
+272/−1, and the single removed line is `}  // namespace`, relocated. Both
XFAIL→required promotions happened in the behaviour commit that earned them
(`t20j` in `7def250`, `t24b` in `6116f7d`), and both promotions swapped the
`expect_known_*` calls back to hard assertions rather than leaving soft ones in
required tests. `t20k` was correctly written as a required pass from the start so
the F11h fix could not be over-generalised to type 0.

**Soak re-mints.** Both are justified by a real behaviour change and both chain
correctly in `AGENTS.md:48-79`.

- `0xa9e5026de83d287c → 0x801a59096c192d26` (F11h): the soak drives random
  register traffic at arbitrary C0 and CLKEN phases across both types, so
  R12/R13 writes coinciding with type-1 row-0 boundaries are reachable, and MA
  is in the sampled projection. Justified.
- `0x801a59096c192d26 → 0xd620fce8b1c05b25` (t24): VSYNC is sampled and random
  R8 traffic reaches R8=3 on type 1, so both the fire-term and the count-tick
  change move the hash. Justified — but see B-1: the mint cannot discriminate
  the two, so it is not a substitute for the missing MID-VSYNC vector.

**Process note.** `docs/review-debt.md` carries no row for this branch. That is
correct as long as the branch is not merged before this review clears; if it
merges with B-1 open, the rule at the bottom of that file requires a row naming
the type-1 IVM MID-VSYNC removal as the thing to read hardest.

---

## Findings summary

| # | Severity | Finding | Remediation |
|---|---|---|---|
| B-1 | Blocking | Type-1 IVM MID-VSYNC (§19.5.3 p.208) removed on both parities; unpinned by any vector; `../f10-implementation-notes.md` claims the residual narrowed | Record the residual and correct the two bullets (minimum), or latch the fire decision and add `t24c` |
| N-1 | Non-blocking | IVM gate uses raw `interlace[0]`, not the engine's latched `ivm`; 1-2 character disagreement window at R8 toggles | Export `e1_ivm` and gate on it, or comment the residual |
| N-2 | Non-blocking | Three `expect_known_*` helpers left dead at HEAD | Remove, or fold into action item A4 |
| N-3 | Non-blocking | Stray one-space indent, `docs/current-status.md:224-229` | Whitespace fix |
| N-4 | Non-blocking | `t20j` comment claims a truncation that does not occur | Reword |
| N-5 | Non-blocking | Frame-origin same-edge catch is an inference from p.242, presented as drawn | One clause in the `../audit-findings.md` F11h entry |
| N-6 | Non-blocking | `upload-artifact@v4` left on the deprecated runtime; `checkout@v7` unverifiable offline | Bump or scope the commit message; confirm on the first CI run |
| N-7 | Non-blocking | New combinational `DI`→`row_addr_r` path awaiting synthesis | Covered by synthesis-on-merge; noted only |

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND 4.0).
