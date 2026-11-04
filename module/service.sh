#!/system/bin/sh
# CoreFlow Modern v2: single-instance daemon supervisor (overlay & system-ready).
export PATH="/system/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin"
umask 077

MODDIR="${0%/*}"
STATE="/data/adb/coreflow"
LOGS="$STATE/logs"
BIN="$MODDIR/system/bin/coreflowd"
[ -f "$BIN" ] || BIN="$MODDIR/coreflowd"
[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"
[ -f "$BIN" ] || BIN="/system/bin/coreflowd"
[ -f "$BIN" ] || exit 1

WAIT_COUNT=0
while [ "$(getprop sys.boot_completed)" != "1" ]; do
  WAIT_COUNT=$((WAIT_COUNT + 1))
  [ "$WAIT_COUNT" -ge 180 ] && exit 1
  sleep 2
done

[ -L "$STATE" ] && exit 1
mkdir -p "$STATE" || exit 1
[ -L "$LOGS" ] && exit 1
mkdir -p "$LOGS" || exit 1
chmod 700 "$STATE" "$LOGS" 2>/dev/null

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

(nohup "$BIN" >> "$LOG" 2>&1 < /dev/null &) 2>/dev/null || ("$BIN" </dev/null >> "$LOG" 2>&1 &)

# A boot only counts as healthy after the daemon has survived its start window.
(
  sleep 120
  if "$BIN" --status 2>/dev/null | grep -q '"running":true'; then
    ATTEMPTS="$STATE/boot_attempts"
    TMP_ATTEMPTS="${ATTEMPTS}.tmp.$$"
    if [ ! -L "$ATTEMPTS" ]; then
      rm -f "$TMP_ATTEMPTS"
      printf '0\n' > "$TMP_ATTEMPTS" && chmod 600 "$TMP_ATTEMPTS" && mv -f "$TMP_ATTEMPTS" "$ATTEMPTS"
    fi
  fi
) </dev/null >/dev/null 2>&1 &
