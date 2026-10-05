#!/system/bin/sh

MODDIR="${0%/*}"
STATE_DIR="/data/adb/coreflow"
LOG_DIR="$STATE_DIR/logs"
BINARY="$MODDIR/system/bin/coreflowd"
PID_FILE="$STATE_DIR/coreflowd.pid"
LOG_FILE="$LOG_DIR/coreflowd.log"

umask 077
mkdir -p "$STATE_DIR" "$LOG_DIR"
chmod 0700 "$STATE_DIR" "$LOG_DIR"

BOOT_TIMEOUT=180
WAITED=0
while [ "$(getprop sys.boot_completed 2>/dev/null)" != "1" ]; do
    sleep 2
    WAITED=$((WAITED + 2))
    [ "$WAITED" -ge "$BOOT_TIMEOUT" ] && exit 0
done

sleep 5

[ -x "$BINARY" ] || { echo "$(date '+%F %T') coreflowd missing" >> "$LOG_FILE"; exit 1; }

if [ -f "$PID_FILE" ]; then
    OLD_PID="$(cat "$PID_FILE" 2>/dev/null)"
    if [ -n "$OLD_PID" ] && kill -0 "$OLD_PID" 2>/dev/null; then
        exit 0
    fi
    rm -f "$PID_FILE"
fi

VERSION="$(awk -F= '$1=="version" {print $2; exit}' "$MODDIR/module.prop" 2>/dev/null)"
VERSION="${VERSION:-unknown}"
echo "$(date '+%F %T') starting CoreFlow Autonomous ${VERSION}" >> "$LOG_FILE"
"$BINARY" >> "$LOG_FILE" 2>&1 &
PID=$!

if [ -n "$PID" ]; then
    echo "$PID" > "$PID_FILE"
    chmod 0600 "$PID_FILE"
fi

sleep 1
if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
    echo "$(date '+%F %T') coreflowd started pid=$PID" >> "$LOG_FILE"
else
    echo "$(date '+%F %T') coreflowd failed to start" >> "$LOG_FILE"
    rm -f "$PID_FILE"
fi
