#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
checker="$script_dir/check-quartus-timing.sh"
fixture_dir="$(mktemp -d)"
trap 'rm -rf "$fixture_dir"' EXIT

write_report() {
	local path="$1"
	local setup_slack="$2"
	local setup_tns="$3"
	local hold_slack="$4"
	local hold_tns="$5"

	cat > "$path" <<EOF
TimeQuest Timing Analyzer Summary

Type  : Setup 'clock_a'
Slack : $setup_slack
TNS   : $setup_tns

Type  : Hold 'clock_a'
Slack : $hold_slack
TNS   : $hold_tns

Type  : Recovery 'clock_a'
Slack : -99.000
TNS   : -99.000
EOF
}

expect_pass() {
	local name="$1"
	local report="$2"
	if ! "$checker" "$report" > "$fixture_dir/$name.out" 2>&1; then
		printf 'expected timing fixture to pass: %s\n' "$name" >&2
		cat "$fixture_dir/$name.out" >&2
		exit 1
	fi
}

expect_fail() {
	local name="$1"
	local report="$2"
	if "$checker" "$report" > "$fixture_dir/$name.out" 2>&1; then
		printf 'expected timing fixture to fail: %s\n' "$name" >&2
		cat "$fixture_dir/$name.out" >&2
		exit 1
	fi
}

write_report "$fixture_dir/clean.summary" 0.378 0.000 0.171 -0.000
expect_pass clean "$fixture_dir/clean.summary"

write_report "$fixture_dir/setup-slack.summary" -0.047 0.000 0.245 0.000
expect_fail setup-slack "$fixture_dir/setup-slack.summary"

write_report "$fixture_dir/hold-slack.summary" 0.378 0.000 -0.001 0.000
expect_fail hold-slack "$fixture_dir/hold-slack.summary"

write_report "$fixture_dir/setup-tns.summary" 0.378 -0.650 0.245 0.000
expect_fail setup-tns "$fixture_dir/setup-tns.summary"

write_report "$fixture_dir/hold-tns.summary" 0.378 0.000 0.245 -0.100
expect_fail hold-tns "$fixture_dir/hold-tns.summary"

cat > "$fixture_dir/missing-hold.summary" <<'EOF'
Type  : Setup 'clock_a'
Slack : 0.378
TNS   : 0.000
EOF
expect_fail missing-hold "$fixture_dir/missing-hold.summary"

cat > "$fixture_dir/truncated.summary" <<'EOF'
Type  : Setup 'clock_a'
Slack : 0.378
Type  : Hold 'clock_a'
Slack : 0.171
TNS   : 0.000
EOF
expect_fail truncated "$fixture_dir/truncated.summary"

write_report "$fixture_dir/malformed.summary" unconstrained 0.000 0.245 0.000
expect_fail malformed "$fixture_dir/malformed.summary"

: > "$fixture_dir/empty.summary"
expect_fail empty "$fixture_dir/empty.summary"
expect_fail missing "$fixture_dir/does-not-exist.summary"

printf 'Quartus timing closure parser: passed\n'
