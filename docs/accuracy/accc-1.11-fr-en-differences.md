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

The section-complete sweep is in progress. Findings below have been checked against the
original edition pages; absence from this interim ledger does not yet mean that a section has
no consequential difference.

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
