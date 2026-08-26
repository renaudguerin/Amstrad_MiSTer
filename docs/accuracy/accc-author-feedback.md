# ACCC v1.10 — Author Feedback, Unresolved Questions & Clarifications

This document compiles feedback, unresolved source questions, and cross-referenced clarifications for Longshot (author of *The Amstrad CPC CRTC Compendium* v1.10, `docs/ACCC1.10-EN.pdf`), gathered during the accuracy audit and RTL implementation of the MiSTer Amstrad core.

Technical information sourced from *The Amstrad CPC CRTC Compendium* by Longshot (CC BY-NC-ND 4.0).

---

## 1. Genuinely Unresolved Questions / Source Conflicts

These items represent genuine internal conflicts or unknown silicon mechanisms in v1.10 where author insight or physical hardware tests are necessary.

### 1.1 §28.1.1 vs §§11.2.4/11.3.2 — Type-1 VSYNC Discriminator Boundary ($R_7=38$ vs $R_7=39$)
* **Location:** Chapter 28.1.1 (p. 292) vs Chapter 11.2.4 (pp. 83–84) & Chapter 11.3.2 (p. 85).
* **The Conflict:**
  * In §28.1.1 (CRTC identification via C4/C9 overflow), the test programs `R4=36, R9=7, R5=16` (312 lines total) and states:
    > *"Type 1: VSYNC stops occurring once R7 > 39"* (implying $C_4$ reaches 39 during extra line management).
  * However, the detailed counting rules in Chapter 11 state that during type-1 adjustment, $C_4$ increments only when $C_9$ reaches $R_9$ (every 8 lines). With $R_5=16$, there are exactly two 8-line wraps ($C_4 = 37 \to 38$). When $C_5$ completes at $R_5=16$, $C_4$ is reset directly to 0 at the frame end, so a third increment ($C_4=39$) is unreachable. Under this arithmetic, VSYNC stops when $R_7 > 38$.
* **Author Clarification Needed:** Is the printed $R_7 > 39$ in §28.1.1 an off-by-one typo for $R_7 > 38$, or does type-1 silicon execute an additional $C_4$ increment / counting cycle during adjustment that is not captured in Chapter 11?

### 1.2 §13.2.5 (p. 107) — Silicon Cause for $R_0=1, R_5=0$ Adjustment Termination
* **Location:** Chapter 13.2.5 (p. 107).
* **The Note:**
  > *"For some reason I haven't determined yet, C9 (not C9+1) is compared to R5 [to stop the adjustment after one line]."*
* **Current Status in Core:** The observable behavior (the adjustment ends after 1 line) is verified and modelled.
* **Author Question:** Did subsequent silicon or decapping analysis determine the hardware cause for comparing $C_9$ rather than $C_9+1$ in this specific $R_0=1$ non-cancellation path?

### 1.3 §19.5.2 (p. 206) — Subsequent-Frame Recovery After Odd-$C_4$ IVM Activation
* **Location:** Chapter 19.5.2 (p. 206, Note).
* **The Context:**
  * The text notes that activating Interlace Video Mode ($R_8 \to 3$) on an odd $C_4$ causes a 1-line VSYNC phase imbalance on the transition frame (e.g., $C_4=1$ receives 5 lines on an odd frame vs 4 lines on an even frame).
  * The text quantifies the transition frame but is silent on subsequent frames.
* **Analysis & Question:** Our implementation infers that because parity state bits (`ParityFrame`, `ParityC9`) are re-evaluated per frame / per character without residual phase memory, the system recovers its normal balanced rhythm on subsequent frames. Can the author confirm that this perturbation is strictly limited to the transition frame and self-corrects on hardware?

---

## 2. ACCC Ambiguities Resolved via Cross-Referencing

The following ambiguities were successfully resolved by cross-referencing other chapters, worked case studies, or external component datasheets. They are listed here to assist the author in refining explanations for the next edition.

