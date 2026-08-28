# Questions for the ACCC author (Longshot)

Collected 2026-08-22 during the faithfulness review of the repository's distillation against
*The Amstrad CPC CRTC Compendium* v1.10 (`docs/ACCC1.10-EN.pdf`). Each item is a place where
the source itself is ambiguous, self-contradictory, or silent where we need precision — not
extraction failures. Page numbers are PDF pages. Companion analysis:
[findings-review.md](findings-review.md).

Re-checked 2026-08-26 against fresh pdf-inspector extraction, rendered pages where a table,
diagram, or exact layout is evidence below, and the CRTC chip/reference documents under
`docs/references/`. The ACCC remains authoritative where those secondary documents conflict
with it.

1. **p.75-76 (§10.3.1) — last-line state and C0>1 writes.** The text says a last-line state
   found at C0<2 "cannot become false again until the next comparison of C4 and C9 (when C0
   will again be 0 or 1)" if R9/R4 are modified when C0>1 such that equality holds — while
   pp.92-93 say that once the state is armed, mid-line writes at C0>1 do not change the
   following reset. Can a write landing at C0>1 *arm* the last-line state for the following
   line (i.e. is late arming possible), or does that sentence only describe an already-armed
   state surviving? A precise statement would settle how permissive the C0∈{2..R0−1} window
   is.

   **RESOLVED BY DEFAULT READING 2026-08-26** (fresh pass pp.75-76, 82, 92-93; no
   author answer). There is **no late arming**. §12.2 p.92 says that R4/R9 must establish the
   equality while C0<2 to validate `Last Line`; §12.2.1 p.93 then demonstrates the converse:
   setting R4=R9=0 after that window on the first C4=C9=0 line does not make it last, and C4
   becomes 1 on the following line. The p.75 sentence therefore describes an already-armed
   state surviving C0>1 writes when adjustment was not selected at C0=2, not a second arming
   window. C0>1 writes can still affect the live adjustment/line-end paths documented on p.82,
   but cannot newly validate the latch. The existing F12 windowed-latch reading stands.
2. **p.104 (in the §13.2.1 run-on; rule restated under §13.2.4, p.105) — cost of the R0=0
   freeze.** The p.104 prose says freezing R0=0 for
   64×8 µsec "amounts to 'forgetting' 8 lines (C4−1 if R9=7)". Is the intended accounting a
   loss of N/64 raster lines of display time (counters frozen, nothing progresses), or is
   there a counter-level effect we should model (what exactly does "(C4−1 …)" mean)?

   **RESOLVED BY DEFAULT READING 2026-08-26.** This is elapsed-time accounting, not a
   counter operation. `64 x 8 µsec` is 512 µsec, or eight normal 64-µsec raster lines; with
   R9=7 those eight lines are one C4 character-row period. The parenthetical means that wall
   clock advanced by the equivalent of one row while C4 did not, not that C4 is decremented.
   The detailed p.108 case study confirms that C9 is frozen, C4 may perform only the separately
   documented one-time "last hiccup", and no counter otherwise advances. No additional RTL
   effect is required beyond the existing freeze.
