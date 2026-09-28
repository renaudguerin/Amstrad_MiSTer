# PA7 DMA / compatible interrupt overlap repair

Status: **READY code** — reviewed and validated; hosted synthesis and MiSTer
comparison in progress. Branch publication is authorized for synthesis;
integration and release remain separate.
Dependency base: `9888a90418551d5d1315b3f3a91e2b9c3538fcbf`.
Branch: `codex/plus/pa7-dma-overlap-repair`. Integration and publication are separate.

## Hardware requirement and repair

The [original-6128-Plus photographs and full transcription](pa7-dma-overlap-original-plus-2026-09-28.md)
require both sources to survive a DMA acknowledge around compatible delivery.
The four unchanged cartridge pages have eight differing rows against the old
model: offsets −3/−2 on pages1/3/4 and −4/−3 on page2. Hardware delivers DMA04
then raster06; the old implementation delivers DMA04 alone. No-DMA anchors
remain 0, −1, 0, 0. These observations establish software behavior, not the
literal time of internal ASIC request creation.

`asic_ga_timing` now samples delivered-raster eligibility at each ASIC ACK
start and holds that classification for the pulse, matching `asic_regs` vector
selection. Only a raster-classified ACK clears compatible pending/delivery or
arms the held `irqack_rst` counter clear. A DMA ACK therefore permits compatible
creation and delivery to mature without changing its selected vector. Gating
only the pending latch would leave the raw-qualified counter-reset path able
to react to a request that the DMA ACK never selected.

Per-pulse classification is separate from DCSR bit7, which retains the first
ACK's provenance across split pulses within one M1. Programmed raster fire
deferral, raster priority, vector latching and DMA automatic/manual clearing
retain their existing behavior. The motherboard and T80 have no production
changes. The next-character compatible delivery stage remains the V5 model;
the photographs still do not prove this internal mechanism. Preservation of
a PRI-masked compatible latch across DMA ACK follows this source-ownership
model and is not a separately photographed hardware case.
If compatible delivery matures between two shaped ACK pulses in one M1 after
a DMA first pulse, the second pulse can now select raster06 while DCSR retains
the first pulse's DMA provenance. This consistent vector/provenance consequence
remains part of the unmeasured split-ACK corner, not a new hardware claim.

## Regression and preservation evidence

The new `sim/plus/pa7_dma_overlap_test.py` runs the unchanged four-page cartridge
through the production GHDL-T80 motherboard fixture. Its expectations are the
tracked photograph transcription: all 68 full displayed records, including
source order, count, DCSR, marker, pre/post status, completion, eight repeats,
and zero disagreement/anomaly flags. Start00 CPR SHA256 remains
`0f499bc6c50f935b0f9a455ec953db44a97a6a2db5da57aa215914c80c7c7106`.

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/plus/pa7_dma_overlap_test.py --out output_files/pa7-dma-repair/before
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/plus/pa7_dma_overlap_test.py --out output_files/pa7-dma-repair/after
```

The first page reported both failures before any RTL edit. The pre-fix compiled
binary continued through the remaining pages unchanged: **60 passed, 8 failed**.
The repaired model reports **68 passed, 0 failed**. The retained slow target is
`make -C sim/plus pa7-dma-overlap`; it needs GHDL, Verilator and sjasmplus.

Additional checks:

- `asic_pri`: programmed timing, deferred fire, mask/unmask, counter phase and
  snapshot controls pass. `pr14` keeps the V5 delivery bounds and MRER cancel
  check. Its former ACK-cancels-undelivered expectation is contradicted by
  the photographs and replaced with DMA retention before/across maturation,
  unchanged ACK provenance and no held raster counter clear.
- `b20-ack-diag`: DMA04 stays stable through 100 clocks spanning compatible
  maturation, in automatic and manual modes; subsequent raster gives06/80.
  Existing split-ACK, first-pulse DCSR and simultaneous raster/DMA priority pass.
- `asic_ga_timing_diff`: all eight cases pass. Every comparison remains active.
  Random ACK stimulus now requires a delivered raster in both models; empty
  and undelivered DMA ACKs are outside the shared GA40010 behavior established
  by hardware. The fixture drives the actual ACK input. Directed counter,
  VSYNC, MRER, reset and bus/sync checks remain.
- V5 screens30–35: all six 640×200 active images are pixel-identical to the
  accepted `production-corrected` simulation from the PA7 follow-up checkout.
  This includes every photographed marker and interrupt count. Reproduction
  and hashes: ignored `output_files/pa7-dma-repair/validate_v5.py` and
  `v5/comparison.json`.

The canonical soak hashes classic CRTC behavior; neither the CRTC nor its
stimulus/projection changes here. This Plus-only repair does not remint that
golden. Classic preservation is covered by the selected integration benches.

## Acceptance and artifact boundary

Opus5.5 high independently reviewed the production change, board/register
wiring, regression oracle and differential stimulus restriction in guarded run
`20260928T163345Z-97447-c4ac`: **GO, no blockers**. It ran no tests; access to
the parent's `/tmp` test logs was denied, so test execution results were supplied
by the parent. The parent directly verified every reported result. The report
is retained under `output_files/pa7-dma-repair/review/`.

The targeted slow hardware regression and B20
boundary bench were run explicitly above, separately from the default selection
gate; they need no duplicated run solely to include them in one command.
No result is inferred from a queued or running stage. Original probe photographs, counterfactuals
and production traces remain in the read-only `pa7-dma-overlap` checkout;
repair outputs live under this checkout's ignored `output_files/pa7-dma-repair/`.

B25 classic phase investigation, PA3 extra original-machine controls, precise
ASIC pin timing and broader title acceptance remain separate work.

### Final local gate

```sh
MAKEFLAGS='CXX=/opt/homebrew/opt/llvm/bin/clang++' python3 sim/select_tests.py --run
```

Exact final result:

```text
select_tests: PASS 41 benches: crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

Index validation: `select_tests: index OK (57 rows)`.
The production-T80 preservation command also passed:

```sh
make -C sim classic-irq-phase-test t80-irq-sample-test lint CXX=/opt/homebrew/opt/llvm/bin/clang++
```

Results: `classic_irq_phase: 8 passed, 0 failed`; INT sampling `Summary: 14 passed, 0 failed`;
all lint targets exit0. Logs are preserved in `output_files/pa7-dma-repair/validation/`.
No production behavior changed after the reviewed regression runs; the final
source edit before the selection gate was comment wrapping.

### Hosted synthesis and MiSTer comparison

The local Quartus VM address does not resolve from this Mac. The user has
authorized pushing the repair branch to GitHub for full-effort hosted synthesis.
The reviewed implementation commit is `5eb2d35354ce6883e80d9c83e01ab104b5d13771`;
later documentation-only descendants preserve its code. The hosted workflow
dispatch SHA and artifact/timing results will be recorded after completion.

MiSTer is reachable and reports `Amstrad`. Only a read-only preflight was made;
its configuration and running session were not changed. Loading/capturing the
new RBF remains pending synthesis. Retain timing reports and RBF hashes, then
use the hardware-loop driver to compare all four overlap pages and V5 screens30–35.
Existing probe cases and images are available in the referenced read-only
checkouts. Preserve and verify the device CFG across that run.
