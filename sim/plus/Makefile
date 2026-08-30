VERILATOR ?= verilator
OBJ_DIR   := obj_dir

ASIC_TOP      := asic_unlock
ASIC_RTL      := $(abspath ../../rtl/plus/asic_unlock.v)
ASIC_OBJ_DIR  := obj_dir/asic_unlock
ASIC_TEST_SRC := $(abspath asic_unlock_test.cpp)
ASIC_TEST_BIN := $(ASIC_OBJ_DIR)/asic_unlock_tests

ASICVID_TOP      := asic_video
ASICVID_RTL      := $(abspath ../../rtl/plus/asic_video.v)
ASICVID_OBJ_DIR  := obj_dir/asic_video
ASICVID_TEST_SRC := $(abspath asic_video_test.cpp)
ASICVID_TEST_BIN := $(ASICVID_OBJ_DIR)/asic_video_tests

MODEL_TOP      := plus_model_select
MODEL_RTL      := $(abspath ../../rtl/plus/plus_model_select.v)
MODEL_OBJ_DIR  := obj_dir/plus_model_select
MODEL_TEST_SRC := $(abspath plus_model_select_test.cpp)
MODEL_TEST_BIN := $(MODEL_OBJ_DIR)/plus_model_select_tests

DAN_BOUNDS_TOP      := dandanator_loader_bounds
DAN_BOUNDS_RTL      := $(abspath ../../rtl/dandanator/dandanator_loader_bounds.v)
DAN_BOUNDS_OBJ_DIR  := obj_dir/dandanator_loader_bounds
DAN_BOUNDS_TEST_SRC := $(abspath dandanator_loader_bounds_test.cpp)
DAN_BOUNDS_TEST_BIN := $(DAN_BOUNDS_OBJ_DIR)/dandanator_loader_bounds_tests

MEMORY_TOP        := plus_cartridge_memory
MEMORY_RTL        := $(abspath ../../rtl/plus/plus_cartridge_memory.v)
MEMORY_OBJ_DIR    := obj_dir/plus_cartridge_memory
MEMORY_TEST_SRC   := $(abspath plus_cartridge_memory_test.cpp)
MEMORY_TEST_BIN   := $(MEMORY_OBJ_DIR)/plus_cartridge_memory_tests
MEMORY_TEST_BYTES := 20'd32

MEMORY_MIN_OBJ_DIR  := obj_dir/plus_cartridge_memory_min
MEMORY_MIN_TEST_BIN := $(MEMORY_MIN_OBJ_DIR)/plus_cartridge_memory_min_tests

SDRAM_TOP      := sdram_cartridge_test_top
SDRAM_RTL      := $(abspath ../../rtl/sdram.v)
SDRAM_TEST_TOP := $(abspath sdram_cartridge_test_top.v)
SDRAM_OBJ_DIR  := obj_dir/sdram_cartridge
SDRAM_TEST_SRC := $(abspath sdram_cartridge_test.cpp)
SDRAM_TEST_BIN := $(SDRAM_OBJ_DIR)/sdram_cartridge_tests

PARSER_TOP      := plus_cpr_parser
PARSER_RTL      := $(abspath ../../rtl/plus/plus_cpr_parser.v)
PARSER_OBJ_DIR  := obj_dir/plus_cpr_parser
PARSER_TEST_SRC := $(abspath plus_cpr_parser_test.cpp)
PARSER_TEST_BIN := $(PARSER_OBJ_DIR)/plus_cpr_parser_tests

MMU_TOP      := plus_mmu
MMU_RTL      := $(abspath ../../rtl/plus/plus_mmu.v)
MMU_DEPS     := $(abspath ../../rtl/plus/asic_unlock.v)
MMU_OBJ_DIR  := obj_dir/plus_mmu
MMU_TEST_SRC := $(abspath plus_mmu_test.cpp)
MMU_TEST_BIN := $(MMU_OBJ_DIR)/plus_mmu_tests

P0BOOT_TOP      := p0_boot_test_top
P0BOOT_TEST_TOP := $(abspath p0_boot_test_top.v)
P0BOOT_SDRAM    := $(abspath ../../rtl/sdram.v)
P0BOOT_PARSER   := $(abspath ../../rtl/plus/plus_cpr_parser.v)
P0BOOT_MEMORY   := $(abspath ../../rtl/plus/plus_cartridge_memory.v)
P0BOOT_MMU      := $(abspath ../../rtl/plus/plus_mmu.v)
P0BOOT_UNLOCK   := $(abspath ../../rtl/plus/asic_unlock.v)
P0BOOT_OBJ_DIR  := obj_dir/p0_boot
P0BOOT_TEST_SRC := $(abspath p0_boot_test.cpp)
P0BOOT_TEST_BIN := $(P0BOOT_OBJ_DIR)/p0_boot_tests