3. **p.107 (§13.2.5) — ending the R0=1 adjustment with R5=0.** The case study states "for some
   reason I haven't determined yet, C9 (**not C9+1**) is compared to R5" to stop the
   adjustment after one line. Why C9 rather than C9+1 — is this an internal comparator
   artefact the author later resolved, or genuinely unexplained?

   **BEHAVIOR CONFIRMED; CAUSE STILL UNSOURCED 2026-08-26.** The rendered p.107 still says
   explicitly that the author had not determined the reason. The adjacent p.108 R0=0 exit
   example exercises the normal `C9+1` form ("C9+1 being different from R5, then C9 is
   incremented"), so the p.107 `C9` comparison is not merely loose notation for the general
   rule. Best inference: the R0=1 path never reaches the C0=2 cancellation stage and therefore
   terminates the default-armed adjustment through a distinct current-C9/R5 decision; with
   C9=R5=0 it marks the forced first adjustment line as complete. Neither the ACCC nor the
   available chip documents expose enough internal logic to prove that mechanism. Treat the
   observed one-line stop as authoritative, but keep the silicon rationale open for the author
   or hardware discrimination.
4. **p.88 (§11.6.1) — repeated-RFD sentence.** "A RFD triggered on the last line C9=R9
   disables the state allowing VMA to be updated with R12/R13" sits between the case-1/case-2
   description and the RFD#10 discussion. Does triggering another RFD on a line where
   C9==R9 disarm the VMA-source flag immediately (and does this interact with RFD#10), or is
   this sentence only about RFD#10?

   **RESOLVED BY DEFAULT READING 2026-08-26; RTL DIVERGENCE F17.** The sentence belongs to
   the ordinary case-2 rule and is not limited to RFD#10: an RFD triggered while C9=R9 leaves
   the R12/R13-source state disabled. The following "However" introduces RFD#10 only as an
   exception for parity management in the C9=R9 test. The source does not expose an internal
   C0 ordering for this effect, so the externally visible disable rule is the safe fixture
   oracle. The current F7 implementation does the opposite: it arms at C0=R0, after the
   C0=R1 save opportunity on a normal R1<R0 line, and `t13d` explicitly requires both flags
   armed. F17 owns re-deriving that vector and fixing the source-flag behavior before RTL.
5. **p.130 (§14.1) — HSYNC start vs stop.** After describing C3l counting from C0=R2 until
   C3l=R3l ("end of HSYNC"), the next sentence reads "the HSYNC **starts** as soon as the C3L
   counter reaches the value of R3L". Should this read "stops"? (Everything downstream uses
   "ends".)

   **RESOLVED 2026-08-26.** The render confirms that `starts` is printed, and it is a typo for
   **stops/ends**. C3l starts at 0 when C0=R2, which is already the HSYNC start; reaching R3l
   terminates the pulse at the start of that character. The tables and all downstream prose
   use that interpretation.
6. **pp.133 (§14.3) — C-HSYNC duration table cells.** Please confirm each cell gives the
   *range of observed values* within one mode (two NJIT samples / two JIT samples, e.g. CRTC0
   R3=4: NJIT {2.0625, 2.1250}, JIT {2.3125, 2.3750}), i.e. JIT ≈ NJIT + 0.25µs — and whether
   the CRTC0 R3=3 NJIT value printed as "1,0525" is a typo for 1,0625.

   **RESOLVED VISUALLY 2026-08-26.** The caption immediately above the rendered table says,
   "I indicated a range of 2 values that I could see," so each cell is the two-value observed
   range for that CRTC/mode. The cited R3=4 values are exact, and corresponding JIT endpoints
   are 0.25 µsec above NJIT throughout the table. `1,0525` is printed, but is a typo for
   **1,0625**: every measurement lies on the 0.0625-µsec lattice, the paired endpoint is
   1.1250, and 1.3125 JIT minus 0.25 is 1.0625.
7. **p.146 (§15.1) — deflector-lock threshold.** "If the HSYNC is too short (>2 µsec and
   <6 µsec)" contradicts the surrounding rules (R3≤2 too short; R3≥6 exact). Should it read
   "<2 µsec" (with 2–6 µs partial-lock distortion)?

   **RESOLVED 2026-08-26: the inequalities are correct.** The rendered parenthetical decodes
   the band explicitly: HSYNC `>2` and `<6` gives C-HSYNC `>0` and `<4` µsec. At R3l=2 the
   generated C-HSYNC is too short for the deflector to process at all; above 2 the monitor
   starts trying to lock, but below 6 the pulse is still too short for clean locking and can
   distort. Thus 3-5 µsec is the partial-lock distortion band and `too short` means too short
   for a clean lock, not shorter than the detection threshold. Changing `>2` to `<2` would be
   wrong.
8. **p.167 (§16.3) — mechanism 2 on CRTC 2.** Mechanism 2 is stated present on CRTCs 0/1 and
   explicitly absent on 3/4. Does CRTC 2 carry mechanism 2, or is it mechanism-1-only like
   types 3/4?

   **RESOLVED BY DEFAULT READING 2026-08-26.** CRTC 2 carries mechanism 2. §16.3 excludes
   only CRTCs 3/4 ("This second mechanism was not renewed" for those ASICs), while §16.4.4
   confirms that 3/4 have no VSYNC reentrancy protection. CRTC 2 is never excluded and its
   §16.4.3 ghost-VSYNC state explicitly prevents another VSYNC while that state is active.
   The source consistently groups mechanism-1-only reentrancy with 3/4, not type 2.
