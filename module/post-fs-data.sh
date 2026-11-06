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

# Xiaomi/Redmi/POCO memory-tier policy. These properties are vendor-specific
# and must never be applied to other manufacturers.
BRAND=$(getprop ro.product.brand 2>/dev/null | tr '[:upper:]' '[:lower:]')
MANUF=$(getprop ro.product.manufacturer 2>/dev/null | tr '[:upper:]' '[:lower:]')
case "$BRAND $MANUF" in
  *xiaomi*|*redmi*|*poco*)
    XIAOMI_MODE="both"
    if [ -f "$STATE/xiaomi_ui_mode" ] && [ ! -L "$STATE/xiaomi_ui_mode" ]; then
      XIAOMI_MODE=$(cat "$STATE/xiaomi_ui_mode" 2>/dev/null | tr -d '[:space:]')
    fi
    case "$XIAOMI_MODE" in
      both|low_ram_only|threshold_only|disabled) ;;
      *)
        printf '%s Invalid Xiaomi UI mode; policy disabled: %s\n' "$(date)" "$XIAOMI_MODE" >> "$LOGS/xiaomi_ui_policy.log"
        XIAOMI_MODE="disabled"
        ;;
    esac

    apply_xiaomi_property() {
      PROP_NAME="$1"
      PROP_VALUE="$2"
      if ! command -v resetprop >/dev/null 2>&1; then
        printf '%s resetprop unavailable; not applied: %s\n' "$(date)" "$PROP_NAME" >> "$LOGS/xiaomi_ui_policy.log"
        return 1
      fi
      if ! resetprop -n "$PROP_NAME" "$PROP_VALUE"; then
        printf '%s resetprop failed: %s=%s\n' "$(date)" "$PROP_NAME" "$PROP_VALUE" >> "$LOGS/xiaomi_ui_policy.log"
        return 1
      fi
      ACTUAL_VALUE=$(getprop "$PROP_NAME" 2>/dev/null)
      if [ "$ACTUAL_VALUE" != "$PROP_VALUE" ]; then
        printf '%s readback mismatch: %s expected=%s actual=%s\n' "$(date)" "$PROP_NAME" "$PROP_VALUE" "$ACTUAL_VALUE" >> "$LOGS/xiaomi_ui_policy.log"
        return 1
      fi
      printf '%s applied and verified: %s=%s\n' "$(date)" "$PROP_NAME" "$PROP_VALUE" >> "$LOGS/xiaomi_ui_policy.log"
      return 0
    }

    APPLY_LOW_RAM=0
    APPLY_THRESHOLD=0
    case "$XIAOMI_MODE" in
      low_ram_only) APPLY_LOW_RAM=1 ;;
      threshold_only) APPLY_THRESHOLD=1 ;;
      both) APPLY_LOW_RAM=1; APPLY_THRESHOLD=1 ;;
    esac

    if [ "$APPLY_LOW_RAM" -eq 1 ]; then
      apply_xiaomi_property "ro.config.low_ram" "false" || :
    fi

    if [ "$APPLY_THRESHOLD" -eq 1 ]; then
      MEMINFO="${COREFLOW_MEMINFO_PATH:-/proc/meminfo}"
      MEM_TOTAL_KB=0
      if [ -f "$MEMINFO" ] && [ -r "$MEMINFO" ]; then
        MEM_TOTAL_KB=$(awk '$1 == "MemTotal:" && $2 ~ /^[0-9]+$/ { print $2; exit }' "$MEMINFO" 2>/dev/null)
      fi
      case "$MEM_TOTAL_KB" in ''|*[!0-9]*) MEM_TOTAL_KB=0 ;; esac
      if [ "$MEM_TOTAL_KB" -gt 0 ]; then
        # Round physical RAM to the nearest GiB, then reserve one GiB for
        # the middle-tier boundary. Do not invent a threshold on missing data.
        RAM_GB=$(( (MEM_TOTAL_KB + 524288) / 1048576 ))
        if [ "$RAM_GB" -gt 1 ]; then
          THRESH_GB=$(( RAM_GB - 1 ))
          apply_xiaomi_property "ro.config.low_ram.middle.threshold_gb" "$THRESH_GB" || :
        else
          printf '%s RAM below threshold-policy minimum; threshold unchanged\n' "$(date)" >> "$LOGS/xiaomi_ui_policy.log"
        fi
      else
        printf '%s MemTotal unavailable or invalid; threshold unchanged\n' "$(date)" >> "$LOGS/xiaomi_ui_policy.log"
      fi
    fi
    ;;
esac
