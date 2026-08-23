# F6 decision gate — spurious type-0 border byte under R1>R0

Status: **option C chosen; Stage 1 landed 2026-08-23** on branch
`accuracy/a3-f6-stage1` (vectors `t10a`-`t10e` red first, then the substituted
border-start term in `crtc_type0_engine.v`, injected ahead of the wrapper's
SKEW-DISPTMG delay line in `rtl/CRTC.v`; golden soak hash re-minted to
`0x326ea81358e7d88f`, delta protected by t10a-t10e). **Stage 2 measured
2026-08-23** — see the dated section below: the visible seam through the
GA40010 co-sim route is 1 µs, not the documented 0.5 µs; recorded and stopped
(no GA/glue change made). Renaud's stated priority 2026-08-22:
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
   (**DONE 2026-08-23**, branch `accuracy/a3-f6-stage1`): when
   `R1_h_displayed > R0_h_total` and `hcc == R0_h_total`, force DE low for that character;
   inject before the existing skew delay-line mux so SKEW-DISPTMG delays/suppresses it like
   a natural border edge (§19.2.4 substitution; note the author-question caveat about the
   p.195 placement ambiguity). Comment cites ACCC §17.6.2. This is *not* an approximation —
   it is what the real pin does. Deterministic vector t10 (both types + skew placement).
   Revert point: commit is self-contained.
   Implementation notes: the term is combinational (`!CRTC_TYPE && R1>R0 && hcc==R0`) in
   `crtc_type0_engine.v`, matching §17.3's live comparator semantics; vectors are t10a-t10e
   (byte at C0=R0, type-1 control, skew 1/2 displacement, non-output blanking). Recorded
   residual: with R0=0 the frozen C0 holds DISPTMG off continuously; the book's
   alternating-byte description of that extreme (p.186) needs a toggle mechanism and is
   deferred to a later F6 stage.
2. **Stage 2 — measure what falls out.** Verilator asserts the DE pin; the visible seam
   width comes from the GA+glue path. Two measurement routes, cheapest first:
   - **In-simulation**: `rtl/GA40010/ga40010_test.v` already instantiates `CRTC` (the
     split wrapper; its Makefile manifest lists the wrapper and both engines) together with
     GA40010 and renders raw RGB frames to PNG
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

## Stage 2 result — seam width measured 2026-08-23 (`accuracy/f6stage2-soak-expand`)

Method, raw numbers, VCD mechanism trace, and reproduction sketch:
[evidence/f6-stage2-seam-measurement-2026-08-23.log](evidence/f6-stage2-seam-measurement-2026-08-23.log).
Summary:

- Route: `ga40010_test.v` CRTC+GA40010 co-simulation render (90f0cda manifest;
  elaboration from the checked-in HDL list re-verified), mode 2, zeroed VRAM,
  border pen distinct from display ink. Registers stock except R0/R1 under test
  and R2/R3 moved so the reshaped GA HSYNC band does not sit on the seam char
  (with stock R2=46/R3=128+14 the sync band swallows it — first attempt,
  negative result). Two geometries: R0=62/R1=63 and R0=49/R1=50.
- **Type 0: every display row shows exactly one interior border seam of
  exactly 16 mode-2 px = 1 µs** (199/199 rows, both geometries).
  **Type 1: zero interior seams** — rows merge seamlessly; §28.1.6 type
  discriminator holds.
- Pin level agrees with Stage 1 / t10a-t10e: DE low exactly one character at
  C0=R0 once per line (type 0); never (type 1).

Mechanism: the GA *does* latch DISPEN at byte phase (`DISPEN_BUF` samples at
S=0xE0/S=0x03 catch the drop mid-character), but the video pipeline
(`u1008 = load ? DISPEN_BUF : u1005`) renders the captured gap one-for-one:
`border_sel` runs a full character, displaced ~one character late relative to
the pin drop. The recreation neither halves nor anticipates the blip.

Disposition — **STOP, measurement recorded, no RTL change**: narrowing the
visible seam to the ACCC §17.6.2 p.186 "one byte" (0.5 µs) would require
behaviour changes downstream of the CRTC pins (GA `DISPEN_BUF`/`u1008`/
`border_sel` timing), which Stage 2 forbids itself. Consequences for the gate:

1. Through this recreation, option B (char-aligned approximation) and option C
   Stage 1 are visually indistinguishable — both render a 1 µs seam here. The
   pin-level difference remains real and vector-pinned; only the visible
   payoff of full fidelity is absent so far.
2. Hardware is the authority and outranks both the book reading and the
   simulation: SHAKER Module A (O) against the Logon System reference photos
   decides. If real silicon shows ~0.5 µs, the recreation diverges and a
   Stage 3-class question opens (GA pipeline timing vs. glue); if it shows
   ~1 µs, the ACCC 0.5 µs wording needs re-examination against the source.
   Until that session, no code path should be "fixed" toward either number.
