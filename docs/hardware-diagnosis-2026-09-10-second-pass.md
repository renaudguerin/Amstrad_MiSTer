# Hardware diagnosis second pass — 2026-09-10

Independent check of the [September 10 diagnosis](hardware-diagnosis-2026-09-10.md)
(D1–D5, GPT-6 Astra) against the unchanged production source, plus a numeric
comparison of every SHAKER 2.7 capture from the [September 9 retest](hardware-evidence-2026-09-09.md)
with the Logon reference photographs. Verdict: D1–D5 stand, D1 is broader than
recorded, one further code-level lead (D6) follows from the RFD rule, and the
apparent A (4) mismatch dissolves on direct image inspection (D7).

Evidence retained locally (ignored, not in Git) under
[references/hardware-diagnosis-2026-09-10/](references/hardware-diagnosis-2026-09-10/README.md):
`shaker/transcription-gemini-2026-09-10.md` (capture/photo transcription),
`parent-rerun-codex-2026-09-10.log` (diagnostic reruns and `make -C sim`),
`origin/type0-variant-codex-2026-09-10.log` and `origin/b9_origin_top_type0.sv`.
Checkout `master` at `91b7d41`; RTL unchanged since `ce1d2da`.

## Confirmation of D1–D5

| Finding | Check performed | Result |
|---|---|---|
| D1 origin VSYNC parity | Read `rtl/CRTC.v` `vsync_ivm_mid`/`vsync_fire` and `crtc_type1_engine.v` `vsync_line_fire`; reran `reproduce.sh origin-prod` / `origin-control` | Mechanism as described. Production: 3 of 6 frames lose VSYNC at R7=0 (R9=7 and R9=8); control: 0. |
| D2 ASCAL DE geometry | `sys/ascal.vhd` `i_hs` has no consumer; GHDL not rerun | Consistent with the retained logs; not independently rerun. |
| D3 sprite first row | Reran `reproduce.sh sprites` | 4 failing cases, all `All16 tgt Spr15` at X=0/16/32/64, first line `miss=16`. |
| D4 Plus PPI control read | `rtl/i8255.v` returns `mode` for every control read; Thacker's text says `80–8F → 00`, `90–9F → FF` | Reran `reproduce.sh ppi`: `82→82` and `9B→9B` FAIL as recorded. |
| D5 ROM 0 → page 3 | `plus_mmu.v:179` and `Amstrad.sv:1213`; `docs/plus/hardware-test-checklist.md:55` | Code follows the repository's own digest. Still a hypothesis until the `/EXP` rule is settled; the one-line page-1 experiment remains the right discriminator. |

`make -C sim` on the unchanged source: 192 classic vectors pass, only the known
`fdc-payload-poll` XFAIL. Working tree clean afterwards.

## SHAKER 2.7 captures versus reference photographs

Transcribed by a Gemini analyst from the PNG captures and the WEBP references
(row by row, all hex values). Amspirit 2.0 is listed as a secondary comparator only.
The A (4), B (9), A (T) and A (U) pairs were afterwards viewed directly in the
main thread; the transcription was exact in every checked cell and shape. The
A (4) error below was an interpretation error on top of a correct transcription.

