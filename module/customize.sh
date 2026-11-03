#!/system/bin/sh
# CoreFlow Engine — installer
ui_print "****************************************"
ui_print "     CoreFlow Engine v2.0-MODERN        "
ui_print "       Author: mystivara-creator        "
ui_print "****************************************"

if [ "$ARCH" != "arm64" ]; then
    abort "! Unsupported architecture ($ARCH). CoreFlow requires ARM64."
fi

ui_print "- Runtime capability discovery will resolve thermal and UClamp interfaces after boot."

if [ -f "$MODPATH/system/bin/coreflowd" ]; then
    set_perm "$MODPATH/system/bin/coreflowd" 0 0 0755
elif [ -f "$MODPATH/coreflowd" ]; then
    set_perm "$MODPATH/coreflowd" 0 0 0755
else
    abort "! coreflowd binary missing from module package."
fi

set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
ui_print "- Installation complete. Reboot to start controlled capability discovery."
