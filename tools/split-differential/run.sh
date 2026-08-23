#!/bin/sh
# Reproducible lockstep differential check: frozen pre-split reference core vs
# the frozen per-type-split integration tip, compared after every CLKEN edge.
#
# Usage: tools/split-differential/run.sh [REF_COMMIT] [SPLIT_COMMIT]
#   REF_COMMIT defaults to 418aa68 (last pre-split commit of the wrapper).
#   SPLIT_COMMIT defaults to 2d4f880 (reviewed split integration tip).
# Requires: verilator 5+, C++17 compiler. Runtime ~15 s, no traces written.
set -eu

REF_COMMIT=${1:-418aa68}
SPLIT_COMMIT=${2:-2d4f880}
HERE=$(cd "$(dirname "$0")" && pwd)
WORK=${TMPDIR:-/tmp}/split-diff-$$
mkdir -p "$WORK"

echo "== reference extraction: git show $REF_COMMIT:rtl/UM6845R.v"
git -C "$HERE/../.." show "$REF_COMMIT:rtl/UM6845R.v" \
	| sed 's/module UM6845R$/module UM6845R_REF/' > "$WORK/um6845r_ref.v"

echo "== split extraction: git show $SPLIT_COMMIT:rtl/{CRTC.v,crtc_type0_engine.v,crtc_type1_engine.v}"
git -C "$HERE/../.." show "$SPLIT_COMMIT:rtl/CRTC.v" > "$WORK/CRTC.v"
git -C "$HERE/../.." show "$SPLIT_COMMIT:rtl/crtc_type0_engine.v" > "$WORK/crtc_type0_engine.v"
git -C "$HERE/../.." show "$SPLIT_COMMIT:rtl/crtc_type1_engine.v" > "$WORK/crtc_type1_engine.v"

echo "== verilator --cc (split model + reference)"
verilator --cc -Wno-fatal --top-module CRTC \
	"$WORK/CRTC.v" "$WORK/crtc_type0_engine.v" \
	"$WORK/crtc_type1_engine.v" --Mdir "$WORK/objA"
verilator --cc -Wno-fatal --top-module UM6845R_REF "$WORK/um6845r_ref.v" \
	--Mdir "$WORK/objB"

VINC=$(dirname "$(command -v verilator)")/../share/verilator/include
test -f "$VINC/verilated.h" || VINC="${VERILATOR_ROOT:?set VERILATOR_ROOT}/include"

echo "== build"
g++ -std=c++17 -O2 -o "$WORK/difftest" "$HERE/diff_main.cpp" "$HERE/tstamp.cpp" \
	"$WORK"/objA/*.cpp "$WORK"/objB/*.cpp \
	"$VINC/verilated.cpp" "$VINC/verilated_threads.cpp" \
	-I"$VINC" -I"$WORK/objA" -I"$WORK/objB"

echo "== run (compares full state via memcmp after every CLKEN edge)"
"$WORK/difftest"