9. **p.193 (§19.2.1) — BORDER ON pointer update condition.** BORDER ON states the current
   pointer "is updated when C0=R1 and **C9=C0=0**". Is this a typo for the row-end condition
   C9=R9 (as in §17.1), or does BORDER ON really latch VMA' under C9=C0=0?

   **RESOLVED BY DEFAULT READING 2026-08-26.** It is a garbled restatement of the normal
   pointer bookkeeping, not a BORDER-ON-specific latch. As printed, `C0=R1 and C9=C0=0` is
   impossible unless R1=0. §17.1 p.176 gives the exact row-end rule in the same sentence
   shape: when `C0=R1 and C9=R9`, VMA is transferred to VMA'; it separately gives the C0=0
   line-start reload (with the frame-origin R12/R13 case at C4=C9=C0=0). BORDER ON forces
   DISPEN off but leaves those normal pointer increments/save/reload events running. No
   special `C9=C0=0` save condition should be implemented.
10. **pp.198/205/216 (§19.3/§19.5.1/§19.6.1) — which frame gets the extra interlace line.**
    §19.3 says it ends "the first frame"; §19.5.1/§19.6.1 attach it to completion of the
    **even** frame's construction; p.199 shows the odd frame lasting 20032µs "inheriting" it.
    Which frame receives the additional line, and is the odd/even labelling relative to
    MID-VSYNC frames or to ParityFrame?

    **RESOLVED BY DEFAULT READING 2026-08-25** (fresh render pass pp.198-199, 205, 206, 216;
    no author answer). The labelling is **ParityFrame-relative**, and the line is generated at
    the end of the **even** (ParityFrame-even) frame's construction while being **duration-
    counted in the following odd frame**:
    - §19.6.2 p.216 (render): type 1 adds the line "at the end of the frame (after the R5
      lines if necessary)" iff R8∈{1,3} and **ParityFrame is even**; §19.6.1 p.216: type 0
      gates on **ParityR6 odd**, which "becomes odd when C4 reaches R6 on an even frame…
      An additional line will then be generated at the end of this even frame. The new frame
      becomes odd (equal to ParityR6)." With the R6>R4 freeze the gate state persists
      (line every frame if frozen odd, never if frozen even).
    - §19.5.1 p.205 (render): the same even frame carries both events — extra line "when the
      construction of the even frame is completed" and MID-VSYNC "when C4=R7 on the even
      frame"; §19.5.2 defines ParityFrame as "the parity of the first C9 of the frame". The
      p.206 table's column headers (PARITYFRAME=ODD / PARITYFRAME=EVEN) confirm the vocabulary.
    - §19.3 p.199 (render): "1 even frame with a duration of 19968µsec… 1 odd frame, which
      inherits the additional line from the even frame, and lasts 20032µsec" — the odd frame's
      313-line duration absorbs the line generated at the even→odd boundary. §19.3.1's "added
      at the end of the first frame" is the same boundary in a pair starting even; its "VSYNC
      not being 'shifted'" is the CPC/GA-effective statement (the GA's 2-HSYNC wait neutralises
      the half-line shift at the gun, "-0.0"), not a claim about CRTC pin timing.
    - Counter accounting: §19.6.1 — type 0 increments C4 once for all additional lines
      (R5 and interlace), C4=R4+1; §19.6.2 — type 1 increments C4 once more on even frames
      when R9+1 is a multiple of R5. Matches §11.2.4 p.84 ("added only at the end of a even
      frame").
    Actionable: the line is unimplemented on both types → finding **F14**
    (audit-findings.md). No RTL change until its failing fixtures exist.
11. **p.205 (§19.5.2) — even R9 total line count.** IVM programming with even R9 yields an
    even number of lines per character (R9=6 → 2×4 = 8 lines). Please confirm the intended
    parity of the *total* line count (even), since a distilled summary inverted this.

    **RESOLVED BY DEFAULT READING 2026-08-25.** The p.205 render states it directly: "In IVM
    mode, R9 is programmed with an **even** number to define an **even** number of lines in a
    character (For example R9=6 to obtain 2 x 4 line/char=8 lines)." Total lines per character
    is even; the distilled inversion was already corrected at D1. Nothing further to implement
    (the F10 even-R9 vectors pin this scheme).
12. **p.206 (§19.5.2) — odd-C4 IVM activation imbalance.** Activating R8=3 on an odd C4 can
    imbalance the VSYNC-delay correction for that transition frame. Does the source assert
    (or could you confirm) that the imbalance self-corrects on subsequent frames?

    **RESOLVED 2026-08-25 (answer: the source does not assert it).** The p.206 render's Note
    acknowledges the misbehavior without stating recovery: "If R8 goes to 3 on an odd C4, this
    can cause a phase of 1 line between the VSYNC of even and odd frames. Indeed, this VSYNC
    shift technique… only works properly to manage the line imbalance between an even C4 and
    an odd C4," and the worked example quantifies the transition frame only (C4=0 identical at
    8 lines; C4=1 gets 5 even lines on an odd frame vs 4 odd lines on an even frame). DRAWN:
    the source is silent on subsequent frames. INFERRED from the documented mechanics
    (§19.5.2): the parity state bits re-derive per frame — ParityFrame is re-anchored from
    ParityR6 at each C4=C9=C0=0 origin and ParityC9 is recomputed per character end — and
    carry no timing information, so the ±1-line perturbation cannot persist in them and the
    steady scheme resumes next frame; ParityR6 itself persists across frames by design (and
    freezes under R6>R4) but is a parity bit, not a timing state. The perturbation is a
    one-frame length change that shifts both fields uniformly — but that is our inference,
    not a sourced rule. No RTL impact while the transition-frame imbalance is
    unmodeled; if it is ever implemented, it needs its own fixtures and this question
    re-opens for the author.
