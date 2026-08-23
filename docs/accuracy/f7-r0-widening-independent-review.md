# Independent review — `accuracy/f7-rfd-r0-widening` (§13.7.1.2 R0-widening RFD trigger)

Reviewed 2026-08-23 by **Claude Opus 5** (Claude Code session, `claude-opus-5`). The delta was
authored by **Ox-Alpha**, so this is a genuine **cross-provider independent review** — a
different model family from the author, which is the pass the project normally requires and
the 2026-08-22 locked-decision fallback in `docs/plans/2026-08-22-accc-review-plan.md` exists
to substitute for when it is unavailable. The fallback is not being invoked here. No part of
the branch was authored, advised on, or previously read by this session.

- Repository branch under review: `accuracy/f7-rfd-r0-widening`, clean at tip `500586c`.
- Reviewed base: `d91fa0b`.
- Range: `git diff d91fa0b..500586c` — commits `95fff05` (RTL + vectors `t13e`-`t13k`),
  `578b803` (docs), `500586c` (review-debt row). Reviewed as one whole diff.
- Staging base `accc-review-and-fixes` was at `d91fa0b` throughout; no rebase was needed.

## Overall verdict

**NOT CLEAR.** Two blocking findings. The branch was **not merged**; no RTL or test file was
modified to make anything agree. All diagnostic experiments described below were run on a
scratch copy of the working tree and reverted (`git status` clean afterwards).

Both blocking findings are inside the new feature; nothing pre-existing regresses, and the
gates all pass as the author reported. The objection is that the RTL contains one behavioural
defect the vectors cannot see, and one vector does not exercise the case it is credited with.

## Method

- Rule claims checked against `docs/accuracy/compendium-01-counters.md` §5 (RFD, ACCC
  §11.6 pp.87-90), §8.5 (ACCC §13.3/§13.7.1.1, pp.113/124) and §8.6 (ACCC §13.7.1.2 p.124),
  never against the RTL.
- Every `t13e`-`t13k` expectation re-derived on paper from the fixture geometry
  (R0=7, R1=4, R4=1, R5=0, R9=1 ⇒ 8 characters × 4 lines per frame) before reading any
  simulator output, including the CLKEN-edge arithmetic of `bus_write` /
  `write_selected_register_at_clken` / `run_characters`.
- RTL read for correctness independently of the vectors, then two targeted diagnostic probes
  run to settle findings the vectors do not reach (see "Diagnostic probes").
- The soak was treated as an equivalence-class check only, and its actual sensitivity in this
  region was measured rather than assumed (finding F-F).

## Blocking findings

### F-1 · Blocking — `rtl/CRTC.v:385` guard suppresses a real `C0==R1` DISPEN event

`hcc_next` is now derived from `hcc_end`, so at the suppressed-wrap edge it already holds the
correct continuation value `R0_old+1` rather than `0`. There is therefore no spurious
roll-into-`R1` comparison left to guard against, and the added
`& ~(CRTC_TYPE & e1_rfd_r0_extend)` term has exactly one effect: when `R1 == R0_old + 1`, it
blocks the legitimate DISPEN-off at `C0==R1`.

Digest §1 (ACCC §6.1.3, p.33): "DISPEN horizontal: enabled at C0==0, disabled at C0==R1." On
the extended line `C0` genuinely reaches `R0_old+1`, so if that equals `R1` the display end
belongs there. The comment above the guard states the opposite rationale and describes a
hazard the `hcc_end` change had already removed.

Confirmed empirically (probe P-2): with `R0` widened 7→9 at the trigger edge and `R1=8`, DE
stays high through `C0=8` and the whole widened remainder. Removing the guard drops DE at
`C0=8` as the rule requires, leaves all 107 required passes green, and leaves the soak hash
unchanged.

Not an idle corner: `R1 > R0` is precisely the configuration family §5 (ACCC p.87) ties to
RFD through the bare-C9 disarm route, and `rfd_r1_gt_r0_disarm` in the same engine exists to
serve it.

**Action**: drop the `~(CRTC_TYPE & e1_rfd_r0_extend)` term from the `hcc_next ==
R1_h_displayed` condition, correct the comment, and add a vector with `R1 == R0+1` pinning
DISPEN low from `C0==R1` on the extended line.

### F-2 · Blocking — `t13j` does not reach the state it claims, so the off-last-line gate is unexercised

