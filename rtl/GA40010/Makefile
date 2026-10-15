VERILATOR_BIN ?= verilator
# VERILATOR_BIN is also an internal Verilator wrapper variable. Keep the
# user-facing override for this legacy target, but do not leak it into the
# invoked wrapper or it can resolve back to itself recursively.
unexport VERILATOR_BIN
TOP       := ga40010_test
R2JIT_OBJ := obj_r2jit
R2JIT_BIN := $(R2JIT_OBJ)/r2jit_tests

HDL_FILES := ga40010_test.v ga40010.sv syncgen.v syncgen_sync.v casgen.v \
	casgen_sync.v video.sv rslatch.v ../CRTC.v \
	../crtc_type0_engine.v ../crtc_type1_engine.v

VERILATOR_FLAGS := --cc --exe --build --trace --top-module $(TOP) -Wno-fatal
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all test lint clean

# The asserted bench is the R2.JIT pin suite (r2jit_tb.cpp); `all` builds it
# without running so `make -C sim build` stays side-effect free.
all: $(R2JIT_BIN)

test: $(R2JIT_BIN)
	./$(R2JIT_BIN)

$(R2JIT_BIN): $(HDL_FILES) r2jit_tb.cpp Makefile
	$(VERILATOR_BIN) $(VERILATOR_FLAGS) --Mdir $(R2JIT_OBJ) \
		-CFLAGS "$(CXXFLAGS)" -o $(notdir $(R2JIT_BIN)) \
		$(HDL_FILES) r2jit_tb.cpp

lint:
	$(VERILATOR_BIN) --lint-only --top-module $(TOP) -Wall -Wno-fatal $(HDL_FILES)

clean:
	$(RM) -r -- $(R2JIT_OBJ)
