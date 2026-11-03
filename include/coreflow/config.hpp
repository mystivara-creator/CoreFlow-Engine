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
#include <cstdlib>
#include <cerrno>

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

    // [stability_policy] - lightweight global pressure gates, not device paths
    float cpu_pressure_boost_pct{8.0f};
    float memory_pressure_guard_pct{3.0f};
    float io_pressure_guard_pct{5.0f};
    float thermal_headroom_celsius{3.0f};
    float thermal_emergency_margin_celsius{4.0f};

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

    static uint32_t parse_uint(const std::string& text, uint32_t fallback) {
        uint32_t result = 0;
        const char* first = text.data();
        const char* last = first + text.size();
        const auto parsed = std::from_chars(first, last, result);
        return parsed.ec == std::errc{} && parsed.ptr == last ? result : fallback;
    }

    static float parse_float(const std::string& text, float fallback) {
        if (text.empty()) return fallback;
        char* end = nullptr;
        errno = 0;
        float val = std::strtof(text.c_str(), &end);
        return errno == 0 && end == text.c_str() + text.size() && std::isfinite(val) ? val : fallback;
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
            else if (key == "top_app_interactive_min") cfg.top_app_interactive_min = parse_uint(val, cfg.top_app_interactive_min);
            else if (key == "top_app_interactive_max") cfg.top_app_interactive_max = parse_uint(val, cfg.top_app_interactive_max);
            else if (key == "top_app_burst_launch_min") cfg.top_app_burst_launch_min = parse_uint(val, cfg.top_app_burst_launch_min);
            else if (key == "top_app_game_min") cfg.top_app_game_min = parse_uint(val, cfg.top_app_game_min);
            else if (key == "top_app_game_max") cfg.top_app_game_max = parse_uint(val, cfg.top_app_game_max);
            else if (key == "foreground_min") cfg.foreground_min = parse_uint(val, cfg.foreground_min);
            else if (key == "background_max") cfg.background_max = parse_uint(val, cfg.background_max);
            else if (key == "sched_util_clamp_min_rt_default") cfg.sched_util_clamp_min_rt_default = parse_uint(val, cfg.sched_util_clamp_min_rt_default);

            // [thermal_guardian]
            else if (key == "temp_monitor_interval_ms") cfg.temp_monitor_interval_ms = parse_uint(val, cfg.temp_monitor_interval_ms);
            else if (key == "thermal_trip_celsius") cfg.thermal_trip_celsius = parse_float(val, cfg.thermal_trip_celsius);
            else if (key == "thermal_recover_celsius") cfg.thermal_recover_celsius = parse_float(val, cfg.thermal_recover_celsius);
            else if (key == "emergency_rollback_on_overheat") cfg.emergency_rollback_on_overheat = parse_bool(val);

            // [stability_policy]
            else if (key == "cpu_pressure_boost_pct") cfg.cpu_pressure_boost_pct = parse_float(val, cfg.cpu_pressure_boost_pct);
            else if (key == "memory_pressure_guard_pct") cfg.memory_pressure_guard_pct = parse_float(val, cfg.memory_pressure_guard_pct);
            else if (key == "io_pressure_guard_pct") cfg.io_pressure_guard_pct = parse_float(val, cfg.io_pressure_guard_pct);
            else if (key == "thermal_headroom_celsius") cfg.thermal_headroom_celsius = parse_float(val, cfg.thermal_headroom_celsius);
            else if (key == "thermal_emergency_margin_celsius") cfg.thermal_emergency_margin_celsius = parse_float(val, cfg.thermal_emergency_margin_celsius);

            // [render_pipeline]
            else if (key == "jank_detection_threshold_ratio") cfg.jank_detection_threshold_ratio = parse_float(val, cfg.jank_detection_threshold_ratio);
            else if (key == "burst_boost_duration_ms") cfg.burst_boost_duration_ms = parse_uint(val, cfg.burst_boost_duration_ms);
            else if (key == "touch_boost_uclamp_min") cfg.touch_boost_uclamp_min = parse_uint(val, cfg.touch_boost_uclamp_min);
        }

        cfg.temp_monitor_interval_ms = std::clamp(cfg.temp_monitor_interval_ms, 1000u, 60000u);
        cfg.thermal_trip_celsius = std::clamp(cfg.thermal_trip_celsius, 35.0f, 65.0f);
        cfg.thermal_recover_celsius = std::clamp(cfg.thermal_recover_celsius, 25.0f, cfg.thermal_trip_celsius - 1.0f);
        cfg.top_app_interactive_min = std::min(cfg.top_app_interactive_min, 1024u);
        cfg.top_app_interactive_max = std::clamp(cfg.top_app_interactive_max, cfg.top_app_interactive_min, 1024u);
        cfg.cpu_pressure_boost_pct = std::clamp(cfg.cpu_pressure_boost_pct, 1.0f, 50.0f);
        cfg.memory_pressure_guard_pct = std::clamp(cfg.memory_pressure_guard_pct, 0.5f, 30.0f);
        cfg.io_pressure_guard_pct = std::clamp(cfg.io_pressure_guard_pct, 0.5f, 30.0f);
        cfg.thermal_headroom_celsius = std::clamp(cfg.thermal_headroom_celsius, 1.0f, 10.0f);
        cfg.thermal_emergency_margin_celsius = std::clamp(cfg.thermal_emergency_margin_celsius, 2.0f, 10.0f);
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
