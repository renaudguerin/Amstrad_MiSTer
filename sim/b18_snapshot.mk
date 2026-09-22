# B18 classic save/reload through production CPU, motherboard and SNA owners.
B18_MBOARD := $(OBJ_DIR)/b18_snapshot/Amstrad_motherboard.v
B18_SNAPSHOT_RTL := b18_snapshot_top.sv $(T80_NETLIST) $(B18_MBOARD) \
	$(addprefix ../rtl/,Amstrad_MMU.v CRTC.v crtc_type0_engine.v crtc_type1_engine.v crt_filter.v i8255.v YM2149.sv hid.sv sna_hw_header.v sna_save_capture.v sna_save_stream.v sna_cpu_header.v) \
	$(addprefix ../rtl/GA40010/,ga40010.sv syncgen.v syncgen_sync.v casgen.v casgen_sync.v video.sv rslatch.v) \
	$(addprefix ../rtl/plus/,asic_regs.v plus_sprite_ram.v asic_ga_timing.v asic_video.v asic_sprites.v asic_dma.v plus_sna_header.v plus_sna_apply.v)
B18_SNAPSHOT_BIN := $(OBJ_DIR)/b18_snapshot/b18_snapshot_tests

# VHDL ports are case-insensitive; GHDL preserves their declaration case in
# Verilog. Adapt named CPU pins only, as in plus/prepare_d5_boot.py. The
# production motherboard body is otherwise copied unchanged.
$(B18_MBOARD): ../rtl/Amstrad_motherboard.v $(T80_NETLIST) b18_snapshot.mk
	mkdir -p $(@D)
	python3 -c 'from pathlib import Path; import re; s=Path("$<").read_text(); n=Path("$(T80_NETLIST)").read_text().split("module T80pa")[1].split(");")[0]; ports={p.lower():p for p in re.findall(r"(?:input|output)\s+(?:\[[^]]+\]\s*)?(\w+)",n)}; a=s.index("T80pa CPU"); b=s.index("\n);",a); part=re.sub(r"\.(\w+)\(",lambda m:"."+ports.get(m[1].lower(),m[1])+"(",s[a:b]); Path("$@").write_text(s[:a]+part+",\n.OUT0(1\x27b0), .R800_mode(1\x27b0)"+s[b:])'

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
