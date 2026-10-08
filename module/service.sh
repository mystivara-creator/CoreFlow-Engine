#!/system/bin/sh

MODDIR="${0%/*}"
STATE_DIR="/data/adb/coreflow"
LOG_DIR="$STATE_DIR/logs"
BINARY="$MODDIR/system/bin/coreflowd"
LIB_DIR="$MODDIR/system/lib64"
MODEL="$MODDIR/system/etc/coreflow/thermal_predictor.onnx"
PID_FILE="$STATE_DIR/coreflowd.pid"
LOG_FILE="$LOG_DIR/coreflowd.log"

# Exit codes from coreflowd that must NOT trigger a restart:
#   0 clean stop, 2 init failure, 3 another instance holds the lock, 4 lock error.
MAX_RESTARTS=5
BOOT_TIMEOUT=180

umask 077
mkdir -p "$STATE_DIR" "$LOG_DIR"
chmod 0700 "$STATE_DIR" "$LOG_DIR"

log() {
    echo "$(date '+%F %T') $*" >> "$LOG_FILE"
}

WAITED=0
while [ "$(getprop sys.boot_completed 2>/dev/null)" != "1" ]; do
    sleep 2
    WAITED=$((WAITED + 2))
    [ "$WAITED" -ge "$BOOT_TIMEOUT" ] && exit 0
done
sleep 5

[ -x "$BINARY" ] || { log "coreflowd missing or not executable"; exit 1; }
[ -s "$LIB_DIR/libonnxruntime.so" ] || { log "libonnxruntime.so missing"; exit 1; }
[ -s "$MODEL" ] || { log "thermal_predictor.onnx missing"; exit 1; }

# Module-local ONNX runtime must win over any system copy.
export LD_LIBRARY_PATH="$LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

if [ -f "$PID_FILE" ]; then
    OLD_PID="$(cat "$PID_FILE" 2>/dev/null)"
    if [ -n "$OLD_PID" ] && kill -0 "$OLD_PID" 2>/dev/null; then
        exit 0
    fi
    rm -f "$PID_FILE"
fi

VERSION="$(awk -F= '$1=="version" {print $2; exit}' "$MODDIR/module.prop" 2>/dev/null)"
VERSION="${VERSION:-unknown}"

# Supervisor: restarts coreflowd after a crash with bounded backoff. The daemon
# recovers its journal on start, so a restart always begins from a safe state.
supervise() {
    CHILD=""
    STOPPING=0
    trap 'STOPPING=1; [ -n "$CHILD" ] && kill -TERM "$CHILD" 2>/dev/null' TERM INT HUP

    restarts=0
    while [ "$STOPPING" -eq 0 ]; do
        log "starting coreflowd attempt=$((restarts + 1)) version=$VERSION"
        "$BINARY" >> "$LOG_FILE" 2>&1 &
        CHILD=$!

        rc=0
        while :; do
            wait "$CHILD"
            rc=$?
            kill -0 "$CHILD" 2>/dev/null || break
        done
        CHILD=""

        if [ "$STOPPING" -eq 1 ]; then
            log "coreflowd stopped rc=$rc"
            break
        fi

        case "$rc" in
            0|2|3|4)
                log "coreflowd exited rc=$rc; not restarting"
                break
                ;;
        esac

        restarts=$((restarts + 1))
        if [ "$restarts" -gt "$MAX_RESTARTS" ]; then
            log "coreflowd restart limit reached; giving up"
            break
        fi

        delay=$((restarts * 5))
        log "coreflowd crashed rc=$rc; restarting in ${delay}s"
        sleep "$delay" &
        wait $!
    done

    rm -f "$PID_FILE"
}

supervise &
SUPERVISOR_PID=$!
echo "$SUPERVISOR_PID" > "$PID_FILE"
chmod 0600 "$PID_FILE"
log "CoreFlow supervisor started pid=$SUPERVISOR_PID version=$VERSION"
