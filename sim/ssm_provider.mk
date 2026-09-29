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