| Test | Result |
|---|---|
| A (4) UPDATE CRTC R0 TIMING | **No CRTC mismatch after re-reading the images directly.** The `UPD R0=3F ON C0vs=0/1 (PREVIOUS LINE R0=1)` bytes are PPI port B reads: low five bits `1_1110` / `1_1111` (50 Hz, Amstrad brand, VSYNC 0 then 1) on all three sources; only bits 7–5 differ (real `010`, MiSTer `111`, Amspirit `000`), and those are cassette-read, printer-busy and `/EXP`, not CRTC state. The `C4==R4` rows read `0000` on real hardware, MiSTer and Amspirit alike on CRTC 1. All 7 `UPDATE R0=7F ... OK/KO` rows match. Remaining open cell: `OUTI ON C0vs=#3c` reads `01` on the real photograph, `00` on MiSTer and Amspirit; the photograph predates SHAKER 2.7 (no `OUTI C0vs 3C ON C9=0..7` row), so version or machine dependence is possible. |
| A (U) R4 & R9 CHECKING | **All 14 common rows match** (`xKOx`, `OK`, `OK`, the `3C=00…05=01` series, twenty `00`). MiSTer shows one extra row absent from the reference photo (version difference). |
| A (T) R2 UPD DURING HSYNC | Parameters identical. Black zone columns `0B..13` on both. The reference's rounded top-left shoulder (to `0A`) and top-right horn (to `14`) are the CRT's horizontal-PLL settling over the first lines after the R2 change. A regenerated-sync scaler path cannot show that transient; judge A (T) on MiSTer by the steady-state column extents, which match. Live/Off captures lose sync entirely (D2). |
| B (1) INTERLACE C4/C9 COUNTERS | Full capture matches reference `A2` in raster structure (no numeric readout on this screen). Live/Off garbled (D2). |
| B (9) INTERLACE VM | **Mismatch = D1.** First two blocks `#4E00/#4E40/#4E40/#4E80/#4E80` versus `#2740/#2760/#2780/#27A0/#27C0`; the `#43C0`, `#1820…#18A0` and `#25C0` rows match. Real values are 157…159 lines; MiSTer's are whole-line pairs of frames, so the half-line steps are lost with the pulses. Screen 2 of the reference (`R8 UPDATE DELAY`, `EVEN+ODD FRAME` blocks) was not captured. |
| C (1) RFD & PARITY STORY | Default interactive state (`R5 LINE=00 VAL=FF`, `R8 LINE=00 POS=00/VAL=71`): grid rows A–V and header values all match. Only cursor-highlight pixels differ. This state does not trigger an RFD, so it does not test the parity rule below. |

## D1 is broader: both CRTC types, rule §19.7.2

French ACCC v1.11 **§19.7.2 p.219** states the rule directly for CRTC 0, 1 and 2:
with R7=0, VSYNC handling coincides with the frame start, ParityFrame handling has
priority over VSYNC handling, and the C4/R7 comparison is processed *after* the
parity toggle. That is the repair contract, sharper than the §19.5.3 table.

The origin fixture rebuilt with `.crtc_type(1'b0)` (`origin/b9_origin_top_type0.sv`,
`ivm` probe replaced by `interlace[0]`) on unchanged RTL:

| CRTC 0, R7 / R9 | Result |
|---|---|
| 0 / 7 | 3 of 6 frames miss VSYNC; surviving rises on `parity=1` frames at C0=31, interval 22,528 µs |
| 0 / 8 | 3 of 6 frames miss VSYNC |
| 24 / 7 and 1 / 8 | one rise per frame |

So the wrapper's type-0 `field` path has the same origin hazard. Two things for
the repair: cover both types in the failing regression, and check the type-0
phase convention. In the type-0 run the mid-line rises (C0=31) coincide with
`parity_frame=1` even at R7=24, while §19.7.2 puts MID-VSYNC on the *even*
ParityFrame; the type-0 ParityFrame comes from the ParityR6 anticipation
(§19.5.2 p.206), so verify which polarity the wrapper's flop encodes before
calling that a second defect. The SHAKER B (9) CRTC 0 reference photograph is
the hardware oracle; the user has only run B (9) on CRTC 1.

## D6 — RFD frame parity is a private flop; §11.6.2 IVM ON/OFF cannot fix it

**Status: source-derived RTL divergence; failing vector not yet written.**

French §11.6.1 p.90: after an RFD the C9=R9 test at C0=R1 uses the current
frame parity; on the odd frame (case 1) VMA′ is not saved and characters
repeat, on the even frame (case 2) the save is normal; unfixed, this is a
stroboscopic effect between two frames. §11.6.2 p.90: that parity is the one
that toggles at every new frame *and in the other cases of §19.5.3*, and
`OUT R8,3` then `OUT R8,0` with odd R9 on an even C9 fixes it even. §19.5.3
p.209 confirms ParityFrame toggles at every C4=C9=C0=0 whatever R8.

Production `rtl/crtc_type1_engine.v` gates the VMA′ save with
`rfd_frame_parity`, a flop that only toggles at `frame_new_w` when R9 is odd
and is otherwise touched by nothing: the R8 toggle stage machine
(`stage_a_edge`/`stage_b_edge`) writes the wrapper's `parity_frame`/`parity_c9`
and never this flop. The wrapper model does implement the ON/OFF fix: with odd
R9 and even C9, stage A sets ParityC9=0 and stage B sets ParityFrame=0 from
either starting parity (the t21 panel vectors, render-verified pp.210–211, pin
that). The RFD gate simply does not read it. Consequences:

