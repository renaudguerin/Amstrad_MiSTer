#!/usr/bin/env bash
set -euo pipefail

# Print every repository path that Quartus actually synthesizes, one per line,
# resolved transitively from the production manifest.
#
# This exists so no second list has to be kept in step with `files.qip`.  The
# previous hand-maintained allowlist in classify-synthesis-paths.sh duplicated
# part of the manifest and silently went stale twice: the GA40010 netlist
# sources and the u765 controller were both invisible to Tier B until a review
# noticed.  Anything added to a QIP is covered here the moment it is added.
#
# Two path conventions appear in the manifests and both are handled:
#   set_global_assignment -name VERILOG_FILE rtl/sdram.v
#   set_global_assignment -name VHDL_FILE [file join $::quartus(qip_path) T80s.vhd]
# The first is relative to the repository root, the second to the directory of
# the QIP that contains it.

repo_root="$(git rev-parse --show-toplevel)"
root_manifest="${1:-files.qip}"

declare -A seen=()

emit_manifest() {
	local manifest="$1"
	local manifest_dir
	manifest_dir="$(dirname -- "$manifest")"

	[[ -f "$repo_root/$manifest" ]] || return 0
	if [[ -n "${seen[$manifest]:-}" ]]; then
		return 0
	fi
	seen[$manifest]=1
	printf '%s\n' "$manifest"

	local kind raw resolved
	while read -r kind raw; do
		[[ -n "$raw" ]] || continue

		if [[ "$raw" == *'quartus(qip_path)'* ]]; then
			# [file join $::quartus(qip_path) name.sv ] -> name.sv
			resolved="${raw##*quartus(qip_path)}"
			resolved="${resolved%%]*}"
			resolved="$(printf '%s' "$resolved" | tr -d '[:space:]')"
			[[ -n "$resolved" ]] || continue
			resolved="$manifest_dir/$resolved"
		else
			resolved="$(printf '%s' "$raw" | tr -d '[:space:]')"
		fi

		# Normalize ./ and any accidental doubled separators.
		resolved="${resolved#./}"
		resolved="${resolved//\/.\//\/}"

		if [[ "$kind" == "QIP_FILE" ]]; then
			emit_manifest "$resolved"
		elif [[ -z "${seen[$resolved]:-}" ]]; then
			seen[$resolved]=1
			printf '%s\n' "$resolved"
		fi
	done < <(
		grep -oE '^set_global_assignment -name (VERILOG_FILE|SYSTEMVERILOG_FILE|VHDL_FILE|QIP_FILE|SDC_FILE)[[:space:]]+.*' \
			"$repo_root/$manifest" |
			sed -E 's/^set_global_assignment -name ([A-Z_]+)[[:space:]]+/\1 /'
	)
}

emit_manifest "$root_manifest" | sort -u
