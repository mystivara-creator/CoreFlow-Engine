#!/system/bin/sh
# ==========================================================
# CoreFlow Autonomous - installer (v2.0.1)
#
# Runs inside the Magisk/KernelSU installer, before first boot. Steps:
#   1. Compatibility gate: arm64, Android 14 (API 34) or newer.
#   2. Payload integrity: ELF checks for coreflowd and libonnxruntime, a pinned
#      SHA-256 for the thermal model, the model's input-name marker, and a
#      check that the shipped default configuration is observe-only.
#   3. Device profile: SoC family and identifier, plus a read-only sysfs/procfs
#      capability probe. Nothing is written to kernel controls here.
#   4. Mode decision: adaptive is written only when every gate in decide_mode
#      passes. Otherwise the device stays observe-only.
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
MODEL_SHA256="38a87e82a50fef0f896846a2bb685928b0c0c85bacc683238759ec930fcbb3c6"
MODEL_INPUT_MARKER="float_input"

# Overridable for host-side testing; the defaults are the device paths.
STATE_DIR="${COREFLOW_STATE_DIR:-/data/adb/coreflow}"
SYSROOT="${COREFLOW_SYSROOT:-}"
ADAPTIVE_FLAG="${COREFLOW_ADAPTIVE_FLAG:-/sdcard/CoreFlow/enable_adaptive}"

CONFIG="$STATE_DIR/config.ini"
REPORT="$STATE_DIR/install_report.txt"
DEFAULT_CONF="$MODPATH/system/etc/coreflow/default.conf"
VALIDATED="$MODPATH/system/etc/coreflow/validated_profiles.txt"
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
grep -qx 'mutation_mode=observe' "$DEFAULT_CONF" \
    && grep -qx 'allow_cpu_governor=no' "$DEFAULT_CONF" \
    && grep -qx 'mutation_armed=false' "$DEFAULT_CONF" \
    || abort "default.conf must ship observe-only defaults. This package is not release-safe."
ui_print "     > Default config     : observe-only OK"
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

# ---- 4. Mode decision ----------------------------------------------------
# Adaptive is written only when ALL of these hold:
#   - the user placed the enable flag before flashing (explicit request);
#   - this is a fresh install (an existing config.ini is never rewritten);
#   - no SAFE_MODE marker is present;
#   - this SoC is listed in validated_profiles.txt (device evidence exists);
#   - thermal telemetry is readable and at least one cpufreq governor is writable.
# Any failed gate keeps the device in observe-only mode and says why.
decide_mode() {
    if [ ! -f "$ADAPTIVE_FLAG" ]; then
        MODE="observe"; MODE_REASON="no adaptive request (flag absent)"
    elif [ -f "$CONFIG" ]; then
        MODE="observe"; MODE_REASON="existing config.ini kept (upgrade or reinstall)"
    elif [ -f "$STATE_DIR/SAFE_MODE" ]; then
        MODE="observe"; MODE_REASON="SAFE_MODE marker present"
    elif ! grep -v '^#' "$VALIDATED" 2>/dev/null | grep -qx "$SOC_ID"; then
        MODE="observe"; MODE_REASON="SoC '$SOC_ID' has no validated profile yet"
    elif [ "$THERMAL_READ" -lt 1 ] || [ "$CPU_POLICY_WRITE" -lt 1 ]; then
        MODE="observe"; MODE_REASON="required telemetry or writable governors not found"
    else
        MODE="adaptive"; MODE_REASON="adaptive requested and all gates passed"
    fi
}
decide_mode
REQUESTED_MODE="$MODE"

ui_print " [>] Mode request"
ui_print "     > requested : $MODE"
ui_print "     > reason    : $MODE_REASON"
ui_print " "

# ---- 5. Configuration and state ------------------------------------------
mkdir -p "$STATE_DIR/logs" 2>/dev/null
chmod 0700 "$STATE_DIR" "$STATE_DIR/logs" 2>/dev/null

if [ -f "$CONFIG" ]; then
    ui_print " [>] Configuration: existing config.ini kept unchanged"
else
    TMP_CONF="$CONFIG.tmp"
    cp "$DEFAULT_CONF" "$TMP_CONF" || abort "Could not stage configuration in $STATE_DIR."
    if [ "$MODE" = "adaptive" ]; then
        sed -e 's/^mutation_mode=observe$/mutation_mode=adaptive/' \
            -e 's/^allow_cpu_governor=no$/allow_cpu_governor=yes/' \
            -e 's/^mutation_armed=false$/mutation_armed=true/' \
            "$TMP_CONF" > "$TMP_CONF.2" && mv "$TMP_CONF.2" "$TMP_CONF" \
            || abort "Could not write adaptive configuration."
        grep -qx 'mutation_mode=adaptive' "$TMP_CONF" \
            && grep -qx 'mutation_armed=true' "$TMP_CONF" \
            && grep -qx 'allow_cpu_governor=yes' "$TMP_CONF" \
            || abort "Adaptive configuration did not verify; aborting."
    fi
    mv "$TMP_CONF" "$CONFIG" || abort "Could not install config.ini."
    chmod 0600 "$CONFIG" 2>/dev/null
    ui_print " [>] Configuration: config.ini written (mode=$MODE)"
fi

# ---- 5b. Effective mode: what the daemon will actually run ---------------
# The installer reports the mode the daemon will read, not the mode it wanted.
# Adaptive is effective only when mutation_mode=adaptive AND mutation_armed is
# true, mirroring the daemon's gates. An existing adaptive config is never
# rewritten here, but it is reported loudly.
EFF_MODE="$(config_value mutation_mode)"
EFF_ARMED="$(config_value mutation_armed)"
EFF_CPU="$(config_value allow_cpu_governor)"
EFF_REASON="mutation_mode=${EFF_MODE:-unset} mutation_armed=${EFF_ARMED:-unset} allow_cpu_governor=${EFF_CPU:-unset}"

MODE="observe"
CPU_GOV="no"
if [ "$EFF_MODE" = "adaptive" ] && config_true "$EFF_ARMED"; then
    MODE="adaptive"
    config_true "$EFF_CPU" && CPU_GOV="yes"
elif [ "$EFF_MODE" = "adaptive" ]; then
    EFF_REASON="$EFF_REASON (adaptive without arming: no mutation can occur)"
fi

ui_print " [>] Effective mode (read from config.ini)"
ui_print "     > effective : $MODE"
ui_print "     > source    : $EFF_REASON"
if [ "$MODE" = "adaptive" ]; then
    ui_print " "
    ui_print "     ! WARNING: this device will run ADAPTIVE mutation after reboot."
    ui_print "     ! The installer did not create this setting. If it was not"
    ui_print "     ! intended, set mutation_mode=observe and mutation_armed=false"
    ui_print "     ! in /data/adb/coreflow/config.ini before rebooting."
fi
if [ "$MODE" != "$REQUESTED_MODE" ] && [ -f "$CONFIG" ]; then
    ui_print "     ! Note: the installer requested '$REQUESTED_MODE' but an existing"
    ui_print "     ! config.ini is in effect. The existing config wins."
fi
ui_print " "

# The enable flag is one-shot: consume it so it cannot re-trigger later.
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
ui_print "     > Permissions applied"
ui_print " "
ui_print " [=== INSTALLATION COMPLETE: reboot to start CoreFlow (mode=$MODE) ===]"
ui_print " "
