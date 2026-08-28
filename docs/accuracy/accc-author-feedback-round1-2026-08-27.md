# ACCC v1.10 / v1.11 — Author Feedback Round 1 (Archived / Resolved)

> **ARCHIVE NOTICE (2026-08-28):**
> This document records **Round 1** of feedback and clarifications submitted to Longshot (author of *The Amstrad CPC CRTC Compendium*) based on the ACCC v1.10 audit.
> 
> Following submission of this feedback, the author released **ACCC v1.11** on **August 27, 2026**, incorporating and acknowledging these clarifications.
>
> Outstanding questions for subsequent rounds are tracked in the active [accc-author-feedback.md](accc-author-feedback.md).

Technical information sourced from *The Amstrad CPC CRTC Compendium* by Longshot (CC BY-NC-ND 4.0).

---

## 1. Questions & Source Conflicts (Status in v1.11)

### 1.1 §28.1.1 vs §§11.2.4/11.3.2 — Type-1 VSYNC Discriminator Boundary ($R_7=38$ vs $R_7=39$)
* **Location:** Chapter 28.1.1 (p. 292) vs Chapter 11.2.4 (pp. 83–84) & Chapter 11.3.2 (p. 85).
* **The Conflict:** In §28.1.1 (CRTC identification via C4/C9 overflow), the test programs `R4=36, R9=7, R5=16` (312 lines total) and stated in v1.10: *"Type 1: VSYNC stops occurring once R7 > 39"*. Arithmetic in Chapter 11 proves $C_4$ reaches at most 38 before direct frame reset to 0.
* **Status in ACCC v1.11:** **RESOLVED / FIXED**. The author corrected p. 292 to *"On a CRTC 1 or 2, if R7>38, then the VSYNC does not occur anymore"*, confirming the off-by-one typo in v1.10.

### 1.2 §13.2.5 (p. 107) — Silicon Cause for $R_0=1, R_5=0$ Adjustment Termination
* **Location:** Chapter 13.2.5 (p. 107).
* **The Note in v1.10:** *"For some reason I haven't determined yet, C9 (not C9+1) is compared to R5 [to stop the adjustment after one line]."*
* **Status in ACCC v1.11:** **RESOLVED / CLARIFIED**. The text was updated to *"The adjustment stops after 1 line when the calculated C9 becomes equal to R5"*. Observable behavior matches our core.

### 1.3 §19.5.2 (p. 206) — Subsequent-Frame Recovery After Odd-$C_4$ IVM Activation
* **Location:** Chapter 19.5.2 (p. 206, Note).
* **The Context:** The text notes that activating Interlace Video Mode ($R_8 \to 3$) on an odd $C_4$ causes a 1-line VSYNC phase imbalance on the transition frame (e.g., $C_4=1$ receives 5 lines on an odd frame vs 4 lines on an even frame).
* **Status in ACCC v1.11:** **OUTSTANDING / INFERRED**. Not modified in v1.11 text. Our implementation's memoryless parity evaluation model self-corrects after the transition frame, matching physical hardware behavior. Promoted to Round 2 feedback.

---

## 2. ACCC Ambiguities Resolved via Cross-Referencing (Status in v1.11)

### 2.1 §10.3.1 (pp. 75–76) & §12.4.1 (p. 95) — Last-Line State, $C_0 > 1$ Writes & CRTC 2 $C_0=0$ Evaluation Timing
* **Ambiguity in v1.10:** p. 75 stated that if $R_9$ or $R_4$ is modified when $C_0 > 1$ *"so that C4==R4 and C9==R9, then the last line state is true..."*, which could be misread as "late-arming".
* **Status in ACCC v1.11:** **RESOLVED & SIGNIFICANTLY EXPANDED**.
  1. p. 75 updated: *"while C4 was equal to R4 and C9 was equal to R9, then the last line state remains true..."*, explicitly confirming no late-arming on $C_0 > 1$.
  2. §12.4.1 (p. 95) added explicit silicon evaluation timing for CRTC 2:
     > *"At the beginning of a line (C0==0), the comparison uses the updated value of R4, but the previous value of R9 (an update of R9 on C0==0 occurs too late for this evaluation)."*
     (For CRTC 0, §12.2 pp. 92–94 governs, evaluating updated $R_4$ and $R_9$ same-edge writes on $C_0<2$).

