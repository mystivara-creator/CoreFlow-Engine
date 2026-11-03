/**
 * @file config.hpp
 * @brief Zero-hardcode runtime configuration loader for CoreFlow Engine
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include "types.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <charconv>
#include <cmath>
#include <vector>

namespace coreflow {

struct DaemonConfig {
    // [general]
    std::string version{"2.0.0"};
    std::string log_level{"info"};
    bool zero_polling_mode{true};
    bool enable_cgroups_v2{true};
    bool enable_uclamp{true};

    // [device_adaptation]
    std::string tier_override{"auto"};
    bool unlock_vendor_clamping{false};
    bool force_120hz_rendering{false};

    // [uclamp_tuning]
    uint32_t top_app_interactive_min{320};
    uint32_t top_app_interactive_max{1024};
    uint32_t top_app_burst_launch_min{800};
    uint32_t top_app_game_min{512};
    uint32_t top_app_game_max{980};
    uint32_t foreground_min{60};
    uint32_t background_max{250};
    uint32_t sched_util_clamp_min_rt_default{128};

    // [thermal_guardian]
    uint32_t temp_monitor_interval_ms{1000};
    float thermal_trip_celsius{44.0f};
    float thermal_recover_celsius{40.0f};
    bool emergency_rollback_on_overheat{true};

    // [render_pipeline]
    float jank_detection_threshold_ratio{1.25f};
    uint32_t burst_boost_duration_ms{120};
    uint32_t touch_boost_uclamp_min{380};

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static bool parse_bool(const std::string& val) {
        std::string lower = val;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower == "true" || lower == "1" || lower == "yes";
    }

    static DaemonConfig load_from_file(const std::string& path) {
        DaemonConfig cfg;
        std::ifstream file(path);
        if (!file.is_open()) {
            return cfg;
        }

        std::string line;
        std::string current_section = "";

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;

            if (line.front() == '[' && line.back() == ']') {
                current_section = line.substr(1, line.size() - 2);
                continue;
            }

            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) continue;

            std::string key = trim(line.substr(0, eq_pos));
            std::string val = trim(line.substr(eq_pos + 1));

            // [general]
            if (key == "version") cfg.version = val;
            else if (key == "log_level") cfg.log_level = val;
            else if (key == "zero_polling_mode") cfg.zero_polling_mode = parse_bool(val);
            else if (key == "enable_cgroups_v2") cfg.enable_cgroups_v2 = parse_bool(val);
            else if (key == "enable_uclamp") cfg.enable_uclamp = parse_bool(val);

            // [device_adaptation]
            else if (key == "tier_override") cfg.tier_override = val;
            else if (key == "unlock_vendor_clamping") cfg.unlock_vendor_clamping = parse_bool(val);
            else if (key == "force_120hz_rendering") cfg.force_120hz_rendering = parse_bool(val);

            // [uclamp_tuning]
            else if (key == "top_app_interactive_min") cfg.top_app_interactive_min = std::stoul(val);
            else if (key == "top_app_interactive_max") cfg.top_app_interactive_max = std::stoul(val);
            else if (key == "top_app_burst_launch_min") cfg.top_app_burst_launch_min = std::stoul(val);
            else if (key == "top_app_game_min") cfg.top_app_game_min = std::stoul(val);
            else if (key == "top_app_game_max") cfg.top_app_game_max = std::stoul(val);
            else if (key == "foreground_min") cfg.foreground_min = std::stoul(val);
            else if (key == "background_max") cfg.background_max = std::stoul(val);
            else if (key == "sched_util_clamp_min_rt_default") cfg.sched_util_clamp_min_rt_default = std::stoul(val);

            // [thermal_guardian]
            else if (key == "temp_monitor_interval_ms") cfg.temp_monitor_interval_ms = std::stoul(val);
            else if (key == "thermal_trip_celsius") cfg.thermal_trip_celsius = std::stof(val);
            else if (key == "thermal_recover_celsius") cfg.thermal_recover_celsius = std::stof(val);
            else if (key == "emergency_rollback_on_overheat") cfg.emergency_rollback_on_overheat = parse_bool(val);

            // [render_pipeline]
            else if (key == "jank_detection_threshold_ratio") cfg.jank_detection_threshold_ratio = std::stof(val);
            else if (key == "burst_boost_duration_ms") cfg.burst_boost_duration_ms = std::stoul(val);
            else if (key == "touch_boost_uclamp_min") cfg.touch_boost_uclamp_min = std::stoul(val);
        }

        return cfg;
    }

    static DaemonConfig load_best_available() {
        const std::vector<std::string> paths = {
            "/data/adb/coreflow/coreflow.conf",
            "/data/adb/modules/coreflow_engine/coreflow.conf",
            "/system/etc/coreflow/default.conf",
            "config/coreflow.conf"
        };
        for (const auto& path : paths) {
            std::ifstream f(path);
            if (f.good()) {
                std::cout << "[Config] Loaded configuration dynamically from: " << path << std::endl;
                return load_from_file(path);
            }
        }
        std::cout << "[Config] Using default in-memory configuration." << std::endl;
        return DaemonConfig{};
    }
};

} // namespace coreflow
