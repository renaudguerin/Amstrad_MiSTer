# ACCC v1.11 French/English consequential differences

This is the curated finding ledger for the current French and English editions of *The
Amstrad CPC CRTC Compendium*. It records only language differences that can change a rule
reading, repository documentation, a test premise, RTL, hardware interpretation, or useful
feedback to the author. It is not a sentence-by-sentence translation review.

Technical information is sourced from *The Amstrad CPC CRTC Compendium* by Longshot
(CC BY-NC-ND 4.0).

## Sources and method

- French authority: v1.11, 295 pages, SHA-256
  `4409e3a2e77cd54e499c6956446b01bce93f79a1c1ba366201d514cf6e3c0d47`.
- English working translation: v1.11, 295 pages, SHA-256
  `3e45eb7eea7dc8f0d7211f78bec4f8d00530ce3c00da2e76034fb24f7a751868`.

Sections are aligned by number because translated prose changes pagination. Position-aware
extraction is used for candidate discovery; rendered original pages decide tables, diagrams,
and layout-dependent readings. Source text, deduction, current RTL/test behavior, and
hardware evidence remain separate. See `accc-update-procedure.md` and the durable sweep plan
at `../plans/2026-08-30-accc-bilingual-sweep.md`.

The section-complete sweep covers technical chapters 3–29. Chapters 1–2 contain front matter,
licensing, and edition metadata and were excluded from implementation review. Every candidate
below was checked against original-page renders; extraction alone never decided a finding.

## Confirmed findings

### BL-001 — §19.5.2 repeated activation qualifier

**Pages:** French p.208; English p.206.

The French worked example says that R8 changes to 3 at C4=1/C9=0 on every frame. The English
example omits the recurrence qualifier. The example therefore recreates the disturbance each
frame; it is not evidence that one activation creates unexplained persistent state.

**Disposition:** Already recorded in `accc-author-feedback.md`. This is a source-reading
clarification, not proof of current RTL, post-toggle pin timing, or hardware behavior.

### BL-002 — §12.2.1 genuine-last-line RLAL write windows

**Pages:** French p.96; English p.94.

For the genuine-last-line sequence, French distinguishes two complementary windows: update
R9/R4 on line N when C0>1, or on line N+1 when C0<2. English omits the C0>1 condition for line
N and leaves the sentence grammatically compatible with applying C0<2 to the wrong part of
the sequence. Its following note partially repairs the R9 case by requiring a wait until
C0=2, but does not restate the complete R9/R4 rule.

The timeline diagram is visually identical in both editions. The checked-in test already
uses the French-safe sequence: R9 lands at C0=2 on N and R4 at C0=0 on N+1. The digest's
current paraphrase incorrectly assigns C0<2 to both lines.

**Disposition:** Documentation correction and English courtesy clarification. No RTL or test
behavior change is currently indicated.

### BL-003 — §19.5.1 MID-VSYNC horizontal condition

**Pages:** French p.206; English p.205.

The French general parity summary says that MID-VSYNC is generated on the even frame when
C4=R7 and C0=R0/2. English omits the half-line C0 condition in this summary. English later
states the complete half-line rule in the worked material, so the edition is locally
self-correcting.

**Disposition:** English courtesy correction. Current digests and implementation already use
the half-line rule; no code change is indicated by this omission.

### BL-004 — §19.5.1 parity-state model scope

**Pages:** French p.206; English p.205.

The French general summary says that CRTCs 0, 2, 3, and 4 have multiple internal parity
management states. English lists CRTCs 0, 3, and 4, omitting type 2. English §19.5.4 later
documents the type-2 states explicitly, so the detailed section repairs the summary.

**Disposition:** English courtesy correction. The current classic parity implementation is
scoped to types 0 and 1, so no RTL consequence is presently indicated.

## Remaining consequential findings

This compact register separates the edition difference from its repository consequence.
"Audit" means derive a directed premise/test before considering RTL; it does not assert that
the current model is wrong. Page pairs are French/English.

