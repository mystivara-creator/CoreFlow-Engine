#!/system/bin/sh

STATE_DIR="/data/adb/coreflow"
PID_FILE="$STATE_DIR/coreflowd.pid"

if [ -f "$PID_FILE" ]; then
    PID="$(cat "$PID_FILE" 2>/dev/null)"
    [ -n "$PID" ] && kill "$PID" 2>/dev/null
    rm -f "$PID_FILE"
fi

# Runtime state is retained intentionally for diagnostics.
