# F10 interlace (IVM) parity machinery — implementation record

Status: **implemented for the unblocked scope, both types; independently reviewed and
remediated** (2026-08-24/25, branch `accuracy/f10-fixtures`; review record
`accuracy/f10-independent-review.md` — NOT CLEAR on two blockings, both fixed with vectors).
Odd-R9 alternation and the additional interlace line remain gated behind findings F15/F14;
the post-exit frozen-C9.VMA behavior resolved 2026-08-26 is finding F16 (see "Deliberately
unmodeled" below).
This file is the durable record for a fresh session: what was verified against the PDF,
what the implemented model is, which conventions the fixtures pin, and what is left open.

## Commits and gates

| Commit | Content |
|---|---|
| `20eb6d5` | Fixture-only: `t21a`-`t21p` (type-1 toggle panels), `t22a`-`t22o` (type-0 entry/exit), inert shared parity flops, soak field expansion. |
| `657ccde` | Type-1 behavior: R8-toggle two-stage parity update, §19.8.2 counting, per-frame/per-C4 parity toggles. All `t21` required. |
| `3a2293a` | Type-0 behavior: split C9/C9.VMA, asymmetric entry/exit limit tests, §19.5.2 parity rules. All `t22` required. |
| (remediation) | Review B-1/B-2/N-1 fixes, `t22` RA column, `t22p`-`t22s` + `t23a`-`t23c` vectors, ivm mirror for snapshot-loaded R8=3, soak re-mint. 147 required passes. |

Gates at the remediated tip: 147 required passes, 0 xfailed/xpassed/failed; lint clean
apart from pre-existing warnings; soak golden `0xa9e5026de83d287c` (chain in AGENTS.md;
the mints are documented there — fixture-stage field expansion, type-1 behavior, type-0
behavior, review remediation).

## Evidence base (all render-verified under `accuracy/extract/README.md`)

- **pp.210-211** — the 16 SHAKER 22C/3 type-1 toggle panels. Every one of the 64 parity
  callouts matches the p.209 pseudocode exactly, and every panel's C9 row matches the
  two-stage model derived from it. These panels are the sole render-verified oracle for
  the type-1 R8-toggle semantics.
- **pp.219-220** — the §19.8.1 pseudocode, the "next C0=0" latch rule, the exit rule
  (C9.VMA vs plain R9), the p.220 Note extending the test to the VMA' save, and the
  p.220 worked example (exit at C9.VMA=R9+1 → C9 increments to 4).
- **pp.221-224** — the type-0 entry tables (switch at C9=0..6, even and odd frames) and
  exit tables. The C9-VMA columns and the pre-switch/overflow segments of the C9 columns
  are the oracle; the digest's flagged quirks (doubled values printed in the settled-C9
  column; post-exit C9-to-7 padding) are avoided, not asserted.
- **p.205, p.208, p.209** — the §19.5.2/§19.5.3 parity-state prose.

## The implemented models

### Type 1 (§19.5.3, §19.8.2; `rtl/crtc_type1_engine.v`)

- ParityFrame toggles at every C4=C9=C0=0 frame boundary regardless of R8. ParityC9
  toggles at each genuine C4 increment when R9 is even.
- An R8 write toggling IVM (0↔3, also 1↔3) runs the documented two-stage update:
  **stage A (3rd µs)** at the next character edge: `ParityC9 := C9.0 xor (C4.0 and not
  R9.0)`, written into C9.0 **in both directions** — p.209 states the 3rd-µs rule once
  ("when R8 changes from 3 to 0 or vice versa"), and the four X=1 panels draw the C9.0
  change in the leaving write's 3rd-µs column (review B-1; an earlier direction-asymmetric
  reading here was an artifact of misaligning the panels' strip columns — each OUT is a
  4-cell structure: 2-cell label, dark µs-3 cell, on/off µs-4 cell); **stage B (4th µs)**
  one character later: entering, an even ParityFrame re-points ParityC9 at `C4.0 and not
  R9.0` and `ParityFrame := ParityFrame and (ParityC9 xor X)` — which for an odd
  ParityFrame reduces to the old C9.0 (the documented "changes to even, except…" clause);
  leaving, `ParityFrame := ParityC9` (and C9.0 already carries that value from stage A).
- §19.8.2 counting while IVM: C9 pre-increments when R9 is even, compares with bit 0
  masked, restarts from the toggled ParityC9 on a match, otherwise steps by two. C9
  itself carries the parity (no C9.VMA split), so RA is the raw counter.
- Row-end plumbing is split three ways: the frame-structure limit (frame-adj/adjustment
  entry), the per-line row event (C4 increments at *every* C9 wrap during adjustment per
  §11.1 — this preserved t08i/j/l bit-identically), and the row-structure test for the
  VMA reload/save/vsync consumers. Scope note (review N-4): `line_last` itself differs
  inside type-1 adjustment (it now follows the adjustment end); observable only through
  `line_last_r` after a live type 1→0 switch during adjustment — not a bug, but the
  "outside IVM bit-identical" claim does not extend to that one signal.
