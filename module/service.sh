#!/system/bin/sh
# CoreFlow Modern v2: single-instance daemon supervisor.
MODDIR="${0%/*}"
STATE="/data/adb/coreflow"
LOGS="$STATE/logs"
BIN="$MODDIR/system/bin/coreflowd"
[ -f "$BIN" ] || BIN="$MODDIR/coreflowd"
[ -f "$BIN" ] || exit 1

while [ "$(getprop sys.boot_completed)" != "1" ]; do
  sleep 2
done
mkdir -p "$LOGS" || exit 1
chmod 700 "$STATE" "$LOGS" 2>/dev/null
echo 0 > "$STATE/boot_attempts"

# Operator stop and boot-loop recovery are authoritative.
if [ -f "$STATE/SAFE_MODE" ] || [ -f "$STATE/DISABLE" ] || [ -f "$MODDIR/disable" ]; then
  echo "$(date) CoreFlow disabled by safety marker" >> "$LOGS/service.log"
  exit 0
fi
chmod 755 "$BIN"
"$BIN" --status 2>/dev/null | grep -q '"running":true' && exit 0

LOG="$LOGS/coreflowd.log"
if [ -f "$LOG" ]; then
  SIZE=$(wc -c < "$LOG" 2>/dev/null)
  case "$SIZE" in
    ''|*[!0-9]*) ;;
    *) [ "$SIZE" -gt 524288 ] && mv -f "$LOG" "$LOG.1" ;;
  esac
fi
nohup "$BIN" >> "$LOG" 2>&1 < /dev/null &
