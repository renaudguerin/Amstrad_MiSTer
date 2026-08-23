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
# Everything the manifest reaches, including sources nested inside a child QIP
# and the GA40010 netlist recreation.  None of these are listed in the
# classifier: they are covered because files.qip compiles them.
expect true rtl/GA40010/ga40010.sv
expect true rtl/GA40010/video.sv
expect true rtl/GA40010/syncgen_sync.v
expect true rtl/GA40010/casgen_sync.v
expect true rtl/GA40010/rslatch.v
expect true rtl/u765/u765.sv
expect true rtl/plus/plus_mmu.v
expect true rtl/plus/plus_cartridge_memory.v
expect true rtl/color_mix.sv
expect true rtl/T80/T80s.vhd
expect true rtl/playcity/Z80CTC/z80ctc_top.vhd

# Internal CRTC RTL IS synthesized, so the classifier says so.  Keeping it off
# Tier B is the workflow's job (stream branches stay on Tier A), not a lie told
# here.  This is the bug that let 27078f4 merge unsynthesized: the old
# allowlist answered "false" for a files.qip source and nothing downstream
# reconsidered it at the integration point.
expect true rtl/CRTC.v
expect true rtl/crtc_type0_engine.v
expect true rtl/crtc_type1_engine.v

# The PLL chain hangs off a Tcl-computed QIP name that the manifest walk cannot
# follow, so it is listed explicitly in the classifier.
expect true rtl/pll.v
expect true rtl/pll.qip
expect true rtl/pll/pll_0002.v

# Simulation, tooling, documentation, and RTL that no QIP compiles.
expect false docs/current-status.md
expect false sim/sim_main.cpp
expect false sim/plus/asic_video_test.cpp
expect false tools/split-differential/diff_main.cpp
expect false rtl/GA40010/Makefile
expect false rtl/GA40010/ga40010_test.v
expect false rtl/plus/asic_video.v
expect false README.md

actual="$(printf '%s\n' docs/current-status.md rtl/plus/asic_video.v Amstrad.sv sim/sim_main.cpp | "$classifier")"
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

# The manifest resolver must reach every source Quartus compiles, including
# through nested QIPs, and every path it prints must exist.  A silent resolver
# failure would make the classifier quietly under-report.
sources="$("$script_dir/list-synthesized-sources.sh")"
for required_source in rtl/CRTC.v rtl/GA40010/ga40010.sv rtl/u765/u765.sv rtl/T80/T80s.vhd; do
	if ! printf '%s\n' "$sources" | grep -qxF -- "$required_source"; then
		printf 'manifest resolver did not reach %s\n' "$required_source" >&2
		exit 1
	fi
done

repo_root="$(git -C "$script_dir" rev-parse --show-toplevel)"
while IFS= read -r resolved_source; do
	[[ -n "$resolved_source" ]] || continue
	if [[ ! -f "$repo_root/$resolved_source" ]]; then
		printf 'manifest resolver produced a path that does not exist: %s\n' \
			"$resolved_source" >&2
		exit 1
	fi
done <<< "$sources"

printf 'synthesized-source manifest resolution: passed\n'
