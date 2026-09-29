# PA7 DMA / compatible interrupt overlap repair

Status: **INTEGRATED** at `4a443942a6755225b79b892ff8fb12242ec30aa2` from
`d563586a06da0edc3dd9c6f69c5aa03dfb0d7dc6`. Review, exact integration CI and
full-effort synthesis pass; the integration RBF is delivered locally. MiSTer
diagnostic acceptance below covers source RBF `011f323d`; the user reports clean
title regression testing on integration RBF `4a44394` (2026-09-29). Release is separate.
Dependency base: `9888a90418551d5d1315b3f3a91e2b9c3538fcbf`.
Branch: `codex/plus/pa7-dma-overlap-repair`. Integration and release are separate.

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
ASIC pin timing remain separate work. Broader title acceptance is recorded below.

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

The user authorized branch publication and hosted synthesis after the local
Quartus VM failed hostname resolution. The reviewed implementation is
`5eb2d35354ce6883e80d9c83e01ab104b5d13771`; full-effort
[workflow run36453223818](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/36453223818)
built its documentation-only descendant `011f323d938d727895bfcbabc16b001ad28602e5`.
All required jobs passed, including selected simulation/lint and production T80.

- Artifact: `Amstrad-build-295-1-full`, `build_mode=clean_full`.
- RBF: `Amstrad_20260928_011f323.rbf`.
- RBF SHA256: `a42a2a5be09b33510be4609dc38697aa3d39eb987c6a2886dccfe4ea4723c8f7`.
- Timing gate: **PASS**, setup minimum **0.145 ns**, hold minimum **0.242 ns**,
  across seven clocks; **TNS zero**. This is closure under the existing constraints;
  the build log retains the existing not-fully-constrained notices.
- Retained build and workflow logs: `output_files/pa7-dma-repair/synthesis/`.
  The retained package contains fitter/STA reports and summaries as well as
  the build log and provenance (contents rechecked 2026-09-29).

The hardware-loop driver loaded this hash-pinned RBF and the unchanged cartridges
on MiSTer with 6128+ and Full filter settings. All ten cases completed and cleaned
up successfully. All thirty PNGs were inspected; each screen's three captures
are byte-identical.

- All four overlap pages: the entire displayed record area matches the repaired
  production simulation, which passes all68 original-Plus photograph records.
  The 639×200 region `(17,40)-(656,240)` on MiSTer matches simulation
  `(17,0)-(656,200)` exactly. The adjacent border column at x16 differs only in
  blue level (MiSTer128, simulation136); the unmodified 640×200 comparison and
  its difference rectangle are retained rather than reported as identical.
- V5 screens30–35: all six full 640×200 active regions match the accepted
  production simulation pixel-for-pixel, including every marker and count.
- CFG SHA256 before and during capture:
  `13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`.
  The saved configuration already selected the requested fields, so its bytes
  did not change. Per the user's instruction, the test settings are left applied;
  the new RBF remains running on V5 screen35.

Cases, original CFG, per-capture hashes, manifests, contact sheets and comparison
results are retained under `output_files/pa7-dma-repair/mister/`, with a durable
ignored copy under `docs/references/pa7-dma-repair-2026-09-28/`.
These native captures establish the diagnostic software results on this MiSTer
build; they do not measure internal ASIC pin timing or replace original hardware
as the rule authority. Broader title regression remains separate.

### Integration artifact

[Push run36460031177](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/36460031177)
verified exact merge SHA `4a443942a6755225b79b892ff8fb12242ec30aa2`. Production T80,
synthesis-policy, selected simulation/lint (41 benches), routing, full hosted
synthesis and required-gate all passed; the unselected local synthesis leg was skipped.
The merge required no code conflict resolution or code edits. Shared docs were
reconciled; review debt and the classic soak golden remain unchanged.

- Artifact `Amstrad-build-296-1-full`, provenance `build_mode=clean_full`.
- Delivered in the main checkout: `output_files/Amstrad_20260928_4a44394.rbf`.
- SHA256: `285f42583bdb52f2f85bdceee087de18594c68c05352f32295e3611b802034a4`;
  download and delivered copy hashes match.
- Timing closure **PASS**: setup minimum **0.238 ns**, hold minimum **0.180 ns**,
  seven clocks each, **zero TNS**, under the existing constraints.
- Logs and downloaded artifact: `output_files/pa7-integration-4a44394/`.
- Retained reports verified 2026-09-29 under
  `output_files/pa7-integration-4a44394/Amstrad-build-296-1-full/reports/`:
  fitter report/summary, STA report/summary/paths and build log are present.
  The adjacent RBF hash matches the delivered identity above. Fitter summary:
  **24,384 ALMs (58%)**, 28,449 registers, 102 RAM blocks, 35 DSP blocks.
  The STA summary independently confirms +0.238/+0.180ns minimum setup/hold
  and zero TNS. The earlier missing-report statement was incorrect for this
  retained package; no workflow report-collection repair is justified.

The thirty MiSTer captures cover the same production source before the merge
(`011f323d`), not a fresh device run of the integration RBF. Subsequent
acceptance-record edits are documentation-only;
they reuse this exact artifact without relabelling it as a newer build.

### Integration title acceptance — 2026-09-29

The user confirms that the requested title regression pass on
`Amstrad_20260928_4a44394.rbf` is complete with **no issues**. The requested
set was FF2, Prehistorik II, Eerie Forest, Sonic GX, Copter 271 and the CRTC3
demo. This closes the pending broader title regression action for this build.
It is user-reported acceptance; no new automated captures, per-title play
lengths, device hash verification or sync/model settings were supplied with
this report. It does not close original-machine PA3 controls, B25 classic
phase measurements or the exact internal ASIC timing assumptions.
