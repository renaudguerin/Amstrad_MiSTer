# B18 attached-cartridge save admission — 2026-09-29

Base: `fedc1e4ff7cdc10698ce294d78c5379cea262e32`.

The documented classic snapshot contract excludes an attached Dandanator because
SNA does not contain its cartridge/mapper state. `Amstrad.sv` previously gated
`save_admit` with `!dan_ena`. `rtl/plus/plus_legacy_cart_gate.v` makes that signal
`loaded && !dandanator_nce && !plus_mode`; the EEPROM select depends on memory
address and read/write strobes in `rtl/dandanator/cpc_dandanator.vhd`'s EEPROM
activation mapper. Running from RAM therefore does not imply detachment.
The admission term now uses `!dan_eeprom_loaded`, whose latch persists until
explicit detach. No capture state machine, abort, DDR or transport logic changes.

## Regression and evidence

`sim/prepare_b18_save_admission.py` injects the current production admission
expression into `sim/b18_save_admission_test.sv`, composing the real legacy-cart
latch and save-capture controller. It rejects missing/ambiguous extraction.
The test retains this cross-module boundary rather than duplicating its Boolean
logic. Its scripted attachment while armed isolates the persistent-loaded term;
it does not claim a normal download bypasses the independent `ioctl_download`
interlock. The bench tests admission reset as an input only, not save-abort wiring.

Command (the local Command Line Tools installation needs the SDK C++ headers):

```sh
CPLUS_INCLUDE_PATH=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1 make -C sim b18-save-admission-test
```

Before the production edit: five assertions fail, covering armed cancellation,
a later boundary wrongly capturing, inactive-select admission/refusal and capture
after the wrongly admitted request. After the edit: `PASS B18 save admission:
attachment state, cancellation and classic controls`. Active-select refusal,
explicit detach followed by successful capture, ordinary classic capture and
Plus refusal pass, as do the other admission-input controls.

Final selected gate:

```sh
CPLUS_INCLUDE_PATH=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1 python3 sim/select_tests.py --run
```

```text
select_tests: PASS 42 benches: crtc-test, crt-filter-blank-test, crtc-cpu-phase-test, video-color-test, video-output-test, ssm-marker-test, sna-cpu-header-test, b18-save-admission-test, video-mixer-rgb-test, ga40010-test, u765-test, run/asic_unlock_tests, run/asic_video_tests, run/b6_menu_mask_tests, run/dandanator_loader_bounds_tests, run/rom_loader_route_tests, run/plus_legacy_cart_gate_tests, run/plus_cartridge_memory_tests, run/plus_cartridge_memory_min_tests, run/sdram_cartridge_tests, run/plus_cpr_parser_tests, run/plus_mmu_tests, run/p0_boot_tests, run/asic_ga_timing_diff_tests, run/p1_video_tests, run/p1_mobo_bench_tests, run/asic_regs_tests, run/plus_sprite_ram_tests, run/asic_pri_tests, run/asic_sprites_tests, run/d3_sprites_tests, run/p4_sprites_regs_tests, run/p4_multiplex_tests, run/asic_dma_tests, run/plus_p8_tests, run/b16_load_model_tests, run/p10_boot_tests, run/p10_input_tests, run/p10_dma_ppi_tests, run/p10_dma_mobo_tests, run/b8_palette_tests, run/sna_save_stream_tests
```

Fresh cross-provider review: `claude-opus-5-5`, medium, guarded `ask-claude`,
run `20260929T002733Z-52071-60b3`: **CLEAR**, no blocking findings. Scope was the
single production admission change and its regression/extractor/make/index.
Reviewer inspected the fail-before/pass-after logs and did not rerun the gate.
Parent separately ran `python3 sim/select_tests.py --check` for index coverage.
Optional review wording advice was applied to the design documentation; code was
not changed after the gate or review.

Raw logs: ignored `sim/obj_dir/b18_save_admission/evidence/{before,after,gate}.log`
and `review-output.log`, `review-process_state.json`, `review-run_info.json` in
this task checkout. Reproducible bench and extraction sources are tracked.

## Limits and handoff

This is a source-derived admission-policy fix, not original-hardware evidence.
It does not simulate the complete MiSTer top level or Dandanator VHDL mapper;
its chip-select stimulus deliberately distinguishes idle and active bus ownership.
The selected gate includes the existing save-stream suite; the slow production
snapshot round-trip fixture was not rerun for this admission-only change.
Exact integrated-SHA CI and full Quartus synthesis remain the coordinator's
responsibility. No hardware access, synthesis dispatch, release or push occurred.
This evidence closes the named admission finding only; historical B18 slice 4c
review-debt archival remains with the coordinator and its broader review record.
