#!/system/bin/sh
# Early boot guard; no native daemon is launched until Android boot completion.
export PATH="/system/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin:${PATH:-/usr/bin:/bin}"
umask 077

MODDIR="${0%/*}"
STATE="${COREFLOW_STATE_DIR:-/data/adb/coreflow}"
LOGS="$STATE/logs"

# Refuse to follow symlinked state directories.
[ -L "$STATE" ] && exit 1
mkdir -p "$STATE" || exit 1
[ -L "$LOGS" ] && exit 1
mkdir -p "$LOGS" || exit 1
chmod 700 "$STATE" "$LOGS" 2>/dev/null

ATTEMPTS="$STATE/boot_attempts"
[ -L "$ATTEMPTS" ] && rm -f "$ATTEMPTS"
BOOT_EPOCH="$STATE/boot_epoch"
[ -L "$BOOT_EPOCH" ] && rm -f "$BOOT_EPOCH"

write_atomic() {
  TARGET="$1"
  VALUE="$2"
  TMP="${TARGET}.tmp.$$"
  [ -L "$TARGET" ] && return 1
  rm -f "$TMP"
  printf '%s\n' "$VALUE" > "$TMP" && chmod 600 "$TMP" && mv -f "$TMP" "$TARGET"
}

# Generate a unique per-boot epoch token so BaselineStore can detect stale journals
# even on kernels or SELinux policies where /proc/sys/kernel/random/boot_id is unreadable.
KERNEL_BOOT_ID="$(cat /proc/sys/kernel/random/boot_id 2>/dev/null | tr -d '[:space:]')"
RAND_UUID="$(cat /proc/sys/kernel/random/uuid 2>/dev/null | tr -d '[:space:]')"
if [ -z "$RAND_UUID" ]; then
  RAND_UUID="$(od -An -tx1 -N16 /dev/urandom 2>/dev/null | tr -d ' \n')"
fi
if [ -n "$KERNEL_BOOT_ID" ]; then
  EPOCH_TOKEN="boot-${KERNEL_BOOT_ID}"
elif [ -n "$RAND_UUID" ]; then
  EPOCH_TOKEN="epoch-${RAND_UUID}-$$"
else
  EPOCH_TOKEN="epoch-$(date +%s 2>/dev/null || echo 0)-$$"
fi
write_atomic "$BOOT_EPOCH" "$EPOCH_TOKEN" || exit 1

# An explicit operator disable is intentional, not a failed boot.
if [ -f "$STATE/DISABLE" ] && [ ! -L "$STATE/DISABLE" ]; then
  write_atomic "$ATTEMPTS" "0"
  exit 0
fi

N="$(cat "$ATTEMPTS" 2>/dev/null)"
case "$N" in ''|*[!0-9]*) N=0 ;; esac
N=$((N + 1))
write_atomic "$ATTEMPTS" "$N"

if [ "$N" -ge 5 ]; then
  [ ! -L "$STATE/SAFE_MODE" ] && : > "$STATE/SAFE_MODE"
  [ ! -L "$LOGS/boot_guard.log" ] && printf '%s Safe mode after %s unconfirmed boots\n' "$(date)" "$N" >> "$LOGS/boot_guard.log"
fi
if [ "$N" -ge 8 ]; then
  [ ! -L "$MODDIR/disable" ] && : > "$MODDIR/disable"
  [ ! -L "$LOGS/boot_guard.log" ] && printf '%s Module disabled after %s unconfirmed boots\n' "$(date)" "$N" >> "$LOGS/boot_guard.log"
fi

# Persistent config is kept outside the module directory, surviving upgrades.
DEFAULT_CONF="$MODDIR/system/etc/coreflow/default.conf"
[ -f "$DEFAULT_CONF" ] || DEFAULT_CONF="/system/etc/coreflow/default.conf"
if [ ! -e "$STATE/coreflow.conf" ] && [ ! -L "$STATE/coreflow.conf" ] && [ -f "$DEFAULT_CONF" ]; then
  TMP_CONF="$STATE/coreflow.conf.tmp.$$"
  rm -f "$TMP_CONF"
  if cp "$DEFAULT_CONF" "$TMP_CONF" && chmod 600 "$TMP_CONF"; then
    mv -f "$TMP_CONF" "$STATE/coreflow.conf"
  else
    rm -f "$TMP_CONF"
  fi
fi

# Xiaomi/HyperOS-only UI policy experiment.
# Select one profile by writing /data/adb/coreflow/xiaomi_ui_mode:
#   both | low_ram_only | threshold_only | disabled
# Default is "both" for compatibility with the current behavior. The mode is
# deliberately evaluated only after Xiaomi/Redmi/POCO vendor detection.
BRAND="$(getprop ro.product.brand 2>/dev/null | tr '[:upper:]' '[:lower:]')"
MANUFACTURER="$(getprop ro.product.manufacturer 2>/dev/null | tr '[:upper:]' '[:lower:]')"

if case "$BRAND" in *xiaomi*|*redmi*|*poco*) true;; *) false;; esac || \
   case "$MANUFACTURER" in *xiaomi*|*redmi*|*poco*) true;; *) false;; esac; then
    XIAOMI_UI_MODE="both"
    MODE_FILE="$STATE/xiaomi_ui_mode"
    if [ -f "$MODE_FILE" ] && [ ! -L "$MODE_FILE" ]; then
      IFS= read -r XIAOMI_UI_MODE < "$MODE_FILE" || true
    fi

    case "$XIAOMI_UI_MODE" in
      both|low_ram_only|threshold_only|disabled) ;;
      *) XIAOMI_UI_MODE="disabled" ;;
    esac

    if command -v resetprop >/dev/null 2>&1; then
      case "$XIAOMI_UI_MODE" in
        both|low_ram_only)
          resetprop -n ro.config.low_ram false
          ;;
      esac

      case "$XIAOMI_UI_MODE" in
        both|threshold_only)
          MEMINFO_PATH="${COREFLOW_MEMINFO_PATH:-/proc/meminfo}"
          TOTAL_KB="$(awk '$1 == "MemTotal:" && $2 ~ /^[0-9]+$/ { print $2; exit }' "$MEMINFO_PATH" 2>/dev/null)"
          case "$TOTAL_KB" in
            ''|*[!0-9]*) TOTAL_KB="" ;;
          esac

          # Preserve the OEM threshold when memory detection is unavailable.
          # Do not assume that zero means "unlimited" across Xiaomi builds.
          if [ -n "$TOTAL_KB" ]; then
            RAM_TIER_GB=$(((TOTAL_KB + 1048575) / 1048576))
            if [ "$RAM_TIER_GB" -gt 1 ]; then
              RAM_THRESHOLD_GB=$((RAM_TIER_GB - 1))
              resetprop -n ro.config.low_ram.middle.threshold_gb "$RAM_THRESHOLD_GB"
            fi
          fi
          ;;
      esac
    fi
fi
