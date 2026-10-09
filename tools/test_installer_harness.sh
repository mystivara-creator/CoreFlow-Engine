#!/bin/sh
# Host harness for module/customize.sh. Stubs the Magisk installer API.
SRC="${COREFLOW_MODULE_SRC:-$(cd "$(dirname "$0")/../module" && pwd)}"
T=$(mktemp -d)
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
flag_validated() { touch "$1/flag"; cp "$1/mod/system/etc/coreflow/validated_profiles.txt" "$1/vp.bak"; echo kalama >> "$1/mod/system/etc/coreflow/validated_profiles.txt"; }
existing_cfg() { mkdir -p "$1/state"; echo "mutation_mode=observe" > "$1/state/config.ini"; }
tampered_model() { echo junk >> "$1/mod/system/etc/coreflow/thermal_predictor.onnx"; }
adaptive_default() { sed -i 's/^mutation_mode=observe$/mutation_mode=adaptive/' "$1/mod/system/etc/coreflow/default.conf"; }
no_sysfs() { rm -rf "$1/sys/class"; }
api33() { :; }
set_api33() { sed -i 's/ro.build.version.sdk=34/ro.build.version.sdk=33/' "$1/props"; }
bad_arch() { :; }

run positive_observe_no_flag ok observe none
grep -q "CoreFlow Autonomous install report" "$T/positive_observe_no_flag/state/install_report.txt" && echo "   report present"
grep -qx 'mutation_mode=observe' "$T/positive_observe_no_flag/state/config.ini" && echo "   config observe"
run flag_unvalidated_soc ok observe flag
grep -q "no validated profile" "$T/flag_unvalidated_soc/state/install_report.txt" && echo "   refused: no validated profile"
[ -f "$T/flag_unvalidated_soc/flag" ] && echo "   WARN flag not consumed" || echo "   flag consumed"
run flag_validated_soc_adaptive ok adaptive flag_validated
grep -qx 'mutation_mode=adaptive' "$T/flag_validated_soc_adaptive/state/config.ini" && grep -qx 'mutation_armed=true' "$T/flag_validated_soc_adaptive/state/config.ini" && grep -qx 'allow_cpu_governor=yes' "$T/flag_validated_soc_adaptive/state/config.ini" && echo "   adaptive triple written"
run existing_config_kept ok observe existing_cfg
grep -q "existing config" "$T/existing_config_kept/state/install_report.txt" && echo "   existing config untouched"
no_thermal_flag() { touch "$1/flag"; cp "$1/mod/system/etc/coreflow/validated_profiles.txt" "$1/vp.bak"; echo kalama >> "$1/mod/system/etc/coreflow/validated_profiles.txt"; rm -rf "$1/sys/sys/class/thermal"; }
run flag_validated_no_thermal ok observe no_thermal_flag
grep -q "required telemetry" "$T/flag_validated_no_thermal/state/install_report.txt" && echo "   refused: telemetry gate"
run api33_rejected fail "" set_api33
run tampered_model_rejected fail "" tampered_model
run adaptive_default_rejected fail "" adaptive_default
run x86_rejected fail "" none x86_64
# Effective-mode cases: the report and banner must reflect config.ini, and an
# adaptive config must trigger the warning even though the installer did not
# write it.
adaptive_cfg() { mkdir -p "$1/state"; printf 'mutation_mode=adaptive\nmutation_armed=true\nallow_cpu_governor=yes\n' > "$1/state/config.ini"; }
unarmed_cfg()  { mkdir -p "$1/state"; printf 'mutation_mode=adaptive\nmutation_armed=false\n' > "$1/state/config.ini"; }
observe_cfg()  { mkdir -p "$1/state"; printf 'mutation_mode=observe\nmutation_armed=false\n' > "$1/state/config.ini"; }
run existing_adaptive_armed_reports_adaptive ok adaptive adaptive_cfg
echo "$LAST_OUT" | grep -q "ADAPTIVE mutation" && echo "   warning shown" || { echo "FAIL warning missing"; FAIL=$((FAIL+1)); }
printf 'mutation_mode=adaptive\nmutation_armed=true\nallow_cpu_governor=yes\n' > "$T/expected.ini"
cmp -s "$T/existing_adaptive_armed_reports_adaptive/state/config.ini" "$T/expected.ini" && echo "   config left untouched" || { echo "FAIL config modified"; FAIL=$((FAIL+1)); }
run existing_adaptive_unarmed_reports_observe ok observe unarmed_cfg
run existing_observe_reports_observe ok observe observe_cfg
echo "$LAST_OUT" | grep -q "ADAPTIVE mutation" && { echo "FAIL spurious warning"; FAIL=$((FAIL+1)); } || echo "   no spurious warning"
echo
echo "RESULT: $PASS passed, $FAIL failed"