# Lockstep differential bench: the Plus behavioural Gate Array support path
# against the classic netlist-derived ga40010. Compiled without
# --language 1364-2001 because ga40010.sv uses SystemVerilog constructs, and
# with -UVERILATOR so its simulation-only shadow domain (whose .* instances
# double-drive the sync outputs in pure simulation) is excluded — leaving
# exactly the synthesised production composition as the reference.
GADIFF_TOP      := asic_ga_diff_top
GADIFF_REPLICA  := $(abspath ../../rtl/plus/asic_ga_timing.v)
GADIFF_REF      := $(abspath ../../rtl/GA40010/ga40010.sv) \
                   $(abspath ../../rtl/GA40010/casgen_sync.v) \
                   $(abspath ../../rtl/GA40010/syncgen_sync.v) \
                   $(abspath ../../rtl/GA40010/video.sv) \
                   $(abspath ../../rtl/GA40010/rslatch.v)
GADIFF_TEST_TOP := $(abspath asic_ga_diff_top.v)
GADIFF_OBJ_DIR  := obj_dir/asic_ga_diff
GADIFF_TEST_SRC := $(abspath asic_ga_timing_diff_test.cpp)
GADIFF_TEST_BIN := $(GADIFF_OBJ_DIR)/asic_ga_timing_diff_tests

# P1 motherboard-integration validation: pixel phase / byte order through
# the production fetch cadence (t05h closure).
P1VID_TOP      := p1_video_test_top
P1VID_TOP_SRC  := $(abspath p1_video_test_top.v)
P1VID_RTL      := $(abspath ../../rtl/plus/asic_ga_timing.v) $(abspath ../../rtl/plus/asic_video.v) \
                   $(abspath ../../rtl/CRTC.v) $(abspath ../../rtl/crtc_type0_engine.v) \
                   $(abspath ../../rtl/crtc_type1_engine.v) \
                   $(abspath ../../rtl/GA40010/ga40010.sv) $(abspath ../../rtl/GA40010/casgen_sync.v) \
                   $(abspath ../../rtl/GA40010/syncgen_sync.v) $(abspath ../../rtl/GA40010/video.sv) \
                   $(abspath ../../rtl/GA40010/rslatch.v)
P1VID_OBJ_DIR  := obj_dir/p1_video
P1VID_TEST_SRC := $(abspath p1_pixel_phase_test.cpp)
P1VID_TEST_BIN := $(P1VID_OBJ_DIR)/p1_video_tests

# Static lint/elaboration pass over the whole Amstrad_motherboard hierarchy
# (queued P1 review follow-up: the gap that let two BLOCKING wiring bugs
# reach Quartus). Runs under --language 1364-2001 like every other Plus
# target; children Verilator cannot read in that mode are replaced by
# full-port-list stubs — ga40010/YM2149/hid in motherboard_lint_stubs.v and
# T80pa in t80pa_lint_stub.v (see each file's header for why it is stubbed
# and where its port list comes from). -UVERILATOR matches the production
# ga40010 composition (no simulation-only clk_16 port).
# Warning classes waived below were triaged 2026-08-25:
#   PINCONNECTEMPTY / PINMISSING : intentionally unconnected output pins
#   UNUSEDSIGNAL                 : reserved capability inputs and upstream/
#                                  classic-side taps; the dead plus_phi_en_*
#                                  pair this class surfaced is fixed by the
#                                  phase-enable ownership mux, not waived
#   DECLFILENAME                 : multi-module stub files
#   CASEX / SYNCASYNCNET / WIDTHTRUNC :
#                                  upstream style idioms in files owned by
#                                  other streams
#   PROCASSINIT / IMPLICITSTATIC are deliberately absent: their only
#   sources were fixed (Amstrad_MMU block-local initializer) or are
#   version-dependent, and CI's older Verilator rejects the -Wno names.
# Everything else under -Wall stays fatal — above all IMPLICIT (undeclared
# nets at instance pins, the original review-finding bug class) and UNDRIVEN.
MBOARD_CORE_RTL := $(abspath ../../rtl/Amstrad_motherboard.v) \
	$(abspath ../../rtl/plus/asic_regs.v) \
	$(abspath motherboard_lint_stubs.v) \
	$(abspath ../../rtl/CRTC.v) \
	$(abspath ../../rtl/crtc_type0_engine.v) \
	$(abspath ../../rtl/crtc_type1_engine.v) \
	$(abspath ../../rtl/plus/asic_ga_timing.v) \
	$(abspath ../../rtl/plus/asic_video.v) \
	$(abspath ../../rtl/plus/asic_sprites.v) \
	$(abspath ../../rtl/plus/asic_dma.v) \
	$(abspath ../../rtl/crt_filter.v) \
	$(abspath ../../rtl/Amstrad_MMU.v) \
	$(abspath ../../rtl/i8255.v)