13. **p.245 vs p.293 (§21.2.2 vs §28.1.9) — type-1 readable registers.** §21.2.2 documents
    readable R14-R17 (plus register 31); the identification chapter states that on CRTC 1
    "*all* registers return 0 except register 31". Which is authoritative — do R14/R15 (and
    R16/R17) really read back stored values on UM6845R?

    **RESOLVED 2026-08-26 FROM ACCC + INDEPENDENT CHIP DOCUMENTATION.** §21.2.2 is correct:
    CPC type 1 is UM6845R, R14/R15 are read/write cursor registers, and R16/R17 read the
    read-only light-pen latch. The UM6845R Figure 3 register summary in
    `docs/references/UM6845 Cathode Ray Tube Controller.md` marks exactly those access modes;
    the per-CPC-type table in
    `docs/references/The 6845 Cathode Ray Tube Controller (CRTC).md` agrees. R12/R13 remain
    write-only on type 1 and read as 0. §28.1.9 is the outlier and should say that all
    **other** registers return 0, with the separately observed undefined register-31 value.
    The RTL already reads back R14/R15. It has no LPSTB input or light-pen latch, so R16/R17
    currently fall through to 0 even though the real CPC routes the CRTC light-pen strobe to
    expansion pin 47. That is an unsupported hardware path, not evidence that zero readback is
    chip-correct; finding F18 owns the interface decision and any capture/readback fixture.
14. **p.293 vs pp.246-248 (§28.1.8 vs §21.3.3) — status bit numbering.** The identification
    test polls "the transition of **bit 6**", but §21.3.3's frame-timed examples transition
    bit 5, while the CRTC 3/4 STATUS-1 table (p.248) has a separate always-1 bit 6. Which bit
    does the &BE00 identification test actually target on UM6845R?

    **RESOLVED VISUALLY + CHIP DOCUMENTATION 2026-08-26.** The frame-timed identification bit
    is **bit 5**. The p.246 render and UM6845R Figure 3 both map bit 6 to `L` (light-pen
    register full) and bit 5 to `V` (vertical blanking/BORDER-R6 state). Thus bit 6 is not
    generally unused, but it cannot provide the described frame-timed transition without a
    light-pen strobe; §21.3.3's rendered examples toggle between `00100000` and `00000000`,
    directly proving bit 5. §28.1.8's "bit 6" is a typo for **bit 5**. The ASIC STATUS-1 bit
    6 on p.248 belongs only to CRTCs 3/4 and is a separate always-1 field. The RTL's bit-5-only
    status view is correct for a CPC with no light-pen event source.

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

    **UNRESOLVED SOURCE CONFLICT; HARDWARE DISCRIMINATOR REQUIRED 2026-08-26.** The detailed
    §§11.2.4/11.3.2 reading still predicts a maximum observable C4 of 38: the last normal row
    enters adjustment at C4=37, the first eight-line C9 wrap reaches 38, and final R5
    completion sets C4 directly to 0. That agrees with current `t08g`/`t08h`. But §28.1.1 is
    explicit that type 1/2 overrun several times and that VSYNC persists through R7=39; no
    available chip reference adjudicates that contradiction. Do not relabel the printed 39
    as a typo or derive new RTL from the current simulation. A hardware/faithful SHAKER sweep
    of R7=38 and R7=39 under the stated R4=36,R9=7,R5=16 program is the remaining gate.

