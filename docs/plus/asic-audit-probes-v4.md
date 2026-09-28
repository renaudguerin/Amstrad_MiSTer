# ASIC audit: V4 hardware-probe design

Design date: 2026-09-28. The design below received independent review **before
building a cartridge**. Its hypotheses are retained separately from the
implementation observations at the end. No hardware result is claimed. The scope is exactly five
new screens, appended to V3: 26 PA1, 27 PA2, 28 PA3, 29 PA4 and 30 PA7. Preserve
screens 01–25, their numbering, handlers, timing constants and geometry. No RTL
change is part of this work. Phase 2 vectors and their acceptance status are
separate from this design.

The operational hypotheses below have independently derived predictions. They
do not all map one-to-one onto the source prose: PA2 contains equivalent boundary
readings, and PA7 cannot adjudicate a general Plus-versus-CPC timing claim using
only a Plus. Those limits must remain visible in the implementation and result
report. Simulation checks whether the cartridge performs the intended experiment;
it does not establish which hypothesis describes hardware.

Read alongside the [reference audit](asic-reference-audit.md),
[source-divergence ledger](source-divergences.md), [implementation reference](references/asic-reference.md)
and [diagnostic instructions](../../scripts/diagnostics/README.md).

## Screen 26: PA1 — instruction byte, CPU return and Gate Array write

Execute real `IN A/B/D/E/H/L,(C)` instructions, each with freshly loaded
`BC=&7F00` and the border selected immediately before the tested instruction.
Save the returned register before reading the border palette. Acquire all values
with interrupts disabled, then render the results. DMA is disabled.
Before each tested IN, reset the border through the mapped palette to `000F`
(blue), outside every candidate palette vector. A result of `000F` then means
no observed palette write for that row rather than retention of a previous
candidate colour. Apply the same control to the port7F54 repeat. After saving
all observations and OUT controls, restore RAM configuration C0 and ASIC mapping
B8 locally in PA1 so an unexpected decoded write does not affect navigation.

Print the instruction, opcode, CPU byte and the border palette's three-digit
**GRB** word. The opcode's low five bits select the legacy palette entry:

| Instruction | Final opcode byte | Opcode-driven palette | Opcode OR 1 palette |
|---|---:|---:|---:|
| `IN A,(C)` | `78` | `066` | `F66` |
| `IN B,(C)` | `40` | `666` | `666` |
| `IN D,(C)` | `50` | `006` | `F06` |
| `IN E,(C)` | `58` | `066` | `F66` |
| `IN H,(C)` | `60` | `666` | `666` |
| `IN L,(C)` | `68` | `0F6` | `FF6` |

These words follow the documented legacy colour table, represented in
`rtl/plus/asic_regs.v` by `legacy_colour_gbr`. A fixed write byte `78` gives `066`
in every row; fixed `79` gives `F66` throughout. Thus the complete palette-result
vector distinguishes opcode tracking, opcode with bit 0 raised, and both fixed
models. Individual rows are deliberately not sufficient: `40` and `41`, for
example, map to the same colour.

KT's Ports section is available locally in
[`Extra CPC Plus Hardware Information.md`](../specs/plus/Extra%20CPC%20Plus%20Hardware%20Information.md).
Its measured `79`/`78` claim is illustrated using `IN A,(C)`; treating it as a
constant for every destination register is a hypothesis this screen tests.
The CPU-return model and the write-value model must be reported independently.
If the same byte supplies both on hardware, the candidate CPU vectors are the
opcode column, that column OR 1, six `78` bytes, or six `79` bytes respectively.
The source does not establish that coupling for all instructions.

The current full-core prediction is CPU `FF` in every tested row and the
opcode-driven palette column. This follows two separate paths:

1. `Amstrad_motherboard.v` supplies its final-M1-byte latch to the Plus I/O write
   decoders during reads.
2. For these otherwise unclaimed I/O reads, main SDRAM OE/WE are inactive.
   `rtl/sdram.v` exports `dout = (oe | we) ? ram_dout : 8'hFF`; its internally
   retained memory byte is therefore not the CPU's input value.

