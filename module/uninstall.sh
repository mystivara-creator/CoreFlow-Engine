#!/system/bin/sh

STATE_DIR="/data/adb/coreflow"
PID_FILE="$STATE_DIR/coreflowd.pid"

# Stop gracefully and wait for the daemon to finish its factory restore.
# Never SIGKILL here: an interrupted restore is recovered from the journal on
# the next start, but a clean restore is strictly better.
if [ -f "$PID_FILE" ]; then
    PID="$(cat "$PID_FILE" 2>/dev/null)"
    if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
        kill -TERM "$PID" 2>/dev/null
        i=0
        while kill -0 "$PID" 2>/dev/null && [ "$i" -lt 30 ]; do
            sleep 1
            i=$((i + 1))
        done
    fi
    rm -f "$PID_FILE"
fi

# Runtime state, logs and any unrecovered mutation journal are kept on purpose.
