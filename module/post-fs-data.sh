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

# Production migration: Trial is an explicit experimental mode and must not
# survive into the Autonomous release line as a stale persisted setting.
if [ -f "$CONFIG" ] && grep -q '^mutation_mode=trial[[:space:]]*$' "$CONFIG" 2>/dev/null; then
    sed -i 's/^mutation_mode=trial[[:space:]]*$/mutation_mode=adaptive/' "$CONFIG" 2>/dev/null
fi