18. **§13.6.2 p.122 vs §13.7.1.2 p.124 — R0-widening RFD arming condition.**
    The §13.6.2 "CRTC 1 : CHRONOGRAM" annotation reads "RFD activated on CRTC 1 if R4 and/or
    R9 **modified** until C0=7F (new R0) on last line of frame" (quoted verbatim from the
    PDF). That wording is ambiguous: read as a write-event condition it would arm even if the
    written value is later restored to the original, but the figure does not say so
    explicitly — that consequence is our inference from the word "modified". The dedicated
    §13.7.1.2 section defines its variants by state: "(C9 != R9 **by line end**)" /
    "(C4 != R4 **by line end**)". Our model follows the end-state reading (`t13h` pins
    cancel-then-restore as NOT arming), on the grounds that a real comparator has no
    "was written" latch and §13.7.1.2 is the dedicated section (review finding F-7,
    `f7-r0-widening-independent-review.md`). Please confirm which reading is right; SHAKER
    Module C `(1)` / D `(9)` can discriminate by restoring R9/R4 mid-extension before the
    widened line end.

    **RESOLVED BY DEFAULT READING 2026-08-26.** Arming is end-state based. The p.122 render
    contains no R9/R4 restore sequence at all; its rows vary only the R0-write acceptance
    timing. Its annotation says R4/R9 remain modified **until** C0 reaches the new R0=7F,
    which is consistent with persistence to line end. The dedicated §13.7.1.2 then states the
    discriminators explicitly as `C9<>R9 at the end of the last line` and `C4<>R4 at the end
    of the last line`, with the RFD triggered at that line end. A cancel-then-restore therefore
    does not arm. `t13h` and the existing end-state RTL model stand; the SHAKER cases remain a
    useful hardware confirmation but no longer block the source reading.

