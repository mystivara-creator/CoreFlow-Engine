#!/usr/bin/env bash
set -euo pipefail

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
VERSION="$(sed -n 's/^project(CoreFlowAutonomous VERSION \([0-9.]*\) LANGUAGES CXX).*$/\1/p' "$ROOT/CMakeLists.txt" | head -n1)"

required_files="
CMakeLists.txt
module/module.prop
module/update.json
.github/workflows/release-update-feed.yml
module/service.sh
module/system/etc/coreflow/default.conf
module/system/etc/coreflow/thermal_predictor.onnx
src/main.cpp
include/coreflow/environment.hpp
src/environment.cpp
include/coreflow/experience.hpp
src/experience.cpp
include/coreflow/resource_model.hpp
src/resource_model.cpp
include/coreflow/context.hpp
src/context.cpp
include/coreflow/control.hpp
src/control.cpp
include/coreflow/outcome.hpp
src/outcome.cpp
include/coreflow/thermal_limits.hpp
include/coreflow/thermal_features.hpp
src/thermal_features.cpp
include/coreflow/thermal_guard.hpp
src/thermal_guard.cpp
tests/release_hardening_tests.cpp
include/coreflow/status_snapshot.hpp
src/status_snapshot.cpp
tests/status_dump.cpp
tools/test_webui_status.sh
tools/test_webui_status.js
tools/test_webui_render.js
module/webroot/engine_status.js
README.md
CHANGELOG.md
BUILD_STATUS.md
"

for rel in $required_files; do
    [ -f "$ROOT/$rel" ] || { echo "error: missing required source file: $rel" >&2; exit 1; }
done

# A source package must not contain generated builds or production binaries.
if [ "${COREFLOW_ALLOW_BUILD_DIR:-0}" != "1" ]; then
    [ ! -d "$ROOT/build" ] || { echo "error: generated build directory must not be shipped in source package" >&2; exit 1; }
fi
[ ! -e "$ROOT/module/system/bin/coreflowd" ] || { echo "error: source package must not ship coreflowd" >&2; exit 1; }
[ ! -e "$ROOT/module/system/lib64/libonnxruntime.so" ] || { echo "error: source package must not ship libonnxruntime.so" >&2; exit 1; }
[ ! -e "$ROOT/third_party/jni/arm64-v8a/libonnxruntime.so" ] || { echo "error: extracted ONNX Runtime must not be shipped" >&2; exit 1; }

# Keep the source package small and deterministic: no local ONNX SDK cache.
[ ! -d "$ROOT/third_party/onnxruntime-android" ] || { echo "error: extracted ONNX Runtime cache must not be shipped" >&2; exit 1; }

