#!/system/bin/sh
# CFAE System Framework — hardened installer
ui_print "****************************************"
ui_print "       CFAE System Framework v3.1.0     "
ui_print "       Author: mystivara-creator        "
ui_print "****************************************"

if [ "$ARCH" != "arm64" ]; then
    abort "! Unsupported architecture ($ARCH). CFAE requires ARM64."
fi

if [ -n "${API:-}" ] && [ "$API" -lt 34 ]; then
    abort "! Unsupported Android API ($API). CFAE requires Android 14+ (API 34+)."
fi

if [ -d "/data/adb/modules/coreflow_engine" ] && [ "$MODPATH" != "/data/adb/modules/coreflow_engine" ]; then
    ui_print "- Migrating from legacy CoreFlow Engine installation..."
    touch "/data/adb/modules/coreflow_engine/remove" 2>/dev/null || rm -rf "/data/adb/modules/coreflow_engine" 2>/dev/null || true
fi

ui_print "- Runtime capability discovery will resolve thermal and UClamp interfaces after boot."

if [ -f "$MODPATH/system/bin/coreflowd" ]; then
    set_perm "$MODPATH/system/bin/coreflowd" 0 0 0755
elif [ -f "$MODPATH/coreflowd" ]; then
    mkdir -p "$MODPATH/system/bin"
    cp "$MODPATH/coreflowd" "$MODPATH/system/bin/coreflowd"
    set_perm "$MODPATH/system/bin/coreflowd" 0 0 0755
    set_perm "$MODPATH/coreflowd" 0 0 0755
else
    abort "! coreflowd binary missing from module package."
fi

set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
[ -f "$MODPATH/sepolicy.rule" ] && set_perm "$MODPATH/sepolicy.rule" 0 0 0644
[ -f "$MODPATH/module.prop" ] && set_perm "$MODPATH/module.prop" 0 0 0644
[ -f "$MODPATH/system/etc/coreflow/default.conf" ] && set_perm "$MODPATH/system/etc/coreflow/default.conf" 0 0 0644
[ -f "$MODPATH/system/lib64/libVkLayer_coreflow.so" ] && set_perm "$MODPATH/system/lib64/libVkLayer_coreflow.so" 0 0 0644
[ -f "$MODPATH/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" ] && set_perm "$MODPATH/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" 0 0 0644
[ -d "$MODPATH/webroot" ] && set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644

LIVE_MOD="/data/adb/modules/cfae_framework"
if [ -d "$LIVE_MOD" ] && [ "$MODPATH" != "$LIVE_MOD" ]; then
    ui_print "- Applying Zero-Reboot Live Hot-Upgrade to active module..."
    mkdir -p "$LIVE_MOD/system/bin" "$LIVE_MOD/system/lib64" "$LIVE_MOD/system/etc/vulkan/implicit_layer.d" "$LIVE_MOD/system/etc/coreflow" "$LIVE_MOD/webroot" 2>/dev/null || true
    cp -f "$MODPATH/system/bin/coreflowd" "$LIVE_MOD/system/bin/coreflowd" 2>/dev/null || true
    chmod 0755 "$LIVE_MOD/system/bin/coreflowd" 2>/dev/null || true
    [ -f "$MODPATH/system/lib64/libVkLayer_coreflow.so" ] && cp -f "$MODPATH/system/lib64/libVkLayer_coreflow.so" "$LIVE_MOD/system/lib64/libVkLayer_coreflow.so" 2>/dev/null || true
    [ -f "$MODPATH/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" ] && cp -f "$MODPATH/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" "$LIVE_MOD/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" 2>/dev/null || true
    [ -f "$MODPATH/system/etc/coreflow/default.conf" ] && cp -f "$MODPATH/system/etc/coreflow/default.conf" "$LIVE_MOD/system/etc/coreflow/default.conf" 2>/dev/null || true
    [ -f "$MODPATH/module.prop" ] && cp -f "$MODPATH/module.prop" "$LIVE_MOD/module.prop" 2>/dev/null || true
    if [ -d "$MODPATH/webroot" ]; then
        cp -rf "$MODPATH/webroot/"* "$LIVE_MOD/webroot/" 2>/dev/null || true
    fi
    "$LIVE_MOD/system/bin/coreflowd" --stop >/dev/null 2>&1 || killall -TERM coreflowd >/dev/null 2>&1 || true
    rm -f /data/adb/coreflow/SAFE_MODE /data/adb/coreflow/DISABLE 2>/dev/null || true
    mkdir -p /data/adb/coreflow/logs 2>/dev/null || true
    "$LIVE_MOD/system/bin/coreflowd" --daemon >> /data/adb/coreflow/logs/coreflowd.log 2>&1 </dev/null || true
    ui_print "- Live daemon & WebUI hot-reloaded! No reboot required."
else
    ui_print "- Universal Vulkan 1.4 Implicit Layer (VK_LAYER_COREFLOW_universal_bridge) armed."
    ui_print "- First-time installation complete. Reboot once to mount systemless overlay."
fi

