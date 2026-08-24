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

TEST_BINS := $(ASIC_TEST_BIN) $(ASICVID_TEST_BIN) $(MODEL_TEST_BIN) $(DAN_BOUNDS_TEST_BIN) $(MEMORY_TEST_BIN) \
	$(MEMORY_MIN_TEST_BIN) $(SDRAM_TEST_BIN) $(PARSER_TEST_BIN) $(MMU_TEST_BIN) $(P0BOOT_TEST_BIN) \
	$(GADIFF_TEST_BIN)
# p1_video_tests builds but is deliberately NOT in the default gate yet:
# its stimulus/sampling calibration is unfinished (see docs/current-status.md,
# P1 pixel-phase item). Run it explicitly via: make p1-video-bench
OPTIN_BINS := $(P1VID_TEST_BIN)

VERILATOR_FLAGS := --cc --exe --build --language 1364-2001
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all build test lint clean

all: test

build: $(TEST_BINS)

# Opt-in benches under active development; not part of `make -C sim`.
p1-video-bench: $(P1VID_TEST_BIN)
	./$(P1VID_TEST_BIN)

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

$(OBJ_DIR):
	mkdir -p -- $@

lint:
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(ASIC_TOP) -Wall $(ASIC_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(ASICVID_TOP) -Wall $(ASICVID_RTL)
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

clean:
	$(RM) -r -- $(OBJ_DIR)
