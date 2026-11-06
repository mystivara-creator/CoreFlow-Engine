#!/bin/sh
set -eu

TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/coreflow-lifecycle.XXXXXX")"
trap 'rm -rf "$TMP_ROOT"' EXIT

STATE_DIR="$TMP_ROOT/state"
MOD_DIR="$TMP_ROOT/mod"
LIVE_MOD="$TMP_ROOT/live_mod"
mkdir -p "$MOD_DIR/system/bin" "$MOD_DIR/system/etc/coreflow" "$MOD_DIR/webroot"

# Mock Xiaomi property tools so the memory-tier policy can be tested on host.
MOCK_BIN="$TMP_ROOT/mock-bin"
mkdir -p "$MOCK_BIN"
cat <<'EOF' > "$MOCK_BIN/getprop"
#!/bin/sh
case "${1:-}" in
  ro.product.brand) echo "${COREFLOW_TEST_BRAND:-Xiaomi}" ;;
  ro.product.manufacturer) echo "${COREFLOW_TEST_MANUFACTURER:-Xiaomi}" ;;
  *)
    [ -f "$COREFLOW_PROP_STATE" ] || exit 0
    awk -F= -v key="${1:-}" '$1 == key { value=substr($0, index($0, "=")+1) } END { if (value != "") print value }' "$COREFLOW_PROP_STATE"
    ;;
esac
EOF
cat <<'EOF' > "$MOCK_BIN/resetprop"
#!/bin/sh
printf '%s\n' "$*" >> "$COREFLOW_PROP_LOG"
[ "${1:-}" = "-n" ] || exit 2
PROP="${2:-}"
VALUE="${3:-}"
[ -n "$PROP" ] || exit 2
[ "${COREFLOW_RESETPROP_FAIL:-}" = "$PROP" ] && exit 7
TMP="${COREFLOW_PROP_STATE}.tmp.$"
awk -F= -v key="$PROP" '$1 != key { print }' "$COREFLOW_PROP_STATE" 2>/dev/null > "$TMP" || :
printf '%s=%s\n' "$PROP" "$VALUE" >> "$TMP"
mv -f "$TMP" "$COREFLOW_PROP_STATE"
EOF
chmod +x "$MOCK_BIN/getprop" "$MOCK_BIN/resetprop"
export PATH="$MOCK_BIN:$PATH"
export COREFLOW_PROP_LOG="$TMP_ROOT/resetprop.log"
export COREFLOW_MEMINFO_PATH="$TMP_ROOT/meminfo"

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
grep -Fq -- '-n ro.config.low_ram false' "$COREFLOW_PROP_LOG" || { echo "FAIL: Xiaomi low_ram policy not applied"; exit 1; }
if grep -Fq -- 'ro.config.low_ram.middle.threshold_gb' "$COREFLOW_PROP_LOG"; then
  echo "FAIL: unreadable RAM must preserve the OEM threshold"
  exit 1
fi

# Verify dynamic RAM tiers: 6 GiB -> 5, 8 GiB -> 7.
printf 'MemTotal:       6291456 kB\n' > "$COREFLOW_MEMINFO_PATH"
: > "$COREFLOW_PROP_LOG"
sh "$MOD_DIR/post-fs-data.sh"
grep -Fq -- '-n ro.config.low_ram.middle.threshold_gb 5' "$COREFLOW_PROP_LOG" || { echo "FAIL: 6 GiB threshold should be 5"; exit 1; }
printf 'MemTotal:       7864320 kB\n' > "$COREFLOW_MEMINFO_PATH"
: > "$COREFLOW_PROP_LOG"
sh "$MOD_DIR/post-fs-data.sh"
grep -Fq -- '-n ro.config.low_ram.middle.threshold_gb 7' "$COREFLOW_PROP_LOG" || { echo "FAIL: 8 GiB threshold should be 7"; exit 1; }

# Verify that the two Xiaomi properties can be isolated independently.
printf 'MemTotal:       6291456 kB\n' > "$COREFLOW_MEMINFO_PATH"

printf 'low_ram_only\n' > "$STATE_DIR/xiaomi_ui_mode"
: > "$COREFLOW_PROP_LOG"
sh "$MOD_DIR/post-fs-data.sh"
grep -Fq -- '-n ro.config.low_ram false' "$COREFLOW_PROP_LOG" || { echo "FAIL: low_ram_only did not set low_ram"; exit 1; }
if grep -Fq -- 'ro.config.low_ram.middle.threshold_gb' "$COREFLOW_PROP_LOG"; then
  echo "FAIL: low_ram_only unexpectedly changed the threshold"
  exit 1