MBOARD_LINT_RTL := $(MBOARD_CORE_RTL) $(abspath t80pa_lint_stub.v)

# plus_mode=1 motherboard-level integration bench: the same hierarchy with a
# scripted fake CPU (t80pa_bench_cpu.v) instead of the lint tie-off, driven
# from p1_mobo_bench_test.cpp. --public-flat-rw exposes the fake CPU's debug
# exports and the asic_video payload taps to C++.
MOBO_TOP      := p1_mobo_bench_top
MOBO_TEST_TOP := $(abspath p1_mobo_bench_top.v)
MOBO_RTL      := $(MBOARD_CORE_RTL) \
	$(abspath ../../rtl/GA40010/casgen_sync.v) \
	$(abspath ../../rtl/GA40010/syncgen_sync.v)
MOBO_CPU      := $(abspath t80pa_bench_cpu.v)
MOBO_OBJ_DIR  := obj_dir/p1_mobo_bench
MOBO_TEST_SRC := $(abspath p1_mobo_bench_test.cpp)
MOBO_TEST_BIN := $(MOBO_OBJ_DIR)/p1_mobo_bench_tests

# P2 ASIC register page leaf (exhaustive decode/mask/mirror/open-bus).
AREGS_TOP      := asic_regs
AREGS_RTL      := $(abspath ../../rtl/plus/asic_regs.v)
AREGS_OBJ_DIR  := obj_dir/asic_regs
AREGS_TEST_SRC := $(abspath asic_regs_test.cpp)
AREGS_TEST_BIN := $(AREGS_OBJ_DIR)/asic_regs_tests

# P3 programmable raster interrupt directed vectors (exact-cycle).
PRI_TOP      := asic_ga_timing
PRI_RTL      := $(abspath ../../rtl/plus/asic_ga_timing.v)
PRI_OBJ_DIR  := obj_dir/asic_pri
PRI_TEST_SRC := $(abspath asic_pri_test.cpp)
PRI_TEST_BIN := $(PRI_OBJ_DIR)/asic_pri_tests

# P4 sprite engine directed vectors (compare/mag/priority/blanking).
SPRITES_TOP      := asic_sprites
SPRITES_RTL      := $(abspath ../../rtl/plus/asic_sprites.v)
SPRITES_OBJ_DIR  := obj_dir/asic_sprites
SPRITES_TEST_SRC := $(abspath asic_sprites_test.cpp)
SPRITES_TEST_BIN := $(SPRITES_OBJ_DIR)/asic_sprites_tests

# P4 production seam: ASIC-page register storage and sprite row-fetch
# arbitration are connected directly to the sprite engine.
SPRREGS_TOP      := p4_sprites_regs_test_top
SPRREGS_TOP_SRC  := $(abspath p4_sprites_regs_test_top.v)
SPRREGS_RTL      := $(abspath ../../rtl/plus/asic_regs.v) \
	$(abspath ../../rtl/plus/asic_sprites.v)
SPRREGS_OBJ_DIR  := obj_dir/p4_sprites_regs
SPRREGS_TEST_SRC := $(abspath p4_sprites_regs_test.cpp)
SPRREGS_TEST_BIN := $(SPRREGS_OBJ_DIR)/p4_sprites_regs_tests

# P7 3-channel DMA sound engine leaf
DMA_TOP      := asic_dma
DMA_RTL      := $(abspath ../../rtl/plus/asic_dma.v)
DMA_OBJ_DIR  := obj_dir/asic_dma
DMA_TEST_SRC := $(abspath asic_dma_test.cpp)
DMA_TEST_BIN := $(DMA_OBJ_DIR)/asic_dma_tests

# P8 platform polish and SNA v3 CPC+ chunk parser
P8_OBJ_DIR  := obj_dir/plus_p8
P8_TEST_SRC := $(abspath plus_p8_test.cpp)
P8_TEST_BIN := $(P8_OBJ_DIR)/plus_p8_tests
P8_RTL      := $(abspath plus_p8_test_top.v) $(abspath ../../rtl/i8255.v) $(abspath ../../rtl/plus/plus_sna_parser.v) $(abspath ../../rtl/plus/plus_model_select.v) $(abspath ../../rtl/plus/plus_fdc_decode.v) $(abspath ../../rtl/plus/asic_regs.v) $(abspath ../../rtl/plus/plus_mmu.v) $(abspath ../../rtl/plus/asic_unlock.v)

