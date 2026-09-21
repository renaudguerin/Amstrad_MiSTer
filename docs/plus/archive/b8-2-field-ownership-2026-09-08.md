# B8-2 Plus FIELD ownership

Date: 2026-09-08. Task branch `codex/plus/b8-field-palette`. This record covers the Plus FIELD repair; palette, SDRAM and snapshot
changes are separate work.

## Defect (B8 review §B8-2)

`rtl/Amstrad_motherboard.v` wired classic `CRTC.FIELD` straight to the
motherboard `field` pin, and `Amstrad.sv` forwards it to `VGA_F1` in Plus mode
too. That pin drives the local scandoubler gate (sampled at VSYNC rise;
enabled while the last three samples are 0) and ASCAL field handling
(`sys/ascal.vhd:1217-1258`). The Plus `asic_video` (CRTC type 3) owned a frame
parity but exported no field, so Plus interlace never reached either scaler
consumer while classic state could leak onto the pin.

## Fix

- `rtl/plus/asic_video.v`: new `FIELD = ~parity_frame & R8_interlace[0]`,
  combinational from the frame-origin flop (stable across the mid-frame
  C4=R7 VSYNC it qualifies). FIELD is never inverted by R7 (below).
- `rtl/Amstrad_motherboard.v`: `field = plus_mode ? plus_field : crtc_field`.
  Classic path bit-identical when `plus_mode=0`. `Amstrad.sv` needs no
  behavioural change beyond the extraction below.
- `rtl/video_interlace.v` (new, in `files.qip`): behavioral extraction of
  the `Amstrad.sv` history+enable decision (shift VGA_F1 in on VSYNC rise;
  `scandoubler` while the 3-bit history is all zero), now shared by
  production and the fixture; an explicit initial block pins deterministic
  zero startup in simulation. The B8-2 test executes this real consumer on
  the motherboard's SELECTED filtered VSYNC + FIELD.

## Rule anchors (French canonical; English reflows by one page)

Tracked extraction: `docs/accuracy/extract/inspector-v1.11-fr/ACCC1.11-FR.md`.

- FR §19.5.5 pp.214-216 (EN pp.213-215): ParityFrame toggles every frame at
  C4=C9=C0=0 whatever R8; even schedules the additional line + MID. Frame
  start loads ParityC9=ParityFrame; R8→1/3 seeds ParityC9=C9.0, so an
  odd-C9 entry really mismatches C9 vs frame parity on the first frame
  (p.214) and settles the next. Even R9 aligns C9 parity to frame parity
  once settled (test uses R9=6; odd-R9 balancing stays with leaf t04j).
- FR §19.6.4 p.218 (EN p.217): either R8=1 or R8=3 adds exactly one C9=0
  line after the R5 block on the even frame; C4 not incremented.
- FR §19.7.3 p.219 (EN p.218): MID (C0=R0/2) on the even frame for either
  mode, seam (C0=0) on the odd frame — EXCEPT R7=0, which samples the
  OUTGOING parity: MID then carries NEW-odd FIELD0, seam NEW-even FIELD1.

## Polarity (source-derived convention, not hardware clearance)

`FIELD=~parity_frame & R8[0]` is kept because `sys/ascal.vhd:1233-1252`
latches field at DE rise and, at the VSYNC+DE frame start, writes the base
buffer when FIELD=1 (first woven line) but offsets one line when FIELD=0;
settled even-R9 IVM aligns C9 parity to frame parity, so `~parity` puts even
lines first. The classic `FIELD=~field` resemblance is a separate legacy
flop, not evidence. Transition C9/frame mismatch is real (FR p.214): FIELD
follows frame parity, never raster parity. No claim that MID is universally
even-parity (R7=0 breaks it) or that FIELD equals C9 parity.

## Regression (`sim/plus/b8_field_test.cpp`, in the default gate)

New bench top (`sim/plus/b8_field_bench_top.v`, sync_filter=1 normal path)
plus a bounded C++-driven CPU source (`sim/plus/b8_field_cpu.v`): every Plus
R8/R7 program is a real I/O OUT pair through the production motherboard
decode (both engines see it, like hardware). The ONLY hierarchical poke is
the INACTIVE classic R8 in the negative control. Plus R8 is never poked;
parity_c9 is never forced (entry seeding happens in-RTL from the bus
write). Nothing reads internal parity to build an expectation; the raw ASIC
VSYNC phase (MID hcc=31 / seam hcc=0 at R0=63) is only the oracle, while the
production consumer decides on filtered rises.

Frame R0=63 R1=40 R4=67 R5=0 R6=100 R9=6 gives 476
ordinary / 272 IVM lines per field clear the crt_filter VSYNC debounce
(vSyncFlt>260), and R0=63 keeps the filter HSYNC mask clearing every line
(shorter lines halve its line rate and accepts go sporadic).

- s1/s2 ordinary R7=5, R8=3 / R8=1: MID+long FIELD1, seam+short FIELD0,
  +4096 clks (the +1 line is origin/event-matched to the MID-phase field,
  not just differenced), consumer disables. s2 proves R8=1 is interlace.
- z1/z2 R7=0, R8=3 / R8=1: MID carries FIELD0, seam FIELD1 (exception
  distinguished; FIELD not inverted).
- t1 live entry: R8=3 at ODD raster mid-frame; a check-then-tick hold watch
  proves FIELD stable over a printed nonzero interior-tick count up to (not
  including) the origin edge, then the settled pairing.
