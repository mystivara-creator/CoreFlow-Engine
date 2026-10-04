#!/system/bin/sh

MODDIR="${0%/*}"

STATE_DIR="/data/adb/coreflow"
LOG_DIR="$STATE_DIR/logs"

BINARY="$MODDIR/system/bin/coreflowd"
PID_FILE="$STATE_DIR/coreflowd.pid"
LOG_FILE="$LOG_DIR/coreflowd.log"

mkdir -p "$STATE_DIR" "$LOG_DIR"

chmod 0700 "$STATE_DIR"
chmod 0700 "$LOG_DIR"

# ------------------------------------------
# Wait for Android boot completion
# ------------------------------------------

BOOT_TIMEOUT=180
WAITED=0

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2

    WAITED=$((WAITED + 2))

    if [ "$WAITED" -ge "$BOOT_TIMEOUT" ]; then
        exit 0
    fi
done

# Give framework/vendor services a little time
# to settle before the observer starts.

sleep 5

# ------------------------------------------
# Validate daemon
# ------------------------------------------

if [ ! -x "$BINARY" ]; then
    echo "$(date '+%Y-%m-%d %H:%M:%S') coreflowd missing or not executable" \
        >> "$LOG_FILE"
    exit 1
fi

# ------------------------------------------
# Duplicate process protection
# ------------------------------------------

if [ -f "$PID_FILE" ]; then
    OLD_PID="$(cat "$PID_FILE" 2>/dev/null)"

    if [ -n "$OLD_PID" ] && kill -0 "$OLD_PID" 2>/dev/null; then
        exit 0
    fi

    rm -f "$PID_FILE"
fi

# ------------------------------------------
# Start native runtime
# ------------------------------------------

echo "$(date '+%Y-%m-%d %H:%M:%S') starting CoreFlow Autonomous" \
    >> "$LOG_FILE"

"$BINARY" >> "$LOG_FILE" 2>&1 &

PID=$!

if [ -n "$PID" ]; then
    echo "$PID" > "$PID_FILE"
    chmod 0600 "$PID_FILE"
fi

# ------------------------------------------
# Verify startup
# ------------------------------------------

sleep 1

if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
    echo "$(date '+%Y-%m-%d %H:%M:%S') coreflowd started pid=$PID" \
        >> "$LOG_FILE"
else
    echo "$(date '+%Y-%m-%d %H:%M:%S') coreflowd failed to start" \
        >> "$LOG_FILE"

    rm -f "$PID_FILE"
fi

exit 0