# P10a real-T80 production CPR boot harness
P10BOOT_TOP      := p10_boot_test_top
P10BOOT_TEST_TOP := $(abspath p10_boot_test_top.v)
P10BOOT_TV80     := $(abspath tv80/t80pa.v) $(abspath tv80/tv80_core.v) $(abspath tv80/tv80_mcode.v) \
                    $(abspath tv80/tv80_alu.v) $(abspath tv80/tv80_reg.v)
P10BOOT_RTL      := $(MBOARD_CORE_RTL) $(P10BOOT_TV80) \
                    $(abspath ../../rtl/plus/plus_cpr_parser.v) \
                    $(abspath ../../rtl/plus/plus_cartridge_memory.v) \
                    $(abspath ../../rtl/plus/plus_mmu.v) \
                    $(abspath ../../rtl/plus/asic_unlock.v) \
                    $(abspath ../../rtl/plus/plus_model_select.v) \
                    $(abspath ../../rtl/plus/plus_fdc_decode.v) \
                    $(abspath ../../rtl/sdram.v) \
                    $(abspath ../../rtl/GA40010/casgen_sync.v) \
                    $(abspath ../../rtl/GA40010/syncgen_sync.v)
P10BOOT_OBJ_DIR  := obj_dir/p10_boot
P10BOOT_TEST_SRC := $(abspath p10_boot_test.cpp)
P10BOOT_TEST_BIN := $(P10BOOT_OBJ_DIR)/p10_boot_tests

# P10b production input-path fixture: the minimal exact PPI/YM2149/hid/joydb
# slice from the motherboard, with a scripted fake Z80 and a 6128+ capability
# set.  The real Plus GA timing module owns CPU phases/READY and YM CCLK_EN_P.
# This target intentionally uses mixed Verilog/SystemVerilog parsing so the
# real YM2149 and HID implementations participate.
P10INPUT_TOP      := p10_input_test_top
P10INPUT_TOP_SRC  := $(abspath p10_input_test_top.v)
P10INPUT_CPU      := $(abspath t80pa_input_bench_cpu.v)
P10INPUT_RTL      := $(abspath ../../rtl/plus/asic_ga_timing.v) \
	$(abspath ../../rtl/i8255.v) \
	$(abspath ../../rtl/YM2149.sv) \
	$(abspath ../../rtl/hid.sv) \
	$(abspath ../../rtl/joydb.sv)
P10INPUT_OBJ_DIR  := obj_dir/p10_input
P10INPUT_TEST_SRC := $(abspath p10_input_test.cpp)
P10INPUT_TEST_BIN := $(P10INPUT_OBJ_DIR)/p10_input_tests

# P10e production-shaped DMA/PPI/PSG/input concurrency fixture.
P10DMA_TOP      := p10_dma_ppi_test_top
P10DMA_TOP_SRC  := $(abspath p10_dma_ppi_test_top.v)
P10DMA_RTL      := $(abspath ../../rtl/plus/asic_dma.v) \
	$(abspath ../../rtl/plus/asic_ga_timing.v) \
	$(abspath ../../rtl/i8255.v) \
	$(abspath ../../rtl/YM2149.sv) \
	$(abspath ../../rtl/hid.sv) \
	$(abspath ../../rtl/joydb.sv)
P10DMA_OBJ_DIR  := obj_dir/p10_dma_ppi
P10DMA_TEST_SRC := $(abspath p10_dma_ppi_test.cpp)
P10DMA_TEST_BIN := $(P10DMA_OBJ_DIR)/p10_dma_ppi_tests

# P10e full production-motherboard DMA/PPI/PSG concurrency fixture.
P10DMAMOBO_TOP      := p10_dma_mobo_test_top
P10DMAMOBO_TOP_SRC  := $(abspath p10_dma_mobo_test_top.v)
P10DMAMOBO_CPU      := $(abspath t80pa_dma_mobo_cpu.v)
P10DMAMOBO_RTL      := $(abspath ../../rtl/Amstrad_motherboard.v) \
	$(abspath ../../rtl/plus/asic_regs.v) \
	$(abspath ../../rtl/plus/asic_ga_timing.v) \
	$(abspath ../../rtl/plus/asic_video.v) \
	$(abspath ../../rtl/plus/asic_sprites.v) \
	$(abspath ../../rtl/plus/asic_dma.v) \
	$(abspath ../../rtl/crt_filter.v) \
	$(abspath ../../rtl/CRTC.v) \
	$(abspath ../../rtl/crtc_type0_engine.v) \
	$(abspath ../../rtl/crtc_type1_engine.v) \
	$(abspath ../../rtl/Amstrad_MMU.v) \
	$(abspath ../../rtl/i8255.v) \
	$(abspath ../../rtl/YM2149.sv) \
	$(abspath ../../rtl/hid.sv) \
	$(abspath motherboard_lint_stubs.v)
