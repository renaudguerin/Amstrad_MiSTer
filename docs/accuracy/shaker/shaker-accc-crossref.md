# SHAKER 2.6 to ACCC v1.10 cross-reference (raw)

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).

Dated evidence document, generated 2026-08-19. Do not edit; supersede it with a newer dated
file if the mapping is redone.

**Provenance.** Produced by Gemini 3.7 Flash reading two artifacts directly: the menu
transcriptions in `menu-transcriptions.md` (captured from a running SHAKER 2.6 on an emulator
set to CRTC type 1) and `docs/ACCC1.10-EN.pdf`. The section numbers, page numbers, and quoted
rules below have **not** been independently re-checked against the PDF by a second reader.

**How to use it.** Treat the mapping tables as a reliable index into the Compendium and the
quoted operative rules as accurate but unverified. Before any rule here is turned into RTL or
into a testbench vector, open the cited ACCC page and confirm the quote. The curated,
verified subset lives in `../shaker-module-a-map.md`.

The confidence column is the model's own rating: HIGH means SHAKER's menu wording matches the
ACCC section title or body almost directly, MEDIUM means a topical match inferred from section
contents, LOW means a guess.

---

// Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot (CC BY-NC-ND 4.0).

---

# SHAKER 2.6 to ACCC v1.10 Mapping & Analysis

## Module A Mapping Table

| Menu Key | Verbatim Entry Title | Test Count | Primary ACCC Section | Secondary Section(s) | Confidence | Hardware Behaviour Checked (per ACCC) | CRTC Scope |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **A (1)** | `UPDATE VRAM VS CRTC (79 TST)` | 79 TST | §8 Display, Z80A & Gate Array (pp. 43–44) | §7.1 Principles (p. 37), §4.4.3 Access Delays (p. 26) | HIGH | Checks the exact microsecond timing when Z80 VRAM write instructions (`LD (HL),r`, `LD (aaaa),HL`, `PUSH rr`) become visible relative to the CRTC $C_0$ video address fetch. | All / Gate Array |
| **A (2)** | `SKEW DISP ON R0 RUPTURE (4 TST)` | 4 TST | §19.2 Functions « Skew-DISPTMG » (pp. 193–197) | §19.2.3 Border Delay +1 / +2 (p. 193), §19.2.4 No Condition C0=R1 (p. 195) | HIGH | Checks R8 DISPTMG skew delay bits (`000100xx`, `001000xx`, `001100xx`) to delay or disable Border during horizontal $R_0$ line ruptures. | 0, 3, 4 |
| **A (3)** | `CRTC 2 RVMB (22 TST)` | 22 TST | §17.4.3 CRTC 2 (pp. 183–184) | §13.4.1 Case Study: Vertical Rupture Last Line (p. 118), §12.4.1 Last Line Concept (pp. 95–96) | HIGH | Checks the CRTC 2 "Meow Bug" vertical rupture behaviour where setting $R_1=0$ at $C_0=0$ on the last line executes only a bitwise `AND` instead of `OR` when reloading $\text{VMA}'$ from $R_{12}/R_{13}$. | 2 |
| **A (4)** | `UPDATE CRTC R0 TIMING (17 TST)` | 17 TST | §13.6 R0 Update (pp. 122–123) | §13.7 Special Cases (p. 124), §4.4.3 Access Delays (p. 26) | HIGH | Evaluates the exact sub-microsecond deadlines across `OUT (C),r` vs `OUTI` when reprogramming $R_0$ before, at, or after $C_0=R_0$. | All |
| **A (5)** | `R13 UPDATE IN 4 USEC SCREENS (R0=3) (5 TST)` | 5 TST | §13.8.1 4 µsec Frames (R0=3) (p. 127) | §13.8 Offset According to C0 (p. 126), §20.3 Update Conditions (pp. 242–243) | HIGH | Checks whether updating $R_{13}$ in 4 µs lines ($R_0=3, R_4=0, R_9=0$) updates $\text{VMA}$ on the next $C_0=0$ across all 5 instruction phase alignments. | All |
| **A (6)** | `R13 UPDATE IN 2 USEC SCREENS (R0=1) (5 TST)` | 5 TST | §13.8.2 2 µsec Frames (R0=1) (p. 128) | §13.2.5 Case Study: R0=1 (p. 107), §20.3 Update Conditions (pp. 242–243) | HIGH | Verifies that CRTC 0 enters an un-cancelled 2 µs vertical adjustment line ($C_4=1$) preventing $R_{13}$ updates until 4 µs have elapsed, whereas CRTC 1 updates $\text{VMA}$ every 2 µs. | All |
| **A (7)** | `R13 UPDATE IN 1 USEC SCREENS (R0=0) (5 TST)` | 5 TST | §13.8.3 1 µsec Frames (R0=0) (p. 129) | §13.2.6 Case Study: R0=0 (p. 108), §20.3 Update Conditions (pp. 242–243) | HIGH | Verifies that CRTC 0 freezes $C_9$ and locks $C_4=1$ during $R_0=0$ (blocking all $R_{13}$ updates), whereas CRTC 1 updates $\text{VMA}$ continuously every 1 µs while $C_4=0$. | All |
| **A (8)** | `GATE ARRAY PIXELISATION` | — | §9.1 Pixels (pp. 46–47) | §9.3.1 Graphic Mode General (p. 51), §4.4.4 OUTs Dissection (p. 27) | HIGH | Checks Gate Array pixel serialization, Mode 0/1/2/3 bit-to-pixel decoding, and Mode 2 1-pixel ($0.0625\text{ µs}$) display advance. | Gate Array |
| **A (9)** | `GATE ARRAY INKERISATION (3 TST)` | 3 TST | §9.2 Colours (pp. 48–50) | §9.2.1 Border and Graphic Mode 2 (p. 48), §9.2.2 Speed of Processing (pp. 48–50) | HIGH | Checks Gate Array ink palette write timings across `OUTI`, `OUT (C),r`, and `OUT (n),A`, as well as Mode 2 border parasitic pixel glitches. | Gate Array |
| **A (E)** | `GATE ARRAY MODERISATION` | — | §9.3 Graphic Mode (pp. 51–52) | §9.3.4 Mode Splitting (pp. 53–72), §4.4.4 OUTs Dissection (p. 28) | HIGH | Checks Gate Array graphics mode switching rules, including the requirement for at least a 2 µs HSYNC to latch mode changes. | Gate Array / All |
| **A (R)** | `MODE UPD >HSYNC DELAY< (2.1.0)(3 TST)` | 3 TST | §9.3.1 Graphic Mode General (p. 51) | §9.3.2 CRTC 0, 1, 2 / §9.3.3 CRTC 3, 4 (p. 52), §14.3 HSYNC Gate Array Versus CRTC (p. 132) | HIGH | Checks the Gate Array ~2 µs HSYNC latency window and tests that ASIC CRTC 3/4 delays HSYNC by 1 µs relative to CRTC 0/1/2. | Gate Array / All |
| **A (CAPS)** | `INTERACTIVE TEST MODE X TO Y (16 INTERACTIVE TST)` | 16 INTERACTIVE TST | §9.3.4 Mode Splitting (pp. 53–72) | §9.3.4.3 CRTC 0, 1, 2 : Cooking of Pixels (pp. 57–67), §9.3.4.5 CRTC 4 : Pixel Cooking (pp. 68–72) | HIGH | Interactive visual inspection of mid-line mode splits across all 16 mode pairs ($0\to 0 \dots 3\to 3$), checking residual pixel corruption ("pixel cooking") across Gate Array versions (40007/8 vs 40010 vs 40226). | Gate Array / All |
| **A (TAB)** | `HSYNC START POSITION (4 INTERACTIVE TST)` | 4 INTERACTIVE TST | §14.6 HSYNC Start-Up (pp. 141–142) | §14.8 HSYNC Schematics (pp. 143–144), §9.3.4.2 HSYNC Under the Microscope (pp. 54–56) | HIGH | Interactive calibration testing exact sub-microsecond HSYNC blanking start positions (CRTC 0: 5th pixel; CRTC 1: 6th pixel; CRTC 2: 4th pixel; ASIC: 19th pixel). | All / Gate Array |
| **A (T)** | `R2 UPD DURING & AFTER HSYNC (6 TST)` | 6 TST | §15.3 Updating R2 During HSYNC (pp. 148–151) | §15.3.2 Infinite HSYNC (p. 148), §15.6 CRTC 2 and HSYNC (p. 155) | HIGH | Checks CRTC response to modifying $R_2$ mid-HSYNC, specifically the infinite HSYNC counter overflow bug on CRTC 1–4 vs non-contiguous HSYNC inhibition on CRTC 0. | All |
| **A (Y)** | `R3 UPD DURING HSYNC (8 TST)` | 8 TST | §14.4 Updating R3 During HSYNC (pp. 134–140) | §14.4.4 Zoom on R3.JIT (pp. 138–140), §14.5 Absence of HSYNC (p. 141) | HIGH | Checks $C_{3l}$ counter behaviour when $R_3$ is updated mid-HSYNC, including R3.JIT 0.25 µs termination extension on CRTC 0/1/2 and instant HSYNC cancellation when $R_3=0$ on CRTC 1. | All |
| **A (U)** | `R4 & R9 CHECKING (54 TST)` | 54 TST | §10.3 Counting Rules (pp. 74–79) | §12 Counting : Register R4 (pp. 92–101), §10.3.1–10.3.4 (pp. 75–77) | HIGH | Validates $C_4$ character and $C_9$ scanline counter incrementing, overflow to 31/127, and line-to-line rupture rules ($C_0<2$ deadline on CRTC 0 vs CRTC 1/2/3/4 logic). | All |
| **A (I)** | `VSYNC CONDITIONS (413 TST)` | 413 TST | §16.4 Conditions to Consider (pp. 168–170) | §16.2 VSYNC-CRTC Versus VSYNC-Gate Array (pp. 159–166), §15.4 VSYNC Consideration During HSYNC (pp. 152–154) | HIGH | Exhaustive suite checking $C_4=R_7$ VSYNC generation, VSYNC blocking when $C_0<2$ on CRTC 0, Ghost VSYNC on CRTC 2 during HSYNC, and Gate Array $V_{26}$ composite sync tracking. | All |
| **A (O)** | `R1 STORIES (8 TST)` | 8 TST | §17 Display : Register R1 (pp. 175–187) | §17.2 Displays According to R1 (pp. 177–181), §17.6 Interline Border (pp. 185–186) | HIGH | Tests horizontal display enable ($R_1$) behaviour, character repetition when $R_1>R_0$ due to lack of $\text{VMA}'$ reload, and 0.5 µs border glitch generation on CRTC 0 and 2. | All |
| **A (P)** | `R6 STORIES (13 TST)` | 13 TST | §18 Display : Register R6 (pp. 188–191) | §18.2 Border R6 Deadlines and Priorities (pp. 188–189), §18.3 R6 Conflicts (pp. 189–191) | HIGH | Tests vertical display enable ($R_6$) priority over $R_1$, the $C_4=R_6=C_9=0$ alternating byte border conflict on CRTC 0/2, and $R_6=0$ non-definitive split-border on CRTC 1. | All |
| **A (COPY)**| `CRTC 2 OFFSET` | — | §17.4.3 CRTC 2 (pp. 183–184) | §20.3.3 CRTC 2 (p. 243), §13.4.1 Case Study: Vertical Rupture Last Line (p. 118) | HIGH | Tests CRTC 2 specific offset reloading rules requiring $C_0$ to reach $R_1$ on the last line for $\text{VMA}'$ transfer to occur. | 2 |

