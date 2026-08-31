# B7 dark-silicon signal-path audit

Date: 2026-09-01

This audit tests the ownership boundary in `rtl/Amstrad_motherboard.v`. The
classic CPC logic and the Plus logic are both instantiated, so a module can be
correct in isolation while its output is discarded by the selected-machine
mux. The audit deliberately mutates one module at a time and hashes only
signals after that mux, together with the CPU-visible bus and state.

## Fixture choice

The selected fixture is `sim/plus/p10_boot_test_top.v`, the P10a real-T80 CPR
boot fixture. It has the widest useful observation surface of the three
candidates:

| Fixture | Relevant coverage | Reason it was not selected or selected |
| --- | --- | --- |
| `p10_dma_mobo_test_top.v` | Real motherboard Plus DMA/PPI/PSG concurrency, with a scripted CPU and several DMA probes | Rich for the P10e input path, but it does not exercise the CPR parser, cartridge memory service, Plus MMU/unlock path, or expose the requested MA and RA outputs. |
| `p1_mobo_bench_top.v` | Motherboard video outputs and a scripted CPU | It has a narrower scripted bus and does not cover the real CPR, cartridge, SDRAM, and unlock sequence. It also does not expose MA and RA. |
| `p10_boot_test_top.v` | Real T80pa, CPR parser, cartridge memory, Plus MMU and unlock detector, SDRAM model, production motherboard, CPU bus/cycle probes, interrupt probes, and B7 taps for RGB, HSYNC, VSYNC, DE, MA, and RA | Selected. It covers the most complete CPU-to-motherboard path while exposing every required selected-machine output. |

The Plus run uses `plus_model_i=2` (6128+), and the classic run uses
`plus_model_i=0`. The test program loads the Plus CPR image, completes the
unlock sequence, selects the cartridge and ASIC windows, writes CRTC, palette,
sprite, split/scroll, and DMA state, and leaves the real CPU repeatedly reading
the ASIC page. The classic run seeds the SDRAM-backed classic map with the
same deterministic program. Under the B7-only conditional the cartridge
instance uses `CLEAR_BYTES=20'd32` to avoid spending time clearing unused
simulation storage; the CPR image writes both 16 KiB pages used by this run
before the reset-release point.

## Signature and mutation method

Each invocation is deterministic. After fixture reset release, the runner
hashes 65,536 prelude ticks and 131,072 measured ticks, for 196,608 ticks in
one post-reset run. The prelude is included in the signature so setup reads
such as the sprite-RAM host read cannot be accidentally discarded. Baseline
invocations are repeated and must produce the same value before any mutation
is accepted.

The signature is a 64-bit FNV-1a rolling hash. Values are serialized as
little-endian bytes in this order at every tick:

1. Selected motherboard outputs: RGB, HSYNC, VSYNC, DE, MA, RA, MODE, left
   audio, right audio, and the CPU interrupt input after the motherboard and
   deterministic testbench interrupt stimulus are combined.
2. CPU bus: PC, address, data out, data in, M1, MREQ, IORQ, RD, WR, the
   cartridge wait signal, and the CPU's combined WAIT input.
3. CPU interrupt and cycle state: interrupt input, interrupt acknowledge,
   interrupt-vector byte, vector-valid, machine cycle, T-state, instruction
   register, machine-cycle maximum, and the positive CPU clock enable.

The runner uses `+mutate_module=<name>`. The Verilog top maps the name to one
mutation ID and applies one force set to one named module instance per run.
The mutations are deliberately large enough to be visible if that module's
boundary is selected, but the hash never includes the mutation ID, mutation
control, or an unselected raw module output. The mutation ID and enable are
read-only fixture outputs; before sampling, the C++ runner independently maps
the requested name and requires the Verilog decoder to report that exact
nonzero ID. An unknown or inactive mutation therefore fails instead of
silently satisfying an unchanged-signature assertion.

The exact guard around the mutation mechanism is:

```verilog
`ifdef B7_DARK_SILICON_MUTATION
...
// synthesis translate_off
...
// synthesis translate_on
`endif
```

The `// synthesis translate_off` and `// synthesis translate_on` comments
enclose the `$value$plusargs`, `force`, `release`, and mutation `always` block.
The surrounding `B7_DARK_SILICON_MUTATION` conditional also guards the B7
ports and taps. Only the B7 Verilator rule in `sim/plus/Makefile` passes
`-DB7_DARK_SILICON_MUTATION`; the production Quartus source list does not
include this simulation fixture and does not define the macro. Thus Quartus
cannot see the mutation statements. The B7 rule also defines
`P10_DMA_MOBO_REAL_IO` and substitutes the real GA, YM2149, and HID sources
for the ordinary simulation stubs.

