#!/system/bin/sh

MODPATH="${MODPATH:-${0%/*}}"
API="$(getprop ro.build.version.sdk 2>/dev/null)"
ABI="$(getprop ro.product.cpu.abi 2>/dev/null)"
BINARY="$MODPATH/system/bin/coreflowd"

ui_print "*******************************"
ui_print "      CoreFlow Autonomous"
VERSION="$(awk -F= '$1=="version" {print $2; exit}' "$MODPATH/module.prop" 2>/dev/null)"
VERSION="${VERSION:-unknown}"
ui_print "      Production ${VERSION}"
ui_print "*******************************"
ui_print "Android API : ${API:-unknown}"
ui_print "Primary ABI : ${ABI:-unknown}"

[ -n "$API" ] && [ "$API" -lt 34 ] && ui_print "! Target is Android 14+ (API 34+)."

case "$ABI" in
    arm64-v8a|arm64) ui_print "ARM64       : OK" ;;
    *) ui_print "! Unsupported primary ABI: $ABI" ;;
esac

if [ ! -f "$BINARY" ] || [ ! -x "$BINARY" ]; then
    ui_print "! coreflowd binary missing/non-executable."
    ui_print "! Install aborted."
    exit 1
fi

chmod 0755 "$MODPATH"/*.sh 2>/dev/null
chmod 0755 "$BINARY"
chmod 0644 "$MODPATH/module.prop" "$MODPATH/sepolicy.rule" 2>/dev/null
mkdir -p "$MODPATH/system/etc/coreflow"
chmod 0644 "$MODPATH/system/etc/coreflow/default.conf" 2>/dev/null

MODEL="$MODPATH/system/etc/coreflow/thermal_predictor.onnx"
if [ -f "$MODEL" ]; then
    chmod 0644 "$MODEL"
    ui_print "Thermal ML  : ONNX model present"
else
    ui_print "Thermal ML  : model missing (heuristic fallback)"
fi

ui_print "coreflowd   : OK"
ui_print "Module      : Ready"
ui_print ""
ui_print "CoreFlow Autonomous installed."