P10DMAMOBO_OBJ_DIR  := obj_dir/p10_dma_mobo
P10DMAMOBO_TEST_SRC := $(abspath p10_dma_mobo_test.cpp)
P10DMAMOBO_TEST_BIN := $(P10DMAMOBO_OBJ_DIR)/p10_dma_mobo_tests

TEST_BINS := $(ASIC_TEST_BIN) $(ASICVID_TEST_BIN) $(MODEL_TEST_BIN) $(DAN_BOUNDS_TEST_BIN) $(MEMORY_TEST_BIN) \
	$(MEMORY_MIN_TEST_BIN) $(SDRAM_TEST_BIN) $(PARSER_TEST_BIN) $(MMU_TEST_BIN) $(P0BOOT_TEST_BIN) \
	$(GADIFF_TEST_BIN) $(P1VID_TEST_BIN) $(MOBO_TEST_BIN) \
	$(AREGS_TEST_BIN) $(PRI_TEST_BIN) $(SPRITES_TEST_BIN) $(SPRREGS_TEST_BIN) $(DMA_TEST_BIN) $(P8_TEST_BIN) $(P10BOOT_TEST_BIN)

TEST_BINS += $(P10INPUT_TEST_BIN) $(P10DMA_TEST_BIN) $(P10DMAMOBO_TEST_BIN)

VERILATOR_FLAGS := --cc --exe --build --language 1364-2001
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all build test lint motherboard-lint p4-sprites-regs clean \
	p10-dma-ppi-bench p10-dma-ppi-lint p10-dma-mobo-bench p10-dma-mobo-lint

all: test

build: $(TEST_BINS)

# Convenience alias; the bench itself runs in the default gate.
p1-video-bench: $(P1VID_TEST_BIN)
	./$(P1VID_TEST_BIN)

p10-input-bench: $(P10INPUT_TEST_BIN)
	./$(P10INPUT_TEST_BIN)

p10-dma-ppi-bench: $(P10DMA_TEST_BIN)
	./$(P10DMA_TEST_BIN)

p10-dma-ppi-lint:
	$(VERILATOR) --lint-only -UVERILATOR --public-flat-rw \
		--top-module $(P10DMA_TOP) -Wall -Wno-fatal -Wno-PROCASSWIRE \
		$(P10DMA_TOP_SRC) $(P10DMA_RTL)

p10-dma-mobo-bench: $(P10DMAMOBO_TEST_BIN)
	./$(P10DMAMOBO_TEST_BIN)

p10-dma-mobo-lint:
	$(VERILATOR) --lint-only +1364-2001ext+.v +1800-2017ext+.sv -UVERILATOR -DP10_DMA_MOBO_REAL_IO --public-flat-rw \
		--top-module $(P10DMAMOBO_TOP) -Wall -Wno-fatal -Wno-PROCASSWIRE \
		$(P10DMAMOBO_TOP_SRC) $(P10DMAMOBO_CPU) $(P10DMAMOBO_RTL)

p4-sprites-regs: $(SPRREGS_TEST_BIN)
	./$(SPRREGS_TEST_BIN)

test: $(TEST_BINS)
	./$(ASIC_TEST_BIN)
	./$(ASICVID_TEST_BIN)
	./$(MODEL_TEST_BIN)
	./$(DAN_BOUNDS_TEST_BIN)
	./$(MEMORY_TEST_BIN)
	./$(MEMORY_MIN_TEST_BIN)
	./$(SDRAM_TEST_BIN)
	./$(PARSER_TEST_BIN)
	./$(MMU_TEST_BIN)
	./$(P0BOOT_TEST_BIN)
	./$(GADIFF_TEST_BIN)
	./$(P1VID_TEST_BIN)
	./$(MOBO_TEST_BIN)
	./$(AREGS_TEST_BIN)
	./$(PRI_TEST_BIN)
	./$(SPRITES_TEST_BIN)
	./$(SPRREGS_TEST_BIN)
	./$(DMA_TEST_BIN)
	./$(P8_TEST_BIN)
	./$(P10BOOT_TEST_BIN)
	./$(P10INPUT_TEST_BIN)
	./$(P10DMA_TEST_BIN)
	./$(P10DMAMOBO_TEST_BIN)

