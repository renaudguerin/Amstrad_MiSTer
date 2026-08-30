# Classic CRTC Accuracy Audit — Findings & Fix Prompts

Audited: the classic CRTC core — since 2026-08-22 the wrapper `rtl/CRTC.v` plus per-type
engines `rtl/crtc_type0_engine.v` / `rtl/crtc_type1_engine.v`, types 0 & 1 via the
`CRTC_TYPE` input (`CRTC_TYPE=0` → type 0 HD6845S/UM6845, `CRTC_TYPE=1` → type 1 UM6845R).
The wrapper was `rtl/UM6845R.v` before the split; historical fix prompts below keep that
original path.
Reference: *The Amstrad CPC CRTC Compendium* v1.11 (Longshot / Logon System), via the digests in this
directory (`compendium-01-counters.md`, `compendium-02-sync.md`, `compendium-03-display-regs.md`).
Cite chains are `digest §section → ACCC §chapter, p.N`.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).

Original audit: 2026-07-07, Claude (Fable 5). ACCC v1.10 documentation rebaseline:
2026-08-13. 2026-08-22 v1.10 faithfulness review accepted
(`findings-review.md`); its code implications are noted inline below (F7/F8/F9/F10).

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
2. Never regress the other CRTC type: type-0 rules live in `rtl/crtc_type0_engine.v`, type-1
   rules in `rtl/crtc_type1_engine.v`, and shared counters/register-file sequencing in the
   `rtl/CRTC.v` wrapper (per-type separation landed 2026-08-22; line references below were
   refreshed for that layout and again for the wrapper rename). A change must land in its
   type's engine unless the rule is explicitly universal.
3. `CRTC.v` uses `CLKEN` as the 1MHz character-clock enable; "per C0 cycle" rules belong inside
   `if (CLKEN)` blocks. Register writes from the Z80 arrive via the `ENABLE & ~nCS & ~R_nW` path
   at full `CLOCK` rate — write-vs-counter races are races between that block and `CLKEN` blocks.
4. Existing register/counter names: `hcc`=C0, `line`=C9, `row`=C4, `hsc`=C3l, `vsc`=C3h,
   `in_adj`=adjustment active, `row_addr_r`=VMA, `row_addr`=VMA'.

---

## F1. R10/R11 must not be readable on types 0 and 1  ⭐ smallest, highest-confidence fix

- **Rule** (digest-03 §21.2 → ACCC §21.2, p.245-246): readable registers are R12-R17 on type 0,
  R14-R17 (+dummy 31) on type 1. **R10 and R11 are not readable on either type** (only on
  types 3/4). Unrecognized register numbers read 0.
- **Current** (fixed): the readback mux (`CRTC.v:111-125`) has no R10/R11 arms, so both
  types return the `default: DO = 0` value; R12/R13 still read back on type 0 only, dummy 31
  differs per type as documented. Protected by `t01_register_readback`.
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

- **Rule** (digest-03 §21.3.3 → ACCC §21.3, p.247): type 1's `&BE00` status bit 5 reflects the
  **R6-border condition** (`C4=R6` reached), **sampled only at C0=R0**, not continuously. The
  "R6=0 forced border while C4>0" state is NOT reflected in bit 5. Bits 0-4 and 7 read 0.
- **Current** (fixed): `status_bit5` lives in the type-1 engine (`crtc_type1_engine.v:216-247`),
  is sampled at `CLKEN && hcc_last` from a dedicated R6-border condition latch, and the DO mux
  returns `{2'b00, status_bit5, 5'b00000}` (`CRTC.v:127`). Protected by `t06a`-`t06d`.
- **Impact**: software polling `&BE00` for raster position (documented technique, ACCC §24.5
  OUTI/status tricks) sees transitions up to a line early/late; detection routines checking the
  latch behavior (§28.1.8) diverge.
- **Confidence: medium-high.** The sampled-at-C0=R0 rule is clean prose; the exact
  ⚠ VERIFY p.247 diagrams should be consulted if the testbench disagrees (p.248 is §21.3.4
  CRTC 3/4 STATUS 1 — render-verified 2026-08-24).
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

- **Rule** (digest-02 §19 → ACCC §16.4.1, p.168-169): on type 0, writing R7=C4 mid-line triggers
  VSYNC **immediately** — *except* when the write lands at C0vs ∈ {0,1}, which produces a
  **BLOCKED VSYNC**: no pulse, and no VSYNC can fire for this C4=R7 value until the comparison's
  truth value changes. When it does fire mid-line (C0vs>1), the line counter C3h starts counting
  **from the next line start**, so total duration is *extended* by `R0−C0vs` µs.
  On type 1 (§16.4.2): the write triggers immediately at **any** C0vs, and the partial line
  *counts* as line 1, so duration is *reduced* by `C0+1` µs. (PPI-visible latency: 6µs type 0
  vs 5µs type 1 — a documented detection vector.)
- **Current** (fixed): the R7-write handler (`CRTC.v:436-451`) arms `VSYNC_r` via per-type
  fire terms (`crtc_type0_engine.v:320` gates on `hcc > 1`; type 1 fires at any C0), loads
  `vsc` from the type's width term, and the type-0 engine owns the partial-first-line holdoff
  latch that blocks the C0<2 pulse. Protected by `t02a`-`t02k`.
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

## F12. CRTC 0 last-line / vertical-adjustment arbitration — deterministic counter path implemented

- **Rule** (digest-01 §3.1/§4.2/§7.1 → ACCC §10.3.1, §11.2.2, §12.2,
  p.75-76/81-83/92-94): `Last Line` is provisional until additional-line handling is
  arbitrated. Adjustment cancels it; completing the R5 count re-establishes it so C4/C9 reset
  on the following line. Entry includes `R5>0` before C0 reaches 3, `R5==0` when a C0==1 write
  breaks equality that was true at C0==0, and the `R0<2` route. On the entry line, R9 writes
  at C0==2..R0-1 use the new R9; R4 writes at C0==2..R0 switch the C9 comparator from R9 to
  R5; an R9 write exactly at C0==R0 can increment both C4 and C9 under that switch.
