# ACCC v1.10 to v1.11 Differences & Accuracy Impact Report

This report documents the changes between the English editions of *The Amstrad CPC CRTC Compendium* by Longshot / Logon System:

- **v1.10**, dated 20 July 2026 / 22 August 2026; 295 PDF pages (SHA-256 `1bd6f0e3a06022d03fd40b51d4d622afef2675954a483780f0922cdf1e33a560`)
- **v1.11**, dated 27 August 2026; 295 PDF pages (SHA-256 `3e45eb7eea7dc8f0d7211f78bec4f8d00530ce3c00da2e76034fb24f7a751868`)

Technical information sourced from *The Amstrad CPC CRTC Compendium* by Longshot (CC BY-NC-ND 4.0).

---

## 1. Methodology and Scope

Both editions were verified page-by-page across all 295 pages:
1. **Mechanical text extraction and normalized diff**: Extracted structured Markdown via `pdf-inspector` and word streams via `PyMuPDF`. Stripped edition footers (`V1.10 – 07.2026` vs `V1.11 – 08.2026`) and compared text across all pages.
2. **Visual & Geometric Element Verification**: Filtered vector drawing streams and rendered 200 DPI PNGs for changed technical pages and diagrams.
3. **Findings**: Both editions have **identical 1-to-1 page mapping (295 pages)**. Exactly **278 pages are 100% word-identical**. Substantive updates occur on **17 pages**.

---

## 2. Complete Inventory of Changes

| Page | Section | Change Summary | Classification & Details |
| :--- | :--- | :--- | :--- |
| **p. 1** | Cover | Title version updated to V1.11 | Metadata |
| **p. 12** | §1.2 Changelog | Added v1.11 entry | *"1.11 27/08/2026 Correction of several minor typos (Thanks to the AI feedback via Renaud Guerin)."* |
| **p. 75** | §10.3.1 | Last-line state survival wording | Replaced *"so that C4==R4 and C9==R9, then the last line state is true"* $\to$ *"while C4 was equal to R4 and C9 was equal to R9, then the last line state remains true"*. Confirms no mid-line late arming. |
| **p. 88** | §11.6.1 | RFD VMA-source disable phrasing | Removed confusing transitional *"However,"* before the RFD#10 sentence. Confirms Case 2 VMA-source suppression is general. |
| **p. 95** | §12.4.1 | **CRTC 2 $C_0=0$ evaluation timing** | Added explicit silicon evaluation order for CRTC 2: at $C_0=0$, the Last Line comparison uses the **updated** value of $R_4$, but the **previous** value of $R_9$ ($R_9$ update occurs too late for this evaluation). |
| **p. 96** | §12.4.1 | Text reflow | Formatting adjustment following p. 95 expansion. |
| **p. 104** | §13.2.1 | $R_0=0$ freeze duration | Removed misleading parenthetical `(C4-1 if R9=7)`. Text states freezing $R_0=0$ for $64 \times 8\ \mu\text{s}$ amounts to "forgetting" 8 lines of wall-clock time. |
| **p. 107** | §13.2.5 | $R_0=1, R_5=0$ adjustment termination | Replaced *"For some reason I haven't determined yet, C9 (not C9+1) is compared to R5"* $\to$ *"The adjustment stops after 1 line when the calculated C9 becomes equal to R5"*. |
| **p. 122** | §13.6.2 | CRTC 1 $R_0$-widening chronogram | Added note *"See chapter 13.7.1 for details"* and adjusted table formatting. |
| **p. 130** | §14.1 | HSYNC termination rule | Corrected *"the HSYNC starts as soon as the C3L counter reaches the value of R3L"* $\to$ *"the HSYNC **ends** as soon as the C3L counter reaches the value of R3L"*. |
| **p. 133** | §14.3 | CRTC 0 $R_3=3$ NJIT table | Corrected typo `1,0525` $\to$ `1,0625` (aligning with $0.0625\ \mu\text{s}$ lattice). |
| **p. 198** | §19.3 | Interlace additional line generation | Clarified frame generation: *"added at the end of the first **even** frame"*. |
| **p. 199** | §19.3 | Interlace additional line duration | Added explicit scanline count: *"and lasts 20032 µsec (**313 lines**)"*. |
| **p. 219** | §19.8.1 | CRTC 0 odd-$R_9$ IVM algorithm | Corrected pseudo-code branch: `If R9.0=0` $\to$ `If R9.0==1` (*"C9 parity switched if R9 is odd"*). |
| **p. 223** | §19.8.1 | CRTC 0 IVM exit panel tables | Fixed corrupted table cells in panels 1 & 3: `1 7 7 1 7 7` / `2 7 7 2 3 3` $\to$ `2 0 0 2 0 0` / `2 0 0 2 3 3`. |
| **p. 224** | §19.8.1 | CRTC 0 IVM exit panel tables | Fixed corrupted table cells in panels 5 & 7: `1 7 7 1 7 7` $\to$ `2 0 0 2 0 0`. |
| **p. 292** | §28.1.1 | Type-1 VSYNC discriminator limit | Corrected off-by-one typo: *"if R7>39"* $\to$ *"if R7>**38**"*. |
| **p. 293** | §28.1.8 | Type-1 status register polling bit | Corrected typo: *"transition of bit 6"* $\to$ *"transition of bit **5**"*. |
| **p. 293** | §28.1.9 | Type-1 readable registers | Corrected *"all registers return 0 except register 31"* $\to$ *"all registers return 0 except for **R14-R17 and undefined R31**"*. |