### 2.1 §10.3.1 (pp. 75–76) — Last-Line State and $C_0 > 1$ Writes (No Late-Arming)
* **Ambiguity:** p. 75 states that if $R_9$ or $R_4$ is modified when $C_0 > 1$ *"so that C4==R4 and C9==R9, then the last line state is true and it cannot become false again until the next comparison"*. This could be interpreted as "late-arming" (a mid-line write establishing equality forces the current line to become the last line).
* **Resolution:** Cross-referencing with §12.2 (p. 92) and the Line-to-Line Rupture case study in §12.2.1 (p. 93) establishes that writing $R_4=R_9=0$ after $C_0=1$ on line 1 does *not* make it the last line ($C_4$ increments to 1 on line 2). The p. 75 rule describes the **survival** of an already-armed state (when vertical adjustment was not activated at $C_0=2$) against mid-line writes that would otherwise break equality, not late-arming.
* **Suggested Clarification:** Clarify that $C_0 > 1$ writes cannot newly arm a line that evaluated false during $C_0 \in \{0, 1\}$.

### 2.2 §13.2.1 (p. 104) / §13.2.4 (p. 105) — $R_0=0$ Freeze Duration Accounting
* **Ambiguity:** The text states that freezing $R_0=0$ for $64 \times 8\ \mu\text{s}$ *"amounts to 'forgetting' 8 lines (C4−1 if R9=7)"*. The notation `(C4−1 ...)` could be misread as an arithmetic decrement of counter $C_4$.
* **Resolution:** The detailed case study in §13.2.6 (p. 108) proves counters are simply frozen (apart from $C_4$'s one-time "last hiccup"). $512\ \mu\text{s}$ equals 8 raster lines of display time, which with $R_9=7$ corresponds to one character row duration. The parenthetical denotes lost wall-clock progress.
* **Suggested Clarification:** Rephrase as elapsed wall-clock / raster time loss rather than arithmetic counter manipulation.

### 2.3 §11.6.1 (p. 88) — Repeated RFD on $C_9=R_9$ Disabling the VMA-Source Flag
* **Ambiguity:** The sentence *"A RFD triggered on the last line C9=R9 disables the state allowing VMA to be updated with R12/R13"* sits between Case 2 and RFD#10.
* **Resolution:** Adjudicated as a general property of the CRTC 1 RFD state machine (Case 2): triggering an RFD when $C_9=R_9$ suppresses the $R_{12}/R_{13}$ VMA-source flag for subsequent lines. The following "However" introduces RFD#10 as an exception only for parity handling.
* **Suggested Clarification:** Make the paragraph separation explicit so the disable rule is clearly understood as general Case 2 behavior.

### 2.4 §§19.3, 19.5.1, 19.6.1/2 (pp. 198, 205, 216) — Additional Interlace Line Generation & Frame Attribution
* **Ambiguity:** p. 198 says the additional line ends *"the first frame"*; pp. 205/216 state it is added at the end of the *even* frame; p. 199 shows the *odd* frame lasting $20032\ \mu\text{s}$ "inheriting" it.
* **Resolution:** Cross-referencing §19.6.1 (type 0 gates on `ParityR6` odd) and §19.6.2 (type 1 gates on `ParityFrame` even) with p. 199 establishes that:
  1. The extra scanline is **generated** at the end of the ParityFrame-even frame's construction.
  2. The extra scanline's duration is **measured/counted** in the following odd frame ($313\text{ lines} = 20032\ \mu\text{s}$).
* **Suggested Clarification:** Standardize the vocabulary across these sections to distinguish between *generation boundary* (end of even frame) and *duration accumulation* (the 313-line odd frame).

### 2.5 §21.2.2 (p. 245) vs §28.1.9 (p. 293) — Type-1 (UM6845R) Readable Register Set
* **Ambiguity:** §28.1.9 states that on CRTC 1 *"all registers return 0 except register 31"*, which directly contradicts §21.2.2 (which lists $R_{14}–R_{17}$ as readable).
* **Resolution:** UM6845R datasheet Figure 3 and CPC hardware pinout (expansion pin 47 connected to CRTC LPSTB) confirm §21.2.2:
  * $R_{14}/R_{15}$ (cursor) are read/write.
  * $R_{16}/R_{17}$ (light pen) are read-only latches.
  * $R_{12}/R_{13}$ are write-only and read back as 0 (the primary CRTC 0 vs 1 discriminator).
* **Suggested Clarification:** Correct §28.1.9 to state that all *other* registers return 0 (except $R_{14}–R_{17}$ and undefined reg 31).

### 2.6 §21.3.3 (pp. 246–248) vs §28.1.8 (p. 293) — Type-1 Status Register Polling Bit
* **Ambiguity:** §28.1.8 specifies polling *"the transition of bit 6"* at `&BE00` to detect CRTC 1. However, the chronograms and bit maps in §21.3.3 show transitions on bit 5 (`00100000` $\leftrightarrow$ `00000000`).
* **Resolution:** UM6845R Figure 3 and p. 246 map:
  * Bit 6 (`L`): Light Pen Strobe occurred (requires an external pulse on LPSTB).
  * Bit 5 (`V`): Vertical Blanking / BORDER-R6 state (toggles automatically each frame).
  * Bit 6 in §28.1.8 is a typographical error for **bit 5**. (The always-1 bit 6 on p. 248 belongs exclusively to ASIC CRTC 3/4 STATUS 1).
* **Suggested Clarification:** Update §28.1.8 to specify bit 5.

### 2.7 §13.6.2 (p. 122) vs §13.7.1.2 (p. 124) — $R_0$-Widening RFD Arming Condition
* **Ambiguity:** The p. 122 chronogram note (*"RFD activated on CRTC 1 if R4 and/or R9 modified until C0=7F"*) could be interpreted as a write-event trigger.
* **Resolution:** §13.7.1.2 defines the variants by line-end states ($C_9 \ne R_9$ / $C_4 \ne R_4$ at the end of the last line). The word "until" in French (*"jusqu'à"*) denotes persistence of the modified values up to line end.
* **Suggested Clarification:** Explicitly state that RFD arming requires the condition to remain unsatisfied at the moment the widened line completes.

### 2.8 §19.8.1 (p. 220, pp. 223–224) — Post-IVM Exit Line-End Comparison
* **Ambiguity:** In the exit tables on pp. 223–224, scanlines continue counting past $C_9=R_9$ with $R_9=6$ after leaving IVM ($R_8 \to 0$), rather than resetting at $C_9=6$.
* **Resolution:** Cross-referencing the 8 exit panels with the p. 220 recovery recipe (*"program R9 with C9.VMA..."*) confirms that upon exit, the comparator continues testing the **frozen $C_9.\text{VMA}$ register content** against plain $R_9$, rather than reverting to live $C_9$.
* **Suggested Clarification:** Explicitly state in §19.8.1 that post-exit lines retain the frozen $C_9.\text{VMA}$ comparison until $R_9$ is reprogrammed.

---

## 3. Typographical Errors and Errata

| Section & Page | Current Text in v1.10 | Corrected Reading | Evidence / Rationale |
| :--- | :--- | :--- | :--- |
| **§14.1, p. 130** | *"the HSYNC **starts** as soon as the C3L counter reaches the value of R3L"* | ...the HSYNC **stops / ends**... | HSYNC starts at $C_0=R_2$; reaching $R_3$ terminates the pulse. |
| **§14.3, p. 133** | CRTC 0 $R_3=3$ NJIT table entry: `1,0525` | `1,0625` | All values sit on the $0.0625\ \mu\text{s}$ lattice; paired JIT is $1.3125$ ($1.3125 - 0.25 = 1.0625$). |
| **§19.8.1, p. 219** | `If R9.0=0` (in C9 reset branch) | `If R9.0=1` | Contradicts gloss (*"C9 parity switched if R9 is odd"*), §19.5.2 p.205, and all $R_9=6$ even / $R_9=7$ odd tables. |
| **§19.8.1, p. 219** | `((C9 x 2) or ParityFrame) == (R9 + ParityFrame)` | Settled by p. 220 prose (three distinct phases) | The two `ParityFrame` terms cancel arithmetically, making odd-$R_9$ row termination impossible. |
| **pp. 223–224, p. 223** | Bottom-left panel cell: $C_4=2, C_9=7$ | $C_4=1, C_9=7$ | Isolated typo; an increment of $C_4$ without a $C_9$ reset is physically impossible. |