---

## Module B Mapping Table

| Menu Key | Verbatim Entry Title | Test Count | Primary ACCC Section | Secondary Section(s) | Confidence | Hardware Behaviour Checked (per ACCC) | CRTC Scope |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **B (1)** | `INTERLACE C4/C9 COUNTERS (Y=PARITY CRT 0/2/3/4:0)(Z/X)=R9 BASE (07)` | — | §19.8 Counting in Interlace Videomode (pp. 219–240) | §19.5 Parity (pp. 205–215), §19.4 Vertical Interlace Programming (pp. 203–204) | HIGH | Validates $C_4$ and $C_9$ raster counting algorithms in Interlace Video Mode (IVM, $R_8=3$) across odd/even frame parities and base $R_9$ values. | All |
| **B (2)** | `INTERLACE CRTC 2 C9 STRANGER THING` | — | §19.8.3 CRTC 2 (pp. 226–234) | §19.4.3 CRTC 2 (pp. 203–204), §19.5.4 CRTC 2 (p. 212) | HIGH | Checks CRTC 2 internal $C_{9.\text{IVM}}$ display counter operation, which resets twice per $C_4$ character (at $R_9$ and $R_9/2$) and produces extra lines on odd frames when $R_9$ is even. | 2 |
| **B (3)** | `FAKE VSYNC ON CRTC 2` | — | §7.3 Fake VSYNC (pp. 41–42) | §15.4.4 CRTC 2 (pp. 153–154), §29.1.3 Bug PPI Port B (p. 294) | HIGH | Tests whether reprogramming PPI 8255 Port B as output to force bit 0 high can bypass CRTC 2 Ghost VSYNC suppression. | 2 (and PPI / Gate Array) |
| **B (4)** | `CRTC 2 FIND C0 MIN` | — | §13.4 CRTC 2 (pp. 117–118) | §13.8 Offset According to C0 (pp. 126–129), §15.5.2 CRTC 2 (p. 155) | HIGH | Determines the minimum allowable horizontal line length ($R_0$) and $R_1/R_2$ constraints before CRTC 2 counter progression or display crashes. | 2 |
| **B (5)** | `CRTC 2 RLAL` | — | §12.4.2 Case Study: Line-to-Line Rupture (R.L.A.L.) (pp. 97–100) | §12.4.1 Last Line Concept (pp. 95–96), §10.3.3 CRTC 2 (p. 76) | HIGH | Validates the CRTC 2 line-to-line rupture sequence requiring temporary invalidation of the Last Line state during HSYNC (`OUT R9,1` then `OUT R9,0`) to prevent $C_4$ overflow. | 2 |
| **B (6)** | `CRTC 1 BUG OUTI R0` | — | §13.3 CRTC 1 (p. 113) | §13.7.1.1 R0 Update: OUTI (p. 124), §4.4.3 Access Delays (p. 26) | HIGH | Tests CRTC 1 premature $R_0$ latching bug when modified via `OUTI`, where $C_0$ is evaluated against the new $R_0$ immediately on the 5th µs, leading to $C_0$ overflow up to 255. | 1 |
| **B (7)** | `CRTC 0 BUG OVF C4` | — | §13.7.2 CRTC 0 (pp. 124–126) | §13.7.2.1-13.7.2.2 (pp. 125–126), §13.2.1 The First 3 Microseconds (pp. 103–104) | HIGH | Verifies the CRTC 0 bug where resizing $R_0$ from 1 to $>1$ at $C_0=1$ while $C_9=R_9$ engages permanent vertical adjustment, triggering an unconditional $C_4$ counter overflow. | 0 |
| **B (9)** | `INTERLACE VM (27 TST) (Y PARITY CRT 0/2/3/4)` | 27 TST | §19.3.2.2 Interlace Sync & Video Mode (pp. 200–201) | §19.8 Counting in Interlace Videomode (pp. 219–240), §19.5 Parity (pp. 205–215) | HIGH | Comprehensive 27-test validation of Interlace Video Mode ($R_8=3$) counting, frame parity alternation, and VRAM addressing across CRTC types. | All |
| **B (O)** | `CRTC 1-A OR 1-B?` | — | §11.6 Rupture for Dummies (R.F.D.) on CRTC 1 (pp. 87–89) | §11.6.2 IVM ON/OFF (pp. 88–89), §4.2 Note 6 (p. 21) | HIGH | Identifies CRTC 1 sub-variants by checking whether writing $R_5=\&10$ during an RFD deactivates parity evaluation in the $C_9=R_9$ test (type 1-B) or maintains it (type 1-A). | 1 (1-A vs 1-B) |
| **B (RETURN)** | `R5 STORIES` | — | §11 Counter : Register R5 (pp. 80–91) | §11.2 Counting in Vertical Adjustement (pp. 81–84), §11.3 Updating R5 During an Adjustment (pp. 85–86) | HIGH | Validates vertical total adjust ($R_5$) mechanics, including separate $C_5$ counter on CRTC 1/2 vs $C_9$ reuse on CRTC 0/3/4 and adjustment cutoff rules. | All |
| **B (F0)** | `BOUNGA!` | — | §24 Tips and Tricks (pp. 253–268) | §24.1 R12/R13 Updates (pp. 253–254), §25 A Brief History of Fixed Time (pp. 269–280) | MEDIUM | Demonstrates a full-screen vertical bounce/rupture torture demo combining dynamic $R_0/R_4/R_5/R_9$ register manipulation and fixed-time Z80 loops. | All |
| **B (CAPS)** | `RVNI LTD` | — | §13.2.7 Case Study: Vertical Rupture Last Line (R.V.L.L.) (pp. 109–112) | §13.3.1 Case Study : Vertical Invisible Rupture (R.V.I.) (pp. 113–116), §3.2 Acronyms (p. 15) | HIGH | Tests constrained "Rupture Verticale Non-Interlace" / Invisible Vertical Ruptures (RVI) creating hidden lines during HSYNC to access arbitrary $C_9$ rasters. | 0, 1, 3, 4 |
| **B (P)** | `ANALYZER / FORCED STAB CRTC 0 R0=0 (4 CONF)` | 4 CONF | §13.2.6 Case Study: R0=0 (p. 108) | §13.2.4 Freeze of C9 (pp. 105–106), §13.8.3 1 µsec Frames (R0=0) (p. 129) | HIGH | Analyzes CRTC 0 forced frame stabilization under $R_0=0$, verifying $C_9$ freeze, single $C_4$ increment hiccup, and recovery behaviour when $R_0>2$. | 0 |
| **B (R)** | `INTERRUPT DELAY FROM R2 (18 CALC)` | 18 CALC | §27.6 CRTC & Interrupts (pp. 286–288) | §27.6.2 CRTC 0, 1, 2 / §27.6.5 CRTC 3, 4 (p. 287), §14.7 HSYNC and Interruptions (p. 142) | HIGH | Measures the exact microsecond delay from $C_0=R_2$ to Z80 interrupt execution across different $R_3$ values, verifying that ASIC CRTC 3/4 interrupts occur 1 µs later than on CRTC 0/1/2. | All |
| **B (U)** | `CRT 000 (CRTC 0)` | — | §13.2.6 Case Study: R0=0 (p. 108) | §13.8.3 1 µsec Frames (R0=0) (p. 129), §10.3.1.2 Exception to General Case (p. 75) | HIGH | Validates extreme CRTC 0 state with $R_0=0, R_4=0, R_9=0$ ("000" frame), checking frozen counter states and un-disarmable vertical adjustment. | 0 |
| **B (I)** | `R3 JIT (8 TST)` | 8 TST | §14.4.4 Zoom on R3.JIT (pp. 138–140) | §14.4.4.1–14.4.4.4 (pp. 139–140), §14.4 Updating R3 During HSYNC (p. 134) | HIGH | Tests R3.JIT HSYNC truncation by rewriting $R_3=C_{3l}$ during HSYNC, verifying that HSYNC stops 0.25 µs after normal end on CRTC 0/1/2 and fails on ASIC 3/4. | 0, 1, 2 (vs 3, 4) |
| **B (T)** | `R3 JIT INTERACTIVE MODE ANALYZER` | — | §14.4.4 Zoom on R3.JIT (pp. 138–140) | §14.8 HSYNC Schematics (pp. 143–144), §9.3.4 Mode Splitting (pp. 53–56) | HIGH | Interactive oscilloscope-like analyzer for inspecting real-time R3.JIT HSYNC truncation and its visual impact on C-HSYNC width and monitor deflection. | 0, 1, 2 (vs 3, 4) |
| **B (S)** | `CRTC 1 : BE00 CHECK` | — | §21.3.3 CRTC 1 (p. 247) | §21.3.1 General (p. 246), §28.1.8 Via Status Register &BE00 (p. 293) | HIGH | Verifies CRTC 1 Status register read at port `&BE00`, checking bit 5 transition on $C_0=R_0$ reflecting $C_4=R_6$ vertical border state. | 1 (vs 0, 2, 3, 4) |
| **B (CTRL)** | `R5 SCANNER / (COPY) R5 T2` | — | §11.2 Counting in Vertical Adjustement (pp. 81–84) | §11.3 Updating R5 During an Adjustment (pp. 85–86), §11.4 R5 Update Before an Adjustment (p. 86) | HIGH | Scans and tests all possible $R_5$ vertical adjust values (0 to 31) and update timings across lines and frames. | All |
| **B (TAB)** | `R5 BENCH (INTERACTIVE)` | INTERACTIVE | §11 Counter : Register R5 (pp. 80–91) | §11.2 Counting in Vertical Adjustement (pp. 81–84), §11.3 Updating R5 During an Adjustment (pp. 85–86) | HIGH | Interactive tool for real-time manipulation of $R_5$, observing vertical frame height adjustment lines and counter evolution on-screen. | All |
| **B (0)** | `VERTICAL SCROLL SUB-PIXEL 1/8,1/16,1/32,1/64,1/128` | — | §16.6 Limitless VSYNC ! (pp. 172–174) | §7.2 VSYNC Synchronization (pp. 38–40), §16.2.1 VSYNC Area Display (p. 159) | HIGH | Demonstrates sub-pixel vertical scrolling down to 1/64th (and 1/128th) of a pixel by modulating the $C_0$ position of VSYNC trigger to shift monitor beam flyback. | All (Monitor deflector) |