- After an RFD, IVM ON/OFF leaves the alternation running: every other frame
  keeps repeating characters. On hardware the same sequence stops it.
- The case-1/case-2 assignment relative to the wrapper parity is unpinned
  (`rfd_frame_parity` resets to 0 and the existing
  `t13b_type1_rfd_alternates_save_by_frame_parity` vector encodes the
  implementation's own convention). §11.6.1 says the *odd* parity is the
  failing test.
- With even R9 the private flop never toggles, while §11.6 says the frame parity
  "combined with the C9 counting mode" (ParityC9, per row) drives the test.

Predicted hardware signature: CRTC 1 only, independent of sync mode, a
two-frame alternation between a correct and a row-repeating picture in any
program that triggers an RFD and relies on IVM ON/OFF. That is the September 9
DSC4 description (garbled *and* flickering on CRTC 1, garbled without flicker
on CRTC 0), and `docs/backlog.md` already records DSC4 as an RFD user. It is a
hypothesis about DSC4, not a proof; the direct oracles are SHAKER **C (4)
IVM ON/OFF** and the interactive C (1) with an R5 write armed, neither captured yet.

**Repair direction:** gate the RFD save on the wrapper parity state
(`parity_c9`, which equals `parity_frame` for odd R9 and follows the per-row
alternation for even R9) instead of a private flop, decide the case-1 polarity
from §11.6.1, and land it with a vector that performs RFD, then IVM ON/OFF on an
even C9 with odd R9, and expects every later frame to save VMA′ on every row.
That vector must fail on the current source. Keep the t13b vector; re-derive its
polarity from the section rather than from the simulator.

## D7 — A (4) `FEFF` versus `5E5F`: port B configuration bits, not CRTC timing

**Status: withdrawn as a CRTC finding after direct image inspection; one
cell stays open.**

Reading the three CRTC 1 images directly (real photograph, Amspirit 2.0,
MiSTer capture) rather than the transcription: every `UPD R0=3F ...` row is a
pair of PPI port B bytes whose low five bits are `11110` then `11111`, i.e.
50 Hz, Amstrad brand bits, and VSYNC low on the first read and high on the
second. That timing content is identical on all three. The differing high
bits are the machine, not the CRTC: `Amstrad_motherboard.v` builds port B as
`{tape_in, 2'b11, ppi_jumpers, vs_sel}`, so MiSTer reads cassette=1,
printer-busy=1, `/EXP`=1 (`FE/FF`); Longshot's photographed machine reads
cassette=0, printer-busy=1, `/EXP`=0 (`5E/5F`, an expansion was attached);
Amspirit reads all three as 0 (`1E/1F`). The `C4==R4` rows read `0000` on all
three CRTC 1 sources. Renaud's Amspirit CRTC 0 screen shows the test's CRTC 0
variant: eight rows, `1E1F` even on the `C4==R4` and `C4==R4=0` rows, one
`1F1F`; it is a different screen, not a contradiction.

Two small residues:

- `OUTI ON C0vs=#3c` reads `01` (I/O on the 5th NOP) on the real CRTC 1
  photograph and `00` (4th NOP) on MiSTer and Amspirit CRTC 1, while the
  Amspirit CRTC 0 screen reads `01`. The photograph is from an earlier SHAKER
  build, so this needs the author's word on version and machine before it
  becomes a vector; question filed in
  [accc-author-feedback.md](accuracy/accc-author-feedback.md).
- Port B bit 7 idles at 1 on MiSTer and at 0 on the photographed 6128. A
  fidelity footnote only; nothing in the September evidence depends on it.

## D8 — "leftover DSC4 at the bottom" is most simply the scaler framebuffer

Astra's reset analysis (no RAM scrub on reset) is right, but the simpler reading
of the A (T) Live capture is D2 itself: Live blanking shortens the DE-derived
acquisition height, the scaler's framebuffer rows below the new height are never
rewritten, and a core reset does not touch DDR. That predicts the remnant is the
*bottom rows of the last taller picture*, at the bottom only, and survives reset.
Cheap discriminator on hardware: after DSC4, show a plain BASIC screen in Full,
switch to Live; if the band now shows BASIC's bottom rows rather than DSC4, it is
the framebuffer, and the RAM-pattern experiment is unnecessary.

## B6 boundary: brief for the next session

**The open question in plain words.** A real CPC sends the monitor a picture and
sync pulses. When a demo moves HSYNC mid-frame, two visible things happen: the
Gate Array paints black during the pulse (a bar that moves), and the monitor's
horizontal PLL physically displaces the next lines while it chases the new
pulse (the A (T) shoulder, the slanted ruler ticks). Do SHAKER's visual entries
and DSC4 look right because of the bars only, or also because of the monitor
displacement? Bars only: paint them at the raw position on HDMI. Displacement
too: add a small monitor model (per-line horizontal offset derived from the
raw HSYNC phase history) on top, as Amspirit evidently does on an LCD.

**Decider.** One CRT session: MiSTer 15 kHz analog output (SCART), Sync filter
`Off`, SHAKER A (T) and DSC4, photographed. If they match the Logon photos, the
core's raw timing is proven and the HDMI work is only the painting stage. Off is
"nearly raw" (its scaler-oriented blanking fallback/watchdog was never tested on
a CRT), so a true raw-sync mode may still be needed; it is core code.

**B6 is required either way.** The scaler must always get the stable Full
tuple, and the raw HSYNC phase must exist as a separate signal before anything
can be painted from it. A monitor model consumes that same signal; it extends
B6, it does not replace it.

**Design inputs already on hand:** [b6-architecture-decision.md](b6-architecture-decision.md)
(decisions 4, 5 and the B1 follow-up sequence), D2 above (ASCAL geometry from
DE only), `rtl/crt_filter.v` (`SHIFT` feeds VRAM byte assembly; `BEGIN_HBORDER`
constants), `crt_filter_output_select` and the `sync_filter` wiring in
`rtl/Amstrad_motherboard.v`, the GA's existing raw-HSYNC RGB force blank,
ACCC §14–§15 (HSYNC), and the September 9 Full/Live/Off captures. Missing for
validation only: the CRT photographs above. Scope: core files only; no `sys/`
change. Route the design to an architect-class model with a cross-provider
reviewer; implementation slices are bounded once the boundary is written.

## Diminishing returns?

Not yet for code-level findings: this pass produced D6 and the type-0 half of
D1 from source reading. D7 is a lesson in the other direction: a transcription
is not evidence until the images are viewed directly. What has stopped
paying is *image similarity* on scaler output (DSC4, A (T) shape, Live/Off
captures), which D2 explains and which no CRTC change can settle. The productive
loop is narrower than "automated hardware feedback" in general:

1. **Numeric SHAKER screens on hardware, both CRTC types, Full sync.** A (4),
   A (U), A (I), B (9) (both screens), C (1)–C (4). These are program-readable
   results (PPI VSYNC, counters), so the scaler cannot corrupt them, and one
   Gemini transcription pass turns a capture into a diff table. The
   [B2 driver](mister-hardware-loop-driver.md) already covers load and capture;
   what it lacks is keyboard input to select tests, which is the one thing
   worth automating next.
2. **Repair D1 (both types, §19.7.2) and D6 together in the accuracy stream**,
   each with a failing vector first, then retest B (9) and C (4).
3. **Keep DSC4 and Amazing Demo as holdouts** until D1/D6 land and the B6
   acquisition contract exists; they mix three causes today.
4. Plus stream unchanged from the September 10 sequence (System/BASIC boot first).

None of this needs MiSTer framework (`sys/`) changes. The acquisition contract
lives in `crt_filter` / `crt_filter_output_select` / `Amstrad_motherboard.v`
(B6 decisions 4 and 5): the scaler keeps the stable Full tuple, and raw HSYNC
or blanking effects are rendered into the pixels at an explicit byte-phase
boundary. The analog output does not escape this either: direct video inherits
the same pre-scaler HBLANK/DE contract, so a CRT would see `crt_filter`'s
regenerated sync unless the deferred raw physical-sync output mode is built.
That mode is the only way to compare physical sync edges, and it is core code too.
