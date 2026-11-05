# Reuse the existing named-port-only GHDL motherboard adaptation.
SSM_PROVIDER_RTL := ssm_provider_top.sv ../rtl/ssm_marker.v $(filter-out b18_snapshot_top.sv,$(B18_SNAPSHOT_RTL))
SSM_PROVIDER_BIN := $(OBJ_DIR)/ssm_provider/ssm_provider_tests
.PHONY: ssm-provider-test
ssm-provider-test: $(SSM_PROVIDER_BIN)
	$<
$(SSM_PROVIDER_BIN): $(SSM_PROVIDER_RTL) ssm_provider_test.cpp ssm_provider.mk
	mkdir -p $(@D)
	$(VERILATOR) --cc --exe --build --top-module ssm_provider_top -UVERILATOR \
		+1364-2001ext+.v +1800-2017ext+.sv --Mdir $(OBJ_DIR)/ssm_provider \
		-Wno-fatal -Wno-PROCASSWIRE -CFLAGS "$(CXXFLAGS)" \
		-o ssm_provider_tests $(SSM_PROVIDER_RTL) ssm_provider_test.cpp

# Temporary default-off S checkpoint observer. The ordinary provider is unchanged.
SIM_TESTS += ssm-s-trace-test
S_TRACE_BIN := $(OBJ_DIR)/ssm_s_trace/ssm_s_trace_tests
.PHONY: ssm-s-trace-test
ssm-s-trace-test: $(S_TRACE_BIN)
	$<
$(S_TRACE_BIN): ssm_s_trace_top.sv ../rtl/ssm_marker.v ssm_s_trace_test.cpp ssm_provider.mk
	mkdir -p $(@D)
	$(VERILATOR) --cc --exe --build --top-module ssm_s_trace_top -DSSM_S_PASSIVE_TRACE $(S_TRACE_FLAGS) \
		--Mdir $(OBJ_DIR)/ssm_s_trace -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o ssm_s_trace_tests ssm_s_trace_top.sv ../rtl/ssm_marker.v ssm_s_trace_test.cpp

S_TRACE_PROVIDER_BIN := $(OBJ_DIR)/ssm_s_trace_provider/ssm_s_trace_provider_tests
.PHONY: ssm-s-trace-provider-test
ssm-s-trace-provider-test: $(S_TRACE_PROVIDER_BIN)
	$<
$(S_TRACE_PROVIDER_BIN): $(SSM_PROVIDER_RTL) ssm_s_trace_provider_test.cpp ssm_provider.mk
	mkdir -p $(@D)
	$(VERILATOR) --cc --exe --build --top-module ssm_provider_top -UVERILATOR \
		-DSSM_S_PASSIVE_TRACE -GCRTC_KIND=1 +1364-2001ext+.v +1800-2017ext+.sv \
		--Mdir $(OBJ_DIR)/ssm_s_trace_provider -Wno-fatal -Wno-PROCASSWIRE \
		-CFLAGS "$(CXXFLAGS)" -o ssm_s_trace_provider_tests $(SSM_PROVIDER_RTL) ssm_s_trace_provider_test.cpp

# Default-off DT observer: real M1/DDR transport plus the offline opaque decoder.
SIM_TESTS += ssm-dt-trace-test
# Main aggregate prerequisites are expanded before this included fragment.
test-run full-run: ssm-dt-trace-test
DT_TRACE_OBJ := $(OBJ_DIR)/ssm_dt_trace
DT_TRACE_BIN := $(DT_TRACE_OBJ)/ssm_dt_trace_tests
DT_TRACE_HOST := ssm_dt_trace_host_test.py
DT_TRACE_DECODER := ../scripts/hardware-loop/ssm_dt_trace.py
DT_TRACE_SCHEMA := ../scripts/hardware-loop/ssm_dt_trace_schema.json
.PHONY: ssm-dt-trace-test
ssm-dt-trace-test: $(DT_TRACE_BIN) $(DT_TRACE_HOST) $(DT_TRACE_DECODER) $(DT_TRACE_SCHEMA) ../scripts/hardware-loop/ssm_ring.py
	cd $(DT_TRACE_OBJ) && ./ssm_dt_trace_tests
	python3 $(DT_TRACE_HOST) $(DT_TRACE_OBJ)/transport-fixture.bin
$(DT_TRACE_BIN): ssm_dt_trace_top.sv ../rtl/ssm_marker.v ssm_dt_trace_test.cpp ssm_provider.mk
	mkdir -p $(@D)
	$(VERILATOR) --cc --exe --build --top-module ssm_dt_trace_top \
		-DSSM_S_PASSIVE_TRACE -DSSM_DT_PASSIVE_TRACE -Werror-PINMISSING \
		--Mdir $(DT_TRACE_OBJ) -Wno-fatal -CFLAGS "$(CXXFLAGS)" \
		-o ssm_dt_trace_tests ssm_dt_trace_top.sv ../rtl/ssm_marker.v ssm_dt_trace_test.cpp
