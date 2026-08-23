# F6 decision gate — spurious type-0 border byte under R1>R0

Status: **option C chosen; Stage 1 landed 2026-08-23** on branch
`accuracy/a3-f6-stage1` (vectors `t10a`-`t10e` red first, then the substituted
border-start term in `crtc_type0_engine.v`, injected ahead of the wrapper's
SKEW-DISPTMG delay line in `rtl/CRTC.v`; golden soak hash re-minted to
`0x326ea81358e7d88f`, delta protected by t10a-t10e). **Stage 2 measured
2026-08-23**: the visible seam through the GA40010 co-sim route is 1 µs.
**Stage 2b disambiguated 2026-08-23**: ACCC pp.186/195 require a 0.5 µs
CRTC-side DE pulse; test and motherboard clock phase match, and the original
and synchronous GA paths agree. Finding promoted as F13, blocked on hardware;
no production RTL changed. Renaud's stated priority 2026-08-22:
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

The audit's fix prompt assumed "DE is consumed by the GA at 1µs granularity here". That
premise is wrong, but Stage 2b also refuted this gate's first replacement premise:

- **F6 is an exceptional sub-character DISPTMG transition.** ACCC p.186 draws DISP ON for
  the first byte of C0=R0 and BORDER for its second byte. Section 19.2.4 p.195 is explicit:
  BORDER is sent 0.5 µs after C0=R0 and disabled on the next character 0.5 µs later. The
  Stage-1 full-character DE dip is therefore an approximation, not exact pin behaviour.
- **The GA40010 recreation resolves DISPEN at byte phase, but does not own this delta.**
  `rtl/GA40010/ga40010.sv` latches `DISPEN` into `DISPEN_BUF` twice per microsecond
  (`vidbuf_clk_en = cen_16 & (S == 8'he0 || S == 8'h03)`); `rtl/GA40010/video.sv` then
  feeds it through the die-schematic gate chain (`u1008 = load ? DISPEN_BUF : u1005`) into
  per-tick `ink_sel`/`border_sel`. VCD comparison shows the original async (`video`) and
  synchronous (`video_sync`) paths produce identical full-character `border_sel` for the
  full-character input pulse.
- **Motherboard byte steering is not the Stage-2 owner.** `rtl/Amstrad_motherboard.v` in
  the `sync_filter` path (`vram_din_shift <= crtc_de ? vram_din[15:8] : 8'd0`, line 220)
  blanks a VRAM byte slot using char-granular `crtc_de`. This glue is fork-added convenience
  blanking, not die behaviour, and is conditional on an OSD setting — real hardware has no
  such mechanism. It was absent from the CRTC+GA Stage-2 route, so cannot explain that
  route's 1 µs result.

## Options

| Option | Cost | Fidelity | Notes |
|---|---|---|---|
| **A. Defer** | zero | Type 0 shows continuous display under R1>R0 (wrong) | Current state. SHAKER Module A `(O)` (8 tests) stays dead; §28.1.6 discriminator fails |
| **B. One-character approximation** | S-M, one optional commit at C4 | Pin-exact? No: seam 2× too wide (1 µs vs 0.5 µs) | Char-aligned DE pulse is the same code as option C's first stage; only the claim differs |
| **C. Full fidelity** | M, staged | Potentially exact | See staged plan below. No GA40010 change expected |

## Option C staged plan (with revert points)

1. **Stage 1 — character-granular pin approximation in the type-0 engine / `rtl/CRTC.v` wrapper**
   (**DONE 2026-08-23**, branch `accuracy/a3-f6-stage1`): when
   `R1_h_displayed > R0_h_total` and `hcc == R0_h_total`, force DE low for that character;
   inject before the existing skew delay-line mux so SKEW-DISPTMG delays/suppresses it like
   a natural border edge (§19.2.4 substitution; note the author-question caveat about the
   p.195 placement ambiguity). Comment cites ACCC §17.6.2. Stage 2b established that this
   gets presence/type/skew right but duration/phase wrong: the book requires a half-character
   pulse. Deterministic vector t10 (both types + skew placement) currently pins the
   approximation and must not be treated as the final hardware fixture.
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
3. **Stage 3 — BLOCKED-PENDING-HARDWARE-EVIDENCE (F13):** confirm the CRTC DE transition
   phase on real type-0 hardware, then replace the Stage-1 full-character pulse at the CRTC
   engine/wrapper boundary if confirmed. The GA and motherboard glue are not implicated by
   current evidence; no production change before SHAKER/logic-analyser confirmation.
4. **Stage 4 (optional, much later): §19.2.5 disintegration** double-R8-write cases — only
   after Stage 2/3 evidence, and gated on the ⚠ p.196-197 visual-tier diagrams.

## Upstream justifiability (the condition attached to choosing C)

