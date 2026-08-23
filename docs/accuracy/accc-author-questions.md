# Questions for the ACCC author (Longshot)

Collected 2026-08-22 during the faithfulness review of the repository's distillation against
*The Amstrad CPC CRTC Compendium* v1.10 (`docs/ACCC1.10-EN.pdf`). Each item is a place where
the source itself is ambiguous, self-contradictory, or silent where we need precision — not
extraction failures. Page numbers are PDF pages. Companion analysis:
[findings-review.md](findings-review.md).

1. **p.75-76 (§10.3.1) — last-line state and C0>1 writes.** The text says a last-line state
   found at C0<2 "cannot become false again until the next comparison of C4 and C9 (when C0
   will again be 0 or 1)" if R9/R4 are modified when C0>1 such that equality holds — while
   pp.92-93 say that once the state is armed, mid-line writes at C0>1 do not change the
   following reset. Can a write landing at C0>1 *arm* the last-line state for the following
   line (i.e. is late arming possible), or does that sentence only describe an already-armed
   state surviving? A precise statement would settle how permissive the C0∈{2..R0−1} window
   is.
2. **p.104 (§13.2.4) — cost of the R0=0 freeze.** The figure caption says freezing R0=0 for
   64×8 µsec "amounts to 'forgetting' 8 lines (C4−1 if R9=7)". Is the intended accounting a
   loss of N/64 raster lines of display time (counters frozen, nothing progresses), or is
   there a counter-level effect we should model (what exactly does "(C4−1 …)" mean)?
3. **p.107 (§13.2.5) — ending the R0=1 adjustment with R5=0.** The case study states "for some
   reason I haven't determined yet, C9 (**not C9+1**) is compared to R5" to stop the
   adjustment after one line. Why C9 rather than C9+1 — is this an internal comparator
   artefact the author later resolved, or genuinely unexplained?
4. **p.88 (§11.6.1) — repeated-RFD sentence.** "A RFD triggered on the last line C9=R9
   disables the state allowing VMA to be updated with R12/R13" sits between the case-1/case-2
   description and the RFD#10 discussion. Does triggering another RFD on a line where
   C9==R9 disarm the VMA-source flag immediately (and does this interact with RFD#10), or is
   this sentence only about RFD#10?
5. **p.130 (§14.1) — HSYNC start vs stop.** After describing C3l counting from C0=R2 until
   C3l=R3l ("end of HSYNC"), the next sentence reads "the HSYNC **starts** as soon as the C3L
   counter reaches the value of R3L". Should this read "stops"? (Everything downstream uses
   "ends".)
6. **pp.133 (§14.3) — C-HSYNC duration table cells.** Please confirm each cell gives the
   *range of observed values* within one mode (two NJIT samples / two JIT samples, e.g. CRTC0
   R3=4: NJIT {2.0625, 2.1250}, JIT {2.3125, 2.3750}), i.e. JIT ≈ NJIT + 0.25µs — and whether
   the CRTC0 R3=3 NJIT value printed as "1,0525" is a typo for 1,0625.
7. **p.146 (§15.1) — deflector-lock threshold.** "If the HSYNC is too short (>2 µsec and
   <6 µsec)" contradicts the surrounding rules (R3≤2 too short; R3≥6 exact). Should it read
   "<2 µsec" (with 2–6 µs partial-lock distortion)?
8. **p.167 (§16.3) — mechanism 2 on CRTC 2.** Mechanism 2 is stated present on CRTCs 0/1 and
   explicitly absent on 3/4. Does CRTC 2 carry mechanism 2, or is it mechanism-1-only like
   types 3/4?
9. **p.193 (§19.2.1) — BORDER ON pointer update condition.** BORDER ON states the current
   pointer "is updated when C0=R1 and **C9=C0=0**". Is this a typo for the row-end condition
   C9=R9 (as in §17.1), or does BORDER ON really latch VMA' under C9=C0=0?
10. **pp.198/205/216 (§19.3/§19.5.1/§19.6.1) — which frame gets the extra interlace line.**
    §19.3 says it ends "the first frame"; §19.5.1/§19.6.1 attach it to completion of the
    **even** frame's construction; p.199 shows the odd frame lasting 20032µs "inheriting" it.
    Which frame receives the additional line, and is the odd/even labelling relative to
    MID-VSYNC frames or to ParityFrame?
11. **p.205 (§19.5.2) — even R9 total line count.** IVM programming with even R9 yields an
    even number of lines per character (R9=6 → 2×4 = 8 lines). Please confirm the intended
    parity of the *total* line count (even), since a distilled summary inverted this.
