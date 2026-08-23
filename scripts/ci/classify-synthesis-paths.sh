#!/usr/bin/env bash
set -euo pipefail

# Print "true" when a changed path crosses a synthesis integration boundary.
# Internal RTL is deliberately absent: it is covered by the per-commit
# Verilator gate and receives a full Quartus build at a named milestone.  The
# semantic exceptions (clocking, memory arbitration, or RGB-width changes)
# cannot be recognized safely from a pathname and must be dispatched manually.
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
		rtl/plus/plus_mmu.v | rtl/plus/plus_cartridge_memory.v | \
		rtl/color_mix.sv)
			required=true
			break
			;;
	esac
done

printf '%s\n' "$required"
