#include "../include/coreflow.hpp"
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/system_properties.h>

// ============================================================
// INTERNAL STOCK PROFILE
// ============================================================
namespace StockProfile {
    unsigned int read_ahead_kb = 512;
    unsigned int nr_requests = 62;
    std::string cpu_policy0_gov = "schedutil";
    std::string cpu_policy4_gov = "schedutil";
} // namespace StockProfile

// ============================================================
// CORE FLOW STATE
// ============================================================
namespace CoreFlowState {
    enum EngineMode { MODE_IDLE, MODE_BALANCED, MODE_BURST, MODE_GAMING };
    EngineMode current_mode = MODE_BALANCED;
    bool is_initialized = false;
    
    // Previous /proc/stat CPU counters
    unsigned long long prev_user = 0;
    unsigned long long prev_nice = 0;
    unsigned long long prev_system = 0;
    unsigned long long prev_idle = 0;
} // namespace CoreFlowState

// ============================================================
// SAFE SYSFS / SYSTEM UTILITY
// ============================================================
namespace SafeTuner {
    void writeSysfs(const std::string& path, const std::string& value) {
        std::ofstream file(path);
        if (!file.is_open()) {
            return;
        }
        file << value;
        file.close();
    }

    void readSysfs(const std::string& path, std::string& output) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return;
        }
        file >> output;
        file.close();
    }
} // namespace SafeTuner

// ============================================================
// CORE FLOW AI (Namespace Tunggal Terpadu)
// ============================================================
namespace CoreFlowAI {

    // ============================================================
    // HELPER: mq-deadline Advanced Tunables & Sysctl Dinamis
    // ============================================================
    void applyMqDeadlineTunables(bool is_high_performance) {
        char path_buffer[128];
        const std::string read_exp = is_high_performance ? "250" : "500";
        const std::string write_exp = is_high_performance ? "2500" : "5000"; 
        const std::string async_dp = is_high_performance ? "128" : "64";
        const std::string fifo_bt = "16";

        for (char blk = 'a'; blk <= 'f'; ++blk) {
            snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iosched/read_expire", blk);
            SafeTuner::writeSysfs(path_buffer, read_exp);

            snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iosched/write_expire", blk);
            SafeTuner::writeSysfs(path_buffer, write_exp);

            snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iosched/async_depth", blk);
            SafeTuner::writeSysfs(path_buffer, async_dp);

            snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iosched/fifo_batch", blk);
            SafeTuner::writeSysfs(path_buffer, fifo_bt);
        }
    }

    void applySysctlTunables(bool is_high_performance) {
        // vfs_cache_pressure: 100 saat performa tinggi, 150 saat efisiensi
        const std::string cache_pressure = is_high_performance ? "100" : "150";
        SafeTuner::writeSysfs("/proc/sys/vm/vfs_cache_pressure", cache_pressure);

        // Dirty ratios: longgarkan ke 20% & 30% saat performa tinggi untuk cegah I/O stutter
        if (is_high_performance) {
            SafeTuner::writeSysfs("/proc/sys/vm/dirty_background_ratio", "20");
            SafeTuner::writeSysfs("/proc/sys/vm/dirty_ratio", "30");
        } else {
            SafeTuner::writeSysfs("/proc/sys/vm/dirty_background_ratio", "10");
            SafeTuner::writeSysfs("/proc/sys/vm/dirty_ratio", "20");
        }
    }