---

## 3. RTL and Verification Impact Analysis

1. **CRTC 2 $C_0=0$ Evaluation Timing (§12.4.1, p. 95) vs CRTC 0 (§12.2, pp. 92–94) — Finding F19**:
   - In ACCC v1.11, §12.4.1 (p. 95) describes the internal state machine for CRTC 2 (Motorola MC6845).
   - In contrast, CRTC 0 (HD6845S / UM6845) is governed by §12.2 (pp. 92–94), which specifies that modifying $R_4$ or $R_9$ on $C_0<2$ evaluates the updated value.
   - Our Type-0 RTL (`rtl/crtc_type0_engine.v`) correctly implements same-edge evaluation for both $R_4$ and $R_9$ on $C_0=0$. Tests `t12c`–`t12e` verify and pin this behavior.
   - Action: Finding F19 categorized as CRTC-2 specific (out of scope for classic Type 0/1 core).
2. **Type-0 IVM Odd-$R_9$ Counting and Exit Panels (§19.8.1, pp. 219, 223–224)**:
   - Our RTL already implements the corrected `If R9.0==1` behavior and post-exit line comparator via Findings F15 and F16. The v1.11 text and table fixes formally validate our RTL implementation and soak hash.
3. **Type-1 Discriminators and Readable Registers (§28.1.1, §28.1.8, §28.1.9, §21.2.2)**:
   - Our RTL already implements the validated register readback and status bit behavior via Findings F17 and F18. The v1.11 errata corrections align the Compendium text with our test vectors.

---

## 4. ACCC v1.11 to v1.11(b) Re-Issue Differences & Accuracy Impact Report (2026-09-11)

In early September 2026, Longshot re-issued both the French and English PDF editions of *The Amstrad CPC CRTC Compendium* v1.11 with the MiSTer project's Round 1 and Round 2 feedback applied, without incrementing the version number. This section documents the differences between the original v1.11 release and the re-issued v1.11(b) release.

### 4.1 Sources and Methodology

- **French Authority (original v1.11)**: 295 pages, SHA-256 `4409e3a2e77cd54e499c6956446b01bce93f79a1c1ba366201d514cf6e3c0d47`.
- **French Authority (re-issue v1.11b)**: 295 pages, SHA-256 `28f25c73c1797578522f34ce9ff558210386972c9257b5b8081927483ee02c3b`.
- **English Working Translation (original v1.11)**: 295 pages, SHA-256 `3e45eb7eea7dc8f0d7211f78bec4f8d00530ce3c00da2e76034fb24f7a751868`.
- **English Working Translation (re-issue v1.11b)**: **296 pages**, SHA-256 `69d6a6a77de472937d41778ad48fc4fb427a937a24d3054f6d42c0b6ccfcc3e9`.

Extraction was performed with `pdf-inspector` into `docs/accuracy/extract/inspector-v1.11b-{en,fr}/` and verified against the original extractions using a normalized text and visual diff pipeline (PyMuPDF at 200 DPI).

