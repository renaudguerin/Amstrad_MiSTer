# Findings review — Fable's ACCC v1.10 distillation vs the source document

> **Later evidence:** The author's direct 2026-08-31 response to round-2 Q20 clarifies the F8
> R5=0 sentence: adjustment remains active and C5 loops, but the ordinary C4==R4 reset still
> applies. See `accc-author-response-round2-2026-08-31.md`; this dated source review otherwise
> remains unchanged.

Reviewed 2026-08-22 against `docs/ACCC1.10-EN.pdf`
(SHA-256 `1bd6f0e3a06022d03fd40b51d4d622afef2675954a483780f0922cdf1e33a560`, 295 pp).
Method and tooling: `docs/plans/2026-08-22-accc-review-plan.md`; extraction manifest:
`extract/README.md`. Verdict scale: **confirmed / partly-confirmed / inaccurate /
unverifiable-from-text**. "Inaccurate" always means the *documentation*, not the RTL.

Companion document: [accc-author-questions.md](accc-author-questions.md) (source ambiguities,
numbered Q1-Q16 at the time of this review; Q17-Q19 added in subsequent passes).

## Headline

The distillation chain (three compendium digests → audit-findings F1-F12) is **substantially
faithful**. Every rule that feeds F1-F12 was located in the source and confirmed, most nearly
verbatim. Problems found are concentrated in: two digest-01 prose errors around the R0=0/R0=1
cases, one table misreading in digest-02 §3, known-wrong bit-position bullets in digest-03
§19, several ⚠ VERIFY flags that are now obsolete (better extraction resolves them), and a
handful of genuine ambiguities **in the ACCC itself** that belong on the author-question list
rather than in guesses. No finding's implementation direction changes.

## Part A — Finding-by-finding verdicts