`t13j_type1_rfd_r0_widen_off_last_line_never_arms` documents "widening R0 at C0==R0 of line 1
(C4=0)". The fixture does not get there. `run_characters(15)` already leaves `C0=7 (==R0)`,
`C9=1`, `C4=0`; the following `run_characters(7)` therefore wraps the line and lands the write
at **`C0=6`, `C4=1`, `C9=0`** (measured, probe P-1).

The assertion still holds, but only because `hcc_last` is false — which is `t13a`'s subject,
not this vector's. The mid-frame-**at**-`C0==R0` gating (`line == crtc1_line_max && row ==
R4_v_total` in `rfd_r0_widen_at_last_line`) is never exercised by any vector. That gate is the
single guard `audit-findings.md` and the commit message rely on when declaring arbitrary
mid-frame widening "deliberately unmodeled", so the coverage claim is not earned.

**Action**: retime the fixture so the write lands on a genuine `C0==R0` edge off the last line
(e.g. `run_characters(15)` then the write directly, exercising `C4=0, C9=1, C0=R0`), keep the
same expectations, and re-derive the trailing `run_characters(10)` arithmetic against the new
alignment.

## Non-blocking observations

### F-3 · Low — `field_count_tick`'s `~rfd_r0_extend` guard is dead code
`rtl/crtc_type1_engine.v:289`. At the extend edge `hcc_next == R0_old + 1`, which can never
equal `R0_old >> 1` for any 8-bit `R0_old` (including the 255 wrap case). Same vestigial cause
as F-1, but harmless. The comment asserts a coincidence that cannot occur.

### F-4 · Info — `rfd_r0_arm` keys on raw `hcc_last`, correct only by a non-obvious invariant
`rfd_r0_arm` uses `hcc_last`, not `hcc_end`. This is right, because `rfd_r0_widen_at_last_line`
requires `line == crtc1_line_max && row == R4_v_total` while `rfd_r0_cancelled` is exactly the
negation of that conjunction — a re-extend and an arm can never share an edge, so `hcc_end ==
hcc_last` on every possible arm edge. Worth stating as a comment; it is the answer to the
review-debt row's "can an arm edge coincide with a self-contradictory `rfd_r0_cancelled`"
question, and the answer is no.

### F-5 · Low — a stale trigger window can survive a C0 overflow line
If `R0` is shrunk below the live `C0` after the window opens, `hcc_last` never matches for the
rest of that line; `hcc` runs to 255 and wraps to 0 with no `hcc_end`, so `rfd_r0_pending`
survives into the next line and can arm at *its* end. Type 1 accepts any `R0` (§8.5, ACCC
§13.3 p.113), so the sequence is legal. Unmodeled and undocumented rather than wrong.

### F-6 · Low — the §8.6 R4-variant's second-frame consequence is neither modelled nor listed
Digest §8.6 says that for the R4 variant, "on the second frame specifically, C4 is NOT reset
to 0 (stuck C4 instead of a repeated line)". Nothing in the engine can suppress a C4 reset,
`t13g` stops at the first frame, and `audit-findings.md` lists only the mid-frame
generalization as a deliberate omission. Add it to the stated scope.

### F-7 · Info — digest tension behind the `t13h` end-state reading is real and unrecorded
Review point 3 resolves in the author's favour, but not unanimously. §8.6's variant
parentheses — "(C9 != R9 by line end)" / "(C4 != R4 by line end)" — support evaluating
register state at the extended end, which is what the RTL does and what `t13h` pins. §8.5's
p.122-123 chronogram gist reads the other way: "RFD activated on CRTC 1 if R4 and/or R9
**modified** until C0=7F (new R0) on last line of frame" is a write-event condition that would
arm on `t13h`'s cancel-then-restore. §8.6 is the dedicated section for §13.7.1.2 and the
end-state reading is the physically plausible one (a real comparator has no "was written"
latch), so the implementation is defensible — but the tension should be recorded and routed to
SHAKER Module C `(1)` / D `(9)`, not left implicit.

### F-8 · Info — the unarmed frame-parity toggle (review point 5a) is an interpretation
`rfd_frame_parity` toggling at every genuine `C4=C9=C0=0` boundary with odd R9 regardless of
arming sets the final-state expectations in `t13e`/`t13h`/`t13i`, and digest §11.6.1's sentence
("Parity flips at every frame boundary where `C4==C9==C0==0 && R9 is odd`") carries no arming
qualifier. That sentence does, however, sit inside a bullet scoped "once armed". Free-running
parity is the reading that keeps the IVM ON/OFF freeze discussion coherent, so accept it, but
mark it ⚠ for hardware.

### F-9 · Info — the unchanged soak hash proves less than the commit message claims
The commit message says the unchanged hash "also pins bit-identical behavior outside the
recipe". Measured (probe P-3): after deliberately changing CRTC behaviour in the region — the
F-1 guard removed, which demonstrably alters DE — the soak still produced
`0x512eaae74a628dca`. The soak is simply insensitive here; it confirms random traffic never
reaches the trigger window, and nothing more. The rationale is honest about *why* the hash is
unchanged, but the "pins" wording overstates what the evidence supports.

## Per-area verdicts (against the open review-debt row)

| Review-debt area | Verdict |
|---|---|
| 1. Completeness of the `hcc_end` consumer switch | **Finding F-1.** The line-event set is otherwise right: `hcc_next` derivation, wrapper `row_addr_r` increment, engine `line_new`, the `status_bit5` sample point, and the `!hcc_next` reload terms all key on the deferred end. The intended raw-only set (R5-route arm `:119`, A2 R4-at-entry caveat `:223`, trigger detection `:149`) is correctly raw. `rfd_r0_arm` `:157` is raw and safe (F-4). Type-0 engine uses (`:193`, `:238`, `:319`, `:321`) are unreachable by the new term, which is `CRTC_TYPE`-gated twice. The one over-suppression is the `R1` display-end guard, plus its dead twin on `field_count_tick` (F-3). |
| 2. Same-edge ordering in the type-1 flag block | **Accepted.** Arm (`rfd_arm \| rfd_r0_arm`) is written after the `row_addr_save` / `rfd_r1_gt_r0_disarm` clear, so a same-edge trigger wins, matching the R5 route's documented intent. The pending-window block touches only `rfd_r0_pending` and prioritises set over clear; on the extend edge `hcc_end` is low anyway. No self-contradictory `rfd_r0_cancelled` arm edge exists (F-4). |
| 3. End-state cancellation reading behind `t13h` | **Accepted with F-7.** Supported by §8.6's "by line end" parentheses; in tension with §8.5's chronogram gist; record the tension. |
| 4. Scope honesty on the mid-frame generalization | **Partly accepted.** The mid-frame omission is documented in the RTL comment, the commit message, and `audit-findings.md`. But the guard that enforces it is untested (F-2), and a second documented consequence is silently absent (F-6). |
| 5a. Unarmed frame-parity toggle | **Accepted with F-8.** Re-derived independently for `t13e` (parity 0→1 at the ordinary restart), `t13h` and `t13i`; all three match. |
| 5b. `t13k`'s one-CLOCK-edge window clear | **Accepted, not a relaxation.** `set_crtc_type(x); run_clock_ticks(1);` is the established house pattern for hidden-flop clearing in `t02j` and `t16l` (`t06d` and `t09f` test combinational round-trip behaviour instead). The clear branch is `always @(posedge CLOCK) ... else <clear>` with no CLKEN gate, so one CLOCK edge is exactly right. |
| 6. Bit-identity outside the recipe | **Accepted, with F-9 on the strength of the evidence.** Every new term is `CRTC_TYPE`-gated (`r0_write_hit` carries `CRTC_TYPE`; `hcc_end` masks with it again), so type-0 paths cannot see the extension. Directed type-0 vectors and the full suite stay green. |

## Paper re-derivations that matched the vectors

Fixture: R0=7, R1=4, R2=6, R3=0x11, R4=1, R5=0, R6=2, R7=20, R8=0, R9=1 ⇒ C0=0..7,
C9∈{0,1}, C4∈{0,1}; 8 characters × 4 lines. `run_characters(24)` lands at C0=0, C9=1, C4=1
(the true last line); `run_characters(7)` reaches C0=7==R0; `write_selected_register_at_clken`
consumes exactly that CLKEN edge, so a following `run_characters(2)` supplies exactly the two
edges C0=8 and C0=9 needed to reach the deferred end under the new R0=9.

- `t13e` — no cancellation: `rfd_r0_cancelled = 0` at the extended end, no arm, `frame_new_w`
  restarts the frame, odd-R9 parity toggles 0→1, `crtc1_row0_reload` gives MA=0x1234. ✔
- `t13f` — R9→3: `crtc1_line_max` becomes 3, `line_last_w` false at the extended end ⇒ arm,
  `crtc1_rfd_reload` fires on the same edge (MA=0x1234), C9→2 with C4 held at 1. Over the
  following 20 characters (10-character lines now) the C9=3 line ends the frame; on it
  `row_addr_save_base` is true at C0==R1=4 but the case-1 gate
  (`~rfd_parity_active | rfd_frame_parity` = 0) suppresses the save, so the VMA flag survives
  and parity toggles to 1. ✔
- `t13g` — R4→2: `row_last_w` false at the extended end ⇒ arm, C4→2 and C9→0, same-edge
  R12/R13 reload; frame ends 20 characters later when C4 reaches 2, parity toggles. ✔
- `t13h` — R9 0 then 1: end state restored, `rfd_r0_cancelled = 0`, no arm, ordinary restart
  with the parity toggle. ✔
- `t13i` — equal-value write: `DI > R0_h_total` false, no window, the line ends at that very
  edge and the frame restarts; the later R9=3 then leaves C4 at 0 after one 8-character line. ✔
- `t13j` — see F-2; the assertion holds but not for the stated reason.
- `t13k` — window opened, R9→3 written, one CLOCK edge on type 0 clears every hidden flag, and
  the deferred end then behaves as a plain cancelled non-event: no arm, C9→2, C4 held at 1. ✔

## Diagnostic probes (run on a scratch tree, reverted)

- **P-1** — instrumented print of `hcc`/`line`/`row` at `t13j`'s write point:
  `hcc=6 line=0 row=1 R0=7`. Establishes F-2.
- **P-2** — new temporary vector with R0=7→9, R1=8, run past one full frame so `vde` is high:
  DE reads 1 at C0=7, **1 at C0=8 (==R1)**, 1 at C0=9. With the F-1 guard removed: DE reads 1
  at C0=7, **0 at C0=8**, 0 at C0=9. Establishes F-1.
- **P-3** — full suite and soak with the F-1 guard removed: `Summary: 108 passed, 0 xfailed,
  0 xpassed, 0 failed` (107 required plus the temporary probe) and soak hash
  `0x512eaae74a628dca`, unchanged. Establishes F-9 and shows no vector is load-bearing on the
  guard.

`git status --porcelain` was empty after reverting; the reviewed tip is unmodified.

## Gate evidence (rerun independently on the branch tip `500586c`)

- `make -C sim` — `Summary: 107 passed, 0 xfailed, 0 xpassed, 0 failed`; Plus leaf, MMU,
  SDRAM cartridge, and P0 boot suites all green.
- `make -C sim lint` — clean of new warnings. The five CRTC warnings
  (`SNA_REGS` unused bits, `interlace[4:1]`, `R10_cursor_mode`, type-0 `DI[7]`,
  type-0 `row_last_w`) are pre-existing and name no new signal; the remainder are the
  known `rtl/sdram.v` and Plus test-top warnings.
- `make -C sim soak SOAK_EXPECT=0x512eaae74a628dca` — `soak hash matches expected`. See F-9
  for what this does and does not establish.
- Verilator 5.050 2026-07-01.

## What a re-review should check after remediation

1. That the F-1 fix drops DISPEN at `C0==R1` on the extended line and that its new vector
   derives that from ACCC §6.1.3 p.33 rather than from the simulator.
2. That the retimed `t13j` actually sits on a `C0==R0` edge off the last line — assert the
   counters at the write point, not only the outcome.
3. That the soak hash is re-minted if, and only if, the F-1 fix moves it (it should not; the
   soak does not reach this region).

## Remediation status (added by the author after the review, 2026-08-23)

All three checks above are now exercisable in-branch: F-1/F-3 guards removed (`rtl/CRTC.v`,
`rtl/crtc_type1_engine.v`), `t13j` retimed with counter asserts at the write point, `t13l`
covers the line-half of the precondition gate, `t13m` pins the §6.1.3 display end; each
vector bite-tested against its reverted mechanism (reinstating the guard fails only
`t13m`; removing either gate term fails exactly `t13j`/`t13l`). Suite 109/109, lint clean,
soak unchanged at `0x512eaae74a628dca`. Non-blocking items: F-4 invariant comment added;
F-5/F-6 recorded under audit-findings F7 "Deliberately unmodeled / interpretation notes";
F-7 routed to `accc-author-questions.md` question 18; F-8 marked ⚠ for hardware there and
in audit-findings; F-9 erratum recorded in `current-status.md`. The re-review pass itself
remains outstanding per the `review-debt.md` row.

## Pass 2 — re-review of the remediation delta (2026-08-24)

Re-reviewed by **Claude Opus 5** (Claude Code session, `claude-opus-5`) — the same reviewer
as the pass-1 verdict above, and still cross-provider with respect to the author (**Ox-Alpha**
wrote both the original feature and this remediation). Continuity is deliberate: the pass-1
checklist is the acceptance criterion, so the reviewer who wrote it verifies it. This session
did not author, advise on, or suggest wording for any part of the remediation.

- Range: `git diff 729ba02..ab98c6b` — `8278069` (RTL + vectors), `ab98c6b` (docs).
- Staging base `accc-review-and-fixes` unmoved at `d91fa0b`; no rebase needed.

### Verdict: **CLEAR** with five non-blocking follow-ups (N-1 to N-5).

Both blocking findings are genuinely fixed, not papered over. No assertion was weakened: the
suite grew 107 → 109 and every previously required pass stayed required.

### Claim-by-claim

**1. F-1 fix — confirmed.** The `~(CRTC_TYPE & e1_rfd_r0_extend)` term is gone from
`hcc_next == R1_h_displayed` and the replacement comment states the correct reason (`hcc_next`
carries the post-mux continuation value, so the comparison is already right). `t13m`'s
expectations are rule-first, not observation-first: ACCC §6.1.3 p.33 gives DISPEN on at
`C0==0` and off at `C0==R1`; with `R1=8` and `R0_old=7` the pre-extension line never reaches
`R1` (hence DE high at `C0=7`), the suppressed wrap carries `C0` to `8==R1` (hence DE low on
that same edge), `C0=9` is past `R1` with no re-enable (hence still low), and the deferred end
is a genuine `line_new` at `C0=0` (hence high again). Re-derived independently; all four
match. The one-frame warm-up is load-bearing and correctly justified: `DE = hde & vde & vde_r`
(`rtl/CRTC.v:488`) and type 1 pins `de_index = 2'b00`, so there is no skew pipeline hiding the
edge, but both vertical terms must be set before any DE assertion means anything.

