SIM_TESTS += shaker-e-c1-terminal-r9-test
test-run full-run: shaker-e-c1-terminal-r9-test

SHAKER_E_C1_OBJ := $(OBJ_DIR)/shaker_e_c1_terminal_r9
SHAKER_E_C1_BIN := $(SHAKER_E_C1_OBJ)/shaker_e_c1_terminal_r9_test

.PHONY: shaker-e-c1-terminal-r9-test
shaker-e-c1-terminal-r9-test: $(SHAKER_E_C1_BIN)
	$<

$(SHAKER_E_C1_BIN): $(RTL) shaker_e_c1_terminal_r9_test.cpp shaker_e_c1.mk
	mkdir -p $(@D)
	$(VERILATOR) --cc --exe --build --top-module CRTC --prefix VShakerEC1TerminalR9 \
		--Mdir $(SHAKER_E_C1_OBJ) \
		-Wno-fatal -CFLAGS "$(CXXFLAGS)" -o $(notdir $@) $(RTL) shaker_e_c1_terminal_r9_test.cpp
