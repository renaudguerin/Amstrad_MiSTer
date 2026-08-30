VERILATOR ?= verilator

TOP       := CRTC
RTL       := ../rtl/CRTC.v ../rtl/crtc_type0_engine.v ../rtl/crtc_type1_engine.v
OBJ_DIR   := obj_dir
TEST_SRC  := sim_main.cpp
TEST_BIN  := $(OBJ_DIR)/crtc_tests

VERILATOR_FLAGS := --cc --exe --build --trace \
	--top-module $(TOP) --Mdir $(OBJ_DIR) -Wno-fatal
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all build test lint soak clean

all: test

build: $(TEST_BIN)
	$(MAKE) -C plus build
	$(MAKE) -C ../rtl/GA40010 all
	$(MAKE) -C ../rtl/u765 build

test: $(TEST_BIN)
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

lint:
	$(VERILATOR) --lint-only --top-module $(TOP) -Wall -Wno-fatal $(RTL)
	$(MAKE) -C plus lint
	$(MAKE) -C ../rtl/GA40010 lint
	$(MAKE) -C ../rtl/u765 lint

clean:
	$(RM) -r -- $(OBJ_DIR)
	$(MAKE) -C plus clean
	$(MAKE) -C ../rtl/GA40010 clean
	$(MAKE) -C ../rtl/u765 clean