Controls and implementation requirements:

- Explicit `OUT 78` and `OUT 79` controls must read back `066` and `F66`. Print
  both, rather than asking a photograph to identify a border shade.
- A second `IN A,(C)` with `BC=&7F54` can reproduce KT's example and expose a
  port-low-byte dependency without adding a screen.
- Reload BC for every row, since `IN B` changes it. For B/D/E/H/L, move the result
  into A and store A before any palette read; Z80 has no arbitrary-register
  `LD (nn),r` instruction.
- Save both palette bytes, with the high byte's green nibble explicit. Do not
  confuse GRB storage order with RGB image encoding.
- CPU `FF` alone cannot identify the written colour; palette aliases cannot
  identify every possible written byte. Report the observations before inferring
  a mechanism.

## Shared geometry for PA2 and PA3

Use `R0=63, R1=40, R2=49, R3=&8B, R4=36, R5=16, R6=40, R7=18, R8=0, R9=7`,
with SSCR zero. This gives `37 × 8 + 16 = 312` scanlines:

- Normal lines 0–295, with terminal normal line 295 at C4=36/C9=7.
- Adjustment indexes 0–15 on lines 296–311. C4 remains 36.
- Split comparison code `32 + (adjustment_index & 7)` during adjustment.
- Frame origin after line 311.

Unchanged C4 during adjustment is an explicit precondition, implemented in
`rtl/plus/asic_video.v`'s `charline_n` logic and attributed there to ACCC v1.11 FR
§11.2.6 p.85. Independently checked against the French PDF: C4 is not
incremented when adjustment begins and remains equal to R4; VMA′ is updated
before adjustment at C0=R1. If hardware instead increments C4 at
adjustment entry, the chosen codes miss and PA2 can be all red regardless of
capture eligibility.

R6 exceeds C4 during adjustment, allowing display enable there. R7 places raw
VSYNC at line 144 so monitor blanking should not cover the experiment. Display
enable alone is not proof of monitor visibility. Keep the border distinctly
blue, with phase marks bracketing the observation region, so either a red or
green adjustment strip can be identified. Let the monitor settle before
photographing. Green adjustment additionally proves an immediate split effect;
it is not required for PA3's deferred-capture hypothesis below.

Use a red R12/R13 baseline at MA `3000` and green SSA at MA `0200`. The latter
maps to physical address `0400` at RA0 and avoids the interrupt entry. Fill bank0
from `0100`, preserving `0038` and its handler. Keep the entire sampled address
range flat on every RA plane. In particular, account for the physical mapping
`{MA13..12, RA2..0, MA9..0, byte}`: MA's low ten bits wrap independently of its
bank bits. With the red baseline, the adjustment source following the terminal
row capture is MA `3000 + 37×40 = 35C8`, whose low-plane physical byte offset is
`0390`. A split stored from the preceding frame may instead carry SSA0200 across
the frame origin. Frame line0 then uses the red baseline, but subsequent row
captures advance from0200. After 37 normal rows the terminal capture gives MA
`0200 + 37×40 = 07C8`, mapping to bank0 byte offset `0790`. Fill that span red on
all eight RA planes too. Otherwise PA2 can start adjustment green solely because
of the previous frame.

Reserve these physical spans, with n from0 through7:

| Role | Physical span on plane n | Fill |
|---|---|---|
| Normal baseline at adjustment entry | `C390 + n×0800` through `C3DF + n×0800` | Red, mode2 byte00 |
| Baseline after a carried green frame | `0790 + n×0800` through `07DF + n×0800` | Red, mode2 byte00 |
| SSA0200 experimental line | `0400 + n×0800` through `044F + n×0800` | Green, mode2 byteFF |

The spans do not overlap each other or the interrupt entry. Fill the green bank
first, then overlay its red `0790–07DF` spans. SSA3000 reads physical `C000–C04F`
on each plane, so keep those spans flat red too. Both PA2 and PA3 use these
fills. Confirm the final program's pointer path, including settled frames after
navigation, before placing text.

