#!/bin/sh
# Builds real status snapshots from the C++ code and runs the WebUI contract test.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
if ! command -v node >/dev/null 2>&1; then echo "SKIP: node not installed; WebUI status contract NOT verified"; exit 0; fi
if ! command -v g++ >/dev/null 2>&1; then echo "SKIP: g++ not installed; WebUI status contract NOT verified"; exit 0; fi
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
g++ -std=c++17 -Wall -Wextra -Werror -I"$ROOT/include" \
    "$ROOT/tests/status_dump.cpp" "$ROOT/src/status_snapshot.cpp" -o "$TMP/status_dump"
mkdir -p "$TMP/out"
"$TMP/status_dump" "$TMP/out"
node "$ROOT/tools/test_webui_status.js" "$TMP/out"
node "$ROOT/tools/test_webui_render.js" "$TMP/out"
