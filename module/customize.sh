#!/system/bin/sh
# CFAE System Framework — hardened installer
ui_print "****************************************"
ui_print "       CFAE System Framework v3.1.0     "
ui_print "       Author: mystivara-creator        "
ui_print "****************************************"

if [ "$ARCH" != "arm64" ]; then
    abort "! Unsupported architecture ($ARCH). CFAE requires ARM64."
fi

if [ -n "${API:-}" ] && [ "$API" -lt 35 ]; then
    abort "! Unsupported Android API ($API). CFAE requires Android 15+ (API 35+)."
fi

STATE_DIR="${COREFLOW_STATE_DIR:-/data/adb/coreflow}"
LIVE_MOD="${COREFLOW_LIVE_MOD_DIR:-/data/adb/modules/cfae_framework}"

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

stage_and_replace() {
    SRC="$1"
    DST="$2"
    MODE="$3"
    [ -f "$SRC" ] || return 1
    DST_DIR="${DST%/*}"
    mkdir -p "$DST_DIR" || return 1
    TMP_DST="${DST}.stage.$$"
    rm -f "$TMP_DST"
    if cp -f "$SRC" "$TMP_DST" && chmod "$MODE" "$TMP_DST" && mv -f "$TMP_DST" "$DST"; then
        return 0
    fi
    rm -f "$TMP_DST"
    return 1
}

if [ -d "$LIVE_MOD" ] && [ "$MODPATH" != "$LIVE_MOD" ]; then
    ui_print "- Staging verified live hot-upgrade to active module..."
    UPGRADE_OK=1
    stage_and_replace "$MODPATH/system/bin/coreflowd" "$LIVE_MOD/system/bin/coreflowd" 0755 || UPGRADE_OK=0
    if [ -f "$MODPATH/system/lib64/libVkLayer_coreflow.so" ]; then
        stage_and_replace "$MODPATH/system/lib64/libVkLayer_coreflow.so" "$LIVE_MOD/system/lib64/libVkLayer_coreflow.so" 0644 || UPGRADE_OK=0
    fi
    if [ -f "$MODPATH/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" ]; then
        stage_and_replace "$MODPATH/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" "$LIVE_MOD/system/etc/vulkan/implicit_layer.d/coreflow_vulkan_layer.json" 0644 || UPGRADE_OK=0
    fi
    if [ -f "$MODPATH/system/etc/coreflow/default.conf" ]; then
        stage_and_replace "$MODPATH/system/etc/coreflow/default.conf" "$LIVE_MOD/system/etc/coreflow/default.conf" 0644 || UPGRADE_OK=0
    fi
    if [ -f "$MODPATH/module.prop" ]; then
        stage_and_replace "$MODPATH/module.prop" "$LIVE_MOD/module.prop" 0644 || UPGRADE_OK=0
    fi
    if [ -d "$MODPATH/webroot" ]; then
        mkdir -p "$LIVE_MOD/webroot" || UPGRADE_OK=0
        cp -rf "$MODPATH/webroot/." "$LIVE_MOD/webroot/" 2>/dev/null || UPGRADE_OK=0
    fi

    if [ "$UPGRADE_OK" -ne 1 ]; then
        ui_print "! Live hot-upgrade staging incomplete; changes will apply on next reboot."
    else
        "$LIVE_MOD/system/bin/coreflowd" --stop >/dev/null 2>&1 || killall -TERM coreflowd >/dev/null 2>&1 || true
        # Never erase SAFE_MODE or DISABLE markers during hot-upgrade.
        if [ -e "$STATE_DIR/SAFE_MODE" ] || [ -e "$STATE_DIR/DISABLE" ] || [ -e "$LIVE_MOD/disable" ]; then
            ui_print "- Safety marker (SAFE_MODE/DISABLE) preserved; skipping automatic daemon restart."
        else
            mkdir -p "$STATE_DIR/logs" 2>/dev/null || true
            if "$LIVE_MOD/system/bin/coreflowd" --daemon >> "$STATE_DIR/logs/coreflowd.log" 2>&1 </dev/null && \
               "$LIVE_MOD/system/bin/coreflowd" --status 2>/dev/null | grep -q '"running":true'; then
                ui_print "- Live daemon & WebUI hot-reloaded and verified!"
            else
                ui_print "! Live daemon restart not confirmed; changes will take effect after reboot."
            fi
        fi
    fi
else
    ui_print "- Vulkan diagnostic layer installed (disabled by default; opt-in via ENABLE_COREFLOW_VULKAN_LAYER=1)."
    ui_print "- First-time installation complete. Reboot once to mount systemless overlay."
fi