$(P8_TEST_BIN): $(P8_RTL) $(P8_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module plus_p8_test_top -Wno-CASEX \
		--Mdir $(P8_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(P8_TEST_BIN)) $(P8_RTL) $(P8_TEST_SRC)

$(ASIC_TEST_BIN): $(ASIC_RTL) $(ASIC_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(ASIC_TOP) \
		--Mdir $(ASIC_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(ASIC_TEST_BIN)) $(ASIC_RTL) $(ASIC_TEST_SRC)

$(ASICVID_TEST_BIN): $(ASICVID_RTL) $(ASICVID_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(ASICVID_TOP) \
		--Mdir $(ASICVID_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(ASICVID_TEST_BIN)) $(ASICVID_RTL) $(ASICVID_TEST_SRC)

$(MODEL_TEST_BIN): $(MODEL_RTL) $(MODEL_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(MODEL_TOP) \
		--Mdir $(MODEL_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(MODEL_TEST_BIN)) $(MODEL_RTL) $(MODEL_TEST_SRC)

$(DAN_BOUNDS_TEST_BIN): $(DAN_BOUNDS_RTL) $(DAN_BOUNDS_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(DAN_BOUNDS_TOP) \
		--Mdir $(DAN_BOUNDS_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(DAN_BOUNDS_TEST_BIN)) $(DAN_BOUNDS_RTL) $(DAN_BOUNDS_TEST_SRC)

$(MEMORY_TEST_BIN): $(MEMORY_RTL) $(MEMORY_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(MEMORY_TOP) \
		"-GCLEAR_BYTES=$(MEMORY_TEST_BYTES)" --Mdir $(MEMORY_OBJ_DIR) \
		-CFLAGS "$(CXXFLAGS)" -o $(notdir $(MEMORY_TEST_BIN)) \
		$(MEMORY_RTL) $(MEMORY_TEST_SRC)

$(MEMORY_MIN_TEST_BIN): $(MEMORY_RTL) $(MEMORY_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(MEMORY_TOP) \
		"-GCLEAR_BYTES=20'd1" --Mdir $(MEMORY_MIN_OBJ_DIR) \
		-CFLAGS "$(CXXFLAGS) -DCLEAR_TEST_BYTES=1" \
		-o $(notdir $(MEMORY_MIN_TEST_BIN)) $(MEMORY_RTL) $(MEMORY_TEST_SRC)

$(SDRAM_TEST_BIN): $(SDRAM_RTL) $(MEMORY_RTL) $(SDRAM_TEST_TOP) $(SDRAM_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(SDRAM_TOP) \
		--Mdir $(SDRAM_OBJ_DIR) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(SDRAM_TEST_BIN)) $(SDRAM_TEST_TOP) $(SDRAM_RTL) \
		$(MEMORY_RTL) $(SDRAM_TEST_SRC)

$(PARSER_TEST_BIN): $(PARSER_RTL) $(PARSER_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(PARSER_TOP) \
		--Mdir $(PARSER_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(PARSER_TEST_BIN)) $(PARSER_RTL) $(PARSER_TEST_SRC)

$(MMU_TEST_BIN): $(MMU_RTL) $(MMU_DEPS) $(MMU_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(MMU_TOP) \
		--Mdir $(MMU_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(MMU_TEST_BIN)) $(MMU_RTL) $(MMU_DEPS) $(MMU_TEST_SRC)

$(P0BOOT_TEST_BIN): $(P0BOOT_TEST_TOP) $(P0BOOT_SDRAM) $(P0BOOT_PARSER) $(P0BOOT_MEMORY) $(P0BOOT_MMU) $(P0BOOT_UNLOCK) $(P0BOOT_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(P0BOOT_TOP) \
		--Mdir $(P0BOOT_OBJ_DIR) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(P0BOOT_TEST_BIN)) $(P0BOOT_TEST_TOP) $(P0BOOT_PARSER) \
		$(P0BOOT_MEMORY) $(P0BOOT_MMU) $(P0BOOT_UNLOCK) $(P0BOOT_SDRAM) $(P0BOOT_TEST_SRC)

$(P1VID_TEST_BIN): $(P1VID_TOP_SRC) $(P1VID_RTL) $(P1VID_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) -UVERILATOR --top-module $(P1VID_TOP) \
		--Mdir $(P1VID_OBJ_DIR) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(P1VID_TEST_BIN)) $(P1VID_TOP_SRC) $(P1VID_RTL) $(P1VID_TEST_SRC)

$(GADIFF_TEST_BIN): $(GADIFF_TEST_TOP) $(GADIFF_REPLICA) $(GADIFF_REF) $(GADIFF_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) --cc --exe --build -UVERILATOR --top-module $(GADIFF_TOP) \
		--Mdir $(GADIFF_OBJ_DIR) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(GADIFF_TEST_BIN)) $(GADIFF_TEST_TOP) $(GADIFF_REPLICA) \
		$(GADIFF_REF) $(GADIFF_TEST_SRC)

$(PRI_TEST_BIN): $(PRI_RTL) $(PRI_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(PRI_TOP) \
		--Mdir $(PRI_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(PRI_TEST_BIN)) $(PRI_RTL) $(PRI_TEST_SRC)

$(SPRITES_TEST_BIN): $(SPRITES_RTL) $(SPRITES_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(SPRITES_TOP) \
		--Mdir $(SPRITES_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(SPRITES_TEST_BIN)) $(SPRITES_RTL) $(SPRITES_TEST_SRC)

$(SPRREGS_TEST_BIN): $(SPRREGS_TOP_SRC) $(SPRREGS_RTL) $(SPRREGS_TEST_SRC) Makefile | $(SPRREGS_OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(SPRREGS_TOP) \
		--Mdir $(SPRREGS_OBJ_DIR) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(SPRREGS_TEST_BIN)) $(SPRREGS_TOP_SRC) $(SPRREGS_RTL) \
		$(SPRREGS_TEST_SRC)

$(DMA_TEST_BIN): $(DMA_RTL) $(DMA_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(DMA_TOP) \
		--Mdir $(DMA_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(DMA_TEST_BIN)) $(DMA_RTL) $(DMA_TEST_SRC)

$(AREGS_TEST_BIN): $(AREGS_RTL) $(AREGS_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(AREGS_TOP) \
		--Mdir $(AREGS_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(AREGS_TEST_BIN)) $(AREGS_RTL) $(AREGS_TEST_SRC)

$(MOBO_TEST_BIN): $(MOBO_TEST_TOP) $(MOBO_RTL) $(MOBO_CPU) $(MOBO_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) -UVERILATOR --public-flat-rw \
		--top-module $(MOBO_TOP) \
		--Mdir $(MOBO_OBJ_DIR) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(MOBO_TEST_BIN)) $(MOBO_TEST_TOP) $(MOBO_RTL) \
		$(MOBO_CPU) $(MOBO_TEST_SRC)

$(P10BOOT_TEST_BIN): $(P10BOOT_TEST_TOP) $(P10BOOT_RTL) $(P10BOOT_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) -UVERILATOR \
		--top-module $(P10BOOT_TOP) \
		--Mdir $(P10BOOT_OBJ_DIR) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(P10BOOT_TEST_BIN)) $(P10BOOT_TEST_TOP) $(P10BOOT_RTL) \
		$(P10BOOT_TEST_SRC)

$(P10INPUT_TEST_BIN): $(P10INPUT_TOP_SRC) $(P10INPUT_CPU) $(P10INPUT_RTL) $(P10INPUT_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) --cc --exe --build -UVERILATOR --public-flat-rw -Wno-PROCASSWIRE \
		--top-module $(P10INPUT_TOP) \
		--Mdir $(P10INPUT_OBJ_DIR) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(P10INPUT_TEST_BIN)) $(P10INPUT_TOP_SRC) $(P10INPUT_CPU) \
		$(P10INPUT_RTL) $(P10INPUT_TEST_SRC)

$(P10DMA_TEST_BIN): $(P10DMA_TOP_SRC) $(P10DMA_RTL) $(P10DMA_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) --cc --exe --build -UVERILATOR --public-flat-rw -Wno-PROCASSWIRE \
		--top-module $(P10DMA_TOP) \
		--Mdir $(P10DMA_OBJ_DIR) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(P10DMA_TEST_BIN)) $(P10DMA_TOP_SRC) $(P10DMA_RTL) \
		$(P10DMA_TEST_SRC)

$(P10DMAMOBO_TEST_BIN): $(P10DMAMOBO_TOP_SRC) $(P10DMAMOBO_CPU) $(P10DMAMOBO_RTL) $(P10DMAMOBO_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) --cc --exe --build +1364-2001ext+.v +1800-2017ext+.sv -UVERILATOR -DP10_DMA_MOBO_REAL_IO \
		--public-flat-rw --top-module $(P10DMAMOBO_TOP) \
		--Mdir $(P10DMAMOBO_OBJ_DIR) -Wall -Wno-fatal -Wno-PROCASSWIRE \
		-CFLAGS "$(CXXFLAGS)" -o $(notdir $(P10DMAMOBO_TEST_BIN)) \
		$(P10DMAMOBO_TOP_SRC) $(P10DMAMOBO_CPU) $(P10DMAMOBO_RTL) \
		$(P10DMAMOBO_TEST_SRC)

$(OBJ_DIR):
	mkdir -p -- $@

$(SPRREGS_OBJ_DIR): | $(OBJ_DIR)
	mkdir -p -- $@

# Standalone form of the motherboard hierarchy lint that `lint` runs first.
motherboard-lint:
	$(VERILATOR) --lint-only --language 1364-2001 -UVERILATOR \
		--top-module Amstrad_motherboard -Wall \
		-Wno-PINCONNECTEMPTY -Wno-PINMISSING -Wno-UNUSEDSIGNAL \
		-Wno-DECLFILENAME -Wno-CASEX \
		-Wno-SYNCASYNCNET -Wno-WIDTHTRUNC \
		$(MBOARD_LINT_RTL)

lint: motherboard-lint
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(ASIC_TOP) -Wall $(ASIC_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(AREGS_TOP) -Wall $(AREGS_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(ASICVID_TOP) -Wall $(ASICVID_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(SPRITES_TOP) -Wall $(SPRITES_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 -UVERILATOR \
		--top-module $(SPRREGS_TOP) -Wall -Wno-fatal \
		$(SPRREGS_TOP_SRC) $(SPRREGS_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(DMA_TOP) -Wall $(DMA_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(MODEL_TOP) -Wall $(MODEL_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(DAN_BOUNDS_TOP) -Wall $(DAN_BOUNDS_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(MEMORY_TOP) -Wall $(MEMORY_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(MEMORY_TOP) -Wall "-GCLEAR_BYTES=20'd1" $(MEMORY_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(SDRAM_TOP) -Wall -Wno-fatal $(SDRAM_TEST_TOP) $(SDRAM_RTL) \
		$(MEMORY_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(PARSER_TOP) -Wall $(PARSER_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(MMU_TOP) -Wall $(MMU_RTL) $(MMU_DEPS)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(P0BOOT_TOP) -Wall -Wno-fatal $(P0BOOT_TEST_TOP) $(P0BOOT_PARSER) \
		$(P0BOOT_MEMORY) $(P0BOOT_MMU) $(P0BOOT_UNLOCK) $(P0BOOT_SDRAM)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module asic_ga_timing -Wall $(GADIFF_REPLICA)
	$(VERILATOR) --lint-only -UVERILATOR \
		--top-module $(GADIFF_TOP) -Wall -Wno-fatal $(GADIFF_TEST_TOP) $(GADIFF_REPLICA) $(GADIFF_REF)
	$(VERILATOR) --lint-only --language 1364-2001 -UVERILATOR \
		--top-module $(P1VID_TOP) -Wall -Wno-fatal $(P1VID_TOP_SRC) $(P1VID_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module plus_sna_parser -Wall $(abspath ../../rtl/plus/plus_sna_parser.v)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module i8255 -Wall -Wno-CASEX $(abspath ../../rtl/i8255.v)
	$(VERILATOR) --lint-only -UVERILATOR --public-flat-rw \
		--top-module $(P10INPUT_TOP) -Wall -Wno-fatal -Wno-PROCASSWIRE \
		$(P10INPUT_TOP_SRC) $(P10INPUT_CPU) $(P10INPUT_RTL)
	$(VERILATOR) --lint-only -UVERILATOR --public-flat-rw \
		--top-module $(P10DMA_TOP) -Wall -Wno-fatal -Wno-PROCASSWIRE \
		$(P10DMA_TOP_SRC) $(P10DMA_RTL)
	$(VERILATOR) --lint-only +1364-2001ext+.v +1800-2017ext+.sv -UVERILATOR -DP10_DMA_MOBO_REAL_IO --public-flat-rw \
		--top-module $(P10DMAMOBO_TOP) -Wall -Wno-fatal -Wno-PROCASSWIRE \
		$(P10DMAMOBO_TOP_SRC) $(P10DMAMOBO_CPU) $(P10DMAMOBO_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module plus_fdc_decode -Wall $(abspath ../../rtl/plus/plus_fdc_decode.v)
	$(VERILATOR) --lint-only --language 1364-2001 -UVERILATOR \
		--top-module $(P10BOOT_TOP) -Wall -Wno-fatal \
		-Wno-PINCONNECTEMPTY -Wno-PINMISSING -Wno-UNUSEDSIGNAL \
		-Wno-DECLFILENAME -Wno-CASEX \
		-Wno-SYNCASYNCNET -Wno-WIDTHTRUNC \
		$(P10BOOT_TEST_TOP) $(P10BOOT_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 -UVERILATOR \
		--top-module Amstrad_motherboard -Wall \
		-Wno-PINCONNECTEMPTY -Wno-PINMISSING -Wno-UNUSEDSIGNAL \
		-Wno-DECLFILENAME -Wno-CASEX \
		-Wno-SYNCASYNCNET -Wno-WIDTHTRUNC \
		$(MBOARD_LINT_RTL)

clean:
	$(RM) -r -- $(OBJ_DIR)
