# Emulator source cross-check of open ACCC questions — 3 November 2026

## Accepted conclusions

CPCSyntaxError gives useful alternative implementations, but it does not resolve
all the hardware questions. Keep the RFD lifetime question and Q21/Q22 open.
Its R5=0 release route differs from the author's existing Q20 answer; treat that
as a source divergence to investigate, not a new hardware rule. No RTL was
changed from this audit and no emulator was executed for these conclusions.

| Question | Local source finding | Disposition |
|---|---|---|
| RFD parity lifetime, current French draft / Q23 | CPCSyntaxError tests live frame parity while `rfdActive`, then clears that flag on the first successful VMA' save | Supports our chosen implementation as a software model; does not distinguish silicon from latch-at-arm. Keep the question, naming the two ACCC statuses explicitly. |
| CRTC0 post-IVM-exit comparison, Q21 | CPCSyntaxError clears `ivmExitDeferred` at the next C0=0; MiSTer retains a frozen comparison until C9 wraps | A concrete alternative worth testing. **Keep Q21:** the French odd-frame exit table has a nonmatching case that continues past C9=6 without a row end; a universal next-line plain comparison does not explain that case. |
| OUTI/R7 visibility, `019D` | CPCSyntaxError dispatches block OUT at T-state offset 16; no UM6845R transparent-latch model was found | Does not settle the sub-character acceptance edge. Retain the focused fifth/fourth-microsecond hardware discriminator; author clarification remains useful if available. |
| Stuck R5=0 adjustment, Q20 and `02D3` follow-up | CPCSyntaxError re-enters adjustment at C4=R4 and overwrites `adjustStateEngaged` from live R5, which can release it with R5=0 | Diverges from the author-confirmed retained-adjustment rule. Its source comment cites SHAKER CE results, but those comments are not original-hardware evidence. Keep the same-edge R5-release question separate from Q20's settled C4 reset. |
| CRTC1 VSYNC width on R0=0, Q22 | CPCSyntaxError's type1 line-end path advances `vsyncCounter`; the raster-freeze exception is type0 | No discriminating hardware answer. Keep Q22. No live failure of CPCSyntaxError on SHAKER AI is claimed here. |

## Examined source identities

Repositories under `/Users/renaudg/code/`, read only. These are local revision
identities, not a claim that upstream has no newer commit.

| Repository | Revision | Worktree observation |
|---|---|---|
| CPCSyntaxError | `22de765ef44da46c55217e4dd7caa385d69a6045` (`v0.12.0`) | clean |
| konCePCja | `f15cdcdbbd0d2c68181f77ab09fb39af46ab30f4` | clean |
| cpcec | `c025aab961a796b918cc99bc3e16216ea65bb5d1` | local `.cpcecrc` modification and untracked build artifacts; untouched |
| caprice32 | `a0d78f0f7c6d19a5d3205d1b231bd4da100ec6f3` | clean |

The secondary implementations do not supply a comparable type1 RFD lifetime
model. konCePCja's `src/hw/crtc.cpp` is a simpler procedural counter model;
cpcec's CRTC behavior lives in `cpcec.c`; caprice32's `src/crtc.cpp` targets
type0 rather than the required type1 rules. Their absence of the relevant
state is not evidence against that state existing in hardware.

## RFD: what CPCSyntaxError actually distinguishes

`src/core/crtc_type_1.h`, `reloadNextRowAddress()` (lines 476–501 at the pinned
revision), uses ordinary row capture when `rfdActive` is false. While active,
even `interlaceField` suppresses capture unless `rfdIgnoresParity` is set.
After a permitted raster match it assigns `nextRowAddress = maRow & 0x3fff`
and sets `rfdActive = false`. Thus later captures no longer consult parity.
This is live parity followed by retirement, rather than an arm-time parity latch.

`src/core/crtc.cpp`, `newFrame()` (around line 905), separately clears
`rfdParityLocked`; it does not simply clear all RFD state there. Do not assume
its full RFD lifecycle is identical to MiSTer's merely because both retire on
successful save. The software comment explains an ACCC interpretation, not a
measurement of two independently observable physical latches.

French ACCC §§11.6.1–11.6.2 pp.90–91 distinguish the R12/R13 source state from
the parity-management state. The current question should ask whether successful
capture only retires the source state, making the parity state ineffective,
or whether parity was sampled earlier. D9 G/H acceptance in the existing
MiSTer evidence admits both candidate explanations. This audit did not run a
fresh CPCSyntaxError SHAKER sweep and cannot assert that its entire D9 passes.

A useful discriminator changes parity **after RFD arming but before the first
successful C0=R1/C9=R9 capture**. Live-parity and arm-latched-parity models can
then predict different first captures. Exact instruction phases and a normal
control must be derived before offering that sequence as a hardware test.

## Q21: source comments do not override the French exit tables

CPCSyntaxError's `src/core/crtc_type_0.cpp`, `c9AtR9()` (lines 164–172),
uses the doubled raster/parity comparison only while `ivmExitDeferred` is
true. `src/core/crtc.cpp` clears that flag when `horizontal == 0` (around
line 1522), then ordinary `raster == R9` applies. That is a genuine model
difference from MiSTer's F16 frozen-comparison lifetime.

The parent read the saved pdf-inspector French report/Markdown and rendered
French pp.221, 224 and 225 directly. In §19.8.1 p.225's **bottom-right odd-frame
exit** example, R9=6, R8 returns to 0 at C9=3/C9-VMA=7. The following rows show
C4=1 at C9=4, 5, 6 **and 7**. Therefore the table does not show an ordinary
C9=6 row end in that example. Other exit examples do return to normal row
counting. This is exactly why reading the flattened table as a universal
next-line release would be unsafe.

The p.221 prose says the R8-selected state is taken into account when C0
returns to zero; that alone is not an explicit universal lifetime for a frozen
comparator. Preserve the [existing Q21](../../classic/accc-author-questions.md)
and its [hardware discriminator](crtc0-ivm-exit-wedge-fix-2026-10-27.md).
CPCSyntaxError supplies a candidate to compare, not authority to withdraw it.
Rendered evidence is ignored under `local/overnight-20261103/fr-p{221,224,225}.png`.

## R5=0 and OUTI boundaries

CPCSyntaxError's `updateVerticalType1()` in `src/core/crtc.cpp` (around
1033–1056) sets `adjustStateEngaged = (registers[5] & 0x1f) != 0` on a genuine
R4 match. Its later `heldOpenByZeroR5` path (around 1080–1107) uses that flag
when deciding whether adjustment can end. The code comment ties the re-entry
to its own SHAKER CE/A result. It establishes software intent and a possible
counterfactual; it does not prove original hardware releases adjustment there.

The [author's Q20 response](../../classic/accc-author-response-round2-2026-08-31.md#q20--n2--type-1-adjustment-after-r5-becomes-zero-1132)
explicitly preserves adjustment while C4 returns to zero on its ordinary
C4/R4 comparison. That higher-ranked evidence remains the rule. The exact
R5 write on the adjustment-completion decision edge involved in `02D3` is a
separate acceptance question; no claim that Q20 already settles that edge is made.

For OUTI, `src/core/emulator.cpp`, `cpcIoEffectOffset()` (around 1099–1140),
returns 16 for `block-out`. This microsecond-oriented dispatch does not model
all the physical E/CCLK visibility alternatives in the
[`019D` investigation](crtc1-outi-019d-2026-10-27.md). It neither proves an
R7-specific exception nor excludes useful author knowledge. Keep the existing
XFAIL and adjacent-phase control until independent evidence chooses the edge.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by
Longshot (CC BY-NC-ND 4.0).
