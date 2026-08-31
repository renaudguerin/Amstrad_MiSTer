# ACCC Round 2 documentation correction — independent review

**Date:** 2026-08-31

**Reviewer:** Claude Opus 5, high effort, guarded read-only bridge

**Reviewed state:** `d74cd2429f1a0fef0a3b00cf45e900d6d02c518c`

**Correction range:** `c11c55d^..605d29f`

**Verdict:** **CLEAR** — no blocking findings

The reviewer opened the original user-owned French and English ACCC v1.11 PDFs directly.
Both SHA-256 fingerprints matched the values in the extraction manifest and active feedback
document. The review was source-first and did not treat the checked-in digests, simulation,
or the current RTL as hardware evidence.

## Source findings

- **Q12 confirmed.** French §19.5.2 p.208 qualifies the R8-to-3 example with
  “à chaque frame”; English p.206 omits that repeated-activation condition. The Round 2
  report correctly separates this edition fact from its conditional deduction for a single
  activation. That deduction depends on stable registers, continuing frame origins, and R6
  remaining reachable. It does not establish post-toggle pin timing or silicon behavior.
- **Q20/N2 confirmed as an open source/model distinction.** English §11.2.4 p.84 and French
  p.85 generally say that C4 increments without considering R4 during adjustment. The more
  specific R5=0 paragraph in English §11.3.2 pp.85–86 and French p.87 retains the C4/R4
  comparison while the additional-management state remains active. The documents accurately
  label the C4/R4 reset route as the preferred reading, the free-running 7-bit C4 route as the
  current RTL model, and `t08j` as model coverage rather than hardware proof.
- **The unrun discriminator arithmetic is correct.** With R4=10 and R9=3, the two recurring
  VSYNC intervals after the initial C4 overflow are `(R4+1) × (R9+1) = 44` and
  `128 × (R9+1) = 512` scanlines. Neither result has been measured on hardware.

Gemini 3.7 Flash high independently performed only the bounded artifact check: PDF hashes,
the two bilingual clauses, and the 44/512 arithmetic. It agreed on those evidence points and
did not issue a second review verdict.

## Non-blocking findings and disposition

1. A trailing-space defect in the archived Round 1 notice was confirmed and removed.
2. The debt row omitted the contemporaneous `AGENTS.md`/`CLAUDE.md` documentation-only gate
   policy edits; the cleared record now names the complete scope.
3. The counter digest's §11.3 heading retained a stale English page range; it now cites
   English pp.85–86 and French pp.87–88.
4. English-only page anchors in French-primary prose are now edition-labelled and paired
   with the French anchor where appropriate.
5. `current-status.md` used 2026-08-28 for a release whose PDF changelog and feedback archive
   record 2026-08-27; the handoff now uses the source date.

The ACCC Round 2 documentation row is therefore cleared. Q20/N2 and both hardware timing
questions remain validation residuals; clearing documentation review debt does not resolve
them.
