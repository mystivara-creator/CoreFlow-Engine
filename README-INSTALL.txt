CoreFlow C++ Quality Gate (patch bundle)

Files:
- tools/cpp_quality_gate.sh
- .github/workflows/cpp-quality-gate.yml

Install from the repository root:
  unzip -o coreflow-cpp-quality-gate.zip -d .

Then inspect:
  git diff --check
  git status --short

Run locally on a machine with CMake, a C++20 compiler, clang-tidy, and cppcheck:
  bash tools/cpp_quality_gate.sh

Reports:
  artifacts/cpp-quality/summary.md
  artifacts/cpp-quality/clang-tidy.txt
  artifacts/cpp-quality/cppcheck.txt

Notes:
- The script builds the existing host test targets before static analysis.
- The CI job is independent of the Android NDK release build.
- Findings fail the job; existing source may surface pre-existing findings on the first run.
- This is static analysis, not an AI model: it can find known classes of defects and explain checker diagnostics, but cannot reliably infer all project-specific logic bugs or generate guaranteed-correct patches.
