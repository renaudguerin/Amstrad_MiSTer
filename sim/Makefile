VERILATOR ?= verilator

TOP       := CRTC
RTL       := ../rtl/CRTC.v ../rtl/crtc_type0_engine.v ../rtl/crtc_type1_engine.v
OBJ_DIR   := obj_dir
TEST_SRC  := sim_main.cpp
TEST_BIN  := $(OBJ_DIR)/crtc_tests

CRTF_TOP      := crt_filter_blank_test_top
CRTF_TOP_SRC  := crt_filter_blank_test_top.v
CRTF_RTL      := ../rtl/crt_filter.v
CRTF_OBJ_DIR  := $(OBJ_DIR)/crt_filter_blank
CRTF_TEST_SRC := crt_filter_blank_test.cpp
CRTF_TEST_BIN := $(CRTF_OBJ_DIR)/crt_filter_blank_tests

VERILATOR_FLAGS := --cc --exe --build --trace \
	--top-module $(TOP) --Mdir $(OBJ_DIR) -Wno-fatal
CXXFLAGS ?= -std=c++17 -O2

VIDEO_COLOR_SRC := plus/video_color_test.sv ../rtl/amstrad_video_color.sv \
    ../rtl/color_mix.sv ../sys/gamma_corr.sv
VIDEO_COLOR_BIN := $(OBJ_DIR)/video_color/Vvideo_color_test

# B6: actual core output chain, including the production mixer and crop state.
VIDEO_OUTPUT_SRC := ../rtl/amstrad_video_output.sv ../rtl/amstrad_video_color.sv \
    ../rtl/color_mix.sv ../rtl/video_interlace.v ../sys/video_mixer.sv \
    ../sys/gamma_corr.sv ../sys/video_freezer.sv ../sys/scandoubler.v \
    ../sys/hq2x.sv ../sys/video_freak.sv ../sys/math.sv
VIDEO_OUTPUT_BIN := $(OBJ_DIR)/video_output/Vamstrad_video_output


.PHONY: all build test lint soak clean crt-filter-blank-build crt-filter-blank-test

all: test

build: $(TEST_BIN) crt-filter-blank-build $(VIDEO_COLOR_BIN) $(VIDEO_OUTPUT_BIN) $(SSM_BIN)
	$(MAKE) -C plus build
	$(MAKE) -C ../rtl/GA40010 all
	$(MAKE) -C ../rtl/u765 build

crt-filter-blank-build: $(CRTF_TEST_BIN)

crt-filter-blank-test: $(CRTF_TEST_BIN)
	./$(CRTF_TEST_BIN)

test: $(TEST_BIN) crt-filter-blank-test crtc-cpu-phase-test video-color-test video-output-test ssm-marker-test
	./$(TEST_BIN)
	$(MAKE) -C plus test
	$(MAKE) -C ../rtl/GA40010 test
	$(MAKE) -C ../rtl/u765 test

# Randomized equivalence soak (sim/README.md). Override the golden hash to
# check one, e.g.: make -C sim soak SOAK_EXPECT=1a2b3c4d5e6f7081
SOAK_EXPECT ?=

soak: $(TEST_BIN)
	./$(TEST_BIN) --soak $(SOAK_EXPECT)

$(TEST_BIN): $(RTL) $(TEST_SRC) Makefile
	$(VERILATOR) $(VERILATOR_FLAGS) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(TEST_BIN)) $(RTL) $(TEST_SRC)

$(CRTF_TEST_BIN): $(CRTF_TOP_SRC) $(CRTF_RTL) $(CRTF_TEST_SRC) Makefile
	$(VERILATOR) --cc --exe --build --trace \
		--top-module $(CRTF_TOP) --Mdir $(CRTF_OBJ_DIR) -Wno-fatal \
		-CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(CRTF_TEST_BIN)) $(CRTF_TOP_SRC) $(CRTF_RTL) $(CRTF_TEST_SRC)

# B8-6: production colour/CE selector plus gamma, without the vendor scaler.
.PHONY: video-color-test
video-color-test: $(VIDEO_COLOR_BIN)
	./$(VIDEO_COLOR_BIN)

