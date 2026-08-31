#!/usr/bin/env bash
set -euo pipefail

if [[ "$#" -ne 1 ]]; then
	printf 'usage: %s <Amstrad.sta.summary>\n' "${0##*/}" >&2
	exit 2
fi

report="$1"
if [[ ! -s "$report" ]]; then
	printf 'timing closure: missing or empty report: %s\n' "$report" >&2
	exit 1
fi

# Quartus emits one Type/Slack/TNS triplet per clock and analysis kind.  Parse
# only Setup and Hold, but require both kinds and complete numeric triplets.
# This deliberately fails closed: an unfamiliar/truncated summary must not
# turn a synthesis leg green merely because no negative number was matched.
awk '
	function fail(message) {
		printf "timing closure: %s\n", message > "/dev/stderr"
		failed = 1
	}
	function numeric(value) {
		return value ~ /^-?([0-9]+([.][0-9]*)?|[.][0-9]+)$/
	}
	function finish_entry() {
		if (kind == "")
			return
		if (!have_slack || !have_tns) {
			fail("incomplete " kind " entry: " label)
			kind = ""
			return
		}

		count[kind]++
		if (slack + 0 < 0)
			fail(kind " slack is negative (" slack "): " label)
		if (tns + 0 != 0)
			fail(kind " TNS is nonzero (" tns "): " label)

		if (!have_min[kind] || slack + 0 < minimum[kind] + 0) {
			minimum[kind] = slack
			have_min[kind] = 1
		}
		kind = ""
	}

	$1 == "Type" && $2 == ":" {
		finish_entry()
		if ($3 == "Setup" || $3 == "Hold") {
			kind = $3
			label = substr($0, index($0, $3))
			have_slack = 0
			have_tns = 0
		}
		next
	}

	kind != "" && $1 == "Slack" && $2 == ":" {
		if (have_slack)
			fail("duplicate Slack field: " label)
		else if (!numeric($3))
			fail("invalid Slack value (" $3 "): " label)
		else {
			slack = $3
			have_slack = 1
		}
		next
	}

	kind != "" && $1 == "TNS" && $2 == ":" {
		if (have_tns)
			fail("duplicate TNS field: " label)
		else if (!numeric($3))
			fail("invalid TNS value (" $3 "): " label)
		else {
			tns = $3
			have_tns = 1
		}
		next
	}

	END {
		finish_entry()
		if (count["Setup"] == 0)
			fail("report contains no complete Setup entries")
		if (count["Hold"] == 0)
			fail("report contains no complete Hold entries")
		if (failed)
			exit 1
		printf "timing closure: PASS (setup min %s ns across %d clocks; hold min %s ns across %d clocks; TNS zero)\n", \
			minimum["Setup"], count["Setup"], minimum["Hold"], count["Hold"]
	}
' "$report"
