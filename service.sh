#!/system/bin/sh
# CoreFlow Engine — Late Start Service
# Starts native daemon after Android boot completion

MODDIR=${0%/*}

# Wait until Android boots completely
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
done

# Grant executable permission
chmod 755 "$MODDIR/coreflowd"

# Execute native daemon with nice -20 (high priority)
nohup "$MODDIR/coreflowd" > /dev/null 2>&1 &
