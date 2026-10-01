#include "../include/coreflow.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <sys/system_properties.h> // Native Android property service (Zero Popen Overhead)

// Struktur Data Internal untuk Menyimpan Nilai Default Pabrik (Stock Memory & I/O)
namespace StockProfile {
    unsigned int read_ahead_kb = 512;
    unsigned int nr_requests = 62;      // Sesuai baseline FKM Kunzite
    std::string cpu_policy0_gov = "schedutil";
    std::string cpu_policy4_gov = "schedutil";
}

// Variabel Kontrol Internal untuk Melacak Transisi State 5 Pilar
namespace CoreFlowState {
    enum EngineMode { MODE_IDLE, MODE_BALANCED, MODE_BURST, MODE_GAMING };
    EngineMode current_mode = MODE_BALANCED;
    bool is_initialized = false;
    
    // CPU Jiffies tracking untuk kalkulasi load
    unsigned long long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0;
}

// =================================================================
// SYSTEM UTIL: Pengubah Parameter Aman & Presisi (Zero Sub-Shell)
// =================================================================
namespace SafeTuner {
    void writeSysfs(const std::string& path, const std::string& value) {
        std::ofstream file(path);
        if (file.is_open()) {
            file << value;
            file.close();
        }
    }
    
    void readSysfs(const std::string& path, std::string& output) {
        std::ifstream file(path);
        if (file.is_open()) {
            file >> output;
            file.close();
        }
    }
}

// =================================================================
// COREFLOW AI: Universal Adaptation & Device Orchestration (5 Pillars)
// =================================================================

void CoreFlowAI::initializeHardwareProfile() {
    if (CoreFlowState::is_initialized) return;

    std::cout << "[CoreFlow AI] Menjalankan pemindaian arsitektur hardware..." << std::endl;

    // Memverifikasi Device Profile secara native tanpa sub-shell popen()
    char board_char[PROP_VALUE_MAX];
    int len = __system_property_get("ro.product.board", board_char);
    std::string board_name = (len > 0) ? std::string(board_char) : "";

    // Bersihkan karakter newline jika ada
    if (!board_name.empty() && board_name.back() == '\n') {
        board_name.pop_back();
    }

    if (board_name == "parrot" || board_name == "Parrot") {
        std::cout << "[CoreFlow AI] Target Terverifikasi: Redmi Note 15 5G (Kunzite/Parrot)." << std::endl;
        std::cout << "[CoreFlow AI] Mengaktifkan Kunzite Privilege Profile (Snapdragon 6 Gen 3)." << std::endl;
    } else {
        std::cout << "[CoreFlow AI] Berjalan di perangkat Universal. Mengaktifkan deteksi adaptif." << std::endl;
    }

    // -------------------------------------------------------------
    // BACKUP STOCK VALUE: Membaca nilai asli pabrik dari sda
    // -------------------------------------------------------------
    std::string raw_read_ahead, raw_nr;
    SafeTuner::readSysfs("/sys/block/sda/queue/read_ahead_kb", raw_read_ahead);
    SafeTuner::readSysfs("/sys/block/sda/queue/nr_requests", raw_nr);
    if (!raw_read_ahead.empty()) StockProfile::read_ahead_kb = std::stoul(raw_read_ahead);
    if (!raw_nr.empty()) StockProfile::nr_requests = std::stoul(raw_nr);

    SafeTuner::readSysfs("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", StockProfile::cpu_policy0_gov);
    SafeTuner::readSysfs("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", StockProfile::cpu_policy4_gov);

    // Inisialisasi Baseline Awal pada semua blok UFS (sda - sdf)
    for (char blk = 'a'; blk <= 'f'; ++blk) {
        std::string prefix = "/sys/block/sd" + std::string(1, blk) + "/queue/";
        SafeTuner::writeSysfs(prefix + "read_ahead_kb", std::to_string(StockProfile::read_ahead_kb));
        SafeTuner::writeSysfs(prefix + "nr_requests", std::to_string(StockProfile::nr_requests));
    }

    CoreFlowState::is_initialized = true;
    std::cout << "[CoreFlow AI] Inisialisasi framework & 5-Pillar baseline selesai." << std::endl;
}

