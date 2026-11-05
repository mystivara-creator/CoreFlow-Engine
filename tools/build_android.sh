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

command -v cargo >/dev/null || { echo "error: Rust 1.85+ and cargo are required" >&2; exit 2; }
rustup target add aarch64-linux-android
BUILD_DIR="$ROOT/build/android-arm64"

echo "CFAE System Framework v3.1.0 Android build: min API=${ANDROID_MIN_API}, ABI=${ANDROID_ABI}"

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

echo "Android ARM64 build: PASS"
echo "Binary: $BINARY"