- **Current**: type 0 captures effective R4/R9 values for same-edge C0=0 comparison,
  activates the R5=0 C0=1 equality-break route, applies a same-cycle R5 write at C0=2,
  retains the selected comparator across the documented C0>=2 windows, and separates the
  exact-R0 C4 and C9 comparisons. The short-R0 path consumes the C4 increment once for R0=0
  and runs the default zero-adjustment line for R0=0/1. `t16a`-`t16s` protect those counter
  results, active-adjustment freeze, exact R0=1 latch consumption, completion, bus phases,
  and retained-state lifecycle; t09h protects the correlated R0=0 freeze entry.
- **Impact**: the documented deterministic C4/C9/RA and adjustment-state paths are now
  guarded. Remaining risk is pin-level sub-character timing for ruptures and short-R0 entry,
  plus interactions exposed when F4 removes the older zero-comparator shortcuts.
- **Confidence: high for the implemented counter results; medium for untraced pin timing.**
  The C4/C9 results are explicit in v1.10 and protected at both bus phases. Exact MA/DE and
  VSYNC behavior for the C0==1 path still benefits from hardware/SHAKER traces.
- **Fix prompt**:
  > Preserve `t09h` and `t16a`-`t16s` as required passes while implementing F4. Keep type 1
  > unchanged and do not infer sub-character MA/DE/VSYNC timing from internal counter state.
- **Verify**: V3 `t09h` and `t16a`-`t16s` required passes; V2/hardware evidence remains for
  sub-character output timing and the complete CRTC-0 rupture matrix.

## F4. Counter overflow defeated by `!line_max` / type 0 `!R4` shortcut terms

- **Rule** (digest-01 §3.1/§3.2/§7.1/§7.2 → ACCC §10.3/§12, p.74-79/92-94): C9 and C4 use
  equality, not magnitude: writing R9 below C9 makes C9 count to 31 and wrap; outside vertical
  adjustment, lowering R4 below C4 makes C4 count to 127 and wrap. Zero is not an unconditional
  match. Type 0's exceptions come from the explicit `Last Line` / adjustment arbitration in
  F12, including completion re-arming—not from a comparator shortcut.
- **Current** (fixed, `de71808`): both comparators are pure equality —
  `line_last = (line == crtc0_line_max)` (`crtc_type0_engine.v:156`, type-1 twin at
  `crtc_type1_engine.v:110`) and `row_last = (row == R4_v_total)` (`crtc_type0_engine.v:197`,
  type-1 twin at `:137`). Zero-limit overflow behavior is protected by `t07a`-`t07l`.
- **Impact**: R9/R4-rewrite overflow behavior is the backbone of "rupture" timing analysis
  (ACCC ch.10/12) and of ID test §28.1.1. Q17 remains an internal source conflict: the
  detailed adjustment rules and current sim predict type-1 R7=39 silence (maximum C4=38),
  while §28.1.1 explicitly says the type-1/2 overrun repeats and VSYNC persists through R7=39.
  Hardware must adjudicate before either boundary is promoted to an oracle.
  However — these terms were probably added to make specific R4=0/R9=0 tricks (RLAL-style
  ruptures, which many demos use) work in a model without the full two-phase latch. **Do not
  remove them blindly.**
- **Confidence: medium.** The overflow rule is certain, but removing the shortcuts before F12's
  state vectors exist could trade one approximation for another. This fix must wait for V3.
- **Fix prompt**:
  > Prerequisite: Verilator testbench operational. In `rtl/UM6845R.v`:
  > (a) Replace `line_last = (line == line_max) || !line_max` with a pure equality
  > `line_last = (line == line_max)` and re-run the RLAL scenarios from
  > `compendium-01-counters.md` §7.1 (the precise one-limit-zero/other-positive C0<2 arm;
  > R4=1/R9=0 delayed arming; and R9=R4=0 on a genuine last line) in simulation. Do not treat
  > the current `hcc==0` snapshots as sufficient: F12 documents the missing C0==1, C0==2,
  > R0<2, and completion-rearm behavior.
  > (b) Same for `row_last`'s `(!CRTC_TYPE && !R4_v_total)` term. ACCC §12.2 handles R4=0
  > through equality plus `Last Line` / adjustment state, not a magnitude special case.
  > (c) Run the §28.1.1 discriminator in sim and hardware: R4=36,R9=7,R5=16, sweep R7=38/39.
  > Current sim makes R7=39 silent, but §28.1.1 predicts a pulse; preserve that conflict until
  > hardware decides it. If (a)/(b) break RLAL scenarios, the correct
  > repair is in the latch logic, not by restoring the shortcut terms.
- **Verify**: V3 t07/t08 plus F12's planned t16 mandatory before merging; then V2 + V1
  regression pass (demos: Batman Forever, The Demo, Yao demos exercise ruptures heavily).

## F5. CRTC 0 R0=0 freeze and deferred entry are deterministic-complete

- **Rule** (digest-01 §8.1 → ACCC §13.2, p.103-109): type 0 with R0=0 pins C0 at 0; the C0==1
  re-authorization never runs, so C9 (and everything driven from it) freezes; R4/R5/R9 writes
  are ignored while frozen; R8 stays live; HSYNC can only occur if R2==0 (C0 never reaches a
  nonzero R2); VMA reload is suppressed until C4/C9 return to 0. v1.10 also frames `R0<2` as a
  default vertical-adjustment route because C0 cannot reach the C0==2 cancellation point.
- **Current**: `hcc_next` pins C0 at 0 and `r0_frozen` suppresses `line_new`; t09a-t09g cover
  the principal freeze, HSYNC, resume, type-1, and interlace regressions. Required-pass t09h
  now consumes the previously armed C4 increment exactly once when C9==R9 at entry. t16p/q
  correlate R0=1 and initial R0=0 with the default-adjustment route and recovery.
- **Impact**: the deterministic freeze and entry counter states are protected. Remaining risk
  is hardware-visible sync/interrupt timing during exotic live ruptures, not a named simulator
  divergence.
