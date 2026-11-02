#!/system/bin/sh
# CoreFlow Engine — Installer Script
ui_print "****************************************"
ui_print "     CoreFlow Engine v2.0-MODERN        "
ui_print "       Author: mystivara-creator        "
ui_print "****************************************"

# Verify ARM64 Architecture
if [ "$ARCH" != "arm64" ]; then
    abort "! Unsupported Architecture ($ARCH). CoreFlow requires ARM64."
fi

# Verify cgroups v2 availability
if [ ! -d "/sys/fs/cgroup" ]; then
    ui_print "! Warning: /sys/fs/cgroup not found. Enabling hybrid mode."
else
    ui_print "- Detected modern cgroups v2 subsystem."
fi

# Set executable permissions
if [ -f "$MODPATH/system/bin/coreflowd" ]; then
    set_perm "$MODPATH/system/bin/coreflowd" 0 0 0755
elif [ -f "$MODPATH/coreflowd" ]; then
    set_perm "$MODPATH/coreflowd" 0 0 0755
fi

set_perm "$MODPATH/service.sh" 0 0 0755
ui_print "- Installation successful! Reboot to activate."
