#!/system/bin/sh
MODDIR=${0%/*}

until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 5
done

sleep 10

daemon_bin="$MODDIR/system/bin/coreflow_daemon"
if [ ! -x "$daemon_bin" ]; then
    chmod 755 "$daemon_bin" 2>/dev/null
fi

if [ ! -x "$daemon_bin" ]; then
    log -p f -t CoreFlowEngine "FATAL: coreflow_daemon tidak ditemukan atau tidak executable!"
    exit 1
fi

while true; do
    log -p i -t CoreFlowEngine "Starting coreflow_daemon..."
    "$daemon_bin" 2>&1 | log -p d -t CoreFlowEngine &
    DAEMON_PID=$!
    wait "$DAEMON_PID"
    EXIT_CODE=$?
    log -p w -t CoreFlowEngine "Daemon exited with code $EXIT_CODE. Restarting in 5 seconds..."
    sleep 5
done