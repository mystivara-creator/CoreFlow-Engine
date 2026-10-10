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
$ROOT/src/cpufreq_actuator.cpp
$ROOT/src/resource_actuator.cpp
$ROOT/src/mutation_journal.cpp
$ROOT/src/main.cpp
$ROOT/src/experience.cpp
$ROOT/src/decision_trace.cpp
$ROOT/src/status_snapshot.cpp
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
grep -q 'kStatusPath = "/data/adb/coreflow/status.json"' "$ROOT/src/autonomous.cpp"
if grep -RIn 'writeStatusFile(' "$ROOT/src" --include='*.cpp' | grep -v 'src/status_snapshot.cpp' | grep -v 'kStatusPath'; then
  echo "error: writeStatusFile must only be called with kStatusPath" >&2; exit 1
fi
for f in src/cpufreq_actuator.cpp src/resource_actuator.cpp src/mutation.cpp src/resource_mutation.cpp \
         src/control.cpp src/policy.cpp src/context.cpp src/effect_model.cpp \
         include/coreflow/actuator.hpp include/coreflow/control.hpp include/coreflow/mutation.hpp \
         include/coreflow/resource_mutation.hpp; do
  if grep -q 'status_snapshot' "$ROOT/$f"; then
    echo "error: $f must not depend on the status export (it carries no authority)" >&2; exit 1
  fi
done
if grep -qE '/sys/|/proc/' "$ROOT/src/status_snapshot.cpp"; then
  echo "error: status_snapshot.cpp must not reference kernel control paths" >&2; exit 1
fi

# Release defaults are observe-only: no governor writes and no arming until opt-in.
grep -q 'mutation_armed_{false}' "$ROOT/include/coreflow/config.hpp"
grep -q 'allow_cpu_governor_{false}' "$ROOT/include/coreflow/config.hpp"
grep -q 'MutationMode mutation_mode_{MutationMode::Disabled}' "$ROOT/include/coreflow/config.hpp"
grep -q '^mutation_mode=observe$' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q 'mutation_armed=false' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q 'allow_cpu_governor=no' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q 'MutationPermit' "$ROOT/include/coreflow/actuator.hpp"
grep -q 'ResourceMutationController' "$ROOT/include/coreflow/resource_mutation.hpp"
grep -q 'ResourceActuator' "$ROOT/include/coreflow/resource_actuator.hpp"
grep -q 'resource_mutation.cpp' "$ROOT/CMakeLists.txt"
grep -q 'config.mutationArmed()' "$ROOT/src/mutation.cpp"
grep -q 'journalEntriesWith' "$ROOT/src/mutation.cpp"
grep -q 'result.writes_attempted != 0' "$ROOT/src/mutation.cpp"
grep -q 'result.status == ActuatorStatus::RolledBack' "$ROOT/src/mutation.cpp"
grep -q 'journal_was_committed' "$ROOT/src/mutation.cpp"

# Tier-2 contract: restores are gated on actual prior mutation, and safety holds
# are evaluated every tick.
grep -q 'MutationResult MutationController::relaxToBaseline' "$ROOT/src/mutation.cpp"
grep -q 'if (!mutated_ && !restore_failed_) return true;' "$ROOT/src/mutation.cpp"
grep -q 'kMaxMutationHoldCycles' "$ROOT/include/coreflow/mutation.hpp"
grep -q 'evaluateSafetyHold({kKillSwitchPath, kSafeModePath})' "$ROOT/src/autonomous.cpp"
grep -q 'BOOT_LOOP_DISABLE=5' "$ROOT/module/post-fs-data.sh"
grep -q 'SAFE_MODE set' "$ROOT/module/post-fs-data.sh"

# Thermal model integration contract: missing telemetry must not reach the
# model as zero, the feature builder is the only feature source, and the guard
# decisions run through the pure, tested helpers.
grep -q 'buildThermalFeatures(history, current_sample)' "$ROOT/src/thermal_predictor.cpp"
grep -q 'if (!built.complete)' "$ROOT/src/thermal_predictor.cpp"
if grep -q 'batteryVoltageVolts\|trendEncoding' "$ROOT/src/thermal_predictor.cpp"; then
  echo "error: thermal predictor must not carry its own feature encoding" >&2; exit 1
fi
grep -q 'thermalGuardShouldEnter' "$ROOT/src/autonomous.cpp"
grep -q 'thermalGuardShouldHold' "$ROOT/src/autonomous.cpp"
grep -q 'kHottestOnlyConfirmSamples' "$ROOT/include/coreflow/thermal_guard.hpp"
grep -q 'isPlausibleThermalMilli' "$ROOT/include/coreflow/thermal_limits.hpp"
grep -q 'coreflow_release_hardening' "$ROOT/CMakeLists.txt"
# The pure feature/guard sources must stay free of Android/ONNX dependencies so
# they remain host-testable.
for f in src/thermal_features.cpp src/thermal_guard.cpp include/coreflow/thermal_features.hpp include/coreflow/thermal_guard.hpp include/coreflow/thermal_limits.hpp; do
  if grep -qE 'android/|onnxruntime' "$ROOT/$f"; then
    echo "error: $f must not depend on Android or ONNX headers" >&2; exit 1
  fi
done

echo "CoreFlow safety foundation contract: PASS"
