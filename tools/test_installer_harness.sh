#!/bin/sh
# Host harness for module/customize.sh. Stubs the Magisk installer API.
SRC="${COREFLOW_MODULE_SRC:-$(cd "$(dirname "$0")/../module" && pwd)}"
T=$(mktemp -d)
# Make the volume-key timeout deterministic and instant in the host harness.
# The production installer still uses the platform `timeout` and `getevent`.
mkdir -p "$T/bin"
cat > "$T/bin/timeout" <<'TIMEOUT_STUB'
#!/bin/sh
exit 124
TIMEOUT_STUB
cat > "$T/bin/date" <<'DATE_STUB'
#!/bin/sh
# Advance synthetic epoch on each call so volume_choice reaches timeout instantly.
[ "${1:-}" = "+%s" ] || exec /usr/bin/date "$@"
COUNT_FILE="${COREFLOW_TEST_DATE_COUNTER:-/tmp/coreflow-test-date-counter}"
COUNT=0
[ ! -f "$COUNT_FILE" ] || COUNT=$(cat "$COUNT_FILE")
COUNT=$((COUNT + 1))
printf '%s\n' "$COUNT" > "$COUNT_FILE"
printf '%s\n' "$COUNT"
DATE_STUB
chmod +x "$T/bin/timeout" "$T/bin/date"
export COREFLOW_TEST_DATE_COUNTER="$T/date-counter"
PATH="$T/bin:$PATH"
export PATH
PASS=0; FAIL=0
prelude() { cat <<'P'
ui_print() { echo "UI: $*"; }
abort() { echo "ABORT: $*"; exit 1; }
set_perm() { :; }
set_perm_recursive() { :; }
getprop() { awk -F= -v k="$1" '$1==k{print $2; found=1; exit} END{ if(!found) exit 1 }' "$PROPS" 2>/dev/null; }
P
}
elf() { python3 - "$1" "$2" <<'PY'
import sys
b = bytearray(64)
b[0:4] = b'\x7fELF'
b[18] = 0xb7 if sys.argv[2] == 'arm64' else 0x3e
open(sys.argv[1], 'wb').write(bytes(b))
PY
}
mkmod() {   # mkmod <dir> <machine>
  mkdir -p "$1"; cp -r "$SRC"/. "$1"/
  mkdir -p "$1/system/bin" "$1/system/lib64"
  elf "$1/system/bin/coreflowd" "$2"; elf "$1/system/lib64/libonnxruntime.so" "$2"
}
mksys() {   # fake sysfs with one readable zone and one writable policy
  mkdir -p "$1/sys/class/thermal/thermal_zone0" "$1/sys/devices/system/cpu/cpufreq/policy0" "$1/proc/sys/vm"
  echo 45000 > "$1/sys/class/thermal/thermal_zone0/temp"
  echo schedutil > "$1/sys/devices/system/cpu/cpufreq/policy0/scaling_governor"
  echo 60 > "$1/proc/sys/vm/swappiness"
}
props() {   # props <file> <api> <board> <arch-machine>
  cat > "$1" <<EOF
ro.product.vendor.model=TestPhone
ro.product.vendor.device=testdev
ro.soc.model=SM8650
ro.board.platform=kalama
ro.build.version.release=14
ro.build.version.sdk=$2
EOF
}
run() {  # run <name> <expect: ok|fail> <mode-expected or -> <setup-fn> [arch]
  name="$1"; expect="$2"; want="$3"; setup="$4"; arch="${5:-arm64}"
  case_dir="$T/$name"; mkdir -p "$case_dir"
  mkmod "$case_dir/mod" "$arch"; mksys "$case_dir/sys"
  export PROPS="$case_dir/props"; props "$PROPS" 34 kalama
  export COREFLOW_STATE_DIR="$case_dir/state" COREFLOW_SYSROOT="$case_dir/sys"
  export COREFLOW_ADAPTIVE_FLAG="$case_dir/flag"
  mkdir -p "$COREFLOW_STATE_DIR"
  $setup "$case_dir"
  { prelude; echo "MODPATH=$case_dir/mod"; echo "ARCH=arm64"; cat "$case_dir/mod/customize.sh"; } > "$case_dir/run.sh"
  out=$(sh "$case_dir/run.sh" 2>&1); rc=$?
  cfg="$COREFLOW_STATE_DIR/config.ini"
  got_mode=$(grep -o 'mode=[a-z]*' "$COREFLOW_STATE_DIR/install_report.txt" 2>/dev/null | head -1)
  ok=1
  if [ "$expect" = ok ] && [ $rc -ne 0 ]; then ok=0; fi
  if [ "$expect" = fail ] && [ $rc -eq 0 ]; then ok=0; fi
  if [ -n "$want" ] && [ "$got_mode" != "mode=$want" ]; then ok=0; fi
  if [ $ok -eq 1 ]; then PASS=$((PASS+1)); echo "PASS $name (rc=$rc ${got_mode:-no-report})"
  else FAIL=$((FAIL+1)); echo "FAIL $name rc=$rc want=$want got=${got_mode:-none}"; echo "$out" | tail -4; fi
  export LAST_OUT="$out"
}
none() { :; }
flag() { touch "$1/flag"; }
existing_cfg() { mkdir -p "$1/state"; printf 'mutation_mode=observe\nmutation_armed=false\nmonitor_interval=7\ngrant_gpu=no\ncustom_key=keep\n' > "$1/state/config.ini"; }
tampered_model() { echo junk >> "$1/mod/system/etc/coreflow/thermal_predictor.onnx"; }
passive_default() { sed -i 's/^mutation_mode=adaptive$/mutation_mode=observe/' "$1/mod/system/etc/coreflow/default.conf"; }
set_api33() { sed -i 's/ro.build.version.sdk=34/ro.build.version.sdk=33/' "$1/props"; }
unarmed_cfg() { mkdir -p "$1/state"; printf 'mutation_mode=adaptive\nmutation_armed=false\n' > "$1/state/config.ini"; }
safe_cfg() { existing_cfg "$1"; touch "$1/state/SAFE_MODE"; }
stop_cfg() { existing_cfg "$1"; touch "$1/state/DISABLE"; }
expect_line() {
  if ! grep -qx "$2" "$T/$1/state/config.ini"; then echo "FAIL $1 missing $2"; FAIL=$((FAIL+1)); fi
}
run automatic_fresh ok adaptive none
expect_line automatic_fresh 'mutation_armed=true'
expect_line automatic_fresh 'interactivity_mode=adaptive'
expect_line automatic_fresh 'monitor_interval=5'
expect_line automatic_fresh 'grant_cpufreq=yes'
echo "$LAST_OUT" | grep -q 'Volume Up' && { echo 'FAIL mode prompt remains'; FAIL=$((FAIL+1)); }
run shared_storage_flag_unnecessary ok adaptive flag
run existing_config_migrated ok adaptive existing_cfg
expect_line existing_config_migrated 'monitor_interval=7'
expect_line existing_config_migrated 'grant_gpu=no'
expect_line existing_config_migrated 'custom_key=keep'
expect_line existing_config_migrated 'grant_memory=yes'
expect_line existing_config_migrated 'mutation_armed=true'
run api33_rejected fail "" set_api33
run tampered_model_rejected fail "" tampered_model
run passive_payload_rejected fail "" passive_default
run x86_rejected fail "" none x86_64
run legacy_unarmed_migrated ok adaptive unarmed_cfg
expect_line legacy_unarmed_migrated 'mutation_armed=true'
run recovery_marker_preserved ok adaptive safe_cfg
[ -f "$T/recovery_marker_preserved/state/SAFE_MODE" ] || { echo 'FAIL recovery marker cleared'; FAIL=$((FAIL+1)); }
run operator_stop_preserved ok adaptive stop_cfg
[ -f "$T/operator_stop_preserved/state/DISABLE" ] || { echo 'FAIL operator stop cleared'; FAIL=$((FAIL+1)); }
# Verify the migrated text with the real daemon parser, not only shell grep.
if command -v g++ >/dev/null 2>&1; then
  if g++ -std=c++20 -I"$SRC/../include" -x c++ - "$SRC/../src/runtime/config.cpp" -o "$T/config_probe" <<'CPP'
#include "coreflow/config.hpp"
int main(int argc,char** argv) {
    coreflow::EngineConfig c;
    return argc!=2 || !c.load(argv[1]) ||
        c.mutationMode()!=coreflow::MutationMode::Adaptive || !c.mutationArmed() ||
        c.interactivityMode()!=coreflow::InteractivityMode::Adaptive ||
        c.monitorIntervalSeconds()!=7 || c.grantGpu() || !c.grantMemory();
}
CPP
  then
    if "$T/config_probe" "$T/existing_config_migrated/state/config.ini"; then
      echo "PASS migrated configuration accepted by daemon parser"
    else echo "FAIL migrated configuration rejected by daemon parser"; FAIL=$((FAIL+1)); fi
  else echo "FAIL configuration probe did not compile"; FAIL=$((FAIL+1)); fi
else
  echo "SKIP daemon parser integration: g++ unavailable"
fi
echo "RESULT: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
