#!/system/bin/sh
# CFAE System Framework / CoreFlow Engine v6.1.1 — Sovereign Preflight, Hard-Block & Atomic Upgrade Installer.
# All runtime capability discovery, hardware calibration, and kernel actuation remain owned by coreflowd.

PATH="/system/bin:/system/xbin:/sbin:/vendor/bin:$PATH"
export PATH
umask 077

# ---------------------------------------------------------------------------
# UI & Formatting Helpers (POSIX sh / Toybox / BusyBox / mksh safe)
# ---------------------------------------------------------------------------
ui() {
    if command -v ui_print >/dev/null 2>&1; then
        ui_print "$1"
    else
        echo "$1"
    fi
}

die() {
    ui ""
    ui "  [FAIL] $1"
    if command -v abort >/dev/null 2>&1; then
        abort "! $1"
    fi
    exit 1
}

report() {
    PREFLIGHT_REPORT="$PREFLIGHT_REPORT
$1"
}

section() {
    ui ""
    ui "+------------------------------------------------------+"
    ui "| $1"
    ui "+------------------------------------------------------+"
    PREFLIGHT_REPORT="$PREFLIGHT_REPORT

=== $1 ==="
}

status_line() {
    printf '  [%-7s] %s\n' "$1" "$2" | while IFS= read -r LINE; do
        ui "$LINE"
    done
    report "[$1] $2"
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

# Pure POSIX JSON field extractors (zero dependency on jq/python)
json_str() {
    printf '%s' "$1" | sed -n "s/.*\"$2\":\"\([^\"]*\)\".*/\1/p" | head -n 1
}

json_val() {
    printf '%s' "$1" | sed -n "s/.*\"$2\":\([^,}]*\).*/\1/p" | head -n 1 | tr -d ' "'
}

bytes_to_gb() {
    is_numeric "$1" || { echo "0.0"; return; }
    awk -v b="$1" 'BEGIN { printf "%.1f", b / 1073741824.0 }'
}

sha256_short() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" 2>/dev/null | awk '{print substr($1,1,16)}'
    elif command -v toybox >/dev/null 2>&1; then
        toybox sha256sum "$1" 2>/dev/null | awk '{print substr($1,1,16)}'
    else
        echo "unavailable"
    fi
}

