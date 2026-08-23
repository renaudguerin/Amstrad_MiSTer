# ACCC v1.10 Digest 01 — Counters (R0, R4, R5, R9) and Frame Sync

Source: *The Amstrad CPC CRTC Compendium* v1.10 (Longshot / Logon System), printed/PDF
pages 33-42 and 73-129.
Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
Scope: **CRTC type 0 (HD6845S/UM6845) and type 1 (UM6845R) only.** Types 2/3/4 (MC6845/ASIC)
noted only where directly contrasted, one line max.

Counter names per the book: **C0** = horizontal char counter (vs R0), **C4** = char-row counter
(vs R4), **C9** = scanline counter within a row (vs R9), **C5** = separate adjustment-line
counter used by CRTC 1/2 during vertical adjustment (CRTC 0 has no C5; reuses C9). VMA = current
video address, VMA' = latched "next row start" address. All C0 values below are **C0vs** (C0
referenced to the CRTC's own timeline), not the Gate-Array-delayed display timeline.

---

## 1. Building a frame — general logic (Ch.6, p.33-34)

- Idealized skeleton, NOT the real per-CRTC model (§6.1.1, p.33): C0++ each cycle; at C0==R0,
  C0→0 & C9++; at C9==R9, C9→0 & C4++; at C4==R4: if C5==R5 then C4→0,C5→0,MA=R12/R13, else
  C5++. Ch.10-13 override this with exact latch timing per C0 phase — treat as mnemonic only.