| ID | Section; pages FR/EN | Consequential difference | Disposition |
|---|---|---|---|
| BL-005 | §4.2; 18/18 | French says the extra two U.S.-ROM lines make the GA interrupt request arrive on the same scanline as CRTC VSYNC, but before it; English says it is not on the same line. | Audit the historical U.S.-ROM phase case; author correction. |
| BL-006 | §4.4.2; 24/24 | French correctly says `INI` increments HL; English says it decrements HL and thereby duplicates `IND`. | English correction only; T80 already implements the increment direction. |
| BL-007 | §4.4.3; 26/26 | French's practical 1 µs-early placement applies specifically to `OUT(C),r8` and separately exempts `OUTI` on CRTC3/4. English broadens it to generic write-I/O and adds `OUTD` to the exception. | Keep instruction-specific repository wording; ask whether `OUTD` is an intended clarification. |
| BL-008 | §7.2; 40–41/39–40 | The synchronization routine differs: French uses `19968-21` and derives 5 µs as `1+1+3`; English uses `19968-23`, derives `2+1+3`, then subtracts 1 µs. | Do not adopt either routine without a complete cycle audit; author question. |
| BL-009 | §4.4.4; 27–31/27–30 | French attributes `/WAIT` request/repetition to the Gate Array; English says Gate Array/ASIC. | Ask whether identical ASIC mechanism/scope is intended. |
| BL-010 | §10.3.1.1; 76/75 | French says type-0 C9 continues after wrapping through zero until it reaches R9; English stops at the wrap. | Current equality/overflow RTL is French-compatible; documentation/courtesy only. |
| BL-011 | §10.3.1.2; 77/76 | French uses `C9<>R9`; English says `C9<=R9`, losing the C9>R9 overflow path. | Current equality RTL is correct; audit prose/comments. |
| BL-012 | §11.1; 81/80 | French has `R5/(R9+1)>1`; English drops the parentheses as `R5/R9+1>1`. | English typesetting correction; audit any copied formula. |
| BL-013 | §11.2.2; 82/81 | French says adjustment freezes C4 and prevents C9 returning to zero at R9; English narrows this to `C4<>R4` and omits the C4 freeze. | Premise/citation audit; current adjustment model appears French-compatible. |
| BL-014 | §11.6; 89–90/87 | French defines exactly two RFD states and says the VMA-source state persists independently of a C0/R1 test; English says “several” and gives a different persistence clause. | Ambiguous lifetime: author clarification before any RTL change. |
| BL-015 | §11.6.1; 90/88 | French identifies odd parity as the reason the case-1 test fails; English only calls the test faulty. | English clarification; no standalone behavior change. |
| BL-016 | §12.5; 103/101 | French says CRTC3/4 C9 returns to zero when C9>R9; English nonsensically says it “increases to 0.” | English wording correction; Plus/future scope. |
| BL-017 | §13.2.1; 106/104 | French says additional management prevents C4 reset and that next-line `C4=R4+1` requires R4 to have remained equal to C4 throughout the line; English distorts the state effect and omits the history condition. | IA-4 predicts a current-model mismatch after an R4 away-and-back history; add the failing vector before RTL. |
| BL-018 | §13.2.4; 108/106 | French explains that C4/R4 inequality switches C9's comparison target to R5 and gives the exact `C4=R4 && C9=R9` entry condition; English loses both precision and cause. | Premise/citation audit; no defect asserted. |
| BL-019 | §13.7.2; 126/124 | French programs the next-line C4 action at `C0=0`, requires `C4=R4 && C9=R9`, and excludes adjustment/interlace (`R5=0,R8=0`). English changes these to `C0=R0`, tautological `C4=C4`, and positive adjustment/interlace. | Correct the counter digest; audit affected R0=1 premises. |
| BL-020 | §13.7.2; 127/125 | French's R0=1 initial programmed increment requires both `C9=R9` and `C4=R4`; English omits the C4 condition. | Correct digest/premises; author correction. |
| BL-021 | §13.7.2.2; 128/126 | French ends the R5=0 count at `R5-1`; English says R5. The stated 8–31 range independently corroborates French. | Current RTL/vector use R5−1; correct digest only. |
| BL-022 | §14.1; 132/130 | French says HSYNC “begins” when C3l reaches R3l; English says “ends.” French conflicts with its own definitions, tables, and later prose. | Treat as a French typo; retain end behavior and ask the author. |
| BL-023 | §14.4 FR / unnumbered EN continuation; 135/133 | French identifies monitor-side C-HSYNC and quantifies 0.25 µs/four Mode-2 pixels; English says generic HSYNC and omits the pixel equivalence. | Normalize signal naming in touched docs. |
| BL-024 | §14.8 FR / §14.7 EN; 144/142 | French says the GA arms an interrupt after HSYNC; English says it triggers one. | Prefer requests/arms to distinguish pending state from CPU execution. |
| BL-025 | §15.3.2; 150/148 | French adds a type-0 exact-end case: changing R3 at `C0=R2+R3` during infinite HSYNC makes C3l overflow. English omits it. | IA-1 finds coarse overflow already modeled but the documented sub-character restart phase predicted wrong; add a counter-plus-pin discriminator. |
| BL-026 | §15.3.3; 151/149 | French calls the approximately 3.5-pixel R2.JIT restart an earliest case; English omits “at earliest.” | Add qualifier; do not encode universal exact equality. |
| BL-027 | §15.4.1; 154/152 | French says CRTC0/1/2 evaluate the VSYNC condition regardless of C0; English's “either C0” is a broken quantifier. | Current digest/model already use any C0; courtesy only. |
| BL-028 | §16.1; 161/159 | French says video pointers continue during VSYNC; English broadens this to counters and pointers. | Do not cite this sentence alone for all counters. |
| BL-029 | §16.2.1; 162/160 | French defines NJIT as programming R7 before C4 reaches it and JIT as writing R7 with current C4. The English NJIT heading incorrectly imports the JIT definition and makes its condition inconsistent. | Correct digest; current equality-write model appears sound. |
| BL-030 | §16.3/§16.4.4; 169,172/167,170 | French requires `C4=R7,C9=0,C0=0` for CRTC3/4. English §16.3 omits the full parenthetical; §16.4.4 retains the headline gate but omits C9>0 from the later write-trigger blocking sentence. | Plus RTL/tests already correct; citation/courtesy only. |
| BL-031 | §16.4.1.2; 170/168 | French says C4 or R7 must change value to unblock type-0 VSYNC; English merely says update, which could imply a same-value write. | Current false-then-true rearm model is correct; clarify prose. |
| BL-032 | §16.4.2–3; 171/169 | French explicitly says a partial VSYNC counts the current line; English omits “current.” | Existing type-1 timing is correct; English clarification. |
| BL-033 | §16.4.1.2; absent/169 | English alone adds two normative R0/VSYNC cases: R0>2→0 can make VSYNC infinite, and →1 with R3h=1 yields 2 µs. | Quarantine as English supplemental material; seek author/hardware confirmation before treating as oracle. |
| BL-034 | §27.2; absent/283 | English alone adds the RMR shorthand `10xIRrmm`; both editions agree that bit 4 resets the R52 interrupt counter. | Preserve bit-4 rule; qualify shorthand until independently validated. |
| BL-035 | §17.2.2; 180/178 | French says row-select bits come from C9; English adds “or C5.” | Correct digest to C9; RTL already uses C9. |
| BL-036 | §18.3.2; 191/190 | French makes definitive border at C0=R1 depend on R6 being zero then; English changes this to R6 having been zero at least once. | Digest corrected; IA-3 finds live-R6 RTL compatible but no focused 0→nonzero-before-R1 guard. |
| BL-037 | §19.3.4; 202/201 | French warns that complete-interlace construction must account for VSYNC at C4=R7; English replaces it with a normative frame-start-only R8 activation rule. | Current mid-frame transition coverage remains valid; ask author about the replacement. |
| BL-038 | §19.5.3; 209/208 | French explicitly sets `ParityC9=ParityFrame` at type-1 frame start; English omits it. | IA-2 reaches unequal states and predicts current toggle-both RTL disagrees with French realignment; add the failing vector before RTL. |
| BL-039 | §19.5.4; 213/212 | French says the CRTC2 state defines all C9 values in a frame; English says only the first C9. | CRTC2 documentary debt/author correction. |
| BL-040 | §19.5.5; 214/213 | French says CRTC3/4 ParityC9 changes on each C4 when R9 is odd; English's opening bullet says even, but its later prose says odd. | Plus RTL/tests already follow odd; English correction. |
| BL-041 | §20.3.2; 242/242 | French says type 1 reloads VMA from R12/R13 every time C0 returns to zero while C4=0, independently of C9; English truncates the recurrence/independence. | Correct digest's first bullet; RTL/tests already implement the French rule. |
| BL-042 | §20.3.3; 243/243 | French says CRTC2 initializes VMA from VMA′ at frame origin; English says “VMA′ & VMS” from R12/R13, contradicting its following paragraph. | CRTC2 documentary debt/author correction. |
| BL-043 | §22; absent/250 | English alone adds a vague warning that small/sync-period writes to other registers may affect other registers, without an actionable condition or result. | Ask whether this is unsupported, omitted from French, or needs a concrete rule. |
| BL-044 | §28.1.9; 293/293 | French lists all eight CRTC3/4 readback entries through R14/R15 and points to §21.2.3; English stops at R13 and points to §20.3.4. | Plus RTL/tests already correct; English correction. |