---

## Module C Mapping Table

| Menu Key | Verbatim Entry Title | Test Count | Primary ACCC Section | Secondary Section(s) | Confidence | Hardware Behaviour Checked (per ACCC) | CRTC Scope |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **C (1)** | `CRTC 1 : RFD & PARITY STORY` | — | §11.6 Rupture for Dummies (R.F.D.) on CRTC 1 (pp. 87–90) | §11.6.1 RFD and Frame Parity (p. 88), §19.5.3 CRTC 1 (p. 208) | HIGH | Verifies the CRTC 1 RFD bug triggered by updating $R_5>0$ on $C_0=R_0$ (when $R_5=0$), activating frame parity in $C_9=R_9$ and allowing line-by-line offset changes. | 1 |
| **C (2)** | `CRTC 1 : R8 IVM ON ODD C9` | — | §19.5.3 CRTC 1 (pp. 208–211) | §19.8.2 CRTC 1 (p. 225), §11.6.2 IVM ON/OFF (p. 88) | HIGH | Tests CRTC 1 parity and $C_9$ counter update when $R_8$ is set to IVM ($R_8=3$) on an odd $C_9$ scanline. | 1 |
| **C (3)** | `CRTC 1 : PARITY SWITCH STATUS` | — | §19.5.3 CRTC 1 (pp. 208–211) | §21.3.3 CRTC 1 (p. 247), §19.8.2 CRTC 1 (p. 225) | HIGH | Checks CRTC 1 `ParityFrame` toggling at $C_4=C_9=C_0=0$ and `ParityC9` toggling between $C_4$ characters when $R_9$ is even. | 1 |
| **C (4)** | `CRTC 1 : IVM ON/OFF` | — | §11.6.2 IVM ON/OFF (pp. 88–89) | §19.5.3 CRTC 1 (pp. 209–211), §11.6.1 RFD and Frame Parity (p. 88) | HIGH | Validates locking CRTC 1 frame parity to EVEN by activating and deactivating IVM (`OUT R8,3` then `OUT R8,0`) on an even $C_9$ line with odd $R_9$. | 1 |
| **C (5)** | `CRTC 0.2 : PARITY CHECK SELECT` | — | §19.5.2 CRTC 0 (pp. 205–207) | §19.5.4 CRTC 2 (p. 212), §19.8.1 CRTC 0 / §19.8.3 CRTC 2 (pp. 219–234) | HIGH | Checks parity anticipation via `ParityR6` ($C_4=R_6$) and parity identification methods on CRTC 0 and CRTC 2. | 0, 2 |
| **C (6)** | `CRTC 2 : C9.IVM SWITCH` | — | §19.8.3 CRTC 2 (pp. 226–234) | §19.5.4 CRTC 2 (p. 212), §19.4.3 CRTC 2 (pp. 203–204) | HIGH | Checks CRTC 2 real-time display address switching using the independent $C_{9.\text{IVM}}$ counter when IVM mode is enabled mid-line. | 2 |
| **C (7)** | `CRTC 2 : LAST LINE COND (OPEN TO OTHER CRTC'S)` | — | §12.4.1 Last Line Concept (pp. 95–96) | §12.4.2 Case Study: Line-to-Line Rupture (pp. 97–100), §12.2 CRTC 0 (pp. 92–94) | HIGH | Tests CRTC 2 Last Line state arming rules at $C_0=0$ and mid-line, comparing with CRTC 0 and other types. | 2 (and All) |
| **C (8)** | `CRTC 2 : ADD LINE ON PARITY BUG` | — | §19.6.3 CRTC 2 (pp. 216–217) | §19.5.4 CRTC 2 (p. 212), §11.9 Interlace Adjustment Line (p. 91) | HIGH | Verifies CRTC 2 bug where enabling IVM on the first line of an odd frame turns line 0 into an additional line, stretching the frame by $R_0$ µs. | 2 |
| **C (9)** | `CRTC 2 : ADD LINE RQ & TRIGGER` | — | §19.6.3 CRTC 2 (pp. 216–217) | §11.9 Interlace Adjustment Line (p. 91), §19.6.1 CRTC 0 (p. 216) | HIGH | Tests additional interlace line request, triggering at the end of even frames, and cancellation if $R_8=0$ during the additional line on CRTC 2. | 2 |
| **C (T)** | `CRTC 2 : VMA' ON R1=0 (OPEN TO OTHER CRTC'S)` | — | §17.4.3 CRTC 2 (pp. 183–184) | §20.3.3 CRTC 2 (p. 243), §17.4.1–17.4.2 (pp. 182–183) | HIGH | Tests CRTC 2 bug where writing $R_{12}/R_{13}$ at $C_0=0$ when $R_1=0$ performs only a bitwise `AND` against $\text{VMA}'$, comparing with other CRTCs. | 2 (and All) |
| **C (RETURN)**| `CRT2 : GHOST VSYNC VS LAST LINE (OPEN TO OTHER CRTC'S)` | — | §15.4.4 CRTC 2 (pp. 153–154) | §15.6 CRTC 2 and HSYNC (pp. 155–156), §16.4.3 CRTC 2 (pp. 169–170) | HIGH | Tests CRTC 2 interaction between Ghost VSYNC (triggered when $C_4=R_7$ during HSYNC) and Last Line evaluation/overflow of $C_4$ at frame end. | 2 (and All) |
| **C (E)** | `ALL : ADD LINE R5 ON LAST LINE` | — | §11.4 R5 Update Before an Adjustment (p. 86) | §11.2 Counting in Vertical Adjustement (pp. 81–85), §13.2.1 (p. 103) | HIGH | Checks vertical adjustment line generation when $R_5$ is updated on the last line of the frame across all CRTC types ($C_0<3$ deadline on CRTC 0 vs $C_0=R_0$ on CRTC 1). | All |
| **C (P)** | `ALL : ADD LINE R8` | — | §19.6 Additional Interlace Line (pp. 216–217) | §11.9 Interlace Adjustment Line (p. 91), §19.3.1 General (pp. 198–199) | HIGH | Checks additional interlace line generation (from $R_8=1$ or $R_8=3$) at the end of even frames across all CRTC models. | All |
| **C (S)** | `ALL : R5 AND INTERLACE MANAGEMENT` | — | §11.8 Adjustment During Interlace (p. 91) | §11.9 Interlace Adjustment Line (p. 91), §19.6 Additional Interlace Line (pp. 216–217) | HIGH | Validates simultaneous execution of $R_5$ vertical adjustment lines and $R_8$ interlace additional lines across all CRTC types. | All |
| **C (O)** | `ALL : INTERLACE VSYNC NIGHTMARE` | — | §19.7 Mid-VSYNC (p. 218) | §16.5 Delayed VSYNC (p. 171), §19.5.2–19.5.5 (pp. 205–215) | HIGH | Evaluates VSYNC timing and 1/2 line delay (Mid-VSYNC on $C_0=R_0/2$) or full 1-line delay during Interlace modes across all CRTCs. | All |
| **C (Y)** | `ALL : CRTC 3/4 PARITY` | — | §19.5.5 CRTC 3 & 4 (pp. 213–215) | §19.8.4 CRTC 3, 4 (pp. 235–240), §19.6.4 CRTC's 3 & 4 (p. 217) | HIGH | Validates ASIC CRTC 3 and 4 parity management (`ParityFrame` and `ParityC9` states, line balancing when $R_9$ is odd, and VSYNC delay). | 3/4 (and All) |
| **C (R)** | `CRTC 0 : R9/R4 UPD LAST LIMIT` | — | §10.3.1.2 Exception to the General Case (p. 75) | §12.2 CRTC 0 (pp. 92–94), §13.2.1 The First 3 Microseconds (pp. 103–104) | HIGH | Checks CRTC 0 $C_0<2$ deadline for evaluating $C_4==R_4$ and $C_9==R_9$ to validate the Last Line state and disarm vertical adjustment. | 0 |