$(VIDEO_COLOR_BIN): $(VIDEO_COLOR_SRC) Makefile
	$(VERILATOR) --binary --timing --top-module video_color_test \
		--Mdir $(OBJ_DIR)/video_color -Wno-fatal $(VIDEO_COLOR_SRC)

lint:
	$(VERILATOR) --lint-only --top-module ssm_marker -Wall -Wno-fatal -Wno-PROCASSINIT $(SSM_RTL)
	$(VERILATOR) --lint-only --top-module amstrad_video_output \
		-Wall -Wno-fatal -Wno-PROCASSWIRE $(VIDEO_OUTPUT_SRC)
	$(VERILATOR) --lint-only --top-module crtc_cpu_phase_top -UVERILATOR -Wall -Wno-fatal $(CPU_PHASE_RTL)
	$(VERILATOR) --lint-only --timing --top-module video_color_test \
		-Wall -Wno-fatal $(VIDEO_COLOR_SRC)
	$(VERILATOR) --lint-only --top-module $(TOP) -Wall -Wno-fatal $(RTL)
	$(VERILATOR) --lint-only --top-module $(CRTF_TOP) -Wall -Wno-fatal $(CRTF_TOP_SRC) $(CRTF_RTL)
	$(MAKE) -C plus lint
	$(MAKE) -C ../rtl/GA40010 lint
	$(MAKE) -C ../rtl/u765 lint

clean:
	$(RM) -r -- $(OBJ_DIR)
	$(MAKE) -C plus clean
	$(MAKE) -C ../rtl/GA40010 clean
	$(MAKE) -C ../rtl/u765 clean

# Real GA divider/enables with post-edge CPU bus launches (B8-1).
CPU_PHASE_RTL := crtc_cpu_phase_top.sv $(RTL) $(addprefix ../rtl/GA40010/,ga40010.sv syncgen.v syncgen_sync.v casgen.v casgen_sync.v video.sv rslatch.v)
.PHONY: crtc-cpu-phase-test
crtc-cpu-phase-test: $(OBJ_DIR)/cpu_phase/crtc_cpu_phase_tests
	./$<
$(OBJ_DIR)/cpu_phase/crtc_cpu_phase_tests: $(CPU_PHASE_RTL) crtc_cpu_phase_test.cpp Makefile
	mkdir -p $(OBJ_DIR)/cpu_phase
	$(VERILATOR) --cc --exe --build --top-module crtc_cpu_phase_top -UVERILATOR \
		--Mdir $(OBJ_DIR)/cpu_phase -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o crtc_cpu_phase_tests $(CPU_PHASE_RTL) crtc_cpu_phase_test.cpp

# Bounded production-T80 executed-instruction validation harness (B8-1).
GHDL ?= ghdl
T80_VHDL_SRC := $(addprefix ../rtl/T80/,T80_Pack.vhd T80_ALU.vhd T80_MCode.vhd T80_Reg.vhd T80.vhd T80pa.vhd)
T80_NETLIST  := $(OBJ_DIR)/t80/T80pa.v

.DELETE_ON_ERROR:

.PHONY: t80-netlist
t80-netlist: $(T80_NETLIST)

$(T80_NETLIST): $(T80_VHDL_SRC)
	mkdir -p $(OBJ_DIR)/t80
	$(GHDL) --synth --std=08 -fsynopsys --workdir=$(OBJ_DIR)/t80 --out=verilog $(T80_VHDL_SRC) -e T80pa > $@.tmp && mv $@.tmp $@

# T80 native VHDL vs translated Verilog trace equivalence test
T80_TRACE_TB_SRC   := t80_trace_tb.vhd
T80_VHDL_TRACE_LOG := $(OBJ_DIR)/t80/vhdl_trace.log
T80_TRACE_TEST_BIN := $(OBJ_DIR)/t80_verilog/t80_trace_tests

.PHONY: t80-trace-test
t80-trace-test: $(T80_TRACE_TEST_BIN) $(T80_VHDL_TRACE_LOG)
	$(T80_TRACE_TEST_BIN) $(T80_VHDL_TRACE_LOG)

