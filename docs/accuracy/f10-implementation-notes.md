# F10 interlace (IVM) parity machinery — implementation record

Status: **implemented for the unblocked scope, both types** (2026-08-24, branch
`accuracy/f10-fixtures`). Odd-R9 alternation and the additional interlace line remain
gated behind author questions Q19 / Q10 as planned; see "Deliberately unmodeled" below.
This file is the durable record for a fresh session: what was verified against the PDF,
what the implemented model is, which conventions the fixtures pin, and what is left open.

## Commits and gates

| Commit | Content |
|---|---|
| `20eb6d5` | Fixture-only: `t21a`-`t21p` (type-1 toggle panels), `t22a`-`t22o` (type-0 entry/exit), inert shared parity flops, soak field expansion. |
| `657ccde` | Type-1 behavior: R8-toggle two-stage parity update, §19.8.2 counting, per-frame/per-C4 parity toggles. All `t21` required. |
| `3a2293a` | Type-0 behavior: split C9/C9.VMA, asymmetric entry/exit limit tests, §19.5.2 parity rules. All `t22` required. |

Gates at the type-0 tip: 140 required passes, 0 xfailed/xpassed/failed; lint clean apart
from pre-existing warnings; soak golden `0x83e80134f7705b46` (chain in AGENTS.md; the
three mints are documented there — fixture-stage field expansion, type-1 behavior,
type-0 behavior).

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
  R9.0)`, written into C9.0 **only when entering** (the panels hold C9.0 through a
  leaving stage A — the four X=1 panels pin this asymmetry); **stage B (4th µs)** one
  character later: entering, an even ParityFrame re-points ParityC9 at `C4.0 and not
  R9.0` and `ParityFrame := ParityFrame and (ParityC9 xor X)` — which for an odd
  ParityFrame reduces to the old C9.0 (the documented "changes to even, except…" clause);
  leaving, `ParityFrame := ParityC9` and `C9.0 := ParityC9` ("deactivation modifies C9").
- §19.8.2 counting while IVM: C9 pre-increments when R9 is even, compares with bit 0
  masked, restarts from the toggled ParityC9 on a match, otherwise steps by two. C9
  itself carries the parity (no C9.VMA split), so RA is the raw counter.
- Row-end plumbing is split three ways: the frame-structure limit (frame-adj/adjustment
  entry), the per-line row event (C4 increments at *every* C9 wrap during adjustment per
  §11.1 — this preserved t08i/j/l bit-identically), and the row-structure test for the
  VMA reload/save/vsync consumers.

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
  changes afterwards — the p.219 row-end update is Q19-gated (see below).
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
  stay clear of setup writes). The panels' drawn *Parity* rows are internally
  inconsistent by one character on the ODD page — a source drawing quirk in the same tier
  as the flagged C9-column quirks of pp.221-224 — so the fixtures assert the callouts and
  the C9 rows, which are mutually consistent and complete.
- **Type-0 entry/exit (`t22`)**: expectations come from the C9-VMA columns and the
  reliable C9 segments; the settled post-reset C9 column (which prints doubled values)
  is replaced by the pseudocode's raw stepping, cross-checked against the VMA column.
  Exit fixtures are table-shaped: IVM entered on row 0, the row completes, and the exit
  write lands on a second-row line (C4=1) exactly as drawn.

## Deliberately unmodeled / open (with reasons)

- **Odd-R9 alternation** (the p.219 `If R9.0=0` gate token vs its own gloss): author
  question Q19. The even-R9 behavior the tables pin needs no row-end ParityC9 update, so
  none is implemented; odd-R9 waits for Q19 before any RTL moves.
- **Post-exit row-end behavior** (six of eight exit tables run C9 to 7 with R9=6 without
  the C4 increment the plain test predicts): author question Q19(b). The `t22` exit walks
  stop before that zone.
- **Additional interlace line** (§19.6, both types): gated on Q10 (which frame receives
  it) — unchanged from the pre-existing plan; not implemented.
- **MID-VSYNC parity coupling**: the wrapper's MID-VSYNC tick and fire still key on the
  legacy `field` flop, which freezes while R8 is outside 1/3, whereas ParityFrame keeps
  toggling every frame. After a mid-frame R8 toggle episode the two diverge; no sourced
  table pins the hardware behavior (adjacent to Q12). The pre-F10 MID-VSYNC vectors
  (t02i, t09g) are unchanged and green. This is the main candidate for a follow-up
  finding if SHAKER Module B interlace entries ever discriminate it.
- **VSYNC delay-by-1-line correction for odd C4s** (§19.5.2, p.206-207): part of the
  odd-R9 balancing scheme — Q19 territory, not implemented. Type 1's documented *lack*
  of the correction (p.208) is likewise unmodeled for the same reason.
- **RFD × IVM interaction** (both types): unpinned; the type-1 RFD terms deliberately
  keep the bare C9==R9 comparison.
- **Adjustment during IVM** (both types): adjustment keeps its plain comparisons; the
  interaction is unpinned in the source.
- **Same-edge coincidences** (stage edge vs row end vs frame boundary): documented
  priorities in the RTL (stage write wins over parity toggles; `line_new` wins over the
  C9.0 poke) — the coincidences themselves are unpinned.
- **Interlace Sync mode (R8=1)**: parity state updates (frame toggle, per-C4 toggle) are
  live, but no IVM counting/display change and no toggle-stage rules fire (the source's
  stage rules are written for 0↔3). The additional line and MID-VSYNC effects of R8=1
  remain under the Q10 gate.

## Reviewer guidance (for the pre-merge review)

- Check the type-1 stage machine against pp.210-211 directly (especially the
  leaving-stage-A C9.0 hold and the odd-ParityFrame stage-B reduction), not against this
  document.
- Check the type-0 seam/toggle lifecycle against p.219's "next C0=0" rule, especially the
  same-edge write race at the seam.
- Check that the three-way row-end split on type 1 is truly behavior-preserving outside
  IVM (t08i/j/l and t13a-d are the sensitive vectors; the soak is the broad guard).
- Check the t22 exit-fixture re-derivation against pp.223-224 (the C4=1 second-row exit
  shape), and the t09g RA re-derivation against §19.5.2.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).
