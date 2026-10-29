#!/system/bin/sh
# ==========================================================
# CoreFlow Autonomous - installer (v3.0.2)
#
# Runs inside the Magisk/KernelSU installer, before first boot. Steps:
#   1. Compatibility gate: arm64, Android 14 (API 34) or newer.
#   2. Payload integrity: ELF checks for coreflowd and libonnxruntime, a pinned
#      SHA-256 for the thermal model, the model's input-name marker, and a
#      check that the shipped default configuration enables the bounded engine.
#   3. Device profile: SoC family and identifier, plus a read-only sysfs/procfs
#      capability probe. Nothing is written to kernel controls here.
#   4. Automatic configuration: preserve tuning and explicit domain exclusions,
#      migrate legacy mode keys to active operation. No volume-key mode prompt.
#   5. Report: $STATE_DIR/install_report.txt, for support requests.
#
# The thermal model is universal: one artifact, one pinned digest, one input
# contract for every device. The device profile never selects a different model.
# It only selects how the engine is configured at install time.
# ==========================================================

# ---- Constants -------------------------------------------------------------
MIN_API=34
# Pinned digest of the single, universal thermal model. Must equal the
# THERMAL_MODEL_SHA256 pin in .github/workflows/build.yml (checked by
# tools/verify_source_release.sh).
MODEL_SHA256="a605b046086f2f30e0a625c64547de5399e407887d69461d3c5642f7344f0703"
MODEL_INPUT_MARKER="float_input"

# Overridable for host-side testing; the defaults are the device paths.
STATE_DIR="${COREFLOW_STATE_DIR:-/data/adb/coreflow}"
SYSROOT="${COREFLOW_SYSROOT:-}"
ADAPTIVE_FLAG="${COREFLOW_ADAPTIVE_FLAG:-/sdcard/CoreFlow/enable_adaptive}"

CONFIG="$STATE_DIR/config.ini"
REPORT="$STATE_DIR/install_report.txt"
DEFAULT_CONF="$MODPATH/system/etc/coreflow/default.conf"
DAEMON="$MODPATH/system/bin/coreflowd"
ONNX_LIB="$MODPATH/system/lib64/libonnxruntime.so"
MODEL="$MODPATH/system/etc/coreflow/thermal_predictor.onnx"

MODULE_VERSION="$(awk -F= '$1=="version" {print $2; exit}' "$MODPATH/module.prop" 2>/dev/null | tr -d '[:space:]')"
MODULE_VERSION="${MODULE_VERSION:-unknown}"

# ---- Helpers ----------------------------------------------------------------
# getprop always succeeds, so fall back on empty values instead of `||`.
first_prop() {
    for key in "$@"; do
        value="$(getprop "$key" 2>/dev/null)"
        if [ -n "$value" ]; then
            echo "$value"
            return 0
        fi
    done
    echo "unknown"
}

# ELF check: magic 7f 45 4c 46 and e_machine == EM_AARCH64 (0xb7), little-endian.
is_arm64_elf() {
    file="$1"
    [ -s "$file" ] || return 1
    magic="$(head -c 4 "$file" 2>/dev/null | od -An -tx1 | tr -d ' \n')"
    [ "$magic" = "7f454c46" ] || return 1
    machine="$(dd if="$file" bs=1 skip=18 count=2 2>/dev/null | od -An -tx1 | tr -d ' \n')"
    [ "$machine" = "b700" ]
}

sha256_of() {
    sha256sum "$1" 2>/dev/null | cut -d' ' -f1
}

lower() {
    echo "$1" | tr 'A-Z' 'a-z'
}

# Number of files matching a glob that are readable / writable.
count_readable() {
    n=0
    for f in $1; do [ -r "$f" ] && n=$((n + 1)); done
    echo "$n"
}

count_writable() {
    n=0
    for f in $1; do [ -w "$f" ] && n=$((n + 1)); done
    echo "$n"
}

