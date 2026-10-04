#!/system/bin/sh

STATE_DIR="/data/adb/coreflow"
PID_FILE="$STATE_DIR/coreflowd.pid"

if [ -f "$PID_FILE" ]; then
    PID="$(cat "$PID_FILE" 2>/dev/null)"

    if [ -n "$PID" ]; then
        kill "$PID" 2>/dev/null
    fi

    rm -f "$PID_FILE"
fi

# Runtime data is intentionally preserved.
# This allows diagnostics to survive module removal.
#
# Remove manually if desired:
# /data/adb/coreflow