- t2 live exit: R8=0 mid-frame; FIELD 0 from the next raw VSYNC; after
  three SELECTED rises the consumer is 0 with the scandoubler re-enabled.
- p1/p2 R8=0 / R8=2 progressive guards: constant 0, all seam, equal lens.
- n1 negative control (Plus 0 via bus, classic 3 poked): three ACTUAL
  selected filtered VSYNC rises witnessed under perturbation (8M bound,
  ~5.85M worst case); pin 0 at every accept, consumer 0/enabled after;
  one final PASS/FAIL for the case.

## Validation (this checkout, corrected test)

Focused B8 test, exact form:

    make -C sim/plus obj_dir/b8_field/b8_field_tests
    ./sim/plus/obj_dir/b8_field/b8_field_tests

Result: exit 0; 11 PASS, 0 FAIL. Raw log
`docs/references/b8-field-evidence/b8-field-focused-final.log` (ignored
directory, not a tracked artifact): t1 hold covered 1109404 interior ticks;
n1 witnessed 3 selected accepts with consumer 0/enabled.

Pristine-base replay, immutable base
`2e104c73e83246bc65fdd2e49d0d0751d439cc80`: its
`rtl/Amstrad_motherboard.v` + `rtl/plus/asic_video.v` materialised to temp
files, replay binary built with those two paths substituted (all else
current), then run against the corrected test. Exact form:

    git show 2e104c73e83246bc65fdd2e49d0d0751d439cc80:rtl/Amstrad_motherboard.v > <tmp>/orig/Amstrad_motherboard.v
    git show 2e104c73e83246bc65fdd2e49d0d0751d439cc80:rtl/plus/asic_video.v > <tmp>/orig/asic_video.v
    verilator --cc --exe --build --language 1364-2001 -UVERILATOR --public-flat-rw \
      --top-module b8_field_bench_top --Mdir sim/plus/obj_dir/b8_field_orig -Wno-fatal \
      -CFLAGS "-std=c++17 -O2" -o b8_field_orig_tests \
      sim/plus/b8_field_bench_top.v <tmp>/orig/Amstrad_motherboard.v \
      rtl/plus/asic_regs.v rtl/plus/plus_sprite_ram.v sim/plus/motherboard_lint_stubs.v \
      rtl/CRTC.v rtl/crtc_type0_engine.v rtl/crtc_type1_engine.v rtl/plus/asic_ga_timing.v \
      <tmp>/orig/asic_video.v rtl/plus/asic_sprites.v rtl/plus/asic_dma.v rtl/crt_filter.v \
      rtl/Amstrad_MMU.v rtl/i8255.v rtl/GA40010/casgen_sync.v rtl/GA40010/syncgen_sync.v \
      rtl/video_interlace.v sim/plus/b8_field_cpu.v "$(pwd)/sim/plus/b8_field_test.cpp"
    ./sim/plus/obj_dir/b8_field_orig/b8_field_orig_tests

Result: exit 1, 10 case-failures. Raw log
`docs/references/b8-field-evidence/b8-field-orig-final.log` (ignored, not
tracked). Failing: s2/z2 constant 0 (classic `interlace=&R8` gates R8=1
out), z1 non-exception polarity, t1 mid-frame hold, n1 pin 1 at selected
accept 3 with consumer leaked (`interlace_o=4`) — the n1 span alone reads
0/0, so the witnessed accepts are what expose the leak. s1/t2/p1/p2 pass
on baseline through the classic leak itself.

The parent reran all aggregate gates after the corrected test: `make -C sim`,
`make -C sim lint`, and `make -C sim soak SOAK_EXPECT=0x6e8258198d6e6137`
all exit 0. The soak matches over 2,845,088 characters and CLKEN samples.
Local raw logs are `docs/references/b8-field-evidence/b8-field-final-sim.log`,
`b8-field-final-lint.log`, and `b8-field-final-soak.log` in that directory.

## Scope and residuals

Changed: `rtl/plus/asic_video.v` (FIELD + FR anchors),
`rtl/Amstrad_motherboard.v` (Plus/classic field mux), `rtl/video_interlace.v`
(new behavioral extraction, shared by `Amstrad.sv` via `files.qip`),
`sim/plus/b8_field_bench_top.v` + `b8_field_cpu.v` + `b8_field_test.cpp`
(new bench, in the default gate), `sim/plus/Makefile` (B8-2 target),
`Amstrad.sv` (consumer instance; unchanged runtime sampling, explicit zero startup
in the extracted module). B8-3, SDRAM and snapshot behavior are outside this change.

Residuals: ASCAL polarity is a source-derived MiSTer convention from
`sys/ascal.vhd:1233-1252`, not hardware-measured clearance. The fixture runs
the motherboard and local interlace-history decision, not full ASCAL,
video_mixer/HQ2x/freeze, or an executed production T80. Quartus timing, an
RBF, and named-title or hardware retests remain integration/hardware gates.

## Independent review

Opus 5 high, run `20260908T043949Z-30604-f10d`, cleared the functional FIELD
polarity, R7=0 association, selected mux, consumer wiring and sampling. Its
blocking test/documentation findings were corrected before acceptance.
A subsequent Opus closure attempt hit the session limit without a verdict.
Gemini 3.8 Flash high, run `20260908T050913Z-72780-1627`, independently
returned CLEAR on the corrected transition watch, selected-edge negative
control, field-length association, initialization wording and evidence record.
That bounded Gemini review supplements the retained Opus findings; it is not
an additional full Opus review or hardware validation.
