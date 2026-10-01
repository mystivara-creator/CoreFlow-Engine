#include "../include/coreflow.hpp"
#include <android/log.h>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/system_properties.h>
#include <unistd.h>
#include <vector>

namespace {
    constexpr const char* kTag = "CoreFlow AI";
    constexpr const char* kLogTag = "CoreFlowEngine";

    bool parseUInt(const std::string& input, unsigned int& out, unsigned int fallback = 0) {
        if (input.empty()) {
            out = fallback;
            return false;
        }
        for (char c : input) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                out = fallback;
                return false;
            }
        }
        char* end = nullptr;
        errno = 0;
        unsigned long val = std::strtoul(input.c_str(), &end, 10);
        if (errno == ERANGE || val > static_cast<unsigned long>(UINT_MAX) || end == input.c_str() || *end != '\0') {
            out = fallback;
            return false;
        }
        out = static_cast<unsigned int>(val);
        return true;
    }

    bool parseInt(const std::string& input, int& out, int fallback = 0) {
        if (input.empty()) {
            out = fallback;
            return false;
        }
        for (char c : input) {
            if (!std::isdigit(static_cast<unsigned char>(c)) && !std::isspace(static_cast<unsigned char>(c))) {
                out = fallback;
                return false;
            }
        }
        char* end = nullptr;
        errno = 0;
        long val = std::strtol(input.c_str(), &end, 10);
        if (errno == ERANGE || val > static_cast<long>(INT_MAX) || end == input.c_str() || *end != '\0') {
            out = fallback;
            return false;
        }
        out = static_cast<int>(val);
        return true;
    }

    bool pathExists(const std::string& path) {
        return access(path.c_str(), F_OK) == 0;
    }

    unsigned int clampu(unsigned int val, unsigned int lo, unsigned int hi) {
        if (val < lo) return lo;
        if (val > hi) return hi;
        return val;
    }

    unsigned int percentOf(unsigned int base, unsigned int pct) {
        return static_cast<unsigned int>((static_cast<unsigned long long>(base) * pct) / 100ULL);
    }
}

namespace StockProfile {
    unsigned int read_ahead_kb = 512;
    unsigned int nr_requests = 128;
    std::string cpu_policy0_gov = "schedutil";
    std::string cpu_policy4_gov = "schedutil";
    CoreFlowAI::GovernorTunables policy0_governor;
    CoreFlowAI::GovernorTunables policy4_governor;
}

namespace CoreFlowState {
    enum EngineMode { MODE_IDLE, MODE_BALANCED, MODE_BURST, MODE_GAMING };
    EngineMode current_mode = MODE_BALANCED;
    bool is_initialized = false;

    unsigned long long prev_user = 0;
    unsigned long long prev_nice = 0;
    unsigned long long prev_system = 0;
    unsigned long long prev_idle = 0;
}

namespace {
    using CoreFlowState::EngineMode;
    using CoreFlowState::MODE_BALANCED;
    using CoreFlowState::MODE_BURST;
    using CoreFlowState::MODE_GAMING;
    using CoreFlowState::MODE_IDLE;
}

namespace SafeTuner {
    void writeSysfs(const std::string& path, const std::string& value) {
        std::ofstream file(path);
        if (!file.is_open()) {
            return;
        }
        file << value << std::flush;
        file.close();
    }

    void readSysfs(const std::string& path, std::string& output) {
        output.clear();
        std::ifstream file(path);
        if (!file.is_open()) {
            return;
        }
        file >> output;
        file.close();
    }
}

namespace CoreFlowAI {

    void readCpuFreqBounds(int policy, GovernorTunables& out) {
        std::string min_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(policy) + "/cpuinfo_min_freq";
        std::string max_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(policy) + "/cpuinfo_max_freq";
        std::string raw_min, raw_max;
        SafeTuner::readSysfs(min_path, raw_min);
        SafeTuner::readSysfs(max_path, raw_max);
        parseUInt(raw_min, out.min_freq, out.min_freq);
        parseUInt(raw_max, out.max_freq, out.max_freq);
    }