---

## Module D Mapping Table

| Menu Key | Verbatim Entry Title | Test Count | Primary ACCC Section | Secondary Section(s) | Confidence | Hardware Behaviour Checked (per ACCC) | CRTC Scope |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **D (U)** | `CRTC 3/4 : STATUS (DEADLOCK MAY HAPPEN IF BAD EMULATION)` | — | §21.3.4 CRTC 3, 4 (pp. 248–249) | §21.3.4.1 Definition of Status 1 / §21.3.4.2 Status 2 (pp. 248–249), §28.1.10 (p. 293) | HIGH | Tests reading ASIC CRTC 3/4 Status 1 and Status 2 registers via $R_{10}$ and $R_{11}$ at `&BE00`/`&BF00` ($1\text{ µs}$ transient flags like $C_0=R_0$, $C_0=R_0/2$, $C_4=R_4$). | 3/4 |
| **D (I)** | `SHAKER KILLER 2 (WARNING : NOT RELIABLE ON CRTC 1)` | — | §27.7.2 Reliability of Interruptions (pp. 289–291) | §27.7.1 R52 in Time ... (p. 288), §27.6 CRTC & Interrupts (pp. 286–288) | HIGH | Torture test for Z80 interrupt jitter and HSYNC trailing-edge timing on the 1/16 MHz Gate Array clock (unreliable on CRTC 1). | All / Gate Array |
| **D (R)** | `VSYNC TORTURE (LOCK MECHANISM)` | — | §16.3 VSYNC Protection (p. 167) | §16.4.1.1 R7 Update / Blocked VSYNC (p. 168), §28.1.1 Via C4/C9 Overflow (p. 292) | HIGH | Validates CRTC anti-infinite VSYNC protection mechanisms (ignoring $C_4=R_7$ during VSYNC, checking equality change upon $C_9$ loop, and ASIC 3/4 lack of 2nd protection). | All |
| **D (T)** | `VSYNC GATE ARRAY` | — | §16.2 VSYNC-CRTC Versus VSYNC-Gate Array (pp. 159–166) | §16.2.1 VSYNC Area Display / §16.2.3 C-SYNC Algorithm (pp. 159–165), §16.1 General (p. 158) | HIGH | Validates Gate Array $V_{26}$ counter operation, C-VSYNC signal duration (4 HSYNCs between $V_{26}=2$ and $V_{26}=6$), and 26-line black blanking window. | Gate Array / All |
| **D (H)** | `HSYNC GATE ARRAY` | — | §14.3 HSYNC Gate Array Versus CRTC (pp. 132–134) | §16.2.2 Monitor C-SYNC Signal / §16.2.3 Algorithm (pp. 162–165), §14.8 HSYNC Schematics (pp. 143–144) | HIGH | Checks Gate Array $H_{06}$ counter, active-low C-HSYNC pulse generation (4 µs max, starting 2 µs after CRTC HSYNC), and XNOR composite sync logic. | Gate Array / All |
| **D (1)** | `CSYNC4 VS 2xCSYNC2` | — | §16.2.2 Monitor C-SYNC Signal (pp. 162–165) | §16.2.4 Tolerances (p. 165), §14.3 HSYNC Gate Array Versus CRTC (pp. 132–134) | HIGH | Compares monitor horizontal deflection response under one standard 4 µs C-HSYNC pulse versus two 2 µs HSYNC pulses on the same line. | Gate Array / All |
| **D (2)** | `R2.JIT >> NO CSYNC UPD` | — | §14.6.1 CRTC 0, 1, 2 (pp. 141–142) | §15.3 Updating R2 During HSYNC (pp. 148–151), §9.3.4.2 HSYNC Under the Microscope (pp. 54–56) | HIGH | Verifies that delaying HSYNC start via R2.JIT (altering blanking cutoff by $0.1875\text{--}0.25\text{ µs}$) does not shift the monitor C-SYNC synchronization pulse. | 0, 1, 2 |
| **D (3)** | `2xCSYNC RELATIVE` | — | §14.3 HSYNC Gate Array Versus CRTC (pp. 132–133) | §16.2.4 Tolerances (p. 165), §15.3.1 General (p. 148) | HIGH | Checks monitor synchronization stability when generating two C-SYNC pulses per line with variable relative spacing. | Gate Array / All |
| **D (4)** | `CSYNC MULTIPLES` | — | §16.2.4 Tolerances (p. 165) | §14.3 HSYNC Gate Array Versus CRTC (p. 132), §15.3.1 General (p. 148) | HIGH | Checks behaviour when multiple HSYNC pulses are generated per line and verifies monitor sync lock threshold ($>11\text{--}12\text{ µs}$ required). | Gate Array / All |
| **D (5)** | `WIP (SOME R3.JIT)` | — | §14.4.4 Zoom on R3.JIT (pp. 138–140) | §14.3 Table of C-HSYNC Durations (p. 133), §9.3.4.4 Pixel Cooking with R3.JIT (pp. 64–67) | HIGH | Work-in-progress test exploring advanced R3.JIT variations, shifting HSYNC end by 0.25 µs to fine-tune C-HSYNC duration. | 0, 1, 2 |
| **D (6)** | `HARDWARE SCROLL 1 PIXEL MODE 1/0 (NO BUFFERING)` | — | §14.3 HSYNC Gate Array Versus CRTC (pp. 132–133) | §133 Citation ("module D test 6"), §15.7 The Right Moment... (pp. 156–157) | HIGH | Demonstrates smooth 1-pixel Mode 1 ($2\text{ Mode 2 pixels}$) hardware scrolling without double buffering by modulating $R_2$ and $R_{3l}$ (C-HSYNC phase shift). | All |
| **D (7)** | `R2 OSCILLATION STORY` | — | §14.3 HSYNC Gate Array Versus CRTC (p. 133) | §15.7 The Right Moment... (pp. 156–157), §15.3 Updating R2 During HSYNC (p. 148) | HIGH | Checks line-by-line $R_2$ modulation (alternating C-SYNC position by 16+ Mode 2 pixels) to create visual raster oscillation/wobble effects. | All |
| **D (8)** | `NO HSYNC FOR XX LINES` | — | §14.5 Absence of HSYNC (p. 141) | §12.4.1 / §12.4.2 Absence of HSYNC (p. 97), §27.1 Interrupts General (p. 283) | HIGH | Checks CRTC, Gate Array $R_{52}$ interrupt counter, and monitor behaviour when HSYNC is suppressed ($R_3=0$ on CRTC 0/1 or $R_2>R_0$) across multiple lines. | All |
| **D (9)** | `CRTC 1 : RFD ROUND 2` | — | §13.7.1.2 R0 Update: OUT(C),R8 (p. 124) | §11.6 Rupture for Dummies (R.F.D.) on CRTC 1 (pp. 87–90), §11.6.3 (p. 90) | HIGH | Tests secondary CRTC 1 RFD triggers, specifically enlarging $R_0$ at $C_0=R_0$ on the last frame line while modifying $R_4/R_9$ during the extension. | 1 |
| **D (E)** | `CRTC 1 : OFS UPD IN ADD MANAGEMENT` | — | §11.2.4 CRTC 1 (p. 84) | §17.4.2 CRTC 1 (p. 182), §20.3.2 CRTC 1 (p. 242) | HIGH | Verifies CRTC 1 display offset updating during vertical adjustment lines when $C_4=0$ before adjustment begins ($\text{VMA}$ updated on each $C_9$ line of $C_4=1$). | 1 |
| **D (CAPS)**| `CRTC 1-OUTI R0 AUTOPSY` | — | §13.3 CRTC 1 (p. 113) | §13.7.1.1 R0 Update: OUTI (p. 124), §4.4.3 Access Delays (p. 26) | HIGH | Diagnostic autopsy of CRTC 1 premature $R_0$ update via `OUTI`, tracing cycle-by-cycle comparison against $C_0$ and resulting counter overflow. | 1 |
| **D (P)** | `CRTC 2 : VMA' BUG` | — | §17.4.3 CRTC 2 (pp. 183–184) | §20.3.3 CRTC 2 (p. 243), §18.3.2 CRTC 0, 2 (p. 190) | HIGH | Verifies CRTC 2 bitwise `AND` bug when reloading $\text{VMA}'$ from $R_{12}/R_{13}$ at $C_0=0$ when $R_1=0$ (missing `OR` step). | 2 |
| **D (S)** | `SPLITBORDER POS` | — | §11.7 R6 and Vertical Adjustment (p. 90) | §19.2.1 Border ON (p. 193), §18.2.3 CRTC 1 (p. 189) | HIGH | Validates split-border positioning and non-definitive border activation using $R_6=0$ on CRTC 1 vs $R_8$ Skew DISPTMG Border ON (`001100xx`) on CRTC 0/3/4. | All (CRTC 1 vs 0, 3, 4) |
| **D (COPY)**| `RESET CPC` | — | §4.1 General (pp. 16–17) | §17 Note 2 (CRTC Low ROM initialization) | HIGH | Utility action executing a software reset of the Amstrad CPC system. | All |