# SoC family from the platform, SoC model and board strings. Only used to
# label the device in the report; it never selects a model or a mutation domain.
soc_family() {
    case "$1" in
        *qcom*|*msm*|*sm[0-9]*|*sdm*|*kona*|*lahaina*|*taro*|*kalama*|*pineapple*) echo qualcomm ;;
        *mt[0-9]*|*mtk*|*dimensity*) echo mediatek ;;
        *exynos*|*s5e[0-9]*) echo exynos ;;
        *tensor*|*gs[0-9]*|*zuma*|*laguna*) echo tensor ;;
        unknown|'') echo unknown ;;
        *) echo other ;;
    esac
}

report() {
    echo "$*" >> "$REPORT" 2>/dev/null
}

# Last value of a key in config.ini, trimmed. Empty when absent. The daemon's
# parser also keeps the last occurrence of each key, so this matches it.
config_value() {
    [ -f "$CONFIG" ] || { echo ""; return 0; }
    grep -E "^[[:space:]]*$1[[:space:]]*=" "$CONFIG" 2>/dev/null \
        | tail -n 1 | cut -d= -f2- | tr -d ' \t\r'
}

# Same truth values the daemon accepts (config.cpp parseBool).
config_true() {
    case "$1" in 1|true|yes|on) return 0 ;; *) return 1 ;; esac
}

# ---- 0. Banner and target identity ---------------------------------------
DEVICE_MODEL="$(first_prop ro.product.vendor.model ro.product.model)"
DEVICE_CODE="$(first_prop ro.product.vendor.device ro.product.device)"
SOC_MODEL="$(first_prop ro.soc.model ro.hardware.chipname)"
SOC_ID="$(lower "$(first_prop ro.board.platform ro.soc.model)")"
SOC_FAMILY="$(soc_family "$(lower "$SOC_MODEL $SOC_ID")")"
ANDROID_VER="$(first_prop ro.build.version.release)"
API_LEVEL="$(first_prop ro.build.version.sdk)"
KERNEL_VER="$(uname -r 2>/dev/null || echo unknown)"

ui_print " "
ui_print " CoreFlow Autonomous ${MODULE_VERSION}"
ui_print " Developed by Mystivara"
ui_print " "
ui_print " [>] Target environment"
ui_print "     Model   : $DEVICE_MODEL ($DEVICE_CODE)"
ui_print "     SoC     : $SOC_MODEL [$SOC_ID] family=$SOC_FAMILY"
ui_print "     OS      : Android $ANDROID_VER (API $API_LEVEL)"
ui_print "     Kernel  : $KERNEL_VER"
ui_print " "

# ---- 1. Compatibility gate ----------------------------------------------
ui_print " [>] Compatibility gate"
case "${ARCH:-}" in
    arm64) ui_print "     > Architecture : arm64 OK" ;;
    *) abort "CoreFlow requires arm64. Detected architecture: ${ARCH:-unknown}." ;;
esac
case "$API_LEVEL" in
    ''|*[!0-9]*) abort "Could not determine the Android API level." ;;
esac
if [ "$API_LEVEL" -lt "$MIN_API" ]; then
    abort "CoreFlow requires Android 14 (API $MIN_API) or newer. Detected API $API_LEVEL."
fi
ui_print "     > Android API  : $API_LEVEL OK"
ui_print " "

# ---- 2. Payload integrity ------------------------------------------------
ui_print " [>] Verifying package payload"
is_arm64_elf "$DAEMON" || abort "coreflowd is missing or is not an ARM64 ELF executable."
ui_print "     > coreflowd          : ARM64 ELF OK"
is_arm64_elf "$ONNX_LIB" || abort "libonnxruntime.so is missing or is not an ARM64 ELF library."
ui_print "     > ONNX Runtime       : ARM64 ELF OK"

