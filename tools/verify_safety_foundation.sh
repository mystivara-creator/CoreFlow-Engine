#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Kernel/resource mutation writes are restricted to dedicated actuators and the
# durable mutation journal. Bounded advisory experience persistence is also an
# approved storage path; it never writes kernel control files.
writers=""
while IFS= read -r f; do
  writers="$writers
$f"
done <<EOFWRITERS
$(grep -RIlE 'std::ofstream|open\([^\n]*O_WRONLY|::write\(' "$ROOT/src" --include='*.cpp' 2>/dev/null | sort)
EOFWRITERS

expected_list="
$ROOT/src/mutation/cpufreq_actuator.cpp
$ROOT/src/mutation/resource_actuator.cpp
$ROOT/src/conscience/mutation_journal.cpp
$ROOT/src/conscience/retained_state.cpp
$ROOT/src/runtime/main.cpp
$ROOT/src/understand/experience.cpp
$ROOT/src/conscience/decision_trace.cpp
$ROOT/src/runtime/status_snapshot.cpp
"

while IFS= read -r f; do
  [ -z "$f" ] && continue
  ok=0
  while IFS= read -r e; do
    [ -z "$e" ] && continue
    if [ "$f" = "$e" ]; then ok=1; break; fi
  done <<EOFEXP
$expected_list
EOFEXP
  if [ "$ok" -ne 1 ]; then
    echo "UNAPPROVED WRITE PATH: $f" >&2
    exit 1
  fi
done <<EOFWR
$writers
EOFWR

# Direct kernel-control paths must remain read-only outside the actuator.
if grep -RInE 'std::ofstream.*(/sys/|/proc/)|open\([^\n]*(/sys/|/proc/)[^\n]*(O_WRONLY|O_RDWR)|::write\([^\n]*(/sys/|/proc/)' "$ROOT/src" --include='*.cpp' 2>/dev/null; then
  echo "UNAPPROVED DIRECT KERNEL WRITE" >&2
  exit 1
fi

# The live status export (WebUI) is observation-only state. It may write only the engine's
# own state file, and nothing that decides or actuates may depend on it.
# RuntimePaths is header-defined after source consolidation.
grep -q 'p.status_json = p.root + "/status.json"' "$ROOT/include/coreflow/runtime_paths.hpp"
grep -q 'writeStatusFile(kPaths.status_json' "$ROOT/src/autonomous/autonomous.cpp"
if grep -RIn 'writeStatusFile(' "$ROOT/src" --include='*.cpp' | grep -v 'src/runtime/status_snapshot.cpp' | grep -v 'kPaths.status_json'; then
  echo "error: writeStatusFile must only be called with kPaths.status_json" >&2; exit 1
fi
for f in src/mutation/cpufreq_actuator.cpp src/mutation/resource_actuator.cpp src/mutation/mutation.cpp src/mutation/resource_mutation.cpp \
         src/balance/control.cpp src/balance/policy.cpp src/understand/context.cpp src/understand/effect_model.cpp \
         include/coreflow/actuator.hpp include/coreflow/control.hpp include/coreflow/mutation.hpp \
         include/coreflow/resource_mutation.hpp; do
  if grep -q 'status_snapshot' "$ROOT/$f"; then
    echo "error: $f must not depend on the status export (it carries no authority)" >&2; exit 1
  fi
done
if grep -qE '/sys/|/proc/' "$ROOT/src/runtime/status_snapshot.cpp"; then
  echo "error: status_snapshot.cpp must not reference kernel control paths" >&2; exit 1
fi

