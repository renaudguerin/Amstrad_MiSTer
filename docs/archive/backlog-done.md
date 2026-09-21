# Completed backlog items

Archived verbatim from `docs/backlog.md` on 2026-09-21: items whose work is done, plus the
"Current priorities after the September 12 hardware retest" summary that `current-status.md`
now replaces. Residual follow-ups named in these items are tracked in their live homes:
`current-status.md`, `implementation-roadmap.md` (including the optional light-pen feature)
and `plus/asic-documentation-gap-map.md`. Relative links were adjusted for this folder;
backticked paths are relative to `docs/` and may predate the 2026-09-14 docs reorganisation.

## Current priorities after the September 12 hardware retest

**Integrated outcome:** reviewed B2 `5de8c5c` and B6 `c2fde66` were refreshed
without implementation changes to `cd08a1e` and `9849b9a` and merged
sequentially into `master`. All destination gates pass; both tasks are published at `9ee710c`.
Exact-SHA CI is recorded in the [status history](../archive/current-status-history-2026-09-21.md). The real capture loop and final-RGB/diagnostic
progress are recorded in the [handoff](../investigations/archive/hardware-followup-handoff-2026-09-12.md).
Remaining physical video, active-mode and CRTC/title acceptance stay open.

The [September 12 report](../investigations/hardware-runs/hardware-evidence-2026-09-12.md) confirmed BASIC boot fixed
on **6128 Plus / `5c16b17`**. Left-edge sprite corruption is much improved,
possibly fixed. Copter 271's logo is fixed on device with `c595031` / `b5c3014`
(PRI no longer fires on line 256+n), and its title flash is much improved with `05cb9fd`
(B19 DCSR bit 7 acknowledge latch).