fi

printf 'threshold_only\n' > "$STATE_DIR/xiaomi_ui_mode"
: > "$COREFLOW_PROP_LOG"
sh "$MOD_DIR/post-fs-data.sh"
grep -Fq -- '-n ro.config.low_ram.middle.threshold_gb 5' "$COREFLOW_PROP_LOG" || { echo "FAIL: threshold_only did not set dynamic threshold"; exit 1; }
if grep -Fq -- 'ro.config.low_ram false' "$COREFLOW_PROP_LOG"; then
  echo "FAIL: threshold_only unexpectedly changed low_ram"
  exit 1
fi

printf 'both\n' > "$STATE_DIR/xiaomi_ui_mode"
: > "$COREFLOW_PROP_LOG"
sh "$MOD_DIR/post-fs-data.sh"
grep -Fq -- '-n ro.config.low_ram false' "$COREFLOW_PROP_LOG" || { echo "FAIL: both mode did not set low_ram"; exit 1; }
grep -Fq -- '-n ro.config.low_ram.middle.threshold_gb 5' "$COREFLOW_PROP_LOG" || { echo "FAIL: both mode did not set threshold"; exit 1; }

printf 'disabled\n' > "$STATE_DIR/xiaomi_ui_mode"
: > "$COREFLOW_PROP_LOG"
sh "$MOD_DIR/post-fs-data.sh"
[ ! -s "$COREFLOW_PROP_LOG" ] || { echo "FAIL: disabled mode changed Xiaomi properties"; exit 1; }

# Invalid modes must fail closed instead of silently applying a partial policy.
printf 'unknown_mode\n' > "$STATE_DIR/xiaomi_ui_mode"
: > "$COREFLOW_PROP_LOG"
sh "$MOD_DIR/post-fs-data.sh"
[ ! -s "$COREFLOW_PROP_LOG" ] || { echo "FAIL: invalid Xiaomi mode must not mutate properties"; exit 1; }
grep -q 'Invalid Xiaomi UI mode' "$STATE_DIR/logs/xiaomi_ui_policy.log" || { echo "FAIL: invalid Xiaomi mode was not reported"; exit 1; }

# A resetprop failure must be visible and must not be reported as applied.
printf 'both\n' > "$STATE_DIR/xiaomi_ui_mode"
: > "$COREFLOW_PROP_LOG"
COREFLOW_RESETPROP_FAIL="ro.config.low_ram" sh "$MOD_DIR/post-fs-data.sh"
grep -q 'resetprop failed: ro.config.low_ram=false' "$STATE_DIR/logs/xiaomi_ui_policy.log" || { echo "FAIL: resetprop failure was not reported"; exit 1; }
if grep -q 'applied and verified: ro.config.low_ram=false' "$STATE_DIR/logs/xiaomi_ui_policy.log"; then
  echo "FAIL: failed resetprop was incorrectly reported as applied"
  exit 1
fi
unset COREFLOW_RESETPROP_FAIL

# Non-Xiaomi devices must never receive either Xiaomi-specific property.
printf 'both\n' > "$STATE_DIR/xiaomi_ui_mode"
: > "$COREFLOW_PROP_LOG"
COREFLOW_TEST_BRAND="Google" COREFLOW_TEST_MANUFACTURER="Google" sh "$MOD_DIR/post-fs-data.sh"
[ ! -s "$COREFLOW_PROP_LOG" ] || { echo "FAIL: Xiaomi properties leaked to a non-Xiaomi device"; exit 1; }

# If physical RAM cannot be detected, do not write an unverified threshold.
printf 'threshold_only\n' > "$STATE_DIR/xiaomi_ui_mode"
: > "$COREFLOW_MEMINFO_PATH"
: > "$COREFLOW_PROP_LOG"
sh "$MOD_DIR/post-fs-data.sh"
[ ! -s "$COREFLOW_PROP_LOG" ] || { echo "FAIL: missing MemTotal should not mutate properties"; exit 1; }

# 2. Simulate 5 unconfirmed boots -> SAFE_MODE must trigger. Reset the
# counter because the preceding policy cases intentionally ran extra boots.
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
