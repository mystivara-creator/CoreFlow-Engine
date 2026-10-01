#!/system/bin/sh
# =================================================================
# COREFLOW ENGINE INSTALLER
# Versi: v1.0.0-rebuild
# Arsitektur: Native C++ Daemon / ARM64
# =================================================================

# Mencegah Magisk/KSU mengekstrak zip secara otomatis agar kita bisa mengontrol alurnya
SKIPUNZIP=1

# --- FUNGSI KOSMETIK TERMINAL ---
print_banner() {
    ui_print "==========================================="
    ui_print "      COREFLOW ENGINE - NATIVE DAEMON      "
    ui_print "               Version: v1.0.0               "
    ui_print "==========================================="
}

print_info() {
    ui_print "[*] $1"
}

print_success() {
    ui_print "[+] $1"
}

print_warning() {
    ui_print "[!] $1"
}

print_error() {
    ui_print "[-] $1"
    abort "Instalasi dibatalkan!"
}

# --- 1. INISIALISASI & BANNER ---
print_banner
print_info "Memulai proses instalasi cerdas..."
sleep 1

# --- 2. DETEKSI ARSITEKTUR CPU ---
print_info "Memverifikasi arsitektur perangkat keras..."
ABI=$(getprop ro.product.cpu.abi)
if [ "$ABI" != "arm64-v8a" ]; then
    print_error "Arsitektur tidak didukung ($ABI). CoreFlow membutuhkan ARM64!"
else
    print_success "Arsitektur terverifikasi: $ABI"
fi

# --- 3. DETEKSI HARDWARE PROFILE (KUNZITE / PARROT) ---
BOARD=$(getprop ro.product.board)
DEVICE=$(getprop ro.product.device)
SOC=$(getprop ro.board.platform)
SDK=$(getprop ro.build.version.sdk)

print_info "Menganalisis Device Tree..."
ui_print "    - Nama Perangkat : $DEVICE"
ui_print "    - Motherboard    : $BOARD"
ui_print "    - Chipset (SoC)  : $SOC"
ui_print "    - API Level      : $SDK"

sleep 1

if [ "$BOARD" = "parrot" ] || [ "$BOARD" = "Parrot" ]; then
    ui_print " "
    ui_print ">>> KUNZITE PRIVILEGE PROFILE TERDETEKSI! <<<"
    ui_print ">>> Mengaktifkan optimasi khusus untuk:   <<<"
    ui_print ">>> Snapdragon 6 Gen 3 + Adreno 710       <<<"
    ui_print " "
else
    ui_print " "
    ui_print ">>> HYBRID UNIVERSAL PROFILE AKTIF!       <<<"
    ui_print ">>> Menyesuaikan sensor untuk mode global <<<"
    ui_print " "
fi

# --- 4. VERIFIKASI PRASYARAT FITUR (ZRAM) ---
print_info "Memeriksa status ZRAM (Swappiness Target)..."
if [ -b "/dev/block/zram0" ] || [ -d "/sys/block/zram0" ]; then
    print_success "ZRAM Aktif. Fitur Ultra Deep Sleep diizinkan."
else
    print_warning "ZRAM tidak terdeteksi! Manipulasi Swappiness mungkin diabaikan sistem."
fi

# --- 5. EKSTRAKSI FILE MODUL ---
print_info "Mengekstrak sistem biner C++ ke ruang modul..."
# Mengekstrak file yang relevan saja dari dalam ZIP
unzip -o "$ZIPFILE" 'module.prop' -d $MODPATH >&2
unzip -o "$ZIPFILE" 'service.sh' -d $MODPATH >&2
unzip -o "$ZIPFILE" 'sepolicy.rule' -d $MODPATH >&2
unzip -o "$ZIPFILE" 'system/*' -d $MODPATH >&2

if [ ! -f "$MODPATH/system/bin/coreflow_daemon" ]; then
    print_error "Biner coreflow_daemon gagal diekstrak! Cek struktur ZIP Anda."
fi

# --- 6. MANAJEMEN IZIN (PERMISSIONS) ---
print_info "Menerapkan atribut eksekusi dan keamanan (CHMOD)..."
# set_perm_recursive <direktori> <pemilik> <grup> <izin_direktori> <izin_file>
set_perm_recursive $MODPATH 0 0 0755 0644

# Berikan izin eksekusi penuh untuk skrip servis dan biner C++ daemon
set_perm $MODPATH/service.sh 0 0 0755
set_perm $MODPATH/system/bin/coreflow_daemon 0 0 0755

print_success "Izin eksekusi sukses diterapkan."

# --- 7. FINALISASI & PESAN KE PENGGUNA ---
sleep 1
ui_print "==========================================="
ui_print "     ⚡ INSTALASI COREFLOW SELESAI ⚡     "
ui_print "==========================================="
ui_print "- Harap restart (Reboot) perangkat Anda."
ui_print "- Jangan lupa MATIKAN Auto-Profile di FKM."
ui_print "- Pantau log: su -c 'logcat -s CoreFlowEngine'"
ui_print "==========================================="
