#!/system/bin/sh
# Stop the daemon and verify OEM restore before removing the module.
export PATH="/system/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin"
umask 077

MODDIR="${0%/*}"
BIN="$MODDIR/system/bin/coreflowd"
[ -f "$BIN" ] || BIN="$MODDIR/coreflowd"
[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"
[ -f "$BIN" ] || BIN="/system/bin/coreflowd"

if [ -x "$BIN" ] || [ -f "$BIN" ]; then
  chmod 755 "$BIN" 2>/dev/null
  "$BIN" --rollback-baseline
  RESULT=$?
  if [ "$RESULT" -ne 0 ]; then
    echo "CoreFlow: OEM recovery incomplete. Preserve /data/adb/coreflow/baseline.journal" >&2
  fi
fi
# Keep user configuration and crash journal for diagnostics and reinstall.
