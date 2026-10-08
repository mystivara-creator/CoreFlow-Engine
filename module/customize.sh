#!/system/bin/sh
# ==========================================================
# CoreFlow Autonomous - installer
# Runs inside the Magisk/KernelSU installer. Every status line printed below
# reflects a real check; the installer aborts rather than installing a package
# that fails validation.
# ==========================================================

MODULE_VERSION="$(awk -F= '$1=="version" {print $2; exit}' "$MODPATH/module.prop" 2>/dev/null | tr -d '[:space:]')"
MODULE_VERSION="${MODULE_VERSION:-unknown}"

# getprop always succeeds, so fall back on empty values instead of `||`.
first_prop() {
    for key in "$@"; do
        value="$(getprop "$key" 2>/dev/null)"
        if [ -n "$value" ]; then
            echo "$value"
            return 0
        fi
    done
    echo "unknown"
}

# ELF check: magic 7f 45 4c 46 and e_machine == EM_AARCH64 (0xb7), little-endian.
is_arm64_elf() {
    file="$1"
    [ -s "$file" ] || return 1
    magic="$(head -c 4 "$file" 2>/dev/null | od -An -tx1 | tr -d ' \n')"
    [ "$magic" = "7f454c46" ] || return 1
    machine="$(dd if="$file" bs=1 skip=18 count=2 2>/dev/null | od -An -tx1 | tr -d ' \n')"
    [ "$machine" = "b700" ]
}

DEVICE_MODEL="$(first_prop ro.product.vendor.model ro.product.model)"
DEVICE_CODE="$(first_prop ro.product.vendor.device ro.product.device)"
CHIPSET="$(first_prop ro.board.platform)"
ANDROID_VER="$(first_prop ro.build.version.release)"
API_LEVEL="$(first_prop ro.build.version.sdk)"
KERNEL_VER="$(uname -r 2>/dev/null || echo unknown)"

ui_print " "
ui_print " CoreFlow Autonomous ${MODULE_VERSION}"
ui_print " Developed by Mystivara"
ui_print " "
ui_print " [>] Target environment"
ui_print "     Model   : $DEVICE_MODEL"
ui_print "     Code    : $DEVICE_CODE"
ui_print "     Chipset : $CHIPSET"
ui_print "     OS      : Android $ANDROID_VER (API $API_LEVEL)"
ui_print "     Kernel  : $KERNEL_VER"
ui_print " "

ui_print " [>] Compatibility gate"
case "${ARCH:-}" in
    arm64) ui_print "     > Architecture : arm64 OK" ;;
    *) abort "CoreFlow requires arm64. Detected architecture: ${ARCH:-unknown}." ;;
esac
case "$API_LEVEL" in
    ''|*[!0-9]*) abort "Could not determine the Android API level." ;;
esac
if [ "$API_LEVEL" -lt 34 ]; then
    abort "CoreFlow requires Android 14 (API 34) or newer. Detected API $API_LEVEL."
fi
ui_print "     > Android API  : $API_LEVEL OK"
ui_print " "

ui_print " [>] Verifying package payload"
DAEMON="$MODPATH/system/bin/coreflowd"
ONNX_LIB="$MODPATH/system/lib64/libonnxruntime.so"
MODEL="$MODPATH/system/etc/coreflow/thermal_predictor.onnx"
CONF="$MODPATH/system/etc/coreflow/default.conf"

is_arm64_elf "$DAEMON" || abort "coreflowd is missing or is not an ARM64 ELF executable."
ui_print "     > coreflowd          : ARM64 ELF OK"
is_arm64_elf "$ONNX_LIB" || abort "libonnxruntime.so is missing or is not an ARM64 ELF library."
ui_print "     > ONNX Runtime       : ARM64 ELF OK"
[ -s "$MODEL" ] || abort "thermal_predictor.onnx is missing or empty."
ui_print "     > Thermal model      : present"
[ -s "$CONF" ] || abort "default.conf is missing or empty."
ui_print "     > Default config     : present"
ui_print " "

ui_print " [>] Setting permissions"
set_perm_recursive "$MODPATH/system" 0 0 0755 0644
set_perm "$DAEMON" 0 0 0755
for script in customize.sh post-fs-data.sh service.sh uninstall.sh; do
    [ -f "$MODPATH/$script" ] && set_perm "$MODPATH/$script" 0 0 0755
done
set_perm "$MODPATH/sepolicy.rule" 0 0 0644
set_perm "$MODPATH/module.prop" 0 0 0644
ui_print "     > Permissions applied"
ui_print " "
ui_print " [=== INSTALLATION COMPLETE: reboot to start CoreFlow ===]"
ui_print " "