[ -s "$MODEL" ] || abort "thermal_predictor.onnx is missing or empty."
MODEL_DIGEST="$(sha256_of "$MODEL")"
if [ "$MODEL_DIGEST" != "$MODEL_SHA256" ]; then
    abort "thermal_predictor.onnx digest mismatch (expected $MODEL_SHA256, got ${MODEL_DIGEST:-none}). sha256sum may be unavailable."
fi
grep -q "$MODEL_INPUT_MARKER" "$MODEL" || abort "thermal_predictor.onnx does not declare the '$MODEL_INPUT_MARKER' input."
ui_print "     > Thermal model      : universal, digest pinned OK"

[ -s "$DEFAULT_CONF" ] || abort "default.conf is missing or empty."
grep -qx 'mutation_mode=adaptive' "$DEFAULT_CONF" \
    && grep -qx 'mutation_armed=true' "$DEFAULT_CONF" \
    || abort "default.conf must enable the bounded engine."
ui_print "     > Default config     : automatic optimization OK"
ui_print " "

# ---- 3. Device profile and read-only probe -------------------------------
ui_print " [>] Device profile (read-only probe)"
THERMAL_READ="$(count_readable "$SYSROOT/sys/class/thermal/thermal_zone*/temp")"
CPU_POLICY_READ="$(count_readable "$SYSROOT/sys/devices/system/cpu/cpufreq/policy*/scaling_governor")"
CPU_POLICY_WRITE="$(count_writable "$SYSROOT/sys/devices/system/cpu/cpufreq/policy*/scaling_governor")"
BLOCK_TUNABLE="$(count_readable "$SYSROOT/sys/block/*/queue/read_ahead_kb")"
BATTERY_READ="$(count_readable "$SYSROOT/sys/class/power_supply/*/capacity")"
VM_SWAPPINESS="no"
[ -r "$SYSROOT/proc/sys/vm/swappiness" ] && VM_SWAPPINESS="yes"

ui_print "     > thermal zones readable : $THERMAL_READ"
ui_print "     > cpufreq policies       : read=$CPU_POLICY_READ writable=$CPU_POLICY_WRITE"
ui_print "     > block read-ahead tunes : $BLOCK_TUNABLE"
ui_print "     > battery telemetry      : $BATTERY_READ"
ui_print "     > vm.swappiness readable : $VM_SWAPPINESS"
ui_print " "