The mutation names are exactly:

`asic_video`, `asic_sprites`, `asic_dma`, `asic_regs`, `asic_ga_timing`,
`asic_unlock`, `plus_mmu`, `plus_sprite_ram`, `plus_cartridge_memory`,
`CRTC`, `crtc_type0_engine`, `crtc_type1_engine`, `ga40010`, and
`negative_control`.

The C++ runner accepts `--xfail`. A row marked with that option prints
`B7 XFAIL` when the measured unexpected result occurs and returns success;
the same row prints `B7 XPASS` and fails the gate if the finding disappears.
No current row needs that marker because this run found no unexpected result.

## Baselines

The required unmodified baselines were stable across repeated invocations:

| Mode | CRTC type | Baseline | Repeat |
| --- | ---: | --- | --- |
| Plus | 0 | `0xea03a76a5520e7eb` | `0xea03a76a5520e7eb` |
| Plus | 1 | `0xea03a76a5520e7eb` | `0xea03a76a5520e7eb` |
| classic | 0 | `0x4ace163975bf3441` | `0x4ace163975bf3441` |
| classic | 1 | `0x4ace163975bf3441` | `0x4ace163975bf3441` |

The type-1 repeats are used to select the type-1 classic-engine mutation;
the required Plus and classic baselines are the type-0 rows.

## Group A: Plus modules in Plus mode

Every row runs with Plus mode and CRTC type 0. The assertion is
`changed=yes`.

| Module | Baseline | Mutated | Changed | Notes |
| --- | --- | --- | --- | --- |
| `asic_video` | `0xea03a76a5520e7eb` | `0xfae0b905e63db697` | yes | Plus video payload reaches the selected RGB path. |
| `asic_sprites` | `0xea03a76a5520e7eb` | `0x968e09989683c79b` | yes | Plus sprite enable and pixel payload reach selected video or CPU-visible state. |
| `asic_dma` | `0xea03a76a5520e7eb` | `0xbc8d428a8c9b5c3c` | yes | Plus DMA state reaches the sampled machine and audio/CPU state. |
| `asic_regs` | `0xea03a76a5520e7eb` | `0xd4a0bfae02206efb` | yes | ASIC-page register output reaches the selected bus or video state. |
| `asic_ga_timing` | `0xea03a76a5520e7eb` | `0x19ec482fd4504163` | yes | Plus timing output reaches selected raster or CPU state. |
| `asic_unlock` | `0xea03a76a5520e7eb` | `0x9241c4f4e9091643` | yes | Unlock state controls the active Plus window. |
| `plus_mmu` | `0xea03a76a5520e7eb` | `0x39e825e9d48e79b3` | yes | Plus cartridge and ASIC-page ownership reaches the CPU bus. |
| `plus_sprite_ram` | `0xea03a76a5520e7eb` | `0xc8342e0fe4150718` | yes | The explicit host read is inside the hashed post-reset window. |
| `plus_cartridge_memory` | `0xea03a76a5520e7eb` | `0xafcb5d736d77b3d6` | yes | Cartridge data reaches the CPU-visible read path. |

No Group A module was dead or muxed away in this fixture.

## Group B: classic modules while Plus mode is selected

The assertion is `changed=no`. CRTC and type-0/GA rows use CRTC type 0; the
type-1 engine row uses CRTC type 1 so the named engine is selected on the
classic side of the design even though Plus owns the selected output.

| Module | Baseline | Mutated | Changed | Notes |
| --- | --- | --- | --- | --- |
| `CRTC` | `0xea03a76a5520e7eb` | `0xea03a76a5520e7eb` | no | Classic CRTC wrapper is not selected by Plus MA/RA/DE/sync ownership. |
| `crtc_type0_engine` | `0xea03a76a5520e7eb` | `0xea03a76a5520e7eb` | no | Type-0 engine mutation does not leak into Plus outputs or bus state. |
| `crtc_type1_engine` | `0xea03a76a5520e7eb` | `0xea03a76a5520e7eb` | no | Type-1 engine mutation does not leak into Plus outputs or bus state. |
| `ga40010` | `0xea03a76a5520e7eb` | `0xea03a76a5520e7eb` | no | Classic Gate Array sync and colour outputs are not selected in Plus mode. |

The supporting classic-active controls also changed as expected: `CRTC`
`0xbb1ff4297d426cd1`, `crtc_type0_engine` `0xa3a62ef6a7438799`,
`crtc_type1_engine` `0xa3a62ef6a7438799`, and `ga40010`
`0x410b8a14822fee31`. These controls are not substituted for Group B; they
show that the classic mutation points are exercised by the same fixture.