### 4.2 Structural and Pagination Findings

1. **French Edition**: Page count remains strictly identical (295 pages). Exactly 273 pages are 100% word-identical; substantive or typographic updates occur on **22 pages**.
2. **English Edition**: Page count increased by 1 (295 $\to$ 296 pages).
   - In original v1.11, §14.8 ("HSYNC ET INTERRUPTIONS" in French p. 144) was numbered §14.7 in English (p. 143), and §14.9 ("SCHÉMATIQUES HSYNC" in French p. 145) was completely omitted from English.
   - In v1.11(b), Longshot restored the missing section as **§14.9 ("HSYNC SCHEMATICS", new pp. 144–145)** with complete chronograms for CRTC 0, 1, 2, 4, 3, and numbered §14.4 ("HSYNC AND FRAME POSITION", p. 123).
   - As a direct consequence, **all English pages from p. 146 onward shift by +1** relative to the original v1.11 English edition. Cross-references throughout the English edition have been systematically updated to match the new pagination.

---

### 4.3 French Edition Inventory of Changes (22 Pages)

| Page | Section | Change Summary | Classification & Details |
| :--- | :--- | :--- | :--- |
| **p. 26** | §4.4.3 | OUTI/OUTD I/O on CRTC 3 & 4 | Added `OUTD` alongside `OUTI`: *"Ce décalage ne se produit pas si l’instruction OUTI/OUTD est utilisée."* |
| **p. 27** | §4.4.4 | Z80 /WAIT line routing | Explicitly notes that the /WAIT line is connected to the Gate Array *or to one of the ASICs of CRTC 3 & 4*. |
| **p. 40** | §7.2 | Code snippet formatting | Joined `RRA` and `JR C,sync_first` on one line. |
| **p. 41** | §7.2 | VSYNC flag detection timing | Corrected comment: `; 5 Le flag a ete détecté au plus tôt, et ce depuis 5 usec (1+1+3)` (was: `; 6`). |
| **p. 47** | §9.1 | Mode 2 byte diagram | Typographic reflow of the 8-pixel mode 2 bit distribution. |
| **p. 54** | §9.3.4.1 | Punctuation | Removed trailing space in HSYNC blanking sentence. |
| **p. 73** | §10 | Mode-to-mode table | Pattern drawing update in vector stream (rendered in `pages-v1.11b/fr_p073.png`). |
| **p. 90** | §11.6.4 | Punctuation | Removed stray period before `(voir chapitre 17.4.2)`. |
| **p. 103** | §12.5 | CRTC 3 & 4 R4 update rules | Grammar and clarity improvements for R4 update at C9 line end and C4 overflow. |
| **p. 127** | §13.7.2 | Spacing | Cleaned up spacing around `C5`. |
| **p. 132** | §14.1 | **HSYNC termination rule** | Corrected critical typo: *"La HSYNC **se termine** dès que le compteur C3l atteint la valeur de R3l"* (was: *"débute"*). Aligns with Round 2 clarification #4. |
| **p. 162** | §16.2.1 | R7.JIT definition | Added explicit qualification: *"Si R7 est programmé avec la valeur de C4 **lorsque C4<>R7**:"*. Clarifies that programming same-value R7 is not a JIT event. |
| **p. 171** | §16.4.1.2 | **CRTC 0 R0/VSYNC normative rules** | Added normative paragraphs for CRTC 0: if R0 changes to 0 on C0=0 at C4=R7, VSYNC starts but C3h is frozen, producing an infinite VSYNC if R3h=1; if R0 changes to 1, VSYNC stops after 2 µs when C0 passes from 1 to 0 (sufficient to trigger GA VSYNC). Previously English-only; aligns with Round 2 clarification #5. |
| **p. 172** | §16.4.4 | CRTC 3 & 4 VSYNC start | Added comma: *"La VSYNC débute lorsque C4=R7, C9=0 et C0=0."* |
| **p. 202** | §19.3.4 | **Interlace activation advice** | Corrected advice: for a full interlace screen, R8 should in principle be modified **when a new frame begins (`C4=C9=0`)**, rather than at `C4=R7`. Aligns with Round 2 clarification #6. |
| **p. 203** | §19.3.4 | Spacing | Minor whitespace cleanup. |
| **p. 209** | §19.5.3 | Spacing | Minor whitespace cleanup. |
| **p. 214** | §19.5.5 | Spacing | Minor whitespace cleanup. |
| **p. 243** | §20.3.3 | Dangling sentence removal | Removed misplaced sentence *"Les CRTC 3 et 4 acceptent de charger VMA' & VMA avec R12/R13 si C4=0 et C0=0"* from the bottom of p. 243. |
| **p. 249** | §21.4 | Typo | Hyphenated *"quelques-uns"*. |
| **p. 250** | §22 | **Cursor register guidance** | Restored guidance note: small values written to unused cursor registers during or outside sync periods could affect other registers in hardware. Aligns with Round 2 clarification #8. |
| **p. 252** | §22.2 | Typo | Spaced *"chapitre 16"*. |
| **p. 282** | §26.1 | **Z80 timing table typo** | Corrected opcode pairing: `LD R,A / LD A,R` (was: `LD R,A / LD R,A`). |

