#!/system/bin/sh

MODDIR="${COREFLOW_MODDIR:-${0%/*}}"
STATE_DIR="${COREFLOW_STATE_DIR:-/data/adb/coreflow}"
LOG_DIR="$STATE_DIR/logs"
BINARY="$MODDIR/system/bin/coreflowd"
LIB_DIR="$MODDIR/system/lib64"
MODEL="$MODDIR/system/etc/coreflow/thermal_predictor.onnx"
PID_FILE="$STATE_DIR/coreflowd.pid"
BOOT_COUNTER="$STATE_DIR/boot_attempts"
SAFE_MODE_MARKER="$STATE_DIR/SAFE_MODE"
LOG_FILE="$LOG_DIR/coreflowd.log"
RESTART_REQUEST="$STATE_DIR/webui_restart_request"

# Exit codes from coreflowd that must NOT trigger a restart:
#   0 clean stop, 2 init failure, 3 another instance holds the lock, 4 lock error.
MAX_RESTARTS=5
BOOT_TIMEOUT=180

umask 077
mkdir -p "$STATE_DIR" "$LOG_DIR"
chmod 0700 "$STATE_DIR" "$LOG_DIR"

MAX_LOG_BYTES=524288

rotate_log_if_needed() {
    [ -f "$LOG_FILE" ] || return 0
    log_bytes="$(wc -c < "$LOG_FILE" 2>/dev/null)"
    case "$log_bytes" in
        ''|*[!0-9]*) return 0 ;;
    esac
    if [ "$log_bytes" -ge "$MAX_LOG_BYTES" ]; then
        mv -f "$LOG_FILE" "${LOG_FILE}.1" 2>/dev/null
    fi
}

log() {
    rotate_log_if_needed
    echo "$(date '+%F %T') $*" >> "$LOG_FILE"
}

WAITED=0
while [ "$(getprop sys.boot_completed 2>/dev/null)" != "1" ]; do
    sleep 2
    WAITED=$((WAITED + 2))
    [ "$WAITED" -ge "$BOOT_TIMEOUT" ] && exit 0
done

# Boot confirmed: clear the boot-loop counter.
echo 0 > "$BOOT_COUNTER" 2>/dev/null
log "boot confirmed; boot-loop counter reset"
sleep 5

# Performance, Rendering & ART Compiler Framework
# Universal dynamic detection: no vendor-locked or hardcoded chipset assumptions.
ncpu="$(getconf _NPROCESSORS_ONLN 2>/dev/null || grep -c ^processor /proc/cpuinfo 2>/dev/null || echo 8)"
[ "$ncpu" -gt 0 ] 2>/dev/null || ncpu=8

if command -v resetprop >/dev/null 2>&1; then
    # 1. ART DEX2OAT multi-threaded compilation (unshackle OEM marketing limits)
    resetprop -n dalvik.vm.dex2oat-threads "$ncpu" 2>/dev/null
    resetprop -n dalvik.vm.image-dex2oat-threads "$ncpu" 2>/dev/null
    resetprop -n dalvik.vm.boot-dex2oat-threads "$ncpu" 2>/dev/null
    resetprop -n pm.dexopt.install speed-profile 2>/dev/null
    resetprop -n pm.dexopt.bg-dexopt speed-profile 2>/dev/null

    # 2. Universal Hardware Vulkan Discovery & Skia Render Pipeline
    # Checks package manager features and vendor driver libraries across all architectures.
    # Excludes CPU-only software rasterizers (e.g. SwiftShader, pastel) to ensure real hardware GPU execution.
    has_hw_vulkan=0
    if pm has-feature android.hardware.vulkan.level 2>/dev/null | grep -q "true" || \
       pm has-feature android.hardware.vulkan.version 2>/dev/null | grep -q "true"; then
        has_hw_vulkan=1
    fi

    if [ "$has_hw_vulkan" -eq 0 ]; then
        for vklib in /vendor/lib64/hw/vulkan.*.so /odm/lib64/hw/vulkan.*.so /system/lib64/hw/vulkan.*.so /vendor/lib/hw/vulkan.*.so; do
            if [ -f "$vklib" ]; then
                case "$vklib" in
                    *pastel*|*swiftshader*) ;;
                    *) has_hw_vulkan=1; break ;;
                esac
            fi
        done
    fi

    if [ "$has_hw_vulkan" -eq 1 ]; then
        resetprop -n debug.hwui.renderer skiavk 2>/dev/null
        resetprop -n ro.hwui.use_vulkan true 2>/dev/null
        resetprop -n renderthread.skia.reduceopstasksplitting true 2>/dev/null
        resetprop -n debug.hwui.use_partial_updates true 2>/dev/null
        resetprop -n debug.hwui.skia_tracing_enabled false 2>/dev/null
        resetprop -n debug.hwui.skia_atrace_enabled false 2>/dev/null
        resetprop -n debug.hwui.render_thread_priority -10 2>/dev/null
        log "Universal hardware Vulkan active: Skia Vulkan GPU pipeline engaged"
    else
        resetprop -n debug.hwui.renderer skiagl 2>/dev/null
        resetprop -n renderthread.skia.reduceopstasksplitting true 2>/dev/null
        resetprop -n debug.hwui.use_partial_updates true 2>/dev/null
        log "Vulkan hardware driver not present: optimized SkiaGL pipeline engaged"
    fi

    # 3. Universal Compositor & Frame Pacing Latency Reduction
    resetprop -n debug.sf.latch_unsignaled 1 2>/dev/null
    resetprop -n debug.sf.enable_gl_backpressure 1 2>/dev/null
    resetprop -n sys.use_fifo_ui 1 2>/dev/null
    if [ -n "$(getprop vendor.perf.gesturefunc 2>/dev/null)" ] || [ -f /sys/devices/system/cpu/perf/perf_profile ]; then
        resetprop -n vendor.perf.gesturefunc 1 2>/dev/null
    fi
    log "Applied framework optimizations: ART ($ncpu threads), Skia rendering, and frame latching"
