#!/usr/bin/env bash
set -euo pipefail

# Append a compile-effort tier to a Quartus QSF.
#
#   apply-quartus-effort.sh <full|smoke> <project.qsf>
#
# "full" leaves the project file untouched.  "smoke" appends overrides at the
# end of the file; later set_global_assignment lines win in a QSF, so this
# downgrades the fitter without editing the checked-in settings.  The intent
# is fast feedback that an integration tip still elaborates, fits, and meets
# timing roughly; milestone builds (pull requests, tags, manual dispatches)
# keep the full-effort settings already in the file.
#
# The workflow's post-compile guard greps the build log for
# "Ignored assignment:" so an edition that silently drops one of these
# overrides fails the run instead of compiling at full cost while looking
# like a smoke run.  Measured figures and rationale live in
# docs/ci-testing-policy.md.

effort="${1:?usage: apply-quartus-effort.sh <full|smoke> <project.qsf>}"
qsf="${2:?usage: apply-quartus-effort.sh <full|smoke> <project.qsf>}"

if [[ ! -f "$qsf" ]]; then
	printf 'no such QSF: %s\n' "$qsf" >&2
	exit 1
fi

case "$effort" in
	full)
		exit 0
		;;
	smoke)
		;;
	*)
		printf 'unknown effort tier: %s\n' "$effort" >&2
		exit 1
		;;
esac

cat >> "$qsf" <<'OVERRIDES'

# --- CI smoke tier (scripts/ci/apply-quartus-effort.sh) ---
set_global_assignment -name FITTER_EFFORT "FAST FIT"
set_global_assignment -name PHYSICAL_SYNTHESIS_COMBO_LOGIC OFF
set_global_assignment -name PHYSICAL_SYNTHESIS_REGISTER_DUPLICATION OFF
set_global_assignment -name PHYSICAL_SYNTHESIS_REGISTER_RETIMING OFF
set_global_assignment -name PHYSICAL_SYNTHESIS_COMBO_LOGIC_FOR_AREA OFF
set_global_assignment -name PHYSICAL_SYNTHESIS_ASYNCHRONOUS_SIGNAL_PIPELINING OFF
set_global_assignment -name FINAL_PLACEMENT_OPTIMIZATION "Never"
set_global_assignment -name PERIPHERY_TO_CORE_PLACEMENT_AND_ROUTING_OPTIMIZATION OFF
OVERRIDES

printf 'smoke-effort overrides appended to %s\n' "$qsf"
