#!/system/bin/sh
# CoreFlow Autonomous Engine v3.2.0 — Magisk / KernelSU installer.
SKIPUNZIP=1

CF_TITLE='COREFLOW AUTONOMOUS ENGINE'
cf_info()  { ui_print "[CoreFlow] $1"; }
cf_warn()  { ui_print "[CoreFlow][!] $1"; }
cf_abort() { ui_print "[CoreFlow][X] $1"; abort "$1"; }
cf_has_entry() { unzip -p "$ZIPFILE" "$1" >/dev/null 2>&1; }

ui_print ''
ui_print '╔══════════════════════════════════════════╗'
ui_print '║       COREFLOW AUTONOMOUS ENGINE          ║'
ui_print '║                 v3.2.0                   ║'
ui_print '╚══════════════════════════════════════════╝'
ui_print ''
cf_info 'Memeriksa lingkungan instalasi...'

ABI="$(getprop ro.product.cpu.abi 2>/dev/null)"
[ "$ABI" = 'arm64-v8a' ] || cf_abort "ABI perangkat tidak didukung: ${ABI:-unknown}. Paket ini memerlukan arm64-v8a."
API="$(getprop ro.build.version.sdk 2>/dev/null)"
case "$API" in ''|*[!0-9]*) cf_warn 'Versi API Android tidak dapat dibaca; melanjutkan pemeriksaan file.' ;; esac
if [ -n "$API" ] && [ "$API" -lt 30 ] 2>/dev/null; then
    cf_abort "Android API $API terlalu lama; paket ini menargetkan Android 11/API 30 ke atas."
fi

[ -n "$MODPATH" ] || cf_abort 'Direktori modul MODPATH tidak tersedia.'
[ -n "$ZIPFILE" ] && [ -f "$ZIPFILE" ] || cf_abort 'Arsip modul ZIPFILE tidak tersedia.'

cf_info 'Memvalidasi isi paket...'
for entry in module.prop service.sh sepolicy.rule strategies.tsv system/bin/coreflow_daemon webroot/index.html; do
    cf_has_entry "$entry" || cf_abort "File wajib tidak ditemukan dalam ZIP: $entry"
done

cf_info 'Mengekstrak daemon, katalog strategi, service, dan WebUI...'
for entry in module.prop service.sh sepolicy.rule strategies.tsv; do
    unzip -o "$ZIPFILE" "$entry" -d "$MODPATH" >&2 || cf_abort "Gagal mengekstrak $entry"
done
unzip -o "$ZIPFILE" 'system/bin/coreflow_daemon' -d "$MODPATH" >&2 || cf_abort 'Gagal mengekstrak daemon.'
unzip -o "$ZIPFILE" 'webroot/*' -d "$MODPATH" >&2 || cf_abort 'Gagal mengekstrak WebUI.'

DAEMON="$MODPATH/system/bin/coreflow_daemon"
[ -s "$DAEMON" ] || cf_abort 'Binary coreflow_daemon kosong atau tidak ditemukan.'
[ -f "$MODPATH/service.sh" ] || cf_abort 'service.sh tidak ditemukan setelah ekstraksi.'
[ -s "$MODPATH/strategies.tsv" ] || cf_abort 'Katalog strategi kosong atau tidak ditemukan.'
[ -f "$MODPATH/sepolicy.rule" ] || cf_abort 'sepolicy.rule tidak ditemukan setelah ekstraksi.'
[ -s "$MODPATH/webroot/index.html" ] || cf_abort 'WebUI tidak ditemukan setelah ekstraksi.'

cf_info 'Mengatur permission file...'
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$DAEMON" 0 0 0755

# Do not execute a kernel mutation during installation. The service starts after
# boot completion; runtime decisions are made by the daemon against live discovery.
ui_print ''
ui_print '┌──────────────────────────────────────────┐'
ui_print '│              INSTALASI SIAP              │'
ui_print '└──────────────────────────────────────────┘'
cf_info "Perangkat: ${ABI} | Android API: ${API:-unknown}"
cf_info 'Daemon akan dijalankan oleh service setelah boot selesai.'
cf_info 'Log opsional: logcat -s CoreFlowEngine'
cf_info 'WebUI: buka dari KernelSU Manager (CoreFlow Console). Tidak perlu terminal.'
cf_info 'Status WebUI: /data/adb/coreflow/status.json'
cf_warn 'Hasil mutasi bergantung pada dukungan kernel dan validasi runtime perangkat.'
ui_print ''
