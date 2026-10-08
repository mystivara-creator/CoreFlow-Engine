#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
ONNX_ROOT="${COREFLOW_ONNX_ROOT:-}"
# Reproducible default toolchain, while allowing deliberate overrides.
EXPECTED_NDK="${EXPECTED_NDK:-30.0.16248370}"
ANDROID_MIN_API="${ANDROID_MIN_API:-34}"
ANDROID_ABI="${ANDROID_ABI:-arm64-v8a}"

if [ -z "$NDK" ] || [ ! -f "$NDK/build/cmake/android.toolchain.cmake" ]; then
    echo "error: ANDROID_NDK_HOME/ANDROID_NDK_ROOT must point to Android NDK ${EXPECTED_NDK}." >&2
    exit 2
fi

case "$ANDROID_MIN_API" in
    ''|*[!0-9]*)
        echo "error: ANDROID_MIN_API must be a numeric API level." >&2
        exit 2
        ;;
esac

if [ "$ANDROID_MIN_API" -lt 34 ]; then
    echo "error: CoreFlow requires Android API 34 or newer." >&2
    exit 2
fi

NDK_REV="$(awk -F= '$1=="Pkg.Revision" {print $2; exit}' "$NDK/source.properties" 2>/dev/null || true)"
if [ "$NDK_REV" != "$EXPECTED_NDK" ]; then
    echo "error: expected Android NDK ${EXPECTED_NDK}, found ${NDK_REV:-unknown}." >&2
    exit 2
fi

if [ -z "$ONNX_ROOT" ]; then
    echo "error: COREFLOW_ONNX_ROOT must point to an extracted ONNX Runtime Android package." >&2
    echo "       Expected: headers/onnxruntime_cxx_api.h and jni/${ANDROID_ABI}/libonnxruntime.so" >&2
    exit 2
fi

if [ ! -f "$ONNX_ROOT/headers/onnxruntime_cxx_api.h" ]; then
    echo "error: ONNX Runtime headers not found under $ONNX_ROOT/headers" >&2
    exit 2
fi
if [ ! -s "$ONNX_ROOT/jni/${ANDROID_ABI}/libonnxruntime.so" ]; then
    echo "error: ONNX Runtime library not found under $ONNX_ROOT/jni/${ANDROID_ABI}" >&2
    exit 2
fi

BUILD_DIR="$ROOT/build/android-arm64"

echo "CoreFlow Android build: NDK=${EXPECTED_NDK}, min API=${ANDROID_MIN_API}, ABI=${ANDROID_ABI}"

cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI="$ANDROID_ABI" \
  -DANDROID_PLATFORM="android-${ANDROID_MIN_API}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_FLOW_BUILD_TESTS=OFF \
  -DCOREFLOW_ONNX_ROOT="$ONNX_ROOT"

cmake --build "$BUILD_DIR" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"

BINARY="$BUILD_DIR/coreflowd"
if [ ! -x "$BINARY" ]; then
    BINARY="$(find "$BUILD_DIR" -type f -name coreflowd -perm -111 | head -n 1)"
fi
if [ -z "$BINARY" ] || [ ! -x "$BINARY" ]; then
    echo "error: coreflowd was not produced." >&2
    exit 1
fi

file "$BINARY"
file "$BINARY" | grep -E 'ELF 64-bit.*ARM aarch64' >/dev/null

echo "Android ARM64 build: PASS"
echo "Binary: $BINARY"