---

## Module E Mapping Table

| Menu Key | Verbatim Entry Title | Test Count | Primary ACCC Section | Secondary Section(s) | Confidence | Hardware Behaviour Checked (per ACCC) | CRTC Scope |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **E (1)** | `R5 STORIES 2ND ROUND` | — | §11 Counter : Register R5 (pp. 80–91) | §11.3 Updating R5 During an Adjustment (pp. 85–86), §11.3.2 CRTC 1 (pp. 85–86) | HIGH | Advanced testing of $R_5$ vertical adjustment corner cases, including mid-adjustment $R_5$ modifications, $R_5=0$ non-reset bug on CRTC 1, and $C_9/R_5$ counting on CRTC 0. | All |
| **E (2)** | `CRTC 1 VMA TRT C4=R4=0 ON ADJ LINE C4=1 ON NO-EXTENT FRAME` | — | §11.2.4 CRTC 1 (p. 84) | §17.4.2 CRTC 1 (p. 182), §20.3.2 CRTC 1 (p. 242) | HIGH | Validates CRTC 1 video address treatment when $C_4=R_4=0$ and vertical adjustment begins, allowing $\text{VMA}$ to update directly from $R_{12}/R_{13}$ on each line of $C_4=1$ without $\text{VMA}'$ reload. | 1 |
| **E (3)** | `CRTC 0 C4/C9 COUNTER LOGIC BUG` | — | §10.3.1.2 Exception to General Case (pp. 75–76) | §11.2.2 CRTC 0 (pp. 81–82), §12.2 CRTC 0 (pp. 92–94) | HIGH | Validates CRTC 0 counter logic bug where omitting a dedicated $C_5$ counter causes $C_9/C_4$ comparison sequencing across $C_0=0, 1, 2$ to govern Last Line and vertical adjustment. | 0 |

