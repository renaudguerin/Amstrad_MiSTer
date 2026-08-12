VERILATOR ?= verilator

TOP      := asic_unlock
RTL      := ../../rtl/plus/asic_unlock.v
OBJ_DIR  := obj_dir
TEST_SRC := asic_unlock_test.cpp
TEST_BIN := $(OBJ_DIR)/asic_unlock_tests

VERILATOR_FLAGS := --cc --exe --build --language 1364-2001 \
	--top-module $(TOP) --Mdir $(OBJ_DIR)
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
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(TOP) -Wall $(RTL)

clean:
	$(RM) -r -- $(OBJ_DIR)
