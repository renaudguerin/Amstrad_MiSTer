#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PAYLOAD="${1:-$ROOT/local/task-archives/plus-compatibility-2026-09-23/66d3/.roster-scratch/b9-result/SHAKE27B.BIN}"
EXPECTED=c34e2fcc273ab427baf9aeb84bcfa1fe6144565c0be6dee2241a8d99f4f2dc88
[[ -f "$PAYLOAD" ]] || { echo "Missing private header-stripped payload: $PAYLOAD" >&2; exit 2; }
ACTUAL="$(shasum -a 256 "$PAYLOAD" | cut -d' ' -f1)"
[[ "$ACTUAL" == "$EXPECTED" ]] || { echo "Private payload SHA-256 mismatch: $ACTUAL" >&2; exit 2; }
mkdir -p "$ROOT/.roster-scratch/b9-mid"
if ! bash "$ROOT/sim/diagnostics/shaker_b9_mid/build.sh" > "$ROOT/.roster-scratch/b9-mid/build.log" 2>&1; then
  tail -40 "$ROOT/.roster-scratch/b9-mid/build.log" >&2
  exit 1
fi
"$ROOT/.roster-scratch/b9-mid/obj/b9_t80_tests" "$PAYLOAD" 2>&1 | tee "$ROOT/.roster-scratch/b9-mid/run.log"
