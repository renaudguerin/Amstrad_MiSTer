# B4 production-T80 fetch-provider evidence

## Scope

This investigation starts at `9ddbf74b35f4f5214f979d5039eb2961b536d121` and
addresses the original SSM raw-fetch-provider boundary. The earlier isolated
production-T80 substitution passed 26/27 tests but stopped at a TV80-specific
HALT address assertion before testing interrupt cancellation. Its copied data
mux and fetch expression could not establish motherboard integration even if
all tests passed. The original result remains recorded in the
[B22 acquisition report](../video-boundary/b22-acquisition-trace-2026-09-29.md).

## Production boundary inspected

`rtl/Amstrad_motherboard.v` exports `ssm_m1_fetch` from the existing
`~M1_n & ~MREQ_n & ~RD_n` expression. `ssm_bus_data` exports `cpu_data_bus`,
the same net connected to production T80's data input. That bus combines the
selected CRTC and PPI responders with `cpu_read_data` by wired AND. The Plus
retained-byte substitution applies only to its qualified GA I/O read.

The CPU's WAIT input combines GA READY, idle-bus qualification and the
`no_wait` option, then applies the cartridge-memory and DMA/PPI wait vetoes.
The observer neither supplies a CPU enable nor changes WAIT. The detector
retains the last byte seen while the fetch level is high and recognizes
completion on the first clock where that level is low. It captures timestamp
and sync observations at that completion edge, before recognition and DDR
publication.

`Amstrad.sv` connects those motherboard exports directly to `ssm_marker` on
`clk_sys`, using `ce_16` and motherboard-selected HS/VS for raster stamping.
Its DDR output passes through shared arbitration. This connectivity review
alone does not prove execution timing, DDR electrical timing or host coherence.

## Why the isolated interrupt test stopped at 0039

The local TV80 surrogate (`sim/plus/tv80/tv80_core.v`) decodes `DInst=76`
in its fetch state by asserting `halted` and skipping the PC increment.
Production T80 (`rtl/T80/T80.vhd`) increments PC at M1/T2 before the newly
fetched HALT is decoded, then asserts `Halt_FF` at the later instruction
boundary. A live address sampled after HALT therefore differs between these
implementations. It is not a record of the opcode fetch that led to HALT.

The meaningful interrupt execution oracle is the completed handler fetch at `0038:76`,
an interrupt acknowledge that contributes no opcode byte, and stacked return
address `0008` between the two ED pairs. The exact completed-fetch sequence proves ACK contributes no byte; the
handler HALT fetch breaks the partial marker. Zero events must still be
asserted, with an interrupt-disabled control executing both pairs. Changing
only the expected live terminal address from `0038` to `0039` would not prove
any of those properties.

## Deterministic integration fixture

`make -C sim ssm-provider-test` builds the actual motherboard with the existing
B18 named-port-only adaptation to the GHDL-translated production T80 netlist.
The test connects the motherboard's exported SSM fetch/data signals to the
production detector. It does not copy either provider expression. The new
slow bench also belongs to `production-t80-test`, the existing CI entry point.

The five cases pass:

| Case | Required evidence |
| --- | --- |
| Classic ROM marker | Exact completed opcode stream, one `FFFE` event; final HH fetch spans 24 master clocks, including 4 with WAIT asserted |
| Plus extended-WAIT marker | HH spans 216 clocks, including 200 with WAIT asserted; provisional `00` becomes `FF` before completion; no early event and exactly one final `FFFE` |
| ROM-to-RAM plus PPI | Executed GA write disables ROM; RAM marker remains exact; operands and PPI I/O are excluded from fetches; resolved PPI byte is consumed by CPU and stored in RAM |
| IM1 between pairs | Exact fetch stream ends at `0038:76`, one ACK, stacked return `0008`, zero markers; observed halted address is `0039` |
| DI control | Same ED pairs execute uninterrupted and produce one `FFFE` marker with no ACK |

