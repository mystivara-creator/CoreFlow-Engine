#!/system/bin/sh
MODDIR=${0%/*}

# Tunggu sampai sistem Android benar-benar selesai booting
until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 5
done

# Jeda ekstra 10 detik agar subsistem sysfs grafis panel siap sepenuhnya
sleep 10

# Pastikan izin eksekusi biner di folder lokal modul sudah aktif
chmod 755 $MODDIR/system/bin/coreflow_daemon

# JALANKAN DAEMON FINAL: Output dialirkan ke sistem Logcat Android
$MODDIR/system/bin/coreflow_daemon 2>&1 | log -p d -t CoreFlowEngine &