# ---- 4. Automatic activation and compatible config migration --------------
# Installation authorizes bounded tuning. Preserve user tuning and explicit
# grant_<domain>=no exclusions. Old mode/arming keys are normalized atomically.
# Existing operator stop/recovery markers are never cleared by installation.
mkdir -p "$STATE_DIR/logs" || abort "Could not create CoreFlow state directory."
chmod 0700 "$STATE_DIR" "$STATE_DIR/logs" 2>/dev/null
umask 077
TMP_CONF="$CONFIG.install.tmp.$$"
PREVIOUS_CONF=/dev/null
[ ! -e "$CONFIG" ] && [ ! -L "$CONFIG" ] || {
    [ -f "$CONFIG" ] && [ -r "$CONFIG" ] && [ ! -L "$CONFIG" ] \
        || abort "Existing config.ini is not a readable regular file."
    PREVIOUS_CONF="$CONFIG"
}
awk '
function key(line, k) {
    if (line ~ /^[ \t]*[#;]/ || index(line,"=")==0) return ""
    k=substr(line,1,index(line,"=")-1)
    gsub(/^[ \t]+|[ \t]+$/, "", k)
    return k
}
function activation(k) { return k=="mutation_mode" || k=="mutation_armed" || k=="interactivity_mode" }
FNR==NR { defaults[++n]=$0; next }
{
    k=key($0)
    if (activation(k)) next
    if (k!="") seen[k]=1
    print
}
END {
    for (i=1;i<=n;i++) {
        k=key(defaults[i])
        if (k!="" && !activation(k) && !seen[k]) { print defaults[i]; seen[k]=1 }
    }
    print "mutation_mode=adaptive"
    print "mutation_armed=true"
    print "interactivity_mode=adaptive"
}' "$DEFAULT_CONF" "$PREVIOUS_CONF" > "$TMP_CONF" || {
    rm -f "$TMP_CONF"
    abort "Could not migrate CoreFlow configuration."
}
grep -qx 'mutation_mode=adaptive' "$TMP_CONF" \
    && grep -qx 'mutation_armed=true' "$TMP_CONF" \
    && grep -qx 'interactivity_mode=adaptive' "$TMP_CONF" \
    || abort "Automatic engine configuration did not verify."
chmod 0600 "$TMP_CONF" || abort "Could not protect configuration."
mv "$TMP_CONF" "$CONFIG" || abort "Could not install configuration."

MODE=adaptive
MODE_REASON="automatic bounded engine; installation configured activation"
REQUESTED_MODE=adaptive
CPU_GOV=no
EFF_CPU="$(config_value allow_cpu_governor)"
config_true "$EFF_CPU" && CPU_GOV=yes
EFF_REASON="automatic activation; tuning and explicit grant exclusions preserved"
if [ -f "$STATE_DIR/SAFE_MODE" ]; then
    EFF_REASON="$EFF_REASON; hold=SAFE_MODE"
    ui_print " [>] Recovery hold remains active (SAFE_MODE)."
elif [ -f "$STATE_DIR/DISABLE" ]; then
    EFF_REASON="$EFF_REASON; hold=DISABLE"
    ui_print " [>] Existing operator stop preserved. Use Start in WebUI to resume."
else
    ui_print " [>] Engine will optimize automatically after reboot."
fi
ui_print "     > Discovery, thermal checks, journal and rollback remain active."
ui_print " "

# Legacy enable flag is one-shot.
[ -f "$ADAPTIVE_FLAG" ] && rm -f "$ADAPTIVE_FLAG" 2>/dev/null

# ---- 6. Install report ---------------------------------------------------
rm -f "$REPORT" 2>/dev/null
report "CoreFlow Autonomous install report"
report "version=$MODULE_VERSION"
report "date=$(date '+%F %T' 2>/dev/null)"
report "device_model=$DEVICE_MODEL"
report "device_code=$DEVICE_CODE"
report "soc_model=$SOC_MODEL"
report "soc_id=$SOC_ID"
report "soc_family=$SOC_FAMILY"
report "android=$ANDROID_VER api=$API_LEVEL"
report "kernel=$KERNEL_VER"
report "model_digest=$MODEL_DIGEST"
report "probe thermal_read=$THERMAL_READ cpu_read=$CPU_POLICY_READ cpu_write=$CPU_POLICY_WRITE block=$BLOCK_TUNABLE battery=$BATTERY_READ swappiness=$VM_SWAPPINESS"
report "mode=$MODE source=\"$EFF_REASON\""
report "requested=$REQUESTED_MODE reason=$MODE_REASON"
report "config_cpu_governor=$CPU_GOV"
chmod 0600 "$REPORT" 2>/dev/null
ui_print " [>] Report written: $REPORT"
ui_print " "

# ---- 7. Permissions ------------------------------------------------------
ui_print " [>] Setting permissions"
set_perm_recursive "$MODPATH/system" 0 0 0755 0644
set_perm "$DAEMON" 0 0 0755
for script in customize.sh post-fs-data.sh service.sh uninstall.sh; do
    [ -f "$MODPATH/$script" ] && set_perm "$MODPATH/$script" 0 0 0755
done
set_perm "$MODPATH/sepolicy.rule" 0 0 0644
set_perm "$MODPATH/module.prop" 0 0 0644
if [ -d "$MODPATH/webroot" ]; then
    set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644
fi
ui_print "     > Permissions applied"
ui_print " "
ui_print " [=== INSTALLATION COMPLETE: reboot to start CoreFlow automatically ===]"
ui_print " "
