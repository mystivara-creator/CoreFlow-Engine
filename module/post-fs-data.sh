#!/system/bin/sh

MODDIR="${0%/*}"
STATE_DIR="/data/adb/coreflow"
LOG_DIR="$STATE_DIR/logs"
CONFIG="$STATE_DIR/config.ini"
DEFAULT_CONFIG="$MODDIR/system/etc/coreflow/default.conf"

umask 077
mkdir -p "$STATE_DIR" "$LOG_DIR"
chmod 0700 "$STATE_DIR" "$LOG_DIR"

if [ ! -f "$CONFIG" ] && [ -f "$DEFAULT_CONFIG" ]; then
    cp "$DEFAULT_CONFIG" "$CONFIG" 2>/dev/null
    chmod 0600 "$CONFIG" 2>/dev/null
fi

# The experimental "trial" mode was removed. A stale persisted "trial" must
# downgrade to observe-only, never silently enable adaptive mutation.
if [ -f "$CONFIG" ] && grep -q '^mutation_mode=trial[[:space:]]*$' "$CONFIG" 2>/dev/null; then
    sed -i 's/^mutation_mode=trial[[:space:]]*$/mutation_mode=observe/' "$CONFIG" 2>/dev/null
fi
