#!/system/bin/sh

OUT="${1:-$HOME/coreflow-device-inventory}"
mkdir -p "$OUT" || exit 1

echo "[1/6] Identitas perangkat..."
{
    echo "=== DEVICE ==="
    getprop ro.product.manufacturer
    getprop ro.product.model
    getprop ro.product.device
    getprop ro.soc.model
    getprop ro.build.version.release
    getprop ro.build.version.sdk
    uname -a
} > "$OUT/device.txt" 2>&1

echo "[2/6] Inventaris node /sys..."
su -c '
OUT="'"$OUT"'"
find /sys -type f 2>/dev/null > "$OUT/sys-files.txt"
find /sys -type d 2>/dev/null > "$OUT/sys-dirs.txt"
'

echo "[3/6] Inventaris node /proc..."
su -c '
OUT="'"$OUT"'"
find /proc -type f 2>/dev/null > "$OUT/proc-files.txt"
find /proc -type d 2>/dev/null > "$OUT/proc-dirs.txt"
'

echo "[4/6] Thermal, CPU, dan daya..."
su -c '
OUT="'"$OUT"'"

for d in /sys/class/thermal/thermal_zone*; do
    [ -d "$d" ] || continue
    {
        echo "=== $d ==="
        for f in "$d"/type "$d"/temp "$d"/mode "$d"/policy; do
            [ -r "$f" ] || continue
            printf "%s=" "${f##*/}"
            cat "$f" 2>/dev/null
        done
    }
done > "$OUT/thermal.txt"

{
    for d in /sys/devices/system/cpu/cpufreq/* /sys/devices/system/cpu/cpu[0-9]*; do
        [ -d "$d" ] || continue
        echo "=== $d ==="
        for f in "$d"/*; do
            [ -f "$f" ] || continue
            case "${f##*/}" in
                *freq*|*governor*|*cpus*|*uclamp*|*capacity*|*online*|*idle*)
                    [ -r "$f" ] || continue
                    printf "%s=" "${f##*/}"
                    cat "$f" 2>/dev/null
                    ;;
            esac
        done
    done
} > "$OUT/cpu.txt"

{
    for d in /sys/class/power_supply/*; do
        [ -d "$d" ] || continue
        echo "=== $d ==="
        for f in "$d"/*; do
            [ -f "$f" ] || continue
            case "${f##*/}" in
                type|status|health|present|online|capacity|temp|voltage_now|current_now|power_now|charge_counter|technology)
                    [ -r "$f" ] || continue
                    printf "%s=" "${f##*/}"
                    cat "$f" 2>/dev/null
                    ;;
            esac
        done
    done
} > "$OUT/power.txt"
'

echo "[5/6] Ringkasan VM dan memori..."
su -c '
OUT="'"$OUT"'"
{
    echo "=== /proc/meminfo ==="
    cat /proc/meminfo 2>/dev/null
    echo
    echo "=== /proc/vmstat ==="
    cat /proc/vmstat 2>/dev/null
    echo
    echo "=== /proc/stat ==="
    cat /proc/stat 2>/dev/null
    echo
    echo "=== /proc/pressure ==="
    for f in /proc/pressure/*; do
        [ -r "$f" ] || continue
        echo "--- $f"
        cat "$f"
    done
    echo
    echo "=== /proc/sys/vm ==="
    for f in /proc/sys/vm/*; do
        [ -f "$f" ] && [ -r "$f" ] || continue
        echo "--- $f"
        cat "$f" 2>/dev/null
    done
} > "$OUT/memory-vm.txt"
'

echo "[6/6] Metadata akses terbatas..."
su -c '
OUT="'"$OUT"'"
{
    for base in /sys/class/thermal /sys/class/power_supply \
                /sys/devices/system/cpu/cpufreq /proc/sys/vm; do
        [ -d "$base" ] || continue
        find "$base" -maxdepth 3 -type f 2>/dev/null |
        while IFS= read -r f; do
            [ -r "$f" ] || continue
            [ -w "$f" ] && access="read-write" || access="read-only"
            printf "%s\t%s\n" "$access" "$f"
        done
    done
} > "$OUT/access-map.txt"
'

echo
echo "Inventory selesai."
echo "Direktori: $OUT"
cat "$OUT/README.txt"
