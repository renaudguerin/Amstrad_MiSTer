# ASIC R3 positioning: rendered-RGB candidate

The current MiSTer build still reverses Before the Storm's scroller on
CRTC3. A focused test identifies the missing ordinary R3=4/5 output shift
and now passes after extending the rendered-RGB correction to ASIC output.
The candidate has completed its simulation gates. DeepSeek cross-provider review gives GO for an experimental full build. Both
Opus routes were unavailable (AGY resource exhaustion; Claude Pro expired OAuth).
Synthesis and native acceptance remain pending.

## Native defect and independent reference

On installed source `c0939d8`, eight CRTC3 bursts provide 192 complete frames
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
- Full synthesis and targeted native candidate acceptance: pending.

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