fi

[ -x "$BINARY" ] || { log "coreflowd missing or not executable"; exit 1; }
[ -s "$LIB_DIR/libonnxruntime.so" ] || { log "libonnxruntime.so missing"; exit 1; }
[ -s "$MODEL" ] || { log "thermal_predictor.onnx missing"; exit 1; }

# Module-local ONNX runtime must win over any system copy.
export LD_LIBRARY_PATH="$LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

if [ -f "$PID_FILE" ]; then
    OLD_PID="$(cat "$PID_FILE" 2>/dev/null)"
    if [ -n "$OLD_PID" ] && kill -0 "$OLD_PID" 2>/dev/null; then
        exit 0
    fi
    rm -f "$PID_FILE"
fi

VERSION="$(awk -F= '$1=="version" {print $2; exit}' "$MODDIR/module.prop" 2>/dev/null)"
VERSION="${VERSION:-unknown}"

# Supervisor: restarts coreflowd after a crash with bounded backoff. The daemon
# recovers its journal on start, so a restart always begins from a safe state.
supervise() {
    CHILD=""
    STOPPING=0
    trap 'STOPPING=1; [ -n "$CHILD" ] && kill -TERM "$CHILD" 2>/dev/null' TERM INT HUP

    restarts=0
    while [ "$STOPPING" -eq 0 ]; do
        rotate_log_if_needed
        log "starting coreflowd attempt=$((restarts + 1)) version=$VERSION"
        # This marker only tracks UI configuration changes that require a daemon restart.
        # It is not a safety marker and never changes engine mutation policy.
        rm -f "$STATE_DIR/webui_config_pending" 2>/dev/null
        "$BINARY" >> "$LOG_FILE" 2>&1 &
        CHILD=$!

        # Optimize daemon handle: elevate scheduling priority, protect from Android LMK (-800),
        # tune I/O niceness, and attach to system cgroups if available.
        if [ -n "$CHILD" ] && [ "$CHILD" -gt 0 ] 2>/dev/null; then
            echo -800 > "/proc/$CHILD/oom_score_adj" 2>/dev/null
            renice -n -10 -p "$CHILD" 2>/dev/null
            ionice -c 2 -n 2 -p "$CHILD" 2>/dev/null
            for cg in /dev/cpuset/system-background/cgroup.procs /dev/cpuset/foreground/cgroup.procs; do
                if [ -w "$cg" ]; then
                    echo "$CHILD" > "$cg" 2>/dev/null
                    break
                fi
            done
        fi

        rc=0
        while :; do
            wait "$CHILD"
            rc=$?
            kill -0 "$CHILD" 2>/dev/null || break
        done
        CHILD=""

        if [ "$STOPPING" -eq 1 ]; then
            log "coreflowd stopped rc=$rc"
            break
        fi

        # WebUI "Restart" writes this marker, then terminates the child. Relaunch at once
        # and do not count it toward the crash limit. A stop request never reaches here
        # because STOPPING is set by SIGTERM to the supervisor itself.
        if [ -f "$RESTART_REQUEST" ]; then
            rm -f "$RESTART_REQUEST" 2>/dev/null
            log "restart requested from WebUI rc=$rc; relaunching coreflowd"
            continue
        fi

        case "$rc" in
            0|2|3|4)
                log "coreflowd exited rc=$rc; not restarting"
                break
                ;;
        esac

        restarts=$((restarts + 1))
        if [ "$restarts" -gt "$MAX_RESTARTS" ]; then
            log "coreflowd restart limit reached; giving up, SAFE_MODE set"
            touch "$SAFE_MODE_MARKER" 2>/dev/null
            break
        fi

        delay=$((restarts * 5))
        log "coreflowd crashed rc=$rc; restarting in ${delay}s"
        sleep "$delay" &
        wait $!
    done

    rm -f "$PID_FILE"
}

supervise &
SUPERVISOR_PID=$!
echo "$SUPERVISOR_PID" > "$PID_FILE"
chmod 0600 "$PID_FILE"
log "CoreFlow supervisor started pid=$SUPERVISOR_PID version=$VERSION"
