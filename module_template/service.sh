#!/system/bin/sh
# Pelatuk Native Daemon CoreFlow Engine

MODDIR=${0%/*}
DAEMON_BIN="$MODDIR/coreflow_daemon"

# Pastikan binary C++ memiliki izin eksekusi
chmod 755 $DAEMON_BIN

# Jalankan daemon C++ di latar belakang secara mandiri (detached)
# Output log diarahkan ke dev/null untuk mencegah memori internal penuh
nohup $DAEMON_BIN > /dev/null 2>&1 &
