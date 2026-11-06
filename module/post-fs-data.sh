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

