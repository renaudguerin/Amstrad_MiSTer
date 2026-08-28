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
| **p. 95** | §12.2.3 | **Type-0 $C_0=0$ evaluation timing** | Added explicit silicon evaluation order: at $C_0=0$, the Last Line comparison uses the **updated** value of $R_4$, but the **previous** value of $R_9$ ($R_9$ update occurs too late for this evaluation). |
| **p. 96** | §12.2.3 | Text reflow | Formatting adjustment following p. 95 expansion. |
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

1. **Type-0 $C_0=0$ Evaluation Timing (§12.2.3, p. 95) — Finding F19**:
   - In `rtl/crtc_type0_engine.v`, `type0_c0_r4` uses `type0_r4_at_c0_write ? DI[6:0] : R4_v_total` (immediately catching $R_4$ writes on $C_0=0$).
   - However, `type0_c0_r9` currently uses `type0_r9_at_c0_write ? DI[4:0] : R9_v_max_line;`.
   - ACCC v1.11 explicitly specifies that an $R_9$ update on $C_0=0$ occurs **too late** for the $C_0=0$ Last Line evaluation. Modifying $R_9$ on $C_0=0$ must not affect this evaluation.
   - Action: Documented in `docs/accuracy/f19-type0-c0-timing-todos.md`.
2. **Type-0 IVM Odd-$R_9$ Counting and Exit Panels (§19.8.1, pp. 219, 223–224)**:
   - Our RTL already implements the corrected `If R9.0==1` behavior and post-exit line comparator via Findings F15 and F16. The v1.11 text and table fixes formally validate our RTL implementation and soak hash.
3. **Type-1 Discriminators and Readable Registers (§28.1.1, §28.1.8, §28.1.9, §21.2.2)**:
   - Our RTL already implements the validated register readback and status bit behavior via Findings F17 and F18. The v1.11 errata corrections align the Compendium text with our test vectors.
