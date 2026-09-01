# ACCC v1.11 Digest 02 — CRTC Sync Rules (R3, R2, R7, Interrupts; Chapters 14–16, 27)

Primary source: ["The Amstrad CPC CRTC Compendium" v1.11 French edition](../references/ACCC1.11-FR.pdf#page=132),
chapters 14–16 and 27. The [English edition](../references/ACCC1.11-EN.pdf#page=130) is a
working translation; see the [bilingual ledger](accc-1.11-fr-en-differences.md).
Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot (CC BY-NC-ND).
Scope: **CRTC type 0 (HD6845S/UM6845) and type 1 (UM6845R) only.** Type 2/3/4 contrasts are marked
`[Tn diff]`, one line, informational only. Counter names: **C0** = horizontal char counter, **C0vs**
= C0 as seen internally by CRTC for sync-condition tests, **C0 disp/C0ga** = C0 as seen by Gate
Array for display (≈ C0vs−1 on CRTC0/1/2), **C3l** = HSYNC width counter (low nibble R3), **C3h** =
VSYNC line counter (high nibble R3), **C4** = row counter, **C9** = scanline-in-row counter, **R52**
= GA's interrupt line counter (0–51), **V26**/**H06** = GA-internal HSYNC-count/char-count counters
for C-SYNC generation (not CRTC registers).

---

## 1. R3 register layout (§14.1, p.130)

- R3l (bits 0–3) = HSYNC width in char clocks (µs, 1 char = 1 µs @ 1MHz CRTC clock). R3h (bits 4–7)
  = VSYNC height in lines, meaningful on CRTC 0 only (`[T3/4 diff]` also on 3/4). CRTC 1 ignores R3h
  entirely — always 16-line VSYNC.
- C3l starts at 0 the instant C0vs reaches R2; HSYNC is asserted until C3l reaches R3l.
  The author's 2026-08-31 response confirms that French §14.1 p.132 saying HSYNC “begins”
  at that terminal count is a typo for **ends**; the promised French correction is not in
  the published v1.11 PDF.
- The p.130 bit-layout comparison table reads cleanly in the text layer (extraction-noise
  flag retired by the 2026-08-22 review); the rule above is prose-confirmed either way.

## 2. VSYNC length via R3h (§14.2, p.131)

- R3h=0 → 16 lines (legacy compat); R3h=1..15 → exactly that many lines. Applies to CRTC 0 (`[T3/4
  diff]` also 3/4). CRTC 1/2 always 16 lines regardless of R3h.
- CPC BASIC ROM sets R3=&8x (R3h=8): CRTC 0 gets 8-line VSYNC, CRTC 1/2 still get 16 with the same
  byte — real compat bug source (e.g. "3D Starstrike" cursor-phase bug on CRTC1/2).
- **Dynamic R3h rewrite during active VSYNC** (applies to CRTCs 0/3/4, where R3h is
  programmable; CRTC 1/2 ignore R3h entirely): C3h (4-bit) keeps counting regardless.
  - New R3h < value **already passed** by C3h → VSYNC ends at end of HSYNC of the *current* line
    (next C3h comparison catches it).
  - New R3h < value **not yet reached** by C3h → C3h keeps counting, wraps its full nibble (0..15,
    i.e. runs 16 lines total), then re-triggers the new count again (e.g. programmed-9→8 mid-flight
    at line 9 → net **exactly 24 lines** (16+8), not 8).
  - Scope/precision note (2026-08-22 review, B9): the section covers CRTCs 0/3/4 per the source,
    not CRTC 0 alone; and the already-passed / not-yet-reached split is this digest's
    generalization from the source's examples, not a quoted rule.
- **GA/CRTC decoupling (critical)**: GA's own C-VSYNC/black-window timing is a fixed 26-HSYNC
  sequence counted by GA's **V26** (incremented per CRTC HSYNC-end event, independent of R3h). CRTC
  0 programmed R3h=1 (1-line internal VSYNC) still gets the full 26-line GA treatment as long as
  HSYNCs keep arriving (confirmed again §16.1–16.2). `[T3/4 diff]` those types need the CRTC VSYNC
  pin to stay active through the 2nd post-VSYNC HSYNC for the ASIC to emit C-VSYNC at all.

## 3. HSYNC: Gate Array vs CRTC (§14.3, p.132–134)

- GA processing of a CRTC HSYNC pulse: (1) ~2 µs black border; (2) **C-HSYNC** monitor pulse, max
  **4 µs**; (3) if R3l>6, black again for the remainder until CRTC's HSYNC actually ends.
- **C-HSYNC's 4 µs window is capped by whichever comes first**: GA's own H06 reaching 4 µs (this is
  the stop condition when **R3l ≥ 6**), or CRTC signaling HSYNC-end (stop condition when **R3l <
  6**, giving a *shorter*, less-precisely-timed pulse, ±1–2 pixel-M2 jitter by CRTC type).
- **R3l=6 is the exact threshold**: C-HSYNC = exactly 4.0000 µs, JIT/NJIT converge, most precise.
- Measured C-HSYNC durations (p.133 — clean in pdftotext; digit-wrap flag retired, corrected
  reading per review B1): each slash pair is the **range of two observed values of the NJIT
  column** ("I indicated a range of 2 values", p.133), **not** NJIT/JIT: CRTC0 R3l=4→NJIT
  2.0625/2.125, R3l=5→3.0625/3.125, R3l=6→4.0000; CRTC1 R3l=4→NJIT 2.125/2.1875,
  R3l=5→3.125/3.1875, R3l=6→4.0000. The JIT column reads **+0.25µs above NJIT** throughout
  (e.g. CRTC0 R3l=4 → JIT 2.3125/2.375), consistent with the stated JIT delay.
- **Prose-confirmed rule of thumb**: R3l 4→5 gives exactly **1.0 µs** difference regardless of CRTC
  type/tolerance — preferred pair for exact pixel-scroll positioning over 5→6 (uneven ~0.875µs delta,
  imprecise since R3l=5 is below the R3l=6 threshold).
- Fine horizontal positioning via R2/R3l (CRTC0/1, prose p.133, exact):
  - R2 += 1 → shift **left** 16 px-M2 (1 char). R2 -= 1 → shift **right** 16 px-M2.
  - R3l += 1 (only if new value < 6) → shift **left** 8 px-M2. R3l -= 1 (only if new value > 2) →
    shift **right** 8 px-M2. Combine both for arbitrary sub-char C-HSYNC positioning.
- GA is only "almost master" of C-HSYNC: VSYNC-CRTC's sole role for GA is arming `VSYNC_GA=true`
  (§16.2.3); all subsequent GA timing is driven by HSYNC-end events, never by VSYNC pin level.

## 4. Updating R3 during HSYNC (§14.4, p.134–140)

- **General rule (all types)**: R3l rewritten to a value **less than current C3l** → C3l must
  **overflow its full nibble (wrap 15→0)** before reaching the new (smaller) target; HSYNC does
  *not* end early. **Exception: CRTC 1, new value = 0** cancels the current HSYNC immediately (no
  wait for overflow) — CRTC-1-specific (confirmed §14.4, §14.4.2, §14.4.4).
- **R3.JIT**: rewrite R3l = current C3l exactly when C0 is at the position corresponding to that
  C3l, to interrupt HSYNC surgically. Works CRTC 0/1/2; **does not work CRTC 3/4** (HSYNC
  synchronized to display, not free-running C3l).
  - Only via **OUT (C),r8** (I/O at 3rd–4th µs of instruction); **OUTI** (I/O at 5th µs) misses the
    window and does not achieve JIT.
  - CRTC 0/1: interrupting HSYNC's **first µs** with R3l=0 via JIT **prematurely ends** HSYNC
    (rather than the usual "delay end by 0.25µs" JIT behavior).
  - Note 1: rewriting R3=0 via OUTI (non-JIT-precise) instead **prevents HSYNC from starting** next
    cycle (same as static R3=0, §5) — does not cut an in-progress one.
  - Exact JIT end offsets (prose p.138): CRTC0 — HSYNC starts 5th px-M2, lasts 4 px-M2 (R3=0
    JIT'd); CRTC1 — starts 6th px-M2, lasts 3 px-M2. Note 2: in NJIT/OUTI mode, CRTC1's HSYNC ends
    1 px-M2 later than CRTC0/2.
  - pp.139–140 render-verified 2026-08-24: the prose offsets above match the diagrams
    (CRTC0's 40010 dark run = px-M2 4..7; CRTC1's = px-M2 5..7; GA 40007/8 runs one cell
    longer; CRTC4 carries a literal NO R3.JIT subsection).
  - `[T3/4 diff]` explicit "NO R3.JIT ON CRTC 4" subsection title confirms the negative.
- pp.135–137 render-verified 2026-08-24: the per-type dynamic-rewrite diagrams are legible and
  consistent with the prose overflow/wrap rules and the CRTC-1 zero exception above; CRTC3/4
  show no R3.JIT interruption.

## 5. Absence of HSYNC (§14.5, p.141)

- **R3l=0 static**: CRTC 0 and 1 → **no HSYNC at all** when C0=R2 (⇒ no interrupt trigger, since
  interrupts derive from HSYNC-end, §27). `[T2/3/4 diff]` those types instead get a 16 µs HSYNC
  (full nibble wrap) unless dynamically interrupted.

## 6. HSYNC start-up — R2 latch timing (§14.6, p.141–142)

- Static: HSYNC when C0==R2, R3l chars wide. R2 write latches during the **3rd µs of OUT(C),reg8**.
- **Display-stop position differs by pre-programmed vs JIT vs OUTI** (all relative to start of
  displayed char R2−1):
  - R2 programmed **before** C0=R2 (normal): black zone starts CRTC0 = 5th mode-2 pixel of char
    R2−1 (half of 4th observed showing); CRTC1 = 6th mode-2 pixel; `[T2 diff]` CRTC2 = 4th (half of
    3rd observed showing).
  - R2 updated exactly **at C0==R2 via OUT(C),r8** ("R2.JIT"): delayed further — black starts 9th
    mode-2 pixel (CRTC0/1) or 8th (CRTC2), i.e. **+4 mode-2 px (0.25µs) on CRTC0/2, +3 mode-2 px
    (0.1875µs) on CRTC1** vs pre-programmed case.
  - Same C0==R2 update via **OUTI** instead: HSYNC reaches GA **0.25µs faster** than OUT(C),r8 case
    — behaves like pre-programmed, NOT like the delayed R2.JIT case.
  - HSYNC *duration* (R3l count) unaffected by which start-timing case occurred — only the visible
    black-zone start position shifts.
  - `[T3/4 diff]` test is against C0vs but HSYNC deferred to align with GA's display of that char;
    R2.JIT via OUT(C),r8 does NOT delay the visible black zone here (already deferred by design).

## 7. HSYNC and interrupts (§14.7, p.142)

- Dynamic HSYNC-width changes shift exactly when the post-HSYNC interrupt trigger fires (interrupt
  = "GA triggers just after HSYNC end") — full rules in §23–27 (chapter 27) below.

## 8. HSYNC schematics — VSYNC-end/HSYNC-end interaction (§14.8, p.143–144)

- At VSYNC end (end of 26th GA-tracked HSYNC, §14): CRTC0/1 → black stops **1 px-M2 after** HSYNC
  end. `[T2/3/4 diff]` CRTC2/3/4 → black stops **at the same instant**.
- ⚠ p.144 — per-CRTC pixel-M2 tables (per CRTC: an R2-NJIT table with Z80A rows OUT(C),r8 /
  OUTI / OUTI plus an R2-JIT / OUT(C),r8 table; rows C0 from Vsync, C0 disp by Gate Array,
  Byte Offset, Pixel Mode 2 / 0,3 / 1) extracted as unusable pixel-index noise and remain
  visual-tier. No GA-model dimension exists on p.144 — the 40007/8-vs-40010 split lives in the
  §14.4.4 diagrams (pp.139–140); CRTC4's table has no total bar and CRTC3 is marked "to come".
  The aggregate totals below survived in the **pdftotext layer only** (pdf2md drops them) and
  were re-verified against the p.144 render 2026-08-24: CRTC0 NJIT=32 M2px (2µs), JIT=28 M2px
  (1.75µs); CRTC1 NJIT=32 M2px (2µs), JIT=29 M2px (1.8125µs); CRTC2 NJIT=33 M2px (2.0625µs),
  JIT=29 M2px (1.8125µs).

---

## 9. R2 general (§15.1, p.145–146)

- HSYNC-CRTC activates when **C0vs reaches R2**; width fixed by R3l.
- **CRTC-vs-GA distinction (critical)**: GA processes HSYNC faster than it displays chars — on
  CRTC0/1/2, HSYNC becomes visible **on the char preceding** R2 (GA still drawing char R2−1).
  `[T3/4 diff]` ASIC types align HSYNC with the actually-displayed C0=R2 char, i.e. **delayed 1µs**
  vs CRTC0/1/2 — with R2=10: CRTC0/1/2 start HSYNC at C0-displayed=9 (R2−1); CRTC3/4 at
  C0-displayed=10. This 1µs gap is why CTM640/644 calibration differs from CM14; ±1 on R2
  compensates when swapping monitor/board combos.
- CTM viewport: first visible char on the left = **15th char counting from C0=R2**, provided
  **R3>5**.
- C0vs vs C0-displayed distinction does **not** change internal counter/comparison behavior for
  R2/R3 updates — always reason in C0vs for latch timing, not GA's delayed display count.
- C-HSYNC timing (reconciles with §3): GA processing window max 6µs total; C-HSYNC sent to monitor
  1.875–2µs after HSYNC-CRTC start. **R3l==2** → pulse too short for monitor deflector lock;
  **R3l>2** → sufficient. Distortion triggers: HSYNC 2–6µs long (partial lock); multiple >2µs
  HSYNCs per line; HSYNCs not vertically aligned frame-to-frame.
- **Re-entrancy lockout (all types)**: while HSYNC-CRTC active, the C0==R2 test is disabled — a new
  HSYNC cannot start while one is in progress (mirrors R52/interrupt lockout, §27).

## 10. Updating R2 during HSYNC (§15.3, p.148–151)

- **General (all types)**: R2 rewrite during active HSYNC is ignored for starting a *new* HSYNC if
  doing so would restart HSYNC within the current one — avoids lock-up when R0 < R3 lets C0 revisit
  R2 multiple times per line.
- **CRTC0**: at the exact position where C3l reaches R3l (HSYNC end — black cutoff is
  sub-character-granular, from bit 3 of the displayed byte onward, not a clean char boundary), if
  C0==R2 again **and R3l was modified there**, a new HSYNC begins **without C3l resetting to 0** —
  starts at earliest approximately 3.5 px-M2 after the one that just ended, an R2.JIT-style
  restart rather than a universal exact offset. If R3l was
  *not* modified at that exact position, no new HSYNC restarts (this is what distinguishes CRTC0
  from the CRTC1/2/3/4 shared bug below).
- **CRTC1/2/3/4 shared bug (§15.3.1)**: if C0==R2 again exactly at **C0 = R2+R3** (one past normal
  HSYNC end), this triggers the infinite-HSYNC path (unconditionally, unlike CRTC0's require-R3l-
  modified gate).
- **Infinite HSYNC reproducer**: R0=0, R2=0, R3=1 (C0 pinned at 0 every char since R0=0, so C0==R2
  every cycle). CRTC0: HSYNC does *not* restart on the 2nd C0=0 occurrence, only the 3rd.
  `[T2/3/4 diff]` CRTC1/2/3/4: C3l overflows (15→0→1...) and if C0 is still==R2 when it wraps back
  to R3l, HSYNC continues indefinitely — **CRTC1 is susceptible to this bug**, CRTC0 is not.
- **CRTC1 (and 2) re-entry at C0=R2+R3l** (§15.3.4): C3l overflows (doesn't cleanly stop); an R3l
  update at this point is applied *immediately*, bypassing the missing zero event.
  - **CRTC1**: has enough timing margin to emit a brief "invisible" HSYNC-off pulse then
    immediately re-assert HSYNC; GA's H06 resets and emits a **second separate C-HSYNC pulse** from
    this 2nd position — transition too fast to see visually as two lines.
  - `[T2 diff]` **CRTC2**: no such margin — GA sees one continuous unbroken black run instead of two
    pulses. **Real, testable CRTC1-vs-CRTC2 divergence** worth asserting if the Verilog model
    distinguishes types.
- pp.149–150 render-verified 2026-08-24: per-value scenarios (R2 rewritten 17..22, R3 fixed=10)
  are cleanly separated in the diagrams and support the prose rules above, including R2=21
  (CRTC0 restarts HSYNC when R3l is modified; CRTC1 emits two monitor-sync pulses where CRTC2
  shows one continuous run — the testable divergence named above). Source artifact noted: one
  p.150 scenario omits its trailing `10/R3` cell and one row carries a duplicated `53`.

## 11. VSYNC consideration during HSYNC (§15.4, p.152–155)

- **CRTC0/1**: C4==R7 VSYNC condition is evaluated on **any C0 value** while it holds true —
  whether reached by natural C4 increment or by rewriting R7 to match current C4 mid-scanline
  (including via the interlace "additional line" path). `[T3/4 diff]` those types evaluate **only
  on the increment edge** (rewriting R7 to an already-equal C4 does not retrigger).
- **No HSYNC/VSYNC pin conflict for CRTC0/1** (unlike CRTC2, below) — VSYNC pin can assert normally
  even mid-HSYNC.
- p.152 render-verified 2026-08-24: the encroachment diagrams (R2=50 with R3=12..15) are legible
  and show HSYNC and VSYNC coexisting without conflict, matching the prose rule.
- `[T2 diff — GHOST VSYNC, §15.4.4, informational only]`: CRTC2 evaluates VSYNC on all C0/C9; if
  landing inside active HSYNC (C0=R2..R2+R3l+1), produces a GHOST VSYNC (internal line-count
  proceeds, blocks real VSYNC, pin never asserted to GA — pins 39/40 conflict). Exception: R2==0
  detects early enough to fire for real.

## 12. Border and HSYNC (§15.5, p.155)

- CRTC0/1 (and 3/4): background/BORDER-restore logic (C0==0 test, C0==R1 to re-enable border) is
  evaluated unconditionally, including mid-HSYNC. No special interaction to model.
- `[T2 diff]` CRTC2: the C0==0 restore-background test is **suppressed during active HSYNC** — if
  HSYNC spans C0=0, the restore doesn't fire and prior state persists an extra pass.

## 13. R2 dynamic update — "the right moment" (§15.7, p.156–157)

- Uncompensated mid-frame R2 rewrite → monitor re-syncs horizontally over several lines (visible
  gradual drift), same phenomenon as an out-of-range R3l change (§3).
- Mitigation: also adjust **R0** so C0 returns to the *new* R2 at the same wall-clock moment the old
  C0=R2 used to land — shrink R0 if new-R2 > old-R2, enlarge if new-R2 < old-R2 (horizontal analog
  of the R4/R7 vertical trick, §22).
- p.157 render-verified 2026-08-24: both OUT sequences are fully legible and realize exactly the
  compensation principle above (shrink to R0=59 when going 46→50; enlarge to R0=67 when going
  50→46), keeping the R2 hit at the same wall-clock column. A duplicated `53` cell in §15.7.2's
  last row is a source typo.

---

## 14. R7 general (§16.1, p.158–159)

- VSYNC-CRTC activates when **C4 reaches R7** (per-type exceptions, §19).
- **GA's V26 state machine (core CRTC-vs-GA distinction)**:
  - On VSYNC-CRTC active: GA sets **V26=0**.
  - **V26 increments once per HSYNC-end event**, regardless of that HSYNC's programmed R3l width
    (even a 1µs HSYNC still counts).
  - V26==2 → GA activates composite C-SYNC. V26==6 → deactivates C-SYNC (4 HSYNC-widths of active
    retrace, nominally 4×64µs=256µs at R0=63). V26==26 → GA's black-hold (`CBLACK_VSYNC`) clears and
    `VSYNC_GA` clears — GA-side VSYNC processing "complete," permits re-arming (§17).
  - **Decoupling**: CRTC's programmed VSYNC width (R3h, or fixed 16 lines on CRTC1) is functionally
    independent of GA's 26-HSYNC black/sync window, as long as HSYNCs keep arriving — R3h=1 on
    CRTC0 still gets the full 26-line GA treatment (confirmed again p.163).
- Background restore after VSYNC ends (26th HSYNC processed): CRTC0/1 → restarts **1 px-M2
  (0.0625µs) after** HSYNC end. `[T2/3/4 diff]` restarts at same instant.
- First visible line on CTM: **34th line** from VSYNC start (2nd scanline of 5th 8-line char row).

## 15. C-SYNC algorithm — GA state machine, exact (§16.2.2–16.2.3, p.162–165)

```
If VSYNC-CRTC Transition OFF->ON:
    V26=0 ; VSYNC_GA=true ; CBLACK_VSYNC=true

If HSYNC-CRTC Transition OFF->ON:
    H06=0 ; CBLACK_HSYNC=true

If HSYNC-CRTC Transition ON->OFF:
    SIG_GA_HSYNC=LOW ; CBLACK_HSYNC=false
    If VSYNC_GA==true:
        V26++
        If V26==2: SIG_GA_VSYNC=HIGH   ; (also drives CRTC-VSYNC pin on CRTC3/4)
        If V26==6: SIG_GA_VSYNC=LOW
        If V26==26: CBLACK_VSYNC=false ; VSYNC_GA=false

If CRTC Transition Character (once per char clock while HSYNC-CRTC ON):
    H06++
    If H06==2: SIG_GA_HSYNC=HIGH   ; (the "2µs black then C-HSYNC" from §3)
    If H06==6: SIG_GA_HSYNC=LOW    ; (4µs C-HSYNC cap from §3)

BLACKCOLOR = CBLACK_HSYNC or CBLACK_VSYNC
CSYNC = SIG_GA_HSYNC XNOR SIG_GA_VSYNC
```

- Polarity: HSYNC-CRTC/VSYNC-CRTC pins active-**high**. C-SYNC to monitor active-**low**, built via
  **XNOR** (not AND) — lets HSYNC pulses keep appearing (polarity-inverted) *during* the VSYNC
  window, required for monitors to keep horizontal lock through vertical retrace.
- H06/V26 are independent GA-internal counters (GA cannot see CRTC's C3l) — hence GA must
  self-count chars-since-HSYNC-start to know when to flip C-HSYNC, which is why the 4µs C-HSYNC cap
  is decoupled from R3l when R3l>6 (§3).
- Author-noted numeric coincidence (not a hard rule): H06 active window 2..6, V26 active window
  2..6 of 0..26 — speculated shared flip-flop hardware in real GA ASIC; do not encode as a
  constraint, informational only.

## 16. VSYNC-CRTC vs VSYNC-GA — display-area timing, pre-set vs JIT R7 (§16.2.1, p.159–160)

- **R7 pre-set before natural C4=R7** (normal case): black starts CRTC0/2 = 5th pixel of VMA word
  preceding C4=R7 (8 px of BORDER shown instead of 2nd byte of that word); CRTC1 = 6th pixel.
  `[T4 diff]` CRTC4 = 2nd pixel.
- **"R7.JIT"** (program R7 with the **current C4**, thereby making R7=C4; does **not** work
  CRTC3/4, whose VSYNC also requires C9=C0=0):
  - CRTC0: black starts 5th px-M2 of the word where R7 becomes==C4 (bit 3 of 1st byte), regardless
    of OUT(C),r8 vs OUTI — but actual VSYNC-pin activation/C-SYNC delayed **1µs** (R7 not considered
    fast enough for immediate effect).
  - CRTC1 (and 2): via **OUT(C),r8** → black starts 9th px-M2 of the word *preceding* the R7==C4
    position (bit 7, 2nd byte of word at "C0−1"). Via **OUTI** → 5th px-M2 of that same preceding
    word (bit 3, 1st byte) — **OUTI is 4 px-M2 (0.25µs) earlier** than OUT(C),r8, same asymmetry
    pattern as R2.JIT (§6).
  - p.160 render-verified 2026-08-24: the R7.NJIT light-cell counts (CRTC0=4, CRTC1=5, CRTC2=4,
    CRTC4=1 leading cells before black) and both R7.JIT tables (CRTC0 identical ~20-cell extents
    under either instruction; CRTC1/2 OUTI 4 px-M2 earlier than OUT(C),r8) match the prose
    bullets above.
- Reaffirms §14.2/§16.1 decoupling: R3h=1 or CRTC VSYNC cut to 2µs still gets full 26-line GA hold.

## 17. VSYNC re-trigger while GA-VSYNC in progress (§16.2.5, p.166)

- If a new VSYNC-CRTC pulse arrives while GA's own V26 sequence is still running (V26<26), **GA's
  V26 resets to 0** and restarts its 2/6/26 sequence, incrementing again per subsequent HSYNC-end.
  This is separate from the CRTC's own internal re-entrancy protection (§18) — GA only watches the
  VSYNC-CRTC pin transition, independent of the CRTC's internal blocking state.
- p.166 — worked numeric trace (CRTC0/1/2, R7 reprogrammed mid-VSYNC at C4=12/C9=4) showing
  GA-counter restart is mostly legible in extraction (flag softened by the 2026-08-22 review);
  the restart rule in prose is trustworthy regardless.
- `[T3/4 diff]` those types additionally require the CRTC VSYNC pin to stay physically active for
  the ASIC to keep emitting C-VSYNC (mirrors ASIC's C-HSYNC dependency). Not applicable to CRTC0/1,
  where GA timing runs off HSYNC-end events only once armed.

## 18. VSYNC protection / re-entrancy (§16.3, p.167)

Two independent mechanisms against infinite VSYNC:

- **Mechanism 1 (all types incl. 0/1)**: C4==R7 comparison is **not evaluated while VSYNC-CRTC is
  already active** — cannot trigger or inhibit a VSYNC during an active one. Rewriting R7==current
  C4 mid-VSYNC does not nest a new one; rewriting R7 to a different value does not cancel the
  running one (it completes its natural R3h/16-line course regardless of R7 edits mid-flight).
- **Mechanism 2 (present on CRTC 0 and 1; explicitly NOT reimplemented on CRTC3/4)**: additionally
  requires the **C4==R7 truth value to have changed** (via C4 incrementing away-and-back, or R7
  rewrite) before a *new* VSYNC can fire — even after the running VSYNC ends, if C4 is still==R7
  with no intervening change, **no new VSYNC fires** until the comparison's truth value flips at
  least once.
  - Worked lock example: R7=0, R4=0 (C4 pinned at 0 every line) → VSYNC fires once at C4=R7=0, then
    **never refires** (truth value never changed) until R7 or R4 changes.
  - **Worked bypass — real reachable bug on CRTC 0/1 (important test vector)**: R7=0, R4=1 → VSYNC
    fires at C4=0. With R3h=0 (16-line VSYNC) and R9=7: C4 increments to 1 on line 9 of the VSYNC,
    back to 0 on line 17 — since C4==R7 truth-value legitimately *changed* (false-then-true) inside
    that window, mechanism 2 is defeated and VSYNC restarts immediately on completion → **infinite
    VSYNC**. Test vector: **R7=0, R4=1, R9=7, R3h=0**.
  - GA/ASIC backstop regardless: even with the CRTC pin stuck high, GA (CRTC0/1/2) only re-arms a
    new V26 sequence once the prior one reaches its 26th row (§17) — monitor-visible C-VSYNC still
    repeats at a bounded rate.

## 19. Per-type exact R7 latch rules (§16.4, p.168–170)

- **Universal**: R7 can be rewritten with C4's value up to the **last µs preceding** the natural
  C4==R7 transition and still correctly arm VSYNC for the next match.

### CRTC 0 (§16.4.1)

- VSYNC starts at C4==R7. If reached exactly when **C0vs==0**, VSYNC starts at C0vs=0.
- **R7=C4 rewrite mid-line**: triggers VSYNC **immediately** if not already active — **except** if
  the rewrite lands at **C0vs==0 or 1**, which produces a **"BLOCKED VSYNC"**: mechanism-2 latches
  as already-seen but **no actual pulse occurs**; VSYNC cannot occur on this C4=R7 condition again
  until an unblocking event (C4 or R7 change). **Do not confuse with CRTC2's GHOST VSYNC** (§11) —
  different mechanisms (CRTC0's is pure mechanism-2 of §18; CRTC2's is the HSYNC/VSYNC pin conflict).
  - If rewrite lands at **C0vs>1**: triggers **mid-line**; the VSYNC scanline counter starts
    counting from 0 **at the start of the next line** (C0==0), not immediately. Net VSYNC duration
    is **increased** by `R0 − C0vs` µs vs a line-aligned start. Example: trigger mid line-1 → ends
    at end of line **17** (base 16-line VSYNC assumed). PPI Port B read 6µs after a rewrite at
    C0vs=&36 shows VSYNC active; read 6µs after a rewrite at C0vs=&00 shows VSYNC **inactive**
    (hits the C0vs<2 blocked case).
- **R0 interaction**: C0 must be able to reach value **2** on the line preceding the C4=R7 line for
  VSYNC to be considered at all. The following two cases appear only in English v1.11
  §16.4.1.2 p.169 and are absent from French p.170. The author confirmed on 2026-08-31 that
  they are normative and were intended for French; use English p.169 as the current published
  anchor. The 2026-09-01 consequence audit found that steady R0=1 lacked the preceding-line
  qualification; failure-first `t02l` now pins the corrected behavior. Native-review follow-ups
  `t02m`-`t02o` pin the exact dynamic writes and blocked-comparison consumption:
  - R0 (was >2) set to **0** exactly at C0=0 of the C4=R7 line: VSYNC starts at C0=0 but **C3h
    freezes** (like all CRTC0 counters at R0=0) — never reaches R3h, VSYNC **never deactivates**
    (distinct lockup from §18's re-entrancy bug).
  - R0 (was >2) set to **1** at that same C0=0: VSYNC starts at C0=0, C3h *can* still increment once
    (on C0 wrap 1→0); if R3h=1, VSYNC correctly stops after **2µs total** — sufficient to arm GA's
    V26 sequence regardless (§14/§16.1 decoupling).

### CRTC 1 (§16.4.2)

- VSYNC starts at C4==R7. **No C0vs<2 exception** — R7=C4 rewrite triggers VSYNC **immediately,
  unconditionally**, at any C0vs.
  - PPI latency: rewrite at C0vs=&36 → next read **5µs later** (C0=&3B, vs CRTC0's 6µs) shows
    active. **5-vs-6µs PPI-latency delta is a concrete testable CRTC0-vs-CRTC1 divergence.**
- **Mid-line trigger counts opposite sign from CRTC0**: counts current partial line as if VSYNC
  started at C0=0 → total duration **reduced** by `C0+1` µs vs line-aligned start (contrast direct
  with CRTC0's *increase* by `R0−C0vs`). Example: trigger mid line-1 → ends at end of line **16**
  (not 17). **Single most implementation-relevant CRTC0-vs-CRTC1 divergence in this chapter** —
  verify the Verilog model branches this calculation by type.

### CRTC 2 (§16.4.3) — contrast only, GHOST VSYNC precisely documented

- VSYNC evaluated on **all** C0/C9 with C4==R7 (CRTC0/1 only meaningfully evaluate around the
  transition/rewrite moment — CRTC2 evaluates every cycle, including deep inside HSYNC).
- Landing inside HSYNC window `C0=R2..R2+R3` (**+R3**, one past visible end, reflecting the C3l
  overflow-non-reset quirk of §10) → **GHOST VSYNC** (internal count proceeds, blocks real VSYNC,
  pin never asserted). R2==0 detects early enough to fire for real instead.
- Workarounds: park R7 far out (e.g. 127), rewrite R7=C4 only when C0 known outside HSYNC window;
  or shrink R3 at the last moment (riskier — can also break C4 rollover at frame end, §12).

### CRTC 3/4 (§16.4.4) — contrast only

- VSYNC requires **C4==R7 AND C9==0 AND C0==0** simultaneously (strict frame-boundary alignment).
- No re-entrancy protection at all — persistent true AND-condition restarts VSYNC immediately (full
  infinite-VSYNC exposure, worse than CRTC0/1's partial mechanism-2 protection).
- VSYNC must last **≥3 lines** for C-VSYNC to be generated at all.

## 20. Delayed VSYNC in interlace modes (§16.5, p.171)

- CRTC0 and CRTC1: interlace mode can delay VSYNC from the nominal C4==R7 transition:
  - **Half-line delay** on **even** frames: VSYNC occurs at C4==R7 **and C0==R0/2** (not C0==0).
  - CRTC0 additionally: **full extra line** delay for IVM mode (R8=3) with R9 odd, on an odd frame,
    odd C4 (cross-ref ch.19.5.2, outside this excerpt's range).
  - `[T1 note]` **CRTC1 does not correctly sync IVM-mode frames when R7 is odd** on a frame built
    from odd-scanline chars (R9 even) — documented CRTC-1-specific interlace deficiency, absent on
    CRTC0/3/4.

## 21. Limitless VSYNC / interlace positioning trick (§16.6, p.172–174)

- GA's composite C-SYNC is timed off its own H06/V26 state machine (§15), normally whole-line
  aligned — this would otherwise mask the CRTC's raw half-line interlace VSYNC (§20).
- Trick: force the **2nd HSYNC after VSYNC-CRTC start** to land at **half the normal line distance**
  (delay R0/2 µs) to recover correct interlace C-VSYNC timing; must separately compensate the
  resulting line-length deficit (e.g. R0 adjustment on a later line) to avoid CTM timebase drift.
  - Generalizes to any of 64 sub-line positions → **1/64-pixel vertical positioning precision**
    (SHAKER 2.1+). Confirms GA's C-VSYNC timing is driven purely by HSYNC-end event timing, no
    direct dependency on CRTC's own VSYNC pulse width once armed (reinforces §14/§16.1).

## 22. R7 dynamic update — "the right moment" (§16.7, p.174)

- Same category as R2 mid-frame updates (§13): uncompensated R7 rewrite → monitor sees
  multiple/missing VSYNCs → visible frame breaks/rolling.
- Mitigation: adjust **R4** so C4 returns to the *new* R7 landing at the same absolute frame time
  the old C4==R7 used to — shrink R4 if new-R7 > old-R7, enlarge if new-R7 < old-R7 (vertical analog
  of the R0/R2 trick, §13).

---

## 23. Chapter 27 — GA's R52 interrupt counter, management (§27.1–27.2, p.283)

- **R52**: GA-internal counter, 0–51 (52 states), entirely GA-side state (not a CRTC register); CRTC
  only supplies the HSYNC-end events that drive it.
- **Increment**: once per HSYNC-end event received from CRTC, per CRTC's own R0/R2/R3 programming.
  **No minimum HSYNC width** — even R3=1 (1µs HSYNC) counts.
  - Given HSYNC re-entrancy protection (min ~2µs gap between HSYNC-end events per §9's lockout),
    R52 can loop in as little as **104µs minimum** (52×2µs) in a pathological config; ordinary
    64µs/line operation gives interrupts every **3328µs (300Hz)** = **6 per 50Hz frame** (6×52=312
    lines).
  - **Bug**: HSYNC re-entrancy protection is itself buggy on CRTC types 1–4 (not 0), permitting
    infinite HSYNC (R0=R2=0, R3=1, cross-ref §10) — indirectly affects R52 cadence if such a config
    is used. Relevant to CRTC1 in our scope.
- **Reset-to-0** (any of): (1) natural overflow past 51; (2) software — GA **RMR bit 4 set to 1**
  (`OUT &7Fxx` with the interrupt-reset control bit, doc shorthand "10xIRrmm on #7f00"); (3)
  hardware — **unconditionally at the end of the 2nd HSYNC after VSYNC start** (§24.2), independent
  of R52's current value.
- **Bit 5 of R52 clears to 0 when**: a pending interrupt gets authorized (accepted) while R52 kept
  incrementing meanwhile. Bit 5 functionally = "R52 ≥ 32" flag.
- **RMR-reset-vs-increment race, exact latch rule**: RMR reset request landing on the **last µs of
  active HSYNC** (C0=R2+R3−1) → **reset has priority over the pending increment**, R52 ends at 0.
  Same request landing **1µs earlier** (C0=R2+R3−2, still mid-HSYNC) → R52 zeroed by RMR
  immediately, **then separately incremented** by the HSYNC-end event at R2+R3−1 → net R52=**1**.
  **Cycle-exact ordering rule** — a behavioral model must apply RMR-reset and HSYNC-end-increment in
  the correct relative C0 order, not "whichever happens last in program order this frame."

## 24. Interrupt trigger conditions (§27.3, p.283–284)

### 27.3.1 — R52 wraps 51→0

- GA requests interrupt when R52 increments 51→0.
  - If interrupts **enabled** at that instant: interrupt taken immediately, bit 5 cleared (no-op,
    already 0 post-wrap).
  - If **disabled**: R52 keeps incrementing, request stays **armed** (GA holds INT) until Z80 **EI**.
    Then: interrupt fires **after the instruction following EI** (standard Z80 EI-shadow; a HALT
    right after EI here counts as **1 NOP** for shadow purposes). **Bit 5 clears at the "real" end
    of that post-EI instruction.** If R52 had already reached ≥32 (bit5=1), clearing bit5 is
    equivalent to **subtracting 32** from R52 — shifts the next interrupt's timing forward.
    **Enforced minimum spacing: ≥20 lines between two consecutive interrupts** (52−32=20 floor).

### 27.3.2 — Trigger 2 HSYNCs after VSYNC start

- GA requests interrupt at end of the 2nd HSYNC after VSYNC-CRTC starts, **only if bit5 of R52==1**
  (R52≥32) at that moment. In a correctly-formatted 312-line frame R52 naturally reads 0 here (exact
  6×52 HSYNCs since last reset) so this path is normally **false**/dormant — exists as a safety net
  for malformed frame timing.
  - **R52 itself resets to 0 unconditionally** at this point regardless of whether the interrupt
    fires (bit5 only gates the interrupt *request*, not the reset).

### 27.3.3 — Z80 EI/DI/HALT semantics (context, not CRTC-specific)

- EI arms interrupt-taking; on interrupt, IFF flags clear (implicit DI) — only one interrupt
  pending/serviced at a time; R52 keeps counting underneath regardless of IFF state.
- Instruction immediately after EI cannot itself be interrupted (guarantees RET/RETI/RETN complete
  atomically). Repeated EI;EI;... defers the pending interrupt once per EI.
- HALT loops on internal 1µs NOPs until an interrupt; if interrupts disabled while HALTed, CPU
  **hangs indefinitely**. (Trailblazer's split-raster HALT usage cited as the practical exception —
  values HALT's cycle-exact interrupt landing.)

## 25. Interrupt modes 1/2 — Z80 background (§27.4–27.5, p.284–286)

- **IM1** (dominant CPC convention): interrupt ≈ implicit `RST #38`. RST #38 from code = 4µs; same
  vector taken via a real interrupt = **5µs** (1µs ack overhead).
- **IM2** (vectorized, rare — "The Great Escape"): vector high byte = Z80 `I` reg, low byte from
  peripheral (floating/undetermined on stock CPC — must build a 257-byte table of one repeated value
  to tolerate the undefined low bit). Interrupt vector-fetch overhead: **7µs**.
- Neither affects CRTC-side modeling directly; included as prerequisite context for §27's latency
  reasoning (IM1's 5µs figure is used implicitly in §26/27).

## 26. CRTC & interrupts — per-type latch timing (§27.6, p.286–288)

- **Universal**: interrupt always begins (GA requests it) **1µs after HSYNC end**, for every CRTC
  type — but "HSYNC end" itself lands at different C0vs per type, per §9's established latch rules:
  - CRTC0/1/2: HSYNC begins ~1µs *before* GA displays the corresponding char C0=R2 (GA-faster-than-
    display pattern). GA display resumes from **C0-displayed = R2+R3−1** (except R3=0-means-no-
    HSYNC on CRTC0/1, §5).
  - `[T3/4 diff]` CRTC3/4: HSYNC begins exactly *at* start of GA's display of char C0=R2 (no lead)
    → **with identical R2/R3, interrupt lands exactly 1µs later on CRTC3/4 than on CRTC0/1/2** —
    direct, testable, single-µs divergence.
- Worked C0vs-relative interrupt timings for CRTC0/1/2, cleanly extracted (p.287):
  - R3=14 → interrupt at **15µs** after C0vs=R2. R3=8 → **9µs**. R3=1 → **2µs**. R3=0 → **no
    interrupt** (CRTC0/1 no-HSYNC case, §5).
  - `[T2/3/4 diff]` R3=0 variant showing interrupt at 17µs (CRTC0/1/2 numbering) or 18µs (CRTC3/4):
    this is the contrasting case where R3=0 instead produces a 16µs HSYNC (§5), so C3 wraps its full
    nibble before HSYNC ends.
  - CRTC3/4 equivalents for the same R3 set are uniformly **+1µs** vs CRTC0/1/2 (16, 10, 3, 18µs for
    R3=14/8/1/0) — consistent with the universal offset rule above.
- No ⚠ flag needed for these specific interrupt-timing figures — unlike ch.14/15/16's pixel-M2
  tables, these rows survived extraction with clear per-scenario R3 labels and are trustworthy.
- **Practical implication**: HSYNCs with **R3≤2** don't produce a monitor-visible C-HSYNC pulse long
  enough to matter (§3/§9's <2µs rules) — it's possible to generate GA interrupts landing **inside
  the visible display area**, not just during blanking. A behavioral model must not assume
  interrupts only occur during border/blanking time.

## 27. "Threesome" — R52/EI race and interrupt reliability (§27.7, p.288–291)

### 27.7.1 — R52-vs-bit5-clear race (exact worked case)

- R52=31, and simultaneously: (a) GA receives an HSYNC-end that would increment R52→32, and (b) GA
  is told (via post-EI ack) to clear bit5. Relative order is **execution-time-dependent** (depends
  on how long the post-EI instruction actually took):
  1. **Increment-then-clear**: 31→32, then bit5 cleared → R52=**0**. Next interrupt blocked for
     **52 lines** (full period).
  2. **Clear-then-increment**: bit5 of 31 cleared (no-op, bit5 of 31=0b011111 already 0), then
     31→**32** → R52=32 (bit5=1). Next interrupt blocked for only **20 lines** (matches §24's floor).
  - **The identical input state (R52=31, simultaneous HSYNC-end + EI-ack) can legitimately produce
    two different next-interrupt-timing outcomes** depending on real instruction-execution jitter —
    doc explicitly calls this bad news for cycle-exact emulation. **A behavioral Verilog model
    should pick one deterministic ordering and document the choice** — this is inherent
    nondeterminism in the reference hardware, not a simple bug to fix.
- Handshake detail: GA's INT sampled against Z80's IFF1; once accepted, Z80 asserts IORQ+M1
  together; GA's "M1 ends" (for bit5-clear timing purposes) is **after all Z80 TWait cycles**, i.e.
  after full execution of the post-EI instruction.

### 27.7.2 — Reliability: HSYNC-end vs Z80 T-state alignment

- GA @ 16MHz is system timing master (derives Z80's 4MHz); Wait-state pattern stretches every Z80
  instruction (Z80 "free" only 4/16MHz = 0.25µs = 1 T-state per group).
- **Core caveat**: CRTC HSYNC-end timing is not perfectly deterministic across units — **especially
  CRTC type 1**, where "disparities exist between models of the same type and different types" (unit
  -to-unit variance at 1/16MHz=0.0625µs granularity is a documented real-hardware phenomenon, not a
  modeling error — no single canonical "CRTC1 timing" exists).
  - By contrast: "no differences noted between different CRTC type 0 units" — **CRTC0 used as the
    timing benchmark** specifically for this consistency. CRTC2/3/4 generally match CRTC0 closely;
    **CRTC1 is the outlier**. GA model (40007/8 vs 40010) can also shift fine timings by up to
    1/16MHz between units, compounding CRTC1 variance.
- **Exact GA-vs-Z80 T-state alignment rule** (pp.289–290, full cycle tables in source):
  - If HSYNC-end (and GA's INT assertion) occurs **at or before** the first 1/16MHz cycle falling
    under Z80 T-state **T3** of the current instruction: GA has lead time to present INT in time —
    **interrupt taken at the end of the current instruction**, on schedule.
  - If HSYNC-end occurs **after** that window (even by one 1/16MHz tick = 0.0625µs): Z80 does not
    see INT in time — it executes **one additional instruction** before the interrupt is taken.
    Worked example: two back-to-back `ADD HL,DE` — borderline HSYNC-end timing causes the interrupt
    to slip past the first and land after the second.
  - **NOP (and by extension HALT)** are cited as robust/safe against this jitter (their T-state/GA-
    cycle alignment keeps the "first GA cycle" comfortably ahead of the last T-state boundary).
    Other instructions are more exposed — e.g. `SET n,(IX+n)`, official 23 T-states, stretched to
    27–28 T-states on real CPC by GA wait-insertion — missing an interrupt here slips it a full
    extra **7µs**.
  - **Practical modeling takeaway**: a cycle-accurate interrupt model must compare the exact
    16MHz-tick timestamp of HSYNC-end against the Z80 instruction's T3-boundary tick, not just
    "which µs" — sub-microsecond (0.0625µs) alignment decides whether an interrupt lands this
    instruction or the next. A µs-granular model **will get this wrong** for borderline cases;
    document the limitation if not modeling at this granularity.
- Author's verdict (p.291): "it is risky to rely on the temporal position of an interrupt as it can
  occur at the very end of an instruction" — even the reference hardware doesn't guarantee exact
  interrupt timing to sub-instruction granularity; treat behavioral-model interrupt timing at this
  granularity as best-effort, not hard spec.

---

## ⚠ Summary of VERIFY flags (poorly-extracted figures/tables)

Retired by the 2026-08-22 faithfulness review (text layer carries the content — B7 in
findings-review.md): p.130 (R3 layout), p.133 (duration table — reinterpreted, §3/B1),
p.166 (restart trace, mostly legible).

Retired by the 2026-08-24 D1 visual re-verification (render-checked against
`docs/ACCC1.10-EN.pdf`; inline notes in the sections above): pp.135–137 (R3-during-HSYNC
dynamic-update diagrams), pp.139–140 (R3.JIT pixel-M2 positioning), pp.149–150 (R2-during-HSYNC
per-value diagrams), p.152 (VSYNC-during-HSYNC encroachment), p.157 (R2 46↔50 OUT-sequence),
p.160 (R7.JIT pixel-M2 positioning).

Still flagged:
- p.144 — per-CRTC pixel-M2 tables; per-row detail remains visual-tier, but the aggregate totals
  were re-verified against the render (see §5).

All other rules are drawn from clean prose extraction. Where the source itself documents inherent
hardware nondeterminism (§23's RMR race, §27.7.1's R52 race, §27.7.2's CRTC1 unit variance), this is
called out explicitly rather than treated as an extraction artifact.
