# ASIC R4 decision lifetime — 4 November 2026

## Position

There is a useful counter-lifetime discriminator, but no source-backed ASIC latch
repair is established yet. The current RTL evaluates live C4/R4 at the line seam.
CPCSyntaxError v0.12 can retain an equality set by an earlier R4 write. The French
source describes live end-of-line effects and the ROM-select boundary exception,
without explicitly defining whether a matching write survives a later unequal write.
Do not transplant the software latch or delay general CRTC admission from this evidence.

The original saved AmSpirit caller proves a coherent C802 counter outcome. It does
not prove the first FPGA divergence is at C802, or calibrate that instruction stop
to the rejected synthetic seam's CPU-fetch phase. Actual SHAKER BI pulse-width
comparisons independently rejected the broad admission qualifier; that rejection
remains binding for this investigation.

No code/test edits, simulations, builds, device/API access, emulator launches,
commits, pushes or delegation were performed. Only this report and ignored source-
reading artifacts under `asic/r4-counter-discriminator/` were written.

## Original evidence located

Accepted pre-initializer state: `asic/dsc4-plus-checkpoints/main-entry-c000/`.
The state before C802 and the fully single-stepped sequence are preserved in
`asic/dsc4-plus-write-chain/`, with `step-dsc4-write-chain.log`. These are authentic
boot/stepping artifacts, not execution from a later snapshot with the initializer
restarted. The older independent `r4-before-c802`/`r4-after-c802` checkpoints show
the analogous C4=R4=1 outcome, but belong to another captured invocation.

| Original same-caller stop | Tick | C0/C4/C9 | R4/R9 | BC |
|---|---:|---|---|---|
| C802, before OUT(C),0 | 92224211 | 63/2/7 | 2/7 | BD04 |
| C804, next instruction | 92224215 | 3/0/0 | 0/7 | BD04 |
| C80B, before R9 data write | 92224224 | 12/0/0 | 0/7 | BD04 |
| C80D, next instruction | 92224228 | 16/0/0 | 0/13 | BD04 |

The v3 snapshots identify Plus/type3/128K, not a guessed counter layout. C802 is
ED71, OUT(C),0; the saved state explicitly selects R4. At C808 the caller selects
R9; C80B is ED59, OUT(C),E, with E=0D. Register/counter stops are known; hidden VMA,
match-latch state and the within-instruction storage edge are not serialized.

The saved 2000-instruction history before C802 contains an earlier explicit R4←2:
C84D selects register4 via OUT(C),D with D=4; C850 increments B; C851 writes E=2
via OUT(C),E at BD09. The CPU state and bytes establish the transfer, but history
contains no C4/C9 at C851. It therefore cannot prove a matching-write latch was
armed. A static summary is retained as
`asic/r4-counter-discriminator/saved-history-r4-writes.json`, with the source
history path/hash. No runtime state was invented from that summary.

The rejected synthetic R4 seam fetched OUT(C),0 at internal C0=60 and stored it at
C0=63. The authentic AmSpirit stop is PC=C802/C0=63 before a four-us instruction.
Those are different observation anchors. Neither old counter-window tests nor the
saved command-start snapshot establish their absolute FPGA equivalence. In
particular, a title screenshot improving after an admission delay is not proof
that this synthetic seam reproduces the title's first counter failure.

## French authority checked

`lookup.py` was used for navigation. The existing French v1.11 pdf-inspector report
and primary Markdown were reused; classification is `native_partial`, with fallback
pages outside this material. §12.5 p.103 was rendered with `pdftoppm` and read
visually (`asic/r4-counter-discriminator/fr-103.png`). §10.3.4.1 p.78 was read in
the primary Markdown; no table/chronogram was used without its rendered form.

French §12.5 p.103, types3/4:

- Writing R4 equal to C4 makes C4 return to zero at the C9 row end and reloads the
  start address. R4 modification is considered immediately at line end.
- Writing R4 below C4 produces C4 overflow. This differs from C9/R9's >= behavior.
- For type3 only, R4←0 on C0=0 can precede the C4 reset if the I/O also selects ROM;
  that can prevent the reset. B bit5=0 activates this overlap; B bit5=1 does not.
  Type4 is excluded. PPI/printer/FDC overlap is not stated to change that speed.

French §10.3.4.1 p.78 says an R9 write at/below C9 resets next C9, then resets C4
if C4=R4 or increments it otherwise. This also states a comparison, without an
explicit type3 persistent last-frame-line latch. The explicit irreversible
last-line discussion in §12.4.1 belongs to type2 and cannot supply that missing
rule for type3.

