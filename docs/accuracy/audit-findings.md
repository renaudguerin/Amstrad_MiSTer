# UM6845R.v Accuracy Audit — Findings & Fix Prompts

Audited: `rtl/UM6845R.v` (347 lines, behavioral CRTC model, types 0 & 1 via `CRTC_TYPE` input;
`CRTC_TYPE=0` → type 0 HD6845S/UM6845, `CRTC_TYPE=1` → type 1 UM6845R).
Reference: "The Amstrad CPC CRTC Compendium" v1.9 (Logon System), via the digests in this
directory (`compendium-01-counters.md`, `compendium-02-sync.md`, `compendium-03-display-regs.md`).
Cite chains are `digest §section → ACCC §chapter, p.N`.

Date: 2026-07-07. Auditor: Claude (Fable 5), cross-referencing digests against the full RTL.

## How to read this document

Each finding has: **Rule** (what the hardware does), **Current** (what the RTL does, with line
references), **Impact** (what software breaks), **Confidence** (how sure we are the fix is right
without hardware/sim verification), and **Fix prompt** (a self-contained instruction block for an
implementation agent). Findings are ordered by `priority = confidence × impact ÷ risk`.

Verification levels referenced below:
- **V1** — visual: boot BASIC, run known games/demos, compare against real hardware or WinAPE/ACE.
- **V2** — SHAKER test suite (Longshot's companion tests for the Compendium) on the built core.
- **V3** — Verilator testbench assertion (see `docs/accuracy/testbench-spec.md` once written).

General implementation rules for all fix prompts:
1. One finding = one commit. Commit message format: `CRTC: <finding id> <summary>`.
2. Never regress the other CRTC type: every change must be gated on `CRTC_TYPE` unless the rule
   is explicitly universal.
3. `UM6845R.v` uses `CLKEN` as the 1MHz character-clock enable; "per C0 cycle" rules belong inside
   `if (CLKEN)` blocks. Register writes from the Z80 arrive via the `ENABLE & ~nCS & ~R_nW` path
   at full `CLOCK` rate — write-vs-counter races are races between that block and `CLKEN` blocks.
4. Existing register/counter names: `hcc`=C0, `line`=C9, `row`=C4, `hsc`=C3l, `vsc`=C3h,
   `in_adj`=adjustment active, `row_addr_r`=VMA, `row_addr`=VMA'.

---

## F1. R10/R11 must not be readable on types 0 and 1  ⭐ smallest, highest-confidence fix

- **Rule** (digest-03 §21.2 → ACCC §21.2, p.241-242): readable registers are R12-R17 on type 0,
  R14-R17 (+dummy 31) on type 1. **R10 and R11 are not readable on either type** (only on
  types 3/4). Unrecognized register numbers read 0.
- **Current** (`UM6845R.v:84-85`): `10: DO = {R10_cursor_mode, R10_cursor_start}; 11: DO = R11_cursor_end;`
  — returns stored values on both types.
- **Impact**: CRTC-detection routines that tabulate readable registers (ACCC §28.1.9 method)
  mis-identify the emulated chip; any software using the readback-register signature.
- **Confidence: high.** Direct table lookup, no timing subtlety.
- **Fix prompt**:
  > In `rtl/UM6845R.v`, in the combinational read block (`always @(*)` around line 79), make
  > register numbers 10 and 11 return `8'h00` for both CRTC types (delete the two case arms;
  > the `default: DO = 0` covers them — but keep an explicit comment noting R10/R11 are readable
  > on CRTC types 3/4 only, per ACCC §21.2). Do not touch the write path (R10/R11 must remain
  > writable — they still drive the CURSOR output). Verify: registers 12,13 still read back on
  > type 0 only; 14,15 on both; 31 returns 0xFF on type 1 / 0x00 on type 0.
- **Verify**: V3 (register-readback table assertion); V2 (SHAKER ID tests).

## F2. Type 1 status register: bit 5 must be the R6-border condition latched at C0=R0

- **Rule** (digest-03 §21.3.3 → ACCC §21.3, p.243): type 1's `&BE00` status bit 5 reflects the
  **R6-border condition** (`C4=R6` reached), **sampled only at C0=R0**, not continuously. The
  "R6=0 forced border while C4>0" state is NOT reflected in bit 5. Bits 0-4 and 7 read 0.
- **Current** (`UM6845R.v:95`): `DO = vde ? 8'h00 : 8'h20;` — continuous `vde`, which conflates
  the R6=0-forced border (which *does* clear `vde`) with the C4=R6 condition, and is not latched
  at C0=R0.
- **Impact**: software polling `&BE00` for raster position (documented technique, ACCC §24.5
  OUTI/status tricks) sees transitions up to a line early/late; detection routines checking the
  latch behavior (§28.1.8) diverge.
- **Confidence: medium-high.** The sampled-at-C0=R0 rule is clean prose; the exact
  ⚠ VERIFY p.243-244 diagrams should be consulted if the testbench disagrees.
- **Fix prompt**:
  > In `rtl/UM6845R.v` add a registered `status_bit5` updated only when `CLKEN && hcc_last`:
  > set to 1 if the R6-border condition is active due to `row` having reached `R6_v_displayed`
  > (i.e. the vertical-display flip-flop was cleared by the C4=R6 comparison), 0 otherwise.
  > It must NOT go high merely because R6 was set to 0 mid-frame while C4>0 (the type 1
  > "R6=0 forces border" path). You will need to separate "border because C4 hit R6" from
  > "border because R6==0" — add a distinct flag for the former, set where `row_next ==
  > R6_v_displayed` clears `vde` at `row_new`, and in the dynamic-R6-write handler only where
  > the write hits `row == DI` (not the `DI==0` arm). Replace line 95's `vde` with the new
  > latched bit: `DO = {2'b00, status_bit5, 5'b00000}`. Type 0 behavior (returns 8'hFF default)
  > unchanged.
- **Verify**: V3 (bit-5 transition timing vs C0=R0); V2.

## F3. CRTC 0 mid-line R7 write: VSYNC blocked at C0vs<2, else starts mid-line with duration +（R0−C0vs)

- **Rule** (digest-02 §19 → ACCC §16.4.1, p.165-167): on type 0, writing R7=C4 mid-line triggers
  VSYNC **immediately** — *except* when the write lands at C0vs ∈ {0,1}, which produces a
  **BLOCKED VSYNC**: no pulse, and no VSYNC can fire for this C4=R7 value until the comparison's
  truth value changes. When it does fire mid-line (C0vs>1), the line counter C3h starts counting
  **from the next line start**, so total duration is *extended* by `R0−C0vs` µs.
  On type 1 (§16.4.2): the write triggers immediately at **any** C0vs, and the partial line
  *counts* as line 1, so duration is *reduced* by `C0+1` µs. (PPI-visible latency: 6µs type 0
  vs 5µs type 1 — a documented detection vector.)
- **Current** (`UM6845R.v:306-313`): R7-write handler triggers `VSYNC_r` for both types
  identically when `row == DI`, with an explicit `// TODO: extra conditions for CRTC0`. `vsc`
  then decrements at the next line event for both — i.e. both types get type-1-style "partial
  line counts" semantics, and type 0's C0vs<2 blocking is absent.
- **Impact**: ACCC §28.1.3/§28.1.4 CRTC-identification via VSYNC timing/length distinguishes
  exactly this; demos synchronizing off mid-line R7 tricks ("PHX" is already name-checked in the
  code) get one-line-off VSYNC on type 0.
- **Confidence: medium-high.** Rules are clean prose; the blocked-VSYNC escape conditions
  (ACCC §16.3) are already approximated by the existing `vsync_allow` mechanism.
- **Fix prompt**:
  > In `rtl/UM6845R.v`, rework the R7-write handler (lines 306-313):
  > (a) Type 0 (`!CRTC_TYPE`): if `row == DI[6:0] && !VSYNC_r`, and current `hcc >= 2`, assert
  > `VSYNC_r` immediately but arrange for `vsc` to load `R3_v_sync_width - 1` only at the next
  > line boundary (add a 1-bit `vsync_pending_load` flag consumed in the vsc-decrement block),
  > so the partial line does not consume a count — duration extends by (R0−C0) as documented.
  > If `hcc < 2`, do NOT assert VSYNC; clear `vsync_allow` (blocked-VSYNC: the C4=R7 condition
  > is consumed without a pulse; `vsync_allow` is re-armed by the existing row-change logic or
  > a later R7 write, matching ACCC §16.3's escape conditions).
  > (b) Type 1 (`CRTC_TYPE`): keep current immediate-trigger behavior at any hcc (correct), and
  > confirm `vsc` loads 15 immediately so the partial line consumes count 1 of 16 (duration
  > reduced by C0+1) — this matches the current code path; add a comment citing ACCC §16.4.2.
  > Remove the TODO. Keep the natural (line-aligned) trigger path untouched.
- **Verify**: V3 (duration measured for mid-line trigger, per type; blocked case at C0=0/1);
  V2 (SHAKER VSYNC tests); V1 (Onescreen Colonies, PHX must not regress — they motivated the
  existing `vsync_allow` code).

## F4. Counter overflow defeated by `!line_max` / type 0 `!R4` shortcut terms

- **Rule** (digest-01 §3.1/§3.2/§7.1/§7.2 → ACCC §10.3/§12, p.73-98): C9 and C4 are equality
  comparators, not magnitude: writing R9 (or R4/R5) below the current count makes the counter
  **overflow to 31 (C9) / 127 (C4) and wrap** before matching the new value. Exceptions are the
  type 0 last-line latch (frozen at C0<2) — not a "compare to 0 always matches" rule.
- **Current** (`UM6845R.v:156`): `line_last = (line == line_max) || !line_max;` — when
  `line_max==0` (R9=0, or R5=0/1 in adjustment, or IVM masking), `line_last` is true regardless
  of `line`'s value, so a C9 that should overflow to 31 instead terminates immediately.
  (`UM6845R.v:162`): `row_last = (row == R4_v_total) || (!CRTC_TYPE && !R4_v_total);` — same
  shortcut for C4 on type 0 with R4=0.
- **Impact**: R9/R4-rewrite overflow behavior is the backbone of "rupture" timing analysis
  (ACCC ch.10/12) and of ID test §28.1.1 (VSYNC stops at R7>37 vs R7>39 under overflow).
  However — these terms were probably added to make specific R4=0/R9=0 tricks (RLAL-style
  ruptures, which many demos use) work in a model without the full two-phase latch. **Do not
  remove them blindly.**
- **Confidence: medium.** The rule is certain; whether removal regresses the RLAL approximation
  needs simulation. This fix should wait for the Verilator testbench (V3).
- **Fix prompt**:
  > Prerequisite: Verilator testbench operational. In `rtl/UM6845R.v`:
  > (a) Replace `line_last = (line == line_max) || !line_max` with a pure equality
  > `line_last = (line == line_max)` and re-run the RLAL scenarios from
  > `compendium-01-counters.md` §7.1 (R4=1/R9=0 arming sequence; R9=R4=0 written at C0<2 on the
  > last line) in simulation. The type 0 path already latches `line_last_r`/`row_last_r` at
  > `hcc==0` (line 186-190) which is the correct mechanism for those tricks; the `!line_max`
  > term is likely masking the type 1 "always live" path where R9=0 with C9=0 already true is
  > the only legitimate immediate-match case (equality covers it).
  > (b) Same for `row_last`'s `(!CRTC_TYPE && !R4_v_total)` term — ACCC §12.2 says type 0
  > handles R4=0 through the ordinary last-line latch, not a magnitude special case.
  > (c) Run the §28.1.1 discriminator in sim: R4=36,R9=7,R5=16, sweep R7 — VSYNC must stop
  > firing above R7=37 (type 0) / R7=39 (type 1). If (a)/(b) break RLAL scenarios, the correct
  > repair is in the latch logic, not by restoring the shortcut terms.
- **Verify**: V3 mandatory before merging; then V2 + V1 regression pass (demos: Batman Forever,
  The Demo, Yao demos exercise ruptures heavily).

## F5. CRTC 0 with R0=0: C0 must freeze at 0 (currently free-runs to 255)

- **Rule** (digest-01 §8.1 → ACCC §13.2, p.100-106): type 0 with R0=0 pins C0 at 0; the C0==1
  re-authorization never runs, so C9 (and everything driven from it) freezes; R4/R5/R9 writes
  are ignored while frozen; R8 stays live; HSYNC can only occur if R2==0 (C0 never reaches a
  nonzero R2); VMA reload is suppressed until C4/C9 return to 0.
- **Current** (`UM6845R.v:150`): `hcc_last = (hcc == R0) && (CRTC_TYPE || R0 != 0)` — on type 0
  with R0=0 `hcc_last` is never true, so `hcc` free-runs through 1..255 and wraps. Line/row do
  freeze (correct side effect: `line_new` never fires), but: HSYNC fires whenever the
  free-running `hcc` passes R2 (period 256µs — real chip: no HSYNC at all unless R2=0), and DE
  (`hde` cleared at `hcc_next==R1`) goes low mid-"line" instead of following the doc's
  alternating/frozen display behavior.
- **Impact**: R0=0 stalls are a documented demo technique for horizontal-timing surgery
  (ACCC §13.2.4/§13.8.3, SHAKER tests them); wrong HSYNC cadence during a stall shifts GA
  interrupt timing (R52 counts HSYNC ends) and monitor sync.
- **Confidence: medium-high** for the C0-freeze itself; the full §8.1 three-instant state
  machine (C0==0/1/2 arm/disarm) is a larger rework — this finding covers only the freeze.
- **Fix prompt**:
  > In `rtl/UM6845R.v`, change the type 0 R0=0 handling from "hcc_last never true" to "hcc
  > frozen": in the `hcc <= hcc_next` path, for `!CRTC_TYPE && R0_h_total==0`, hold `hcc` at
  > its current value if it is already 0 (the common case — R0 was set to 0 while C0 was
  > anywhere; C0 continues to its current R0-match... NOTE: per ACCC §13.2, when R0 is set to 0
  > mid-line, C0 first *reaches* the old R0? No — the C0==R0 comparison is live: C0 next
  > matches R0=0 only when it wraps. Model as: `hcc_next = (hcc == R0_h_total) ? 0 : hcc+1`
  > for both types (remove the `CRTC_TYPE || R0_h_total` gate), which pins C0 at 0 once
  > reached, THEN suppress the downstream events that must not fire while frozen on type 0:
  > `line_new` must not fire from the pinned C0 (add `r0_frozen = !CRTC_TYPE && R0==0 && hcc==0`
  > gating line/row/adjustment updates and VMA reload), while HSYNC's `hcc==R2` comparison
  > works naturally (only fires if R2==0 — correct per doc). C9's "one last update against the
  > previous line's R9" subtlety (§13.2.1) and the C4-increments-once case can be added later;
  > get the freeze + HSYNC cadence right first. Type 1 path (hcc wraps every cycle at R0=0,
  > 1µs lines) must remain exactly as-is.
- **Verify**: V3 (R0=0 stall for N cycles → counters resume correctly, no HSYNC unless R2=0);
  V2 (SHAKER R0 tests); V1 regression.

## F6. Type 0 spurious border byte when R1>R0 (and its R8-skew suppression)

- **Rule** (digest-03 §17.6.2/§19.2.4 → ACCC §17.6, p.183): when R1>R0 (C0=R1 never fires),
  type 0 emits **one border byte (0.5µs)** keyed on C0=R0, "BORDER OFF" again on the following
  character; suppressible via R8 SKEW-DISPTMG. Type 1 emits nothing (rows seamlessly merge).
- **Current**: `hde` is cleared only by `hcc_next == R1_h_displayed` (`UM6845R.v:254`) — never
  fires when R1>R0, so both types show continuous display: correct for type 1, missing the
  spurious byte for type 0.
- **Impact**: visual discriminator (ACCC §28.1.6); demos doing "frame merging" rely on the
  presence (type 0) or absence (type 1) of the seam.
- **Confidence: high** for the basic byte; the half-µs phase within the character and the R8
  double-write "disintegration" cases (⚠ VERIFY p.193-194) are refinements.
- **Fix prompt**:
  > In `rtl/UM6845R.v`, for type 0 only: when `R1_h_displayed > R0_h_total` (compare at the
  > moment of use, registers are live) and `hcc == R0_h_total`, force the DE output low for
  > that character (half-character granularity if the pixel pipeline allows — DE is consumed
  > by the GA at 1µs granularity here, so a full-character border byte is the achievable
  > approximation; note this in a comment). Respect the existing skew path: the forced-low
  > must be injected *before* the `de[R8_skew...]` delay-line mux so SKEW-DISPTMG delays it
  > like a natural border edge, and R8 skew mode 2'b11 (non-output) already blanks everything.
  > Type 1: no change (already correct). Cite ACCC §17.6.2 in the comment.
- **Verify**: V3 (DE trace with R1=R0+1: type 0 shows 1-char gap per line, type 1 none); V1.

## F7. RFD ("Rupture For Dummies") — CRTC 1 frame-parity address-reload quirk — NOT implemented

- **Rule** (digest-01 §5 → ACCC §11.6, p.86-89): on type 1, writing R5 from 0 to nonzero exactly
  at C0==R0 (or the R0-widening route, §8.6) arms two flags: VMA loads from R12/R13 on **every**
  row (not just C4=0), and the C9=R9-at-C0=R1 test becomes **frame-parity dependent**, making
  behavior alternate per frame unless pinned via R8 IVM toggling. Used by real demos as a
  CRTC-1 rupture technique.
- **Current**: absent entirely. The R5 write path is a plain register store; no C0==R0
  coincidence detection.
- **Impact**: CRTC-1-specific demo effects (the technique is popular precisely because it's the
  easy rupture on type 1); SHAKER tests it.
- **Confidence: medium.** The digest's rules are detailed but this is the most intricate
  finding; needs the testbench and possibly ⚠ p.89 re-verification.
- **Fix prompt** (for a capable agent, after V3 exists):
  > Implement RFD in `rtl/UM6845R.v` per `compendium-01-counters.md` §5 (read it in full
  > first, plus §8.6 for the second trigger route): detect `R5 0→nonzero` writes landing in
  > the same character cycle as `hcc == R0_h_total` (the write block runs at CLOCK rate;
  > coincidence = write strobe while `CLKEN && hcc_last` this cycle — mind the race, ACCC
  > timing says the write is seen by the rollover logic of that cycle). Arm: (1)
  > `rfd_vma_flag` — extends the existing `CRTC1_reload` term to any row, cleared when the
  > VMA' save (`row_addr_save`) actually fires; (2) `rfd_parity_flag` — gates `row_addr_save`
  > with a frame-parity bit that toggles at frame boundaries where `C4==C9==C0==0 && R9 odd`.
  > Skip the RFD#10 "1-B chip" variant (document as not modeled). This is a self-contained
  > additive feature: when never triggered, behavior must be bit-identical to current (add a
  > testbench regression run to prove it).
- **Verify**: V3 mandatory; V2 (SHAKER RFD tests); V1 (CRTC-1 demos).

## F8. CRTC 1 vertical adjustment must use a separate C5 counter (C4/C9 keep counting)

- **Rule** (digest-01 §4/§4.1/§4.3 → ACCC §11.1-11.2, p.79-83): during adjustment, type 0 reuses
  C9 vs R5 (current model's approach — correct for type 0). **Type 1 has a genuine separate C5**:
  C9 keeps counting 0..R9 (VRAM row-select derives from C9!), C4 keeps incrementing at each
  C9==R9 while C5 counts the adjustment lines against R5. Plus the §4.4 bug: R5 rewritten to 0
  mid-adjustment does NOT end it on type 1 (C5 free-runs until R5 is set to a reachable value).
- **Current** (`UM6845R.v:154`): one shared mechanism — `line_max` switches to `R5-1` for both
  types; C9 counts adjustment lines directly; C4 frozen during adjustment for both. Type 0 ✓,
  type 1 ✗ (no C5, no C4/C9 continuation, R5=0 ends adjustment immediately via the `!line_max`
  term — F4 interaction).
- **Impact**: type 1 ruptures/overscan using adjustment rows show wrong row addressing (RA
  bits come from C9, which real type 1 keeps cycling); the R5=0 escape trick (force C4/C9=0 on
  an arbitrary line) doesn't work.
- **Confidence: medium-high** on the structure; interactions with F4/F7 mean sequencing matters
  (do F4 first).
- **Fix prompt**:
  > In `rtl/UM6845R.v`, add a 5-bit `c5` counter used only when `CRTC_TYPE && in_adj`: C9
  > (`line`) continues its normal 0..R9 cycle (RA output therefore cycles — this is the
  > user-visible change), `row` increments at each `line`==R9 wrap, and `c5` increments once
  > per line, with adjustment ending when `c5 + 1 == R5_v_total_adj` evaluated at the line
  > boundary (equality, not magnitude — R5=0 therefore never ends it: ACCC §11.3.2 bug,
  > reproduce it). On adjustment end, C4=C9=0 unconditionally. Type 0 path unchanged (C9-vs-R5
  > reuse is correct there). Read `compendium-01-counters.md` §4 fully first; mind §4.3's
  > "VMA from R12/R13 while C4==1" special case — implement it only if the existing
  > CRTC1_reload logic doesn't already produce it, and add the §4.1 worked example
  > (R4=10,R5=16,R9=3) as a testbench vector.
- **Verify**: V3 (the §4.1 table); V2.

## F9. Type 0 two-step R9 sampling at C0==R0 (old value in step (a), new in step (b))

- **Rule** (digest-01 §3.1 → ACCC §10.3.1, p.74-75): a write to R9 landing in the same cycle as
  C0==R0 is seen by the C9-reset decision (step b) but NOT by the C4-increment decision (step a),
  which sampled the old R9. Can produce simultaneous C4++ and C9++ (worked examples in digest).
- **Current**: type 0 uses `line_last_r`/`row_last_r` latched at `hcc==0` for both steps —
  a write at C0==R0 is seen by *neither* step (both use the C0=0 snapshot). Half right: step (a)
  correct, step (b) wrong.
- **Impact**: single-cycle JIT R9 writes (demo timing surgery). Narrow but documented with test
  vectors.
- **Confidence: medium.** Cycle-exact write-race modeling; testbench first.
- **Fix prompt**:
  > In `rtl/UM6845R.v`, for type 0 at the `line_new` event: compute the C9-reset decision
  > (`line_next`) from the **live** `line == R9_v_max_line` comparison (seeing any write that
  > landed this cycle) while keeping the C4-increment decision (`row_new`/`row` update) on the
  > latched `line_last_r`. Reproduce digest-01 §3.1's Ex.1 and Ex.2 as testbench vectors
  > (expected: C4=39,C9=8 / C4=38,C9=8 respectively). Beware: the current code uses
  > `line_last_r` for both; split the usage, don't just swap it.
- **Verify**: V3 only (not visually observable except in contrived demos).

## F10. Interlace (IVM) parity machinery — partial/approximate for both types

- **Rule** (digest-03 §19.4-19.8 → ACCC ch.19): type 0 keeps split C9/C9.VMA with asymmetric
  IVM entry/exit comparisons, ParityFrame/ParityR6/ParityC9 state, VSYNC-delay-by-1-line
  correction for odd R9; type 1 has 2 parity states, R8-write effects at the 3rd vs 4th µs of
  the OUT, R9-parity-triggered alternation, and NO VSYNC drift correction. Additional interlace
  line appended per type-specific parity conditions. R9 programming formula differs (N-2 type 0
  IVM vs N-1 type 1).
- **Current** (`UM6845R.v:51,54,145,157,201`): minimal IVM: `line` steps by 2 with bit 0 masked,
  RA ORs in `field`, `field` toggles at frame end, MID-VSYNC at R0/2 on `field`. No parity
  state machines, no per-type differences, no additional interlace line, no entry/exit
  asymmetry.
- **Impact**: interlace demos (SHAKER 2.x uses 1/64-line positioning tricks); most games unaffected.
- **Confidence: medium** (digest pseudocode is complete but ⚠ p.203-209 tables unverified).
- **Fix prompt**: deliberately NOT written yet — this is a multi-week finding. Treat
  digest-03 §19.5-19.8 pseudocode as the spec, build V3 fixtures from the SHAKER 22C/3 tables
  (⚠ re-extract p.207-209 from the PDF first), and implement type-by-type. Do this LAST.

## F11. Minor / confirmatory findings (no immediate action)

- **F11a — HSYNC width semantics** (`UM6845R.v:235-236`): equality-based `hsc == R3l` end +
  4-bit wrap naturally reproduces the "overflow on shrink" rule (digest-02 §4) ✓; type 1
  R3l=0-cancels-immediately ✓ explicitly coded. Type 0 mid-HSYNC write of R3l=0 wraps (correct).
  Only gap: CRTC 0's "restart without C3l reset if R3l modified at the exact end position"
  (§10) — exotic; leave.
- **F11b — VSYNC re-entrancy** (`vsync_allow`, `UM6845R.v:283-296`): reproduces mechanism 2
  including the R7=0/R4=0 lock and the R7=0,R4=1,R9=7 infinite-VSYNC bypass (digest-02 §18) ✓.
  Keep; add V3 vectors for both cases to protect it.
- **F11c — R12/R13 readback** (`UM6845R.v:86-87`): type 0 returns stored, type 1 returns 0 ✓
  (primary detection vector, correct). Protect with V3 assertion.
- **F11d — Dummy register 31** (`UM6845R.v:90`): 0xFF on type 1, 0x00 on type 0 ✓.
- **F11e — VSYNC width R3h** (`UM6845R.v:294`): type 0 uses R3h (0→16 via 4-bit wrap ✓),
  type 1 fixed 16 ✓.
- **F11f — R16/R17 light pen**: not implemented (reads 0). Real chips return a latched address;
  with no pen attached the value is effectively arbitrary. Low value; note only.
- **F11g — DE skew** (`UM6845R.v:56`): R8 bits 5:4, type 0 only, 0/1/2-char delay + non-output ✓
  (matches ACCC §19.1 table; note the digest prose "(bits 4:3)" is a typo — the table and the
  UM6845 datasheet say bits 5:4, which is what the code uses).
- **F11h — R12/R13 mid-row immediacy on type 1** (digest-03 §20.3.2, ⚠ VERIFY p.238): digest
  suggests type 1 applies an R12/R13 write to VMA possibly mid-line within C4=0, current model
  applies at next line start. Re-read PDF p.238 before deciding; if real, fold into F7's work.
- **F11i — Interrupt/R52, GA-side rules** (digest-02 §23-27): live in `rtl/GA40010` (netlist,
  gate-accurate) — out of CRTC scope, nothing to do. The CRTC's job is correct HSYNC *edges*,
  which F3/F5/F6 improve.

---

## Suggested execution order

| Order | Finding | Size | Prereq |
|---|---|---|---|
| 1 | F1 register readback | XS | none — calibration fix |
| 2 | F2 status bit 5 latch | S | none |
| 3 | Verilator testbench (separate spec) | M | none — unblocks the rest |
| 4 | F3 VSYNC mid-line/blocked | M | testbench strongly advised |
| 5 | F6 spurious border byte | S-M | testbench |
| 6 | F5 R0=0 freeze | M | testbench |
| 7 | F4 overflow shortcut removal | M (risky) | testbench mandatory |
| 8 | F8 type 1 C5 counter | M | F4 |
| 9 | F9 two-step R9 sample | S | testbench |
| 10 | F7 RFD | L | F4, F8 |
| 11 | F10 interlace overhaul | XL | all above |