# Release defaults are observe-only: no governor writes and no arming until opt-in.
grep -q 'mutation_armed_{false}' "$ROOT/include/coreflow/config.hpp"
grep -q 'MutationMode mutation_mode_{MutationMode::Disabled}' "$ROOT/include/coreflow/config.hpp"
grep -q '^mutation_mode=observe$' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q 'mutation_armed=false' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q 'MutationPermit' "$ROOT/include/coreflow/actuator.hpp"
grep -q 'ResourceMutationController' "$ROOT/include/coreflow/resource_mutation.hpp"
grep -q 'ResourceActuator' "$ROOT/include/coreflow/resource_actuator.hpp"
# Sources are discovered by glob (cmake/CoreFlowSources.cmake), not listed by name.
# The safety contract therefore checks that discovery covers src/ and that the
# mutation controller is not excluded to the device-only set.
grep -q 'file(GLOB_RECURSE _candidates CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/\*.cpp")' "$ROOT/cmake/CoreFlowSources.cmake"
test -f "$ROOT/src/mutation/resource_mutation.cpp"
if grep -q '@coreflow-platform: android' "$ROOT/src/mutation/resource_mutation.cpp"; then
  echo 'resource_mutation.cpp must not be device-only' >&2; exit 1
fi
grep -q 'config.mutationArmed()' "$ROOT/src/mutation/mutation.cpp"
grep -q 'journalEntriesWith' "$ROOT/src/mutation/mutation.cpp"
grep -q 'result.writes_attempted != 0' "$ROOT/src/mutation/mutation.cpp"
grep -q 'result.status == ActuatorStatus::RolledBack' "$ROOT/src/mutation/mutation.cpp"
grep -q 'journal_was_committed' "$ROOT/src/mutation/mutation.cpp"

# Tier-2 contract: restores are gated on actual prior mutation, and safety holds
# are evaluated every tick.
grep -q 'MutationResult MutationController::relaxToBaseline' "$ROOT/src/mutation/mutation.cpp"
grep -q 'if (!mutated_ && !restore_failed_) return true;' "$ROOT/src/mutation/mutation.cpp"
grep -q 'retainLastMutation' "$ROOT/include/coreflow/mutation.hpp"
grep -q 'rollbackLastMutation' "$ROOT/include/coreflow/mutation.hpp"
grep -q 'RetainedStateStore' "$ROOT/include/coreflow/retained_state.hpp"
grep -q 'EFFICIENCY_BENEFICIAL action=RETAINED' "$ROOT/src/autonomous/autonomous.cpp"
grep -q 'evaluateSafetyHold({kPaths.kill_switch, kPaths.safe_mode})' "$ROOT/src/autonomous/autonomous.cpp"
grep -q 'BOOT_LOOP_DISABLE=5' "$ROOT/module/post-fs-data.sh"
grep -q 'SAFE_MODE set' "$ROOT/module/post-fs-data.sh"

# Thermal model integration contract: missing telemetry must not reach the
# model as zero, the feature builder is the only feature source, and the guard
# decisions run through the pure, tested helpers.
grep -q 'buildThermalFeatures(history, current_sample)' "$ROOT/src/understand/thermal_predictor.cpp"
grep -q 'if (features.complete)' "$ROOT/src/understand/thermal_predictor.cpp"
if grep -q 'batteryVoltageVolts\|trendEncoding' "$ROOT/src/understand/thermal_predictor.cpp"; then
  echo "error: thermal predictor must not carry its own feature encoding" >&2; exit 1
fi
grep -q 'thermalGuardShouldEnter' "$ROOT/src/autonomous/autonomous.cpp"
grep -q 'thermalGuardShouldHold' "$ROOT/src/autonomous/autonomous.cpp"
grep -q 'kHottestOnlyConfirmSamples' "$ROOT/include/coreflow/thermal_guard.hpp"
grep -q 'isPlausibleThermalMilli' "$ROOT/include/coreflow/thermal_limits.hpp"
grep -q 'file(GLOB COREFLOW_TEST_SOURCES CONFIGURE_DEPENDS' "$ROOT/CMakeLists.txt"
test -f "$ROOT/tests/release_hardening_tests.cpp"
# The pure feature/guard sources must stay free of Android/ONNX dependencies so
# they remain host-testable.
for f in src/understand/thermal_features.cpp src/understand/thermal_guard.cpp include/coreflow/thermal_features.hpp include/coreflow/thermal_guard.hpp include/coreflow/thermal_limits.hpp; do
  if grep -qE 'android/|onnxruntime' "$ROOT/$f"; then
    echo "error: $f must not depend on Android or ONNX headers" >&2; exit 1
  fi
done

echo "CoreFlow safety foundation contract: PASS"