- HSYNC starts at C0==R2 (C3l=0, incr while C3l<R3l, ends at C3l==R3l). VSYNC starts at C4==R7
  (C3h=0, incr, ends at C3h==R3h, or ==16 if the field doesn't apply) (§6.1.2, p.33).
- DISPEN horizontal: enabled at C0==0, disabled at C0==R1. DISPEN vertical: enabled at C4==0,
  disabled at C4==R6 (§6.1.3, p.33).
- Precise video-pointer algorithm (§6.1.4, p.34):
  ```
  If C0==R0: C0=0
    If C9==R9: C9=0
      If C4==R4: If R5==0: C4=0, MA=R12/R13   else: C4=C4+1 (enter adjustment)
      Else: C4=C4+1
    Else: C9=C9+1
  Else: C0=C0+1
  ```
- C9 occupies VRAM address bits 11-13 (p.34 table confirmed from the text layer; earlier
  extraction-smearing flag retired by the 2026-08-22 review). On CRTC 3/4 the same bit position
  is occupied by C5 during adjustment, not C9 — one-line contrast only.

## 2. Synchronization principles (Ch.7, p.37-41)

- All timing in this book is referenced to **C0vs**: the instant C0 is considered 0 from the
  CRTC's own perspective, ~1µs ahead of the Gate Array's actual displayed character ("C0 from
  GA") (§7.1, p.37-38). For CRTC 0/1/2, **GA's HSYNC generation uses the CRTC's own
  (un-shifted) C0**, not the display-delayed value — a model's latch decisions and HSYNC output
  should both key off the same internal C0, with any pixel-display delay implemented as a
  separate downstream pipeline stage, not folded into the compare logic.
- VSYNC pin raised when C4==R7 — the single reliable sync point (§7.2, p.38). PPI port B bit 0
  mirrors it with no extra CRTC-side delay. Standard frame period: 312×64µs = 19968µs.
- Fake VSYNC (§7.3, p.41): PPI-wiring trick, out of scope for a CRTC-only model.

## 3. Register R9 — scanline counter C9 (Ch.10, p.73-79)

- R9 is 5 bits (0-31); only bits 0-2 feed the VRAM address ("curtain" wrap past C9=7), but full
  5-bit C9/R9 values are used for counting comparisons (§10.1, p.73).
- **R9 update is considered while C0<=R0** — live comparator input, effective at the next
  C0==R0 rollover (§10.2, p.74). ⚠ VERIFY p.74 (diagram-based timing table).
- **The CRTC does not buffer C9/C4 against R9/R4** — it re-evaluates the raw equality each
  cycle; only VMA' is actually latched (at C0==R1) (§10.3.1, p.75). If R9 is rewritten to 0
  while C9>0, C9 does **not** snap to 0 — it overflows up to 31 then wraps, unless the
  last-line exception applies (§10.3, p.74).

### 3.1 CRTC 0 — last-line and adjustment arbitration (§10.3.1, p.75-76, CRITICAL)

- CRTC 0 compares C9/R9 and C4/R4 while **C0<2** to establish `Last Line`. At C0==0 an
  R4/R9 write can still override the state. If `Last Line` was true at C0==0 and a write at
  **C0==1** breaks either equality, vertical adjustment becomes active even with `R5==0`; the
  current line is its first line.
- An otherwise armed last line is not yet an unconditional reset. At C0==2 the chip first
  arbitrates additional-line handling: `R5>0` activates adjustment and cancels `Last Line`.
  The `R0<2` and interlace routes can also select adjustment (§4.2/§7.1). Only when adjustment
  is not active does an armed `Last Line` become immutable for C0>1 and reset C4/C9 on the
  following line.
- With `Last Line` false, the live general case applies at rollover: C9==R9 increments C4;
  otherwise C9 increments. R9 updated ==C9 makes C9 reset on the next line; R9 updated below
  C9 makes C9 count through 31 and wrap; R9 updated above C9 increments C9 normally
  (§10.3.1.1, p.75).
- **⚠ Dynamic-update hazard — R9 written exactly at C0==R0** (p.76): C4 may still increment
  because C9 matched the earlier R9, while C9 is incremented under the comparison selected
  after the write. On a last line entering adjustment, that second comparison is against R5,
  not merely a second sample of R9 (§4.2). The documented R4=38/R9=7 exact-`C0==R0` example
  yields C4=39,C9=8; the windowed companion write (inside `C0∈[2,R0−1]`) yields C4=38,C9=8 —
  p.82 example 3, see §4.2. Both cases belong in t12.
- A single snapshot applied uniformly to C4 and C9 cannot reproduce this race. However, the
  Compendium prose does not by itself settle every RTL-clock ordering detail; use the focused
  hardware/trace vectors in `testbench-spec.md` before choosing an implementation seam.

### 3.2 CRTC 1 — "pure logic", no latch window (§10.3.2, p.76)

- R9 updated ==C9: next line, C9→0, C4 increments (wraps to 0 if it was R4, "offset
  considered" i.e. normal VMA reload on that C4=0 transition).
- R9 updated <C9: C9=C9+1, C4 unchanged, until overflow completes. **Offset/VMA reload only
  fires if C4==C9==C0==0 simultaneously** — narrower than CRTC 0's row-boundary reload.
- R9 updated >C9: C9=C9+1, C4 unchanged.
- **No last-line exception at all** — comparisons are always live, at every C0 (§10.3.2.2,
  p.76): "pure logic... unspeakable and tasteless simplicity." This is the structural
  difference from CRTC 0 a shared hcc/line/row model must special-case.

### 3.3 CRTC 0/1 divergence cross-check (§10.3, p.78-79 tables)

Cross-check tables (p.78-79) read cleanly in the text layer; the earlier heavy-smearing flag
was retired by the 2026-08-22 review:
- "C4=R4>0" scenarios, previous R9=7: CRTC 1 marks offset-reconsidered "Yes if C4=0" for
  intermediate C9-overflow cases (reconsiders R12/R13 whenever resulting C4==0, regardless of
  "last line"); CRTC 0 does NOT reconsider offset in those same intermediate cases, only at
  true frame-end reset.
- "R9 updated to 0, C4==R4==0" case: CRTC 0 result depends on whether the write landed C0<2 vs
  C0>1 (the freeze rule above); CRTC 1 has no such split (always same result, any C0 phase).

## 4. Register R5 — vertical adjustment counter (Ch.11, p.80-91)

- **CRTC 0 has no separate C5 — reuses C9**, comparing C9 vs R5 instead of R9 once in
  adjustment. **CRTC 1 has a genuine separate C5** alongside C9 (C5 counts adjustment "lines",
  C9 keeps counting "characters" within each using R9) (§11.1, p.80-81).
- C4 behavior during adjustment (R5>0):
  - **CRTC 0**: C4 increments **once only**, on the transition into "last line" (frozen before
    C0==3 of that line). No further increments; C9 vs R5 (not R9) governs end of adjustment.
  - **CRTC 1**: C4 increments **every time C9 reaches R9** during adjustment (regardless of R4)
    as long as C5<R5. First additional line: `C4=R4+1` when C9==R9 fires.

### 4.1 Worked example (R4=10, R5=16, R9=3, R1=40, R0=63) (§11.2.1, p.81)

- CRTC 0: C4 freezes at 11 (R4+1) for the whole adjustment; C9 counts 0..15 continuously,
  addressing cycles every 4 steps (R9=3 → 4 distinct addresses).
- CRTC 1/2: C4 increments once per 4 C9-steps (11,12,13,14) while C5 counts 0..15 continuously
  across those increments; VRAM pointer still derives from **C9** (0..3 repeating), not C5.
  ⚠ VERIFY p.81 exact table rows.

### 4.2 CRTC 0 adjustment detail (§11.2.2, p.81-83)

- CRTC 0 reuses C9 for adjustment. Its end limit becomes **R5 at the beginning of the line**;
  R9 remains a separate comparison for video-pointer capture at `C0==R1 && C9==R9`.
- Entry has three non-interlace routes: `R5>0` before C0 reaches 3 on a last frame line;
  `R5==0` when an R4/R9 write at C0==1 breaks equality that was true at C0==0; and the
  default-adjustment route when `R0<2` prevents C0 from reaching the C0==2 cancellation point.
  Entry cancels `Last Line`.
- On the last frame line before adjustment, with no R4/R9 write, C9 resets to 0 and C4
  increments. Once C4 differs from R4, C9 is no longer reset at R9; it can exceed R9 and count
  to R5. When the R5 count completes, `Last Line` is re-established, allowing C4/C9 to reset
  on the following line.
- Writes on that entry line change the selected comparison:
  - R9 written from `C0==2` through `C0==R0-1`: compare C9 with the new R9 and increment C9
    when they differ. C4 is not yet incremented, so C9/R5 handling begins only once C4 differs
    from R4.
  - R4 written from `C0==2` through `C0==R0`: C4 now differs from R4, switching the line-end
    comparison from C9/R9 to C9/R5; C9 increments and is tested against R5.
  - R9 written exactly at `C0==R0`: the earlier C9/R9 match can increment C4, then the changed
    C4/R4 result switches C9 to the R5 comparison. Both C4 and C9 can therefore increment;
    the documented R4=38/R9=7 example ends at C4=39,C9=8.
  - Companion case (p.82, example 3): an R9 write landing inside the same `C0∈[2,R0−1]` window
    (rather than exactly at `C0==R0`) leaves **C4=38, C9=8** on the next line — C4 is not yet
    incremented. Encode both documented results in t12 (`testbench-spec.md`).
- **VMA'/offset capture keeps working during adjustment** through the independent live
  `C0==R1 && C9==R9` comparison, so R9 changes can still redirect the saved pointer
  (§11.2.2, p.82-83).

### 4.3 CRTC 1 adjustment detail (§11.2.3-11.2.4, p.83-84)

- VMA'/VMA transfer at `C0==R1` uses the live C9==R9 test, same as normal operation; R9 update
  is live against current C9 counting during adjustment too.
- Interlace line (if added on top of R5 lines) is just one more appended line, same C4 as the
  last R5 line.
- **⚠ VMA source switches during first adjustment line** (IMPORTANT, §11.2.4, p.83-84): if C4
  was 0 immediately before adjustment began, **VMA loads from R12/R13 (not VMA') for as long as
  C4==1** in adjustment (the new post-increment C4). This suspends the normal "R1 gates VMA'"
  logic for that row — offset changeable on every C9 line of that C4==1 row, like RFD. Caveat:
  only holds if R4 was NOT rewritten to >0 exactly at C0==R0 entering adjustment; if it was,
  VMA is NOT updated from R12/R13 at C4==1. Conversely (§11.2.4 note, p.84): an **R9** write
  landing exactly at `C0==R0` entering adjustment does **not** cancel this behaviour — only the
  R4(>0) rewrite does. (F8 corner rule; the untested corner is recorded in docs/review-debt.md.)
- Adjustment becomes **irreversible** only once `C4==R4 && C9==R9` is true exactly at
  `C0==R0`. Before that instant, rewriting R4/R9 so the equality fails aborts/postpones
  adjustment. But if the equality still holds AND R4/R9 are rewritten *simultaneously* at that
  same C0==R0, adjustment stays committed using the **new** R4/R9 values for the ensuing count.

### 4.4 R5 updated mid-adjustment (§11.3, p.84-85)

- Both CRTC 0/1: if R5 is rewritten to exactly `C5+1` (CRTC1) / `C9+1` (CRTC0) on the current
  adjustment line, adjustment stops — next line **C4=C9=0 unconditionally**, regardless of C9's
  actual value, unless interlace conditions apply.
- CRTC 0 (§11.3.1, p.85): reaching R5 stops adjustment; R5 rewritten to LESS than C9+1 →
  overflow (counts to 31, wraps, continues to new R5).
  - **⚠ Timing caveat**: R5 update on the last line is honored only **while C0<3**. If R5 is
    updated when C0>2 on that last line, the update is **ignored** — C4/C9 reset to 0 as if
    adjustment ended normally.
- CRTC 1 (§11.3.2, p.85): same "reaches R5 stops" logic on `C5+1`; less-than → C5 overflows.
  - **⚠ Bug: R5 set to 0 during adjustment does NOT reset C4** (IMPORTANT): CRTC 1 arms an
    internal "additional management active" state when R5>0 at the moment C4 would otherwise
    reset at frame end; normally cleared when `C5+1==R5` (which also resets C4). **If R5 is set
    to 0 while active, the state is NOT cleared** — C4 stays nonzero, C5 free-runs, because
    `C5+1==R5` can never be satisfied against R5=0. C4 still compares against R4 for its own
    logic, but adjustment-active persists. **Only way out**: set R5 to a value >0 that C5+1
    will actually reach; then the state clears and C4 resets. This lets you force C4/C9 to 0 on
    an arbitrary line — an exploit technique, and a trap for models assuming "R5=0 ⇒ adjustment
    ends this line."

### 4.5 R5 update BEFORE adjustment starts (§11.4, p.86)

- CRTC 1: R5 evaluated live at every C0. **Writing R5>0 exactly at C0==R0, when R5 was
  previously 0, triggers RFD** (§5) — one of two RFD trigger routes.
- CRTC 0 (§11.4.2, p.86): R5>0 update arriving **after C0>2** on the last frame line is not
  considered at all (window closed — cross-ref §7.2's C0==2 instant). Next line is simply
  C4=C9=0 unless an interlace line was separately scheduled.

### 4.6 VSYNC during vertical adjustment (§11.5, p.86)

- CRTC 0/1: R7 can be set to any value C4 passes through during adjustment and will trigger
  VSYNC there — adjustment doesn't inherently block VSYNC.
- **CRTC 0 exception**: VSYNC is **blocked** if R7 is updated with the current C4 value **while
  C0<2** (same underlying arm/disarm mechanism as §7.3 generalized to adjustment C4 values).

## 5. "Rupture For Dummies" (RFD) — CRTC 1 only (Ch.11.6, p.87-90)

The single most intricate dynamic-update bug in the CRTC-1 model.

- **Trigger**: writing R5 **non-zero**, at **C0==R0**, on a line where R5 was previously **0**.
  (R5>0→0, or R5>0→another >0, does NOT trigger.) Second trigger route via R0-update
  interaction: §8.7.
- **Two independent flags activated** (§11.6, restated §11.6.2 p.88-89):
  1. **VMA-source flag**: normally VMA loads from R12/R13 only at C4==0/C0==0; RFD forces this
      "load from R12/R13" to stay **true regardless of C4** — address changeable on every row
      for the rest of the frame. Disarmed again once `C9==R9` at `C0==R1` next succeeds (see
      parity below). **R1>R0 disarm path** (p.87): if R1>R0, `C0==R1` can never fire, so the
      bare `C9==R9` match alone deactivates the VMA-source state. F7's design must include
      this route.
  2. **Parity-management flag** in the C9==R9 test at C0==R1 (used by IVM): RFD arms
     consideration of frame parity in that test, otherwise parity-blind.
- **Frame-parity alternation** (§11.6.1, p.88-89): once armed, C9==R9-at-C0==R1 becomes
  parity-dependent:
  - "Case 1" frame (parity makes the test read false): VMA' NOT updated at C0==R1 — characters
    repeat all frame; VMA-source flag never disarms (address stays freely updatable, R12/R13,
    every row). C4 still increments normally elsewhere.
  - "Case 2" frame (opposite parity, test reads correctly): VMA' updates normally at C0==R1;
    VMA-source flag disarms as soon as that reload happens, restoring normal per-C4==0-only
    R12/R13 loading for the rest of the frame.
  - Parity flips at every frame boundary where `C4==C9==C0==0 && R9 is odd`. **A model treating
    RFD as static (no per-frame parity toggle) will get every other frame wrong.**
- **"IVM ON/OFF" freezes parity** (§11.6.2, p.88-89): `OUT R8,3` then `OUT R8,0`, timed on an
  **even C9** line (odd C9 risks corrupting the count via bit-0 flip). Before an RFD → locks in
  "Case 1" (repeat-every-row) for all following frames. After an RFD → locks in "Case 2"
  (single reload per row, offset changeable until C9 diverges from R9).
- **RFD#10, CRTC "1-B" only** (§11.6.2, p.89, ⚠ unverified hardware taxonomy — no reliable way
  to identify by chip markings; author found 3/7 tested UM6845R chips exhibit it): writing R5
  with the value **10 (decimal)** at the RFD trigger instant **deactivates** the
  parity-management flag instead of activating it, on "1-B" chips only ("1-A" chips, and other
  R5 values on 1-B, behave as ordinary RFD). Once the parity-flag disposition is set for a
  frame (armed by plain RFD, or disarmed by RFD#10), **it cannot flip again until the frame
  ends**. Recommend modeling as an optional chip-variant quirk, not baseline CRTC-1 behavior.
- **Recipe** (§11.6.3, p.90): `OUT R5,1` then `OUT R5,0`, with the R5,1 write landing exactly
  at C0==R0 on a line where C9!=R9 for the target C4. Modify R12/R13 the line *before* the R5
  update takes effect.
- **CRTC 3/4 do not have RFD.** HITACHI's own CRTC-0 errata table alludes to a vaguely similar
  R5-at-C0==R0 caveat but the author flags it self-contradictory (⚠ VERIFY p.90 — "a real
  playground"); do not port RFD to CRTC 0 without independent verification.

## 6. R6 and vertical adjustment; interlace interactions (§11.7-11.9, p.90-91)

- R6 gates row display (DISPEN low at C4==R6), same general rule — must be positioned to match
  whichever C4 values occur during adjustment if those rows should display data.
- **CRTC 1 special R6==0 case** ("split-border"): BORDER activation via C4==R6 is handled
  non-persistently; same mechanism applies during adjustment (⚠ VERIFY p.90 — mechanism
  asserted but not spelled out in extracted text; cross-ref Ch.19.2).
- Interlace adjustment line: independent of, and appended after, any R5 lines. Its condition is
  evaluated **on the last line of the frame, at C0==R0**, using whatever R8 value is active
  **at that instant** — R8 can legally be toggled on one of the R5-adjustment lines to switch
  the interlace-line behavior on/off for that frame.

## 7. Register R4 — character row counter C4 (Ch.12, p.92-101)

- C4 increments when C9==R9 (wraps to 0 if C4==R4). Compared to R7 (VSYNC) and R6 (BORDER).
- Outside vertical adjustment, if R4 is already less than C4, equality is not a magnitude
  catch-up: C4 continues to its 7-bit limit (127), wraps, and counts onward toward R4.
- **CRTC 0/1 both allow C4>R4 during adjustment** (CRTC 3/4 clamp C4==R4 — contrast only).
- VMA reload from R12/R13 fires when C4 transitions to 0, and for CRTC 1 specifically "while it
  is 0" — potentially reconsidered throughout the whole row where C4==0, not just at the
  transition instant (§12.1, p.92) — general form of the §4.3 adjustment special case.

### 7.1 CRTC 0 (§12.2, p.92-94)

- **Core windowing rule**: C4/R4 and C9/R9 establish `Last Line` while C0<2. A write at C0==0
  can override the state. A write at C0==1 that breaks an equality which was true at C0==0
  activates vertical adjustment even with R5==0.
- `Last Line` must arbitrate adjustment before reset. If no additional line is selected, later
  writes at C0>1 cannot change the following-line C4/C9 reset. If adjustment is selected
  (`R5>0` before C0 reaches 3, `R0<2`, or the applicable interlace case), entry cancels
  `Last Line`; completion sets it again, so a C4 that exceeded R4 returns to 0 on the next line.
- To arm `Last Line` on the first frame line while C4==C9==0, **one limit must already be 0 and
  the other positive**: either R9==0/R4>0 or R4==0/R9>0. Write the remaining positive limit to
  0 while C0<2 (e.g. begin `OUT(C),reg8` at C0vs==#3E for R0==#3F). This is the precise
  zero-limit condition; a loose "R4>0 or R9>0" test is insufficient.
- Outside adjustment, if C4 has passed a newly lowered R4, it advances on ordinary C9==R9 row
  completions through 127 and wraps. Adjustment is the separate, expected route by which C4
  can exceed R4 and then return to 0 through completion/re-arming.
- **RLAL case study, first-line variant** (§12.2.1, p.93): setting R9=R4=0 after the C0<2
  window while C9==C4==0 (first line) does NOT make that line "last line" — the test already
  ran and found inequality before the write landed. Fix: write **R4=1** (not 0) with R9=0;
  C0>1 is fine here because the state is
  evaluated at the *next* line's C0<2 window). Line 2: C4 becomes 1, C4==R4(1) && C9==R9(0)
  true at that line's C0<2 → arms → line 3 resets C4=C9=0. To perpetuate, change R4 back to 0
  on line 2 while C0>1.
- **RLAL from the genuine last line** (§12.2.1, p.94): rewrite R9=R4=0 on the last line or the
  next, **while C0<2**, to perpetuate C9=C4=0 on every subsequent line. **Caveat**: after C9
  first becomes ==R9, must **wait until C0==2** before writing R9 again — CRTC 0 needs C0∈{0,1}
  exclusively for its own internal bookkeeping around that transition.

### 7.2 CRTC 1 (§12.3, p.94)

- R4 rewritten to current C4 value: if currently between rows 0..R9-1, C9=C9+1 next line (C4→0
  with reload only once C9 itself later wraps). If currently on last C9 row (C9==R9): C9→0,
  C4→0, R12/R13 reload **immediate**.
- R4 rewritten <current C4: overflow toward max (127) before wrapping — analogous to §3.2.
- **Contrast with CRTC 0**: R4=0 on the last line causes C4 to *overflow* on CRTC 1 (0 is just
  an ordinary value, no special-casing) — CRTC 0 treats R4=0 as a legitimate frame-end target
  via its last-line latch. **To loop C4 back to 0 on CRTC 1, set R4=0 only when C4 is already
  0** — no latch/lookahead exists.
- RLAL on CRTC 1 is "trivial": set R4=0 and R9=0 simultaneously while C4==0 and C9==0 already
  hold — no C0-phase timing constraint (consistent with "pure logic, always live", §3.2).

## 8. Register R0 — horizontal counter C0 (Ch.13, p.102-129)

- C0 ("HCC") counts 0..R0 inclusive; R0 = char-count-per-line minus 1. C0 reset to 0 potentially
  updates C4/C9/C5.
- **Key asymmetry**: CRTC internal counters run ahead of the GA display. For CRTC 0/1/2, **GA's
  HSYNC generation uses the CRTC's own un-shifted C0**, not the display-delayed value (ASIC
  CRTC3/4 do shift — contrast only).

### 8.1 CRTC 0 — the "first 3 microseconds" state machine (§13.2.1, p.103-104, CRITICAL)

The chip spreads end-of-line/end-of-frame decisions across three distinct instants:

- **At C0==0**: C4/C9 updated per decisions armed on the *previous* line. Fresh tests then run
  for the *next* C0==0: if C9==R9 (post-update), schedule "C4 increments next time" — **this
  can still be cancelled at C0==2** of the same line if it turns out not to be the true last
  line (C4!=R4 or C9!=R9), or if adjustment/interlace lines need to be inserted instead.
- **At C0==1**: re-authorizes C9-counting-management for the upcoming C0==R0 event. If this
  authorization does NOT fire (happens iff R0==0, so C0 never reaches 1), C9's management for
  the next rollover is not armed → C9 freezes. Also: if R0 is set to 0 exactly at C0==0, C9
  gets one last update against the *previous* line's R9, provided that line's R0 was >1. The
  last-line true/false decision is (re)computed here too, matching the C0==0 computation.
- **At C0==2**: on a genuine last frame line, decides whether adjustment management actually
  begins (R5 / interlace-line conditions evaluated **at this instant**). If it begins, C4/C9
  switch to the adjustment two-step (§4.2).

- **Consequences of R0==0** (§13.2.1/§13.2.4, p.104-105): C0 never advances past 0, so the
  C0==1 re-authorization never runs:
  - C9!=R9 at the (only) C0==0 when R0 became 0 → **all counters freeze** entirely while
    R0 stays 0. The cost is wall-clock display time — counters are frozen, nothing progresses;
    the source's own figure is that freezing R0=0 for 64×8 µsec "forgets" 8 raster lines
    (p.104), i.e. an N-µs stall costs N/64 lines. The caption's "(C4−1 if R9=7)" gloss is
    unresolved — author-question Q2.
  - C9==R9 at that instant → **C4 increments exactly once** on the *second* C0==0 occurrence
    (already-armed decision, not cancellable until C0==2, unreachable while R0==0); everything
    (incl. C4) freezes after that second C0==0.
  - **R4/R5/R9 writes are ignored while R0==0** (the logic that applies them never runs).
    **R8 remains live**, still evaluated every C0==0.
  - R0 later >1 again → normal incrementing resumes seamlessly from whatever was frozen.
  - **Adjustment-state interaction**: if last-line conditions were satisfied right before R0
    froze at 0, the chip is **locked into comparing C9 vs R5 instead of R9** for the rest of
    adjustment — **not undone by later passing through C0==2** once R0 unfreezes; only stops
    when the (frozen-then-resumed) C9 count reaches R5.
  - Worked test vectors (§13.2.6, p.108-109), all regs C0=R0=C4=R4=C9=R9=R5=0: 1st C0==0 → VMA
    reload (VMA'=VMA=R12/13). Loop to (still) C0==0 → this IS "end of frame" (C4==R4/C9==R9
    already) → C4→1 (adjustment entered); C9 does NOT truly reset (frozen, "no longer
    managed"). Further C0==0 cycles: frozen C4=1,C9=0. R0 later >2 → adjustment can't be
    cancelled this late; next real C0==0: C9+1 vs R5 → if different, C9→1. (Example 2: if R0
    set back to 0 on the very next line, C9 re-freezes at 1, adjustment-active persists.)
- **Case R0==1** (§13.2.1/§13.2.5, p.104-108): C0 alternates 0,1,0,1... (2µs "lines"), never
  reaching 2, so the adjustment **disarm** step never runs — if C4==R4 && C9==R9 fires at
  C0==0/1, adjustment arms and **stays armed** (uncancellable) even though it lasts only as
  long as R5 dictates once reachable. With R4=R9=0 and R5=0 the frames strictly **alternate**
  normal/additional: each additional "frame" lasts one 2µs line before ceasing ("lasts 1 line
  of 2 µsec before ceasing", p.104; alternating pattern in the p.107 case study). The chained
  into-another-adjustment-frame reading applies only while R5>0. ⚠ VERIFY p.105-108
  (chronogram partially garbled in extraction — cross-check exact screen-grid before using as
  test vectors).

### 8.2 CRTC 0 — VSYNC freeze window (§13.2.2, p.104-105)

- Each C0==2, a flag (re)arms authorizing "check C4 vs R7 at the next C0==0" (allow VSYNC to
  fire there). Cleared again at C0==0 (single-shot per line).
- **⚠ VSYNC-blocking hazard**: R0 rewritten <2 on the line immediately preceding the C4==R7
  match (C0 never reaches 2 there to re-arm) → **VSYNC will NOT fire** on the following line.
  Likewise, rewriting R7 to the about-to-be-current C4 value while C0<2 (intending to trigger
  VSYNC right after this C0==0) **also blocks** VSYNC for that C4==R7 value: **cannot be
  unblocked for this same C4 value** except two escape conditions the book defers to Ch.16.3
  (cross-reference only, out of scope here).

### 8.3 CRTC 0 — additional-line freeze under R0==0 (§13.2.3, p.105)

- Adjustment row active (e.g. R5==1, C4==R4+1, C9==0) and R0 set to 0 exactly at C0==0 of that
  row: C9 stays fixed at 0; C4 proceeds to 0 only once C9-management resumes (C0 reaching 1
  again, requires R0>0). Rewriting R5 while frozen has no effect until C0 actually reaches 1.

### 8.4 CRTC 0 — R0==1 overflow trap (§13.7.2, p.124-126)

- General: at C0==R0, C4 increments unless reset-to-0 was already scheduled; this reset becomes
  fully effective starting at C0==2.
- **⚠ Hazard**: R0 (re)programmed to **1**, and the line reaches `C0==1` while `C9==R9` →
  **C4 overflows WITHOUT C0 or C9 resetting**, even if `C4!=R4`. Mechanism: `C0==R0` comparison
  (R0=1) fires *before* the write is applied for that purpose, entering "additional management"
  unconditionally (C4 forced to increment regardless of R4; C9 can no longer reset even on
  C9==R9) — but R0's new value is still picked up in time for the *next* C0 increment decision.
  Same hazard class as §3.1's R9-at-C0==R0 case, but with R0 itself threading between two
  internal uses within one cycle.
  - Worked example (R9=7, R4>=6; `OUT R0,1` then later `OUT R0,63`): on the line where C9
    reaches 7 (==R9) and C0 reaches the (temporarily 1) R0:
    - Not a true last line (C4!=R4): disarm never runs (R0=1 keeps C0<2) → C4 overflows past R4
      once C0 finally reaches 2 after R0 is widened back; needs an R9 compensation trick to fix
      line count (§13.7.2.1, p.125).
    - True last line (C4==R4): same overflow, but chip is left in "additional management"
      state afterward — recoverable by programming R5 once R0 is widened back, or letting C9
      free-run to R5 (§13.7.2.2, p.125-126).
  - **Mitigation**: to widen R0 from 1 without this side effect, the widening write must land
    while **C0==0** (not C0==1).

### 8.5 CRTC 1 — R0 accepts any value freely (§13.3, p.113)

- No special-casing: R0 rewrites (incl. to 0) never disturb C9/C4 counting (contrast with
  CRTC 0's C0<2/C0==2 machinery above).
- **⚠ Instruction-dependent write-timing hazard — OUTI vs OUT(C),reg8** (§13.3/§13.7.1.1,
  p.113/124): the µs at which R0's new value reaches the C0==R0 comparator depends on which Z80
  instruction wrote it:
  - `OUT(C),reg8`: effective from the instruction's **3rd µs**.
  - `OUTI`: effective from the instruction's **5th µs**, but sampled a further cycle later —
    text states **"R0 is actually modified 6µs after the start of OUTI"** for comparator
    purposes — an extra +1-cycle lag specific to OUTI vs how CRTC 1 samples it, not present for
    OUT(C),reg8.
  - Concretely: `OUTI` shrinking R0 (e.g. 49→20) can cause C0 to overflow past where it
    "should" reset if the write's effective 6th-µs lands exactly on the cycle following the old
    C0==R0(old) match — comparator sees the *new* R0 in cases where an equivalent-looking
    `OUT(C),reg8` two µs "later" would not. **A model treating all I/O as atomic at
    "instruction-start + fixed offset" needs a +1-cycle correction specifically for CRTC-1 OUTI
    writes to R0** relative to OUT(C),reg8.
  - **§13.6 R0 UPDATE, p.122-123** — §13.6.1 "CRTC 0, 2 : CHRONOGRAM" and §13.6.2 "CRTC 1 :
    CHRONOGRAM", both p.122 (§13.6.3 covers CRTC 3/4, p.123). Do not confuse these with
    §13.7.1.1 "R0 UPDATE: OUTI" p.124, which is the instruction-timing hazard above.
    ⚠ the figures themselves are still only available here as flattened text, alignment and
    braces lost, but the annotation quoted below was re-checked verbatim against the PDF on
    2026-08-24 and is exact. Textual gist: CRTC0/2 and CRTC1 both show a
    "too-late write not considered until following line" vs "just-in-time write considered
    this rollover" two-outcome split; CRTC1's figure additionally marks: **"RFD activated on
    CRTC 1 if R4 and/or R9 modified until C0=7F (new R0) on last line of frame"** — widening R0
    on the last line, combined with R4/R9 writes through the new (widened) R0 value, itself
    triggers RFD (§8.6 below, second trigger route for §5).

### 8.6 CRTC 1 — R0-update-driven RFD trigger (§13.7.1.2, p.124)

- R0 widened via `OUT(C),reg8` exactly at `C0==R0` on the **last line of the frame** (where
  `C9==R9 && C4==R4 && R5==0` beforehand), AND the last-line condition is then *cancelled* (by
  also rewriting R9/R4) during the newly-widened remainder of that line → **arms RFD**, firing
  at the line's actual end, behaving per §5 including frame-parity alternation.
- If **R9** changed (C9!=R9 by line end): R12/R13 considered, but **1 frame out of 2**
  (parity) VMA' fails to update from VMA — line repetition, exactly Case-1/Case-2 from §5.
- If **R4** changed (C4!=R4 by line end, C9==R9 still held): R12/R13 considered, and **on the
  second frame specifically, C4 is NOT reset to 0** (stuck C4 instead of a repeated line).
- Both variants: "IVM ON/OFF" (§5) freezes parity and stabilizes cross-frame behavior.

### 8.7 CRTC 2/3/4 R0 — one-line contrast (§13.4-13.5, out of scope)

- Also accept any R0 without breaking C9/C4 (like CRTC 1), but have unrelated quirks (GHOST
  VSYNC, HSYNC-during-adjustment inhibits, ROM-select I/O race on CRTC 3) irrelevant here.

### 8.8 Offset (R12/R13) latch timing vs C0 (§13.8, p.126-129)

Diagrams for very short lines (R0=3,1,0) show exactly which C0 must be reached before an
R12/R13 write is honored for VMA. Initial conditions: R4=0, R9=0, R1=4, R13=0.

- **R0==3 (4µs "frames")** (§13.8.1, p.127): CRTC 0 and CRTC 1 behave identically — `OUT R13,4`
  is picked up starting the VRAM-address cycle immediately following the write, effective once
  C4 returns to/stays at 0 (R4=0 here so every row is row 0, fires every 4µs frame). ⚠ VERIFY
  p.127 exact column alignment (grid partially smeared).
- **R0==1 (2µs "frames")** (§13.8.2, p.128, IMPORTANT): explicit caption "the event C0==R0
  after 2µs leaves C4=1 for the 2nd period of 2µs, which represents a vertical adjustment 'not
  cancelled' (because C0 is never equal to 2)" — concrete restatement of §8.1/§8.4's "R0==1
  traps C4 in uncancellable adjustment", **CRTC 0 only** — CRTC 1/3/4 in the same figure do
  NOT show this trap (consistent with §8.5's "R0 accepts any value").
- **R0==0 (1µs "frames")** (§13.8.3, p.129, IMPORTANT): explicit caption "R12/R13 cannot be
  considered until C4 and C9 both go back to 0" for CRTC 0 — under the R0==0 freeze (§8.1), VMA
  reload is stuck too, gated on the same suppressed C4==0-transition event. **CRTC 1 in the
  same scenario continues to reload correctly** (reaches new address a few µs after
  `OUT R13,4`, unaffected by R0==0) — CRTC 1 has no analogous freeze state when R0==0.
- ⚠ VERIFY p.127-129 — grids extracted as flattened numeric rows without source
  color/alignment cues; qualitative conclusions above are explicit in the prose (safe to rely
  on), but exact µs-by-µs address values should be re-checked against the PDF figures before
  use as cycle-exact test vectors.

## Deliberately excluded

- CRTC 2/3/4 "Last Line Management" arbitration state machine (§12.4.1) — substantially more
  complex, out of scope; only one-line contrasts kept above.
- R.V.I./R.V.L.L. programming-technique lookup tables (§13.3.1, §13.4.1, pp.113-120) — demo
  cookbooks (which R0/R2/R9 pokes reach a target C9), not hardware rules per se; underlying
  rules (C9 overflow, last-line latch windows) already captured in §3-§4/§8.1/§8.4.
- Fake-VSYNC/PPI wiring detail (§7.3, Ch.7) — out of scope for a CRTC-only model.