$(T80_VHDL_TRACE_LOG): $(T80_TRACE_TB_SRC) $(T80_VHDL_SRC)
	mkdir -p $(OBJ_DIR)/t80
	$(GHDL) -a --std=08 -fsynopsys --workdir=$(OBJ_DIR)/t80 $(T80_VHDL_SRC) $(T80_TRACE_TB_SRC)
	$(GHDL) -e --std=08 -fsynopsys --workdir=$(OBJ_DIR)/t80 -o $(OBJ_DIR)/t80/t80_trace_tb t80_trace_tb
	$(OBJ_DIR)/t80/t80_trace_tb > $@.tmp && mv $@.tmp $@

$(T80_TRACE_TEST_BIN): $(T80_NETLIST) t80_trace_test.cpp Makefile
	mkdir -p $(OBJ_DIR)/t80_verilog
	$(VERILATOR) --cc --exe --build --top-module T80pa \
		--Mdir $(OBJ_DIR)/t80_verilog -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o t80_trace_tests $(T80_NETLIST) t80_trace_test.cpp

# Executed-instruction validation with production T80pa, real GA divider/enables/WAIT, and CRTC
CRTC_T80_RTL := crtc_t80_top.sv $(T80_NETLIST) $(RTL) $(addprefix ../rtl/GA40010/,ga40010.sv syncgen.v syncgen_sync.v casgen.v casgen_sync.v video.sv rslatch.v)
CRTC_T80_BIN := $(OBJ_DIR)/crtc_t80/crtc_t80_tests

.PHONY: crtc-t80-test
crtc-t80-test: $(CRTC_T80_BIN)
	$<

$(CRTC_T80_BIN): $(CRTC_T80_RTL) crtc_t80_test.cpp Makefile
	mkdir -p $(OBJ_DIR)/crtc_t80
	$(VERILATOR) --cc --exe --build --top-module crtc_t80_top -UVERILATOR \
		--Mdir $(OBJ_DIR)/crtc_t80 -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o crtc_t80_tests $(CRTC_T80_RTL) crtc_t80_test.cpp

# Negative discriminator on old pre-B8 engines (commit d46609d066aafb6b182fd6fa504a91719500cd91)
CRTC_OLD_ENGINES_DIR := $(OBJ_DIR)/old_engines
CRTC_T80_OLD_RTL := crtc_t80_top.sv $(T80_NETLIST) ../rtl/CRTC.v $(CRTC_OLD_ENGINES_DIR)/crtc_type0_engine.v $(CRTC_OLD_ENGINES_DIR)/crtc_type1_engine.v $(addprefix ../rtl/GA40010/,ga40010.sv syncgen.v syncgen_sync.v casgen.v casgen_sync.v video.sv rslatch.v)
CRTC_T80_OLD_BIN := $(OBJ_DIR)/crtc_t80_old/crtc_t80_old_tests

$(CRTC_OLD_ENGINES_DIR)/crtc_type0_engine.v:
	mkdir -p $(CRTC_OLD_ENGINES_DIR)
	git show d46609d066aafb6b182fd6fa504a91719500cd91:rtl/crtc_type0_engine.v > $@.tmp && mv $@.tmp $@

$(CRTC_OLD_ENGINES_DIR)/crtc_type1_engine.v:
	mkdir -p $(CRTC_OLD_ENGINES_DIR)
	git show d46609d066aafb6b182fd6fa504a91719500cd91:rtl/crtc_type1_engine.v > $@.tmp && mv $@.tmp $@