**2. F-2 fix — confirmed.** `t13j` now writes at `run_characters(15)`: `C0=7==R0`, `C9=1==R9`,
`C4=0!=R4`, with the counter asserts at the write point that the pass-1 checklist demanded.
Trailing arithmetic re-derived on paper and matched: the write edge is an ordinary line end
because `C9==R9` still holds, so `C4→1` and `C9→0` while `R0` becomes 9; the following ten
CLKEN edges are exactly one widened line, whose end finds `C9=0 != R9=1` and therefore raises
no row boundary, leaving `C4=1`. The later `R4` rewrite has no window to act in.

**3. `t13l` — confirmed.** `run_characters(23)` lands `C0=7==R0`, `C4=1==R4`, `C9=0!=R9`: the
mirror-image half of the gate. Re-derived: the write edge wraps ordinarily with no row
boundary (`C9=0` does not match `R9=1`), so `C9→1` with `C4` held; `R9→3` then makes
`crtc1_line_max=3`, so the ten-character extended line ends with `line_last_w` false and
`C9→2`, `C4` still 1. Matches every assertion.

**4. F-3/F-4 — confirmed, with N-2.** The dead `~rfd_r0_extend` term on `field_count_tick` is
removed and provably could never have fired: at the extend edge `hcc_next = R0_old+1` while the
comparison reads the stored `R0_old>>1`, and `R0_old+1 == R0_old>>1` has no solution over the
full 8-bit range (`R0_old+1 - R0_old/2 = R0_old/2 + 1 > 0` for `R0_old ≤ 254`; the `255` wrap
gives `0` vs `127`). Removal is therefore behaviour-identical, consistent with the unchanged
soak. The F-4 invariant comment was added at `rfd_r0_arm` and its substantive clause is right —
see N-2 for its faulty opening clause.

