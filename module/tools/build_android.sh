#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"

if [ -z "$NDK" ] || [ ! -f "$NDK/build/cmake/android.toolchain.cmake" ]; then
    echo "ANDROID_NDK_HOME/ANDROID_NDK_ROOT must point to Android NDK 27.2.12479018." >&2
    exit 2
fi

cmake -S "$ROOT" -B "$ROOT/build/android-arm64" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-34 \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF

cmake --build "$ROOT/build/android-arm64" --parallel 2
