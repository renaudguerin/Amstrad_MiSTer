# Simulation bench index

Run only the benches a change can break. `python3 sim/select_tests.py` reads this table and the
change set (default: working tree against `git merge-base HEAD origin/master`), prints what it
picked and why, and runs them with `--run`. CI runs the same selection on every push.

A bench is selected when a changed file:

1. matches one of its **Covers** globs (the files the bench exists to protect), or
2. is one of its own testbench sources under `sim/`, found from `make -n` plus C++ includes, or
3. is RTL under `rtl/` or `sys/` that no row covers, and the bench compiles it (safety net; add
   the file to the right row when the script reports it).

A change to a sim Makefile or the pinned Verilator selects every fast bench. A **slow** bench is
only listed, never run, unless `--slow` is given: the motherboard-scale fixtures belong to
closed backlog tasks and run on demand or in an occasional full check (`make -C sim full`, or the
`full` simulation scope of a manual CI dispatch). Compiling a file is not the same as covering
it: a row covers a file when the file is part of the behaviour the bench asserts. Most Plus
motherboard benches compile the classic CRTC and GA40010, but classic CRTC or GA work selects
only the classic benches (plus the GA lockstep diff, which compares against GA40010).
`make -C sim lint` elaborates the whole motherboard in about a second and catches broken wiring
that no selected bench builds.

Keep the table in step with the Makefiles: `python3 sim/select_tests.py --check` fails when a
test target has no row, a row names a missing target, or a glob matches no tracked file.
Descriptions state what the bench checks, not its history. Timings are local runs on a 10-core
Mac, including the Verilator build.

