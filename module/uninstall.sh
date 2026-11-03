#!/system/bin/sh
# Stop the daemon and ask it to verify OEM restore before removing the module.
MODDIR="${0%/*}"
BIN="$MODDIR/system/bin/coreflowd"
[ -f "$BIN" ] || BIN="$MODDIR/coreflowd"
if [ -f "$BIN" ]; then
  "$BIN" --rollback-baseline
  RESULT=$?
  if [ "$RESULT" -ne 0 ]; then
    echo "CoreFlow: OEM recovery incomplete. Preserve /data/adb/coreflow/baseline.journal" >&2
  fi
fi
# Keep user configuration and crash journal for diagnostics and reinstall.
