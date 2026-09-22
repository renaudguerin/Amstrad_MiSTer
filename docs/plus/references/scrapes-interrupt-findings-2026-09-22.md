# Plus interrupt sources: findings and discriminators

Source ingestion at base `a33d93e967d71afb30143222cf6c23ca4f03f3f5`, 2026-09-22.
This document compares captured sources with current code; it does not establish
new hardware behavior or a cause of Sonic's defects. The
[inventory](../../reference-ingestion/scrapes-2026-09-22.md) maps S01–S35 to exact
local filenames. Page numbers below are **one-based PDF pages**, not web sections.
Bulk extracts and rendered copies remain local; no source PDF is reproduced here.

## Agreed programming surface

S01 (*ASIC*, pp.6–7), S28 (*Modes…*, p.3) and S22 (*L'ASIC*, p.7 vector diagram)
agree on vector offsets 0/2/4/6 for DMA2/DMA1/DMA0/raster respectively, with
IVR[7:3] supplying the high five vector bits. S01 p.6 gives raster > DMA2 > DMA1 >
DMA0 priority. S22 p.7's diagram/table assigns IVR[0]=0 to automatic acknowledge,
1 to explicit DMA clearing. Raster is separately automatically acknowledged.

**Verified integration:** `rtl/plus/asic_regs.v:625` samples this priority at
`intack && !intack_d` and holds `ack_src`; `auto_clr_dma` at 370 uses that same
edge and suppresses DMA clear while raster is pending. DCSR maps DMA0/1/2 status
to bits 6/5/4 (`dcsr` at 237); enable bits are 0/1/2. Writes clear selected flags
with ones. This is code evidence, not proof of every source's DCSR prose.

S03's address table pp.8–9 agrees on PRI/SPLT/SSA/SSCR/IVR at 6800–6805 and
SAR/PPR at 6C00/02, 6C04/06, 6C08/0A. Visually checked tables confirm the
associations. Its p.10 labels DCSR write-only, whereas S01 p.10 and
`asic_regs.v:581` provide readback. Do not copy its access-type column wholesale.

## Consequential source/model differences

| ID | Captured claim | Verified current implementation | Acceptance boundary / next discriminator |
|---|---|---|---|
| I1: empty vector | S29 (*Vectored Interrupt Bug*) pp.3–4 reports a second acknowledge after raster was cleared, with no DMA pending, returning DMA0 / offset 4. | `asic_regs.v:636` returns DMA2 / offset 0 when nothing pends; first-ack source is held during a continuous acknowledge. | Confirmed mismatch to the described source scenario. A focused two-distinct-ack vector can pin the discrepancy; separately trace whether production CPU/board logic ever generates this sequence. Do not silently generalize a bug-window result to every idle acknowledge. |
| I2: A13 bug applicability | S29 p.3 and S28 p.5 constrain the **interrupted instruction** address; S29 excludes I value and handler location and describes instruction-class dependence. DMA can be affected with IVR[0]=0. | The current vector path uses pending flags, not CPU A13; no A13-conditioned doubled-ack mechanism is present at this boundary. | Source/model difference, not a proven Sonic cause. Capture interrupted PC/instruction, /M1, /IORQ, /WAIT, request, vector and clear provenance. Table/ISR placement alone proves nothing about immunity. |
| I3: PRI range | S01 p.4 says PRI=10 repeats at 266; S22 p.8 agrees, with a visible-height qualification. S07 pp.1–2 describes wrap but gives the erroneous sum 255+8=263. | `asic_ga_timing.v:606` compares `{1'b0,pri}` to nine-bit `crtc_line`; motherboard line 477 supplies `{plus_vc[5:0],plus_rc[2:0]}`. It excludes ≥256 and adjustment, with no DE test. | Source conflict already present in the digest; now explicit there. Preserve the Copter-tested policy. Compare controlled 10/266 and 55/311 cases, holding R4/R6/R9 and frame origin fixed. Copter success does not establish all hardware configurations. |
| I4: PRI phase | S01 p.4 says exactly HSYNC-start+10 µs independent of width. Existing digest §7 prefers revised Arnold's monitor-trailing-edge / 6 µs account. | `asic_ga_timing.v:604–608` fires on internal shaped monitor HSYNC falling, not on `crt_filter` output. | Unresolved source/timing conflict. Measure raw HSYNC, shaped HSYNC and IRQ against a shared clock across R3 widths. Revised Arnold is cited by the existing digest but not among these local captures; its wording was not freshly verified. |
| I5: live PPR writes | S02 (*ASIC et DMA son*) p.4 says a changed pause unit takes effect immediately, with the counter returning to 0 next. | `asic_dma.v:253–262` decrements the existing prescaler until zero, then samples PPR. Register writes update PPR but expose no PPR-write strobe to this engine. | Confirmed source/model discrepancy. Test a mid-pause PPR change with a following INT marker; compare physical interrupt interval. First establish whether Sonic performs such a write. No title causality yet. |
| I6: DCSR read semantics | S01 p.6 describes active-low request bits; S22 p.7 calls bits 4–7 frozen in automatic mode. S28 p.4 and S22 pp.6–7 diagrams describe DI/EI and write-to-clear, not an unambiguous read waveform. | `asic_regs.v:237,430,585` exposes active-high live DMA flags; bit7 records last raster-ack provenance (`asic_ga_timing.v:641–651`). No IVR-mode read freeze. | Source disagreement, not a settled polarity defect. Read before/after ack in both modes, keeping CPU masking distinct from peripheral pending state. Do not infer hardware polarity solely from RTL or the DI/EI labels. |

## Source mistakes and limits to avoid inheriting

- **S29 pp.1–2 example vector table:** assigns handlers at offsets 0,2,6,8 with
  wrong channel associations. Use the agreed 0/2/4/6 table, not this listing.
  Its p.3 workaround also says “DCSR bit 0” for auto-clear control; IVR bit0 is
  the control described elsewhere in that same article.
- **S28 p.4:** describes the changing output-vector bit as an IVR bit. IVR[2:1]
  is not the dynamically encoded source field. Its claims about interrupts
  disappearing under DI in universal mode are not a substitute for bus evidence.
- **S02 p.2 and S22 p.9:** say REPEAT 0 equals REPEAT 1; S01 p.5 says REPEAT 0
  is NOP. The current `asic_dma.v` REPEAT branches ignore zero, matching S01.
  Preserve this as a source disagreement; a small loop/INT trace discriminates it.
- **S02 p.3:** says PPR=0 makes PAUSE equivalent to NOP. Current positive PAUSE
  still counts HSYNCs with PPR=0. Do not conflate zero instruction count with
  zero prescaler; this is another candidate timing discriminator, not a fix brief.
- **S01 p.7 versus S29 pp.3–4:** the overview describes a DMA0-to-raster mix-up;
  the detailed bug account explains raster-to-DMA0. S22 p.7 reports confusion
  in both directions. Keep scenario/direction attached to each observation.
- **DMA cadence:** S02 pp.2–4 and S22 pp.9–11 are useful AY-list navigation;
  S01 p.6 separates fetching all channels from sequential execution. Current
  `asic_dma.v` starts at raw HSYNC rise, snapshots enabled/non-pausing channels,
  then runs dead/fetch/execute states. “One instruction per HBL” is not evidence
  that every channel's INT occurs simultaneously or that LOAD/CPU contention
  can be ignored. Existing digest §9 already owns the detailed timing contract.

## Sonic: corrections and next useful trace

The Sonic notes were corrected in place: safe-zone applicability; raw PRI
comparison versus a visible-area gate; first-edge vector sampling; the stale
pre-B19 drop-on-ack snippet; and the invalid SPLT=255-to-line311 explanation.

**Verified arithmetic:** `asic_video.v:540–541` uses eight-bit
`{charline[4:0],raster[2:0]}` for SPLT, independently of PRI's nine-bit comparison.
With R9=7 and unmodified counters, line311 maps to55; SPLT=255 next repeats at511,
not311. The character-R1 capture deadline is real code; a Sonic write missing
that deadline remains a hypothesis until register/bus traces establish it.

**Already implemented:** `asic_ga_timing.v:637–687` retains raster fire across
`int_ack_active`; a new investigation must inspect this B19 latch, not reapply
its repair. The vector and auto-clear use the same first-ack priority. The
same-channel DMA set/clear collision is clear-dominant in the current assignment
(`asic_regs.v:430`), despite its nearby set-dominant comment. This is a confirmed
comment/code mismatch; its hardware semantics need a discriminator before calling it a bug.

Recommended order for the next Sonic task: verify the reported IM2/IVR setup
against the actual cartridge/trace; capture raw counters and PRI/SPLT/SSA writes
with interrupt source/ack/vector; use I1/I2 only if that trace reaches their
conditions, and I5 only if live PPR writes occur. None of this ingestion proves
the cause of garbled menu text, sky flicker, or a hardware fix. Original hardware
and the repository's ACCC authority hierarchy remain unchanged.

## Verification record

All seven priority PDFs were classified by pdf-inspector 1.17.0 as native text.
Primary reading used its Markdown; `pdftotext -layout` only located PDF pages.
Rendered inspection covered S03 pp.8–10 and S22 pp.6–7,9 plus S28 pp.2,4, including
vector/ack and DCSR diagrams. Diagram arrow/bit mappings were compared with code.
A bounded Gemini source pass and native RTL scout supplied leads; parent checking
corrected page-number errors and rejected claims that diagrams proved DCSR read
polarity or that a software regression universally refuted PRI wrap. No simulation,
synthesis or device action was performed for this documentation-only change.
