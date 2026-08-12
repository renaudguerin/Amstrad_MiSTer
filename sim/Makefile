VERILATOR ?= verilator

TOP       := UM6845R
RTL       := ../rtl/UM6845R.v
OBJ_DIR   := obj_dir
TEST_SRC  := sim_main.cpp
TEST_BIN  := $(OBJ_DIR)/crtc_tests

VERILATOR_FLAGS := --cc --exe --build --trace \
	--top-module $(TOP) --Mdir $(OBJ_DIR) -Wno-fatal
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all build test lint clean

all: test

build: $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(RTL) $(TEST_SRC) Makefile
	$(VERILATOR) $(VERILATOR_FLAGS) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(TEST_BIN)) $(RTL) $(TEST_SRC)

lint:
	$(VERILATOR) --lint-only --top-module $(TOP) -Wall -Wno-fatal $(RTL)

clean:
	$(RM) -r -- $(OBJ_DIR)
