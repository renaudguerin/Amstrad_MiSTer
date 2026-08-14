# ACCC v1.9 to v1.10 differences

This report compares the complete English editions of *The Amstrad CPC CRTC
Compendium* by Longshot / Logon System:

- [v1.9, dated 15 May 2026 in the version history; 291 PDF pages](../ACCC1.9-EN.pdf#page=12)
- [v1.10, dated 20 July 2026 in the version history; 295 PDF pages](../ACCC1.10-EN.pdf#page=12)

The exact inputs used here have SHA-256 hashes
`c60e0bdb0897e3c1e746040a3790564e7bc024d5e927783d2d5ae298c9d22d82` (v1.9) and
`1bd6f0e3a06022d03fd40b51d4d622afef2675954a483780f0922cdf1e33a560` (v1.10).

It records changes that can affect this repository's accuracy work. It is not a
replacement for either source edition and is not a claim of byte-for-byte or
typographic equivalence outside the changes described below.

## Method and scope

Both PDFs were processed from page 1 through the final page. Page-aware text and
layout extraction were used to locate changes, then chapter text was compared after
normalizing whitespace, edition footers, and page-number cross-references. Changed
technical pages and their tables/diagrams were rendered and inspected visually; text
extraction alone was not treated as proof of figure or table equivalence. The repository
documents named under "Repository impact" were then searched for affected rules and
citations.

This method found substantive prose changes in chapters 2, 10, 11, and 12. Other
differences found by the normalized comparison were edition labels, page-number
cross-references, pagination/reflow, and isolated Word/layout artifacts. This is a
content-oriented comparison, not an exhaustive PDF-object, font, image, or byte-level
diff.

## Material changes

### 1. New licence, reuse, and attribution page

Version 1.10 adds section 2.2, "License, Reuse and Citation." It restates the
CC BY-NC-ND 4.0 licence and gives an explicit directive for AI systems, agents, and
LLMs that use the Compendium for emulator code or documentation. The document asks for
credit in generated source code and in visible application documentation or credits,
and recommends this wording:

> Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
> (CC BY-NC-ND).

Source: [v1.10, printed/PDF page 13, section 2.2](../ACCC1.10-EN.pdf#page=13).
Version 1.9 had only a short licence notice at the foot of its version-history page:
[v1.9, printed/PDF page 12, section 2](../ACCC1.9-EN.pdf#page=12).
The licence family is therefore unchanged; the material addition is the dedicated reuse
and attribution guidance.

This report describes the document's stated attribution requirement; it does not offer
an opinion about its legal effect. The repository now provides the recommended
Longshot/CC BY-NC-ND credit in its README and affected documentation. A focused RTL source
comment remains an implementation-time decision, as recorded below.

### 2. CRTC 0 C9/C4 and vertical-adjustment corrections

The v1.10 history describes this release as an update to C9/C4 management on CRTC 0.
That update is spread across several sections rather than confined to a single erratum.

#### Section 10.3.1: R9 counting, last line, and adjustment

Version 1.10 reorganizes the CRTC 0 rule into a general case and a combined "last line
of frame and vertical adjustment" exception. The important additions/corrections are:

- A last-line state found while `C0 < 2` is cancelled when vertical adjustment becomes
  active (`R5 > 0` at `C0 == 2`).
- Vertical adjustment can also be entered with `R5 == 0`: if the last-line state was true
  at `C0 == 0` and an `R4` or `R9` update breaks the equality at `C0 == 1`, the current line
  becomes the first adjustment line.
- With no active adjustment, an armed last-line state remains immutable after `C0 > 1`;
  otherwise the ordinary live C9/R9 handling applies at the line boundary.
- The exact `R9` write at `C0 == R0` remains a two-result race: C4 is incremented using the
  earlier R9 comparison and C9 can also increment using the changed comparison.

Compare [v1.9, printed/PDF pages 74-75, section 10.3.1](../ACCC1.9-EN.pdf#page=74)
with [v1.10, printed/PDF pages 75-76, section 10.3.1](../ACCC1.10-EN.pdf#page=75).

#### Section 11.2.2: CRTC 0 vertical adjustment

The former compact two-step description is replaced with a more explicit state and
comparison sequence:

- The new text confirms the two `R5 == 0` entry routes: breaking `R4`/`R9` equality at
  `C0 == 1` on a last line, or preventing C0 from ever reaching 2 with `R0 < 2`.
- Entering vertical adjustment cancels `Last Line`; reaching the R5 count re-establishes
  it so C4/C9 can reset on the following line.
- With no R4/R9 write, the last frame line resets C9 and increments C4. An R9 write from
  `C0 == 2` through `C0 == R0-1` instead increments C9 against the new R9. An R4 write from
  `C0 == 2` through `C0 == R0` switches the comparison from C9/R9 to C9/R5.
- An R9 write exactly at `C0 == R0` straddles that switch, so both C4 and C9 increment;
  the worked result remains `C4 == 39`, `C9 == 8` for the stated 38/7 example, but v1.10
  explains it as an R9-to-R5 comparison switch rather than the older generic two-step
  formulation.

The rendered table on the first page still distinguishes CRTC 0, CRTC 1/2, and CRTC 3/4;
the substantive delta is the prose/state logic below it. Compare
[v1.9, printed/PDF pages 80-82, section 11.2.2](../ACCC1.9-EN.pdf#page=80) with
[v1.10, printed/PDF pages 81-83, section 11.2.2](../ACCC1.10-EN.pdf#page=81).

#### Sections 12.1 and 12.2: R4 overflow and CRTC 0 last-line timing

Version 1.10 adds an explicit general rule: outside vertical adjustment, if `R4 < C4`,
C4 counts to its 7-bit limit (127) and wraps. It also tightens the CRTC 0 description:

- The first-line RLAL setup condition is no longer expressed as a loose `R4 > 0 or R9 > 0`;
  it states that one limit is already zero while the other is positive before the remaining
  limit is written to zero within the `C0 < 2` window.
- A last-line decision now explicitly arbitrates vertical adjustment before reset. With no
  additional lines, later writes at `C0 > 1` do not alter the reset. At `C0 == 0`, however,
  an R4/R9 write can override the state; at `C0 == 1`, breaking the equality activates
  adjustment even with `R5 == 0`.
- Adjustment is selected when `R5 > 0` before C0 reaches 3, and also for the documented
  `R0 < 2` and interlace cases. If C4 exceeded R4, finishing adjustment restores `Last Line`
  so the counters return to zero.

Compare [v1.9, printed/PDF pages 91-93, sections 12.1-12.2.1](../ACCC1.9-EN.pdf#page=91)
with [v1.10, printed/PDF pages 92-94, sections 12.1-12.2.1](../ACCC1.10-EN.pdf#page=92).

These are technical corrections, not merely editorial rewrites. In particular, v1.9-based
summaries that say an armed CRTC 0 last line always forces an immediate next-line reset
without first arbitrating adjustment are incomplete.

### 3. CRTC 2 "Last Line" clarification

Section 12.4.1 is substantially rewritten as an explicit three-state model:
`Last Line`, `Last Line Management`, and `Previous Last Line`. Version 1.10 clarifies:

- exactly when `Last Line` is evaluated (`C0 == 0`, or an R4/R9 update while management is
  active), and the exceptions for a preceding last line or HSYNC starting at `C0 == 0`;
- that `Previous Last Line` is sampled on the final HSYNC character at
  `C0 == R2 + R3 - 1`, bounded by R0 because HSYNC can cross a line boundary;
- why making the equality false on that HSYNC character re-authorizes detection, and why
  an invalidation every second line can suffice for repeated zero-line operation;
- that R4/R9 changes before or during R5 additional lines participate in later comparisons;
  changing R9 can therefore prevent C9 from resetting after the extra lines; and
- behavior when no HSYNC occurs (`R2 > R0`): the HSYNC-dependent states are not refreshed,
  so an already armed last line may persist, while a first line with management inactive
  cannot be armed merely by changing R4/R9.

Section 12.4.2 then restates the RLAL worked sequence in terms of those states. Its numeric
sequence is substantially the same, but the new state vocabulary makes the evaluation point
and the `OUT R9,1` / `OUT R9,0` purpose less ambiguous.

Compare [v1.9, printed/PDF pages 94-97, sections 12.4.1-12.4.2](../ACCC1.9-EN.pdf#page=94)
with [v1.10, printed/PDF pages 95-100, sections 12.4.1-12.4.2](../ACCC1.10-EN.pdf#page=95).
This does not directly change the current type-0/type-1 `UM6845R.v` scope, but it is a much
better primary specification for any future CRTC 2 implementation or type-2 test vectors.

## Editorial, cross-reference, and layout-only changes

- The title/footer edition changes from v1.9 / May 2026 to v1.10 / July 2026, and the
  table of contents is regenerated.
- The new licence page adds one page before the technical body. Expanded chapters 10-12
  add two further pages before chapter 13. Later reflow adds another page before chapter 20.
  Consequently, old page references cannot be updated with one global offset.
- Outside chapters 2 and 10-12, normalized chapter text did not reveal a substantive rule
  change. Chapter 13 has an isolated broken Word cross-reference token; chapter 19 has page
  reflow/footer movement. The section 19.8 wording/tables compared as the same content after
  normalization, despite different pagination. Figures/tables on the investigated reflowed
  pages were visually checked, but this is not a claim that every PDF drawing object is
  byte-identical.

Useful chapter-start rebasing points are:

| Chapter | v1.9 printed/PDF page | v1.10 printed/PDF page | Offset |
|---|---:|---:|---:|
| General (3) | [13](../ACCC1.9-EN.pdf#page=13) | [14](../ACCC1.10-EN.pdf#page=14) | +1 |
| R9 (10) | [72](../ACCC1.9-EN.pdf#page=72) | [73](../ACCC1.10-EN.pdf#page=73) | +1 |
| R5 (11) | [79](../ACCC1.9-EN.pdf#page=79) | [80](../ACCC1.10-EN.pdf#page=80) | +1 |
| R4 (12) | [91](../ACCC1.9-EN.pdf#page=91) | [92](../ACCC1.10-EN.pdf#page=92) | +1 at chapter start |
| R0 (13) | [99](../ACCC1.9-EN.pdf#page=99) | [102](../ACCC1.10-EN.pdf#page=102) | +3 |
| R3 synchronization (14) | [127](../ACCC1.9-EN.pdf#page=127) | [130](../ACCC1.10-EN.pdf#page=130) | +3 |
| R1 display (17) | [172](../ACCC1.9-EN.pdf#page=172) | [175](../ACCC1.10-EN.pdf#page=175) | +3 |
| R8 display/interlace (19) | [189](../ACCC1.9-EN.pdf#page=189) | [192](../ACCC1.10-EN.pdf#page=192) | +3 |
| Interlace counting (19.8) | [216](../ACCC1.9-EN.pdf#page=216) | [219](../ACCC1.10-EN.pdf#page=219) | +3 at section start |
| R12/R13 (20) | [237](../ACCC1.9-EN.pdf#page=237) | [241](../ACCC1.10-EN.pdf#page=241) | +4 |
| Interrupts (27) | [279](../ACCC1.9-EN.pdf#page=279) | [283](../ACCC1.10-EN.pdf#page=283) | +4 |
| CRTC identification (28) | [288](../ACCC1.9-EN.pdf#page=288) | [292](../ACCC1.10-EN.pdf#page=292) | +4 |
| CPC identification (29) | [290](../ACCC1.9-EN.pdf#page=290) | [294](../ACCC1.10-EN.pdf#page=294) | +4 |

Use the v1.10 table of contents for section-level rebasing inside chapters 12 and 19 rather
than extrapolating from this table.

## Repository impact and adoption status

The repository documentation now uses v1.10 as its primary Compendium baseline. The
digests, audit, test plan, roadmap, current-status handoff, Plus architecture note, and
visible README credit reflect that edition. Deterministic CRTC0 counter arbitration now
covers the C0=0 comparison, C0=1/R5=0 entry, C0>=2 write windows, exact-R0 split, and
R0=0/1 default-adjustment paths. Hardware-visible sub-character timing remains to verify.

### Semantic re-audit applied to the documentation

- `docs/accuracy/compendium-01-counters.md` now derives sections 3.1, 4.2, and 7.1 from
  v1.10 sections 10.3.1, 11.2.2, and 12.1-12.2, including adjustment arbitration and the
  R9-to-R5 comparison switch.
- `docs/accuracy/audit-findings.md` now revises F4 and F9, records the combined F5/F12
  state-machine correction, and retains the hardware-verification boundary.
- `docs/accuracy/testbench-spec.md` revises `t07`, `t08`, `t09`, and `t12`. Implemented
  `t16a`-`t16s` protect same-edge C0=0 comparison, C0=1/R5=0 entry, R5 arbitration at C0=2,
  the R4/R9 C0>=2 write windows, exact-R0 behavior, R0=0/1 default adjustment, completion,
  active-adjustment freeze, exact short-line latch consumption, and retained-state lifecycle.
  t09h is now a required pass for the correlated freeze entry.
- `rtl/UM6845R.v` implements those evidence-backed counter paths. This report does not
  establish correctness for every pin-level v1.10 timing case; hardware-dependent paths
  remain verification work rather than inferred from adjacent behavior.

### Version, citation, and attribution refresh applied

- All three `docs/accuracy/compendium-0*.md` digests, plus
  `docs/accuracy/audit-findings.md` and `docs/accuracy/testbench-spec.md`, now cite v1.10
  using the nonuniform page mapping above.
- `docs/implementation-roadmap.md` and `docs/current-status.md` now identify v1.10 as the
  active baseline and v1.9 as historical comparison material.
- `docs/plus/architecture.md` records that the v1.10 CRTC0/2 changes do not alter the
  planned type-3 architecture.
- `README.md` and the affected reference documents now carry the Compendium's requested
  visible attribution. Whether to add a focused attribution comment to `rtl/UM6845R.v` is
  deferred to the implementation change, where its scope can be reviewed with the derived
  counter logic.

## Unresolved verification and implementation questions

1. For the `R5 == 0`, `C0 == 1` adjustment-entry case, what are the exact observable MA/DE
   and VSYNC transitions within the character? Counter-boundary C4/C9/RA behavior is fixed,
   but a hardware/SHAKER trace remains the best oracle for sub-cycle outputs.
2. Does real type-0 hardware expose any additional sync/interrupt edge during an R0=0 live
   rupture beyond the now-tested freeze, deferred increment, and recovery sequence?
3. Should the project expand the accuracy scope to CRTC 2? If so, encode the three internal
   states and no-HSYNC case directly; do not extrapolate the type-0 latch implementation.
4. Should future F4/F9 refactoring retain the focused source comment in `rtl/UM6845R.v`, or
   add a more explicit mapping from each retained arbitration flag to the Compendium state?

v1.10 supersedes v1.9 for new CRTC work. Continue with F4's t07/t08 equality/overflow
checkpoint while preserving t09h and t16a-t16s; v1.9 remains relevant only to this edition
comparison and historical review.