---

# Specific Question Answers

## Q1. Module A Entries (5), (6), (7) — R13 UPDATE in Reduced Screens ($R_0 = 3, 1, 0$)

### Operative Rules & Differences between CRTC 0 and CRTC 1

#### 1. Case $R_0 = 3$ (4 µs Frames / Lines) — Section 13.8.1 (p. 127), §20.3 (p. 242)
* **CRTC 0 & CRTC 1 Behaviour:**
  * Both controllers behave identically.
  * In a 4 µs frame ($R_0=3, R_4=0, R_9=0$), counter $C_0$ counts through $0, 1, 2, 3$.
  * Because $C_0$ reaches and exceeds 2, CRTC 0 successfully passes its internal $C_0=2$ evaluation check, which disarms the default vertical adjustment mechanism (since $R_5=0$).
  * Consequently, $C_4$ remains 0 on each 4 µs line ($C_4=0, C_9=0$).
  * Whenever $R_{13}$ is updated by the Z80 prior to $C_0=0$ of the subsequent 4 µs line, $\text{VMA}$ is loaded with the new $R_{12}/R_{13}$ value on that $C_0=0$.
  * **Operative Rule:** On both CRTC 0 and CRTC 1, $\text{VMA}$ is updated every 4 µs on the next $C_0=0$.

#### 2. Case $R_0 = 1$ (2 µs Frames / Lines) — Section 13.8.2 (p. 128), §13.2.5 (p. 107), §20.3.1–20.3.2 (p. 242)
* **CRTC 0 Operative Rules:**
  * *"The event C0 = R0 after 2 µsec leaves C4 = 1 for the 2nd period of 2 µsec, which represents a vertical adjustment 'not canceled' (because C0 is never equal to 2)"* (§13.8.2, p. 128).
  * *"When C0=0 (or C0=1), the CRTC assesses whether it is on the last line, and if so, arms an internal flag by default to trigger vertical adjustment... When C0=2, the CRTC assesses the conditions for disarming this vertical adjustment mechanism (in particular by testing the value of R5)"* (§13.2.5, p. 107).
  * *"Consequently : If C0 never exceeds 1, and R4=R9=0, the CRTC generates a '1 line' vertical adjustment every 'frame' when C4=R4 and C9=R9... When R4=R9=0, each 'line' of 2 µsec is therefore immediately followed by a 'line' of 2 µsec for which C9=0 and C4=1"* (§13.2.5, p. 107).
  * *"Once this 'additional' line of 2 µsec is completed, C4 and C9 reach 0 and the offset is updated. In this situation, the R12/R13 registers can therefore be considered 'only' after 4 µsec, whereas this is possible every 2 µsec on a CRTC’s 1, 2, 3 or 4"* (§13.2.5, p. 107).
* **CRTC 1 Operative Rules:**
  * CRTC 1 does not rely on a $C_0=2$ cycle to disarm vertical adjustment.
  * $C_4$ remains 0 on every line ($C_4=0$ throughout, §13.8.2, p. 128).
  * *"On CRTC 1, VMA is loaded with R12/R13 while C4=0"* (§20.3.2, p. 242).
  * $\text{VMA}$ is updated from $R_{12}/R_{13}$ at every $C_0=0$ boundary (every 2 µs).
* **Difference:** CRTC 0 alternates $C_4=0$ and $C_4=1$, meaning $R_{13}$ updates are accepted only every 4 µs (when $C_4=0$). CRTC 1 keeps $C_4=0$ and accepts $R_{13}$ updates every 2 µs.

