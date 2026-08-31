# ACCC v1.11 — Author Feedback & Clarifications (Round 2)

**Date:** August 30, 2026
**Reference:** *The Amstrad CPC CRTC Compendium* v1.11 (Longshot / Logon System, 27 August 2026)
**Target:** Longshot (Author of *The Amstrad CPC CRTC Compendium*)

This document contains the courtesy corrections and clarification requests from the MiSTer
Amstrad core audit against **ACCC v1.11**. Source deductions, current model choices, and
hardware observations are distinguished below. The complete section-audited register is
[the bilingual difference ledger](accc-1.11-fr-en-differences.md); this note keeps the
author-facing report compact.

*Note: Historical Round 1 feedback (which resulted in the publication of ACCC v1.11) is archived in [accc-author-feedback-round1-2026-08-27.md](accc-author-feedback-round1-2026-08-27.md).*

Technical information sourced from *The Amstrad CPC CRTC Compendium* by Longshot (CC BY-NC-ND 4.0).

---

## 1. §19.5.2 — English Edition Omits the Repeated IVM Activation Qualifier (CRTC 0)

**Location:** English p.206, Note and worked-example introduction (diagram p.207);
French pp.207–208, with the qualifier on p.208.

**Resolved by comparing editions:** The French v1.11 example specifies that R8 goes to 3
on C4=1, C9=0 **“à chaque frame” — on every frame**. The English example omits those words.
The example therefore describes a disturbance recreated by repeated activation, not evidence
of an unexplained persistent state after a single activation.

**Suggested English clarification:** Add **“on every frame”** after the example's
R8-to-3 activation on C4=1, C9=0, matching the French edition.

**Separate deduction for a single activation:** If R8 is subsequently held at 3, registers
remain stable, frame origins continue to occur, and C4 continues to reach R6, the documented
counter/parity rules imply a return to the normal row-count pattern at the next frame origin:
`ParityFrame` is loaded from `ParityR6`, and for odd R9 the row parity is recalculated from
`C4.0 xor ParityFrame` (§19.5.2 English pp.205–206; §19.8.1 p.219). This deduction does not
undo the elapsed timing offset caused by the transition. `ParityR6` persists across frames
and can freeze when R6 is unreachable; frame totals also depend on register programming.

This is a source-reading resolution, not hardware confirmation or proof that our core's
post-toggle pin timing is correct. Local transition/timing validation remains separate;
there is no outstanding author question about a hidden recovery latch based on this example.

---

## 2. §11.3.2 — Confirm the C4 Reset Route During Stuck R5=0 Adjustment (CRTC 1)

**Location:** English §11.2.4 p.84 and §11.3.2 pp.85–86; French §11.2.4 p.85 and
§11.3.2 p.87.

**Source tension:** §11.2.4 says that C4 increments without considering R4 during adjustment.
The more specific R5=0 case in §11.3.2 says that the C4/R4 comparison continues to process
C4's return to zero while adjustment remains active. Both editions retain this distinction.

**Preferred reading (1), pending confirmation:** With R8=0, the R5=0 case permits the ordinary
C4/R4 reset at a C9=R9 line end without clearing adjustment or stopping C5. The later
positive-R5 sentence describes a reset route that also exits adjustment; it does not say
that this is the only possible C4 reset.

**Question for the author:**

> With R8=0 and adjustment stuck after R5 becomes zero, does C4 reset at the next
> C4=R4, C9=R9 line end while C5 continues counting and adjustment remains active?

**Current implementation, not silicon evidence:** Our core retains reading (2): during
stuck R5=0 adjustment, C4 ignores R4, advances through 127, and wraps by 7-bit overflow.
A reachable positive R5 terminates adjustment and resets the counters. `t08j` pins this
chosen model, including a pass through the next C4=R4, but cannot establish hardware behavior.
The model residual remains open; this documentation correction does not change RTL or tests.

**Proposed hardware discriminator (not yet run):** Use a Type-1 CRTC with R0=63, R4=10,
R9=3, R7=1 and R8=0. Enter adjustment with R5=16, then set R5=0 at C5=2 and leave the
registers unchanged. After the initial C4 overflow, measure successive VSYNC rising edges.
Reading (1) predicts a recurring interval of `(R4+1) × (R9+1) = 44` scanlines; reading (2)
predicts `128 × (R9+1) = 512` scanlines. These are paper-derived predictions for the two
readings, not measured results. A later reachable positive-R5 write provides an exit control.

---

## 3. French/English v1.11 corrections from the section-complete sweep

The comparison covered every technical section in chapters 3–29 and checked candidate
tables/diagrams on rendered original pages. These are offered as edition corrections, not
as hardware confirmations.

### Highest-impact English corrections

- **§4.2 p.18:** French says the two extra U.S.-ROM lines make the GA interrupt request arrive
  on the same scanline as CRTC VSYNC, but before it; English says “not the same line.”