- **Confidence: high for deterministic counter and pin assertions; medium for the broader
  hardware matrix.** The combined t09/t16 traces establish deferred increment, adjustment
  activation, freeze, and recovery as one state sequence.
- **Fix prompt**:
  > Keep t09a-t09h and t16p/q green. Do not loosen `r0_frozen` or change type 1's
  > one-character lines while removing F4 shortcuts.
- **Verify**: V3 t09a-t09h and t16p-s required passes; V2/hardware SHAKER R0 tests remain.

## F6. Type 0 spurious border byte when R1>R0 (and its R8-skew suppression)

- **Rule** (digest-03 §17.6.2/§19.2.4 → ACCC §17.6, p.186): when R1>R0 (C0=R1 never fires),
  type 0 emits **one border byte (0.5µs)** keyed on C0=R0, "BORDER OFF" again on the following
  character; suppressible via R8 SKEW-DISPTMG. Type 1 emits nothing (rows seamlessly merge).
- **Current** (Stage 1 implemented, `accuracy/a3-f6-stage1` 2026-08-23):
  `crtc_type0_engine.v` drives a combinational substituted border-start term
  (`!CRTC_TYPE && R1>R0 && hcc==R0`, ACCC §17.6.2 p.186 / §19.2.4 p.195) that the wrapper
  injects ahead of the SKEW-DISPTMG delay line, so the byte lands at C0=R0 with skew 0,
  displaces to C0=0/C0=1 with skew 1/2 (§19.2.3), and is suppressed by non-output skew
  2'b11. Type 1 has no such term (rows merge; §17.6.2 p.186-187). Protected by
  t10a-t10e. Residual: the R0=0 alternating-byte extreme (p.186) is not modeled — the
  frozen C0 holds DISPTMG off continuously; needs a toggle mechanism in a later stage.
  Stage 2 measured 16 mode-2 px (1 µs) through the GA co-simulation route. Stage 2b's
  visual reading of pp.186/195 established that the book requires a CRTC-side 0.5 µs
  pulse, not GA halving of a full-character pulse; formal extension F13 below owns the
  blocked duration/phase correction. The §19.2.5 double-R8-write disintegration cases
  remain out of scope.
- **Impact**: visual discriminator (ACCC §28.1.6); demos doing "frame merging" rely on the
  presence (type 0) or absence (type 1) of the seam.
- **Confidence: high** for the basic byte; the half-µs phase within the character and the R8
  double-write "disintegration" cases (⚠ VERIFY p.196-197) are refinements.
- **SUPERSEDED twice (2026-08-22 / Stage 2b 2026-08-23):** the fix prompt's claim
  that "DE is consumed by the GA at 1µs granularity here, so a full-character border byte is
  achievable approximation" was wrong. The first correction then wrongly assumed all real
  DISPTMG edges were character-aligned and assigned the half-byte to the GA pipeline.
  Stage 2b is authoritative: visual ACCC pp.186/195 specify a sub-character CRTC signal;
  test/production CRTC clock phase matches and both GA paths agree. **Do not implement the
  prompt as written** — follow F13 and the staged option C plan in
  [f6-decision-gate.md](f6-decision-gate.md). The prompt below is retained verbatim as
  history, including its pre-split path name (`rtl/UM6845R.v`, now `rtl/CRTC.v`).
- **Fix prompt** (superseded — see banner above; current plan: `f6-decision-gate.md`):
  > In `rtl/UM6845R.v`, for type 0 only: when `R1_h_displayed > R0_h_total` (compare at the
  > moment of use, registers are live) and `hcc == R0_h_total`, force the DE output low for
  > that character (half-character granularity if the pixel pipeline allows — DE is consumed
  > by the GA at 1µs granularity here, so a full-character border byte is the achievable
  > approximation; note this in a comment). Respect the existing skew path: the forced-low
  > must be injected *before* the `de[R8_skew...]` delay-line mux so SKEW-DISPTMG delays it
  > like a natural border edge, and R8 skew mode 2'b11 (non-output) already blanks everything.
  > Type 1: no change (already correct). Cite ACCC §17.6.2 in the comment.
- **Verify**: V3 (DE trace with R1=R0+1: type 0 shows 1-char gap per line, type 1 none); V1.

## F13. Type-0 R1>R0 blip width — pin vs GA phase ownership

- **Rule** (ACCC §17.6.2 p.186, §19.2.4 p.195; visual tier 2026-08-23): for type 0 with
  R1>R0, C0=R0 contains one DISP-ON byte followed by one BORDER byte; the BORDER signal is
  sent 0.5 µs after C0=R0 and disabled at the following character boundary, 0.5 µs later.
  R1=R0 is the p.185 control (full 1 µs border character). Type 1 emits no seam (p.187).
