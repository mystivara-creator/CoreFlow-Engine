#!/system/bin/sh
# KernelSU action menu entry: read-only inspection or an explicit OEM restore.
export PATH="/system/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin:${PATH:-/usr/bin:/bin}"
umask 077
STATE="${COREFLOW_STATE_DIR:-/data/adb/coreflow}"
[ ! -L "$STATE" ] || { echo "CoreFlow state path is unsafe." >&2; exit 1; }
MODDIR="${0%/*}"
BIN="$MODDIR/system/bin/coreflowd"
[ -f "$BIN" ] || BIN="$MODDIR/coreflowd"
[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"
[ -f "$BIN" ] || BIN="/system/bin/coreflowd"
[ -x "$BIN" ] || { echo "CoreFlow daemon binary is unavailable."; exit 1; }

case "${1:-status}" in
  status) "$BIN" --status ;;
  atlas) "$BIN" --atlas ;;
  episodes) "$BIN" --episodes ;;
  journal) "$BIN" --journal ;;
  diag) "$BIN" --diag ;;
  recover)
    "$BIN" --recover-only
    exit $?
    ;;
  clear-safety)
    "$BIN" --recover-only >/dev/null 2>&1 || true
    rm -f "$STATE/SAFE_MODE" "$STATE/DISABLE" "$MODDIR/disable" \
      "/data/adb/modules/cfae_framework/disable" "/data/adb/modules/coreflow_engine/disable"
    if [ ! -L "$STATE/boot_attempts" ]; then
      TMP_ATTEMPTS="$STATE/boot_attempts.tmp.$$"
      printf '0\n' > "$TMP_ATTEMPTS" && chmod 600 "$TMP_ATTEMPTS" && mv -f "$TMP_ATTEMPTS" "$STATE/boot_attempts"
    fi
    echo "CFAE safety markers cleared and boot_attempts reset to 0."
    exit 0
    ;;
  rollback)
    "$BIN" --rollback-baseline
    RESULT=$?
    if [ "$RESULT" -eq 0 ]; then
      rm -f "$STATE/direct-adaptive.wal"
    else
      echo "OEM rollback incomplete; preserve /data/adb/coreflow/baseline.journal" >&2
    fi
    exit "$RESULT"
    ;;
  *)
    echo "Usage: action.sh [status|atlas|episodes|journal|diag|recover|clear-safety|rollback]" >&2
    exit 2
    ;;
esac
