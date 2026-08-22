# F6 decision gate — spurious type-0 border byte under R1>R0

Status: **decision open** (default remains defer). Renaud's stated priority 2026-08-22:
exact hardware preservation, materialised by passing all SHAKER tests — strongly leaning
toward the full-fidelity path (option C), conditional on the validation gates below. This
file exists so any option can be picked up or abandoned without re-deriving the analysis.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).

## The rule

ACCC §17.6.2 p.186 (digest-03 §17.6): when `R1 > R0`, `C0=R1` never fires. Type 0
substitutes `C0=R0` as border-start trigger: one spurious border blip between every
scanline, **anticipated by 0.5 µs (one byte)**, "BORDER OFF" again on the following
character. Holds for any R0 incl. R0=0 (alternating display/border bytes). Suppressible via
R8 SKEW-DISPTMG; cancellable by the double-R8-write "disintegration" trick (§19.2.5).
Type 1 emits nothing — rows merge seamlessly (frame-merging effects rely on this fork).
The presence/absence difference is a documented type discriminator (§28.1.6).

## Interface evidence (corrects the original audit premise)

The audit's fix prompt assumed "DE is consumed by the GA at 1µs granularity here", which
made a half-character result look impossible without touching the Gate Array. That premise
is contradicted by the code:

- **Real DISPTMG edges are character-aligned.** The CRTC is clocked at CCLK (1 MHz); its
  pins toggle on the character grid. The sub-character visible effects elsewhere in the
  ACCC (blanking starting at the 5th/6th px-M2 of a character, §14.6/§16.2.1) are produced
  *inside the GA* from char-aligned pin events plus its byte-pair fetch pipeline.
- **The GA40010 recreation already resolves DISPEN at byte/pixel phase.**
  `rtl/GA40010/ga40010.sv` latches `DISPEN` into `DISPEN_BUF` twice per microsecond
  (`vidbuf_clk_en = cen_16 & (S == 8'he0 || S == 8'h03)`); `rtl/GA40010/video.sv` then
  feeds it through the die-schematic gate chain (`u1008 = load ? DISPEN_BUF : u1005`) into
  per-tick `ink_sel`/`border_sel`. The 0.5 µs border byte of F6 is exactly this pipeline
  reacting to a char-aligned DE edge.
- **One genuine suspect outside the netlist**: `rtl/Amstrad_motherboard.v` byte steering in
  the `sync_filter` path (`vram_din_shift <= crtc_de ? vram_din[15:8] : 8'd0`, ~line 213)
  blanks a VRAM byte slot using char-granular `crtc_de`. This glue is fork-added convenience
  blanking, not die behaviour, and is conditional on an OSD setting — real hardware has no
  such mechanism (border is decided by the GA's `border_sel`, not by zeroing fetched data).

## Options

| Option | Cost | Fidelity | Notes |
|---|---|---|---|
| **A. Defer** | zero | Type 0 shows continuous display under R1>R0 (wrong) | Current state. SHAKER Module A `(O)` (8 tests) stays dead; §28.1.6 discriminator fails |
| **B. One-character approximation** | S-M, one optional commit at C4 | Pin-exact? No: seam 2× too wide (1 µs vs 0.5 µs) | Char-aligned DE pulse is the same code as option C's first stage; only the claim differs |
| **C. Full fidelity** | M, staged | Potentially exact | See staged plan below. No GA40010 change expected |

## Option C staged plan (with revert points)

1. **Stage 1 — exact pin behaviour in the type-0 engine / `rtl/CRTC.v` wrapper**
   (type 0 only): when
   `R1_h_displayed > R0_h_total` and `hcc == R0_h_total`, force DE low for that character;
   inject before the existing skew delay-line mux so SKEW-DISPTMG delays/suppresses it like
   a natural border edge (§19.2.4 substitution; note the author-question caveat about the
   p.195 placement ambiguity). Comment cites ACCC §17.6.2. This is *not* an approximation —
   it is what the real pin does. Deterministic vector t10 (both types + skew placement).
   Revert point: commit is self-contained.
2. **Stage 2 — measure what falls out.** Verilator asserts the DE pin; the visible seam
   width comes from the GA+glue path. Two measurement routes, cheapest first:
   - **In-simulation**: `rtl/GA40010/ga40010_test.v` already instantiates `UM6845R`
     together with GA40010 and its Makefile renders raw RGB frames to PNG
     (`make video`). Render a frame with R1>R0 programmed and measure the seam width in
     mode-2 pixels (8 px = 0.5 µs) without any hardware session. This co-simulation
     harness predates the fork work and was forgotten — see the corrected non-goals note
     in `testbench-spec.md`.
   - **Hardware**: SHAKER Module A `(O)` against the Logon System reference photos for the
     selected CRTC type, at the next manual milestone session.
3. **Stage 3 — only if the seam measures 1 µs:** align or remove the fork-added byte-blanking
   in `Amstrad_motherboard.v` so border rendering goes through the GA's own `border_sel`
   like real hardware. Upstream framing: removes a non-hardware shortcut; netlist untouched.
4. **Stage 4 (optional, much later): §19.2.5 disintegration** double-R8-write cases — only
   after Stage 2/3 evidence, and gated on the ⚠ p.196-197 visual-tier diagrams.

## Upstream justifiability (the condition attached to choosing C)

- `CRTC.v`/engine change: exact documented pin behaviour with an ACCC citation — trivially
  justifiable.
- `GA40010`: expected **zero changes**. If Stage 3 ever seemed to require them, stop and
  re-examine the glue first; changing the netlist recreation is out of scope for F6.
- Motherboard glue change (if needed): defensible as removing fork-added behaviour that real
  hardware does not have; keep it minimal and cite the schematic-derived GA path it restores.

## Revert / switch conditions

- Any time before Stage 2 completes: fall back to B by keeping Stage 1 and describing it as
  the accepted approximation (roadmap wording applies), or to A by reverting Stage 1.
- After a SHAKER session: if photos show the seam correct → C done, record the milestone row.
  If subtly wrong in width/phase → Stage 3. If structurally wrong → back to A and document
  why the pipeline hypothesis failed.

Related constraint: sequencing with the per-CRTC model separation is discussed in
[crtc-per-type-separation.md](crtc-per-type-separation.md).
