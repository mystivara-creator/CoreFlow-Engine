#!/system/bin/sh
# CFAE System Framework — installer preflight, package validation and safe upgrade.
# Runtime capability discovery and all kernel actuation remain owned by coreflowd.

PATH="/system/bin:/system/xbin:/sbin:/vendor/bin:$PATH"
export PATH
umask 077

ui() {
    if command -v ui_print >/dev/null 2>&1; then
        ui_print "$1"
    else
        echo "$1"
    fi
}

die() {
    ui "! $1"
    if command -v abort >/dev/null 2>&1; then
        abort "! $1"
    fi
    exit 1
}

report() {
    ui "$1"
    PREFLIGHT_REPORT="$PREFLIGHT_REPORT
$1"
}

read_value() {
    [ -r "$1" ] || return 1
    cat "$1" 2>/dev/null
}

get_property() {
    if command -v getprop >/dev/null 2>&1; then
        getprop "$1" 2>/dev/null
    fi
}

is_numeric() {
    case "$1" in
        ''|*[!0-9]*) return 1 ;;
        *) return 0 ;;
    esac
}

stage_and_replace() {
    SRC="$1"
    DST="$2"
    MODE="$3"

    [ -f "$SRC" ] || return 1
    [ ! -L "$SRC" ] || return 1
    DST_DIR="$(dirname "$DST")"
    mkdir -p "$DST_DIR" || return 1

    TMP_DST="$DST.stage.$$"
    rm -f "$TMP_DST"
    if cp -f "$SRC" "$TMP_DST" &&
       chmod "$MODE" "$TMP_DST" &&
       { chown 0:0 "$TMP_DST" 2>/dev/null || true; } &&
       mv -f "$TMP_DST" "$DST"; then
        return 0
    fi

    rm -f "$TMP_DST"
    return 1
}

stage_optional() {
    [ -f "$1" ] || return 0
    stage_and_replace "$1" "$2" "$3"
}

set_file_perm() {
    FILE="$1"
    MODE="$2"
    [ -f "$FILE" ] || return 0

    if command -v set_perm >/dev/null 2>&1; then
        set_perm "$FILE" 0 0 "$MODE"
    else
        chown 0:0 "$FILE" 2>/dev/null || return 1
        chmod "$MODE" "$FILE"
    fi
}

set_webroot_perm() {
    [ -d "$1" ] || return 0
    if command -v set_perm_recursive >/dev/null 2>&1; then
        set_perm_recursive "$1" 0 0 0755 0644
    else
        chown -R 0:0 "$1" 2>/dev/null || return 1
        find "$1" -type d -exec chmod 0755 {} + 2>/dev/null
        find "$1" -type f -exec chmod 0644 {} + 2>/dev/null
    fi
}

detect_root_manager() {
    if [ -n "$KSU" ] || [ -n "$KSU_VER_CODE" ] ||
       [ -d /data/adb/ksu ] || [ -x /data/adb/ksud ]; then
        ROOT_MANAGER="KernelSU / KernelSU Next"
    elif [ -n "$APATCH" ] || [ -n "$APATCH_VER_CODE" ] ||
         [ -d /data/adb/ap ] || [ -x /data/adb/apd ]; then
        ROOT_MANAGER="APatch"
    elif [ -n "$MAGISK_VER_CODE" ] || [ -d /data/adb/magisk ] ||
         [ -x /data/adb/magisk/magisk32 ] || [ -x /sbin/.magisk/magisk ]; then
        ROOT_MANAGER="Magisk"
    elif [ -n "$BOOTMODE" ] || [ -n "$MODPATH" ]; then
        ROOT_MANAGER="Root-manager installer environment (unidentified)"
    else
        ROOT_MANAGER="Unknown"
    fi
}

count_thermal_zones() {
    COUNT=0
    for ZONE in /sys/class/thermal/thermal_zone*; do
        [ -d "$ZONE" ] || continue
        [ -r "$ZONE/temp" ] || continue
        COUNT=$((COUNT + 1))
    done
    echo "$COUNT"
}

