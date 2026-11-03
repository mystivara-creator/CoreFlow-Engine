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
if [ -f "/sys/fs/cgroup/cgroup.controllers" ]; then
    ui_print "- Detected unified cgroups v2; node support checked by daemon."
else
    ui_print "! No unified cgroups v2 controller detected. Unsupported tuning stays inactive."
fi

# Set executable permissions
if [ -f "$MODPATH/system/bin/coreflowd" ]; then
    set_perm "$MODPATH/system/bin/coreflowd" 0 0 0755
elif [ -f "$MODPATH/coreflowd" ]; then
    set_perm "$MODPATH/coreflowd" 0 0 0755
fi

set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
ui_print "- Installation successful! Reboot to activate."
