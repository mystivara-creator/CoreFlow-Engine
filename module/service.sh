#!/system/bin/sh
# CFAE System Framework v3.1.0: post-boot launcher & 120s boot-health confirmation (overlay & system-ready).
export PATH="/system/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin:${PATH:-/usr/bin:/bin}"
umask 077

MODDIR="${0%/*}"
STATE="${COREFLOW_STATE_DIR:-/data/adb/coreflow}"
LOGS="$STATE/logs"
BIN="${COREFLOW_BIN_OVERRIDE:-$MODDIR/system/bin/coreflowd}"
[ -f "$BIN" ] || BIN="$MODDIR/coreflowd"
[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"
[ -f "$BIN" ] || BIN="/system/bin/coreflowd"
[ -f "$BIN" ] || exit 1

if [ "${COREFLOW_SKIP_BOOT_WAIT:-0}" != "1" ]; then
  WAIT_COUNT=0
  while [ "$(getprop sys.boot_completed)" != "1" ]; do
    WAIT_COUNT=$((WAIT_COUNT + 1))
    [ "$WAIT_COUNT" -ge 180 ] && exit 1
    sleep 2
  done
fi

[ -L "$STATE" ] && exit 1
mkdir -p "$STATE" || exit 1
[ -L "$LOGS" ] && exit 1
mkdir -p "$LOGS" || exit 1
chmod 700 "$STATE" "$LOGS" 2>/dev/null

# Ensure boot_epoch exists if post-fs-data.sh was skipped by the root manager.
BOOT_EPOCH="$STATE/boot_epoch"
if [ ! -f "$BOOT_EPOCH" ] && [ ! -L "$BOOT_EPOCH" ]; then
  KERNEL_BOOT_ID="$(cat /proc/sys/kernel/random/boot_id 2>/dev/null | tr -d '[:space:]')"
  RAND_UUID="$(cat /proc/sys/kernel/random/uuid 2>/dev/null | tr -d '[:space:]')"
  if [ -n "$KERNEL_BOOT_ID" ]; then
    EPOCH_TOKEN="boot-${KERNEL_BOOT_ID}"
  elif [ -n "$RAND_UUID" ]; then
    EPOCH_TOKEN="epoch-${RAND_UUID}-$$"
  else
    EPOCH_TOKEN="epoch-$(date +%s 2>/dev/null || echo 0)-$$"
  fi
  TMP_EPOCH="${BOOT_EPOCH}.tmp.$$"
  printf '%s\n' "$EPOCH_TOKEN" > "$TMP_EPOCH" && chmod 600 "$TMP_EPOCH" && mv -f "$TMP_EPOCH" "$BOOT_EPOCH"
fi

# Operator stop and boot-loop recovery are authoritative.
if [ -e "$STATE/SAFE_MODE" ] || [ -e "$STATE/DISABLE" ] || [ -e "$MODDIR/disable" ] || [ -e "/data/adb/modules/coreflow_engine/disable" ]; then
  [ ! -L "$LOGS/service.log" ] && printf '%s CFAE disabled by safety marker\n' "$(date)" >> "$LOGS/service.log"
  exit 0
fi

chmod 755 "$BIN" 2>/dev/null
"$BIN" --status 2>/dev/null | grep -q '"running":true' && exit 0

LOG="$LOGS/coreflowd.log"
[ -L "$LOG" ] && rm -f "$LOG"
if [ -f "$LOG" ]; then
  SIZE=$(wc -c < "$LOG" 2>/dev/null)
  case "$SIZE" in
    ''|*[!0-9]*) ;;
    *)
      if [ "$SIZE" -gt 524288 ] && [ ! -L "$LOG.1" ]; then
        mv -f "$LOG" "$LOG.1"
      fi
      ;;
  esac
fi

"$BIN" --daemon >> "$LOG" 2>&1 </dev/null || \
  (setsid "$BIN" >> "$LOG" 2>&1 < /dev/null &) 2>/dev/null || \
  (nohup "$BIN" >> "$LOG" 2>&1 < /dev/null &) 2>/dev/null || \
  ("$BIN" </dev/null >> "$LOG" 2>&1 &)

CONFIRM_DELAY="${COREFLOW_CONFIRM_DELAY_SEC:-120}"
# A boot only counts as healthy after the daemon has survived its start window.
(
  sleep "$CONFIRM_DELAY"
  if "$BIN" --status 2>/dev/null | grep -q '"running":true'; then
    ATTEMPTS="$STATE/boot_attempts"
    TMP_ATTEMPTS="${ATTEMPTS}.tmp.$$"
    if [ ! -L "$ATTEMPTS" ]; then
      rm -f "$TMP_ATTEMPTS"
      printf '0\n' > "$TMP_ATTEMPTS" && chmod 600 "$TMP_ATTEMPTS" && mv -f "$TMP_ATTEMPTS" "$ATTEMPTS"
    fi
  fi
) </dev/null >/dev/null 2>&1 &