    // ============================================================
    // INITIALIZE HARDWARE PROFILE
    // ============================================================
    void initializeHardwareProfile() {
        if (CoreFlowState::is_initialized) {
            return;
        }

        std::cout << "[CoreFlow AI] Menjalankan pemindaian arsitektur hardware..." << std::endl;

        // --------------------------------------------------------
        // Detect board
        // --------------------------------------------------------
        char board_char[PROP_VALUE_MAX];
        int len = __system_property_get("ro.product.board", board_char);
        std::string board_name = (len > 0) ? std::string(board_char) : "";
        
        if (!board_name.empty() && board_name.back() == '\n') {
            board_name.pop_back();
        }

        if (board_name == "parrot" || board_name == "Parrot") {
            std::cout << "[CoreFlow AI] Target Terverifikasi: Redmi Note 15 5G (Kunzite/Parrot)." << std::endl;
            std::cout << "[CoreFlow AI] Mengaktifkan Kunzite Privilege Profile (Snapdragon 6 Gen 3)." << std::endl;
        } else {
            std::cout << "[CoreFlow AI] Berjalan di perangkat Universal. Mengaktifkan deteksi adaptif." << std::endl;
        }

        // --------------------------------------------------------
        // Capture stock I/O configuration
        // --------------------------------------------------------
        std::string raw_read_ahead;
        std::string raw_nr;
        SafeTuner::readSysfs("/sys/block/sda/queue/read_ahead_kb", raw_read_ahead);
        SafeTuner::readSysfs("/sys/block/sda/queue/nr_requests", raw_nr);

        if (!raw_read_ahead.empty()) {
            StockProfile::read_ahead_kb = std::stoul(raw_read_ahead);
        }
        if (!raw_nr.empty()) {
            StockProfile::nr_requests = std::stoul(raw_nr);
        }

        // --------------------------------------------------------
        // Capture stock CPU governors
        // --------------------------------------------------------
        SafeTuner::readSysfs("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", StockProfile::cpu_policy0_gov);
        SafeTuner::readSysfs("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", StockProfile::cpu_policy4_gov);

        // --------------------------------------------------------
        // Priming Read CPU Counters (Mencegah Lonjakan Jiffies Pertama)
        // --------------------------------------------------------
        std::ifstream stat_file("/proc/stat");
        if (stat_file.is_open()) {
            std::string line, cpu;
            std::getline(stat_file, line);
            stat_file.close();
            
            std::stringstream ss(line);
            ss >> cpu >> CoreFlowState::prev_user >> CoreFlowState::prev_nice 
               >> CoreFlowState::prev_system >> CoreFlowState::prev_idle;
        }

        // --------------------------------------------------------
        // Khusus SDA: Paksa I/O Scheduler ke mq-deadline
        // --------------------------------------------------------
        SafeTuner::writeSysfs("/sys/block/sda/queue/scheduler", "mq-deadline");

        // --------------------------------------------------------
        // PENGATURAN MUTLAK (Static Tunables) untuk SDA - SDF
        // - rq_affinity = 2 (Optimal CPU cache locality)
        // - iostats = 0 (Memangkas overhead kernel)
        // --------------------------------------------------------
        char path_buffer[64];
        for (char blk = 'a'; blk <= 'f'; ++blk) {
            snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/rq_affinity", blk);
            SafeTuner::writeSysfs(path_buffer, "2");

            snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iostats", blk);
            SafeTuner::writeSysfs(path_buffer, "0");
        }

        // --------------------------------------------------------
        // PENGATURAN MUTLAK SYSCTL (Static Memory Tunables)
        // --------------------------------------------------------
        SafeTuner::writeSysfs("/proc/sys/vm/max_map_count", "65530");
        SafeTuner::writeSysfs("/proc/sys/vm/page-cluster", "3");

        // --------------------------------------------------------
        // Apply stock baseline to block devices (SDA - SDF)
        // --------------------------------------------------------
        const std::string stock_ra = std::to_string(StockProfile::read_ahead_kb);
        const std::string stock_nr = std::to_string(StockProfile::nr_requests);

        for (char blk = 'a'; blk <= 'f'; ++blk) {
            snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
            SafeTuner::writeSysfs(path_buffer, stock_ra);
            snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/nr_requests", blk);
            SafeTuner::writeSysfs(path_buffer, stock_nr);
        }

        CoreFlowState::is_initialized = true;
        std::cout << "[CoreFlow AI] Inisialisasi framework & baseline selesai." << std::endl;
    }