The PPI responder returns `F4` or `F5` according to live VSYNC with the
fixture's jumper configuration; the test asserts its fixed bits and verifies
the CPU's stored byte against the observed resolved bus. This exercises the
motherboard data mux outside opcode fetches as well as fetch qualification.

A temporary control changed only `ssm_bus_data` in the generated motherboard
to constant zero. The test failed with `opcode fetch bus data mismatch`;
restoring the production assignment passed all five cases. The mutation is
not retained. Logs are `output_files/b4-production-fetch/negative-tap.log`
and `focused.log`. This is an intentional integration regression test, not
a production behavior repair; no RTL assertion was weakened to make it pass.

The fixture supplies external ROM/RAM bytes from C++, uses real motherboard
GA ROM-selection output, and drives the existing extra-WAIT input. It does not run
the SDRAM controller or cartridge responder and does not test DMA/PPI WAIT
arbitration. It indexes memory by the CPU address, so bank mapping and
`mem_addr` are not exercised. Identical low ROM/RAM bytes allow the transition
to run but do not independently pin ROM-disable timing at the immediate
post-OUT fetch. The Plus case exercises motherboard `plus_mode` and the
external wait veto, not cartridge-backed boot or natural Plus READY waits.
It stops eight clocks after HALT and does not test repeated HALT M1 cycles.
Detector HS/VS are tied low and DDR busy is tied low: raster
stamping and DDR publication/coherence are outside this bench. Existing
`ssm-marker-test` retains their isolated detector tests.

The production T80 wrapper's startup `IntCycleD_n` state is a separate B23
boundary: initial M1 may show IORQ alongside MREQ. The ACK monitor requires
MREQ inactive, and exact completed-fetch assertions remain enforced. This
task does not change or declare parity for that startup behavior.

## Evidence boundaries

- Source review establishes the connected provider and passive observer logic.
- Deterministic production-T80 execution can establish behavior only within
  the fixture's memory, clock, interrupt and peripheral models.
- GHDL translation and Verilator execution are not Quartus synthesis or
  physical timing closure.
- No MiSTer or original CPC access occurs in this task. Physical DDR/host
  coherence and format-2 marker observations require device acceptance.

No new bitstream is produced here. The coordinator's existing
`Amstrad_20260929_cec641c.rbf` acceptance candidate keeps its identity; these
simulation results do not extend its recorded hardware acceptance.

## Validation

The focused command was `make -C sim ssm-provider-test`, with final result
`ssm-provider: PASS 5 production motherboard/T80 cases`. The selected gate was:

```sh
CPLUS_INCLUDE_PATH=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1 python3 sim/select_tests.py --run
```

The include path is a host build workaround, not a repository requirement.
The recorded final line in `output_files/b4-production-fetch/final-gate.log` is:

```text
select_tests: PASS 42 benches: crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, b18-save-admission-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

Fresh cross-provider review: Claude Opus 5.5 medium, run
`20260929T043549Z-23673-ac79`, **CLEAR** for source/in-simulation provider
closure. The review suggested removing the post-HALT address assertion;
it is now diagnostic output only. Its other two low-severity observations
are addressed by specifying what the exact fetch sequence proves and limiting
the ROM-disable claim rather than adding unrelated timing coverage. Saved
review: `output_files/b4-production-fetch/review-opus.log` and
`review-process-state.json`. The focused and selected logs above were refreshed
after the assertion removal. No production RTL changed.

## Integration evidence reconciliation

Rebased onto documentation integration `d188607` without changing simulation or
RTL files. The separate [MiSTer session](../mister-format2-b25-2026-09-29.md)
provides bounded format-2 startup/coherence and steady requested/applied mode
acceptance on the same `cec641c` candidate. That device evidence supplements this
fixture; it does not prove live transitions or independently measure the fetch
sampling cut. Existing review and gate results remain applicable unchanged.