# ---------------------------------------------------------------------------
# Safe Atomic Staging & Permission Helpers
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# Environment & Hardware Inventory Helpers (Strictly Read-Only)
# ---------------------------------------------------------------------------
detect_root_manager() {
    if [ -n "${KSU:-}" ] || [ -n "${KSU_VER_CODE:-}" ] ||
       [ -d /data/adb/ksu ] || [ -x /data/adb/ksud ]; then
        if [ -n "${KSU_VER:-}" ]; then
            ROOT_MANAGER="KernelSU ($KSU_VER)"
        elif [ -n "${KSU_VER_CODE:-}" ]; then
            ROOT_MANAGER="KernelSU (code $KSU_VER_CODE)"
        else
            ROOT_MANAGER="KernelSU / KernelSU Next"
        fi
    elif [ -n "${APATCH:-}" ] || [ -n "${APATCH_VER_CODE:-}" ] ||
         [ -d /data/adb/ap ] || [ -x /data/adb/apd ]; then
        if [ -n "${APATCH_VER:-}" ]; then
            ROOT_MANAGER="APatch ($APATCH_VER)"
        else
            ROOT_MANAGER="APatch"
        fi
    elif [ -n "${MAGISK_VER_CODE:-}" ] || [ -d /data/adb/magisk ] ||
         [ -x /data/adb/magisk/magisk32 ] || [ -x /sbin/.magisk/magisk ]; then
        if [ -n "${MAGISK_VER:-}" ]; then
            ROOT_MANAGER="Magisk ($MAGISK_VER)"
        else
            ROOT_MANAGER="Magisk"
        fi
    elif [ -n "${BOOTMODE:-}" ] || [ -n "${MODPATH:-}" ]; then
        ROOT_MANAGER="Root-manager installer environment"
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

detect_conflicting_modules() {
    CONFLICTS=""
    CONFLICTS_JSON=""
    CONFLICT_COUNT=0
    MOD_ROOT="${COREFLOW_MODULES_ROOT:-/data/adb/modules}"
    [ -d "$MOD_ROOT" ] || return 0
    for DIR in "$MOD_ROOT"/*; do
        [ -d "$DIR" ] || continue
        [ ! -e "$DIR/disable" ] && [ ! -e "$DIR/remove" ] || continue
        MID="$(basename "$DIR")"
        case "$MID" in
            cfae_framework|coreflow_engine) continue ;;
        esac
        MPROP="$DIR/module.prop"
        [ -r "$MPROP" ] || continue
        MNAME="$(awk -F= '$1 == "name" { print $2; exit }' "$MPROP" 2>/dev/null)"
        MDESC="$(awk -F= '$1 == "description" { print $2; exit }' "$MPROP" 2>/dev/null)"
        FOLDED_ID_NAME="$(printf '%s %s' "$MID" "$MNAME" | tr 'A-Z' 'a-z')"
        FOLDED_ALL="$(printf '%s %s %s' "$MID" "$MNAME" "$MDESC" | tr 'A-Z' 'a-z')"
        IS_CONFLICT=0
        # 1. Known competing performance/thermal/scheduler optimizer module IDs & names
        case "$FOLDED_ID_NAME" in
            *encore*|*uperf*|*fas-rs*|*fas_rs*|*fasrs*|*ktweak*|*magnetar*|*godspeed*|*celestial*|*strp*|*fkm*|*magisk-perf*|*kakarot*|*vortex*|*feast*|*nfs-injector*|*fde.ai*|*fde_ai*|*asopt*|*yc_sched*|*lyb*|*no-thermal*|*nothermal*|*stop-thermal*|*thermal-mod*|*thermal_mod*|*thermal-kill*|*disable-thermal*|*cpu-governor*|*uclamp*|*fpsgo*)
                IS_CONFLICT=1
                ;;
        esac
        # 2. Explicit kernel/thermal/CPU-tuner phrases in full metadata (avoids false positives on
        #    harmless audio "bass boost", UI "font tweaks", or "perfect integrity" modules)
        if [ "$IS_CONFLICT" -eq 0 ]; then
            case "$FOLDED_ALL" in
                *"encore tweaks"*|*"fas-rs"*|*"fas_rs"*|*"franco kernel"*|*"uclamp"*|*"cpu governor"*|*"sched_boost"*|*"disable thermal"*|*"thermal throttling"*|*"thermal mod"*|*"thermal engine"*|*"kernel tweak"*|*"performance module"*|*"game optimizer"*|*"fpsgo"*|*"walt governor"*)
                    IS_CONFLICT=1
                    ;;
            esac
        fi
        if [ "$IS_CONFLICT" -eq 1 ]; then
            CONFLICT_COUNT=$((CONFLICT_COUNT + 1))
            if [ -n "$MNAME" ] && [ "$MNAME" != "$MID" ]; then
                ENTRY_LABEL="$MNAME ($MID)"
            else
                ENTRY_LABEL="$MID"
            fi
            SAFE_JSON_NAME="$(printf '%s' "$ENTRY_LABEL" | sed 's/["\\]/ /g')"
            if [ -z "$CONFLICTS" ]; then
                CONFLICTS="$ENTRY_LABEL"
                CONFLICTS_JSON="\"$SAFE_JSON_NAME\""
            else
                CONFLICTS="$CONFLICTS, $ENTRY_LABEL"
                CONFLICTS_JSON="$CONFLICTS_JSON,\"$SAFE_JSON_NAME\""
            fi
        fi
    done
}

# ---------------------------------------------------------------------------
# Banner & Staging Guards
# ---------------------------------------------------------------------------
ui ""
ui "+======================================================+"
ui "|       CFAE SYSTEM FRAMEWORK  v6.1.1 (REBUILD)      |"
ui "|      SOVEREIGN AUTONOMOUS ANDROID SYSTEM ENGINE      |"
ui "|              by mystivara-creator                    |"
ui "+======================================================+"
ui "  Sovereign Guard · Coupled Domains · Zero-Wear DNA"

[ -n "$MODPATH" ] || die "MODPATH is not provided by the root-manager installer."
[ -d "$MODPATH" ] || die "Module staging directory does not exist: $MODPATH"
[ ! -L "$MODPATH" ] || die "Module staging directory must not be a symlink."

STATE_DIR="${COREFLOW_STATE_DIR:-/data/adb/coreflow}"
LIVE_MOD="${COREFLOW_LIVE_MOD_DIR:-/data/adb/modules/cfae_framework}"

[ ! -L "$STATE_DIR" ] || die "Refusing symlinked runtime state directory: $STATE_DIR"
[ ! -L "$LIVE_MOD" ] || die "Refusing symlinked active module directory: $LIVE_MOD"
mkdir -p "$STATE_DIR" || die "Cannot prepare runtime state directory."
chmod 0700 "$STATE_DIR" 2>/dev/null

PREFLIGHT_REPORT="CFAE System Framework / CoreFlow Engine v6.1.1 — read-only installer preflight"

# Detect previous installation version for transparent upgrade reporting
PREV_VERSION=""
if [ -r "$LIVE_MOD/module.prop" ] && [ ! -L "$LIVE_MOD/module.prop" ]; then
    PREV_VERSION="$(awk -F= '$1 == "version" { print $2; exit }' "$LIVE_MOD/module.prop" 2>/dev/null)"
fi

# ---------------------------------------------------------------------------
# 01 / DEVICE & PLATFORM VALIDATION
# ---------------------------------------------------------------------------
section "01 / DEVICE & PLATFORM"

INSTALL_ARCH="$ARCH"
[ -n "$INSTALL_ARCH" ] || INSTALL_ARCH="$(uname -m 2>/dev/null)"
case "$INSTALL_ARCH" in
    arm64|aarch64|arm64-v8a) ;;
    *) die "Unsupported architecture ($INSTALL_ARCH). CoreFlow requires ARM64." ;;
esac

API_LEVEL="$API"
is_numeric "$API_LEVEL" || API_LEVEL="$(get_property ro.build.version.sdk)"
is_numeric "$API_LEVEL" || die "Android API level could not be determined."
[ "$API_LEVEL" -ge 33 ] || die "Unsupported Android API $API_LEVEL. Android 13 / API 33+ is required."

ANDROID_RELEASE="$(get_property ro.build.version.release)"
MANUFACTURER="$(get_property ro.product.manufacturer)"
BRAND="$(get_property ro.product.brand)"
MODEL="$(get_property ro.product.model)"
DEVICE="$(get_property ro.product.device)"
ABI_LIST="$(get_property ro.product.cpu.abilist64)"
[ -n "$MANUFACTURER" ] || MANUFACTURER="unknown"
[ -n "$BRAND" ] || BRAND="unknown"
[ -n "$MODEL" ] || MODEL="unknown"
[ -n "$DEVICE" ] || DEVICE="unknown"
[ -n "$ANDROID_RELEASE" ] || ANDROID_RELEASE="unknown"

case "$ABI_LIST" in
    *arm64-v8a*|'') ;;
    *) die "64-bit ABI list does not contain arm64-v8a: $ABI_LIST" ;;
esac

ROOT_MANAGER=""
detect_root_manager
KERNEL_RELEASE="$(uname -r 2>/dev/null)"
[ -n "$KERNEL_RELEASE" ] || KERNEL_RELEASE="unknown"
if command -v getenforce >/dev/null 2>&1; then
    SELINUX_STATE="$(getenforce 2>/dev/null)"
else
    SELINUX_STATE="unknown"
fi
[ -n "$SELINUX_STATE" ] || SELINUX_STATE="unknown"

status_line "PASS" "Platform : Android $ANDROID_RELEASE (API $API_LEVEL) · $INSTALL_ARCH"
status_line "INFO" "Device   : $BRAND $MODEL ($DEVICE)"
status_line "INFO" "Kernel   : $KERNEL_RELEASE"
status_line "INFO" "Root/MAC : $ROOT_MANAGER · SELinux $SELINUX_STATE"
if [ -n "$PREV_VERSION" ]; then
    status_line "UPGRADE" "Detected existing installation: $PREV_VERSION -> v6.1.1"
else
    status_line "INSTALL" "Fresh installation of CFAE System Framework v6.1.1"
fi

# ---------------------------------------------------------------------------
# 02 / PACKAGE INTEGRITY & BINARY VERIFICATION
# ---------------------------------------------------------------------------
section "02 / PACKAGE INTEGRITY"

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
    status_line "TEST" "Non-ELF mock daemon accepted by regression harness"
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
    BIN_SHA="$(sha256_short "$CORE_BIN")"
    status_line "PASS" "ELF64 AArch64 binary verified (sha256:${BIN_SHA}...)"
fi

if [ -f "$MODPATH/module.prop" ] && [ ! -L "$MODPATH/module.prop" ]; then
    MODULE_ID="$(awk -F= '$1 == "id" { print $2; exit }' "$MODPATH/module.prop")"
    MODULE_VERSION="$(awk -F= '$1 == "version" { print $2; exit }' "$MODPATH/module.prop")"
    MODULE_CODE="$(awk -F= '$1 == "versionCode" { print $2; exit }' "$MODPATH/module.prop")"
    [ "$MODULE_ID" = "cfae_framework" ] || die "Unexpected module id: $MODULE_ID"
    status_line "PASS" "Metadata : $MODULE_ID · $MODULE_VERSION (code $MODULE_CODE)"
fi

for REQ_SCRIPT in service.sh post-fs-data.sh uninstall.sh action.sh; do
    [ -f "$MODPATH/$REQ_SCRIPT" ] || die "$REQ_SCRIPT missing from package."
done
if [ -f "$MODPATH/sepolicy.rule" ]; then
    RULE_COUNT="$(grep -c '^allow ' "$MODPATH/sepolicy.rule" 2>/dev/null || echo 0)"
    status_line "PASS" "SELinux  : scoped live policy packaged ($RULE_COUNT targeted rules)"
else
    status_line "WARN" "SELinux  : sepolicy.rule not packaged"
fi
if [ -f "$MODPATH/webroot/index.html" ]; then
    status_line "PASS" "WebUI    : MD3 Control Center payload verified"
else
    status_line "WARN" "WebUI    : webroot/index.html missing"
fi

# ---------------------------------------------------------------------------
# 03 / NATIVE HARDWARE & 4-PILLAR READINESS PROBE
# ---------------------------------------------------------------------------
section "03 / 4-PILLAR HARDWARE READINESS"

# Apply executable bit before read-only native probe
set_file_perm "$CORE_BIN" 0755 || die "Cannot set coreflowd permissions."

CAPABILITY_TMP="$STATE_DIR/install-capabilities.tmp.$$"
CAP_JSON=""
NATIVE_PROBE_STATUS="DEFERRED"
if "$CORE_BIN" --capabilities > "$CAPABILITY_TMP" 2>&1; then
    NATIVE_PROBE_STATUS="PASS"
    CAP_JSON="$(tail -n 1 "$CAPABILITY_TMP" 2>/dev/null)"
    chmod 0600 "$CAPABILITY_TMP" 2>/dev/null
    if [ -s "$CAPABILITY_TMP" ] && [ ! -L "$STATE_DIR/install-capabilities.log" ]; then
        mv -f "$CAPABILITY_TMP" "$STATE_DIR/install-capabilities.log"
    else
        rm -f "$CAPABILITY_TMP"
    fi
else
    rm -f "$CAPABILITY_TMP"
fi

# Baseline sysfs read-only signals
CGROUP2_ROOT="$(awk '$0 ~ / - cgroup2 / { print $5; exit }' /proc/self/mountinfo 2>/dev/null)"
if [ -n "$CGROUP2_ROOT" ] && [ -r "$CGROUP2_ROOT/cgroup.controllers" ]; then
    CGROUP_CONTROLLERS="$(cat "$CGROUP2_ROOT/cgroup.controllers" 2>/dev/null)"
    [ -n "$CGROUP_CONTROLLERS" ] || CGROUP_CONTROLLERS="none"
    if [ "$CGROUP_CONTROLLERS" = "none" ]; then CGROUP_STATUS="LIMITED"; else CGROUP_STATUS="AVAILABLE"; fi
else
    CGROUP_CONTROLLERS="unavailable"
    CGROUP_STATUS="UNAVAILABLE"
fi

THERMAL_COUNT="$(count_thermal_zones)"
INPUT_COUNT="$(count_input_nodes)"
PSI_AVAILABLE_COUNT=0
for PSI_DOMAIN in cpu memory io; do
    [ -r "/proc/pressure/$PSI_DOMAIN" ] && PSI_AVAILABLE_COUNT=$((PSI_AVAILABLE_COUNT + 1))
done

SOC_MODEL="runtime-auto"
GPU_MODEL="runtime-auto"
STORAGE_TYPE="runtime-auto"
RAM_GB="?"
ZRAM_GB="?"
CPU_CLUSTERS="?"
REFRESH_HZ="?"
P1_UCLAMP="deferred"
P3_MGLRU="deferred"
P2_CGROUP="deferred"

if [ "$NATIVE_PROBE_STATUS" = "PASS" ] && [ -n "$CAP_JSON" ]; then
    SOC_TMP="$(json_str "$CAP_JSON" "socModel")"
    GPU_TMP="$(json_str "$CAP_JSON" "gpuModel")"
    STO_TMP="$(json_str "$CAP_JSON" "storageType")"
    RAM_TMP="$(json_val "$CAP_JSON" "ramBytes")"
    ZRM_TMP="$(json_val "$CAP_JSON" "zramBytes")"
    CLU_TMP="$(json_val "$CAP_JSON" "cpuClusters")"
    RHZ_TMP="$(json_val "$CAP_JSON" "refreshHz")"
    UCL_TMP="$(json_val "$CAP_JSON" "perThreadUclampSupported")"
    MGL_TMP="$(json_val "$CAP_JSON" "mglruAvailable")"
    CG2_TMP="$(json_val "$CAP_JSON" "cgroupV2Available")"

    [ -n "$SOC_TMP" ] && SOC_MODEL="$SOC_TMP"
    [ -n "$GPU_TMP" ] && GPU_MODEL="$GPU_TMP"
    [ -n "$STO_TMP" ] && STORAGE_TYPE="$STO_TMP"
    [ -n "$RAM_TMP" ] && RAM_GB="$(bytes_to_gb "$RAM_TMP")"
    [ -n "$ZRM_TMP" ] && ZRAM_GB="$(bytes_to_gb "$ZRM_TMP")"
    [ -n "$CLU_TMP" ] && CPU_CLUSTERS="$CLU_TMP"
    [ -n "$RHZ_TMP" ] && REFRESH_HZ="$RHZ_TMP"
    [ -n "$UCL_TMP" ] && P1_UCLAMP="$UCL_TMP"
    [ -n "$MGL_TMP" ] && P3_MGLRU="$MGL_TMP"
    [ -n "$CG2_TMP" ] && P2_CGROUP="$CG2_TMP"

    status_line "PROBE" "SoC: $SOC_MODEL | GPU: $GPU_MODEL | Clusters: $CPU_CLUSTERS"
    status_line "PROBE" "Storage: $STORAGE_TYPE | RAM: ${RAM_GB} GB | ZRAM: ${ZRAM_GB} GB | Panel: ${REFRESH_HZ} Hz"
else
    status_line "DEFER" "Native probe deferred to post-boot daemon startup"
fi

# 4-Pillar Readiness Breakdown
if [ "$P1_UCLAMP" = "true" ]; then
    status_line "READY" "Pillar 1 — Per-TID Render & UI Smoothness (SYS_sched_setattr)"
elif [ "$P1_UCLAMP" = "false" ]; then
    status_line "LIMITED" "Pillar 1 — Per-TID UClamp unsupported by kernel"
else
    status_line "DEFER" "Pillar 1 — Per-TID UClamp (evaluated at boot)"
fi

if [ "$STORAGE_TYPE" != "Unknown" ] && [ "$STORAGE_TYPE" != "runtime-auto" ]; then
    status_line "READY" "Pillar 2 — Dynamic $STORAGE_TYPE Burst & Background QoS (cgroup2=$P2_CGROUP)"
else
    status_line "DEFER" "Pillar 2 — Dynamic Storage Burst & QoS (evaluated at boot)"
fi

if [ "$P3_MGLRU" = "true" ]; then
    status_line "READY" "Pillar 3 — MGLRU Anti-Stutter Memory & Smart Pre-Read"
elif [ "$P3_MGLRU" = "false" ]; then
    status_line "READY" "Pillar 3 — Adaptive VM Memory Guard & Smart Pre-Read (no MGLRU)"
else
    status_line "DEFER" "Pillar 3 — Adaptive Memory Guard & Smart Pre-Read"
fi

status_line "READY" "Pillar 4 — Rust PID Thermal Pacing ($THERMAL_COUNT zones, PSI $PSI_AVAILABLE_COUNT/3, $INPUT_COUNT evdev)"

# Sovereign Pre-Install Conflict Gate (Hard-Block competing tweakers)
CONFLICTS=""
CONFLICTS_JSON=""
CONFLICT_COUNT=0
detect_conflicting_modules
CONFLICT_FILE="$STATE_DIR/detected_conflicts.json"
if [ "$CONFLICT_COUNT" -gt 0 ]; then
    if [ ! -L "$CONFLICT_FILE" ]; then
        printf '{"count":%s,"modules":[%s]}\n' "$CONFLICT_COUNT" "$CONFLICTS_JSON" > "$CONFLICT_FILE.tmp.$$" 2>/dev/null &&
            chmod 0600 "$CONFLICT_FILE.tmp.$$" 2>/dev/null &&
            mv -f "$CONFLICT_FILE.tmp.$$" "$CONFLICT_FILE" 2>/dev/null || rm -f "$CONFLICT_FILE.tmp.$$"
    fi
    ui ""
    ui "  ======================================================"
    ui "  [SOVEREIGN BLOCK] MODUL PERFORMA BENTROK TERDETEKSI!"
    ui "  ======================================================"
    status_line "CONFLICT" "Terdeteksi $CONFLICT_COUNT modul aktif: $CONFLICTS"
    ui "  CFAE System Framework / CoreFlow Engine bersifat berdaulat"
    ui "  dan tidak boleh berjalan bersamaan dengan modul tweaking"
    ui "  performa/termal lain (seperti Encore Tweaks, UPerf, FAS-RS,"
    ui "  KTweak, Magnetar, atau modul FKM) agar tidak terjadi"
    ui "  bentrok kernel (tug-of-war) & lonjakan suhu."
    ui ""
    ui "  Tindakan wajib: Hapus atau nonaktifkan modul di atas di"
    ui "  KernelSU / Magisk / APatch, reboot, lalu flash ulang CFAE."
    die "Instalasi dibatalkan oleh Sovereign Guard karena modul bentrok: $CONFLICTS"
else
    rm -f "$CONFLICT_FILE" 2>/dev/null || true
    status_line "PASS" "Zero conflicting performance/thermal modules detected (Sovereign Ready)"
fi

# ---------------------------------------------------------------------------
# 04 / CONFIGURATION PRESERVATION & PERMISSIONS
# ---------------------------------------------------------------------------
section "04 / CONFIGURATION & STAGING"

USER_CONF="$STATE_DIR/coreflow.conf"
DEFAULT_CONF="$MODPATH/system/etc/coreflow/default.conf"
if [ -f "$USER_CONF" ] && [ ! -L "$USER_CONF" ]; then
    if [ ! -L "$STATE_DIR/coreflow.conf.bak" ]; then
        cp -f "$USER_CONF" "$STATE_DIR/coreflow.conf.bak" 2>/dev/null &&
            chmod 0600 "$STATE_DIR/coreflow.conf.bak" 2>/dev/null
    fi
    status_line "KEEP" "Preserved user configuration & per-app rules ($USER_CONF)"
elif [ -f "$DEFAULT_CONF" ] && [ ! -L "$DEFAULT_CONF" ]; then
    TMP_CONF="$USER_CONF.tmp.$$"
    rm -f "$TMP_CONF"
    if cp -f "$DEFAULT_CONF" "$TMP_CONF" && chmod 0600 "$TMP_CONF" && mv -f "$TMP_CONF" "$USER_CONF"; then
        status_line "INIT" "Initialized default v6.1.1 configuration ($USER_CONF)"
    else
        rm -f "$TMP_CONF"
        status_line "WARN" "Could not initialize default config; daemon will use compiled defaults"
    fi
fi

# Persist read-only preflight log without touching safety markers
PREFLIGHT_LOG="$STATE_DIR/install-preflight.log"
if [ ! -L "$PREFLIGHT_LOG" ]; then
    PREFLIGHT_TMP="$PREFLIGHT_LOG.tmp.$$"
    rm -f "$PREFLIGHT_TMP"
    printf '%s\n' "$PREFLIGHT_REPORT" > "$PREFLIGHT_TMP" &&
        chmod 0600 "$PREFLIGHT_TMP" &&
        mv -f "$PREFLIGHT_TMP" "$PREFLIGHT_LOG"
fi

# Mark legacy module ID for manager-controlled removal without recursive rm
if [ -d /data/adb/modules/coreflow_engine ] &&
   [ "$MODPATH" != /data/adb/modules/coreflow_engine ]; then
    status_line "MIGRATE" "Marking legacy module coreflow_engine for root-manager removal"
    touch /data/adb/modules/coreflow_engine/remove 2>/dev/null || true
fi

set_file_perm "$MODPATH/service.sh" 0755
set_file_perm "$MODPATH/post-fs-data.sh" 0755
set_file_perm "$MODPATH/uninstall.sh" 0755
set_file_perm "$MODPATH/action.sh" 0755
set_file_perm "$MODPATH/customize.sh" 0755
set_file_perm "$MODPATH/module.prop" 0644
set_file_perm "$MODPATH/sepolicy.rule" 0644
set_file_perm "$MODPATH/system/etc/coreflow/default.conf" 0644
set_webroot_perm "$MODPATH/webroot"

# ---------------------------------------------------------------------------
# 05 / ATOMIC LIVE HOT-UPGRADE OR FRESH STAGING
# ---------------------------------------------------------------------------
if [ -d "$LIVE_MOD" ] && [ "$MODPATH" != "$LIVE_MOD" ]; then
    status_line "STAGE" "Performing atomic live hot-upgrade into active module..."
    UPGRADE_OK=1

    stage_and_replace "$CORE_BIN" "$LIVE_MOD/system/bin/coreflowd" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/system/etc/coreflow/default.conf" "$LIVE_MOD/system/etc/coreflow/default.conf" 0644 || UPGRADE_OK=0
    stage_optional "$MODPATH/module.prop" "$LIVE_MOD/module.prop" 0644 || UPGRADE_OK=0
    stage_optional "$MODPATH/service.sh" "$LIVE_MOD/service.sh" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/post-fs-data.sh" "$LIVE_MOD/post-fs-data.sh" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/uninstall.sh" "$LIVE_MOD/uninstall.sh" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/action.sh" "$LIVE_MOD/action.sh" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/customize.sh" "$LIVE_MOD/customize.sh" 0755 || UPGRADE_OK=0
    stage_optional "$MODPATH/sepolicy.rule" "$LIVE_MOD/sepolicy.rule" 0644 || UPGRADE_OK=0

    if [ -d "$MODPATH/webroot" ]; then
        mkdir -p "$LIVE_MOD/webroot" || UPGRADE_OK=0
        cp -rf "$MODPATH/webroot/." "$LIVE_MOD/webroot/" 2>/dev/null || UPGRADE_OK=0
        set_webroot_perm "$LIVE_MOD/webroot" || UPGRADE_OK=0
    fi

    if [ "$UPGRADE_OK" -ne 1 ]; then
        status_line "WARN" "Live staging incomplete; new files will activate cleanly after reboot"
    else
        LIVE_BIN="$LIVE_MOD/system/bin/coreflowd"
        "$LIVE_BIN" --stop >/dev/null 2>&1 || killall -TERM coreflowd >/dev/null 2>&1 || true

        # Safety markers are authoritative; hot-upgrade never clears SAFE_MODE/DISABLE.
        if [ -e "$STATE_DIR/SAFE_MODE" ] || [ -e "$STATE_DIR/DISABLE" ] ||
           [ -e "$LIVE_MOD/disable" ] || [ -e /data/adb/modules/coreflow_engine/disable ]; then
            status_line "SAFE" "SAFE_MODE/DISABLE marker preserved; automatic restart skipped"
        else
            mkdir -p "$STATE_DIR/logs" 2>/dev/null || true
            LIVE_LOG="$STATE_DIR/logs/coreflowd.log"
            LIVE_SERVICE="$LIVE_MOD/service.sh"
            if [ -f "$LIVE_SERVICE" ] && [ ! -L "$LIVE_SERVICE" ] && [ ! -L "$LIVE_LOG" ]; then
                chmod 0755 "$LIVE_SERVICE" 2>/dev/null || true
                COREFLOW_STATE_DIR="$STATE_DIR" COREFLOW_SKIP_BOOT_WAIT=1 \
                    sh "$LIVE_SERVICE" >/dev/null 2>&1 || true
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
                    status_line "LIVE" "CFAE System Framework v6.1.1 daemon restarted & verified via --status"
                else
                    status_line "WARN" "Live restart unconfirmed; reboot once to activate v6.1.1"
                fi
            else
                status_line "WARN" "Live service.sh unavailable; reboot once to activate v6.1.1"
            fi
        fi
    fi
else
    status_line "DONE" "Fresh staging complete — reboot once to mount overlay & start coreflowd"
fi

ui ""
ui "+======================================================+"
ui "|  CFAE System Framework v6.1.1 — Install Complete   |"
ui "+------------------------------------------------------+"
ui "|  WebUI   : KernelSU / APatch (or KsuWebUI on Magisk) |"
ui "|  Report  : $STATE_DIR/install-preflight.log"
ui "|  Policy  : Zero kernel/SELinux mutation in installer |"
ui "|  Safety  : CFJ4 durable OEM baseline & rollback      |"
ui "+======================================================+"
ui ""