- §19.8.2's match branch toggles ParityC9 and restarts C9 from it as one step at every
  row end, including the C4=0 frame-boundary arm (review B-2; vector `t23a`).

### Type 0 (§19.8.1, §19.5.2; `rtl/crtc_type0_engine.v`)

- C9 counts by 1 ("continues to increment normally"); the address-visible value is the
  split `C9.VMA = ((C9×2) + ParityC9) mod 32`. RA carries C9.VMA on lines that started
  with IVM active.
- The line-limit test has two independent IVM bits, which reproduces all four documented
  forms from one comparison:
  - *value doubled* — on lines that started with IVM on. Latched at each C0=0 seam from
    the live R8 register, implementing "performed on the next C0=0, after the C9/R9 test
    of the line" (p.219): the switch line tests raw C9, the doubled test starts the
    following line.
  - *target parity* — `R9 or ParityFrame` on the switch line (p.219), `R9 or ParityC9`
    on steady IVM lines (p.220), plain R9 from the exit line on (p.220). The exit line's
    doubled-value-vs-plain-R9 form is the same two bits composed differently.
  - The seam capture uses the same composition with the new line's mode, and a
    line-scoped toggle status (set by the bus write, consumed at the next seam) carries
    the switch/exit target adjustment. A write landing exactly on a seam edge qualifies
    only from the next line — the documented "after the test" race.
- ParityC9 is seeded from ParityFrame when IVM turns on at a seam (the tables' doubled
  display carries the frame parity from the first doubled line). With even R9 it never
  changes afterwards — the p.219 row-end update is absent (gate = R9 odd, Q19 resolved;
  see below and F15).
- §19.5.2 parity rules: ParityFrame snapshots ParityR6 at the frame origin; ParityR6
  captures ParityFrame xor 1 when C4 reaches R6 (independent of R8; frozen when R6>R4).
  The p.219 alternative frame-end toggle (`ParityFrame ^= ParityR6` at C4==R4) is
  equivalent to the snapshot at the origin and is not duplicated.
- The old approximation (C9 stepping by 2 with bit 0 masked, halved limit, field-OR into
  RA bit 0) is removed. Non-IVM behavior is bit-identical (t01-t20 green unchanged);
  t09g's single RA expectation was re-derived from §19.5.2 (see the comment in
  `sim_main.cpp`): for that register set R6>R4 freezes ParityFrame at 0, so the
  documented RA is 0 where the approximation produced 1.

### Shared state

`parity_frame` / `parity_c9` / `parity_r6` live in the wrapper (`rtl/CRTC.v`) so a live
`CRTC_TYPE` switch continues from the same state, matching the shared-counter contract
(t02j/t06d/t09f/t16l). Each engine contributes its type's update decisions through
write/value port pairs.

## Fixture conventions (pinned in the test headers)

