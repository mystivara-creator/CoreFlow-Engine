#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
ANDROID_MIN_API="${ANDROID_MIN_API:-35}"
ANDROID_ABI="${ANDROID_ABI:-arm64-v8a}"

if [ -z "$NDK" ] || [ ! -f "$NDK/build/cmake/android.toolchain.cmake" ]; then
    echo "error: ANDROID_NDK_HOME or ANDROID_NDK_ROOT must be set." >&2
    exit 2
fi

case "$ANDROID_MIN_API" in
    ''|*[!0-9]*)
        echo "error: ANDROID_MIN_API must be a numeric Android API level >= 35 (got: $ANDROID_MIN_API)." >&2
        exit 2
        ;;
esac
if [ "$ANDROID_MIN_API" -lt 35 ]; then
    echo "error: Android 15 / API 35+ is required (got: $ANDROID_MIN_API)." >&2
    exit 2
fi
if [ "$ANDROID_ABI" != "arm64-v8a" ]; then
    echo "error: CoreFlow Android builds currently support arm64-v8a only (got: $ANDROID_ABI)." >&2
    exit 2
fi

for tool in cmake ninja cargo; do
    command -v "$tool" >/dev/null 2>&1 || { echo "error: required build tool is missing: $tool" >&2; exit 2; }
done
if command -v rustup >/dev/null 2>&1; then
    rustup target add aarch64-linux-android
fi
BUILD_DIR="$ROOT/build/android-arm64"

echo "CFAE System Framework v4.0.0 Android build: min API=${ANDROID_MIN_API}, ABI=${ANDROID_ABI}"

cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI="$ANDROID_ABI" \
  -DANDROID_PLATFORM="android-${ANDROID_MIN_API}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_FLOW_BUILD_TESTS=OFF

cmake --build "$BUILD_DIR" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-$(nproc 2>/dev/null || echo 2)}"

BINARY="$(find "$BUILD_DIR" -type f -name coreflowd -perm -u+x | head -n 1)"
if [ -z "$BINARY" ] || [ ! -x "$BINARY" ]; then
    echo "error: coreflowd was not produced." >&2
    exit 1
fi
VK_LAYER_SO="$(find "$BUILD_DIR" -type f -name libVkLayer_coreflow.so | head -n 1)"
if [ -z "$VK_LAYER_SO" ] || [ ! -s "$VK_LAYER_SO" ]; then
    echo "error: libVkLayer_coreflow.so was not produced." >&2
    exit 1
fi

echo "Android ARM64 build: PASS"
echo "Binary: $BINARY"
echo "Vulkan layer: $VK_LAYER_SO"