Draw explanatory labels in both possible banks, outside the measurement spans.
For a frame carried from SSA0200, green-bank labels need the physical `0400`
base offset. Label rows 4–8 relative to that base are a suitable starting point;
do not overwrite the flat measurement strip at `0400`–`044F`.

A PRI255 handler enters a calibrated delay with interrupts disabled. Recalculate
its coarse and fine delays for these screens; V3's line311 constants are not
reusable evidence. Keep completed writes several character periods away from
the tested capture boundaries. Cyan border dashes locate the timed sections,
while diagnostic logs establish precise coordinates. A dash can wrap visually
around HSYNC, so its image position is not itself a raw C0 measurement.

## Screen 27: PA2 — captures inside adjustment

The intended sequence is:

| Time window | Completed action | Purpose |
|---|---|---|
| Late line295, after R1 and before R0 | SSA0200, then SPLT32 | Arm adjustment index0 without an earlier normal-line alias |
| Early line297, complete before its R1 | SSA3000, then SPLT33 | Preserve index0 through line296/R0; give index1 a contrasting target |
| Early line298, complete before its R1 | SPLT0 | Preserve index1 through line297/R0; prevent repeat matches |
| Early line304, complete before its R1 | SSA0200, then SPLT32 | Test the alias at adjustment index8 |
| Early line305, complete before its R1 | SPLT0 | Preserve index8 through line304/R0; end before the next alias |

SSA must be fully written before enabling the next SPLT value. SPLT32 does not
match the terminal normal line's code39; terminal capture therefore cannot
create the first experimental green region. Finish early-window writes with
margin, for example before C0=30. Each update then occurs after the preceding
tested line's R0 and before the next tested line's R1. This supports either R1
capture or an extension of R0 capture to adjustment lines; changing SSA just
after R1 would not. Either capture point changes the next scanline. With the
normalization spans above, the predictions are:

| Operational hypothesis | Visible adjustment lines |
|---|---|
| No capture inside adjustment, current RTL | All red |
| Only adjustment index0 captures | Red296; green297–311 |
| Only adjustment indexes0–7 capture | Red296; green297; red298–311 |
| All indexes eligible, but no SPLT32 alias at raw raster8 | Red296; green297; red298–311 |
| All adjustment indexes capture, with SPLT32 alias at raw raster8 | Red296; green297; red298–304; green305–311 |

The index8 experiment provides decisive positive evidence, but its negative
result is ambiguous: ignoring RC3 is source-derived, and the R9>7 double-match
remains unprobed on hardware. Green305–311 establishes both index8 eligibility
and the alias. Red298–311 cannot distinguish first-eight-only eligibility from
unrestricted eligibility without that alias. With R5=8 even the positive
distinction would be unavailable. The single green line297 also matters: if
capture blur hides it, the image cannot identify the middle readings.

Source-language limits:

- The reading “a terminal-line split first becomes visible on adjustment entry”
  predicts the same PA2 image as blocking all in-adjustment captures. PA3
  establishes that terminal capture works; it does not distinguish these two
  descriptions of the same behavior.
- “First character line” is ambiguous. Interpreting it as indexes0–7 yields the
  third row. Interpreting it as the unchanged C4 character row throughout
  adjustment permits all indexes but still depends on the comparison rule.
  Report operational capture eligibility; do not claim the photograph uniquely
  interprets the sentence.
- A normal R9>7 alias-control band would require extra geometry changes and
  total-line compensation. Even a positive result there would not prove that
  adjustment uses the identical comparator. Keep the stated equivalence in this
  version rather than expanding it into that separate experiment.
- The experiment directly displays adjustment behavior. The normalization spans
  cover baseline reload and SSA0200 carry from the previous frame under the
  stated pointer models; another pointer mechanism requires a fresh trace.

## Screen 28: PA3 — terminal split across adjustment and frame restart

