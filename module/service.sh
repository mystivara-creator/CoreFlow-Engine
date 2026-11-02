#!/system/bin/sh
# CoreFlow Engine — Late Start Service
# Starts native daemon after Android boot completion

MODDIR=${0%/*}

# Wait until Android boots completely
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
done

# Grant executable permission
chmod 755 "$MODDIR/system/bin/coreflowd" 2>/dev/null || chmod 755 "$MODDIR/coreflowd" 2>/dev/null

# Execute native daemon with nice -20 (high priority)
if [ -f "$MODDIR/system/bin/coreflowd" ]; then
    nohup "$MODDIR/system/bin/coreflowd" > /dev/null 2>&1 &
elif [ -f "$MODDIR/coreflowd" ]; then
    nohup "$MODDIR/coreflowd" > /dev/null 2>&1 &
fi