| ID | Subject | Verdict | Evidence |
|---|---|---|---|
| F1 | R10/R11 unreadable on types 0/1 | **confirmed** | §21.2.1/21.2.2 p.245: type 0 reads R12-R17, type 1 reads R14-R17 only, unrecognized regs read 0, reg 31 returns 127/255 on type 1. Q13 was resolved 2026-08-26 in favor of §21.2.2 with corroboration from the UM6845R datasheet and independent per-type table; ch.28 is the outlier. |
| F2 | Type 1 status bit 5 latched at C0=R0 | **confirmed** | §21.3.3 p.247: bit 5 = BORDER-R6 condition sampled only at C0=R0 (`false` = C4=C9=C0=0, `true` = C4=R6 ∧ C9=C0=0 at sample); bits 0-4/7 unused-read-0; R6=0-forced-border-while-C4>0 not reflected. All three F2 nuances verbatim. |
| F3 | Mid-line R7 write timing per type | **confirmed** | §16.4.1 pp.168-169 (blocked at C0vs<2, mechanism-2 latch, duration +(R0−C0vs), counter starts at next C0=0, PPI &36-active/&00-inactive at +6µs) and §16.4.2 p.169 (unconditional trigger, partial line counts, duration −(C0+1), PPI 5µs). Every F3 element near-verbatim. |
| F12 | Type-0 last-line/adjustment arbitration | **confirmed** | §10.3.1 pp.75-76, §11.2.2 pp.81-83, §12.2 pp.92-94, §13.2 pp.103-106: C0<2 window, C0==0 override, C0==1 break→adjustment with R5==0, C0==2 arbitration, R5>0-before-C0==3, completion re-establishing Last Line, R4/R9 write windows, exact-C0==R0 comparator switch. All confirmed. New: p.82 example 3 documents the **companion case** (R9 written in C0∈[2,R0−1] window → next line C4=38, C9=8) that t12 should encode beside the 39/8 exact-C0==R0 case. |
| F4 | Equality-only counter overflow | **confirmed** | §10.3 p.74 ("count to its maximum value (31) before going back to 0"), §12.1 p.92 ("C4 counts up to its limit (127) and loops back"), exception wording matches. p.78-79 tables (readable despite digest's smearing flag) corroborate. |
| F5 | Type-0 R0=0 freeze | **confirmed, digest prose errors nearby** | Freeze machinery, single deferred increment ("C4's last hiccup", §13.2.6 p.108), R4/R5/R9 ignored while R8 live, resume behaviour, and C9-vs-R5 lock persisting through C0=2 after unfreeze (p.106) all confirmed, including the full worked-vector table. Two adjacent digest-01 §8.1 sentences are wrong — see B2/B3; they do not affect the implemented t09/t16/t20 behaviour (which matches the source and `shaker-module-a-map.md`). |
| F6 | Type-0 spurious border byte (R1>R0) | **confirmed** | §17.6.2 pp.186: 0.5µs border byte raised at C0=R0, "BORDER OFF" on following character, any R0 incl. 0 (byte alternation), suppression via R8 SKEW-DISPTMG type-0-only, type 1/3/4 emit nothing (pp.186-187 "aggressive plagiarisms"). |
| F7 | RFD (CRTC 1) | **confirmed** | §11.6 pp.87-90: trigger (R5 0→nonzero exactly at C0==R0), two flags (VMA-source regardless of C4; parity management in the C9=R9-at-C0==R1 test), case-1/case-2 alternation, parity flip at C4=C9=C0=0 with R9 odd, IVM ON/OFF freezing (even C9 caveat), RFD#10/1-B taxonomy (UM6845R-8804T vs -8802T, 3 of 7 machines), recipes, HITACHI errata "real playground" remark (p.90, visually verified). Second trigger route §13.7.1.2 p.124 confirmed incl. R9-variant (line repetition) and R4-variant (stuck C4 on second frame). |
| F8 | Type-1 separate C5 counter | **confirmed** | §11.1 p.80 (C5/C9 dissociated on CRTCs 1/2; CRTC 0 has no C5), §11.2.3 p.83 worked table (C4 11→14 while C5 counts 0..15; VRAM pointer from C9), §11.3.2 p.85 R5=0 bug verbatim ("the state is not deactivated, C4 does not return to 0 and C5 loops"). F8's implementation description matches the source throughout. |
| F9 | R9 write at exact C0==R0 | **confirmed** | p.82: "The value of C9 is first compared to R9, causing C4 to increment… C9 is then compared to R5, and C9 is also incremented… we end up with C4==39 and C9==8." The digest's warning against modelling this as generic old/new-R9 sampling is consistent with the prose. |
| F10 | Interlace parity machinery | **confirmed with digest corrections required first** | §19.5-19.8 pseudocode and rules confirmed (incl. type-0 transition-line old-C9 test, exit asymmetry C9.VMA vs plain R9, type-1 3rd/4th-µs formulas verbatim, no-VSYNC-correction divergence, §19.6 single-C4-increment rule). Fix digest items B10-B12 before starting F10 fixtures. |
| F11a-h | Minor/confirmatory | **confirmed** | HSYNC width semantics (§14.4), re-entrancy + infinite-VSYNC bypass (§16.3 p.167, vector values verbatim), R12/R13 readback (§21.2), dummy 31 (§21.4 p.249 incl. UM6845E distinction), VSYNC width R3h (§14.2), light-pen listing (§21.2 tables), skew bits (see B10), type-1 mid-row R12/R13 immediacy (§20.3.2 p.242 "loaded with R12/R13 **while C4=0**"; Living Daylights cited). F11i out of CRTC scope unchanged. |

## Part B — Required documentation corrections

Priority order; each item names file, location, and the fix.

1. **B1 — digest-02 §3 duration-table interpretation is wrong.** The slash pairs (e.g.
   "CRTC0 R3l=4 → 2.0625/2.125") were labelled "NJIT/JIT" but are the two observed values of
   the **NJIT** column ("I indicated a range of 2 values", p.133); the JIT column reads
   +0.25µs above NJIT (e.g. 2.3125/2.3750), consistent with the stated JIT delay. Retire the
   "digit-wrap corruption" ⚠ for p.133 (clean in pdftotext) and reinterpret the numbers.
2. **B2 — digest-01 §8.1 R0=0 stall accounting.** "Stall of N cycles ≈ losing N/(R9+1)
   character rows" misstates the source. Counters freeze; the cost is wall-clock time: the
   source's own figure is "freezing R0=0 for 64×8 µsec amounts to 'forgetting' 8 lines"
   (p.104), i.e. N/64 raster lines of display time. Q2 was resolved 2026-08-26: with R9=7,
   "C4-1" is the equivalent one-character-row timing debit, not a counter decrement.
3. **B3 — digest-01 §8.1 R0=1 description.** "Every 2µs frame chains into another 2µs
   adjustment frame … until C9's adjustment count reaches R5" contradicts the source: with
   R5=0 the adjustment "lasts 1 line of 2 µsec before ceasing" (p.104), giving a strictly
   **alternating** normal/additional pattern (p.107 case study, caption extracted verbatim on
   p.128). Replace with the alternating description; keep the R5-reach clause only for R5>0.
4. **B4 — digest-01 §4.2/F9: record the companion case.** p.82 example 3: R9 written inside
   the C0∈[2,R0−1] window leaves C4=38, C9=8 (C4 not yet incremented); the exact-C0==R0 write
   gives 39/8. Encode both in t12.
5. **B5 — digest-01 §4.3 omission:** an R9 write at C0==R0 does **not** cancel the
   VMA-from-R12/R13-while-C4==1 rule; only an R4(>0) write does (§11.2.4 note, p.84).
   Relevant to the untested F8 corner recorded in review-debt.
6. **B6 — digest-01 §5 omission:** disarm path when R1>R0 — if C0=R1 can never be met,
   C9==R9 alone deactivates the VMA-source state (p.87). Matters for F7.
7. **B7 — obsolete ⚠ VERIFY flags to retire** (text layer carries the content cleanly):
   digest-01 p.34 (VRAM bit table), p.78-79 (divergence tables); digest-02 p.130 (R3 layout),
   p.133 totals (see B1), p.144 aggregate totals (pdftotext only), p.166 trace (mostly
   legible); digest-03 §19.2.5 sub-case rules (prose readable; only cycle diagrams compressed),
   §21.3.3 transition rules (fully in prose), §17.4 p.182 JIT rule (plain prose; keep flag
   only for the p.183 worked table and p.185 deadline boundary).
8. **B8 — digest-03 §19.1/§19.2 bit-position bullets contradict their own table.** Source
   layout: Sc = bits 7:6, Sd = bits 5:4 (BORDER ON `001100xx` sets bits 5,4). The digest
   bullets say "Sd (bits 4:3)", "Sc (bits 6:5)", "R8 bits 3:2". audit-findings F11g already
   notes this typo; fix the digest bullets themselves. Also: SKEW exists on CRTCs 0/3/4, not
   "type 0 only" (scope-narrow the heading instead).
9. **B9 — digest-02 §2 scope:** dynamic R3h rewrite section covers CRTCs 0/3/4 per the
   source, not CRTC 0 alone; and the 9→8 example implies exactly 24 lines (16+8), not "~24".
   The "already passed / not yet reached" generalization is an interpolation — mark it as such.
10. **B10 — digest-03 §19.5.2 inversions/misattributions.** Even R9 ⇒ **even** total
    line-count per character in the source (R9=6 → 2×4=8 lines); the digest's "odd total
    line-count N" parenthetical is inverted. The odd-C4 R8→3 imbalance note sits on p.206
    (digest cites p.207) and "self-correcting on subsequent frames" is not in the text —
    mark as digest inference (Q12).
11. **B11 — digest-03 §19.3 extra-line frame attribution.** Digest states the extra scanline
    ends "the odd frame's construction"; source pages conflict (p.198 "end of the first
    frame", p.205/216 "construction of the even frame", p.199 odd frame "inherits" it and
    lasts 20032µs). Q10 was resolved 2026-08-25: the line is generated at the even-to-odd
    boundary and duration-counted in the following odd frame.
12. **B12 — digest-03 §28.1.8:** drop the "described as always-1 per §21.3" gloss (not in
    source; "always 1" bit 6 appears only in the CRTC3/4 STATUS-1 table, p.248). Q14 resolved
    the UM6845R identification field as bit 5; Q13 resolved R14-R17 readback in favor of
    §21.2.2. The concrete OUT/IN acceptance steps remain digest-added.
13. **B13 — digest-03 glosses to mark as inference:** §17.5's precise 3d/3e/3f deadline
    mapping and the "CRTC3/4 need one extra character lead" rule (diagram-shape inference;
    visual tier pending); §18.1's "checked on the row's first scanline" (source's "(1st
    line-character R6)" is ambiguous — Q16); "border permanently on / pointer still
    counts" under R1=0 (pointer part is stated, permanence is implied — border-alternation
    wording Q15).

