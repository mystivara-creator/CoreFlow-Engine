#!/system/bin/sh

MODPATH="${MODPATH:-${0%/*}}"

ui_print "*******************************"
ui_print "     CoreFlow Autonomous"
ui_print "     Native Stability Engine"
ui_print "*******************************"
ui_print ""

# ------------------------------------------
# Environment
# ------------------------------------------

API="$(getprop ro.build.version.sdk)"
ABI="$(getprop ro.product.cpu.abi)"

ui_print "Android API : ${API:-unknown}"
ui_print "Primary ABI  : ${ABI:-unknown}"

# CoreFlow Autonomous currently targets
# Android 14+ / ARM64.

if [ -n "$API" ] && [ "$API" -lt 34 ]; then
    ui_print "! Android 14+ is required."
    ui_print "! Installation will continue, but"
    ui_print "! this build is not supported here."
fi

case "$ABI" in
    arm64-v8a|arm64)
        ui_print "ARM64       : OK"
        ;;
    *)
        ui_print "! Unsupported primary ABI: $ABI"
        ;;
esac

# ------------------------------------------
# Required files
# ------------------------------------------

BINARY="$MODPATH/system/bin/coreflowd"

if [ ! -f "$BINARY" ]; then
    ui_print "! coreflowd binary not found."
    ui_print "! Module installation cannot continue."
    exit 1
fi

if [ ! -x "$BINARY" ]; then
    chmod 0755 "$BINARY"
fi

chmod 0755 "$MODPATH/service.sh" 2>/dev/null
chmod 0755 "$MODPATH/post-fs-data.sh" 2>/dev/null
chmod 0755 "$MODPATH/uninstall.sh" 2>/dev/null

mkdir -p "$MODPATH/system/etc/coreflow"

chmod 0644 "$MODPATH/module.prop" 2>/dev/null
chmod 0644 "$MODPATH/sepolicy.rule" 2>/dev/null

ui_print "coreflowd   : OK"
ui_print "Module      : Ready"
ui_print ""
ui_print "CoreFlow Autonomous installed."
