#!/system/bin/sh
# KernelSU action menu entry: read-only inspection or an explicit OEM restore.
export PATH="/system/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin"
umask 077
MODDIR="${0%/*}"
BIN="$MODDIR/system/bin/coreflowd"
[ -f "$BIN" ] || BIN="$MODDIR/coreflowd"
[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"
[ -f "$BIN" ] || BIN="/system/bin/coreflowd"
[ -x "$BIN" ] || { echo "CoreFlow daemon binary is unavailable."; exit 1; }

case "\${1:-status}" in
  status) "$BIN" --status ;;
  journal) "$BIN" --journal ;;
  rollback)
    "$BIN" --rollback-baseline
    RESULT=$?
    if [ "$RESULT" -eq 0 ]; then
      rm -f /data/adb/coreflow/direct-adaptive.wal
    else
      echo "OEM rollback incomplete; preserve /data/adb/coreflow/baseline.journal" >&2
    fi
    exit "$RESULT"
    ;;
  *)
    echo "Usage: action.sh [status|journal|rollback]" >&2
    exit 2
    ;;
esac