count_input_nodes() {
    COUNT=0
    for NODE in /dev/input/event[0-9]*; do
        [ -c "$NODE" ] || continue
        COUNT=$((COUNT + 1))
    done
    echo "$COUNT"
}

ui "**********************************************"
ui "          CFAE System Framework v3.1.0"
ui "          Installer Preflight / ARM64"
ui "          Author: mystivara-creator"
ui "**********************************************"

[ -n "$MODPATH" ] || die "MODPATH is not provided by the root-manager installer."
[ -d "$MODPATH" ] || die "Module staging directory does not exist: $MODPATH"
[ ! -L "$MODPATH" ] || die "Module staging directory must not be a symlink."

STATE_DIR="$COREFLOW_STATE_DIR"
[ -n "$STATE_DIR" ] || STATE_DIR="/data/adb/coreflow"
LIVE_MOD="$COREFLOW_LIVE_MOD_DIR"
[ -n "$LIVE_MOD" ] || LIVE_MOD="/data/adb/modules/cfae_framework"

[ ! -L "$STATE_DIR" ] || die "Refusing symlinked runtime state directory: $STATE_DIR"
[ ! -L "$LIVE_MOD" ] || die "Refusing symlinked active module directory: $LIVE_MOD"
mkdir -p "$STATE_DIR" || die "Cannot prepare runtime state directory."
chmod 0700 "$STATE_DIR" 2>/dev/null

PREFLIGHT_REPORT="CFAE System Framework v3.1.0 — read-only installer preflight"
report "Preflight: package and platform identity"

INSTALL_ARCH="$ARCH"
[ -n "$INSTALL_ARCH" ] || INSTALL_ARCH="$(uname -m 2>/dev/null)"
case "$INSTALL_ARCH" in
    arm64|aarch64|arm64-v8a) ;;
    *) die "Unsupported architecture ($INSTALL_ARCH). CFAE requires ARM64." ;;
esac
report "PASS: installer architecture = $INSTALL_ARCH"

API_LEVEL="$API"
is_numeric "$API_LEVEL" || API_LEVEL="$(get_property ro.build.version.sdk)"
is_numeric "$API_LEVEL" || die "Android API level could not be determined."
[ "$API_LEVEL" -ge 35 ] || die "Unsupported Android API $API_LEVEL. Android 15 / API 35+ is required."
report "PASS: Android API level = $API_LEVEL"

ANDROID_RELEASE="$(get_property ro.build.version.release)"
MANUFACTURER="$(get_property ro.product.manufacturer)"
BRAND="$(get_property ro.product.brand)"
MODEL="$(get_property ro.product.model)"
DEVICE="$(get_property ro.product.device)"
FINGERPRINT="$(get_property ro.build.fingerprint)"
ABI_LIST="$(get_property ro.product.cpu.abilist64)"
[ -n "$MANUFACTURER" ] || MANUFACTURER="unknown"
[ -n "$BRAND" ] || BRAND="unknown"
[ -n "$MODEL" ] || MODEL="unknown"
[ -n "$DEVICE" ] || DEVICE="unknown"
[ -n "$ANDROID_RELEASE" ] || ANDROID_RELEASE="unknown"
[ -n "$FINGERPRINT" ] || FINGERPRINT="unavailable"

report "Device: $MANUFACTURER / $BRAND / $MODEL ($DEVICE)"
report "Android release: $ANDROID_RELEASE; SDK: $API_LEVEL"
report "Build fingerprint: $FINGERPRINT"

case "$ABI_LIST" in
    *arm64-v8a*) report "PASS: 64-bit ABI list contains arm64-v8a" ;;
    '') report "INFO: ro.product.cpu.abilist64 unavailable; installer ARCH is the ABI evidence" ;;
    *) die "64-bit ABI list does not contain arm64-v8a: $ABI_LIST" ;;
esac
if [ -e /system/bin/linker64 ]; then
    report "PASS: 64-bit Android linker present"
else
    report "WARN: /system/bin/linker64 was not visible during installer preflight"
fi

ROOT_MANAGER=""
detect_root_manager
report "Root manager: $ROOT_MANAGER"