C802 uses B=BD (bit5=1). The documented ROM-select overlap exception is therefore
not directly present. It remains a useful separate boundary control, not a reason
to add an R4/opcode admission exception for DSC4.

## Actual RTL and software hypothesis

In current `asic_video.v`, `last_charline = (charline == R4_v_total)` is live.
For the saved ordinary R8=0/R5=0 state, `body_frame_end = c9_done & last_charline`.
C4/C9 commit together on CLKEN with old C0=R0. If R4 becomes0 before this edge with
C4=2/C9=R9=7, last_charline is false and next C4/C9 is3/0. If the write shares the
edge, nonblocking assignments leave the old R4 visible to that decision and it
wraps to0/0. There is no retained old equality or distinct post-C0-wrap C4 stage.
Those are model facts, not a new physical phase rule.

The inspected CPCSyntaxError working source is clean at
`22de765ef44da46c55217e4dd7caa385d69a6045`; `v0.12.0^{commit}` resolves to that same
commit (the annotated tag object's own SHA differs). No executable was run.
`crtc_type_3.h:114–135`:

- `verticalTotalMatches()` accepts `verticalMatchForced` before the live equality.
- An R4 write equal to current C4 sets that flag.
- A later unequal R4 write normally does not clear it; the type3 ROM-select/C0=0
  exception can clear it.
- `crtc.cpp:1252` consumes the flag at row end; `newFrame()` also clears it.

This gives a concrete decision-lifetime hypothesis. It is not automatically the
DSC4 cause: C851's C4 is missing, and a matching write that survives an unequal one
is a stronger claim than the French text explicitly makes. Its instruction-level,
character-rounded I/O scheduler cannot settle the actual ASIC bus/decision edge.

## Proposed bounded discriminator, not run

Keep raw I/O admission and the R3/HSYNC logic unchanged. Use type3, R0=63, R9=7,
R5=R8=0, current C4=2/C9=7, and ordinary BD writes (no ROM-select overlap).
Land writes safely mid-line before testing a terminal edge:

| Case | R4 history in the terminal raster | Live comparison model | Sticky matching-write hypothesis |
|---|---|---|---|
| Equal-and-held | Start R4=3; write2 at C0=20 | Next C4=0 | Next C4=0 |
| Lower-and-held | Start R4=3; write1 at C0=40 | Next C4=3 | Next C4=3 |
| Match-then-lower | Start R4=3; write2 at20, then1 at40 | Next C4=3 | Next C4=0 |
| No matching write | Seed R4=2; write1 at40 | Next C4=3 | Next C4=3 in CPCSE, since seeding does not arm its write flag |

All predict next C9=0. The first two have direct §12.5 expectations. The third
isolates the disputed lifetime without any terminal bus-phase ambiguity; it should
remain a diagnostic/XFAIL question until hardware or an author clarification fixes
its expected outcome. The fourth catches an unjustified general "old equality was
once true" latch. An unconditional per-line type0/2 latch is not justified here.

This matrix is deliberately a **paper discriminator**, not a claim that the current
RTL fails a new source-established vector. The current live model predicts the
French-safe controls correctly. A generic held equality fix cannot be recommended
before the disputed row has an independent outcome. Retain original BI pulse-width
acceptance if any later counter-only hypothesis is implemented.

## Exact missing evidence and next action

First obtain the earliest natural divergence from the original pre-initializer
C000 state, preserving the original RAM/mapping and caller history. Do not restart
C000 from C802's already initialized/self-modified RAM. Around C851 and C802 record:
PC, accepted R4 bus data/port, stored R4, C0/C4/C9 immediately before/after acceptance
and the row-reset decision. At C851 we specifically need whether C4 was2. At C802 we
need whether R4 changed before the C4 reset decision, after it, or on the same edge.
Do not infer any unrecorded latch from the existing v3 SNA.

If those traces favor retained equality, the focused author/hardware question is:

> On type3, with R5=0 and C4=2/C9=R9=7, does an ordinary BD-port R4←2 at C0=20
> still force next C4=0 if R4←1 is then written at C0=40? Or does the later
> R4<C4 value cancel that decision and give next C4=3? Separately, when is the
> ordinary non-ROM-overlap terminal R4 comparison committed relative to C0=0?

This distinguishes a counter-decision lifetime from general admission and from the
source's explicit ROM-select race. Until it is answered or an actual FPGA trace
shows a different first divergence, the correct result is a named evidence gap,
not a production latch or a port-wide delay.

Technical information sourced from the “Amstrad CPC CRTC Compendium” by Longshot
(CC BY-NC-ND 4.0).
