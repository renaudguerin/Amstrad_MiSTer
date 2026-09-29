#!/bin/bash
# Build the standalone MID FRAME replay from unchanged production sources.
# May be invoked from any working directory.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
RESULT="$ROOT/sim/diagnostics/shaker_b9_mid"
OBJ="$ROOT/.roster-scratch/b9-mid/obj"
mkdir -p "$OBJ/t80"
# 1. Production T80pa netlist via GHDL (same sources/flags as sim/Makefile)
ghdl --synth --std=08 -fsynopsys --workdir="$OBJ/t80" --out=verilog \
  "$ROOT/rtl/T80/T80_Pack.vhd" "$ROOT/rtl/T80/T80_ALU.vhd" "$ROOT/rtl/T80/T80_MCode.vhd" \
  "$ROOT/rtl/T80/T80_Reg.vhd" "$ROOT/rtl/T80/T80.vhd" "$ROOT/rtl/T80/T80pa.vhd" \
  -e T80pa > "$OBJ/t80/T80pa.v.tmp" && mv "$OBJ/t80/T80pa.v.tmp" "$OBJ/t80/T80pa.v"
# 2. Standalone bench (mirrors the CRTC_T80_BIN recipe in sim/Makefile)
verilator --cc --exe --build -j 4 --top-module b9_t80_top -UVERILATOR \
  --Mdir "$OBJ" -Wno-fatal -CFLAGS "-std=c++17 -O2" \
  -o b9_t80_tests \
  "$RESULT/b9_t80_top.sv" "$OBJ/t80/T80pa.v" \
  "$ROOT/rtl/CRTC.v" "$ROOT/rtl/crtc_type0_engine.v" "$ROOT/rtl/crtc_type1_engine.v" \
  "$ROOT/rtl/GA40010/ga40010.sv" "$ROOT/rtl/GA40010/syncgen.v" \
  "$ROOT/rtl/GA40010/syncgen_sync.v" "$ROOT/rtl/GA40010/casgen.v" \
  "$ROOT/rtl/GA40010/casgen_sync.v" "$ROOT/rtl/GA40010/video.sv" \
  "$ROOT/rtl/GA40010/rslatch.v" \
  "$RESULT/b9_t80_test.cpp"
echo "BUILD_OK $OBJ/b9_t80_tests"
