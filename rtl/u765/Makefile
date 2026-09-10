VERILATOR ?= verilator

TOP       := u765_test
OBJ_DIR   := obj_dir
TEST_BIN  := $(OBJ_DIR)/u765_tests
HDL       := u765_test.sv u765.sv
TEST_SRC  := u765_tb.cpp

VERILATOR_FLAGS := --cc --exe --build --trace --top-module $(TOP) \
	--Mdir $(OBJ_DIR) -Wno-fatal
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all build test lint clean

all: test

build: $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(HDL) $(TEST_SRC) Makefile
	$(VERILATOR) $(VERILATOR_FLAGS) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(TEST_BIN)) $(HDL) $(TEST_SRC)

lint:
	$(VERILATOR) --lint-only --top-module $(TOP) -Wall -Wno-fatal $(HDL)

clean:
	$(RM) -r -- $(OBJ_DIR)