    // ============================================================
    // DYNAMIC LOAD EVALUATION
    // ============================================================
    void evaluateDynamicLoad(AppClass foreground_app) {
        // --------------------------------------------------------
        // Read CPU statistics
        // --------------------------------------------------------
        std::ifstream file("/proc/stat");
        if (!file.is_open()) {
            return;
        }

        std::string line;
        std::string cpu;
        std::getline(file, line);
        file.close();

        unsigned long long user = 0, nice = 0, system = 0, idle = 0;
        unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0;
        std::stringstream ss(line);
        ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

        // --------------------------------------------------------
        // Calculate CPU usage
        // --------------------------------------------------------
        const unsigned long long prev_total = CoreFlowState::prev_user + CoreFlowState::prev_nice + 
                                              CoreFlowState::prev_system + CoreFlowState::prev_idle;
        const unsigned long long current_total = user + nice + system + idle;
        const unsigned long long total_diff = current_total - prev_total;
        const unsigned long long idle_diff = idle - CoreFlowState::prev_idle;

        double cpu_usage = 0.0;
        if (total_diff > 0) {
            cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff;
        }

        // Update previous counters
        CoreFlowState::prev_user = user;
        CoreFlowState::prev_nice = nice;
        CoreFlowState::prev_system = system;
        CoreFlowState::prev_idle = idle;

        // --------------------------------------------------------
        // Read GPU busy percentage
        // --------------------------------------------------------
        int gpu_busy = 0;
        std::string raw_gpu_busy;
        SafeTuner::readSysfs("/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage", raw_gpu_busy);
        if (!raw_gpu_busy.empty()) {
            gpu_busy = std::stoi(raw_gpu_busy);
        }

        // --------------------------------------------------------
        // Determine target engine mode
        // --------------------------------------------------------
        CoreFlowState::EngineMode target_mode = CoreFlowState::MODE_BALANCED;

        if (foreground_app == APP_GAME || gpu_busy > 45 || cpu_usage > 75.0) {
            target_mode = CoreFlowState::MODE_GAMING;
        } else if (foreground_app == APP_LAUNCHER || (cpu_usage < 15.0 && gpu_busy < 8)) {
            target_mode = CoreFlowState::MODE_IDLE;
        } else if (cpu_usage >= 25.0 && cpu_usage <= 75.0) {
            target_mode = CoreFlowState::MODE_BURST;
        }

        // --------------------------------------------------------
        // Apply mode only when state changes
        // --------------------------------------------------------
        if (target_mode == CoreFlowState::current_mode) {
            return;
        }
        CoreFlowState::current_mode = target_mode;

        char path_buffer[64];

        // ========================================================
        // GAMING MODE
        // ========================================================
        if (CoreFlowState::current_mode == CoreFlowState::MODE_GAMING) {
            std::cout << "[CoreFlow AI] Pilar 3 Aktif: Gaming Performance Unleashed." << std::endl;

            for (char blk = 'a'; blk <= 'f'; ++blk) {
                snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
                SafeTuner::writeSysfs(path_buffer, "2048");
                snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/nr_requests", blk);
                SafeTuner::writeSysfs(path_buffer, "256");
            }

            // Terapkan Advanced Tunables & Sysctl High-Performance
            applyMqDeadlineTunables(true);
            applySysctlTunables(true);

            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "60");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "55");
            SafeTuner::writeSysfs("/sys/class/kgsl/kgsl-3d0/devfreq/governor", "msm-adreno-tz");
            __system_property_set("vendor.dsp.default_qos", "1");
        } 
        // ========================================================
        // BURST MODE
        // ========================================================
        else if (CoreFlowState::current_mode == CoreFlowState::MODE_BURST) {
            std::cout << "[CoreFlow AI] Pilar 2 Aktif: App Launch / Tango Burst." << std::endl;

            for (char blk = 'a'; blk <= 'f'; ++blk) {
                snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
                SafeTuner::writeSysfs(path_buffer, "1024");
                snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/nr_requests", blk);
                SafeTuner::writeSysfs(path_buffer, "128");
            }

            // Terapkan Advanced Tunables & Sysctl High-Performance
            applyMqDeadlineTunables(true);
            applySysctlTunables(true);

            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "70");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "70");
        } 
        // ========================================================
        // BALANCED MODE
        // ========================================================
        else if (CoreFlowState::current_mode == CoreFlowState::MODE_BALANCED) {
            std::cout << "[CoreFlow AI] Pilar 1 Aktif: Daily Balanced State." << std::endl;

            const std::string stock_ra = std::to_string(StockProfile::read_ahead_kb);
            const std::string stock_nr = std::to_string(StockProfile::nr_requests);

            for (char blk = 'a'; blk <= 'f'; ++blk) {
                snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
                SafeTuner::writeSysfs(path_buffer, stock_ra);
                snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/nr_requests", blk);
                SafeTuner::writeSysfs(path_buffer, stock_nr);
            }

            // Kembalikan Advanced Tunables & Sysctl ke profil Balanced
            applyMqDeadlineTunables(false);
            applySysctlTunables(false);

            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "80");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "80");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", StockProfile::cpu_policy0_gov);
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", StockProfile::cpu_policy4_gov);
            __system_property_set("vendor.dsp.default_qos", "1");
        } 
        // ========================================================
        // IDLE MODE
        // ========================================================
        else if (CoreFlowState::current_mode == CoreFlowState::MODE_IDLE) {
            std::cout << "[CoreFlow AI] Pilar 1 Aktif: Daily Efficiency (Layar On Idle)." << std::endl;

            const std::string stock_ra = std::to_string(StockProfile::read_ahead_kb);

            for (char blk = 'a'; blk <= 'f'; ++blk) {
                snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
                SafeTuner::writeSysfs(path_buffer, stock_ra);
            }

            // Kembalikan Advanced Tunables & Sysctl ke profil Efficiency
            applyMqDeadlineTunables(false);
            applySysctlTunables(false);

            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "85");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "85");
        }
    }

    // ============================================================
    // ULTRA IDLE / SCREEN OFF MODE
    // ============================================================
    void setUltraIdleMode() {
        std::cout << "[CoreFlow AI] Layar Mati. Mengaktifkan Ultra Deep Sleep & NPU Off." << std::endl;

        SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", "powersave");

        char path_buffer[64];
        for (char blk = 'a'; blk <= 'f'; ++blk) {
            snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
            SafeTuner::writeSysfs(path_buffer, "256");
        }

        // Set efisiensi maksimal saat deep sleep
        applyMqDeadlineTunables(false);
        applySysctlTunables(false);

        __system_property_set("vendor.dsp.default_qos", "0");
        CoreFlowState::current_mode = CoreFlowState::MODE_IDLE;
    }

    // ============================================================
    // FOREGROUND APP DETECTION (Dengan Lazy Evaluation)
    // ============================================================
    AppClass detectForegroundApp() {
        int gpu_busy = 0;
        std::string raw_gpu_busy;
        SafeTuner::readSysfs("/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage", raw_gpu_busy);
        if (!raw_gpu_busy.empty()) {
            gpu_busy = std::stoi(raw_gpu_busy);
        }

        if (gpu_busy < 15 && CoreFlowState::current_mode != CoreFlowState::MODE_GAMING && 
            CoreFlowState::current_mode != CoreFlowState::MODE_BURST) {
            return APP_DEFAULT;
        }

        DIR* dir = opendir("/proc");
        if (!dir) {
            return APP_DEFAULT;
        }

        struct dirent* entry;
        AppClass detected_class = APP_DEFAULT;

        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type != DT_DIR) {
                continue;
            }

            bool is_digit = true;
            for (int i = 0; entry->d_name[i] != '\0'; ++i) {
                if (entry->d_name[i] < '0' || entry->d_name[i] > '9') {
                    is_digit = false;
                    break;
                }
            }

            if (!is_digit) {
                continue;
            }

            char cmdpath_buffer[32];
            snprintf(cmdpath_buffer, sizeof(cmdpath_buffer), "/proc/%s/cmdline", entry->d_name);
            
            std::ifstream cmd_file(cmdpath_buffer);
            if (!cmd_file.is_open()) {
                continue;
            }

            std::string cmdline;
            std::getline(cmd_file, cmdline);
            cmd_file.close();

            if (cmdline.empty()) {
                continue;
            }

            if (cmdline.find("com.tencent") != std::string::npos || 
                cmdline.find("com.dts.freefiremax") != std::string::npos || 
                cmdline.find("mobilelegends") != std::string::npos || 
                cmdline.find("genshin") != std::string::npos || 
                cmdline.find("game") != std::string::npos) {
                detected_class = APP_GAME;
                break;
            }

            if (cmdline.find("miui.home") != std::string::npos || 
                cmdline.find("launcher") != std::string::npos || 
                cmdline.find("hyperos") != std::string::npos) {
                detected_class = APP_LAUNCHER;
            }
        }

        closedir(dir);
        return detected_class;
    }

} // namespace CoreFlowAI