## Section coverage and negative result

The sweep aligned every numbered section in chapters 3–29; the explicit per-section clean/
finding register is `accc-1.11-fr-en-coverage.md`. Consequential candidates were
found in chapters 4, 7, 10–20, 22, 27, and 28. Chapters 3, 5, 6, 8, 9, 21, 23–26, and 29
had no implementation-consequential language difference after original-page checks. Minor
cross-reference or programming-only differences remain useful courtesy notes: French §7.2
points to §25.6 instead of §25.7; French §9.3.4.1 points to §16.2.2 instead of §16.2.3;
English §23.2 says “Chapter 0” instead of §14.4; English §23.3 points to chapter 15 instead
of 16; and §24.10.2 differs in page-base scope and omits the once-per-2048 qualifier.

No figure-only technical mismatch was found. Dense tables and chronograms in every packet
were rendered when they carried candidate conditions or extraction/layout risk.

## Visual calibration result

Both PDFs contain 18 embedded raster streams (17 images and one soft mask). Every paired
stream is byte-identical; only its page placement changes where translated prose reflows.

Representative vector/render comparisons covered a counter table, HSYNC schematics,
interlace parity and chronogram tables, R1 display tables, a video-pointer diagram, and the
diagnostic chapter. Main path/fill signatures and rendered technical content match. The
video-pointer page contains six extra unfilled French line segments but preserves the same 99
filled shapes and rendered relationships. The §12.2.1 timeline and sampled §19.5.2 diagrams
also match, including a source typo shared by both editions.

This supports targeted rather than exhaustive visual processing. Render unmatched vector
signatures, translated labels/tables, extraction fallback pages, and semantic candidates.
Asset equality alone does not establish semantic equality where labels or layout carry the
rule.