**5. Bite-test honesty — reproduced, with one precision correction (N-4).** All three probes
were run on a scratch basis (file copied aside, edited, rebuilt, restored; `git status` empty
afterwards):

| Probe | Mutation | Measured failure set |
|---|---|---|
| A | reinstate the F-1 guard on `hcc_next == R1_h_displayed` | `t13m` only — `108 passed, 1 failed` |
| B1 | drop `(row == R4_v_total)` from `rfd_r0_widen_at_last_line` | `t13j` only — `108 passed, 1 failed` |
| B2 | drop `(line == crtc1_line_max)` from the same term | `t13l` only — `108 passed, 1 failed` |

This is a *stronger* result than the claim as worded: each gate term is load-bearing for
exactly one vector and disturbs nothing else, and the union across B1/B2 is exactly
`{t13j, t13l}`. Predicted on paper before running, and matched.

**6. Docs — confirmed, with N-1, N-3 and N-5.** The audit-findings F7 "Deliberately unmodeled /
interpretation notes" block covers F-5 (window surviving a legal C0-overflow line), F-6
(§8.6 second-frame stuck-C4), F-7 (end-state vs write-event reading) and F-8 (free-running
unarmed parity), each pointing somewhere actionable. Author question 18 states both source
readings verbatim and neither is strawmanned: the p.122-123 gist is quoted with its
"**modified**" emphasis intact and its consequence spelled out (would arm on a restore), the
§13.7.1.2 parentheses are quoted in full, our choice is given with its two reasons, and a
discriminating SHAKER experiment is named. `current-status.md` carries the F-9 erratum and now
says the directed vectors, not the soak, carry the behavioural proof.

