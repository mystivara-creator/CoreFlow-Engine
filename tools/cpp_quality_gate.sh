#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="${COREFLOW_QUALITY_BUILD_DIR:-$ROOT/build/cpp-quality}"
REPORT_DIR="${COREFLOW_QUALITY_REPORT_DIR:-$ROOT/artifacts/cpp-quality}"
mkdir -p "$REPORT_DIR"

for tool in cmake clang-tidy cppcheck; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'error: required tool not found: %s\n' "$tool" >&2
        exit 2
    fi
done

printf '== CoreFlow C++ Source Quality Audit ==\n'
printf 'Source: %s\nBuild:  %s\n' "$ROOT" "$BUILD_DIR"
clang-tidy --version | head -n1
cppcheck --version

# Reuse the project's real CMake source discovery and C++20 compile options.
cmake -S "$ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCORE_FLOW_BUILD_TESTS=ON

if [[ "${COREFLOW_SKIP_BUILD_TESTS:-0}" != "1" ]]; then
    cmake --build "$BUILD_DIR" --parallel "${COREFLOW_BUILD_JOBS:-2}"
    ctest --test-dir "$BUILD_DIR" --output-on-failure --no-tests=error
else
    printf '\\nSkipping build/tests: static-analysis-only mode.\\n'
fi

# Clang-Tidy must use the exact compile commands CMake generated. This avoids
# inventing host flags for Android-only translation units that are not host-built.
mapfile -t SOURCES < <(python3 - "$BUILD_DIR/compile_commands.json" <<'PY'
import json, os, sys
db = json.load(open(sys.argv[1], encoding="utf-8"))
for entry in sorted(db, key=lambda item: item["file"]):
    path = os.path.realpath(entry["file"])
    if path.endswith(".cpp"):
        print(path)
PY
)
if ((${#SOURCES[@]} == 0)); then
    echo "error: compilation database contains no C++ translation units" >&2
    exit 2
fi

# Cppcheck also scans Android-only source files. Missing platform headers are
# suppressed here because the Android NDK is validated by the existing build job.
mapfile -t ALL_SOURCES < <(find "$ROOT/src" "$ROOT/tests" -type f -name '*.cpp' -print | sort)

TIDY_LOG="$REPORT_DIR/clang-tidy.txt"
CPPCHECK_LOG="$REPORT_DIR/cppcheck.txt"
SUMMARY="$REPORT_DIR/summary.md"

printf '\n== Clang-Tidy: bugprone, analyzer, and performance checks ==\n'
set +e
clang-tidy -p "$BUILD_DIR" \
    -checks='-*,bugprone-*,clang-analyzer-*,performance-*' \
    -header-filter='^.*/include/coreflow/.*' \
    "${SOURCES[@]}" 2>&1 | tee "$TIDY_LOG"
TIDY_STATUS=${PIPESTATUS[0]}

printf '\n== Cppcheck: warning, performance, portability ==\n'
cppcheck \
    --enable=warning,performance,portability \
    --inline-suppr \
    --suppress=missingIncludeSystem \
    --suppress=missingInclude \
    --suppress=unmatchedSuppression \
    --error-exitcode=0 \
    --language=c++ \
    --std=c++20 \
    -I "$ROOT/include" \
    --file-list=<(printf '%s\n' "${ALL_SOURCES[@]}") \
    2>&1 | tee "$CPPCHECK_LOG"
CPPCHECK_STATUS=${PIPESTATUS[0]}
set -e

{
    echo "# CoreFlow C++ Quality Audit"
    echo
    echo "- C++ files scanned: ${#SOURCES[@]}"
    echo "- Clang-Tidy exit code: $TIDY_STATUS"
    echo "- Cppcheck exit code: $CPPCHECK_STATUS"
    echo
    echo "## How to interpret findings"
    echo
    echo "- **Clang-Tidy** checks bug-prone patterns, Clang static-analyzer diagnostics, and performance hazards."
    echo "- **Cppcheck** checks additional control-flow, portability, and performance patterns."
    echo "- Each diagnostic includes source location and checker ID; use the checker ID to find the relevant rule and apply its suggested fix only after reviewing surrounding code."
    echo "- These tools cannot prove arbitrary business-logic correctness or replace code review and behavioral tests."
    echo
    echo "## Clang-Tidy output"
    echo
    echo '```text'
    cat "$TIDY_LOG"
    echo '```'
    echo
    echo "## Cppcheck output"
    echo
    echo '```text'
    cat "$CPPCHECK_LOG"
    echo '```'
} > "$SUMMARY"

printf '\nReports written to: %s\n' "$REPORT_DIR"
if ((TIDY_STATUS != 0 || CPPCHECK_STATUS != 0)); then
    echo "CoreFlow C++ quality audit: FAIL (inspect summary.md and logs)"
    exit 1
fi

echo "CoreFlow C++ quality audit: PASS"
