VERILATOR_BIN ?= verilator
# VERILATOR_BIN is also an internal Verilator wrapper variable. Keep the
# user-facing override for this legacy target, but do not leak it into the
# invoked wrapper or it can resolve back to itself recursively.
unexport VERILATOR_BIN
TOP       := ga40010_test
R2JIT_OBJ := obj_r2jit
R2JIT_BIN := $(R2JIT_OBJ)/r2jit_tests
SNA_LIFECYCLE_OBJ := $(R2JIT_OBJ)/sna_lifecycle
SNA_LIFECYCLE_BIN := $(SNA_LIFECYCLE_OBJ)/sna_lifecycle_tests
SNA_LIFECYCLE_RTL := sna_lifecycle_top.sv ga40010.sv syncgen.v syncgen_sync.v \
	casgen.v casgen_sync.v video.sv rslatch.v

HDL_FILES := ga40010_test.v ga40010.sv syncgen.v syncgen_sync.v casgen.v \
	casgen_sync.v video.sv rslatch.v ../CRTC.v \
	../crtc_type0_engine.v ../crtc_type1_engine.v

VERILATOR_FLAGS := --cc --exe --build --top-module $(TOP) -Wno-fatal
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all test lint clean

# Build the pin and SNA lifecycle suites without running, so
# `make -C sim build` stays side-effect free.
all: $(R2JIT_BIN) $(SNA_LIFECYCLE_BIN)

test: $(R2JIT_BIN) $(SNA_LIFECYCLE_BIN)
	./$(R2JIT_BIN)
	./$(SNA_LIFECYCLE_BIN)

$(R2JIT_BIN): $(HDL_FILES) r2jit_tb.cpp Makefile
	$(VERILATOR_BIN) $(VERILATOR_FLAGS) --Mdir $(R2JIT_OBJ) \
		-CFLAGS "$(CXXFLAGS)" -o $(notdir $(R2JIT_BIN)) \
		$(HDL_FILES) r2jit_tb.cpp

# Public GA SNA restore across active-mode and outer-prefetch/inner-LOAD ownership.
.PHONY: sna-lifecycle-test
sna-lifecycle-test: $(SNA_LIFECYCLE_BIN)
	./$(SNA_LIFECYCLE_BIN)

$(SNA_LIFECYCLE_BIN): $(SNA_LIFECYCLE_RTL) sna_lifecycle_tb.cpp Makefile
	mkdir -p $(SNA_LIFECYCLE_OBJ)
	$(VERILATOR_BIN) --cc --exe --build --top-module sna_lifecycle_top \
		-Wno-fatal -Werror-PINMISSING --Mdir $(SNA_LIFECYCLE_OBJ) \
		-CFLAGS "$(CXXFLAGS)" -o $(notdir $(SNA_LIFECYCLE_BIN)) \
		$(SNA_LIFECYCLE_RTL) sna_lifecycle_tb.cpp

lint:
	$(VERILATOR_BIN) --lint-only --top-module $(TOP) -Wall -Wno-fatal $(HDL_FILES)

clean:
	$(RM) -r -- $(R2JIT_OBJ) $(SNA_LIFECYCLE_OBJ)