    bool readGovernorTunables(int policy, GovernorTunables& out) {
        std::string base = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(policy) + "/";
        std::string raw;
        out.valid = false;

        SafeTuner::readSysfs(base + "hispeed_freq", raw);
        parseUInt(raw, out.hispeed_freq, 0);

        SafeTuner::readSysfs(base + "hispeed_load", raw);
        parseUInt(raw, out.hispeed_load, 85);

        SafeTuner::readSysfs(base + "rtg_boost_freq", raw);
        parseUInt(raw, out.rtg_boost_freq, 0);

        SafeTuner::readSysfs(base + "up_rate_limit_us", raw);
        parseUInt(raw, out.up_rate_limit_us, 0);

        SafeTuner::readSysfs(base + "down_rate_limit_us", raw);
        parseUInt(raw, out.down_rate_limit_us, 0);

        readCpuFreqBounds(policy, out);
        out.valid = (out.max_freq > 0);
        return out.valid;
    }

    void applyGovernorTunables(int policy, const GovernorTunables& t) {
        std::string base = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(policy) + "/";
        SafeTuner::writeSysfs(base + "hispeed_freq", std::to_string(t.hispeed_freq));
        SafeTuner::writeSysfs(base + "hispeed_load", std::to_string(t.hispeed_load));
        SafeTuner::writeSysfs(base + "rtg_boost_freq", std::to_string(t.rtg_boost_freq));
        SafeTuner::writeSysfs(base + "up_rate_limit_us", std::to_string(t.up_rate_limit_us));
        SafeTuner::writeSysfs(base + "down_rate_limit_us", std::to_string(t.down_rate_limit_us));
    }

    // Sweet-spot tunables: more responsive governor, not overclocking.
    GovernorTunables buildModeProfile(const GovernorTunables& stock, EngineMode mode, bool is_big_cluster) {
        GovernorTunables t = stock;
        if (!stock.valid) return t;

        unsigned int rtg_base = (stock.rtg_boost_freq > 0) ? stock.rtg_boost_freq : stock.max_freq;

        if (is_big_cluster) {
            switch (mode) {
                case MODE_GAMING:
                    t.hispeed_load = clampu(percentOf(stock.hispeed_load, 75), 45, stock.hispeed_load);
                    t.hispeed_freq = clampu(percentOf(stock.hispeed_freq, 105), stock.hispeed_freq, stock.max_freq);
                    t.rtg_boost_freq = clampu(percentOf(rtg_base, 110), stock.hispeed_freq, stock.max_freq);
                    t.up_rate_limit_us = (stock.up_rate_limit_us > 500) ? (stock.up_rate_limit_us / 2) : 250;
                    t.down_rate_limit_us = clampu(percentOf(stock.down_rate_limit_us, 120), 2000, 200000);
                    break;

                case MODE_BURST:
                    t.hispeed_load = clampu(percentOf(stock.hispeed_load, 82), 55, stock.hispeed_load);
                    t.hispeed_freq = clampu(percentOf(stock.hispeed_freq, 102), stock.hispeed_freq, stock.max_freq);
                    t.rtg_boost_freq = clampu(percentOf(rtg_base, 105), stock.hispeed_freq, stock.max_freq);
                    t.up_rate_limit_us = (stock.up_rate_limit_us > 500) ? static_cast<unsigned int>((stock.up_rate_limit_us * 2) / 3) : 350;
                    t.down_rate_limit_us = clampu(percentOf(stock.down_rate_limit_us, 110), 2000, 150000);
                    break;

                default:
                    t = stock;
                    break;
            }
        } else {
            switch (mode) {
                case MODE_GAMING:
                    t.hispeed_load = clampu(percentOf(stock.hispeed_load, 80), 55, stock.hispeed_load);
                    t.hispeed_freq = clampu(percentOf(stock.hispeed_freq, 103), stock.hispeed_freq, stock.max_freq);
                    t.rtg_boost_freq = clampu(percentOf(rtg_base, 105), stock.hispeed_freq, stock.max_freq);
                    t.up_rate_limit_us = (stock.up_rate_limit_us > 500) ? (stock.up_rate_limit_us / 2) : 300;
                    t.down_rate_limit_us = stock.down_rate_limit_us;
                    break;

                case MODE_BURST:
                    t.hispeed_load = clampu(percentOf(stock.hispeed_load, 85), 60, stock.hispeed_load);
                    t.hispeed_freq = clampu(percentOf(stock.hispeed_freq, 101), stock.hispeed_freq, stock.max_freq);
                    t.rtg_boost_freq = clampu(percentOf(rtg_base, 103), stock.hispeed_freq, stock.max_freq);
                    t.up_rate_limit_us = (stock.up_rate_limit_us > 500) ? static_cast<unsigned int>((stock.up_rate_limit_us * 2) / 3) : 400;
                    t.down_rate_limit_us = stock.down_rate_limit_us;
                    break;

                default:
                    t = stock;
                    break;
            }
        }

        t.hispeed_freq = clampu(t.hispeed_freq, stock.min_freq, stock.max_freq);
        t.rtg_boost_freq = clampu(t.rtg_boost_freq, stock.min_freq, stock.max_freq);

        return t;
    }

