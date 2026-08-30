#!/usr/bin/env bash
set -euo pipefail

# Install the repository's pinned Verilator release from the official source.
# Callers own prerequisite installation and choose a writable prefix with
# VERILATOR_INSTALL_PREFIX.  The exact upstream commit keeps hosted CI and the
# local Quartus VM on the same reproducible simulator rather than their older
# distribution packages.
readonly verilator_version="5.050"
readonly verilator_commit="848d926ebd4addacacd294dc84e35d9d4ae8078c"
readonly verilator_repo="https://github.com/verilator/verilator.git"
install_prefix="${VERILATOR_INSTALL_PREFIX:-/usr/local}"
install_marker="$install_prefix/.verilator-source-$verilator_commit"

if [[ -n "${VERILATOR_EXPECTED_COMMIT:-}" &&
      "$VERILATOR_EXPECTED_COMMIT" != "$verilator_commit" ]]; then
	printf 'Verilator pin mismatch: script=%s caller=%s\n' \
		"$verilator_commit" "$VERILATOR_EXPECTED_COMMIT" >&2
	exit 1
fi

version_matches() {
	local binary="$1"
	local reported

	[[ -x "$binary" && -f "$install_marker" ]] || return 1
	reported="$($binary --version)"
	[[ "$reported" == "Verilator $verilator_version "* ]]
}

if version_matches "$install_prefix/bin/verilator"; then
	printf 'Verilator %s already installed under %s\n' \
		"$verilator_version" "$install_prefix"
	exit 0
fi

if [[ "${1:-}" == "--check" ]]; then
	exit 1
elif [[ $# -ne 0 ]]; then
	printf 'usage: %s [--check]\n' "$0" >&2
	exit 2
fi

for command_name in autoconf bison flex g++ git help2man make perl python3; do
	if ! command -v "$command_name" >/dev/null 2>&1; then
		printf 'missing Verilator build prerequisite: %s\n' "$command_name" >&2
		exit 1
	fi
done

build_root="$(mktemp -d "${TMPDIR:-/tmp}/verilator-${verilator_version}.XXXXXX")"
trap 'rm -rf -- "$build_root"' EXIT
source_dir="$build_root/source"

git init --quiet "$source_dir"
git -C "$source_dir" remote add origin "$verilator_repo"
git -C "$source_dir" fetch --quiet --depth 1 origin "$verilator_commit"
git -C "$source_dir" checkout --quiet --detach FETCH_HEAD

actual_commit="$(git -C "$source_dir" rev-parse HEAD)"
if [[ "$actual_commit" != "$verilator_commit" ]]; then
	printf 'Verilator source mismatch: expected %s, got %s\n' \
		"$verilator_commit" "$actual_commit" >&2
	exit 1
fi

if [[ -n "${VERILATOR_BUILD_JOBS:-}" ]]; then
	build_jobs="$VERILATOR_BUILD_JOBS"
elif command -v nproc >/dev/null 2>&1; then
	build_jobs="$(nproc)"
else
	build_jobs=2
fi

(
	cd "$source_dir"
	autoconf
	./configure --prefix="$install_prefix"
	make -j "$build_jobs"
	make install
)

if ! version_matches "$install_prefix/bin/verilator"; then
	# The exact-source marker is created only after installation; test the
	# binary directly on this first pass.
	reported="$($install_prefix/bin/verilator --version)"
	if [[ "$reported" != "Verilator $verilator_version "* ]]; then
		printf 'installed Verilator does not report version %s\n' \
			"$verilator_version" >&2
		exit 1
	fi
fi

# A cache or provisioned prefix is accepted only when this exact-source marker
# accompanies the expected version string.
touch "$install_marker"
"$install_prefix/bin/verilator" --version