## Group C: Plus modules while classic mode is selected

The assertion is `changed=no`. Every row uses classic mode and CRTC type 0
with baseline `0x4ace163975bf3441`.

| Module | Baseline | Mutated | Changed | Notes |
| --- | --- | --- | --- | --- |
| `asic_video` | `0x4ace163975bf3441` | `0x4ace163975bf3441` | no | Plus video cannot override classic selected outputs. |
| `asic_sprites` | `0x4ace163975bf3441` | `0x4ace163975bf3441` | no | Plus sprite state is isolated in classic mode. |
| `asic_dma` | `0x4ace163975bf3441` | `0x4ace163975bf3441` | no | Plus DMA state is isolated in classic mode. |
| `asic_regs` | `0x4ace163975bf3441` | `0x4ace163975bf3441` | no | Plus register-page mutation does not reach the classic bus. |
| `asic_ga_timing` | `0x4ace163975bf3441` | `0x4ace163975bf3441` | no | Plus timing mutation does not reach classic raster state. |
| `asic_unlock` | `0x4ace163975bf3441` | `0x4ace163975bf3441` | no | Plus unlock state is isolated in classic mode. |
| `plus_mmu` | `0x4ace163975bf3441` | `0x4ace163975bf3441` | no | Plus MMU mutation does not claim classic memory. |
| `plus_sprite_ram` | `0x4ace163975bf3441` | `0x4ace163975bf3441` | no | Plus sprite RAM mutation does not reach classic outputs or bus. |
| `plus_cartridge_memory` | `0x4ace163975bf3441` | `0x4ace163975bf3441` | no | Plus cartridge mutation does not reach classic memory reads. |

## Negative control and findings

The negative control forces `mb.asic_ga.MODE` to `2'b11`. The motherboard
instance leaves the ASIC timing module's `MODE` output unconnected, so this is
an intentionally harmless dead wire. Its result was:

| Control | Baseline | Mutated | Changed |
| --- | --- | --- | --- |
| `negative_control` in Plus mode | `0xea03a76a5520e7eb` | `0xea03a76a5520e7eb` | no |

This proves that the audit does not report every force as live and that the
signature is based on selected outputs and CPU-visible state rather than raw
internal mutation targets.

The active-mode companion rows provide the other anti-faking control. Every
Plus mutation name that is expected to stay unchanged in classic mode first
changes the Plus signature, and every classic mutation expected to stay
unchanged in Plus mode changes the matching classic signature. Together with
the decoder check, this prevents a misspelled, optimized-away, or unapplied
mutation from passing an isolation row merely because both signatures are
unmodified.

There were no expected-failure findings. All nine Group A modules changed,
all four Group B classic mutations stayed unchanged in Plus mode, all nine
Group C Plus mutations stayed unchanged in classic mode, and the negative
control stayed unchanged. Consequently the run emitted no `B7 XFAIL` or
`B7 XPASS` row.

The decisive B7 target completed with:

```text
All B7 dark-silicon audit assertions PASSED (expected findings remain visible as XFAIL)
```

The target is part of the default `sim/plus` test recipe, so `make -C sim`
also executes it.

## Limits found when the parent independently reran the audit (2026-09-01)

The audit was rerun in full by the parent session rather than accepted from the delegated
report. Every assertion reproduced, and the classic-active-control group behaves as intended:
mutating CRTC, crtc_type0_engine, crtc_type1_engine and ga40010 in classic mode moves the
signature, which is what makes the unchanged result for those same modules in Plus mode
meaningful rather than vacuous. Two limits were visible in that run and are not recorded above.

The two classic rule engines are not independently proven. In classic-active-control,
crtc_type0_engine and crtc_type1_engine both produce the same mutated signature,
0xa3a62ef6a7438799. Either both mutations reach the same shared wrapper state, or the mutation
selector is not distinguishing the two engines. The audit's conclusion about the Plus path does
not depend on this, but no claim should be made that either engine was separately exercised
until the cause is known.

The fixture does not reach CRTC-type-divergent behaviour. Classic-mode baselines are identical
for crtc_type 0 and crtc_type 1 (0x4ace163975bf3441 in both cases). A run long enough to
exercise the documented type differences would diverge. This caps what the audit can say about
the classic path generally: it proves the classic modules are reachable and mutable in classic
mode, not that the run covers type-specific behaviour. Extending the fixture, or accepting the
limit explicitly, is the follow-up.

Neither limit affects the audit's primary result, which concerns the Plus path.