### Non-blocking follow-ups

- **N-1 · Low — the `review-debt.md` row was malformed.** The remediation edit appended a
  fourth cell (`**Open — remediation delta awaits independent re-review before merge.**`) to a
  three-column table, so most Markdown renderers drop it silently — hiding the open status in
  the one file whose job is to track it. Fixed as part of flipping the row to CLEARED in this
  pass; worth watching for on future row edits.
- **N-2 · Low — the F-4 invariant comment misstates its own invariant in its first clause.**
  "arming requires `rfd_r0_widen_at_last_line` false at every earlier edge" is backwards:
  arming requires it *true* at an earlier edge, which is what sets `rfd_r0_pending`. What must
  be false is the term at the arm edge itself, which is exactly what the rest of the sentence
  establishes. Deliberately not corrected in this pass so the merged tip matches the reviewed
  tip; recommended wording: "arming requires `rfd_r0_cancelled` at this edge, and that is the
  exact complement of the widen term's line/row conjunction, so ...".
- **N-3 · Low — an unverified integration claim in the F-6 scope note.** "emerges from the
  armed flags only in specific geometries" asserts something about how our model behaves that
  nothing pins. No mechanism in the engine can suppress a `C4` reset; what does exist is the
  R4-cancellation path (`t13g`) advancing `C4` past the old total at the extended end, which is
  a different phenomenon from §8.6's second-frame statement. Per the AGENTS.md
  verification-ownership rule this is a boundary claim, unverified by default. Recommend
  dropping the emergence clause and keeping the plain "not modeled or vector-pinned".