| Target | Tier | Covers | Checks |
|---|---|---|---|
| `sim b18-snapshot-test` | slow | `rtl/sna_hw_header.v` `rtl/sna_save_capture.v` `rtl/sna_save_stream.v` `rtl/sna_cpu_header.v` `rtl/plus/plus_sna_header.v` `rtl/plus/plus_sna_apply.v` `sim/b18_snapshot*` `Amstrad.sv` `rtl/Amstrad_motherboard.v` `rtl/T80/*` `rtl/i8255.v` `rtl/YM2149.sv` `rtl/hid.sv` `rtl/Amstrad_MMU.v` `rtl/GA40010/*` `rtl/CRTC.v` `rtl/crtc_type*_engine.v` `scripts/hardware-loop/sna_pull.py` | Production T80 and real motherboard classic save/header/RAM restore, DDR stalls, and production host-reader publication races; also runs in production-t80 CI |
| `sim crtc-test` | fast | `rtl/CRTC.v` `rtl/crtc_type0_engine.v` `rtl/crtc_type1_engine.v` | CRTC types 0 and 1 at pin level against ACCC-derived vectors; the main classic accuracy suite (about 20 s) |
| `sim crt-filter-blank-test` | fast | `rtl/crt_filter.v` | crt_filter live blanking, SHIFT compensation and the type-1 R2.JIT displacement at production clocking |
| `sim crtc-cpu-phase-test` | fast | `rtl/CRTC.v` `rtl/crtc_type0_engine.v` `rtl/crtc_type1_engine.v` `rtl/GA40010/*.v` `rtl/GA40010/*.sv` | CRTC register writes landing at every legal CPU bus phase, with the real GA40010 |
| `sim video-color-test` | fast | `rtl/amstrad_video_color.sv` `rtl/color_mix.sv` `sys/gamma_corr.sv` | Colour and CE selection plus gamma, without the scaler |
| `sim video-output-test` | fast | `rtl/amstrad_video_output.sv` `rtl/amstrad_video_color.sv` `rtl/color_mix.sv` `sys/gamma_corr.sv` `sys/video_mixer.sv` `rtl/video_interlace.v` `sys/video_freak.sv` `sys/video_freezer.sv` `sys/scandoubler.v` `sys/hq2x.sv` `sys/math.sv` | Production output chain from a synthetic timing source: crop window, retained processing settings, sample-recorder tap |
| `sim ssm-marker-test` | fast | `rtl/ssm_marker.v` | SSM v1.2 consecutive opcode-fetch markers and event-ring publication |
| `sim sna-cpu-header-test` | fast | `rtl/sna_cpu_header.v` | SNA CPU header decode from the T80pa register layout |
| `sim video-mixer-rgb-test` | fast | `sys/video_mixer.sv` | MiSTer video mixer RGB scope correction |
| `sim ga40010-test` | fast | `rtl/GA40010/*` `rtl/crtc_type1_engine.v` | GA40010 netlist with the CRTC: type-1 R2.JIT timing |
| `sim u765-test` | fast | `rtl/u765/*` | u765 floppy controller |
| `sim/plus run/asic_unlock_tests` | fast | `rtl/plus/asic_unlock.v` | ASIC unlock sequence detector |
| `sim/plus run/asic_video_tests` | fast | `rtl/plus/asic_video.v` | Plus CRTC (type 3) counters and register bus |
| `sim/plus run/b6_menu_mask_tests` | fast | `rtl/plus/plus_menu_capability_mask.v` `rtl/plus/plus_model_select.v` | OSD capability mask and RAM size per machine model |
| `sim/plus run/dandanator_loader_bounds_tests` | fast | `rtl/dandanator/*` | Dandanator loader address bounds |
| `sim/plus run/rom_loader_route_tests` | fast | `rtl/rom_loader_route.v` | ROM download routing against a frozen model of the old Amstrad.sv decoder |
| `sim/plus run/plus_legacy_cart_gate_tests` | fast | `rtl/plus/plus_legacy_cart_gate.v` | Dandanator mapping lifecycle against a later Plus cartridge load |
| `sim/plus run/plus_cartridge_memory_tests` | fast | `rtl/plus/plus_cartridge_memory.v` | Cartridge memory service handshakes, aborts and image replacement |
| `sim/plus run/plus_cartridge_memory_min_tests` | fast | `rtl/plus/plus_cartridge_memory.v` | Same bench at the minimum image size |
| `sim/plus run/sdram_cartridge_tests` | fast | `rtl/sdram.v` `rtl/tape_write_queue.v` | SDRAM slot arbitration and refresh deferral with the cartridge port |
| `sim/plus run/plus_cpr_parser_tests` | fast | `rtl/plus/plus_cpr_parser.v` | CPR RIFF parser, including empty and short chunks |
| `sim/plus run/plus_mmu_tests` | fast | `rtl/plus/plus_mmu.v` | Plus MMU request/acknowledge port and I/O read-write aliasing |
| `sim/plus run/p0_boot_tests` | fast | `rtl/sdram.v` `rtl/plus/plus_cpr_parser.v` `rtl/plus/plus_cartridge_memory.v` `rtl/plus/plus_mmu.v` | CPR download through parser and memory service to CPU cartridge reads |
| `sim/plus run/asic_ga_timing_diff_tests` | fast | `rtl/plus/asic_ga_timing.v` `rtl/GA40010/*.v` `rtl/GA40010/*.sv` | Plus Gate Array timing in lockstep against the classic GA40010 |
| `sim/plus run/p1_video_tests` | fast | `rtl/plus/asic_ga_timing.v` `rtl/plus/asic_video.v` | Plus pixel phase and byte order at the production fetch cadence |
| `sim/plus run/p1_mobo_bench_tests` | fast | `rtl/Amstrad_motherboard.v` `rtl/Amstrad_MMU.v` `rtl/plus/asic_video.v` `rtl/plus/asic_ga_timing.v` | Plus-mode motherboard: register writes reach asic_video, interrupt reaches the CPU and clears |
| `sim/plus run/asic_regs_tests` | fast | `rtl/plus/asic_regs.v` | ASIC page decode, read, write, mirrors and masks |
| `sim/plus run/plus_sprite_ram_tests` | fast | `rtl/plus/plus_sprite_ram.v` | Sprite RAM storage contract used for M10K inference |
| `sim/plus run/asic_pri_tests` | fast | `rtl/plus/asic_ga_timing.v` | Programmable raster interrupt line compare and acknowledge |
| `sim/plus run/asic_sprites_tests` | fast | `rtl/plus/asic_sprites.v` | Sprite engine: attributes, priority, row fetch |
| `sim/plus run/d3_sprites_tests` | fast | `rtl/plus/asic_sprites.v` | First visible sprite row at production cadence |
| `sim/plus run/p4_sprites_regs_tests` | fast | `rtl/plus/asic_regs.v` `rtl/plus/asic_sprites.v` `rtl/plus/plus_sprite_ram.v` | Sprite page storage and row-fetch arbitration between asic_regs and asic_sprites |
| `sim/plus run/asic_dma_tests` | fast | `rtl/plus/asic_dma.v` | DMA sound channel commands and timing |
| `sim/plus run/plus_p8_tests` | fast | `rtl/plus/asic_dma.v` `rtl/plus/asic_regs.v` `rtl/i8255.v` `rtl/plus/plus_sna_*.v` `rtl/plus/plus_fdc_decode.v` | Plus PPI quirks, SNA v3 Plus chunk parsing, FDC model gating, live PPR write events |
| `sim/plus run/b16_load_model_tests` | fast | `rtl/plus/plus_load_model.v` `rtl/plus/plus_sna_apply.v` | CPR/SNA selection before reset release, delayed Main status echo, F1 collision and aborted restore |
| `sim/plus run/p10_boot_tests` | fast | `rtl/Amstrad_motherboard.v` `rtl/Amstrad_MMU.v` `rtl/plus/plus_model_select.v` `rtl/plus/plus_fdc_decode.v` `sim/plus/tv80/*` | Plus cartridge boot on the full motherboard with frame capture (about 50 s) |
| `sim/plus run/p10_input_tests` | fast | `rtl/YM2149.sv` `rtl/hid.sv` `rtl/joydb.sv` `rtl/i8255.v` | PPI to AY to keyboard and joystick input path |
| `sim/plus run/p10_dma_ppi_tests` | fast | `rtl/plus/asic_dma.v` `rtl/plus/asic_ga_timing.v` `rtl/YM2149.sv` `rtl/i8255.v` | DMA, PPI and PSG write concurrency |
| `sim/plus run/p10_dma_mobo_tests` | fast | `rtl/plus/asic_dma.v` `rtl/plus/asic_ga_timing.v` `rtl/YM2149.sv` `rtl/i8255.v` `rtl/Amstrad_motherboard.v` | DMA and PPI concurrency on the full motherboard |
| `sim/plus run/b8_palette_tests` | fast | `rtl/plus/asic_regs.v` `rtl/plus/asic_ga_timing.v` | Plus palette writes through the legacy and ASIC paths |
| `sim/plus run/sna_save_stream_tests` | fast | `rtl/sna_save_stream.v` `rtl/sna_ddr_mux.v` `rtl/sna_cart_mux.v` | SNA save stream to DDR3, including bus stalls |
| `sim/plus b6-video-boundary` | slow | `rtl/crt_filter.v` `rtl/amstrad_video_output.sv` | Sync filter modes on the full motherboard to the output chain; 6 boots (about 1 min) |
| `sim/plus b6-video-boundary-strict` | slow | — | The same with all 18 boots (adds classic type 1 and HSYNC width 14); run it when a change touches ordinary-width filtering or classic type-1 sync |
| `sim/plus b6-dynamic` | slow | `rtl/crt_filter.v` `rtl/amstrad_video_output.sv` | Output chain under CPU-written short, missing, multiple and wide sync |
| `sim/plus b6-plus-layers` | slow | `rtl/plus/asic_video.v` `rtl/plus/asic_sprites.v` `rtl/amstrad_video_output.sv` | Plus scroll and sprites through the colour converter and output chain |
| `sim/plus b8-field` | slow | `rtl/video_interlace.v` | Plus FIELD ownership through the scaler consumer |
| `sim/plus b7-dark-silicon-audit` | slow | `rtl/Amstrad_motherboard.v` | Mutation audit: Plus modules inert in classic mode and classic modules inert in Plus mode |