Use the same geometry and fills as PA2. Set SSA0200 and enable SPLT39 late on
line295, before R0. Code39 matches `{36[4:0],7}` on the terminal normal line.
Disable SPLT early in adjustment, well before adjustment index7 could match39
again. This isolates terminal capture from PA2's disputed adjustment captures.

| Operational hypothesis | Adjustment296–311 | Frame line0 | Frame lines1–7 |
|---|---|---|---|
| Frame restart reloads stored MA, current RTL | Green | Red | Red |
| Terminal SSA survives adjustment and frame restart | Green | Red | Green |
| Terminal SSA deferred until frame line1, the next opportunity | Red | Red | Green |
| Capture missed, or deferred capture discarded at restart | Red | Red | Red |

The independently derived current path is: terminal R0 capture loads the stored
pointer; adjustment repeats it; adjustment end reloads both pointers from
R12/R13. The alternative keeps the stored SSA while reloading the active pointer
for frame line0, exposing SSA again on line1. The deferred reading retains the
baseline during adjustment and first applies SSA at frame line1. The red
normalization spans make its adjustment baseline red even after a green frame.
The table judges the first seven lines after the origin, before the next row
capture. Later green-bank lines can contain the deliberate red normalization
patches and labels; they are not required to remain uniformly green.

Visibility is established by the strip's contrast with the blue border and its
phase marks. Green adjustment additionally proves an immediate terminal-capture
effect. Red adjustment followed by green frame line1 is a predicted deferred
result, not an invalid control. All red remains ambiguous between a missed
capture and deferred capture discarded at restart. V3 terminal-split screens
can cross-check setup but do not distinguish those mechanisms here. SSCR remains
zero so line0 cannot cancel a held split through the separately established
vscroll interaction. Green frame line0 or another pattern is an unexpected
observation, not a reason to weaken these predictions.

## Screen 29: PA4 — unmapped-page read source

With DI and both ROMs disabled, temporarily unmap the ASIC page and seed
underlying RAM `5000=A5` and `6800=5A`. Read back and save those setup values.
Then explicitly map the ASIC page with RMR2=&B8 and perform the four reads below.
Save A immediately after each; render only after acquisition.

| Read | Last memory-read byte | Last M1 opcode only | Inactive/FF bus | Underlying RAM |
|---|---:|---:|---:|---:|
| `LD A,(&5000)` | `50` | `3A` | `FF` | `A5` |
| `LD A,(&6800)` | `68` | `3A` | `FF` | `5A` |
| `LD HL,&5000; LD A,(HL)` | `7E` | `7E` | `FF` | `A5` |
| `LD HL,&6800; LD A,(HL)` | `7E` | `7E` | `FF` | `5A` |

For an absolute read, the last byte fetched before the target access is the
address operand's high byte; for `(HL)`, it is opcode7E. The pair of absolute
reads therefore separates operand retention from an incorrect M1-only latch.

Write/read a known sprite-RAM nibble and a known palette byte while mapped, and
print those controls. This establishes that the page is active and that
supported readback works. Print the pre-mapping A5/5A controls as well. Do not
infer mapping solely from four FF results, or confuse a failed page mapping
with operand retention.

The current full-core prediction is **FF FF FF FF**. Unmapped/write-only reads
leave `plus_asic_rd` inactive; ASIC-page selection disables SDRAM OE; `sdram.v`
exports FF with its port inactive; the ordinary CPU input mux consequently
receives FF. The internal retained SDRAM byte does not escape that output gate.
The audit's prediction is consequently defensible, but the ASIC module's neutral
FF contribution alone is not a sufficient derivation of the CPU value.

An actively driven FF device and a bus whose inactive contributors all export
FF are observationally equivalent here. Likewise, reproducing the source vector
does not prove a particular internal latch implementation. Record returned bytes.

## Screen 30: PA7 — relative compatible-interrupt phase

This screen measures whether PRI0 adds a delay relative to raw HSYNC. It does
**not** by itself settle the broader statement that Plus interrupts are later
than CPC interrupts.