- **N-4 · Info — bite-test phrasing.** "removing either gate term fails exactly `t13j`+`t13l`"
  is true only as a union across the two probes; individually each removal fails exactly one.
  See the table above for the measured per-probe result.
- **N-5 · Info — the debt row's scope cell still reads "vectors `t13e`-`t13k`".** Accurate as a
  description of what pass 1 targeted, now stale as a description of the branch. Left as
  historical scope; `audit-findings.md` and `current-status.md` both carry `t13e`-`t13m`.

### Evidence (rerun on branch tip `ab98c6b`, Verilator 5.050)

- `make -C sim` — `Summary: 109 passed, 0 xfailed, 0 xpassed, 0 failed`; Plus leaf, MMU, SDRAM
  cartridge and P0 boot suites green.
- `make -C sim lint` — 62 warnings, 0 errors: the same five CRTC ones as pass 1
  (`SNA_REGS` bits, `interlace[4:1]`, `R10_cursor_mode`, type-0 `DI[7]`, type-0 `row_last_w`)
  plus the known `rtl/sdram.v` and Plus test-top warnings. No new warning, and no
  UNUSEDSIGNAL appeared for `e1_rfd_r0_extend` (still consumed by `hcc_end`).
- `make -C sim soak SOAK_EXPECT=0x512eaae74a628dca` — matches. Per F-9 this is not behavioural
  evidence either way; it is consistent with the F-3 removal being provably inert.