Hardware testing on build **`a0778b6`** (fix "general: reset PSG R7 to 0x00 so
bare-metal keyboard scans work") confirms it **fixes all known keyboard and joystick
issues with `arn5diag`, `Pang`, and `Plotting`** (see [September 13 report](../investigations/hardware-runs/hardware-evidence-2026-09-13.md)).
AY-3-8912 /RESET clears all registers to 0x00 (GI datasheet); the previous 0xFF reset configured
Port A as output and wedged uninitialized R14 reads to 0x00 (active-low, meaning all keys and
fire buttons read as permanently pressed). This closes the held-fire defect in Pang/Plotting
and the inoperable keyboard in `arn5diag`.

Full versus Raw pixels shows no visible difference so far in Amazing Demo, DSC4 or
SHAKER A (T). B1/B6 visual acceptance, sprite edge closure, and remaining P10 title
stability remain open.

---

## D5 ROM 0 boot configuration: reported boot symptom closed

**Production input repaired; 6128 Plus hardware boot confirmed 2026-09-12.** D5 supplies low
`/EXP` for CPC Plus BASIC boot while preserving the live MMU decoder polarity.
The regression executes both unchanged BASIC CPRs on 6128 Plus and 464 Plus,
requires firmware `Ready`, and rejects disc-missing output and timeouts.
ROM7, direct-page and GX4000 controls are retained.

See the [D5 validation report](../plus/d5-basic-boot-input-2026-09-11.md) for gates
and review, and the [source handoff](../plus/architecture.md#additional-evidence-and-implementation-handoff)
for the accepted configuration rationale. Factory strap measurement is not an
implementation prerequisite. Other model/cartridge combinations and disk I/O
acceptance remain separate from the user's confirmed boot result.

---

## B5. ASIC documentation-gap map

**Priority: high. Cheap, and it converts "we don't know what we don't know" into a list.**

**DONE 2026-09-01:** `docs/plus/asic-documentation-gap-map.md` inventories the Arnold/CPCWiki
register families and externally visible behaviors against their production owner and
deterministic fixture. Most of the register page is owned. The actionable gaps are CRTC3
light-pen input, live ADC routing, sprite access-blank timing, three source-conflict
discriminators, undocumented DMA-fetch stalls, external-expansion semantics, and printer BUSY
sampling. B6 should consume that narrowed list rather than reopening the whole ASIC page.

The Plus symptoms are stubborn because the ASIC is far less documented than the CRTC, and we
have no explicit statement of where the documentation runs out.

**Method — invert the problem.** Enumerate every ASIC register and behaviour named in
`docs/references/ArnoldV15.txt` and the CPCWiki ASIC pages already in `docs/references/`. For
each row record: which RTL module owns it, which simulation test exercises it, and which
source documents it. The rows with no owner, no test, or no source *are* the gaps, made
explicit and prioritizable.

**Note on sources.** Arnold 5 is the Plus's development codename, so `ArnoldV15.txt` is
Amstrad's own documentation and outranks any emulator. Amspirit is the most accurate emulator
but is closed-source. Open alternatives (MAME's CPC driver, Caprice32) are weaker on the Plus
than the material already held. Emulator source is not a promising avenue here; the ArnoldV15
document, the CPCWiki ASIC pages, and Longshot himself are the real references.

---

## B7. Two cheap audits

**Priority: high. Both are small and both target a class of defect already observed.**

Motivated by the P10j sprite-RAM incident, where a large memory was inferred as thousands of
flip-flops instead of M10K block RAM and was only caught because ALM utilization approached
90%. That is a "knows Verilog, does not know synthesis" failure, and reviewers focused on
finding *mistakes* rather than reconsidering the *approach* will not catch its siblings.

**Audit 1 — synthesis inference sweep. DONE 2026-08-31, see
`docs/investigations/archive/b7-synthesis-inference-audit.md`.** Result: no second sprite-RAM-class defect. All 28
"uninferred RAM" instances are correctly too small for block RAM; every real memory inferred.
One follow-up: `asic_video` R16/R17 (CRTC3 light pen) are stuck at GND because nothing writes
them, and this is an unowned gap — F18 covers the classic CRTC readback only and is closed. Original scope follows. Read the Quartus fitter and Analysis & Synthesis
reports for the current build and check, for every memory-shaped structure in the design,
whether it inferred as block RAM or as registers. Also read the removed/stuck-register report:
anything optimized away as unreachable is either dead code or a wiring bug.

**Audit 2 — dark silicon test. DONE 2026-09-01 on `plus/b7-dark-silicon-audit`, see
`docs/plus/archive/b7-dark-silicon-audit.md`.** Result, reproduced independently by the parent rather
than accepted from the delegated report: **the observed Plus RGB/bus signature is
isolated from the tested classic mutations.** Mutating `CRTC`, `crtc_type0_engine`,
`crtc_type1_engine` or `ga40010` in Plus mode leaves the Plus signature bit-identical,
while the same mutations in classic mode do move
it, so the null result is meaningful rather than vacuous. All nine Plus modules shift the
 signature when corrupted, demonstrating that each participates in that exercised
 path; this does not prove every feature within each module is live. Two recorded limits: the two
 classic engines are not independently proven, and the fixture does not reach CRTC-type-divergent
 behaviour. Read-only review 2026-09-03 at `a98590a` records CLEAR on the primary
 Plus-live/classic-isolated result with those limits plus the TV80-surrogate CPU bound
 retained (`docs/plus/archive/plus-review-2026-09-03.md` §5). **B8 correction, 2026-09-08:**
 FIELD was not observed; its production output still belongs to classic CRTC in
 Plus mode. The earlier signature result remains valid, but does not establish
 complete output isolation. Original scope follows. For each Plus module, deliberately corrupt it in simulation
and assert that a Plus-mode output changes. Anything that stays green is not in the active
path. Do the mirror test for classic modules in classic mode. This directly answers "are we
running everything we built, and is a classic path overriding a Plus path".

---

## B12. Merge to `master` for an accurate README

**Status: Prepared 2026-09-10 ahead of merge to `master`.**

`master` previously presented the upstream core. A casual visitor who found the fork saw no
statement of its aims and no sign of the work on `accc-review-and-fixes`.

The original plan — clean separated PRs upstream, accuracy and Plus split cleanly — became
unrealistic after several hundred commits. The README has been rewritten to describe the fork's
aims, its two work streams, and its current status, and integration references have been aligned
to `master` ahead of merging `accc-review-and-fixes` into `master`.

---

## B15. Re-diff the re-issued ACCC v1.11 PDFs

**Filed 2026-09-11. Completed 2026-09-11.** Longshot re-issued both v1.11 editions with our
round-1/round-2 feedback applied, without changing the version number. The user's copies are
`docs/references/ACCC1.11-EN(b).pdf` (SHA-256
`69d6a6a77de472937d41778ad48fc4fb427a937a24d3054f6d42c0b6ccfcc3e9`, 296 pages) and
`docs/references/ACCC1.11-FR(b).pdf` (SHA-256
`28f25c73c1797578522f34ce9ff558210386972c9257b5b8081927483ee02c3b`, 295 pages); both are
gitignored like the originals.

Work completed:
- Extracted both new editions with `pdf-inspector` into `docs/accuracy/extract/inspector-v1.11b-{en,fr}/`.
- Performed full page-by-page text and vector diff against original v1.11 extractions.
- Rendered changed pages carrying tables or chronograms at 200 DPI into `docs/accuracy/extract/pages-v1.11b/`.
- Recorded complete difference report in `docs/classic/archive/accc-1.11-differences.md §4`.
- Updated `docs/accuracy/accc-author-feedback.md` (fingerprints updated, items in print marked).
- Migrated affected citations in code comments (`sim/plus/asic_video_test.cpp`, `sim/plus/b8_field_test.cpp`, `rtl/CRTC.v`, `rtl/plus/asic_video.v`).
- The `(b)` copies are now the working oracle; old files remain for provenance.