#### 3. Case $R_0 = 0$ (1 µs Frames / Lines) — Section 13.8.3 (p. 129), §13.2.6 (p. 108), §13.3 (p. 113), §20.3.1–20.3.2 (p. 242)
* **CRTC 0 Operative Rules:**
  * *"R12 / R13 cannot be considered until C4 and C9 both go back to 0"* (§13.8.3, p. 129).
  * *"C9 is frozen: Since C0 never reaches 1, C9 is no longer managed. If, for example, it was equal to 3 when R0 went to 0, it will remain at 3 regardless of the value of R9, as long as R0=0"* (§13.2.6, p. 108).
  * *"C4's last hiccup: If C9=R9 on C0=0 (when R0 goes to 0) then C9 is no longer managed, but C4 will however be incremented regardless of the value of R4. C4 is incremented without C9 returning to 0. Magical ! After that, no more counter will be managed except C0"* (§13.2.6, p. 108).
  * *"The CRTC is in additional management, but C9 is frozen... All counters are frozen"* (§13.2.6, p. 108).
  * Because $C_4$ is locked at 1, the condition $C_4=0 \land C_0=0$ is never satisfied. As a result, $R_{12}/R_{13}$ updates are completely ignored by $\text{VMA}$ while $R_0=0$.
* **CRTC 1 Operative Rules:**
  * *"If R0 is 0, then C9 and R4 continue to be managed normally"* (§13.3, p. 113).
  * *"If the conditions are met, it is possible to change the offset on frames of 1 µs (R0=0). On CRTC 1, VMA is loaded with R12/R13 while C4=0"* (§20.3.2, p. 242).
  * $C_4$ remains 0, and $\text{VMA}$ is updated with the new $R_{13}$ value on every 1 µs line.
* **Difference:** On CRTC 0, all counters freeze with $C_4=1$, completely blocking $R_{13}$ updates from taking effect. On CRTC 1, $C_4$ stays 0 and $R_{13}$ updates apply immediately on 1 µs lines.

---

### What Emulators Must Implement to Pass the 15 Tests

1. **Phase & Timing Calibration (5 Tests per $R_0$ Case):**
   * Emulators must evaluate the Z80 instruction cycle alignment across 5 distinct execution phases relative to $C_0$, correctly accounting for the 3rd µs of `OUT (C),r` vs the 5th µs of `OUTI` (§4.4.3, p. 26).
2. **CRTC 0 Specific Multi-Phase $C_0$ State Machine:**
   * **$C_0=0$:** Evaluate $C_4==R_4$ and $C_9==R_9$. If true, arm the Last Line state and arm vertical adjustment by default.
   * **$C_0=1$:** Latch permission for $C_9$ counting on the subsequent line. If $C_0$ never reaches 1 ($R_0=0$), permanently freeze $C_9$.
   * **$C_0=2$:** Check vertical adjustment disarm conditions ($R_5=0$). If $C_0$ never reaches 2 ($R_0\le 1$), leave vertical adjustment armed.
   * **$R_0=1$ Execution:** Emulate the automatic 1-line vertical adjustment ($C_4=1$), causing $C_4$ to alternate $0 \leftrightarrow 1$ every 2 µs and restricting $\text{VMA}$ reload to every 4 µs.
   * **$R_0=0$ Execution:** Emulate the "last hiccup" ($C_4$ incrementing to 1 on the 2nd $C_0=0$), followed by freezing all counter progression and blocking all $\text{VMA}$ updates from $R_{12}/R_{13}$.
3. **CRTC 1 Direct $\text{VMA}$ Reload Logic:**
   * Do not require $C_0=2$ to disarm vertical adjustment.
   * Keep $C_4=0$ during $R_4=0, R_9=0$ regardless of $R_0 \in \{0, 1, 3\}$.
   * Reload $\text{VMA}$ from $R_{12}/R_{13}$ on every $C_0=0$ while $C_4=0$.

---

## Q2. Module A Scope Partitioning

```
+---------------------------------------------------------------------------------------+
|                                    MODULE A ENTRIES                                   |
+------------------------------------+--------------------------------+-----------------+
| CRTC 0 & CRTC 1 Scope              | CRTC 2 Scope Only              | Gate Array /    |
| (Addressable by Type 0/1 work)     | (Untouchable by Type 0/1 work) | ASIC Scope      |
+------------------------------------+--------------------------------+-----------------+
| (4) UPDATE CRTC R0 TIMING          | (3) CRTC 2 RVMB                | (1) UPDATE VRAM |
| (5) R13 UPDATE IN 4 USEC (R0=3)    | (COPY) CRTC 2 OFFSET           |     VS CRTC     |
| (6) R13 UPDATE IN 2 USEC (R0=1)    |                                | (8) GA PIXEL-   |
| (7) R13 UPDATE IN 1 USEC (R0=0)    |                                |     ISATION     |
| (2) SKEW DISP ON R0 RUPTURE (CRTC0)|                                | (9) GA INKER-   |
| (TAB) HSYNC START POSITION         |                                |     ISATION     |
| (T) R2 UPD DURING & AFTER HSYNC    |                                | (E) GA MODER-   |
| (Y) R3 UPD DURING HSYNC            |                                |     ISATION     |
| (U) R4 & R9 CHECKING               |                                | (R) MODE UPD    |
| (I) VSYNC CONDITIONS               |                                |     >HSYNC<     |
| (O) R1 STORIES                     |                                | (CAPS) INTER-   |
| (P) R6 STORIES                     |                                |   ACTIVE MODE   |
+------------------------------------+--------------------------------+-----------------+
```

1. **CRTC 0 / CRTC 1 Scope (Directly targeted by Type 0/1 emulation):**
   * `(4) UPDATE CRTC R0 TIMING`: $R_0$ latching deadlines and `OUTI` vs `OUT` phase shifts.
   * `(5) R13 UPDATE IN 4 USEC SCREENS (R0=3)`: 4 µs frame offset update.
   * `(6) R13 UPDATE IN 2 USEC SCREENS (R0=1)`: CRTC 0 ($C_4=1$ adjustment) vs CRTC 1 ($C_4=0$ continuous).
   * `(7) R13 UPDATE IN 1 USEC SCREENS (R0=0)`: CRTC 0 (frozen $C_9$, $C_4=1$) vs CRTC 1 (continuous $\text{VMA}$ reload).
   * `(2) SKEW DISP ON R0 RUPTURE`: CRTC 0 R8 Skew DISPTMG delay/border functions (ignored on CRTC 1).
   * `(TAB) HSYNC START POSITION`: Blanking start pixel differences between CRTC 0 (pixel 5) and CRTC 1 (pixel 6).
   * `(T) R2 UPD DURING & AFTER HSYNC`: CRTC 0 non-contiguous HSYNC vs CRTC 1 infinite HSYNC loop.
   * `(Y) R3 UPD DURING HSYNC`: CRTC 0/1 R3.JIT 0.25 µs end shift vs CRTC 1 $R_3=0$ immediate cancel.
   * `(U) R4 & R9 CHECKING`: CRTC 0 $C_0<2$ Last Line evaluation vs CRTC 1 simple comparison.
   * `(I) VSYNC CONDITIONS`: CRTC 0 $C_0<2$ VSYNC blocking and $R_{3h}$ length vs CRTC 1 fixed 16 lines.
   * `(O) R1 STORIES`: CRTC 0 0.5 µs border byte on $R_1>R_0$ vs CRTC 1 clean display.
   * `(P) R6 STORIES`: CRTC 0 $C_4=R_6=C_9=0$ alternating border conflict vs CRTC 1 $R_6=0$ split-border.

2. **CRTC 2 Scope Only (Untouchable by Type 0/1 work):**
   * `(3) CRTC 2 RVMB`: Motorola-specific $R_1=0$ VMA bitwise `AND` bug and Last Line interaction (§17.4.3, p. 183).
   * `(COPY) CRTC 2 OFFSET`: Motorola-specific $\text{VMA}'$ transfer on $C_0=R_1$ of the last line (§17.4.3, p. 183; §20.3.3, p. 243).

