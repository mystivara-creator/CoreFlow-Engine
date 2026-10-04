#!/system/bin/sh

MODDIR="${0%/*}"

STATE_DIR="/data/adb/coreflow"
LOG_DIR="$STATE_DIR/logs"

mkdir -p "$STATE_DIR" "$LOG_DIR"

chmod 0700 "$STATE_DIR"
chmod 0700 "$LOG_DIR"

# Runtime state is intentionally created here.
# No kernel/system tuning is performed at post-fs-data.