- `CRTC.v`/engine change: current code implements the discriminator but not the documented
  half-character pin phase. A correction is justifiable only after the F13 hardware gate.
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

Disposition at the Stage-2 stop — **measurement recorded, no RTL change**.
Its initial inference that narrowing belonged downstream of the CRTC pin is
superseded by Stage 2b below. Consequences retained from the measurement:

1. Through this recreation, option B (char-aligned approximation) and Stage 1
   Stage 1 are visually indistinguishable — both render a 1 µs seam here. The
   pin-level difference remains real and vector-pinned; only the visible
   payoff of full fidelity is absent so far.
2. Hardware is the authority and outranks both the book reading and the
   simulation: SHAKER Module A (O) against the Logon System reference photos
   decides. Stage 2b narrows the expected owner to the CRTC DE phase; until that
   session, no code path should be changed toward either number.

## Stage 2b disambiguation — paper + phase ownership, 2026-08-23

### Visual ACCC reading

- **p.185 §17.6.1 control:** with R1=R0 the entire C0=R0 cell is BORDER: one
  character, 1 µs.
- **p.186 §17.6.2 R1>R0:** the chronogram splits C0=R0 into two byte columns:
  first byte DISP ON (green), second byte BORDER (orange), then full DISP ON at
  C0=0. Prose says exactly one 0.5 µs border byte immediately before C0 goes to
  zero. For R0=0 it states exactly: one DISP ON byte alternating with one DISP
  OFF byte. Type 1/3/4 remain continuously DISP ON on p.187.
- **p.195 §19.2.4 resolves pin ownership:** when C0=R1 is unreachable, the
  BORDER signal is sent 0.5 µs after C0=R0, then disabled on the next character
  0.5 µs later. Thus the CRTC DE dip itself is sub-character; the GA is not
  expected to halve a full-character pulse.
- **Q15 default reading (p.190):** despite the contradictory "condition R1 is
  fulfilled (BORDER R1 is false)" sentence, the operative state is BORDER-R1
  false. The immediately preceding mechanism says DISPLAY ENABLE goes ON at
  each character start and OFF 0.5 µs later, producing byte alternation.
- **Q16 default reading (p.188):** "(1st line-character R6)" does not restrict
  the check to the first scanline. The same page explicitly says C4=R6 is
  considered immediately regardless of C0 (except the separately described
  type-3/4 behaviour). These defaults follow the plan's visual-tier fallback;
  author confirmation remains welcome but neither is blocked.

### GA / CRTC phase map

The 16-state Johnson sequence is
`00→01→03→07→0f→1f→3f→7f→ff→fe→fc→f8→f0→e0→c0→80→00`.

| Event | S phase / evidence |
|---|---|
| CRTC character step | `CCLK_EN_N`, S=`03` |
| Opposite CRTC phase | `CCLK_EN_P`, S=`e0` |
| DISPEN samples | S=`e0` and S=`03` into `DISPEN_BUF` |
| Video load windows | `S[5]^S[6]` decodes at S=`3f`,`c0`; registered `load` is consumed at S=`7f`,`80` |
| Border commit | `u1008` takes `DISPEN_BUF` in those windows; `border_sel` follows per pixel |

- `ga40010_test.v` uses `.CLKEN(CCLK_EN_N)`. Production
  `Amstrad_motherboard.v` uses the identical `.CLKEN(cclk_en_n)` and additionally
  `.nCLKEN(cclk_en_p)`. Adding that omitted production `nCLKEN` connection to
  the temporary Stage-2 top leaves the result unchanged: 199/199 type-0 rows,
  16 px seams. There is no test-top half-character phase mismatch.
- In the Stage-2 VCD, DE changes just after S=`03`; S=`e0` captures it,
  `u1008` commits at the subsequent load window, and `border_sel` remains active
  for one full character because the input pulse is one full character.
- Verilator contains both the original async netlist path (`DISPEN_BUF_a`,
  `video`) and synchronous production path (`DISPEN_BUF`, `video_sync`). Their
  S sequence, `u1008`, and `border_sel` transitions are identical at the seam;
  GA sampling-placement divergence is not supported by repository evidence.

### Verdict

**Owner: CRTC-side sub-character DE phase required.** The test harness uses the
production phase, both GA implementations agree, and the book directly specifies
the half-character CRTC signal. ACCC-reading nuance is ruled out by the p.186
chronogram plus p.195 prose. No production RTL changed. F13 records the formal
hardware block: a SHAKER Module A (O) capture should measure eight mode-2 pixels,
and a logic-analyser capture of type-0 DE should show low only from the midpoint
of C0=R0 to the next C0=0 boundary. Only that evidence graduates a CRTC
engine/wrapper correction; GA/glue changes are not indicated.