KERNEL_RELEASE="$(uname -r 2>/dev/null)"
KERNEL_ARCH="$(uname -m 2>/dev/null)"
[ -n "$KERNEL_RELEASE" ] || KERNEL_RELEASE="unknown"
[ -n "$KERNEL_ARCH" ] || KERNEL_ARCH="unknown"
report "Kernel: $KERNEL_RELEASE ($KERNEL_ARCH)"

if command -v getenforce >/dev/null 2>&1; then
    SELINUX_STATE="$(getenforce 2>/dev/null)"
else
    SELINUX_STATE="unknown"
fi
[ -n "$SELINUX_STATE" ] || SELINUX_STATE="unknown"
report "SELinux: $SELINUX_STATE (installer does not change SELinux mode or policy)"

report "Preflight: package integrity"
CORE_BIN="$MODPATH/system/bin/coreflowd"
if [ ! -f "$CORE_BIN" ]; then
    if [ -f "$MODPATH/coreflowd" ] && [ ! -L "$MODPATH/coreflowd" ]; then
        mkdir -p "$MODPATH/system/bin" || die "Cannot create system/bin."
        cp -f "$MODPATH/coreflowd" "$CORE_BIN" || die "Cannot stage coreflowd."
    else
        die "coreflowd binary missing from package."
    fi
fi
[ ! -L "$CORE_BIN" ] || die "coreflowd must not be a symlink."
[ -s "$CORE_BIN" ] || die "coreflowd binary is empty."

if [ "$COREFLOW_TEST_ALLOW_NON_ELF_BINARY" = "1" ]; then
    report "TEST MODE: non-ELF mock daemon accepted by lifecycle regression harness"
else
    command -v od >/dev/null 2>&1 || die "od is required to validate the packaged ELF binary."
    ELF_MAGIC="$(od -An -tx1 -N4 "$CORE_BIN" 2>/dev/null | tr -d ' \n')"
    ELF_CLASS="$(od -An -tx1 -j4 -N1 "$CORE_BIN" 2>/dev/null | tr -d ' \n')"
    ELF_ENDIAN="$(od -An -tx1 -j5 -N1 "$CORE_BIN" 2>/dev/null | tr -d ' \n')"
    ELF_MACHINE="$(od -An -tx1 -j18 -N2 "$CORE_BIN" 2>/dev/null | tr -d ' \n')"
    [ "$ELF_MAGIC" = "7f454c46" ] || die "coreflowd is not an ELF executable."
    [ "$ELF_CLASS" = "02" ] || die "coreflowd is not a 64-bit ELF binary."
    [ "$ELF_ENDIAN" = "01" ] || die "coreflowd is not little-endian ARM64 ELF."
    [ "$ELF_MACHINE" = "b700" ] || die "coreflowd ELF machine is not AArch64 (e_machine=183)."
    report "PASS: coreflowd is 64-bit little-endian AArch64 ELF"
fi

if [ -f "$MODPATH/module.prop" ] && [ ! -L "$MODPATH/module.prop" ]; then
    MODULE_ID="$(awk -F= '$1 == "id" { print $2; exit }' "$MODPATH/module.prop")"
    MODULE_VERSION="$(awk -F= '$1 == "version" { print $2; exit }' "$MODPATH/module.prop")"
    MODULE_CODE="$(awk -F= '$1 == "versionCode" { print $2; exit }' "$MODPATH/module.prop")"
    [ "$MODULE_ID" = "cfae_framework" ] || die "Unexpected module id: $MODULE_ID"
    [ -n "$MODULE_VERSION" ] || MODULE_VERSION="unknown"
    [ -n "$MODULE_CODE" ] || MODULE_CODE="unknown"
    report "PASS: module metadata id=$MODULE_ID version=$MODULE_VERSION code=$MODULE_CODE"
else
    report "WARN: module.prop unavailable in staging directory"
fi