---

### 4.4 English Edition Inventory of Changes

| Old Page | New Page | Section | Change Summary | Classification & Details |
| :--- | :--- | :--- | :--- | :--- |
| **p. 18** | **p. 18** | §4.2 | **U.S. ROM GA interrupt line** | Corrected: *"These 2 more lines cause the interruption to arrive on the **same line** on which the CRTC reports the start of VSYNC, BUT before it"* (was: *"not the same line"*). |
| **p. 24** | **p. 24** | §4.4.2 | **INI definition** | Corrected: *"HL **increment**, B decrement"* (was: *"HL decrement"*). |
| **p. 27** | **p. 27** | §4.4.4 | ASIC /WAIT line | Added: *"or ones of the ASIC’s of CRTC 3 & 4"*. Text reflows through p. 29. |
| **p. 30** | **p. 30** | §7.2 | VSYNC sync constant | Corrected: `ld hl,19968-21` (was: `19968-23`). Aligns with French edition and Round 2 clarification #1. |
| **p. 31** | **p. 31** | §7.2 | Flag timing comment | Corrected comment: `; 5 The flag has been detected at the earliest...` (was: `; 6`). |
| **p. 66** | **p. 67** | §10.3.1.2 | **CRTC 1 C9 counting condition** | Corrected: *"Otherwise, if **C9 <> R9**, then C9 is incremented and C4 retains its value"* (was: *"C9 <= R9"*, which incorrectly suppressed C9>R9 overflow). |
| **p. 77** | **pp. 78–79**| §11.6.1 | RFD text reflow | Cleaner paragraph break around VMA update state persistence. |
| **p. 84** | **p. 85** | §12.2.1 | **CRTC 0 RLAL line N write window** | Added line N window: *"R9 and R4 are updated to 0 on this line N **when C0>1** or on the line N+1 when C0<2..."*. Resolves BL-002. |
| **pp. 114–115**| **pp. 115–116**| §13.7.2 | **CRTC 0 last line and adjustment rules** | Restructured and aligned with French text: *"When C0=0, the CRTC programs the increment or reset of C4 for the next line. This reset of C4 is fully effective from C0=2 when the CRTC has determined that it is on a last frame line (C4=R4 and C9=R9) and that there is no additional management after this last line (R5=0, R0)... and the increment of C4 remains programmed when C9=R9 and C4=R4."* Fixed broken cross-reference `Chapter13.6.1`. |
| **p. 122** | **p. 123** | §14.4 | Section heading added | Numbered heading `14.4 HSYNC AND FRAME POSITION` restored (was unnumbered in v1.11). |
| **pp. 124–143**| **pp. 125–144**| §14.5–14.8 | Chapter 14 renumbering | Shifted section numbers: §14.5 Updating R3 during HSYNC (was 14.4); §14.6 Absence of HSYNC (was 14.5); §14.7 HSYNC Start-up (was 14.6); §14.8 HSYNC and Interrupts (was 14.7). |
| — | **pp. 144–145**| §14.9 | **NEW: HSYNC Schematics** | Restored missing section matching French §14.9 with complete chronograms for CRTC 0, 1, 2, 4, 3. Adds page 145 and shifts subsequent pagination. |
| **p. 160** | **p. 161** | §16.2.1 | R7.NJIT heading | Corrected: *"If R7 is programmed before C4=R7:"* (removed extraneous *"with C4"*). |
| **p. 167** | **p. 168** | §16.3 | VSYNC infinite start qualification | Added: *"(if the conditions required for a VSYNC are met, namely C4=R7 and C9=C0=0)"*. |
| **p. 170** | **p. 171** | §16.4.4 | CRTC 3 & 4 VSYNC inhibition | Added: *"while C0>0 **and/or C9>0**, it will not trigger **CRTC** VSYNC"*. |
| **p. 178** | **p. 179** | §17.2.2 | Character line bits | Corrected: *"The bits that determine the block number (Character line) **(C9)** continue to 'participate' in the address"* (removed erroneous *"or C5"*). |
| **p. 190** | **p. 191** | §18.3.2 | **CRTC 0 R6=0 first-line conflict** | Corrected: *"In this situation however, **if R6 is 0 when C0=R1**, the BORDER becomes definitive"* (was: *"since R6 has been updated with 0 at least 1 time"*). Aligns with French p. 191 and Finding IA-3. |
| **p. 206** | **p. 207** | §19.5.2 | **Repeated IVM qualifier** | Added: *"if R9=7 and R8 goes to 3 on C4=1 (C9=0) **on every frame**..."*. Resolves BL-001. |
| **p. 208** | **p. 209** | §19.5.3 | **Type-1 frame start parity** | Added explicit assignment: *"At the beginning of the Frame, **ParityC9=ParityFrame**."* Resolves Round 2 clarification #7. |
| **p. 213** | **p. 214** | §19.5.5 | **CRTC 3 & 4 IVM parity toggle** | Corrected: *"ParityC9 switch between each C4 **if R9 is odd**"* (was: *"even"*). |
| **p. 242** | **p. 243** | §20.3.2 | **CRTC 1 VMA reload recurrence** | Corrected: *"CRTC 1 then loads VMA with R12/R13 **as long as C4=0 and each time C0 returns to 0, regardless of the value of C9**"*. Resolves F11h bilingual discrepancy. |
| **p. 243** | **p. 244** | §20.3.3 | Dangling sentence removal | Removed misplaced CRTC 3 & 4 VMA reload sentence from bottom of page. |
| **p. 250** | **p. 251** | §22 | Layout | Formatting adjustment. |
| **p. 252** | **p. 253** | §23.2, 23.3| Cross-reference fixes | Fixed Chapter 0 $\to$ Chapter 14.4; Chapter 15 $\to$ Chapter 16. |
| **p. 282** | **p. 283** | §26.1 | **Z80 timing table typo** | Corrected: `LD R,A / LD A,R` (was: `LD R,A / LD R,A`). |
| **p. 293** | **p. 294** | §28.1.9 | Cross-reference fix | Corrected pointer to Chapter 21.3.4 (was: 20.3.4). |