    void applyCpuGovernorProfile(EngineMode mode) {
        if (StockProfile::policy0_governor.valid) {
            GovernorTunables profile = buildModeProfile(StockProfile::policy0_governor, mode, false);
            applyGovernorTunables(0, profile);
            __android_log_print(ANDROID_LOG_DEBUG, kLogTag,
                "[CoreFlow CPU] policy0 mode=%d hispeed=%u load=%u up=%u down=%u",
                static_cast<int>(mode), profile.hispeed_freq, profile.hispeed_load,
                profile.up_rate_limit_us, profile.down_rate_limit_us);
        }

        if (StockProfile::policy4_governor.valid) {
            GovernorTunables profile = buildModeProfile(StockProfile::policy4_governor, mode, true);
            applyGovernorTunables(4, profile);
            __android_log_print(ANDROID_LOG_DEBUG, kLogTag,
                "[CoreFlow CPU] policy4 mode=%d hispeed=%u load=%u up=%u down=%u",
                static_cast<int>(mode), profile.hispeed_freq, profile.hispeed_load,
                profile.up_rate_limit_us, profile.down_rate_limit_us);
        }
    }

    void applyMqDeadlineTunables(bool is_high_performance) {
        char path_buffer[128];
        const std::string read_exp = is_high_performance ? "250" : "500";
        const std::string write_exp = is_high_performance ? "2500" : "5000";
        const std::string async_dp = is_high_performance ? "128" : "64";
        const std::string fifo_bt = "16";

        for (char blk = 'a'; blk <= 'f'; ++blk) {
            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c", blk);
            if (!pathExists(path_buffer)) continue;

            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iosched/read_expire", blk);
            SafeTuner::writeSysfs(path_buffer, read_exp);
            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iosched/write_expire", blk);
            SafeTuner::writeSysfs(path_buffer, write_exp);
            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iosched/async_depth", blk);
            SafeTuner::writeSysfs(path_buffer, async_dp);
            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iosched/fifo_batch", blk);
            SafeTuner::writeSysfs(path_buffer, fifo_bt);
        }
    }

    void applySysctlTunables(bool is_high_performance) {
        const std::string cache_pressure = is_high_performance ? "100" : "150";
        SafeTuner::writeSysfs("/proc/sys/vm/vfs_cache_pressure", cache_pressure);

        if (is_high_performance) {
            SafeTuner::writeSysfs("/proc/sys/vm/dirty_background_ratio", "20");
            SafeTuner::writeSysfs("/proc/sys/vm/dirty_ratio", "30");
        } else {
            SafeTuner::writeSysfs("/proc/sys/vm/dirty_background_ratio", "10");
            SafeTuner::writeSysfs("/proc/sys/vm/dirty_ratio", "20");
        }
    }