[ -f "$MODPATH/service.sh" ] || die "service.sh missing from package."
[ -f "$MODPATH/post-fs-data.sh" ] || die "post-fs-data.sh missing from package."
[ -f "$MODPATH/uninstall.sh" ] || die "uninstall.sh missing from package."
if [ ! -f "$MODPATH/system/etc/coreflow/default.conf" ]; then
    report "WARN: default.conf not found; daemon may use compiled defaults"
fi
if [ ! -f "$MODPATH/sepolicy.rule" ]; then
    report "INFO: no additional SELinux policy file packaged"
fi

report "Preflight: read-only kernel capability inventory"

CGROUP2_ROOT="$(awk '$0 ~ / - cgroup2 / { print $5; exit }' /proc/self/mountinfo 2>/dev/null)"
if [ -n "$CGROUP2_ROOT" ] && [ -r "$CGROUP2_ROOT/cgroup.controllers" ]; then
    CGROUP_CONTROLLERS="$(cat "$CGROUP2_ROOT/cgroup.controllers" 2>/dev/null)"
    [ -n "$CGROUP_CONTROLLERS" ] || CGROUP_CONTROLLERS="none"
    report "cgroup v2: mounted at $CGROUP2_ROOT; controllers=$CGROUP_CONTROLLERS"
else
    CGROUP2_ROOT=""
    report "cgroup v2: no readable unified mount detected; runtime will evaluate available fallbacks"
fi

TOP_APP_PATH=""
for CANDIDATE in "$CGROUP2_ROOT/top-app" /sys/fs/cgroup/top-app /dev/cpuctl/top-app; do
    [ -n "$CANDIDATE" ] || continue
    if [ -d "$CANDIDATE" ]; then
        TOP_APP_PATH="$CANDIDATE"
        break
    fi
done
if [ -n "$TOP_APP_PATH" ]; then
    report "top-app cgroup: $TOP_APP_PATH"
    if [ -r "$TOP_APP_PATH/cpu.uclamp.min" ] && [ -r "$TOP_APP_PATH/cpu.uclamp.max" ]; then
        UCLAMP_MIN="$(cat "$TOP_APP_PATH/cpu.uclamp.min" 2>/dev/null)"
        UCLAMP_MAX="$(cat "$TOP_APP_PATH/cpu.uclamp.max" 2>/dev/null)"
        [ -n "$UCLAMP_MIN" ] || UCLAMP_MIN="unreadable"
        [ -n "$UCLAMP_MAX" ] || UCLAMP_MAX="unreadable"
        report "UClamp nodes visible: min=$UCLAMP_MIN; max=$UCLAMP_MAX; encoding is verified by the native engine"
    else
        report "UClamp nodes: not readable at the top-app path visible to installer"
    fi
else
    report "top-app cgroup: not found at common mount locations; native discovery remains authoritative"
fi

CPU_POSSIBLE="$(read_value /sys/devices/system/cpu/possible)"
CPU_PRESENT="$(read_value /sys/devices/system/cpu/present)"
CPU_ONLINE="$(read_value /sys/devices/system/cpu/online)"
[ -n "$CPU_POSSIBLE" ] || CPU_POSSIBLE="unknown"
[ -n "$CPU_PRESENT" ] || CPU_PRESENT="unknown"
[ -n "$CPU_ONLINE" ] || CPU_ONLINE="unknown"
report "CPU topology: possible=$CPU_POSSIBLE; present=$CPU_PRESENT; online=$CPU_ONLINE"

CPUFREQ_COUNT=0
for POLICY in /sys/devices/system/cpu/cpufreq/policy*; do
    [ -d "$POLICY" ] || continue
    CPUFREQ_COUNT=$((CPUFREQ_COUNT + 1))
done
report "CPUFreq policies visible: $CPUFREQ_COUNT"
if [ -r /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies ]; then
    report "CPUFreq OPP table: readable on policy0"
else
    report "CPUFreq OPP table: policy0 table not visible; each policy is evaluated independently at runtime"
fi