## Part C — Consequences for code

No finding reverses direction. Concrete follow-ups once docs are corrected:

- **F9/t12**: add the C4=38/C9=8 window-write companion vector (B4) — pure test addition.
- **F8 corner** (review-debt row for the F8 commit): implement/test the §11.2.4 note that an
  R9 write at C0==R0 must not cancel the VMA-from-R12/R13 C4=1 behaviour (B5) before closing.
- **F7/RFD**: include the R1>R0 disarm path (B6) in the RFD design notes.
- **F10**: the former Q10/Q11/Q12 gates were adjudicated 2026-08-25; remaining work is
  fixture-gated under F14/F15, with the subsequently resolved post-exit behavior under F16.
- Everything else: documentation-only.

## Part D — Not verifiable from the text layer (visual tier available)

Pixel-granular material remains genuinely diagram-only in both extractions and needs the
rendered PNGs before being quoted as fact: pixel-M2 positioning diagrams (pp.135-137, 139-140,
144 rows, 149-151, 160), R2 46↔50 sequences (p.157), SHAKER 22C/3 parity truth tables
(pp.210-212), type-0 IVM worked tables (pp.221-224), p.183 worked table, p.185 deadline grid,
p.246 status-register table layout. None of these blocks the F1-F12 verdicts above; they gate
only future pin-exact vectors, where the existing protocol (derive from rule, cite page) still
applies.

(Update 2026-08-24, D1: this list is the state as of 2026-08-22 and is retained as a dated
record. Every page group above has since been render-verified — see the 2026-08-24 notes in
the compendium digests; corrections that fell out: the parity truth tables are pp.210-211
only, the IVM tables use R9=6 even, the p.185 deadline boundary was corrected, and p.247
alone carries the bit-5 diagrams. p.246 remains unverified.)

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
