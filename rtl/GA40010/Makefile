VERILATOR_BIN ?= verilator
# VERILATOR_BIN is also an internal Verilator wrapper variable. Keep the
# user-facing override for this legacy target, but do not leak it into the
# invoked wrapper or it can resolve back to itself recursively.
unexport VERILATOR_BIN
OBJ_DIR   := obj_dir
TOP       := ga40010_test
TEST_BIN  := $(OBJ_DIR)/ga40010_tb
R2JIT_OBJ := obj_r2jit
R2JIT_BIN := $(R2JIT_OBJ)/r2jit_tests

HDL_FILES := ga40010_test.v ga40010.sv syncgen.v syncgen_sync.v casgen.v \
	casgen_sync.v video.sv rslatch.v ../CRTC.v \
	../crtc_type0_engine.v ../crtc_type1_engine.v

VERILATOR_FLAGS := --cc --exe --build --trace --top-module $(TOP) \
	--Mdir $(OBJ_DIR) -Wno-fatal
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all test lint video clean

all: $(TEST_BIN)

test: $(R2JIT_BIN)
	./$(R2JIT_BIN)

$(TEST_BIN): $(HDL_FILES) ga40010_tb.cpp Makefile
	$(VERILATOR_BIN) $(VERILATOR_FLAGS) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(TEST_BIN)) $(HDL_FILES) ga40010_tb.cpp

$(R2JIT_BIN): $(HDL_FILES) r2jit_tb.cpp Makefile
	$(VERILATOR_BIN) $(VERILATOR_FLAGS) --Mdir $(R2JIT_OBJ) \
		-CFLAGS "$(CXXFLAGS)" -o $(notdir $(R2JIT_BIN)) \
		$(HDL_FILES) r2jit_tb.cpp

lint:
	$(VERILATOR_BIN) --lint-only --top-module $(TOP) -Wall -Wno-fatal $(HDL_FILES)

video: video.png

video.png: video.rgb
	$(RM) -- video.png
	ffmpeg -vcodec rawvideo -f rawvideo -pix_fmt rgb444 -s 1024x312 \
		-i video.rgb -f image2 -vcodec png video.png
	display video.png

clean:
	$(RM) -r -- $(OBJ_DIR)
	$(RM) -r -- $(R2JIT_OBJ)
	$(RM) -- ga40010_tb video.png