- **Type-1 toggle (`t21`)**: the R8 write lands mid-line; stage A applies at the next
  character edge (the panels' "on"/"off" column), stage B at the one after. The panels
  pin the +1/+2 character offsets, not an absolute C0 (the fixtures write at C0=20/27 to
  stay clear of setup writes). The panels' drawn *Parity* rows, read as effective parity (`IVM ? ParityC9 :
  ParityFrame`), validate 14 of the 16 panels cell-for-cell (review N-13); only
  27(ZA)/29(ZC) and (M)/15(O) drop one cell early at the on-write's µs 3 — a genuine but
  local drawing slip. The fixtures assert the callouts and the C9 rows, which are
  mutually consistent and complete; the Parity rows serve as a cross-check.
- **Type-0 entry/exit (`t22`)**: expectations come from the C9-VMA columns and the
  reliable C9 segments; the settled post-reset C9 column (which prints doubled values)
  is replaced by the pseudocode's raw stepping, cross-checked against the VMA column.
  Exit fixtures are table-shaped: IVM entered on row 0, the row completes, and the exit
  write lands on a second-row line (C4=1) exactly as drawn.

## Deliberately unmodeled / open (with reasons)

- **Odd-R9 alternation** (the p.219 `If R9.0=0` gate token vs its own gloss): the token was
  adjudicated 2026-08-25 as a typo for `R9.0=1` — the row-end ParityC9 update fires only when
  R9 is **odd** (author question Q19 main token, RESOLVED by default reading; see
  accc-author-questions.md item 19). The even-R9 behavior the tables pin needs no row-end
  ParityC9 update, so none is implemented; odd-R9 counting is finding **F15**
  (audit-findings.md) — fixtures before RTL.
- **Post-exit row-end behavior** (seven non-match windows run C9 to 7 with R9=6; six keep C4
  unchanged and one has an anomalous C4 increment): author question Q19(b) was resolved visually
  2026-08-26. The eight tables discriminate the **frozen C9.VMA register content** against
  plain R9 after a non-matching R8=0 write: even frozen 6 resets, while even 0/2/4 and odd
  1/3/5/7 run through C9=7. This core instead resumes a live plain C9==R9 test on post-write
  lines (unpinned; the `t22` exit walks stop at the write line's seam). A persistent mismatch
  can leave the row unable to complete until software changes the comparison state; p.220's
  recovery rewrites R9 to frozen C9.VMA. The divergence is finding **F16** and needs fixtures
  before RTL. The anomalous p.223 C4 cell is excluded, but its C9 run-on remains usable evidence.
- **Additional interlace line** (§19.6, both types): Q10 RESOLVED 2026-08-25 by default
  reading — the line is generated at the end of the ParityFrame-even frame (type 1 gate:
  ParityFrame even; type 0 gate: ParityR6 odd, with the R6>R4 freeze) and duration-counted
  in the following odd frame. Now finding **F14** (audit-findings.md); not
  implemented, fixtures before RTL.
- **MID-VSYNC parity coupling**: the wrapper's MID-VSYNC tick and fire still key on the
  legacy `field` flop for the NON-IVM interlace paths; `field` freezes while R8 is outside
  1/3, whereas ParityFrame keeps toggling every frame, so after a mid-frame R8 toggle
  episode the two diverge and no sourced table pins the hardware behavior (adjacent to
  Q12). The pre-F10 MID-VSYNC vectors (t02i, t09g) are unchanged and green. Narrowed
  2026-08-25 (t24 closure + review B-1 remediation): during type-1 IVM the VSYNC no longer
  keys on `field` at all — p.208 pins the start line on both parities (t24a/t24b) and
  schedules the MID-VSYNC on the ParityFrame-even frame, which the wrapper now implements
  from ParityFrame directly with a seam-latched fire decision consumed at the half-line
  tick (t24c); the residual covers type-0 IVM and the non-IVM post-episode divergence only.
- **VSYNC delay-by-1-line correction for odd C4s** (§19.5.2, p.206-207): part of the
  odd-R9 balancing scheme — now in F15 scope (Q19's main token resolved 2026-08-25; the
  correction stays unimplemented pending F15 fixtures). Type 1's documented *lack*
  of the correction (p.208) is now modeled and pinned by `t24a`/`t24b` (2026-08-25, t24
  closure: the pulse starts at the first line of C4=R7 on both frame parities, giving the
  documented permanent 1-line gap for odd R7), together with the p.208 MID-VSYNC on the
  ParityFrame-even frame (`t24c`, half-line start/end via a seam-latched fire decision);
  the type-0 delay itself remains unimplemented.
- **RFD × IVM interaction** (both types): unpinned; the type-1 RFD terms deliberately
  keep the bare C9==R9 comparison.
- **Type-0 double R8 write in one line** (review N-6): the line keeps its entering-form
  target; unpinned, documented in the engine.
- **IVM toggle during an R0=0 freeze** (review N-7): the seam fires every CLKEN while C0
  is pinned, consuming the toggle status immediately; unpinned, documented in the engine.
- **Type-0 odd-R9 IVM is wrong by construction** (finding F15): the p.219 row-end
  ParityC9 update is absent altogether (its corrected gate is R9 odd, Q19 main token
  resolved 2026-08-25). Even R9 — everything the tables pin — is unaffected.
- **Adjustment during IVM** (both types): adjustment keeps its plain comparisons; the
  interaction is unpinned in the source.
- **Same-edge coincidences** (stage edge vs row end vs frame boundary): documented
  priorities in the RTL (stage write wins over parity toggles; `line_new` wins over the
  C9.0 poke) — the coincidences themselves are unpinned.
- **Interlace Sync mode (R8=1)**: parity state updates (frame toggle, per-C4 toggle) are
  live, but no IVM counting/display change and no toggle-stage rules fire (the source's
  stage rules are written for 0↔3). The additional line and MID-VSYNC effects of R8=1
  remain under finding F14 (Q10 resolved 2026-08-25; fixtures before RTL).

## Review outcome

Reviewed 2026-08-25 by Claude Opus 5 (fresh session via the claude CLI): verdict NOT
CLEAR on two blockings — B-1 (the leaving stage A must write C9.0; accepted against the
panels and the p.209 prose) and B-2 (the §19.8.2 frame-boundary toggle split; accepted) —
plus 13 non-blockings. All blockings and accepted non-blockings are remediated (see the
remediation section of `accuracy/f10-independent-review.md`); N-4 and N-12 are recorded
as scope note/erratum. The same-edge seam race was reviewed and judged defensible. Five
reviewer bite-tests were reproduced during the review; the remediation adds the missing
vectors so the mechanisms are now deterministically pinned (RA column in `t22`,
frame-boundary continuity in `t23a`, snapshot-loaded R8=3 in `t23b`, R8=1 RA in `t23c`).

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
