#!/system/bin/sh
# CoreFlow Autonomous Engine v3.2.0 persistent service.
MODDIR=${0%/*}
TAG='CoreFlowEngine'
DAEMON="$MODDIR/system/bin/coreflow_daemon"
CATALOG="$MODDIR/strategies.tsv"
# Persistent state shared with the WebUI. Lives outside the module directory so it
# survives module updates. The WebUI writes only fixed command tokens here.
STATE_DIR='/data/adb/coreflow'
LOGFILE="$MODDIR/coreflow.log"
LOG_ROTATE_BYTES=1048576

log_msg() {
    _level="$1"; shift
    if command -v log >/dev/null 2>&1; then
        log -p "$_level" -t "$TAG" "$*"
    fi
}

# Wait until Android has completed boot. Do not touch kernel resources during boot.
while [ "$(getprop sys.boot_completed 2>/dev/null)" != '1' ]; do
    sleep 3
done
sleep 8

if [ ! -s "$DAEMON" ] || [ ! -x "$DAEMON" ]; then
    log_msg e "FATAL: daemon tidak ada, kosong, atau tidak executable: $DAEMON"
    exit 1
fi
if [ ! -s "$CATALOG" ]; then
    log_msg e "FATAL: katalog strategi tidak ditemukan/kosong: $CATALOG"
    exit 1
fi

mkdir -p "$STATE_DIR" 2>/dev/null
chmod 0700 "$STATE_DIR" 2>/dev/null

export COREFLOW_STRATEGY_CATALOG="$CATALOG"
export COREFLOW_MODULE_DIR="$MODDIR"
export COREFLOW_STATE_DIR="$STATE_DIR"

# Keep a single instance owned by this service. Avoid killing an unrelated process
# by checking the executable path when /proc is available.
if command -v pidof >/dev/null 2>&1; then
    for pid in $(pidof coreflow_daemon 2>/dev/null); do
        [ -r "/proc/$pid/cmdline" ] || continue
        cmdline="$(tr '\000' ' ' < "/proc/$pid/cmdline" 2>/dev/null)"
        case "$cmdline" in *"$DAEMON"*) log_msg w "Instance daemon sudah berjalan (pid=$pid); service keluar."; exit 0 ;; esac
    done
fi

log_msg i "Service aktif; menunggu keputusan runtime dari discovery perangkat."
failures=0
while :; do
    [ -x "$DAEMON" ] || { log_msg e 'Daemon hilang atau tidak executable; service berhenti.'; exit 1; }
    start_time="$(date +%s 2>/dev/null)"
    log_msg i "Menjalankan daemon v3.2.0; katalog=$CATALOG; state=$STATE_DIR"

    # Rotate the module log so a long-running device cannot fill /data with it.
    if [ -f "$LOGFILE" ]; then
        size="$(wc -c < "$LOGFILE" 2>/dev/null)"
        case "$size" in ''|*[!0-9]*) size=0 ;; esac
        [ "$size" -gt "$LOG_ROTATE_BYTES" ] && mv -f "$LOGFILE" "$LOGFILE.1" 2>/dev/null
    fi
    : >> "$LOGFILE" 2>/dev/null

    # The daemon writes its own diagnostics. On Android they go to logcat through
    # __android_log_print; the file captures stderr and early startup failures.
    "$DAEMON" >> "$LOGFILE" 2>&1 &
    DAEMON_PID=$!
    wait "$DAEMON_PID"
    exit_code=$?
    end_time="$(date +%s 2>/dev/null)"
    runtime=0
    case "$start_time:$end_time" in *[!0-9:]*|'':*) ;; *) runtime=$((end_time - start_time)) ;; esac

    if [ "$runtime" -ge 120 ]; then
        failures=0
    else
        failures=$((failures + 1))
    fi

    if [ "$failures" -le 1 ]; then delay=5
    elif [ "$failures" -eq 2 ]; then delay=10
    elif [ "$failures" -eq 3 ]; then delay=20
    elif [ "$failures" -eq 4 ]; then delay=30
    else delay=60
    fi
    log_msg w "Daemon berhenti (exit=$exit_code, runtime=${runtime}s); restart dalam ${delay}s, kegagalan beruntun=$failures."
    sleep "$delay"
done
