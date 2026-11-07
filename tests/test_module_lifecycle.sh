#!/bin/sh
set -eu

TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/coreflow-lifecycle.XXXXXX")"
trap 'rm -rf "$TMP_ROOT"' EXIT

STATE_DIR="$TMP_ROOT/state"
MOD_DIR="$TMP_ROOT/mod"
LIVE_MOD="$TMP_ROOT/live_mod"
mkdir -p "$MOD_DIR/system/bin" "$MOD_DIR/system/etc/coreflow" "$MOD_DIR/webroot"

# Property mutation is intentionally absent from the boot lifecycle.
MOCK_BIN="$TMP_ROOT/mock-bin"
mkdir -p "$MOCK_BIN"
cat <<'EOF' > "$MOCK_BIN/resetprop"
#!/bin/sh
printf '%s\n' "$*" >> "$COREFLOW_PROP_LOG"
EOF
chmod +x "$MOCK_BIN/resetprop"
export PATH="$MOCK_BIN:$PATH"
export COREFLOW_PROP_LOG="$TMP_ROOT/resetprop.log"
: > "$COREFLOW_PROP_LOG"

cp "$(dirname "$0")/../module/post-fs-data.sh" "$MOD_DIR/post-fs-data.sh"
cp "$(dirname "$0")/../module/service.sh" "$MOD_DIR/service.sh"
cp "$(dirname "$0")/../module/customize.sh" "$MOD_DIR/customize.sh"
cp "$(dirname "$0")/../module/uninstall.sh" "$MOD_DIR/uninstall.sh"
cp "$(dirname "$0")/../module/system/etc/coreflow/default.conf" "$MOD_DIR/system/etc/coreflow/default.conf"
chmod +x "$MOD_DIR/post-fs-data.sh" "$MOD_DIR/service.sh" "$MOD_DIR/customize.sh" "$MOD_DIR/uninstall.sh"

# Create a mock coreflowd binary that supports --daemon, --status, and --stop
cat <<'EOF' > "$MOD_DIR/system/bin/coreflowd"
#!/bin/sh
PID_FILE="${COREFLOW_STATE_DIR}/coreflowd.mock.pid"
case "${1:-}" in
  --daemon)
    echo "$$" > "$PID_FILE"
    exit 0
    ;;
  --stop)
    rm -f "$PID_FILE"
    exit 0
    ;;
  --status)
    if [ -f "$PID_FILE" ]; then
      echo '{"running":true,"pid":1234}'
    else
      echo '{"running":false,"pid":0}'
    fi
    exit 0
    ;;
esac
exit 0
EOF
chmod +x "$MOD_DIR/system/bin/coreflowd"

export COREFLOW_STATE_DIR="$STATE_DIR"
export COREFLOW_LIVE_MOD_DIR="$LIVE_MOD"

# 1. Test post-fs-data.sh boot_epoch generation & boot_attempts escalation
sh "$MOD_DIR/post-fs-data.sh"
[ -f "$STATE_DIR/boot_epoch" ] || { echo "FAIL: boot_epoch not created"; exit 1; }
EPOCH_1="$(cat "$STATE_DIR/boot_epoch")"
[ -n "$EPOCH_1" ] || { echo "FAIL: boot_epoch is empty"; exit 1; }
[ "$(cat "$STATE_DIR/boot_attempts")" = "1" ] || { echo "FAIL: boot_attempts != 1"; exit 1; }
[ -f "$STATE_DIR/coreflow.conf" ] || { echo "FAIL: default.conf not copied"; exit 1; }
[ ! -s "$COREFLOW_PROP_LOG" ] || { echo "FAIL: boot lifecycle must not mutate vendor properties"; exit 1; }

# 2. Simulate 5 unconfirmed boots -> SAFE_MODE must trigger.
printf '1\n' > "$STATE_DIR/boot_attempts"
for i in 2 3 4 5; do
  sh "$MOD_DIR/post-fs-data.sh"
done
[ "$(cat "$STATE_DIR/boot_attempts")" = "5" ] || { echo "FAIL: boot_attempts != 5"; exit 1; }
[ -f "$STATE_DIR/SAFE_MODE" ] || { echo "FAIL: SAFE_MODE not triggered at 5 boots"; exit 1; }

# 3. Test customize.sh hot-upgrade preserves SAFE_MODE and DISABLE markers
mkdir -p "$LIVE_MOD/system/bin"
cp "$MOD_DIR/system/bin/coreflowd" "$LIVE_MOD/system/bin/coreflowd"
touch "$STATE_DIR/DISABLE"

run_customize() (
  ARCH="arm64"
  API="35"
  MODPATH="$MOD_DIR"
  ui_print() { :; }
  abort() { echo "ABORT: $*" >&2; exit 1; }
  set_perm() { chmod "$4" "$1"; }
  set_perm_recursive() { :; }
  . "$MOD_DIR/customize.sh"
)
run_customize

[ -f "$STATE_DIR/SAFE_MODE" ] || { echo "FAIL: customize.sh deleted SAFE_MODE"; exit 1; }
[ -f "$STATE_DIR/DISABLE" ] || { echo "FAIL: customize.sh deleted DISABLE"; exit 1; }
[ ! -f "$STATE_DIR/coreflowd.mock.pid" ] || { echo "FAIL: daemon started despite SAFE_MODE/DISABLE"; exit 1; }

# 4. Clear safety markers and verify customize.sh hot-reloads and verifies daemon status
rm -f "$STATE_DIR/SAFE_MODE" "$STATE_DIR/DISABLE"
run_customize
[ -f "$STATE_DIR/coreflowd.mock.pid" ] || { echo "FAIL: daemon did not start after clearing markers"; exit 1; }

echo "Module lifecycle scripts PASS"
