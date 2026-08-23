#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
classifier="$script_dir/classify-synthesis-paths.sh"

expect() {
	local expected="$1"
	local path="$2"
	local actual

	actual="$(printf '%s\n' "$path" | "$classifier")"
	if [[ "$actual" != "$expected" ]]; then
		printf 'classification mismatch: %s: expected %s, got %s\n' \
			"$path" "$expected" "$actual" >&2
		exit 1
	fi
}

# Project, manifest, platform, and known integration boundaries synthesize
# automatically.
expect true .github/workflows/build.yml
expect true scripts/ci/classify-synthesis-paths.sh
expect true scripts/ci/test-classify-synthesis-paths.sh
expect true Amstrad.sv
expect true Amstrad.qpf
expect true Amstrad.qsf
expect true Amstrad.sdc
expect true files.qip
expect true sys/sys.qip
expect true rtl/GA40010/ga40010.qip
expect true rtl/tzxplayer.vhd
expect true rtl/dandanator/cpc_dandanator.vhd
expect true rtl/pll.v
expect true rtl/pll/pll_0002.v
expect true rtl/Amstrad_motherboard.v
expect true rtl/Amstrad_MMU.v
expect true rtl/sdram.v
# The qip-listed GA40010 netlist sources and the u765 FDC are hardware-facing:
# a silent source edit must trigger Tier B even though the qip itself is
# unchanged.  Their directory siblings stay simulation-only.
expect true rtl/GA40010/ga40010.sv
expect true rtl/GA40010/video.sv
expect true rtl/GA40010/syncgen_sync.v
expect true rtl/GA40010/casgen_sync.v
expect true rtl/GA40010/rslatch.v
expect true rtl/u765/u765.sv
expect true rtl/plus/plus_mmu.v
expect true rtl/plus/plus_cartridge_memory.v
expect true rtl/color_mix.sv

# Test/tool changes and internal RTL are simulation-only until a named
# milestone or an explicitly dispatched semantic-risk build.
expect false docs/current-status.md
expect false sim/sim_main.cpp
expect false tools/split-differential/diff_main.cpp
expect false rtl/GA40010/Makefile
expect false rtl/GA40010/ga40010_test.v
expect false rtl/CRTC.v
expect false rtl/crtc_type0_engine.v
expect false rtl/plus/asic_video.v

actual="$(printf '%s\n' docs/current-status.md rtl/CRTC.v Amstrad.sv sim/sim_main.cpp | "$classifier")"
if [[ "$actual" != "true" ]]; then
	printf 'classification mismatch: boundary path in a list must win, got %s\n' \
		"$actual" >&2
	exit 1
fi

actual="$(printf '' | "$classifier")"
if [[ "$actual" != "false" ]]; then
	printf 'classification mismatch: empty input must be false, got %s\n' \
		"$actual" >&2
	exit 1
fi

printf 'synthesis path classification: passed\n'
