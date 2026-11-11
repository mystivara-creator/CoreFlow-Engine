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

# Serialize service invocations with atomic directory creation. The owner marker
# allows safe stale-lock recovery after a killed service shell.
SERVICE_LOCK="$STATE/service.lock"
SERVICE_LOCK_OWNER="$SERVICE_LOCK/owner.pid"
release_service_lock() {
  if [ -f "$SERVICE_LOCK_OWNER" ] && [ ! -L "$SERVICE_LOCK_OWNER" ] &&
     [ "$(cat "$SERVICE_LOCK_OWNER" 2>/dev/null)" = "$" ]; then
    rm -f "$SERVICE_LOCK_OWNER"
    rmdir "$SERVICE_LOCK" 2>/dev/null || true
  fi
}
acquire_service_lock() {
  if mkdir "$SERVICE_LOCK" 2>/dev/null; then
    printf '%s\n' "$" > "$SERVICE_LOCK_OWNER.tmp.$" &&
      chmod 600 "$SERVICE_LOCK_OWNER.tmp.$" &&
      mv -f "$SERVICE_LOCK_OWNER.tmp.$" "$SERVICE_LOCK_OWNER" || return 1
    return 0
  fi
  [ -L "$SERVICE_LOCK" ] && return 1
  WAIT_LOCK=0
  while [ "$WAIT_LOCK" -lt 10 ] && [ ! -f "$SERVICE_LOCK_OWNER" ]; do
    sleep 0.1
    WAIT_LOCK=$((WAIT_LOCK + 1))
  done
  [ -L "$SERVICE_LOCK_OWNER" ] && return 1
  LOCK_OWNER="$(cat "$SERVICE_LOCK_OWNER" 2>/dev/null)"
  case "$LOCK_OWNER" in
    ''|*[!0-9]*) ;;
    *)
      if kill -0 "$LOCK_OWNER" 2>/dev/null; then return 2; fi
      ;;
  esac
  rm -f "$SERVICE_LOCK_OWNER" 2>/dev/null
  rmdir "$SERVICE_LOCK" 2>/dev/null || return 2
  mkdir "$SERVICE_LOCK" 2>/dev/null || return 2
  printf '%s\n' "$" > "$SERVICE_LOCK_OWNER.tmp.$" &&
    chmod 600 "$SERVICE_LOCK_OWNER.tmp.$" &&
    mv -f "$SERVICE_LOCK_OWNER.tmp.$" "$SERVICE_LOCK_OWNER" || return 1
  return 0
}
acquire_service_lock
LOCK_RESULT=$?
if [ "$LOCK_RESULT" -eq 2 ]; then exit 0; fi
[ "$LOCK_RESULT" -eq 0 ] || exit 1
trap 'release_service_lock' 0
trap 'exit 1' HUP INT TERM

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

PID_FILE="$STATE/coreflowd.pid"
[ -L "$PID_FILE" ] && exit 1
if [ -s "$PID_FILE" ] && "$BIN" --status 2>/dev/null | grep -q '"running":true'; then
  exit 0
fi

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

# Exactly one daemon launch path: coreflowd owns its double-fork and PID-file lock.
if ! "$BIN" --daemon >> "$LOG" 2>&1 </dev/null; then
  if ! "$BIN" --status 2>/dev/null | grep -q '"running":true'; then
    printf '%s CoreFlow daemon launch failed\n' "$(date)" >> "$LOG"
    exit 1
  fi
fi

# The first --daemon parent can exit before the detached child publishes its PID.
START_ATTEMPT=0
DAEMON_READY=0
while [ "$START_ATTEMPT" -lt 20 ]; do
  if [ -s "$PID_FILE" ] && "$BIN" --status 2>/dev/null | grep -q '"running":true'; then
    DAEMON_READY=1
    break
  fi
  sleep 0.5
  START_ATTEMPT=$((START_ATTEMPT + 1))
done
if [ "$DAEMON_READY" -ne 1 ]; then
  printf '%s CoreFlow daemon did not publish a verified PID within 10 seconds\n' "$(date)" >> "$LOG"
  exit 1
fi

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
