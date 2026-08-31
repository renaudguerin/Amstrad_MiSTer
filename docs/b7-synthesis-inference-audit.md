# B7 audit 1: synthesis inference sweep

Read-only analysis, 2026-08-31. Backlog item B7, audit 1. No RTL was changed.

Evidence: Quartus 17.0.2 reports from GitHub Actions run `33439105361` (`Build core`, full
effort) at integration SHA `e5dc56a`, artifact `Amstrad-build-178-1-full`, producing
`Amstrad_20260831_e5dc56a.rbf`.

## Why this audit exists

P10j found that the Plus sprite pixel array had been inferred as thousands of flip-flops
instead of Cyclone V M10K block RAM. It surfaced only because ALM utilization approached 90%
and the user asked whether that was normal. Review had passed the module repeatedly, because
reviewers were asked whether the code was correct rather than whether it was the right shape.
This audit asks the fitter directly instead of asking a reviewer.

## Build health

| Metric | Value |
|---|---|
| Logic utilization | 22,077 / 41,910 ALMs (53%) |
| Total registers | 26,040 |
| Block memory bits | 701,596 / 5,662,720 (12%) |
| RAM blocks | 102 / 553 (18%) |
| DSP blocks | 35 / 112 (31%) |
| Worst setup slack | +0.417 ns (HDMI PLL domain), TNS 0.000 |

Fitter status successful, timing clean.

## Memory inference: no second sprite-RAM-class defect

Every structure that should be block RAM is block RAM. Analysis and Synthesis inferred 23
megafunctions, including both `plus_sprite_ram` banks (`even_bank` and `odd_bank`, each an
explicit `altsyncram` with `ram_block_type = "M10K"`), the u765's `sector_ram`, `tinfo_ram`,
`sector_offset` and `image_track_offsets`, the Multiface 2 RAM, the hq2x line buffers, and the
gamma curve.

Quartus also reports **28 instances of uninferred RAM logic**. Every one of them is correct
behaviour, not a defect:

- **17 instances at `rtl/plus/asic_regs.v:232`** (`Ram0` through `Ram16`) are the
  `legacy_colour_gbr` constant lookup, a 32-entry by 12-bit table. Quartus rejects it as
  "inappropriate RAM size". Correct: 384 bits belong in logic, and committing an entire M10K
  to them would be the actual mistake.
- **10 instances in `rtl/u765/u765.sv`** are per-drive state arrays of two entries
  (`image_tracks`, `i_steptimer`, `i_step_state`, `sector_c/h/r/n/st1/st2`), likewise far too
  small for block RAM.
- **2 instances** (`fdc.sector_offset`, `tinfo_ram|ram`) are reported as uninferred due to
  asynchronous read logic, and both have a separately inferred `altsyncram` alongside.

Quartus logs these at Info severity precisely because they are expected. **Conclusion: the
sprite-RAM incident was a single defect, not the visible instance of a pattern.**

## Removed registers

874 registers were removed during synthesis across the whole design, 116 of them inside
`Amstrad_motherboard`. The overwhelming majority are ordinary optimizer results — "Merged
with" (duplicate registers combined, for example the eleven `asic_sprites` `FQ_ADDR` bits and
the `asic_video` HSYNC/VSYNC output registers) and "Lost fanout" (the `asic_dma` state
machine's unreachable one-hot encodings). Neither indicates a design problem.

Two entries are removed as **"Stuck at GND due to stuck port data_in"**, which is the
signature of a register nothing ever drives. Both were traced to source:

- **`asic_video R16_pen_h[0..5]` and `R17_pen_l[0..7]`** (`rtl/plus/asic_video.v:201-202`) are
  declared, cleared at reset, and returned on the register read path
  (`rtl/plus/asic_video.v:846-847`), but **written nowhere**. Reading CRTC3 R16/R17 therefore
  always returns zero. These are the 6845 light-pen address registers.

  **This is an unowned gap, not a known one.** Finding F18 covers the *classic* CRTC's
  readable register set in `rtl/CRTC.v`, and it was validated and pinned on 2026-08-26 for all
  32 addresses on both types. It says nothing about the Plus path. Nothing in the CRTC3/ASIC
  implementation latches a light-pen position, and no finding records that as a decision.

  Practical impact is probably nil, since light-pen use on a CPC+ is vanishingly rare, but the
  gap should be made explicit rather than left as a stuck-at-GND artifact discovered by
  synthesis. Open it as a small finding, or state in `asic_video.v` that the light-pen
  registers are deliberately read-as-zero on this implementation.
- **`asic_regs ack_src[0]`** (`rtl/plus/asic_regs.v:528`) is a three-bit field whose every
  assignment (`3'd0`, `3'b110`, `3'b000`, `3'b010`, `3'b100`) leaves bit 0 clear, so the
  optimizer is right to drop it. Not a defect. Worth a comment in the source noting the LSB is
  structurally always zero, since the declaration invites the reader to expect otherwise.

## What this audit cannot tell you

Synthesis reports show what was *built*, not what is *reached at runtime*. A module can be
fully synthesized, timing-clean, and still be muxed away in the mode you care about — which is
the failure mode the project has actually experienced. That question belongs to B7 audit 2, the
mutation/dark-silicon harness.

## Verdict

No action required from this audit. The one follow-up is to confirm the R16/R17 light-pen
stuck-at-GND finding against F18 when that finding is taken up, rather than assuming it.
