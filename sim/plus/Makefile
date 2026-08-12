VERILATOR ?= verilator
OBJ_DIR   := obj_dir

ASIC_TOP      := asic_unlock
ASIC_RTL      := ../../rtl/plus/asic_unlock.v
ASIC_OBJ_DIR  := obj_dir/asic_unlock
ASIC_TEST_SRC := asic_unlock_test.cpp
ASIC_TEST_BIN := $(ASIC_OBJ_DIR)/asic_unlock_tests

MODEL_TOP      := plus_model_select
MODEL_RTL      := ../../rtl/plus/plus_model_select.v
MODEL_OBJ_DIR  := obj_dir/plus_model_select
MODEL_TEST_SRC := plus_model_select_test.cpp
MODEL_TEST_BIN := $(MODEL_OBJ_DIR)/plus_model_select_tests

TEST_BINS := $(ASIC_TEST_BIN) $(MODEL_TEST_BIN)

VERILATOR_FLAGS := --cc --exe --build --language 1364-2001
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all build test lint clean

all: test

build: $(TEST_BINS)

test: $(TEST_BINS)
	./$(ASIC_TEST_BIN)
	./$(MODEL_TEST_BIN)

$(ASIC_TEST_BIN): $(ASIC_RTL) $(ASIC_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(ASIC_TOP) \
		--Mdir $(ASIC_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(ASIC_TEST_BIN)) $(ASIC_RTL) $(ASIC_TEST_SRC)

$(MODEL_TEST_BIN): $(MODEL_RTL) $(MODEL_TEST_SRC) Makefile | $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $(MODEL_TOP) \
		--Mdir $(MODEL_OBJ_DIR) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(MODEL_TEST_BIN)) $(MODEL_RTL) $(MODEL_TEST_SRC)

$(OBJ_DIR):
	mkdir -p -- $@

lint:
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(ASIC_TOP) -Wall $(ASIC_RTL)
	$(VERILATOR) --lint-only --language 1364-2001 \
		--top-module $(MODEL_TOP) -Wall $(MODEL_RTL)

clean:
	$(RM) -r -- $(OBJ_DIR)