Show a programmed-PRI reference marker and a PRI0 compatible marker in separate
vertical bands on the same screen. Use `R0=63, R1=40, R2=49, R3=&88` (HSYNC
width8 and VSYNC width8), `R4=38, R5=0, R6=25, R7=30, R8=0, R9=7`, SSCR0.
Both windows use the same byte-identical palette-marker ISR and uninterrupted-NOP acceptance
environment. Use no handler overlay or extra JP for just one of the two paths.
DMA and all other pending interrupt sources must be absent.

A practical sequence is a PRI7 reference window, then an explicit compatible
counter reset and a short NOP window spanning its expected 52nd HSYNC. A coarse
delay before that final window avoids allocating several kilobytes of NOPs.
With interrupts disabled, write PRI0 **before** the MRER bit4 reset so a
compatible request retained while PRI was nonzero is cleared before opening the
acceptance window. Complete both operations well before the event. Keep the reset
and the measured event away from VSYNC resynchronization. Prefer the acceptance
sled at A13=1 to avoid the separate acknowledge-shaping anomaly. Record both
marker counts; an absent or additional request invalidates the intended pair.

Let P be the reference marker's right edge in mode2 dots. These predictions are
for NOP acceptance slots, not direct measurements of request time. Conditional
on equal CPU acceptance phase and equal ISR/write timing:

| Operational hypothesis | Compatible marker right edge |
|---|---|
| Request at raw HSYNC falling edge | P +112 dots |
| Extra exactly1µs delay after raw HSYNC fall | P +128 dots |

Edge-level derivation: the hardware-supported programmed PRI slot is raw HSYNC rise+1µs.
Width8 falls at rise+8µs, a difference of7µs or112 dots. An additional1µs makes
128 dots. Identical instruction acceptance and palette-write delays cancel,
provided the requests enter the assumed slots. Establish the production model's
actual request-to-ACK slot in simulation; edge subtraction alone does not prove
the CPU sampling boundary. A submicrosecond change across that boundary can also
shift the marker by16 dots.
Compare marker right edges because blanking may hide their starts. Print a
horizontal ruler or reference guides that do not overwrite the marker area.
The implemented layout places pen1 ticks every8 dots on lines10 and71, extending
every second tick onto lines11 and72 to mark16-dot steps. These rulers sit just
below the measured marker lines8 and69; neither changes the ISR or acceptance
window. Label the compatible marker's expected line69 explicitly so an early
request accepted at window entry is visibly outside its intended location.

These predictions must not be presented as “RTL versus Quasar” without the
conditional qualification. The local primary source
[`Furthur details of timing.md`](../specs/plus/Furthur%20details%20of%20timing.md)
attributes the later Plus compatible interrupt to the ASIC HSYNC itself occurring
one character, reported as17 pixels, later than a type0 CRTC. A common HSYNC shift
cancels from the same-Plus comparison. That reading can give112 dots while still
making the interrupt later than a CPC. It is observationally equivalent on this
screen to the current raw-HSYNC-relative timing model.

Further limits:

- Submicrosecond request differences accepted in the same NOP slot are
  indistinguishable. A source's approximate1µs is not a guaranteed exact16-dot
  displacement. Record the software-visible slot and image resolution.
- A matched classic CPC artifact would need different initialization and a
  classic-accessible marker path: the ASIC-page palette write cannot be used
  unchanged. Comparing that with this Plus screen would introduce marker-path
  and sync-to-video alignment assumptions. It is not a proportionate substitute
  for the present five-screen experiment.
- To settle the general cross-machine claim, use a separately reviewed matched
  CPC/Plus probe with identical legacy-port writes and controlled CPU acceptance,
  or external traces of named HSYNC, INT and CPU signals. Neither is silently
  included in this scope.

If the requirement is that every written-source interpretation must produce a
different image, PA7 cannot meet it under the five-screen, Plus-only scope. The
explicit deliverable is the narrower raw-HSYNC-relative discriminator, retaining
the common-HSYNC-shift interpretation as unresolved.

## Diagnostic evidence and acceptance

