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

.PHONY: all build test lint soak clean crt-filter-blank-build crt-filter-blank-test

all: test

build: $(TEST_BIN) crt-filter-blank-build
	$(MAKE) -C plus build
	$(MAKE) -C ../rtl/GA40010 all
	$(MAKE) -C ../rtl/u765 build

crt-filter-blank-build: $(CRTF_TEST_BIN)

crt-filter-blank-test: $(CRTF_TEST_BIN)
	./$(CRTF_TEST_BIN)

test: $(TEST_BIN) crt-filter-blank-test
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

lint:
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