### 2.2 §13.2.1 (p. 104) / §13.2.4 (p. 105) — $R_0=0$ Freeze Duration Accounting
* **Ambiguity in v1.10:** Notation `(C4−1 if R9=7)` could be misread as counter decrement.
* **Status in ACCC v1.11:** **RESOLVED / FIXED**. Parenthetical removed; text clarifies that freezing $R_0=0$ for $64 \times 8\ \mu\text{s}$ amounts to "forgetting" 8 lines of wall-clock time.

### 2.3 §11.6.1 (p. 88) — Repeated RFD on $C_9=R_9$ Disabling the VMA-Source Flag
* **Ambiguity in v1.10:** Misleading "However" before RFD#10.
* **Status in ACCC v1.11:** **RESOLVED / FIXED**. "However" removed; paragraph distinction made clear.

### 2.4 §§19.3, 19.5.1, 19.6.1/2 (pp. 198, 205, 216) — Additional Interlace Line Generation & Frame Attribution
* **Ambiguity in v1.10:** Frame naming ("first frame" vs "even frame" vs 313-line odd frame).
* **Status in ACCC v1.11:** **RESOLVED / FIXED**. p. 198 clarified: *"added at the end of the first even frame"*; p. 199 clarified: *"and lasts 20032 µsec (313 lines)"*.

### 2.5 §21.2.2 (p. 245) vs §28.1.9 (p. 293) — Type-1 (UM6845R) Readable Register Set
* **Ambiguity in v1.10:** §28.1.9 stated *"all registers return 0 except register 31"*, contradicting §21.2.2.
* **Status in ACCC v1.11:** **RESOLVED / FIXED**. §28.1.9 updated to *"except for R14-R17 and undefined R31"*.

### 2.6 §21.3.3 (pp. 246–248) vs §28.1.8 (p. 293) — Type-1 Status Register Polling Bit
* **Ambiguity in v1.10:** §28.1.8 specified polling bit 6 at `&BE00` instead of bit 5.
* **Status in ACCC v1.11:** **RESOLVED / FIXED**. §28.1.8 updated to *"test the transition of bit 5"*.

### 2.7 §13.6.2 (p. 122) vs §13.7.1.2 (p. 124) — $R_0$-Widening RFD Arming Condition
* **Ambiguity in v1.10:** Chronogram note phrasing.
* **Status in ACCC v1.11:** **RESOLVED / FIXED**. Cross-reference added to §13.7.1.

### 2.8 §19.8.1 (p. 220, pp. 223–224) — Post-IVM Exit Line-End Comparison
* **Ambiguity in v1.10:** Exit panels in tables.
* **Status in ACCC v1.11:** **RESOLVED / FIXED**. Table cells corrected on pp. 223–224.

---

## 3. Typographical Errors and Errata (Status in v1.11)

| Section & Page | v1.10 Text | Corrected Reading in v1.11 | Status in v1.11 |
| :--- | :--- | :--- | :--- |
| **§14.1, p. 130** | *"the HSYNC **starts** as soon as the C3L counter reaches the value of R3L"* | *"the HSYNC **ends** as soon as the C3L counter reaches the value of R3L"* | **FIXED** |
| **§14.3, p. 133** | CRTC 0 $R_3=3$ NJIT table entry: `1,0525` | `1,0625` | **FIXED** |
| **§19.8.1, p. 219** | `If R9.0=0` (in C9 reset branch) | `If R9.0==1` | **FIXED** |
| **§19.8.1, p. 219** | `((C9 x 2) or ParityFrame) == (R9 + ParityFrame)` | Settled by p. 220 prose | **CONFIRMED** |
| **pp. 223–224, p. 223** | Bottom-left panel cell: $C_4=2, C_9=7$ / $C_4=1, C_9=7$ | $C_4=2, C_9=0$ (`2 0 0`) | **FIXED** |
| **pp. 223–224, p. 224** | Bottom cells corrupted with `1 7 7` | Corrected to `2 0 0` | **FIXED** |
| **§1.2, p. 12** | (Changelog) | *"1.11 27/08/2026 Correction of several minor typos (Thanks to the AI feedback via Renaud Guerin)."* | **ADDED** |