Extend only the diagnostic fixture and logger, not production RTL. Required taps
and records are:

- Actual motherboard `mb.INT_n`; existing `dbg_int_n=~force_irq` is synthetic.
- Raw HSYNC rise and fall, actual interrupt acknowledge, and a monotonic master
  clock timestamp. `g_fire` alone sees programmed PRI, not the compatible path.
- PA2/PA3: adjustment flag and index, split capture, active and stored MA,
  SSA/SPLT writes, and the border-marker writes with line/C0 coordinates.
- PA1/PA4: returned values at the CPU's sampling point, not merely a module's
  combinational data output.
- Explicit frame identity. The existing logger collects between VSYNC edges
  while images use CRTC-origin coordinates; PA3 evidence must not combine
  unmatched frame states when establishing adjustment-to-origin behavior.

Before judging hardware, establish in simulation that each instruction executes,
each write is inside its intended timing window, the controls are visible, and
the sampled display addresses contain the intended flat colours. Inspect images
as well as logs. These are cartridge-validation checks; unexpected production
model behavior does not authorize changing the predeclared hardware hypotheses.

Keep `assert $ < &BF00`, normal navigation, cold-start selection and V3 behavior.
Review the design independently before any cartridge build; review the resulting
nontrivial diagnostic implementation independently before declaring it ready.
Record hardware results as observations with model and capture path identified.


## Production-model observations and hardware handoff (2026-09-28)

These observations use the unchanged production RTL at branch base `77ebf51`
and the production T80/SDRAM diagnostic fixture. They validate this experiment,
not the disputed hardware rules. Final source builds a 14712-byte program, below
BF00; the shared PA7 NOP window is at B655 and has A13=1 throughout, checked by
assembler assertions. No production RTL was changed.

The final hardware artifacts are generated, ignored files:

- [Title-first V4 CPR](../../output_files/plus-hw-probes/v4/plus-hw-probes.cpr),
  SHA256 `635bff41862f8ca70eac47e06f15bf5b43b6fa051816b22b8f5b86ff92186697`.
- [Start-at-26 V4 CPR](../../output_files/plus-hw-probes/v4/plus-hw-probes-start26.cpr),
  SHA256 `aa30d77f68120b46749a031d87f8bba9a339b3187c1807b793f57fb09022374c`.

Rebuild them with:

```sh
python3 scripts/diagnostics/plus_hw_probes.py --output-dir output_files/plus-hw-probes/v4
python3 scripts/diagnostics/plus_hw_probes.py --output-dir output_files/plus-hw-probes/v4 --start 26
```

The initial five-screen run built the production model. After the independent
implementation review, the bounded PA1 sentinel/mapping and PA7 ruler changes
were checked with these commands; the unchanged model binary was reused:

```sh
python3 scripts/diagnostics/plus_hw_probes_sim.py --no-build --screens 26 27 28 30 --frames 40 --out output_files/plus-hw-probes/v4/sim
python3 scripts/diagnostics/plus_hw_probes_sim.py --no-build --screens 29 --frames 40 --out output_files/plus-hw-probes/v4/sim
```

Every run reports exit0 after40 VSYNC intervals. The retained image/event pair
uses complete CRTC-origin frame38, rather than joining pieces at VSYNC. The
initial model-build command is the same without `--no-build`, selecting all five
screens. The diagnostic wrapper now propagates any failed screen's exit status.

| Screen | Observed production-model result |
|---|---|
| 26 PA1 | Seven CPU reads FF, including the port7F54 repeat. GRB values066/666/006/066/666/0F6, then066 for the repeat. Explicit OUT controls066/F66. Each tested IN has its own00F sentinel. |
| 27 PA2 | All adjustment lines296–311 red; no adjustment split event. Stored MA35C8 supplies the expected red physical spans. |
| 28 PA3 | Split capture at295:C63 with SSA0200; lines296–311 green. Frame0–7 red, with both pointers reloaded to3000 at origin. |
| 29 PA4 | FF FF FF FF. Underlying-RAM controls A5/5A, mapped sprite/palette controls0B/5A. |
| 30 PA7 | Yellow marker right edges x68 andx180, difference112 mode2 dots. IRQ/FRAME02. Actual INT-to-ACK delay95 master ticks in both measured windows. |

