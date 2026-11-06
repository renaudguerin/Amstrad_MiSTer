# ASIC R3 positioning: rendered-RGB correction

Source `fdf348c` removes Before the Storm's sampled reversing wobble on
CRTC3. The production fixture fails before the correction and passes after it;
full synthesis, static native LEFT8, the original BTF run and scoped
BI/AE/AR/DSC4/Amazing preservation now pass. The branch remains experimental.
DeepSeek independently permits the trial; Opus assurance and the documented
promotion coverage remain open. No full-demo or physical HDMI/CRT claim follows.

## Native defect and independent reference

On prior source `c0939d8`, eight CRTC3 bursts provide 192 complete frames
and 176 eligible consecutive pairs. The fixed middle-band registration gives
88 holds, 43 left 10 and 45 right 6 steps. Root views every frame. The same
qualified reader and geometry guards show no reversal on sampled CRTC0,
while CRTC1 already has before/after acceptance. The
[BTF report](before-the-storm-r3-2026-11-05.md) owns input identities,
capture-storage recovery and physical-display limits.

French ACCC v1.11 **§14.4 pp. 134–135** recommends the ordinary 4/5 pair for
an exact 1 µs C-HSYNC difference across CRTC types: increasing 4→5 shifts the
rendered line left by eight Mode2 pixels. Root reads the primary native
Markdown and fresh rendered pages. The duration table itself lists only
types 0/1/2; no ASIC numeric table is inferred from it.

Three independent normal-boot AmSpirit 1.17.1 / core 2491682 Plus cartridges
corroborate the relative shift. They use the same original calibration
guest with only the R3 immediate changed to 4/5/6. CPU initialization,
palette/register/idle checks and the entire 16 KiB VRAM pattern qualify.
Monitor Off uncropped images are 1024×700, with doubled vertical rows.

| R3 | Paper rectangle, exclusive right/bottom |
|---:|---|
|4|(236,190,876,590)|
|5|(228,190,868,590)|
|6|(218,190,858,590)|

Both paper edges therefore move left 8 for 4→5. All 256,000 paper RGB pixels
match after translation; 40 differences outside the paper remain in the
common full-canvas comparison. This is software-reference evidence,
not a physical ASIC measurement. The observed 5→6 left 10 is not promoted
to a new requirement; the candidate retains the existing coarse 5/6 output.

## Old-build native static controls

On prior build `c0939d8`, all three original calibration cartridges now complete
normal boots through the hardware loop, with two stable native captures per
width. The parent views all six. R3=4 and R3=5 images are byte-identical
(SHA256 `780c7946aaacb599d64e25db5d772c472df310c8132c81b6e12ce6432dc390a6`).
Independent decoding of the original 48-dot marker places both at x409 on
rows40/100/239; width 6 places it at x401. All three non-border rectangles
are `(65,40,705,240)` in the 768×273 captures. These are old-build output
observations, not candidate acceptance or an original-ASIC measurement.
Private `btf-asic-r3-hardware-plan/PARENT-OLD-STATIC-ACCEPTANCE.json` binds
the CPR/RBF/CFG identities and all six captures. The new-build trial retains
these inputs and checks the final captured image, including its borders.

## Candidate build and static native acceptance — 6 November

[Full run 42131293902](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42131293902)
passes every required job on `fdf348cd04475589c48c5b9fd41a4d160039ad20`.
Artifact 13464902429, `Amstrad-build-401-1-full`, contains
`Amstrad_20261106_fdf348c.rbf`, 3,138,276 bytes, SHA256
`17466675860ee115418e712c55250a866c1b2f45bc9a796fd7f314a86d1cbf0d`.
Clean-full Quartus 17.0.2 Build 602 closes setup/hold at 0.507/0.201 ns across
seven clocks, TNS zero. Warning categories/counts match c0939d8; no critical,
implicit-net or latch warning is added. The device file hash matches.

The same three normal-boot cartridges now complete two captures each on the
candidate. Root views all six. Width 4 paper moves to `(73,40,713,240)` and
its unique marker to x417; width 5 remains `(65,40,705,240)`/x409. The entire
760×273 common canvas matches after the expected eight-pixel translation:
207,480 pixels, zero RGB mismatches. Both paper edges move; no border clipping
is introduced. Width 5/6 remain byte-identical to their old-build images.

`btf-asic-r3-build/ACCEPTANCE.json` binds CI, timing and device provenance;
`btf-asic-r3-hardware-plan/PARENT-CANDIDATE-STATIC-ACCEPTANCE.json` binds
all six final images. All eight old-c093 BI/R3-JIT captures are additionally
byte-identical to the accepted 275af8a images, establishing the current
preservation baseline. The candidate BI run is byte-identical to all eight.
The original BTF trial completes eight bursts / 192 frames: 180 geometry-qualified
frames, 168 eligible pairs, 85 holds / 83 left-two steps and no reversal. Eleven
attempts rejected by unchanged copy/poll timing guards remain explicit in the
[BTF acceptance](before-the-storm-r3-2026-11-05.md).

## Targeted preservation and AR palette phase

Four candidate runs complete 75 commands/20 captures with no failure. Root
views every image. These checks preserve the previously qualified scope;
they do not refresh the whole-page photo tally.

| Target | Commands / captures | Result |
|---|---:|---|
| BI / R3-JIT |25 /8| All eight images are byte-identical to current c0939d8, which also matches 275af8a. |
| DSC4 |16 /3| Credits, including all four colons/names, and the first otter image are exact 275af8a matches. The later rotating/fire-band image remains visible at a different animation phase. |
| Amazing |13 /2| Logo and lower text/effects remain visible in both sampled frames; no phase-matched pixel claim. |
| AE / AR |21 /7| Every candidate frame has an exact current c093 native phase counterpart, including the guest's intentional colour cycle. |

