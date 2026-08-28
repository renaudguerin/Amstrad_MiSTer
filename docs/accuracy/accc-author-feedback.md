# ACCC v1.11 — Author Feedback & Clarifications (Round 2)

**Date:** August 28, 2026
**Reference:** *The Amstrad CPC CRTC Compendium* v1.11 (Longshot / Logon System, 27 August 2026)
**Target:** Longshot (Author of *The Amstrad CPC CRTC Compendium*)

This document contains one English-edition clarification and one hardware-behavior confirmation
request from the MiSTer Amstrad core accuracy audit against **ACCC v1.11**. Source deductions,
current model choices, and hardware observations are distinguished below.

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

## Source verification

The 2026-08-28 comparison used pdf-inspector extraction and rendered relevant pages from
[English v1.11](https://shaker.logonsystem.eu/ACCC1.11-EN.pdf) and
[French v1.11](https://shaker.logonsystem.eu/ACCC1.11-FR.pdf). SHA-256 fingerprints:

- English: `3e45eb7eea7dc8f0d7211f78bec4f8d00530ce3c00da2e76034fb24f7a751868`
- French: `4409e3a2e77cd54e499c6956446b01bce93f79a1c1ba366201d514cf6e3c0d47`