void CoreFlowAI::evaluateDynamicLoad() {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return;

    std::string line, cpu;
    std::getline(file, line);
    file.close();
    
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    std::stringstream ss(line);
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    unsigned long long prev_total = CoreFlowState::prev_user + CoreFlowState::prev_nice + CoreFlowState::prev_system + CoreFlowState::prev_idle;
    unsigned long long current_total = user + nice + system + idle;

    unsigned long long total_diff = current_total - prev_total;
    unsigned long long idle_diff = idle - CoreFlowState::prev_idle;

    double cpu_usage = 0.0;
    if (total_diff > 0) {
        cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff;
    }

    CoreFlowState::prev_user = user; CoreFlowState::prev_nice = nice; 
    CoreFlowState::prev_system = system; CoreFlowState::prev_idle = idle;

    int gpu_busy = 0;
    std::string raw_gpu_busy;
    SafeTuner::readSysfs("/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage", raw_gpu_busy);
    if (!raw_gpu_busy.empty()) {
        gpu_busy = std::stoi(raw_gpu_busy);
    }

    // -------------------------------------------------------------
    // DECISION ENGINE: 5-PILLAR CLASSIFICATION & STATE SYMBIOSIS
    // -------------------------------------------------------------
    CoreFlowState::EngineMode target_mode = CoreFlowState::MODE_BALANCED;

    if (gpu_busy > 45 || cpu_usage > 75.0) {
        target_mode = CoreFlowState::MODE_GAMING; // Pilar 3: Gaming Unleashed
    } else if (cpu_usage >= 25.0 && cpu_usage <= 75.0) {
        target_mode = CoreFlowState::MODE_BURST;   // Pilar 2: Tango & App Launch Burst
    } else if (cpu_usage < 15.0 && gpu_busy < 8) {
        target_mode = CoreFlowState::MODE_IDLE;    // Pilar 1: Daily Efficiency
    }

    // Eksekusi perubahan hanya jika terjadi pergeseran state (Zero Loop Overhead)
    if (target_mode != CoreFlowState::current_mode) {
        CoreFlowState::current_mode = target_mode;

        if (CoreFlowState::current_mode == CoreFlowState::MODE_GAMING) {
            std::cout << "[CoreFlow AI] Pilar 3 Aktif: Gaming Performance Unleashed." << std::endl;
            
            for (char blk = 'a'; blk <= 'f'; ++blk) {
                std::string prefix = "/sys/block/sd" + std::string(1, blk) + "/queue/";
                SafeTuner::writeSysfs(prefix + "read_ahead_kb", "2048");
                SafeTuner::writeSysfs(prefix + "nr_requests", "256");
            }
            
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "60");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "55");
            SafeTuner::writeSysfs("/sys/class/kgsl/kgsl-3d0/devfreq/governor", "msm-adreno-tz");
            
            // Setel QoS NPU via Bionic API secara aman untuk gaming render assistance
            __system_property_set("vendor.dsp.default_qos", "1");
        } 
        else if (CoreFlowState::current_mode == CoreFlowState::MODE_BURST) {
            std::cout << "[CoreFlow AI] Pilar 2 Aktif: App Launch / Tango Burst (Sweet Spot)." << std::endl;
            
            for (char blk = 'a'; blk <= 'f'; ++blk) {
                std::string prefix = "/sys/block/sd" + std::string(1, blk) + "/queue/";
                SafeTuner::writeSysfs(prefix + "read_ahead_kb", "1024");
                SafeTuner::writeSysfs(prefix + "nr_requests", "128");
            }
            
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "70");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "70");
        }
        else if (CoreFlowState::current_mode == CoreFlowState::MODE_BALANCED) {
            std::cout << "[CoreFlow AI] Pilar 1 Aktif: Daily Balanced State." << std::endl;
            
            // Profil harian optimal (menggunakan nilai hasil backup dinamis)
            for (char blk = 'a'; blk <= 'f'; ++blk) {
                std::string prefix = "/sys/block/sd" + std::string(1, blk) + "/queue/";
                SafeTuner::writeSysfs(prefix + "read_ahead_kb", std::to_string(StockProfile::read_ahead_kb));
                SafeTuner::writeSysfs(prefix + "nr_requests", std::to_string(StockProfile::nr_requests));
            }
            
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "80");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "80");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", StockProfile::cpu_policy0_gov);
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", StockProfile::cpu_policy4_gov);
            __system_property_set("vendor.dsp.default_qos", "1");
        }
        else if (CoreFlowState::current_mode == CoreFlowState::MODE_IDLE) {
            std::cout << "[CoreFlow AI] Pilar 1 Aktif: Daily Efficiency (Layar On Idle)." << std::endl;
            
            for (char blk = 'a'; blk <= 'f'; ++blk) {
                std::string prefix = "/sys/block/sd" + std::string(1, blk) + "/queue/";
                SafeTuner::writeSysfs(prefix + "read_ahead_kb", std::to_string(StockProfile::read_ahead_kb));
            }
            
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "85");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "85");
        }
    }
}

void CoreFlowAI::setUltraIdleMode() {
    std::cout << "[CoreFlow AI] Layar Mati. Mengaktifkan Ultra Deep Sleep & NPU Off." << std::endl;
    
    // Core Besar dipaksa masuk ke Powersave agar kernel masuk deep sleep sempurna
    SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", "powersave");
    
    // Potong I/O buffer ke ukuran minimal saat tidur untuk mencegah cache bloat
    for (char blk = 'a'; blk <= 'f'; ++blk) {
        std::string prefix = "/sys/block/sd" + std::string(1, blk) + "/queue/";
        SafeTuner::writeSysfs(prefix + "read_ahead_kb", "256");
    }
    
    // Matikan QoS DSP secara native agar NPU ikut tertidur total
    __system_property_set("vendor.dsp.default_qos", "0");
    
    // Reset state kontrol agar saat layar menyala kembali, deteksi dimulai dari awal
    CoreFlowState::current_mode = CoreFlowState::MODE_IDLE;
}