- **§4.4.2 p.24:** French correctly says `INI` increments HL; English says it decrements HL.
- **§10.3.1.2 FR p.77 / EN p.76:** French uses `C9<>R9`; English's `C9<=R9` loses the
  C9>R9 overflow route.
- **§12.2.1 FR p.96 / EN p.94:** French gives complementary RLAL write windows—line N at
  `C0>1`, or N+1 at `C0<2`. English omits the line-N condition.
- **§13.7.2 FR pp.126–128 / EN pp.124–126:** English changes `C0=0` to `C0=R0`,
  `C4=R4` to `C4=C4`, reverses the no-adjustment condition, omits the additional `C4=R4`
  conjunct from the **initial programmed-increment sentence** in the R0=1 case, and says R5
  where French and the 8–31 example require `R5-1`. Both editions separately describe the
  later partially completed overflow after C4 has diverged.
- **§16.2.1 FR p.162 / EN p.160:** the English R7.NJIT heading incorrectly imports the JIT
  definition and makes its condition inconsistent.
  French defines NJIT as programming R7 before C4 reaches it and JIT as programming R7 with
  the current C4 value.
- **§16.3 and §16.4.4 FR pp.169,172 / EN pp.167,170:** English §16.3 omits the complete
  `C4=R7,C9=0,C0=0` parenthetical. English §16.4.4 retains that headline condition but omits
  C9>0 from the later sentence describing when an R7 write fails to trigger VSYNC.
- **§17.2.2 FR p.180 / EN p.178:** French derives row-select bits from C9; English adds C5.
- **§18.3.2 FR p.191 / EN p.190:** French tests whether R6 is zero when C0 reaches R1;
  English changes this into the historical condition that R6 was zero at least once.
- **§19.5.5 FR p.214 / EN p.213:** French and the later English explanation say CRTC3/4
  ParityC9 changes for odd R9; the opening English bullet says even.
- **§20.3.2 p.242:** French says type 1 reloads VMA at every C0=0 while C4=0,
  independently of C9. English omits the recurrence and independence qualifiers.
- **§28.1.9 p.293:** French lists the full type-3/4 readback map through R14/R15 and points
  to §21.2.3; English stops at R13 and points to §20.3.4.

### Clarifications requested

1. **§7.2:** which synchronization routine is intended: French `19968-21` with `1+1+3`, or
   English `19968-23` with `2+1+3` and the separate −1 µs positioning adjustment?
2. **§4.4.3–4:** is English's addition of `OUTD` intentional, and does the ASIC use the same
   `/WAIT` request/repetition mechanism as the Gate Array? The generic English “write I/O
   instruction 1 µs earlier” appears broader than the French `OUT(C),r8` rule.
3. **§11.6:** does the French statement that the RFD VMA-source state persists independently
   of a C0/R1 test describe persistence before the later comparative event, rather than an
   unconditional lifetime?
4. **§14.1 FR p.132:** should “HSYNC begins when C3l reaches R3l” say **ends**? The French
   definitions, tables, and later prose all appear to require “ends.”
5. **§16.4.1.2 EN p.169:** are the two English-only R0/VSYNC paragraphs normative material
   accidentally omitted from French, or unsupported supplemental text?
6. **§19.3.4:** should English retain the French warning about complete-interlace screen
   construction from VSYNC at C4=R7, rather than replacing it with frame-start-only R8 advice?
7. **§19.5.3 FR p.209:** is `ParityC9=ParityFrame` at every type-1 frame start an explicit
   realignment assignment even after the states have been made unequal by an R8 transition?
8. **§22 EN p.250:** can the English-only warning about effects on “other registers” be made
   concrete, or should it be removed/added to French as intentionally non-normative guidance?

### Lower-risk errata

The full ledger also records malformed formulas/wording in §§11.1, 12.5, French 14.4 /
English unnumbered continuation, French 14.8 / English 14.7, 15.4.1, 16.4.1–3, 19.5.4,
20.3.3, and 27.2, plus cross-reference errors at French §§7.2 and 9.3.4.1 and English
§§23.2, 23.3, and 28.1.9. These are useful for editorial cleanup but do not presently
justify RTL changes.

---

## Source verification

The initial 2026-08-28 comparison and section-complete 2026-08-30 sweep used pdf-inspector
extraction and rendered relevant pages from
[English v1.11](https://shaker.logonsystem.eu/ACCC1.11-EN.pdf) and
[French v1.11](https://shaker.logonsystem.eu/ACCC1.11-FR.pdf). SHA-256 fingerprints:

- English: `3e45eb7eea7dc8f0d7211f78bec4f8d00530ce3c00da2e76034fb24f7a751868`
- French: `4409e3a2e77cd54e499c6956446b01bce93f79a1c1ba366201d514cf6e3c0d47`
