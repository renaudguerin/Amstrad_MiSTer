#!/usr/bin/env bash
set -euo pipefail

# Install upstream GHDL binaries on Apple Silicon macOS.
# The Homebrew cask is disabled (fails the macOS Gatekeeper check: upstream
# releases are ad-hoc signed, not Apple notarized), so fetch the same tarball
# brew used, verify its checksum, and link it into the brew prefix.
# Upstream 6.0.0 ships macOS builds for arm64 only.
readonly ghdl_version="6.0.0"
readonly ghdl_macos_version="15"
readonly ghdl_arch="aarch64"
readonly ghdl_dir="ghdl-llvm-${ghdl_version}-macos${ghdl_macos_version}-${ghdl_arch}"
readonly ghdl_url="https://github.com/ghdl/ghdl/releases/download/v${ghdl_version}/${ghdl_dir}.tar.gz"
readonly ghdl_sha256="69beb5913a490b5971980bd045af6a63b6f52e861b444fa990bffac436587478"
install_root="${GHDL_INSTALL_ROOT:-/opt/homebrew/opt/ghdl-manual}"

if command -v brew >/dev/null 2>&1; then
	brew_prefix="$(brew --prefix)"
else
	brew_prefix="/opt/homebrew"
fi

version_matches() {
	local binary="$1"
	local reported

	[[ -x "$binary" ]] || return 1
	reported="$("$binary" --version 2>/dev/null)"
	[[ "$reported" == "GHDL $ghdl_version "* ]]
}

if version_matches "$brew_prefix/bin/ghdl"; then
	printf 'GHDL %s already installed under %s\n' "$ghdl_version" "$brew_prefix"
	exit 0
fi

if [[ "${1:-}" == "--check" ]]; then
	exit 1
elif [[ $# -ne 0 ]]; then
	printf 'usage: %s [--check]\n' "$0" >&2
	exit 2
fi

if [[ "$(uname -s)" != "Darwin" || "$(uname -m)" != "arm64" ]]; then
	printf 'this script installs the %s macOS arm64 build only (uname: %s %s)\n' \
		"$ghdl_dir" "$(uname -s)" "$(uname -m)" >&2
	exit 1
fi

for command_name in curl shasum tar xattr; do
	if ! command -v "$command_name" >/dev/null 2>&1; then
		printf 'missing prerequisite: %s\n' "$command_name" >&2
		exit 1
	fi
done

download_root="$(mktemp -d "${TMPDIR:-/tmp}/ghdl-${ghdl_version}.XXXXXX")"
trap 'rm -rf -- "$download_root"' EXIT
archive="$download_root/${ghdl_dir}.tar.gz"

curl -fL -o "$archive" "$ghdl_url"
printf '%s  %s\n' "$ghdl_sha256" "$archive" | shasum -a 256 -c -

mkdir -p "$install_root"
tar xzf "$archive" -C "$install_root"
xattr -dr com.apple.quarantine "$install_root/$ghdl_dir"

ln -sf "$install_root/$ghdl_dir/bin/ghdl" "$brew_prefix/bin/ghdl"
ln -sf "$install_root/$ghdl_dir/bin/ghwdump" "$brew_prefix/bin/ghwdump"
ln -sf "$install_root/$ghdl_dir/bin/ghdl1-llvm" "$brew_prefix/bin/ghdl1-llvm"
ln -sfn "$install_root/$ghdl_dir/include/ghdl" "$brew_prefix/include/ghdl"
ln -sfn "$install_root/$ghdl_dir/lib/ghdl" "$brew_prefix/lib/ghdl"

version_matches "$brew_prefix/bin/ghdl"
"$brew_prefix/bin/ghdl" --version
