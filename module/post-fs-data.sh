#!/system/bin/sh
# Early boot hook. Keep it short and non-blocking: it runs before the framework.

MODDIR="${COREFLOW_MODDIR:-${0%/*}}"
STATE_DIR="${COREFLOW_STATE_DIR:-/data/adb/coreflow}"
LOG_DIR="$STATE_DIR/logs"
LOG_FILE="$LOG_DIR/boot_guard.log"
CONFIG="$STATE_DIR/config.ini"
DEFAULT_CONFIG="$MODDIR/system/etc/coreflow/default.conf"
BOOT_COUNTER="$STATE_DIR/boot_attempts"
SAFE_MODE_MARKER="$STATE_DIR/SAFE_MODE"

# Boot-loop guard tiers (consecutive boots that never reached boot_completed):
#   >= 3 : SAFE_MODE, daemon stays observe-only
#   >= 5 : Magisk "disable" marker, module is skipped on the next boot
BOOT_LOOP_SAFE_MODE=3
BOOT_LOOP_DISABLE=5

umask 077
mkdir -p "$STATE_DIR" "$LOG_DIR"
chmod 0700 "$STATE_DIR" "$LOG_DIR"

log() {
    echo "$(date '+%F %T') $*" >> "$LOG_FILE"
}

# Count this boot attempt before doing anything else that could fail.
attempts="$(cat "$BOOT_COUNTER" 2>/dev/null)"
case "$attempts" in
    ''|*[!0-9]*) attempts=0 ;;
esac
attempts=$((attempts + 1))
echo "$attempts" > "$BOOT_COUNTER"
log "boot attempt=$attempts (unconfirmed until boot_completed)"

if [ "$attempts" -ge "$BOOT_LOOP_DISABLE" ]; then
    touch "$MODDIR/disable" 2>/dev/null
    log "boot-loop guard: $attempts unconfirmed boots, module disable marker set"
fi
if [ "$attempts" -ge "$BOOT_LOOP_SAFE_MODE" ]; then
    touch "$SAFE_MODE_MARKER" 2>/dev/null
    log "boot-loop guard: SAFE_MODE set"
fi

if [ ! -f "$CONFIG" ] && [ -f "$DEFAULT_CONFIG" ]; then
    cp "$DEFAULT_CONFIG" "$CONFIG" 2>/dev/null
    chmod 0600 "$CONFIG" 2>/dev/null
fi

# The experimental "trial" mode was removed. A stale persisted "trial" must
# downgrade to observe-only, never silently enable adaptive mutation.
if [ -f "$CONFIG" ] && grep -q '^mutation_mode=trial[[:space:]]*$' "$CONFIG" 2>/dev/null; then
    sed -i 's/^mutation_mode=trial[[:space:]]*$/mutation_mode=observe/' "$CONFIG" 2>/dev/null
fi