.PHONY: crtc-t80-old-test
crtc-t80-old-test: $(CRTC_T80_OLD_BIN)
	@echo "Running old-engine discriminator (expecting 4 B8 failures)..."
	@set +e; \
	out=$$($< 2>&1); \
	status=$$?; \
	echo "$$out"; \
	if [ $$status -eq 0 ]; then \
		echo "ERROR: Old pre-B8 engine unexpectedly passed!"; exit 1; \
	fi; \
	echo "$$out" | grep -q "Type 1 OUT (C), C.*vma_flag" || { echo "FAIL: Missing Case 1 B8 failure (vma_flag)"; exit 1; }; \
	echo "$$out" | grep -q "Type 1 OUTI.*vma_flag" || { echo "FAIL: Missing Case 2 B8 failure (vma_flag)"; exit 1; }; \
	echo "$$out" | grep -q "Type 0 OUT (C), C.*row" || { echo "FAIL: Missing Case 3 B8 failure (row)"; exit 1; }; \
	echo "$$out" | grep -q "Type 1 OUT (C), C.*pending" || { echo "FAIL: Missing Case 4 B8 failure (pending)"; exit 1; }; \
	echo "PASS: Old pre-B8 engine correctly failed all 4 B8 side-effect checks"

$(CRTC_T80_OLD_BIN): $(CRTC_T80_OLD_RTL) crtc_t80_test.cpp Makefile
	mkdir -p $(OBJ_DIR)/crtc_t80_old
	$(VERILATOR) --cc --exe --build --top-module crtc_t80_top -UVERILATOR \
		--Mdir $(OBJ_DIR)/crtc_t80_old -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o crtc_t80_old_tests $(CRTC_T80_OLD_RTL) crtc_t80_test.cpp

.PHONY: video-output-test
video-output-test: $(VIDEO_OUTPUT_BIN)
	./$(VIDEO_OUTPUT_BIN)

$(VIDEO_OUTPUT_BIN): $(VIDEO_OUTPUT_SRC) b6_video_output_test.cpp Makefile
	mkdir -p $(@D)
	$(VERILATOR) --cc --exe --build -j 4 --top-module amstrad_video_output \
		--Mdir $(OBJ_DIR)/video_output -Wno-fatal -Wno-PROCASSWIRE -CFLAGS "$(CXXFLAGS)" \
		$(VIDEO_OUTPUT_SRC) $(abspath b6_video_output_test.cpp)

# Failure-first RGB scoping regression across supported mixer parameter widths.
MIXER_RGB_SRC := video_mixer_rgb_test.sv ../sys/video_mixer.sv ../sys/gamma_corr.sv \
    ../sys/video_freezer.sv ../sys/scandoubler.v ../sys/hq2x.sv
MIXER_RGB_BIN := $(OBJ_DIR)/video_mixer_rgb/Vvideo_mixer_rgb_test
test: video-mixer-rgb-test
.PHONY: video-mixer-rgb-test
video-mixer-rgb-test: $(MIXER_RGB_BIN)
	./$(MIXER_RGB_BIN)
$(MIXER_RGB_BIN): $(MIXER_RGB_SRC) Makefile
	$(VERILATOR) --binary --timing --top-module video_mixer_rgb_test \
		--Mdir $(OBJ_DIR)/video_mixer_rgb -Wno-fatal -Wno-PROCASSWIRE $(MIXER_RGB_SRC)

# SSM marker detector and DDR3 event ring (B4 phase 1). The executing leg uses
# the TV80-based T80pa wrapper, the same substitution the Plus boot fixtures
# make, so the default gate does not need GHDL.
SSM_RTL  := ../rtl/ssm_marker.v
SSM_SRC  := ssm_marker_top.v $(SSM_RTL) \
    $(addprefix plus/tv80/,t80pa.v tv80_core.v tv80_mcode.v tv80_alu.v tv80_reg.v) \
    $(addprefix ../rtl/GA40010/,ga40010.sv syncgen.v syncgen_sync.v casgen.v casgen_sync.v video.sv rslatch.v)
SSM_BIN  := $(OBJ_DIR)/ssm_marker/ssm_marker_tests

.PHONY: ssm-marker-test
ssm-marker-test: $(SSM_BIN)
	./$(SSM_BIN)

$(SSM_BIN): $(SSM_SRC) ssm_marker_test.cpp Makefile
	mkdir -p $(OBJ_DIR)/ssm_marker
	$(VERILATOR) --cc --exe --build --top-module ssm_marker_top -UVERILATOR \
		--Mdir $(OBJ_DIR)/ssm_marker +1364-2001ext+.v +1800-2017ext+.sv -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o ssm_marker_tests $(SSM_SRC) $(abspath ssm_marker_test.cpp)