3. **Gate Array / ASIC Scope (Untouchable by CRTC controller logic):**
   * `(1) UPDATE VRAM VS CRTC`: Z80 opcode fetch / memory contention and GA RAM access timing (§8, pp. 43–44).
   * `(8) GATE ARRAY PIXELISATION`: GA pixel shift registers, bit decoding, Mode 2 1-pixel advance (§9.1, p. 46).
   * `(9) GATE ARRAY INKERISATION`: GA palette DAC and ink update timing (§9.2, pp. 48–50).
   * `(E) GATE ARRAY MODERISATION`: GA graphic mode switching hardware (§9.3, pp. 51–52).
   * `(R) MODE UPD >HSYNC DELAY< (2.1.0)`: GA 2 µs HSYNC detection circuit and ASIC 1 µs delay (§9.3.1, p. 51).
   * `(CAPS) INTERACTIVE TEST MODE X TO Y`: GA pixel cooking and byte residue during mode transitions (§9.3.4, pp. 53–72).

---

## Q3. Module E Entry (3) CRTC 0 C4/C9 COUNTER LOGIC BUG

### ACCC Sections, Version History, and Operative Rules

* **ACCC Version History Citation (Section 2.1, p. 12):**
  * *"1.10 – 20/07/2026: Update about C9/C4 management on CRTC 0. Clarification of states Last Line CRTC 2."*

* **Primary Sections:**
  * §10.3.1.2 *Exception to the General Case : Last Line of Frame & Vertical Adjustment* (pp. 75–76)
  * §11.2.2 *CRTC 0 [Counting in Vertical Adjustement]* (pp. 81–82)
  * §12.2 *CRTC 0 [Counting : Register R4]* (pp. 92–94)
  * §13.2.1 *The First 3 Microseconds* (pp. 103–104)

### Rule Summary with Operative Quotes

1. **Omission of Dedicated $C_5$ Counter (§11.2.2, p. 81):**
   > *"On CRTC 0, HITACHI engineers saved a C5 counter to use C9 instead. In additional management, C9 is compared with R9 and R5."*
   The new limit of $C_9$ switches from $R_9$ to $R_5$ when vertical adjustment becomes active.

2. **Evaluation Spread Over the First 3 Microseconds ($C_0=0, 1, 2$) (§13.2.1, pp. 103–104; §10.3.1.2, p. 75; §12.2, p. 92):**
   * **$C_0=0$:**
     > *"The value of C9 is compared with R9 when C0<2 (just as C4 is compared with R4) to determine a state that indicates whether the line was the last on the frame... When C0=0 (or C0=1), the CRTC assesses whether it is on the last line, and if so, arms an internal flag by default to trigger vertical adjustment."* (§10.3.1.2, p. 75; §13.2.5, p. 107).
   * **$C_0=1$:**
     > *"When C0=1, then the management of C9 is again authorized for the next C0=R0... If the 'last line' state is true at position C0==0 (therefore C9==R9 and C4==R4), and R9 or R4 is updated at position C0==1 with a value different from C9 or C4, then vertical adjustment becomes active and the current line becomes the 'first' adjustment line."* (§13.2.1, p. 103; §10.3.1.2, p. 75).
   * **$C_0=2$:**
     > *"When C0=2,the additional management state is deactivated if there was no line programmed (R5=0 or no 'Interlace Line')... If R5 becomes greater than 0 when C0>2, then no additional line will be added to the frame. The next line will correspond to a new frame with C4=C9=0."* (§13.2.1, p. 104).

3. **$C_4$ Increment Logic During Vertical Adjustment (§11.2.2, p. 81; §13.2.4, p. 106):**
   > *"In order to prevent resetting C9 to 0 from leading to a loop if R5>R9+1, the incrementation of C4 and C9 follows a specific logic. As long as C4<>R4 in vertical adjustment, C9 can no longer be zeroed. This allows C9 to exceed R9 and reach R5, to end the vertical adjustment and start a new frame by repositioning Last Line state to true (so that C4 and C9 go to 0 on the next line)... C4 is incremented only once, whatever the value of R5. C4 returns to 0 once the adjustment is complete, whatever the value of R4."* (§11.2.2, p. 81; §13.2.4, p. 106).

4. **Simultaneous Increment Bug on $C_0=R_0$ (§10.3.1.2, p. 76; §11.2.2, p. 82):**
   > *"When R9 is modified exactly at C0==R0, this causes a problem with the C9/R9 to C9/R5 toggle process. The value of C9 is first compared to R9, causing C4 to increment. Since C4 is then different from R4, C9 is then compared to R5, and C9 is also incremented. In the previous example, we end up with C4==39 and C9==8."* (§11.2.2, p. 82).

---

## Q4. Unrepresented ACCC Chapters & Unmappable Menu Entries

### 1. ACCC Chapters with No Dedicated SHAKER Menu Test Entry

* **Chapter 1: Preface (p. 11)** — General introduction and credits.
* **Chapter 2: Version History & Licence (pp. 12–13)** — Revision history and CC BY-NC-ND license directives.
* **Chapter 3: General (pp. 14–15)** — General terminology and acronyms.
* **Chapter 4: CRTC & CPC (pp. 16–30)** — General architectural overview and Z80 instruction timings (theoretical background).
* **Chapter 5: Other Circuits (pp. 31–32)** — I/O port address decoding map and CPC+/GX4000 17-byte ASIC unlock sequence.
* **Chapter 6: Building a Frame (pp. 33–36)** — "Ideal" CRTC counting baseline and standard frame construction diagrams.
* **Chapter 22: Other Registers (p. 250)** — Cursor registers ($R_{14}/R_{15}$) and Light Pen registers ($R_{16}/R_{17}$) expansion port pinouts (Pin 19 `CUDISP`, Pin 3 `LPSTPB`).
* **Chapter 23: Fullscreen & Centering (pp. 251–252)** — CTM 644 CRT monitor curvature, 48-character / 272-line fullscreen dimensions.
* **Chapter 25: A Brief History of Fixed Time (pp. 269–280)** — Z80 fixed-time assembly coding methodologies and `CalcCPU` / `waitHLusec` compensation tools.
* **Chapter 26: Duration of Instr. on the CPC (pp. 281–282)** — Complete lookup table of Z80 instruction execution times on CPC.
* **Chapter 28: CRTC Identification (pp. 292–293)** — Summary of the 10 identification techniques (used by SHAKER's internal boot detection harness).
* **Chapter 29: CPC Identification (p. 294)** — Identification via ASIC unlocking, PPI Port C, and PPI Port B bugs.

### 2. Unmappable / Utility Menu Entries in SHAKER

* **Module D `(COPY) RESET CPC`:**
  * **Status:** Utility command / Non-test entry.
  * **Explanation:** Executes a software system reset of the CPC. Referenced in ACCC §4.1 Note 2 (p. 17) regarding Low ROM initialization start ($64\text{ µs}$ after reset), but does not test any CRTC hardware functionality.
* **Module B `(F0) BOUNGA!`:**
  * **Status:** Demo / Torture routine (Medium confidence mapping to Chapter 24, pp. 253–268).
  * **Explanation:** A graphical demonstration showing extreme full-screen vertical bouncing using dynamic register manipulation rather than an isolated, verifiable register test case.
