#!/system/bin/sh
# Stop coreflowd and restore its captured OEM values before module removal.
export PATH="/system/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin"
umask 077
MODDIR="\${0%/*}"
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
    exit "$RESULT"
  fi
fi

STATE="/data/adb/coreflow"
[ ! -L "$STATE" ] || exit 1
# Remove only volatile state after verified rollback. Retain user config and logs.
rm -f "$STATE/direct-adaptive.wal" "$STATE/coreflowd.pid" "$STATE/status.json" \
  "$STATE/status.json.tmp" "$STATE"/status.json.tmp.*
rm -f "$STATE"/coreflowd.pid.tmp.* "$STATE"/direct-adaptive.wal.tmp.*
exit 0