MODULE_VERSION="$(awk -F= '$1=="version" {print $2; exit}' "$ROOT/module/module.prop" | tr -d '[:space:]')"
MODULE_CODE="$(awk -F= '$1=="versionCode" {print $2; exit}' "$ROOT/module/module.prop" | tr -d '[:space:]')"
CMAKE_VERSION="$(sed -n 's/^project(CoreFlowAutonomous VERSION \([0-9.]*\) LANGUAGES CXX).*$/\1/p' "$ROOT/CMakeLists.txt" | head -n1)"
CONFIG_VERSION="$(sed -n 's/.*kCoreFlowVersion = "\([^"]*\)".*/\1/p' "$ROOT/include/coreflow/config.hpp" | head -n1)"
[[ "$CMAKE_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
EXPECTED_CODE="$(awk -F. '{printf "%d", $1 * 1000 + $2 * 100 + $3 * 10}' <<< "$CMAKE_VERSION")"
[ "$MODULE_VERSION" = "v${CMAKE_VERSION}" ] || { echo "error: module version does not match CMake version" >&2; exit 1; }
[ "$MODULE_CODE" = "$EXPECTED_CODE" ] || { echo "error: versionCode must be ${EXPECTED_CODE} for ${CMAKE_VERSION}" >&2; exit 1; }
[ "$CONFIG_VERSION" = "$CMAKE_VERSION" ] || { echo "error: runtime version does not match CMake version" >&2; exit 1; }
grep -q 'set(CMAKE_CXX_STANDARD 20)' "$ROOT/CMakeLists.txt"
grep -q 'target_compile_features(coreflow_compile_options INTERFACE cxx_std_20)' "$ROOT/CMakeLists.txt"
grep -q '^updateJson=https://github.com/mystivara-creator/CoreFlow-Engine/releases/latest/download/update.json$' "$ROOT/module/module.prop"
python3 - "$ROOT/module/update.json" "$MODULE_VERSION" "$MODULE_CODE" <<'PYEOF'
import json, sys
path, expected_version, expected_code = sys.argv[1:]
with open(path, encoding="utf-8") as handle:
    data = json.load(handle)
assert data.get("version") == expected_version, "update.json version must match module.prop"
assert data.get("versionCode") == int(expected_code), "update.json versionCode must match module.prop"
assert data.get("zipUrl", "").startswith("https://github.com/mystivara-creator/CoreFlow-Engine/releases/download/"), "zipUrl must point to an exact GitHub release asset"
assert data.get("changelog", "").startswith("https://raw.githubusercontent.com/mystivara-creator/CoreFlow-Engine/"), "changelog URL must be public raw GitHub content"
PYEOF
grep -q 'ResourceStateModel' "$ROOT/include/coreflow/resource_model.hpp"
grep -q 'ContextEngine' "$ROOT/include/coreflow/context.hpp"
grep -q 'PolicyEngine' "$ROOT/include/coreflow/control.hpp"
grep -q 'ActuatorManager' "$ROOT/include/coreflow/control.hpp"
grep -q 'DecisionOutcome' "$ROOT/include/coreflow/outcome.hpp"
# Release default is observe-only; kernel mutation requires explicit opt-in.
grep -q '^mutation_mode=observe$' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q '^allow_cpu_governor=no$' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q '^mutation_armed=false$' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q 'v2.1.0' "$ROOT/README.md"
grep -q 'v2.1.0' "$ROOT/BUILD_STATUS.md"
grep -q 'v2.1.0' "$ROOT/CHANGELOG.md"

MODEL="$ROOT/module/system/etc/coreflow/thermal_predictor.onnx"
MODEL_SHA="$(sha256sum "$MODEL" | cut -d' ' -f1)"
EXPECTED_SHA="a605b046086f2f30e0a625c64547de5399e407887d69461d3c5642f7344f0703"
[ "$MODEL_SHA" = "$EXPECTED_SHA" ] || { echo "error: thermal model digest mismatch" >&2; exit 1; }

# 18-feature contract: the Python training order and the C++ feature header
# comment must list exactly the same names in the same order.
python3 - "$ROOT" <<'PYEOF'
import re, sys
root = sys.argv[1]
py = open(root + "/tools/train_coreflow_thermal_predictor_v1_2_18f.py").read()
block = py.split("FEATURE_NAMES = [", 1)[1].split("]", 1)[0]
py_names = re.findall(r'"([a-z_0-9]+)"', block)
hdr = open(root + "/include/coreflow/thermal_features.hpp").read()
body = hdr.split("//   0", 1)[1].split("inline constexpr", 1)[0]
pairs = re.findall(r"(\d+)\s+([a-z_0-9]+)", "0" + body)
hdr_names = [n for _, n in sorted((int(i), n) for i, n in pairs)]
if len(py_names) != 18 or py_names != hdr_names:
    sys.exit("error: 18-feature order mismatch between Python and C++")
PYEOF

# The WebUI must understand what the C++ engine writes (needs node and g++; says so when skipped).
sh "$ROOT/tools/test_webui_status.sh"

printf '%s\n' "CoreFlow v${VERSION} source release checks: PASS"
