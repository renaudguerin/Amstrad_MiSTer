#!/usr/bin/env bash
set -euo pipefail

# Print "true" when a changed path can affect the synthesized bitstream.
#
# Two sources, no hand-maintained duplication:
#
#   1. Every source Quartus actually compiles, resolved transitively from
#      `files.qip` by list-synthesized-sources.sh.  Adding a module to a QIP is
#      therefore enough to put it under Tier B; nothing has to be mirrored here.
#      The previous version of this script kept its own RTL allowlist and went
#      stale twice, hiding the GA40010 netlist sources and the u765 controller
#      from synthesis until a review caught it.
#   2. Files outside the manifest that still change the build: the Quartus
#      project files, the MiSTer platform tree, and the CI definition itself.
#
# WHEN this answers "true" leads to a Quartus run is the workflow's decision,
# not this script's.  Stream branches stay on Tier A no matter what they touch;
# integration branches, pull requests, tags, and manual dispatches synthesize.
# See docs/ci-testing-policy.md.

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

synthesized="$(mktemp)"
trap 'rm -f "$synthesized"' EXIT
"$script_dir/list-synthesized-sources.sh" > "$synthesized"

required=false

while IFS= read -r path; do
	[[ -n "$path" ]] || continue

	# 2. Build-affecting files the manifest walk cannot reach.
	#
	#    - The Quartus project files and the CI definition are not sources.
	#    - `sys/` is the MiSTer platform tree, pulled in by the project rather
	#      than by files.qip.
	#    - The PLL chain is reachable only through a *Tcl-computed* reference:
	#      sys/sys.qip builds its child QIP name from $quartus(version) at run
	#      time (pll_q17.qip), which then names rtl/pll.qip and rtl/pll/*.
	#      A static walk cannot follow that, so the PLL paths are listed here
	#      on purpose.  If another Tcl-computed reference is ever introduced,
	#      it needs the same treatment - the walk will not warn you.
	case "$path" in
		.github/workflows/build.yml | \
		.github/workflows/local-build.yml | \
		scripts/ci/classify-synthesis-paths.sh | \
		scripts/ci/check-quartus-timing.sh | \
		scripts/ci/list-synthesized-sources.sh | \
		scripts/ci/test-check-quartus-timing.sh | \
		scripts/ci/test-classify-synthesis-paths.sh | \
		scripts/ci/apply-quartus-effort.sh | \
		Amstrad.qpf | Amstrad.qsf | \
		sys/* | \
		rtl/*.qip | rtl/pll.v | rtl/pll/*)
			required=true
			break
			;;
	esac

	# 1. Anything Quartus compiles, per the manifest.
	if grep -qxF -- "$path" "$synthesized"; then
		required=true
		break
	fi
done

printf '%s\n' "$required"