DEVFREQ_COUNT=0
for DEVFREQ in /sys/class/devfreq/*; do
    [ -d "$DEVFREQ" ] || continue
    DEVFREQ_COUNT=$((DEVFREQ_COUNT + 1))
done
report "Generic devfreq devices visible: $DEVFREQ_COUNT"
if [ -r /sys/class/kgsl/kgsl-3d0/gpubusy ]; then
    report "Qualcomm KGSL gpubusy: readable; current implementation treats Qualcomm KGSL utilization as observation-only"
else
    report "Qualcomm KGSL gpubusy: not visible"
fi

THERMAL_COUNT="$(count_thermal_zones)"
report "Thermal zones with readable temp: $THERMAL_COUNT"
if [ -r /sys/class/power_supply/battery/temp ]; then
    report "Battery temperature node: readable"
else
    report "Battery temperature node: unavailable"
fi
for PSI_DOMAIN in cpu memory io; do
    if [ -r "/proc/pressure/$PSI_DOMAIN" ]; then
        PSI_SAMPLE="$(head -n 1 "/proc/pressure/$PSI_DOMAIN" 2>/dev/null)"
        report "PSI $PSI_DOMAIN: available ($PSI_SAMPLE)"
    else
        report "PSI $PSI_DOMAIN: unavailable"
    fi
done

INPUT_COUNT="$(count_input_nodes)"
report "evdev event nodes visible: $INPUT_COUNT; native ioctl capability filtering decides which nodes are actual touchscreens"

report "Preflight: invoke native read-only capability inspection"
CAPABILITY_TMP="$STATE_DIR/install-capabilities.tmp.$$"
if "$CORE_BIN" --capabilities > "$CAPABILITY_TMP" 2>&1; then
    chmod 0600 "$CAPABILITY_TMP" 2>/dev/null
    if [ -s "$CAPABILITY_TMP" ] && [ ! -L "$STATE_DIR/install-capabilities.log" ]; then
        mv -f "$CAPABILITY_TMP" "$STATE_DIR/install-capabilities.log"
        report "PASS: coreflowd --capabilities completed in read-only mode; report saved to $STATE_DIR/install-capabilities.log"
    else
        rm -f "$CAPABILITY_TMP"
        report "PASS: coreflowd --capabilities returned successfully"
    fi
else
    rm -f "$CAPABILITY_TMP"
    report "INFO: native capability probe could not complete in installer context; service will rediscover after boot"
fi

report "Installer policy: no sysfs, procfs, cgroup, boot property, thermal, or SELinux mutation is performed by customize.sh"

# Persist only the read-only preflight report; never rewrite an existing safety marker.
PREFLIGHT_LOG="$STATE_DIR/install-preflight.log"
if [ ! -L "$PREFLIGHT_LOG" ]; then
    PREFLIGHT_TMP="$PREFLIGHT_LOG.tmp.$"
    rm -f "$PREFLIGHT_TMP"
    printf '%s\n' "$PREFLIGHT_REPORT" > "$PREFLIGHT_TMP" &&
        chmod 0600 "$PREFLIGHT_TMP" &&
        mv -f "$PREFLIGHT_TMP" "$PREFLIGHT_LOG"
fi

# Keep legacy module removal manager-controlled; do not recursively delete an old installation.
if [ -d /data/adb/modules/coreflow_engine ] &&
   [ "$MODPATH" != /data/adb/modules/coreflow_engine ]; then
    ui "- Marking legacy CoreFlow Engine installation for root-manager removal..."
    touch /data/adb/modules/coreflow_engine/remove 2>/dev/null || true
fi

# Apply package permissions after validation.
set_file_perm "$CORE_BIN" 0755 || die "Cannot set coreflowd permissions."
set_file_perm "$MODPATH/service.sh" 0755
set_file_perm "$MODPATH/post-fs-data.sh" 0755
set_file_perm "$MODPATH/uninstall.sh" 0755
set_file_perm "$MODPATH/customize.sh" 0755
set_file_perm "$MODPATH/module.prop" 0644
set_file_perm "$MODPATH/sepolicy.rule" 0644
set_file_perm "$MODPATH/system/etc/coreflow/default.conf" 0644
set_file_perm "$MODPATH/system/lib64/libVkLayer_coreflow.so" 0644
set_file_perm "$MODPATH/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" 0644
set_webroot_perm "$MODPATH/webroot"

if [ -d "$LIVE_MOD" ] && [ "$MODPATH" != "$LIVE_MOD" ]; then
    ui "- Staging verified live hot-upgrade into active module..."
    UPGRADE_OK=1

    stage_and_replace "$CORE_BIN" "$LIVE_MOD/system/bin/coreflowd" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/system/lib64/libVkLayer_coreflow.so" "$LIVE_MOD/system/lib64/libVkLayer_coreflow.so" 0644 || UPGRADE_OK=0
    stage_optional "$MODPATH/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" "$LIVE_MOD/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" 0644 || UPGRADE_OK=0
    stage_optional "$MODPATH/system/etc/coreflow/default.conf" "$LIVE_MOD/system/etc/coreflow/default.conf" 0644 || UPGRADE_OK=0
    stage_optional "$MODPATH/module.prop" "$LIVE_MOD/module.prop" 0644 || UPGRADE_OK=0
    stage_optional "$MODPATH/service.sh" "$LIVE_MOD/service.sh" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/post-fs-data.sh" "$LIVE_MOD/post-fs-data.sh" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/uninstall.sh" "$LIVE_MOD/uninstall.sh" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/customize.sh" "$LIVE_MOD/customize.sh" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/sepolicy.rule" "$LIVE_MOD/sepolicy.rule" 0644 || UPGRADE_OK=0

    if [ -d "$MODPATH/webroot" ]; then
        mkdir -p "$LIVE_MOD/webroot" || UPGRADE_OK=0
        cp -rf "$MODPATH/webroot/." "$LIVE_MOD/webroot/" 2>/dev/null || UPGRADE_OK=0
        set_webroot_perm "$LIVE_MOD/webroot" || UPGRADE_OK=0
    fi

    if [ "$UPGRADE_OK" -ne 1 ]; then
        ui "! Live upgrade staging incomplete; the installed module remains in place and new files apply after reboot."
    else
        LIVE_BIN="$LIVE_MOD/system/bin/coreflowd"
        "$LIVE_BIN" --stop >/dev/null 2>&1 || killall -TERM coreflowd >/dev/null 2>&1 || true

        # Safety markers are authoritative. A hot-upgrade must never clear them.
        if [ -e "$STATE_DIR/SAFE_MODE" ] || [ -e "$STATE_DIR/DISABLE" ] ||
           [ -e "$LIVE_MOD/disable" ] || [ -e /data/adb/modules/coreflow_engine/disable ]; then
            ui "- SAFE_MODE/DISABLE marker preserved; automatic daemon restart skipped."
        else
            mkdir -p "$STATE_DIR/logs" 2>/dev/null || true
            LIVE_LOG="$STATE_DIR/logs/coreflowd.log"
            if [ ! -L "$LIVE_LOG" ] &&
               "$LIVE_BIN" --daemon >> "$LIVE_LOG" 2>&1 </dev/null; then
                START_ATTEMPT=0
                DAEMON_READY=0
                while [ "$START_ATTEMPT" -lt 20 ]; do
                    if "$LIVE_BIN" --status 2>/dev/null | grep -q '"running":true'; then
                        DAEMON_READY=1
                        break
                    fi
                    sleep 0.5
                    START_ATTEMPT=$((START_ATTEMPT + 1))
                done
                if [ "$DAEMON_READY" -eq 1 ]; then
                    ui "- Live daemon restart confirmed by native --status."
                else
                    ui "! Live daemon restart was not confirmed; reboot before relying on the new package."
                fi
            else
                ui "! Live daemon start was not confirmed; reboot before relying on the new package."
            fi
        fi
    fi
else
    ui "- First-time package staging complete."
    ui "- Reboot once to mount the systemless overlay and let service.sh start coreflowd after boot completion."
fi

ui "**********************************************"
ui "CFAE installer preflight complete."
ui "Read-only device report: $STATE_DIR/install-preflight.log"
ui "Native runtime remains responsible for final capability admission."
ui "**********************************************"