    void initializeHardwareProfile() {
        if (CoreFlowState::is_initialized) {
            return;
        }

        std::cout << "[" << kTag << "] Menjalankan pemindaian arsitektur hardware..." << std::endl;

        char board_char[PROP_VALUE_MAX] = {0};
        int len = __system_property_get("ro.product.board", board_char);
        std::string board_name = (len > 0) ? std::string(board_char, static_cast<size_t>(len)) : "";

        if (board_name == "parrot" || board_name == "Parrot") {
            std::cout << "[" << kTag << "] Target Terverifikasi: Redmi Note 15 5G (Kunzite/Parrot)." << std::endl;
            std::cout << "[" << kTag << "] Mengaktifkan Kunzite Privilege Profile (Snapdragon 6 Gen 3)." << std::endl;
        } else {
            std::cout << "[" << kTag << "] Berjalan di perangkat Universal. Mengaktifkan deteksi adaptif." << std::endl;
        }

        std::string raw_read_ahead;
        std::string raw_nr;
        SafeTuner::readSysfs("/sys/block/sda/queue/read_ahead_kb", raw_read_ahead);
        SafeTuner::readSysfs("/sys/block/sda/queue/nr_requests", raw_nr);

        parseUInt(raw_read_ahead, StockProfile::read_ahead_kb, StockProfile::read_ahead_kb);
        parseUInt(raw_nr, StockProfile::nr_requests, StockProfile::nr_requests);

        SafeTuner::readSysfs("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", StockProfile::cpu_policy0_gov);
        SafeTuner::readSysfs("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", StockProfile::cpu_policy4_gov);

        if (StockProfile::cpu_policy0_gov.empty()) StockProfile::cpu_policy0_gov = "schedutil";
        if (StockProfile::cpu_policy4_gov.empty()) StockProfile::cpu_policy4_gov = "schedutil";

        readGovernorTunables(0, StockProfile::policy0_governor);
        readGovernorTunables(4, StockProfile::policy4_governor);

        std::ifstream stat_file("/proc/stat");
        if (stat_file.is_open()) {
            std::string line;
            std::string cpu;
            if (std::getline(stat_file, line)) {
                std::stringstream ss(line);
                ss >> cpu >> CoreFlowState::prev_user >> CoreFlowState::prev_nice
                   >> CoreFlowState::prev_system >> CoreFlowState::prev_idle;
            }
            stat_file.close();
        }

        if (pathExists("/sys/block/sda")) {
            SafeTuner::writeSysfs("/sys/block/sda/queue/scheduler", "mq-deadline");
        }

        char path_buffer[64];
        for (char blk = 'a'; blk <= 'f'; ++blk) {
            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c", blk);
            if (!pathExists(path_buffer)) continue;

            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/rq_affinity", blk);
            SafeTuner::writeSysfs(path_buffer, "2");
            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/iostats", blk);
            SafeTuner::writeSysfs(path_buffer, "0");
        }

        SafeTuner::writeSysfs("/proc/sys/vm/max_map_count", "65530");
        SafeTuner::writeSysfs("/proc/sys/vm/page-cluster", "3");

        const std::string stock_ra = std::to_string(StockProfile::read_ahead_kb);
        const std::string stock_nr = std::to_string(StockProfile::nr_requests);

        for (char blk = 'a'; blk <= 'f'; ++blk) {
            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c", blk);
            if (!pathExists(path_buffer)) continue;

            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
            SafeTuner::writeSysfs(path_buffer, stock_ra);
            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/nr_requests", blk);
            SafeTuner::writeSysfs(path_buffer, stock_nr);
        }

        CoreFlowState::is_initialized = true;
        std::cout << "[" << kTag << "] Inisialisasi framework & baseline selesai." << std::endl;
        __android_log_print(ANDROID_LOG_DEBUG, kLogTag,
            "Stock policy0 hispeed=%u load=%u max=%u",
            StockProfile::policy0_governor.hispeed_freq,
            StockProfile::policy0_governor.hispeed_load,
            StockProfile::policy0_governor.max_freq);
        __android_log_print(ANDROID_LOG_DEBUG, kLogTag,
            "Stock policy4 hispeed=%u load=%u max=%u",
            StockProfile::policy4_governor.hispeed_freq,
            StockProfile::policy4_governor.hispeed_load,
            StockProfile::policy4_governor.max_freq);
    }

