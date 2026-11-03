#!/system/bin/sh
# Early boot guard; no native daemon is launched until Android boot completion.
MODDIR="${0%/*}"
STATE="/data/adb/coreflow"
LOGS="$STATE/logs"
mkdir -p "$LOGS"
chmod 700 "$STATE" "$LOGS"
umask 077
ATTEMPTS="$STATE/boot_attempts"
N="$(cat "$ATTEMPTS" 2>/dev/null)"
case "$N" in ''|*[!0-9]*) N=0 ;; esac
N=$((N+1))
echo "$N" > "$ATTEMPTS"
if [ "$N" -ge 3 ]; then
  touch "$STATE/SAFE_MODE"
  echo "$(date) Safe mode after $N unconfirmed boots" >> "$LOGS/boot_guard.log"
fi
if [ "$N" -ge 5 ]; then
  touch "$MODDIR/disable"
  echo "$(date) Module disabled after $N unconfirmed boots" >> "$LOGS/boot_guard.log"
fi
# Persistent config is kept outside the module directory, surviving upgrades.
if [ ! -f "$STATE/coreflow.conf" ] && [ -f "$MODDIR/system/etc/coreflow/default.conf" ]; then
  cp "$MODDIR/system/etc/coreflow/default.conf" "$STATE/coreflow.conf"
  chmod 600 "$STATE/coreflow.conf"
fi
