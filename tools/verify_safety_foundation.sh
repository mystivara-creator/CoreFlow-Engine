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

grep -q 'mutation_armed_{true}' "$ROOT/include/coreflow/config.hpp"
grep -q 'allow_cpu_governor_{true}' "$ROOT/include/coreflow/config.hpp"
grep -q 'mutation_armed=true' "$ROOT/module/system/etc/coreflow/default.conf"
grep -q 'allow_cpu_governor=yes' "$ROOT/module/system/etc/coreflow/default.conf"
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

echo "CoreFlow safety foundation contract: PASS"