19. **p.219 (§19.8.1) — one-bit polarity of the type-0 IVM parity gate.** In the C9 reset
    branch the pseudocode reads `If R9.0=0` immediately followed by the gloss "(C9 parity
    switched if R9 is odd)", then `ParityC9 = C4.0 xor ParityFrame`. The token gates on R9
    **even**; the gloss, §19.5.2 (p.205: with an even R9 "the parity is identical regardless of
    the value of C4"; with an odd R9 the line parity "depends on that of C4 and on the current
    parity at the start of the frame"), the p.206 worked R9=7 example, §19.5.3's mirror-image
    statement for CRTC 1, and the pp.221-224 tables themselves (R9=6, no per-C4 alternation in
    any of them, which the literal token would forbid) all point to R9 **odd**. Is `If R9.0=0`
    a typo for `If R9.0=1`? Two related details in the same block: (a) the compared value is
    written "R9 or ParityFrame" on p.219 but "R9 or ParityC9" on p.220 — identical while R9 is
    even, different exactly in the alternating odd-R9 case; which one is the hardware
    comparison? (b) in the exit tables (p.223 bottom, p.224) the character keeps counting to
    C9=7 with R9=6 after IVM is left, without the C4 increment the parity-dropped
    `C9.VMA == R9` test would produce at C9=6 — illustrative padding, or does leaving IVM
    really add a line? (Render-verified 2026-08-24. The earlier three-contradiction form of
    this question rested on misreading pp.221-224 as R9=3 and is withdrawn.)

    **Main token + (a): RESOLVED BY DEFAULT READING 2026-08-25** (fresh render pass pp.219,
    220, 223, 224; no author answer). `If R9.0=0` is a typo for `R9.0=1` — the row-end
    ParityC9 update fires only when R9 is **odd**. The gloss, §19.5.2 p.205 (even R9 → parity
    identical regardless of C4, so no per-C4 update), the pp.221-224 R9=6 tables (no
    alternation in any panel), and the p.206 R9=7 example (per-C4 alternation) are unanimous;
    the literal token would make every one of them wrong. (a) is settled by the p.220 prose,
    which names all three phases: the **switch line** tests raw C9 against "R9 or
    ParityFrame"; **steady IVM lines** test C9x2+ParityFrame against "R9 or ParityC9"; the
    **exit/write line** tests C9.VMA against plain R9 ("ParityC9 is no longer considered for
    R9"). The p.219 pseudocode line `((C9 x 2) or ParityFrame) == (R9 + ParityFrame)` is the
    garbled one — its two ParityFrame terms cancel, which would make odd-R9 termination
    unsolvable. Our RTL already implements exactly this three-phase form (type-0 engine,
    t22-family). Actionable: the odd-R9 counting machinery (row-end ParityC9 update with the
    corrected gate, odd-R9 limit tests, and the §19.5.2 VSYNC delay correction) is
    unimplemented → finding **F15** (audit-findings.md); fixtures before RTL.
    (b): **RESOLVED BY VISUAL TABLE READING 2026-08-26.** After a non-matching R8=0 write, the
    line-end test keeps comparing the **frozen C9.VMA register content** against plain R9; it
    does not resume a live plain-C9 test. The eight rendered exit windows form a discriminating
    set with R9=6:
    - even-frame frozen values 0, 2 and 4 do not match 6 and run through plain C9=7;
    - the even-frame frozen value 6 matches R9 and resets exactly at the write-line seam;
    - odd-frame frozen values 1, 3, 5 and 7 all fail the plain-R9 comparison and run through
      C9=7, including frozen 7 (which would match only if parity were still considered).
    A live C9==R9 test would reset at C9=6 in all seven run-on windows and is therefore ruled
    out. The p.220 worked recipe, which programs R9 to the frozen C9.VMA value to recover the
    reset, independently agrees. One p.223 bottom-left cell remains anomalous: it ends at
    C4=2,C9=7, an impossible C4 increment without a C9 reset. It is most likely a table typo
    (C4=1 would match the other run-on windows), but that isolated anomaly cannot overturn the
    seven-window discriminator. Actionable: current post-exit RTL resumes the live plain-C9
    test; finding **F16** now owns the focused failing fixtures required before any RTL change.

20. **p.85 (§11.3.2) — C4 comparison during stuck $R_5=0$ vertical adjustment.** The text says:
    *"But if R5 becomes zero during additional management, the state is not deactivated, C4 does
    not return to 0 and C5 loops. C4, however, continues to be compared to R4 to process the change
    from C4 to 0. The additional management, however, remains activated. Thus, if C5+1 reaches an
    R5>0, then the additional management changes C4 to 0 before deactivating its state."*
    Does the sentence *"C4, however, continues to be compared to R4 to process the change from C4 to 0"*
    mean that C4 resets to 0 when it next cycles around and reaches R4 (while C5 continues looping
    and adjustment stays active), or does it simply restate the normal frame-end mechanism vs the
    stuck C5 looping condition where C4 free-runs until a reachable R5>0 deactivates additional management?

    **ADJUDICATED DEFAULT READING 2026-08-28 (Review finding N2):**
    The following sentence (*"Thus, if C5+1 reaches an R5>0, then the additional management changes C4 to 0
    before deactivating its state"*) explains that changing C4 to 0 occurs upon exiting additional
    management when a reachable $R_5>0$ is satisfied. During the stuck $R_5=0$ adjustment, $C_4$
    increments past $R_4+1$, free-running through 127 and wrapping to 0 by 7-bit overflow while
    adjustment remains active (`in_adj=1`). Test `t08j` pins this behavior.

Also noted while verifying (no answer needed, listed for completeness): p.195 places the
skew-delay-from-substitution note inside the CRTCs-1/3/4 paragraph — we assume the delay
applies to type 0's substituted trigger (the placement caveat cited by
`f6-decision-gate.md`). Corrections welcome.

Implementation note on Q4: the F7 RTL (`rtl/crtc_type1_engine.v:239-240,432-435`) arms at
`hcc_last`/C0=R0. On an ordinary R1<R0 line, the C0=R1 save-clear opportunity has already
passed, so the new source flag survives into the next line; required vector `t13d` pins that
behavior even with C9=R9. This is a direct F17 divergence from the resolved external rule,
not a same-edge-only residual.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
