#!/usr/bin/env bash
set -euo pipefail

# Print "true" when a changed path crosses a synthesis integration boundary.
# Internal RTL is deliberately absent: it is covered by the per-commit
# Verilator gate and receives a full Quartus build at a named milestone.  The
# semantic exceptions (clocking, memory arbitration, or RGB-width changes)
# cannot be recognized safely from a pathname and must be dispatched manually.
# The individually listed GA40010 and u765 sources are the exception to that
# rule: they are qip-referenced hardware-facing modules whose silent edits must
# never evade Tier B (the GA netlist recreation is additionally treated as a
# frozen reference).  Their directory siblings (test bench, tooling) stay
# simulation-only.
required=false

while IFS= read -r path; do
	[[ -n "$path" ]] || continue

	case "$path" in
		.github/workflows/build.yml | \
		scripts/ci/classify-synthesis-paths.sh | \
		scripts/ci/test-classify-synthesis-paths.sh | \
		Amstrad.qpf | Amstrad.qsf | Amstrad.sdc | files.qip | Amstrad.sv | \
		sys/* | \
		rtl/*.qip | rtl/*.vhd | \
		rtl/pll.v | rtl/pll/* | \
		rtl/Amstrad_motherboard.v | rtl/Amstrad_MMU.v | rtl/sdram.v | \
		rtl/GA40010/ga40010.sv | rtl/GA40010/video.sv | \
		rtl/GA40010/syncgen_sync.v | rtl/GA40010/casgen_sync.v | \
		rtl/GA40010/rslatch.v | \
		rtl/plus/plus_mmu.v | rtl/plus/plus_cartridge_memory.v | \
		rtl/u765/u765.sv | \
		rtl/color_mix.sv)
			required=true
			break
			;;
	esac
done

printf '%s\n' "$required"
