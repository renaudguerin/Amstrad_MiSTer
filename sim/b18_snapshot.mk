# B18 classic save/reload through production CPU, motherboard and SNA owners.
B18_MBOARD := $(OBJ_DIR)/b18_snapshot/Amstrad_motherboard.v
B18_SNAPSHOT_RTL := b18_snapshot_top.sv $(T80_NETLIST) $(B18_MBOARD) \
	$(addprefix ../rtl/,Amstrad_MMU.v CRTC.v crtc_type0_engine.v crtc_type1_engine.v crt_filter.v i8255.v YM2149.sv hid.sv sna_hw_header.v sna_save_capture.v sna_save_stream.v sna_cpu_header.v) \
	$(addprefix ../rtl/GA40010/,ga40010.sv syncgen.v syncgen_sync.v casgen.v casgen_sync.v video.sv rslatch.v) \
	$(addprefix ../rtl/plus/,asic_regs.v plus_sprite_ram.v asic_ga_timing.v asic_video.v asic_sprites.v asic_dma.v plus_sna_header.v plus_sna_apply.v)
B18_SNAPSHOT_BIN := $(OBJ_DIR)/b18_snapshot/b18_snapshot_tests
ASIC_R0_TERMINAL_BIN := $(OBJ_DIR)/asic_r0_terminal/asic_r0_terminal_tests

# VHDL ports are case-insensitive; GHDL preserves declaration case in Verilog.
# Adapt named CPU pins as in plus/prepare_d5_boot.py, and explicitly open the
# unused CPU BUSAK_n, GA ROM and PSG ACTIVE/IOA_out/IOB_out outputs.
# The production motherboard body is otherwise copied unchanged.
$(B18_MBOARD): ../rtl/Amstrad_motherboard.v $(T80_NETLIST) b18_snapshot.mk
	mkdir -p $(@D)
	python3 -c 'from pathlib import Path; import re; s=Path("$<").read_text(); n=Path("$(T80_NETLIST)").read_text().split("module T80pa")[1].split(");")[0]; ports={p.lower():p for p in re.findall(r"(?:input|output)\s+(?:\[[^]]+\]\s*)?(\w+)",n)}; a=s.index("T80pa CPU"); b=s.index("\n);",a); part=re.sub(r"\.(\w+)\(",lambda m:"."+ports.get(m[1].lower(),m[1])+"(",s[a:b]); s=s[:a]+part+",\n."+ports.get("busak_n","BUSAK_n")+"(), .OUT0(1\x27b0), .R800_mode(1\x27b0)"+s[b:]; ga_b=s.index("\n);",s.index("ga40010 GateArray")); s=s[:ga_b]+",\n\t.ROM()"+s[ga_b:]; psg_b=s.index("\n);",s.index("YM2149 PSG")); s=s[:psg_b]+",\n\t.ACTIVE(),\n\t.IOA_out(),\n\t.IOB_out()"+s[psg_b:]; Path("$@").write_text(s)'

.PHONY: b18-snapshot-test
b18-snapshot-test: $(B18_SNAPSHOT_BIN)
	$<
	python3 b18_snapshot_host_test.py

$(B18_SNAPSHOT_BIN): $(B18_SNAPSHOT_RTL) b18_snapshot_test.cpp b18_snapshot.mk Makefile
	mkdir -p $(OBJ_DIR)/b18_snapshot
	$(VERILATOR) --cc --exe --build --top-module b18_snapshot_top -UVERILATOR \
		+1364-2001ext+.v +1800-2017ext+.sv \
		--Mdir $(OBJ_DIR)/b18_snapshot -Wno-fatal -Wno-PROCASSWIRE -CFLAGS "$(CXXFLAGS)" \
		-o b18_snapshot_tests $(B18_SNAPSHOT_RTL) b18_snapshot_test.cpp

# Same real CPU/motherboard/SNA owners, with Plus selected at elaboration.
# This fixture parameter leaves all classic B18 runs unchanged by default.
.PHONY: asic-r0-terminal-test
asic-r0-terminal-test: $(ASIC_R0_TERMINAL_BIN)
	$<

$(ASIC_R0_TERMINAL_BIN): $(B18_SNAPSHOT_RTL) asic_r0_terminal_test.cpp b18_snapshot.mk Makefile
	mkdir -p $(OBJ_DIR)/asic_r0_terminal
	$(VERILATOR) --cc --exe --build --top-module b18_snapshot_top -GPLUS_MODE=1 -UVERILATOR \
		+1364-2001ext+.v +1800-2017ext+.sv \
		--Mdir $(OBJ_DIR)/asic_r0_terminal -Wno-fatal -Wno-PROCASSWIRE -CFLAGS "$(CXXFLAGS)" \
		-o asic_r0_terminal_tests $(B18_SNAPSHOT_RTL) asic_r0_terminal_test.cpp

# Pixel-boundary reference through the same real CPU/motherboard/RAM owners.
ASIC_BLANK_BIN := $(OBJ_DIR)/asic_blank_alignment/asic_blank_alignment_tests
.PHONY: asic-blank-alignment-test
asic-blank-alignment-test: $(ASIC_BLANK_BIN)
	$<

$(ASIC_BLANK_BIN): $(B18_SNAPSHOT_RTL) asic_blank_alignment_top.sv asic_blank_alignment_test.cpp b18_snapshot.mk Makefile
	mkdir -p $(OBJ_DIR)/asic_blank_alignment
	$(VERILATOR) --cc --exe --build --top-module asic_blank_alignment_top -UVERILATOR \
		+1364-2001ext+.v +1800-2017ext+.sv \
		--Mdir $(OBJ_DIR)/asic_blank_alignment -Wno-fatal -Wno-PROCASSWIRE -CFLAGS "$(CXXFLAGS)" \
		-o asic_blank_alignment_tests $(B18_SNAPSHOT_RTL) asic_blank_alignment_top.sv asic_blank_alignment_test.cpp

# Asset-free classic byte/mode boundaries on the same production T80 and RAM owners.
CLASSIC_MODE_BOUNDARY_BIN := $(OBJ_DIR)/classic_mode_boundary/classic_mode_boundary_tests
.PHONY: classic-mode-boundary-test
classic-mode-boundary-test: $(CLASSIC_MODE_BOUNDARY_BIN)
	$<

$(CLASSIC_MODE_BOUNDARY_BIN): $(B18_SNAPSHOT_RTL) classic_mode_boundary_top.sv classic_mode_boundary_test.cpp b18_snapshot.mk Makefile
	mkdir -p $(@D)
	$(VERILATOR) --cc --exe --build --top-module classic_mode_boundary_top -UVERILATOR \
		+1364-2001ext+.v +1800-2017ext+.sv \
		--Mdir $(OBJ_DIR)/classic_mode_boundary -Wno-fatal -Werror-PINMISSING -Wno-PROCASSWIRE \
		-CFLAGS "$(CXXFLAGS)" -o classic_mode_boundary_tests \
		$(B18_SNAPSHOT_RTL) classic_mode_boundary_top.sv classic_mode_boundary_test.cpp
