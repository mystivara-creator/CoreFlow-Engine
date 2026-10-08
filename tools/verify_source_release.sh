#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
VERSION="1.9.2"
MODULE_VERSION="v1.9.2"
VERSION_CODE="1902"

required_files="
CMakeLists.txt
module/module.prop
module/service.sh
module/system/etc/coreflow/default.conf
module/system/etc/coreflow/thermal_predictor.onnx
src/main.cpp
include/coreflow/environment.hpp
src/environment.cpp
include/coreflow/resource_model.hpp
src/resource_model.cpp
include/coreflow/context.hpp
src/context.cpp
include/coreflow/control.hpp
src/control.cpp
include/coreflow/outcome.hpp
src/outcome.cpp
README.md
CHANGELOG.md
BUILD_STATUS.md
"

for rel in $required_files; do
    [ -f "$ROOT/$rel" ] || { echo "error: missing required source file: $rel" >&2; exit 1; }
done

# A source package must not contain generated builds or production binaries.
[ ! -d "$ROOT/build" ] || { echo "error: generated build directory must not be shipped in source package" >&2; exit 1; }
[ ! -e "$ROOT/module/system/bin/coreflowd" ] || { echo "error: source package must not ship coreflowd" >&2; exit 1; }
[ ! -e "$ROOT/module/system/lib64/libonnxruntime.so" ] || { echo "error: source package must not ship libonnxruntime.so" >&2; exit 1; }
[ ! -e "$ROOT/third_party/jni/arm64-v8a/libonnxruntime.so" ] || { echo "error: extracted ONNX Runtime must not be shipped" >&2; exit 1; }

# Keep the source package small and deterministic: no local ONNX SDK cache.
[ ! -d "$ROOT/third_party/onnxruntime-android" ] || { echo "error: extracted ONNX Runtime cache must not be shipped" >&2; exit 1; }

grep -q '^version=v1.9.2$' "$ROOT/module/module.prop"
grep -q '^versionCode=1902$' "$ROOT/module/module.prop"
grep -q 'project(CoreFlowAutonomous VERSION 1.9.2 LANGUAGES CXX)' "$ROOT/CMakeLists.txt"
grep -q 'kCoreFlowVersion = "1.9.2"' "$ROOT/include/coreflow/config.hpp"
grep -q 'ResourceStateModel' "$ROOT/include/coreflow/resource_model.hpp"
grep -q 'ContextEngine' "$ROOT/include/coreflow/context.hpp"
grep -q 'PolicyEngine' "$ROOT/include/coreflow/control.hpp"
grep -q 'ActuatorManager' "$ROOT/include/coreflow/control.hpp"
grep -q 'DecisionOutcome' "$ROOT/include/coreflow/outcome.hpp"
grep -q '^mutation_mode=disabled$' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q '^allow_cpu_governor=no$' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q '^mutation_armed=false$' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q 'v1.9.2' "$ROOT/README.md"
grep -q 'v1.9.2' "$ROOT/BUILD_STATUS.md"

MODEL="$ROOT/module/system/etc/coreflow/thermal_predictor.onnx"
MODEL_SHA="$(sha256sum "$MODEL" | cut -d' ' -f1)"
EXPECTED_SHA="422c64313947688d1cda942f3a6f6612a103b3449991361d66a8bb1839fcab97"
[ "$MODEL_SHA" = "$EXPECTED_SHA" ] || { echo "error: thermal model digest mismatch" >&2; exit 1; }

printf '%s\n' "CoreFlow v${VERSION} source release checks: PASS"