Final PA2/PA3 timed writes, all at dot0 unless noted:

| Screen | Action | Line:C0 |
|---|---|---|
| 27 | SPLT32 | 295:50 |
| 27 | SSA3000 high/low; SPLT33 | 297:6 / 11 / 17 |
| 27 | SPLT0 | 298:16 |
| 27 | SSA0200 high/low; SPLT32 | 304:4 / 9 / 15 |
| 27 | SPLT0 | 305:14 |
| 28 | SPLT39 | 295:48 |
| 28 | Terminal capture | 295:63, dot15 at the sampling edge |
| 28 | SPLT0 | 296:10 |

The calibrated constants are ADJ_COARSE350, ADJ_FINE66 and ADJ_NEXT57. The later
SSA0200 restore happens only with SPLT disabled. Top cyan border dashes occupy
48 pixels (PA2) or16 pixels (PA3) at raw x961 onward on line295, after HSYNC;
the monitor wraps this vicinity toward its left border. Later border writes
inside active display are recorded in the trace but do not create visible
border marks. The adjustment strip itself remains distinct from the blue border.

PA7 resets through `OUT 7F00,9E` at line17:C13, after selecting PRI0. The measured
compatible request is at68:C57; the complete reset-to-event interval precedes
VSYNC240 and its resynchronization. The reference is at7:C50. Marker write/clear
coordinates are7:C58/8:C4 and69:C1/69:C11, each at dot4. PRI2 is restored at
74:C20, suppressing later compatible requests. Rulers have80 short ticks every8
pixels on lines10/71 and40 long ticks every16 pixels on lines11/72; they leave
marker lines8/69 untouched. The change did not move either marker edge.

PNG previews are in `output_files/plus-hw-probes/v4/sim/26.png` through `30.png`.
The additional27/28 `-origin-preview.png` files rotate the raster ordering at
line200, without changing pixel values, to show adjustment and the next origin
together. They are navigation aids, not monitor-timing simulations. The PPM,
`-events.txt`, `-acquisition.txt`, `-results.txt` and `-navigation.txt` files retain
the underlying evidence.

V3 preservation was checked by assembling all25 original screens from `77ebf51`
and all25 V4 counterparts, then running both through the same production model
for40 VSYNC intervals. All normalized complete-frame event traces matched.
Pixel differences were confined to the V3→V4 version and25→30 screen-count glyphs,
including their expected repetitions in split/scroll screens. The final bounded
PA1/PA7 edits leave all V3 handlers, sleds and geometry untouched; their addresses
remain unchanged. The detailed run and comparison reports are retained in
`output_files/plus-hw-probes/v4/preservation-runs.txt` and
`preservation-comparison.txt`.

A key-navigation run passed26→27→28→29→30→title, and a separate title-first run
passed title→1. These checks exercise release-before-press behavior and wrap at
30. The final PA1 restoration and PA7 ruler delta additionally passed focused
26→27 (80 VSYNC intervals) and30→title (65 intervals) navigation runs on the
final CPRs. Their `final-nav26-navigation.txt` and `final-nav30-navigation.txt`
records identify each transition. These final checks used:

```sh
sim/plus/obj_dir/plus_hw_probes_sim/plus_hw_probes_sim output_files/plus-hw-probes/v4/plus-hw-probes-start26.cpr output_files/plus-hw-probes/v4/final-nav26 80 40
sim/plus/obj_dir/plus_hw_probes_sim/plus_hw_probes_sim output_files/plus-hw-probes/v4/sim/plus-hw-probes-start30.cpr output_files/plus-hw-probes/v4/final-nav30 65 40
```

Original hardware remains
the next acceptance step; no result here closes the observational equivalences
listed above or authorizes a production RTL fix.