---

### 4.5 RTL and Verification Assessment

1. **Faithfulness Validation**:
   - The corrections formally incorporate into the canonical Compendium text the exact model behaviors already implemented in our RTL and pinned in our testbenches:
     - Finding **IA-3** / **BL-036** (CRTC 0 live `R6==0 at C0==R1` conflict, §18.3.2).
     - Finding **IA-2** / **BL-038** (Type-1 `ParityC9 = ParityFrame` at frame start, §19.5.3).
     - Finding **IA-6** (Type-0 R0=1 widening and last-line evaluation, §13.7.2).
     - Finding **F11h** (Type-1 VMA reload recurrence at C0=0 while C4=0, §20.3.2).
     - Finding **BL-001** (Type-0 repeated IVM disturbance on every frame, §19.5.2).
     - Finding **BL-002** (Type-0 RLAL line N write window at C0>1, §12.2.1).
2. **Impact on RTL and Vectors**:
   - No RTL logic changes are required: our simulation models and hardware engines already match the corrected rules.
   - Testbench comments and citations in `sim/plus/asic_video_test.cpp`, `sim/plus/b8_field_test.cpp`, `rtl/CRTC.v`, and `rtl/plus/asic_video.v` have been updated to reflect the new Chapter 14 numbering and the post-p.145 English page shift (+1).