12. **p.206 (§19.5.2) — odd-C4 IVM activation imbalance.** Activating R8=3 on an odd C4 can
    imbalance the VSYNC-delay correction for that transition frame. Does the source assert
    (or could you confirm) that the imbalance self-corrects on subsequent frames?
13. **p.245 vs p.293 (§21.2.2 vs §28.1.9) — type-1 readable registers.** §21.2.2 documents
    readable R14-R17 (plus register 31); the identification chapter states that on CRTC 1
    "*all* registers return 0 except register 31". Which is authoritative — do R14/R15 (and
    R16/R17) really read back stored values on UM6845R?
14. **p.293 vs pp.246-248 (§28.1.8 vs §21.3.3) — status bit numbering.** The identification
    test polls "the transition of **bit 6**", but §21.3.3 defines bit 5 as the only dynamic
    status bit (bit 6 unused/read-0), while the CRTC 3/4 STATUS-1 table (p.248) has an
    always-1 bit 6. Which bit does the &BE00 identification test actually target on UM6845R?

15. **p.190 — border-alternation condition wording — RESOLVED BY DEFAULT READING
    2026-08-23.** "Alternation only takes place when the condition R1 is fulfilled
    (BORDER R1 is false)" is self-contradictory in isolation. The immediately preceding
    mechanism is unambiguous: DISPLAY ENABLE turns ON at each character start and OFF
    0.5 µs later; therefore BORDER-R1 *false* is the operative state permitting the
    BORDER-R6 conflict to alternate bytes. Visual reading recorded in
    `f6-decision-gate.md` Stage 2b; author confirmation remains welcome but nothing is
    blocked.
16. **p.188 (§18.1) — "(1st line-character R6)" — RESOLVED BY DEFAULT READING
    2026-08-23.** This is not a first-scanline restriction. The same page explicitly says
    C4=R6 is considered immediately regardless of C0 (with type 3/4 separately excepted),
    and says the rule holds whatever C9. Visual reading recorded in
    `f6-decision-gate.md` Stage 2b; author confirmation remains welcome.
17. **p.292 vs pp.86-89 (§28.1.1 vs §§11.2/11.4) — type-1 VSYNC discriminator boundary.**
    §28.1.1 states that for R4=36, R9=7, R5=16 the type-1 discriminator is "VSYNC stops
    occurring once R7>39", implying C4 reaches 39. But §§11.1-11.4 increment C4 once per
    C9==R9 wrap during type-1 adjustment, i.e. once per 8 scanlines; 16 adjustment lines
    yield two increments, so C4 tops out at 38 and R7=39 is unreachable. Our corrected
    oracle (`t08g`/`t08h`, review action A1) therefore expects silence from R7=39 using
    §§16.1/16.4.2 ("VSYNC starts at C4==R7"). Please confirm which reading is right: does
    §28.1.1 count an extra increment we are missing (a third C9 wrap, an interlace line,
    or a different R5 accounting), or is its boundary value imprecise?

18. **p.122-123 vs p.124 (§13.7.1.1 gist vs §13.7.1.2) — R0-widening RFD arming condition.**
    The p.122-123 chronogram annotation reads "RFD activated on CRTC 1 if R4 and/or R9
    **modified** until C0=7F (new R0) on last line of frame" — a write-event condition that
    would arm even if the written value is later restored to the original. The dedicated
    §13.7.1.2 section defines its variants by state: "(C9 != R9 **by line end**)" /
    "(C4 != R4 **by line end**)". Our model follows the end-state reading (`t13h` pins
    cancel-then-restore as NOT arming), on the grounds that a real comparator has no
    "was written" latch and §13.7.1.2 is the dedicated section (review finding F-7,
    `f7-r0-widening-independent-review.md`). Please confirm which reading is right; SHAKER
    Module C `(1)` / D `(9)` can discriminate by restoring R9/R4 mid-extension before the
    widened line end.

Also noted while verifying (no answer needed, listed for completeness): p.195 places the
skew-delay-from-substitution note inside the CRTCs-1/3/4 paragraph — we assume the delay
applies to type 0's substituted trigger (the placement caveat cited by
`f6-decision-gate.md`). Corrections welcome.

Implementation note on open Q4 (p.88 repeated-RFD sentence, added 2026-08-23): the F7 RTL
(`rtl/crtc_type1_engine.v`) now models the RFD state machine that question asks about. In
this model a same-edge R5 0→nonzero write always sets both flags and wins over any
same-edge save-clear, so an RFD triggered on a C9==R9 line arms rather than disables; the
documented recipe (§11.6.3) triggers where C9!=R9, so nothing observable depends on Q4
until it is answered.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