- **Current** (`accuracy/f13-dsc4-fdc-investigation`, 2026-08-30): the wrapper now gates the
  substituted type-0 event with the opposite CRTC phase. With no skew, DE is high for the
  first half of C0=R0, low from nCLKEN to the following CLKEN, then high at C0=0. The p.195
  SKEW-DISPTMG 1/2 diagrams remain full-character delayed events at C0=0/C0=1; type 1 emits
  none. `t31a` pins all three no-skew edges and `t10a`-`t10e` retain type/skew controls.
  Candidate-owner elimination preceding the change remains relevant:
  test-top phase mismatch is false (`ga40010_test.v` and `Amstrad_motherboard.v` both use
  CCLK_EN_N/S=03; adding production's `nCLKEN` connection does not move the result);
  original async and synchronous GA paths transition identically; ACCC nuance is ruled out
  by the p.186 chronogram plus p.195 prose. Remaining owner: CRTC-side sub-character DE
  phase.
- **Impact**: the §28.1.6 presence/absence discriminator and the ACCC-model width/phase are
  implemented. SHAKER Module A (O) and a DE-pin capture remain the hardware validation gate;
  simulation is not hardware evidence.
- **Confidence: high for document/mechanism ownership; hardware confirmation pending.**
  Multimodal source reading plus agreement between the original async and synchronous
  GA buffer/sequencer realizations (which feed the same `video` module) excludes a
  path-specific discrepancy for the tested full-character pulse. It does not substitute
  for the proposed half-character hardware stimulus; real hardware remains authoritative.
- **Status: IMPLEMENTED-PENDING-HARDWARE-VALIDATION.** The 2026-08-30 accuracy task
  explicitly authorized proceeding from the render-verified ACCC model. Capture SHAKER
  Module A (O) on a real type-0 CPC and, if possible, the CRTC DE pin: expected book result
  is an 8-mode-2-px seam with DE low only for the second byte of C0=R0. Any disagreement
  reopens F13; it does not get explained away by the model.

## F14. Additional interlace line — IMPLEMENTED on both types (2026-08-26)

- **Rule** (ACCC §19.5.1 p.205, §19.6.1/§19.6.2 p.216, §19.3 p.199, §11.2.4 p.84; renders
  2026-08-25, see accc-author-questions.md item 10): with R8∈{1,3}, one extra line is
  appended after the frame's R5 lines when the **even** (ParityFrame-even) frame completes.
  Type 1 gates on **ParityFrame even**; type 0 gates on **ParityR6 odd** (ParityR6 becomes
  odd when C4 reaches R6 on an even frame; the R6>R4 freeze persists the gate state — line
  every frame if frozen odd, never if frozen even). Counter accounting: type 0 increments C4
  once for all additional lines (R5 and interlace), C4=R4+1; type 1 increments C4 once more
  on even frames when R9+1 is a multiple of R5. Duration-wise the line lands in the following
  odd frame's count (even frame 19968µs/312 lines, odd frame 20032µs/313 lines, §19.3 p.199).
- **Current** (branch `accuracy/f14-f15-interlace`, commit `5bec99a`, 2026-08-26):
  implemented on both types behind the gates above, with the frame origin (C4/C9 reset,
  ParityFrame snapshot, VMA reload) moved to the additional line's end. Type 0: the line
  holds C4=R4+1 and continues the adjustment count at C9=R5 ("as if added to R5",
  section 11.2 p.84); the R6>R4 freeze persists the gate (frozen odd → line every frame,
  frozen even → never). Type 1: the adjustment end is deferred one line (the extra line
  holds C9=0 at C4 one past the last adjustment row, section 11.2.4 p.84); with R5=0 the
  R9+1-multiple condition is vacuous, so adjustment-less frames never gain the line —
  which is what keeps the t21-t24 IVM walks (all R5=0) undisturbed. Vectors: `t27a`-`t27d`
  (type 0: basic, after-R5 position, both freeze persistences), `t28a` (type 1 basic,
  R9=7/R5=4), `t28b` (condition control, required pass). Bite-tested (gate disable, parity
  term drop, condition drop).
- **Impact**: IVM/interlace-sync frames are one line short on even frames; total frame
  cadence and any interlace-aware demo effect that depends on the 625-line structure
  diverges. Affects both classic types.
- **Confidence: high** on the documented gates (three independent sections agree); the
  within-frame counter mechanics (where exactly the extra line sits relative to the R5 count
  and the frame-origin reset) are sourced but unfixed against hardware.
- **Residual** (recorded in `f10-implementation-notes.md`): the section 11.2.3 p.84 worked
  example's R5=7 sub-case shows the additional line where the section 19.6.2 type-1
  condition (R9+1 multiple of R5) produces none; it is read as the CRTC 2 accounting
  (section 11.2.5 — the p.217 bug example is likewise section 19.6.3). The example's R5=8
  sub-case matches the implemented type-1 behavior exactly.

## F15. Type-0 odd-R9 IVM counting — IMPLEMENTED (2026-08-26)

- **Rule** (ACCC §19.8.1 p.219-220, §19.5.2 p.205-206; renders 2026-08-25, see
  accc-author-questions.md item 19): with IVM active and **R9 odd**, the line parity
  alternates per character: the p.219 row-end update `ParityC9 = C4.0 xor ParityFrame` fires
  when R9 is odd (the printed token `If R9.0=0` is a typo for `R9.0=1` — adjudicated 2026-08-25
  against the gloss, §19.5.2, the p.206 R9=7 example, and the R9=6 tables). The limit tests
  keep the three-phase form the engine already implements: switch line raw C9 vs "R9 or
  ParityFrame", steady lines C9x2+ParityFrame vs "R9 or ParityC9", exit line C9.VMA vs plain
  R9. §19.5.2's VSYNC delay-by-1-line correction for odd-C4 R7 (p.206-207) is part of the
  same odd-R9 balancing scheme.
- **Current** (branch `accuracy/f14-f15-interlace`, commit `1c1d084`, 2026-08-26):
  implemented. The limit target is R9 + (ParityC9 xor R9.0) — rows end at the first
  C9.VMA at or past R9, reproducing the rendered p.206 R9=7 worked example line for line
  on both frame parities (`t29a`/`t29b`); the p.219 row-end update ParityC9 :=
  C4.0(new) xor ParityFrame fires at every IVM row end and the origin re-anchors it to
  the frame parity; the switch line tests raw C9 against R9 + ParityFrame (the p.219
  overflow sentence pins the addition form); and the section 19.5.2 VSYNC
  delay-by-1-line correction fires on ParityFrame-odd frames when R7 is odd (`t29c`,
  the pulse at the second line of C4=R7, C9.VMA=2). Even-R9 behavior is bit-identical to
  the previous model (the addend reduces to the old R9-or-parity form), so the t22
  family and every even-R9 vector are unchanged. Bite-tested (parity update off, target
  form reverted, delay arm off — each fails exactly the t29 family / t29c).
- **Impact**: type-0 IVM with odd R9 (the p.206 balancing scheme) counts wrong by
  construction; any software using odd-R9 interlace on a type-0 CRTC diverges. Type 1 is
  unaffected (its §19.8.2 scheme is a different, already-implemented structure).
- **Confidence: high** on the gate polarity (four mutually independent sources agree);
  medium on the full odd-R9 line sequencing (the p.206 example's within-character 5+4 split
  is not fully derivable from the pseudocode). Q19(b)'s adjacent post-exit behavior is now
  resolved separately as F16 and does not block the odd-R9 fixtures.
- **Residual**: Q19(b)'s post-exit behavior after a non-matching R8=0 write is resolved visually
  as finding F16 (see below). The within-line VSYNC phase on type-0 IVM frames still follows the
  legacy field-keyed mechanics; F15 moves the start line only.

## F16. Type-0 post-IVM exit keeps the frozen C9.VMA comparison

- **Rule** (ACCC v1.10 §19.8.1 pp.219-220 and tables pp.223-224; author feedback §2.8): On Type 0
  (HD6845S/UM6845), when leaving IVM ($R_8 \to 0$), the line-end comparison does not revert to
  evaluating live $C_9 == R_9$. Instead, the comparator continues testing the **frozen $C_9.\text{VMA}$
  register content** (the last computed IVM raster address from the exit line) against plain $R_9$ until
  a match occurs (or IVM is re-entered). If $R_9 \ne \text{frozen } C_9.\text{VMA}$, $C_9$ continues counting
  past $R_9$ without ending the row, wrapping at 31. Software can recover normal counting by reprogramming
  $R_9 = \text{frozen } C_9.\text{VMA}$ (the p.220 recovery recipe).
- **Current** (implemented 2026-08-26): `rtl/crtc_type0_engine.v` tracks `ivm_exit_frozen` and latches
  `exit_frozen_vma` at the IVM exit line. `type0_limit_value` and `type0_seam_value` compare `exit_frozen_vma`
  against plain $R_9$ while frozen, clearing on comparator match or IVM re-entry. Verified by extended
  `t22l`-`t22s` walking through $C_9=7$ without premature reset at $C_9=6$ on non-matching exits, and `t30a`/`t30b`
  verifying the p.220 mid-line recovery recipe on odd and even frames.
- **Confidence: high.** Derived directly from ACCC v1.10 pp.219-224 exit tables and author confirmation.

## F17. Type-1 RFD triggered on C9=R9 disables VMA-source state

- **Rule** (ACCC v1.10/v1.11 §11.6.1 p.88 Case 2; author question Q4): On Type 1 (UM6845R), triggering an RFD
  (via the general $R_5$ written 0 $\to$ nonzero route) on the last character line of a row where $C_9==R_9$ disables
  the state allowing VMA to be updated with $R_{12}/R_{13}$ (`rfd_vma_flag = false`), while the parity flag
  arms normally (`rfd_parity_flag = true`). Subsequent character lines continue sequential VMA counting
  without reloading $R_{12}/R_{13}$. (In contrast, the §13.7.1.2 p.124 $R_0$-widening $R_4$-variant route
  explicitly specifies "R12/R13 considered" when $C_9==R_9$ still holds, arming both flags; review N1).
- **Current** (implemented 2026-08-26): `rtl/crtc_type1_engine.v` disarms `rfd_vma_flag` and disables
  `rfd_vma_active` when `rfd_arm` occurs with `line == crtc1_line_max`, while keeping `rfd_parity_flag` armed.
  Effective `crtc1_rollover_r5` is also wired into `crtc1_adj_entry_from_row0`. Verified by `t13d` (source flag
  disabled on final line) and `t13n` (VMA sequential progression without $R_{12}/R_{13}$ reload).
- **Confidence: high.** Derived directly from ACCC v1.10/v1.11 §11.6.1 p.88.

## F18. Type-1 readable register set validation and pinning

- **Rule** (ACCC v1.10 §21.2.2 p.245 vs §28.1.9 p.293): On Type 1 (UM6845R), registers $R_{14}/R_{15}$
  (cursor address) and $R_{16}/R_{17}$ (light pen) are readable; undefined dummy register 31 reads 0xFF.
  $R_{12}/R_{13}$ (start address) and all other registers return 0x00. On Type 0, $R_{12}–R_{17}$ are readable
  and register 31 reads 0x00.
- **Current** (validated and pinned 2026-08-26): `rtl/CRTC.v` readback mux enforces §21.2.2 and §28.1.9,
  verified and pinned for all 32 register addresses on both CRTC types in `t01` (`sim/sim_main.cpp`).
- **Confidence: high.** Verified against ACCC v1.10 §21.2.2 and §28.1.9.

## F19. CRTC 2 $C_0=0$ Last Line Evaluation Timing ($R_4$ vs $R_9$) — OUT-OF-SCOPE / CRTC-2 SPECIFIC

- **Rule** (ACCC v1.11 §12.4.1 p.95): On CRTC 2 (MC6845), at the beginning of a line ($C_0=0$), the `Last Line`
  comparison uses the **updated** value of $R_4$, but the **previous** value of $R_9$ (an update of $R_9$ on
  $C_0=0$ occurs too late for this evaluation).
- **Adjudication & Status** (independent review 2026-08-28): This rule is located under **§12.4 CRTC 2**
  (p.95) and applies strictly to CRTC 2's internal Last Line Management state machine. In contrast, **CRTC 0**
  is governed by **§12.2** (pp.92–94), which explicitly specifies that modifying $R_4$ or $R_9$ on $C_0<2$
  evaluates the updated values to validate or clear the Last Line state.
- **Current Core State**: `rtl/crtc_type0_engine.v` evaluates both $R_4$ and $R_9$ same-edge writes on $C_0=0$
  (`type0_c0_r4` and `type0_c0_r9`) per §12.2. Verified by unit tests `t12c` ($R_9$ write clears Last Line),
  `t12d` ($R_9$ write validates Last Line), and `t12e` ($R_4$ write clears Last Line). Golden soak hash remains
  `0x48146d2b681268ab`.
- **Confidence: high.** Verified against ACCC v1.11 §12.2 pp.92-94 vs §12.4.1 p.95.

## F20. CRTC-1 R2.JIT sub-character HSYNC start — IMPLEMENTED-PENDING-HARDWARE-VALIDATION

- **Rule** (ACCC v1.11 §9.3.4.1 pp.53-54, §9.3.4.3 p.57, §14.6.1 p.141):
  in the static Mode-2 case the CRTC-1 blank starts one pixel later than
  CRTC-0. An `OUT (C),r8` write that makes R2 equal to the current C0 exactly
  at the comparator position delays the start by four Mode-2 pixels on type 0
  and three on type 1. Display reactivation does not move: the physical raw
  pulse therefore shortens by the same four/three pixels even though the
  character-count duration remains governed by R3.
- **Current** (`accuracy/f13-dsc4-fdc-investigation`, 2026-08-30): the wrapper
  tracks the master-clock phase within a character. CRTC-1's ordinary
  comparator start is deferred by four 64 MHz clocks, while the first edge of
  an R2 write that creates the live equality starts at the actual write phase.
  The normal type-1 path replays its ordinary one-pixel phase at the trailing
  edge; JIT retains only that type-specific trailing phase, not the later write
  phase. The integrated CRTC+GA regressions drive production-phased Z80 I/O
  writes and pin type 0 at +4/-4 pixels and type 1 at +3/-3 pixels (start/pulse
  width), plus a same-value normal-path intent control. Production bus phasing
  lands that write after HSYNC has risen, so it documents the invariant rather
  than making the defensive same-value guard load-bearing. The classic CRTC suite
  retains the R3=0, live-R3, reset, and type
  round-trip controls; all new phase/deferred-edge latches join the soak
  projection.
- **Residuals**: DSC4 on real CRTC-1 hardware remains the title-level gate.
  The current bus interface cannot distinguish `OUTI` from an otherwise
  identical write, and R2 updates during an already-active pulse need a
  separate vector before broader §14/§15 closure. A live CRTC-type switch
  during an already phase-shifted pulse keeps that pulse's origin timing; no
  hardware rule currently justifies a mid-pulse reinterpretation. RFD×IVM remains an
  independent compound DSC4 discriminator.
- **Confidence: high for the ACCC model and integrated timing fixture;
  hardware confirmation pending.** Simulation is not evidence that DSC4 now
  passes on MiSTer.

## F7. RFD ("Rupture For Dummies") — CRTC 1 frame-parity address-reload quirk — R5 and R0-widening triggers implemented

- **Rule** (digest-01 §5 → ACCC §11.6, p.87-90): on type 1, writing R5 from 0 to nonzero exactly
  at C0==R0 (or the R0-widening route, §8.6) arms two flags: VMA loads from R12/R13 on **every**
  row (not just C4=0), and the C9=R9-at-C0=R1 test becomes **frame-parity dependent**, making
  behavior alternate per frame unless pinned via R8 IVM toggling. Used by real demos as a
  CRTC-1 rupture technique.
- **Current** (implemented 2026-08-23 on `accuracy/f7-rfd`): the type-1 engine detects the
  R5 0→nonzero bus write on the same `CLKEN && hcc_last` edge, feeds the newly armed state
  into that edge's reload decision, and maintains independent VMA-source, parity-management,
  and odd-R9 frame-parity state. A parity-qualified VMA' save clears the source flag; R1>R0
  uses the p.87 bare-C9 disarm. Required vectors `t13a`-`t13d` pin the never-triggered path,
  same-cycle reload and adjustment entry, parity alternation, normal disarm, and B6 route. RFD#10's optional
  "1-B" variant is deliberately not modeled.
- **Current** (§13.7.1.2 R0-widening route implemented 2026-08-23): a type-1 R0 write that
  strictly widens R0 and lands exactly on the C0==R0 comparator edge of the frame's last line
  (C9==R9, C4==R4, R5==0, outside adjustment) defers that line end — per §13.6.2's p.122
  chronogram, "just-in-time write considered this rollover" — so the line runs into the widened
  remainder (`hcc_end` in `rtl/CRTC.v`; engine term `rfd_r0_extend`). If the register state
  held at the line's actual end no longer satisfies C9==R9 ∧ C4==R4 (the documented "by line
  end" variant definitions), both RFD flags arm there with the same same-edge reload
  participation as the R5 route; a condition cancelled and restored before the end does not
  arm. Vectors `t13e`-`t13m` pin expiry, both precondition halves off the last line, the R9
  and R4 variants, restore, equal-value writes, display blanking at `C0==R1` on the extended
  line, and the live-type round-trip clearing of the hidden window. Cross-provider reviewed
  (`f7-r0-widening-independent-review.md`; blocking findings F-1/F-2 remediated same day).
- **Deliberately unmodeled / interpretation notes** (recorded per the same review):
  whether an arbitrary mid-frame widening write at `C0==R0` also extends its own line is left
  unmodeled pending sourced chronograms — the gate enforcing that boundary is exercised by
  `t13j`/`t13l`. The §8.6 R4-variant's second-frame stuck-C4 consequence is not
  modeled or vector-pinned: the RFD flags select reload and save behaviour only, and
  `row_frame_last`/`row_next` carry no RFD dependency, so nothing in the engine can
  suppress a C4 reset.
  A trigger window opened just before R0 is shrunk below the live C0 survives across that
  legal C0-overflow line (§13.5) and can arm at its far end — unmodeled corner. The
  end-state cancellation reading follows §13.7.1.2's explicit variants against §13.6.2's
  write-timing note (author question Q18, resolved 2026-08-26; SHAKER Module C `(1)` / D `(9)`
  provides hardware confirmation). The unarmed odd-R9 frame-parity toggle (§11.6.1) is read as
  free-running rather than armed-only; marked ⚠ for hardware.
- **Impact**: CRTC-1-specific demo effects (the technique is popular precisely because it's the
  easy rupture on type 1); SHAKER tests it.
- **Confidence: medium-high for both implemented routes.** The deterministic vectors are
  source-derived and exercise the timing races, cancellation semantics, and disarm paths;
  real SHAKER hardware remains the higher-authority check (Module C `(1)` / D `(9)`). Q4's
  later recheck opened F17 for the C9=R9 source-flag case; it is excluded from this confidence.
- **Verify**: V3 passed for the R5 and R0-widening routes; V2 (SHAKER RFD tests); V1 (CRTC-1 demos).

## F8. CRTC 1 vertical adjustment must use a separate C5 counter (C4/C9 keep counting)

- **Rule** (digest-01 §4/§4.1/§4.3 → ACCC §11.1-11.2, p.80-84): during adjustment, type 0 reuses
  C9 vs R5 (current model's approach — correct for type 0). **Type 1 has a genuine separate C5**:
  C9 keeps counting 0..R9 (VRAM row-select derives from C9!), C4 keeps incrementing at each
  C9==R9 while C5 counts the adjustment lines against R5. Plus the §4.4 bug: R5 rewritten to 0
  mid-adjustment does NOT end it on type 1 (C5 free-runs until R5 is set to a reachable value).
- **Current** (fixed, `c9f4a4e`): type 1 has a genuine C5 (`crtc_type1_engine.v:116` equality
  end, `:124-135` counter next-state) while C9 keeps cycling 0..R9 and C4 increments at each
  wrap; type 0 still reuses C9 vs R5 (`crtc_type0_engine.v:151` line-max selection). The §4.4
  R5=0 free-run bug is deliberately reproduced. Protected by `t08i`-`t08o`. A1's
  adjustment-ending VSYNC correction is now protected by `t08m` (and the corrected `t08g`
  oracle). A2 is protected in both directions: `t08n` requires a positive R4 rewrite on the
  exact adjustment-entry edge to suppress the C4=1 reload, while `t08o` requires an R9 write
  on that edge to retain it.
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
  (R4=10,R5=16,R9=3) as a testbench vector.
- **§11.2.4 corner closed by A2** (findings-review.md B5; p.84): an **R9 write landing
  exactly at `C0==R0` entering adjustment does not cancel** the VMA-from-R12/R13-while-C4==1
  reload, while an R4(>0) rewrite on that edge does. Deterministic `t08n`/`t08o` pin both
  directions.
- **Verify**: V3 (the §4.1 table); V2.

## F9. Type 0 R9 write at C0==R0 straddles the R9-to-R5 comparison switch

- **Rule** (digest-01 §3.1/§4.2 → ACCC §10.3.1/§11.2.2, p.75-76/81-83): on a type-0 last
  line entering adjustment, an R9 write exactly at C0==R0 straddles a comparator switch. C9 is
  first compared with the earlier R9, incrementing C4; because C4 then differs from R4, C9 is
  compared with R5 and can also increment. The documented R4=38/R9=7 example ends at
  C4=39,C9=8. This is not adequately specified as "old R9 in step (a), new R9 in step (b)".
- **Current**: the C0>=2 F12 slice now preserves the earlier C9/R9 result for C4 while using
  C9/R5 for C9 at exact C0==R0. `t16e` covers the character-edge result and `t16h` proves the
  same result for a mid-character bus write. The documented R4=38/R9=7 worked example pair is
  now encoded (`t12a` exact-R0 → C4=39,C9=8; `t12b` windowed companion → C4=38,C9=8, ACCC
  p.82 example 3), closing F9's deterministic coverage on branch `accuracy/f9-t12-closure`.
- **Impact**: single-cycle JIT R9 writes (demo timing surgery). The numeric expectation may be
  unchanged, but a fix based on two R9 snapshots would encode the wrong state transition and
  fail adjacent R4/R9-window cases from F12.
- **Confidence: medium.** The comparison sequence and numeric result are documented; exact RTL
  write/CLKEN ordering and adjacent-cycle observables still require traces.
- **Fix prompt**:
  > Keep `t16e`/`t16h` green. Add t12's complete documented C4=39,C9=8 case **and its
  > companion control** — an R9 write inside the `C0∈[2,R0−1]` window, documented as leaving
  > C4=38,C9=8 (ACCC p.82 example 3; findings-review.md B4) — without changing the implemented
  > comparator split. Do not replace it with a generic old-R9/new-R9 pair.
- **Verify**: V3 `t16e`/`t16h` required; full t12 and a hardware trace remain desirable before
  independent F9 closure.

## F10. Interlace (IVM) parity machinery — implemented for the unblocked scope, both types

- **Rule** (digest-03 §19.4-19.8 → ACCC ch.19): type 0 keeps split C9/C9.VMA with asymmetric
  IVM entry/exit comparisons, ParityFrame/ParityR6/ParityC9 state, VSYNC-delay-by-1-line
  correction for odd R9; type 1 has 2 parity states, R8-write effects at the 3rd vs 4th µs of
  the OUT, R9-parity-triggered alternation, and NO VSYNC drift correction. Additional interlace
  line appended per type-specific parity conditions. R9 programming formula differs (N-2 type 0
  IVM vs N-1 type 1).
- **Current** (implemented 2026-08-24 on `accuracy/f10-fixtures`, commits `20eb6d5` /
  `657ccde` / `3a2293a`; full record `accuracy/f10-implementation-notes.md`): both types run
  the documented machinery for everything the render-verified tables pin. Type 1: two-stage
  R8-toggle parity update (stage A at the next character edge, stage B one later; leaving
  stage A holds C9.0), ParityFrame/ParityC9 toggles, §19.8.2 counting with C9 carrying the
  parity. Type 0: split C9/C9.VMA ((C9×2)+ParityC9 mod 32), per-line limit test composed
  from a seam-latched value-doubled bit and a line-scoped target-parity bit (switch line
  R9|ParityFrame, steady IVM R9|ParityC9, exit plain R9), ParityFrame/ParityR6 per §19.5.2,
  ParityC9 seeded from ParityFrame at IVM turn-on. Shared parity flops live in the wrapper
  for the live-type-switch contract. 38 deterministic vectors (`t21a`-`t21p`,
  `t22a`-`t22s`, `t23a`-`t23c`) are all required passes, derived from the pp.210-211 panels
  and the pp.219-224 tables (render-verified 2026-08-24) — including the RA (C9-VMA)
  column, all eight exit tables, the type-1 IVM frame-boundary continuity, and
  snapshot-loaded R8=3 activation. The stack was independently reviewed 2026-08-25
  (`accuracy/f10-independent-review.md`; two blockings fixed, record has the remediation
  section). The old stepping/halving/field-OR
  approximation is removed; non-IVM behavior is bit-identical (t01-t20 unchanged; t09g's
  single RA expectation re-derived from §19.5.2).
- **Impact**: interlace demos (SHAKER 2.x uses 1/64-line positioning tricks); most games unaffected.
- **Confidence: high for the implemented even-R9 surface** (every asserted value traces to a
  render-verified table cell or the pseudocode; the type-1 model reproduces all 64 panel
  callouts). Medium overall until hardware: the odd-R9 half (F15), additional interlace line
  (F14), and post-exit frozen C9.VMA (F16) are deliberately unimplemented; the residuals
  listed in `f10-implementation-notes.md` (MID-VSYNC parity source after mid-frame R8 toggles,
  RFD×IVM, adjustment-during-IVM) are unpinned.
- **Remaining work**: F14/F15/F16 require fixtures before RTL. Q12 is resolved as an English
  qualifier omission: French v1.11 p.208 specifies IVM activation on every frame. The
  odd-C4 transition case still needs local fixtures; the source resolution does not verify
  the core's post-toggle pin timing (see `accc-author-questions.md` item 12). SHAKER
  interlace suite (Module B `(1)`/`(9)`, C parity entries) is the hardware exit for what is
  implemented.

## F11. Minor / confirmatory findings (no immediate action)

- **F11a — HSYNC width semantics** (`CRTC.v:350-351`; type-1 zero-width cut at
  `crtc_type1_engine.v:164`): equality-based `hsc == R3l` end +
  4-bit wrap naturally reproduces the "overflow on shrink" rule (digest-02 §4) ✓; type 1
  R3l=0-cancels-immediately ✓ explicitly coded. Type 0 mid-HSYNC write of R3l=0 wraps (correct).
  Only gap: CRTC 0's "restart without C3l reset if R3l modified at the exact end position"
  (§10) — exotic; leave.
- **F11b — VSYNC re-entrancy** (`vsync_allow`, `CRTC.v:380`, `:383-388`, `:436-451`):
  reproduces mechanism 2 including the R7=0/R4=0 lock and the R7=0,R4=1,R9=7 infinite-VSYNC
  bypass (digest-02 §18) ✓. Protected by `t03a`/`t03b`.
- **F11c — R12/R13 readback** (`CRTC.v:118-119`): type 0 returns stored, type 1 returns 0 ✓
  (primary detection vector, correct). Protected by `t01`.
- **F11d — Dummy register 31** (`CRTC.v:122`): 0xFF on type 1, 0x00 on type 0 ✓.
- **F11e — VSYNC width R3h** (`crtc_type0_engine.v:352`, `crtc_type1_engine.v:161`): type 0
  uses R3h (0→16 via 4-bit wrap ✓), type 1 fixed 16 ✓.
- **F11f — R16/R17 light pen**: not implemented (`CRTC.v:123` default arm reads 0). Real
  chips return a latched address; with no pen attached the value is effectively arbitrary.
  Low value; note only.
- **F11g — DE skew** (`CRTC.v:87`; index term `crtc_type0_engine.v:356`): R8 bits 5:4,
  type 0 only, 0/1/2-char delay + non-output ✓
  (matches ACCC §19.1 table; the digest's wrong bit-position bullets were corrected 2026-08-22
  per B8 — the table says bits 5:4, which is what the code uses).
- **F11h — R12/R13 mid-row immediacy on type 1** (ACCC §20.3.2 p.242, render-verified
  2026-08-25): the model reloads VMA from R12/R13 at **every non-final line boundary within
  C4=0** (plus row 1 after a row-0 adjustment entry) via `crtc1_row0_reload` /
  `crtc1_adj_row1_reload`, so an R12/R13 write lands in VMA within one line during row 0 —
  protected by `t20b`/`t20d`/`t20f`/`t20h`. The p.242 re-read resolved the residual: the
  second CRTC-1 chronogram draws the OUT bus activity spanning C0=62..1 across a row-0 seam
  (write landing on the 63→0 boundary edge) with OFFSET=#30xx from C0=0, while the paired
  CRTC-0 chronogram (§20.3.1) with identical timing keeps OFFSET=#10xx — so the type-1
  reload catches a write whose register update coincides with the reload edge, and the
  type-0 frame load misses it. Implemented 2026-08-25: the row-0 arm loads the post-edge
  register file (`r12_effective`/`r13_effective` mirror the register block's write/SNA
  priority); `t20j` pins the catch at a mid-row-0 boundary and at the frame origin, `t20k`
  pins the type-0 miss. Scope note (review N-5, 2026-08-25): the mid-row-0 catch is the
  drawn chronogram; the frame-origin catch is a prose inference ("when C0 and C9 go to 0
  and C4=0" covers the frame origin) — a later hardware test should treat the two halves
  accordingly. Deliberately unpinned residuals: writes landing mid-C0=0 or later
  (beyond the drawn window), and the same-edge phase of the §11.2.4 adjustment and §11.6
  RFD reload arms (different rules, not governed by p.242).
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
| 5 | F12 type 0 last-line/adjustment arbitration — deterministic complete | L (risky) | t16 required; hardware vectors pending |
| 6 | F5 deferred R0<2 entry state — deterministic complete | S-M | t09/t16 required; hardware pending |
| 7 | F4 overflow shortcut removal | M (risky) | F12 complete; testbench mandatory |
| 8 | F8 type 1 C5 counter | M | F4 |
| 9 | F9 C0==R0 R9-to-R5 switch | S-M | F12, t12/t16 |
| 10 | F7 RFD | L | F4, F8 |
| 11 | F10 interlace overhaul | XL | all above |
| Deferred | F6 spurious border byte | S-M | half-character interface decision or accepted approximation |