    void evaluateDynamicLoad(AppClass foreground_app) {
        std::ifstream file("/proc/stat");
        if (!file.is_open()) {
            return;
        }

        std::string line;
        std::string cpu;
        if (!std::getline(file, line)) {
            file.close();
            return;
        }
        file.close();

        unsigned long long user = 0, nice = 0, system = 0, idle = 0;
        unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0;
        std::stringstream ss(line);
        if (!(ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal)) {
            return;
        }
        if (cpu != "cpu") {
            return;
        }

        const unsigned long long prev_total =
            CoreFlowState::prev_user + CoreFlowState::prev_nice +
            CoreFlowState::prev_system + CoreFlowState::prev_idle;
        const unsigned long long current_total = user + nice + system + idle;
        const unsigned long long total_diff =
            (current_total > prev_total) ? (current_total - prev_total) : 0;
        const unsigned long long idle_diff =
            (idle > CoreFlowState::prev_idle) ? (idle - CoreFlowState::prev_idle) : 0;

        double cpu_usage = 0.0;
        if (total_diff > 0) {
            cpu_usage = 100.0 * static_cast<double>(total_diff - idle_diff) /
                        static_cast<double>(total_diff);
        }

        CoreFlowState::prev_user = user;
        CoreFlowState::prev_nice = nice;
        CoreFlowState::prev_system = system;
        CoreFlowState::prev_idle = idle;

        int gpu_busy = 0;
        std::string raw_gpu_busy;
        SafeTuner::readSysfs("/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage", raw_gpu_busy);
        parseInt(raw_gpu_busy, gpu_busy, 0);

        EngineMode target_mode = MODE_BALANCED;

        if (foreground_app == APP_GAME || gpu_busy > 45 || cpu_usage > 75.0) {
            target_mode = MODE_GAMING;
        } else if (foreground_app == APP_LAUNCHER || (cpu_usage < 15.0 && gpu_busy < 8)) {
            target_mode = MODE_IDLE;
        } else if (cpu_usage >= 25.0 && cpu_usage <= 75.0) {
            target_mode = MODE_BURST;
        }

        if (target_mode == CoreFlowState::current_mode) {
            return;
        }
        CoreFlowState::current_mode = target_mode;

        char path_buffer[64];

        if (CoreFlowState::current_mode == MODE_GAMING) {
            std::cout << "[" << kTag << "] Pilar 3 Aktif: Gaming Responsiveness." << std::endl;

            for (char blk = 'a'; blk <= 'f'; ++blk) {
                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c", blk);
                if (!pathExists(path_buffer)) continue;

                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
                SafeTuner::writeSysfs(path_buffer, "2048");
                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/nr_requests", blk);
                SafeTuner::writeSysfs(path_buffer, "256");
            }

            applyMqDeadlineTunables(true);
            applySysctlTunables(true);
            applyCpuGovernorProfile(MODE_GAMING);

            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "70");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "65");
            __system_property_set("vendor.dsp.default_qos", "1");
        }
        else if (CoreFlowState::current_mode == MODE_BURST) {
            std::cout << "[" << kTag << "] Pilar 2 Aktif: App Launch Responsiveness." << std::endl;

            for (char blk = 'a'; blk <= 'f'; ++blk) {
                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c", blk);
                if (!pathExists(path_buffer)) continue;

                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
                SafeTuner::writeSysfs(path_buffer, "1024");
                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/nr_requests", blk);
                SafeTuner::writeSysfs(path_buffer, "128");
            }

            applyMqDeadlineTunables(true);
            applySysctlTunables(true);
            applyCpuGovernorProfile(MODE_BURST);

            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "78");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "75");
        }
        else if (CoreFlowState::current_mode == MODE_BALANCED) {
            std::cout << "[" << kTag << "] Pilar 1 Aktif: Daily Balanced State." << std::endl;

            const std::string stock_ra = std::to_string(StockProfile::read_ahead_kb);
            const std::string stock_nr = std::to_string(StockProfile::nr_requests);

            for (char blk = 'a'; blk <= 'f'; ++blk) {
                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c", blk);
                if (!pathExists(path_buffer)) continue;

                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
                SafeTuner::writeSysfs(path_buffer, stock_ra);
                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/nr_requests", blk);
                SafeTuner::writeSysfs(path_buffer, stock_nr);
            }

            applyMqDeadlineTunables(false);
            applySysctlTunables(false);
            applyCpuGovernorProfile(MODE_BALANCED);

            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "80");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "80");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor", StockProfile::cpu_policy0_gov);
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", StockProfile::cpu_policy4_gov);
            __system_property_set("vendor.dsp.default_qos", "1");
        }
        else if (CoreFlowState::current_mode == MODE_IDLE) {
            std::cout << "[" << kTag << "] Pilar 1 Aktif: Daily Efficiency (Layar On Idle)." << std::endl;

            const std::string stock_ra = std::to_string(StockProfile::read_ahead_kb);

            for (char blk = 'a'; blk <= 'f'; ++blk) {
                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c", blk);
                if (!pathExists(path_buffer)) continue;

                std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
                SafeTuner::writeSysfs(path_buffer, stock_ra);
            }

            applyMqDeadlineTunables(false);
            applySysctlTunables(false);
            applyCpuGovernorProfile(MODE_IDLE);

            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy0/walt_target_load", "85");
            SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/walt_target_load", "85");
        }
    }

    void setUltraIdleMode() {
        std::cout << "[" << kTag << "] Layar Mati. Mengaktifkan Ultra Deep Sleep & NPU Off." << std::endl;

        SafeTuner::writeSysfs("/sys/devices/system/cpu/cpufreq/policy4/scaling_governor", "powersave");

        char path_buffer[64];
        for (char blk = 'a'; blk <= 'f'; ++blk) {
            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c", blk);
            if (!pathExists(path_buffer)) continue;

            std::snprintf(path_buffer, sizeof(path_buffer), "/sys/block/sd%c/queue/read_ahead_kb", blk);
            SafeTuner::writeSysfs(path_buffer, "256");
        }

        applyMqDeadlineTunables(false);
        applySysctlTunables(false);
        applyCpuGovernorProfile(MODE_IDLE);

        __system_property_set("vendor.dsp.default_qos", "0");
        CoreFlowState::current_mode = MODE_IDLE;
    }

    AppClass detectForegroundApp() {
        int gpu_busy = 0;
        std::string raw_gpu_busy;
        SafeTuner::readSysfs("/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage", raw_gpu_busy);
        parseInt(raw_gpu_busy, gpu_busy, 0);

        if (gpu_busy < 15 && CoreFlowState::current_mode != MODE_GAMING &&
            CoreFlowState::current_mode != MODE_BURST) {
            return APP_DEFAULT;
        }

        FILE* dumpsys_pipe = popen("dumpsys activity top 2>/dev/null | head -n 40", "r");
        if (dumpsys_pipe) {
            char buffer[512];
            std::string output;
            while (fgets(buffer, sizeof(buffer), dumpsys_pipe) != nullptr) {
                output.append(buffer);
            }
            pclose(dumpsys_pipe);

            static const std::vector<std::string> kGamePatterns = {
                "com.tencent",
                "com.dts.freefire",
                "com.mobile.legends",
                "com.miHoYo.GenshinImpact",
                "com.activision.callofduty.shooter",
                "com.garena.game",
                "com.supercell",
                "com.ea.game",
                "com.blizzard.arc",
                "com.riotgames.league",
                "com.netease",
                "com.krafton",
                "com.pubg",
                "com.dts.freefiremax"
            };

            static const std::vector<std::string> kLauncherPatterns = {
                "com.miui.home",
                "com.android.launcher",
                "launcher",
                "com.hyperos"
            };

            for (const auto& pattern : kGamePatterns) {
                if (output.find(pattern) != std::string::npos) {
                    return APP_GAME;
                }
            }
            for (const auto& pattern : kLauncherPatterns) {
                if (output.find(pattern) != std::string::npos) {
                    return APP_LAUNCHER;
                }
            }
        }

        DIR* dir = opendir("/proc");
        if (!dir) {
            return APP_DEFAULT;
        }

        struct dirent* entry;
        AppClass detected_class = APP_DEFAULT;

        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type != DT_DIR) continue;

            bool is_digit = true;
            for (int i = 0; entry->d_name[i] != '\0'; ++i) {
                if (entry->d_name[i] < '0' || entry->d_name[i] > '9') {
                    is_digit = false;
                    break;
                }
            }
            if (!is_digit) continue;

            char cmdpath_buffer[64];
            std::snprintf(cmdpath_buffer, sizeof(cmdpath_buffer), "/proc/%s/cmdline", entry->d_name);

            std::ifstream cmd_file(cmdpath_buffer);
            if (!cmd_file.is_open()) continue;

            std::string cmdline(256, '\0');
            if (!cmd_file.read(&cmdline[0], cmdline.size())) {
                cmd_file.close();
                continue;
            }
            std::streamsize bytes = cmd_file.gcount();
            cmd_file.close();
            cmdline.resize(static_cast<size_t>(bytes));

            for (auto it = cmdline.begin(); it != cmdline.end(); ++it) {
                if (*it == '\0') *it = ' ';
            }
            std::string::size_type pos;
            while ((pos = cmdline.find("  ")) != std::string::npos) {
                cmdline.erase(pos, 1);
            }
            while (!cmdline.empty() && std::isspace(static_cast<unsigned char>(cmdline.back()))) {
                cmdline.pop_back();
            }

            if (cmdline.empty()) continue;

            static const char* kGameSubstrings[] = {
                "com.tencent",
                "com.dts.freefiremax",
                "com.dts.freefireth",
                "com.mobile.legends",
                "com.miHoYo.GenshinImpact",
                "com.activision.callofduty.shooter",
                "com.pubg",
                "com.krafton"
            };
            static const char* kLauncherSubstrings[] = {
                "miui.home",
                "com.android.launcher",
                "launcher",
                "hyperos"
            };

            for (const char* pattern : kGameSubstrings) {
                if (cmdline.find(pattern) != std::string::npos) {
                    closedir(dir);
                    return APP_GAME;
                }
            }
            for (const char* pattern : kLauncherSubstrings) {
                if (cmdline.find(pattern) != std::string::npos) {
                    detected_class = APP_LAUNCHER;
                }
            }
        }

        closedir(dir);
        return detected_class;
    }
}