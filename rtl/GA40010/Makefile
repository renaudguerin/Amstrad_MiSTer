VERILATOR_BIN ?= verilator
# VERILATOR_BIN is also an internal Verilator wrapper variable. Keep the
# user-facing override for this legacy target, but do not leak it into the
# invoked wrapper or it can resolve back to itself recursively.
unexport VERILATOR_BIN
OBJ_DIR   := obj_dir
TOP       := ga40010_test
TEST_BIN  := $(OBJ_DIR)/ga40010_tb

HDL_FILES := ga40010_test.v ga40010.sv syncgen.v syncgen_sync.v casgen.v \
	casgen_sync.v video.sv rslatch.v ../CRTC.v \
	../crtc_type0_engine.v ../crtc_type1_engine.v

VERILATOR_FLAGS := --cc --exe --build --trace --top-module $(TOP) \
	--Mdir $(OBJ_DIR) -Wno-fatal
CXXFLAGS ?= -std=c++17 -O2

.PHONY: all video clean

all: $(TEST_BIN)

$(TEST_BIN): $(HDL_FILES) ga40010_tb.cpp Makefile
	$(VERILATOR_BIN) $(VERILATOR_FLAGS) -CFLAGS "$(CXXFLAGS)" \
		-o $(notdir $(TEST_BIN)) $(HDL_FILES) ga40010_tb.cpp

video: video.png

video.png: video.rgb
	$(RM) -- video.png
	ffmpeg -vcodec rawvideo -f rawvideo -pix_fmt rgb444 -s 1024x312 \
		-i video.rgb -f image2 -vcodec png video.png
	display video.png

clean:
	$(RM) -r -- $(OBJ_DIR)
	$(RM) -- ga40010_tb video.png