AR initially differs from older captures in 640/320 pixels, all the same
red/blue pair. A focused read-only Sol review locates the original INK10
cycle at guest 901E–903D: the program counts 50 loop iterations, then toggles
its colour literal 5F/5C with XOR3. All three AR variants use that loop. Its
exact bytes are present in the pinned input disk at offset 0x1209E; root
checks the listing and ASIC legacy-palette entries.

The immediate-parent c093 control already reproduces the candidate's 640-pixel
variant. An original walk stopped before the final SPACE then retains BC.
Six live old-build screenshots, with no reset, input or register edits, show
both 320-pixel variants: four are byte-identical to the candidate BC/B7 image,
two to the earlier old image. Thus the apparent residual is a guest colour
phase, with exact old-source counterparts, not a newly observed regression.
The initial live menu captures are excluded, not treated as AR evidence.

`btf-asic-r3-hardware-plan/PARENT-AE-AR-ACCEPTANCE.json` binds all seven
counterparts; `AR-INK10-SOURCE-JUDGMENT.md` and the held-page captures retain
the source and native discriminator. BI and title comparison receipts are in
the same private root. Physical geometry, broad Plus compliance and the
promotion limits below remain open.

## Failing-first production proof

The retained `make -C sim asic-r3-position-test` uses production
motherboard/T80/READY, ASIC video and actual VRAM reads. Its asset-free
guest executes from base RAM because this simulation composition has no
top-level cartridge overlay. Each width has an actual CPU R3 write and
three natural frame origins before capture. No host beam/counter release,
instruction-delay fit or hidden-state restoration is used.

Before RTL changes, the public test reports 671 mismatches among 1,104
French LEFT8 bitmap comparisons. All four visible border/bitmap entry and
exit edges move 0 rather than 32 master clocks. Independently decoded
six-byte marker data, full 12-bit colours, production READY and Full 256
acquisition prevent an empty or wrong-source pass. The two initial fixture
blockers—RAM placement and a backend write during external reset—are
preserved separately from the first qualified RED run.

After the fix, all 1,104 comparisons and all four 32-clock edge differences
pass, both initially and after a real filter-mode round trip. Raw-pixels
and Raw-CRT each pass 8,192 direct 12-bit source/output checks. Every
non-public source/input/acquisition field remains identical before/after
over 8,192 clocks for widths 4/5/6 and Full return. Public width 5/6 captures
remain exact; only the intended Full 4 final RGB changes. The source hashes
and guest/marker inputs are unchanged.

## Change and gates

The motherboard's existing eight-dot history now holds the selected
complete 12-bit rendered pixel. Classic DAC pairs retain their original
low channel bits; ASIC samples come after palette/sprite composition.
An owner change bypasses the old history immediately, clears its clocked
state and requires a fresh complete pulse before correction. Existing
reset, SNA, filter-mode and pulse-ownership cancellation remain in place.
The filter, VRAM, raw HSYNC and CRTC/ASIC counter rules are unchanged.

- Public focused test: RED→GREEN with unchanged guest and assertions.
- Existing classic R3 fixture: PASS 17 callers, including the R2/R3 controls.
- Final `python3 sim/select_tests.py --run`: PASS 46 fast benches. The new
  motherboard fixture is slow and is explicitly owned by production-T80 CI.
- DeepSeek V4.1 Flash free max, run `20261105T232421Z-32325-af93`: GO for
  experimental synthesis; no P0/P1 RTL finding. The parent read the report,
  verified the counter limitation below and confirmed all six source hashes
  still match the gate. The provider exited normally; no edits or tests ran.
- The direct focused ASIC run is the slow-gate evidence; the 46-bench selector
  run covers the affected fast benches. CI additionally runs the slow fixture.
- Full synthesis/required CI: PASS on exact source `fdf348c`, including the
  slow production-T80 fixture. Static native controls, BTF and scoped
  title/SHAKER candidate acceptance pass within the limits below.

The review identifies three limits to retain before promotion:

- Return-pass `complete` and `readyLow` checks use cumulative counters. Three
  fresh natural origins, the live acquisition fields, nonempty pixel checks
  and four visible-edge assertions still qualify that pass. Use counter deltas
  to strengthen those two guards before promotion; this is not an observed
  false pass or an RTL blocker for the experimental trial.
- The fixture uses sprites disabled. The history structurally follows the
  final palette/sprite compositor, but narrow-sync output with enabled sprites
  has no end-to-end proof.
- Live classic/Plus switching is supported by bypass/clear inspection only;
  neither static-owner fixture toggles the owner at runtime.

These limits and the pending Opus review are tracked in [review debt](../../review-debt.md).
No code changed after the reviewed gate.

The focused ASIC fixture uses Mode 2 with sprites disabled. It checks the
whole rendered line and full 12-bit source identity, rather than a proposed
silicon register stage. General monitor settling, every JIT waveform,
enabled-sprite narrow-sync combinations and full-demo acceptance remain
outside this result. Raw-mode and old 5/6 preservation are explicit bounds.

Private evidence is under
`local/overnight-20261103/resumed-20261104/btf-asic-r3-proof/`:
`run4/` preserves the first qualified failure; `candidate/` holds the public
RED/GREEN logs, classic preservation, final gate, source hashes and
`before-after-invariance.json`. The independent reference is in sibling
`btf-asic-r3-amspirit/`, including `PARENT-ACCEPTANCE.json` and all three
normal-boot cases. Review inputs/results belong in sibling
`btf-asic-r3-review/`. Media and captures remain ignored.

Technical information sourced from the Amstrad CPC CRTC Compendium by
Longshot (CC BY-NC-ND).
