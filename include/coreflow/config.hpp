/**
 * @file config.hpp
 * @brief Zero-hardcode runtime configuration loader for CoreFlow Engine
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include "types.hpp"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace coreflow {

struct DaemonConfig {
    // [general]
    std::string version{"6.0.0"};
    std::string profile{"balanced"}; // balanced | smoothness | battery | gaming
    std::string log_level{"info"};
    bool zero_polling_mode{true};
    bool enable_cgroups_v2{true};
    bool enable_uclamp{true};
    // Normal daemon operation is autonomous; read-only inspection is an explicit CLI mode.
    bool inspection_mode{false};
    bool enable_tuner{true};

    // [features] CoreFlow v5/v6 4-pillar feature toggles
    bool enable_frame_smoothness{true};
    bool enable_storage_burst{true};
    bool enable_memory_guard{true};
    bool enable_thermal_pacing{true};

    // [power] CoreFlow v6 CFJ4-protected power & bypass charging controls
    bool bypass_charging{false};
    bool bypass_on_gaming{false};
    uint32_t charge_limit_pct{0}; // 0 = off, 60..95 = active charge ceiling

    // [apps] Optional per-app overrides (comma-separated Android package names)
    std::string gaming_apps{};
    std::string smoothness_apps{};
    std::string battery_apps{};
    std::string excluded_apps{};

    // [direct_adaptive] Legacy compatibility fields
    bool enable_direct_adaptive{true};
    std::string direct_adaptive_policy{"auto"};

    // [device_adaptation]
    std::string tier_override{"auto"};
    bool unlock_vendor_clamping{false};
    bool force_120hz_rendering{false};
    bool enable_cpu_frequency_control{true};
    bool enable_gpu_frequency_control{true};

    // [uclamp_tuning]
    uint32_t top_app_interactive_min{320};
    uint32_t top_app_interactive_max{1024};
    uint32_t top_app_burst_launch_min{800};
    uint32_t top_app_game_min{512};
    uint32_t top_app_game_max{980};
    uint32_t foreground_min{60};
    uint32_t background_max{250};
    uint32_t sched_util_clamp_min_rt_default{128};
    std::string ui_adaptive_bias{"balanced"};
    uint32_t ui_balanced_uclamp_min{320};

    // [thermal_guardian]
    uint32_t temp_monitor_interval_ms{1000};
    // Fallback Celsius thresholds used ONLY when OEM thermal policy (Android Thermal API & sysfs trip points) is unavailable.
    float thermal_trip_celsius{44.0f};
    float thermal_recover_celsius{40.0f};
    float thermal_emergency_margin_celsius{5.0f};
    float thermal_headroom_celsius{3.0f};
    double memory_pressure_guard_pct{15.0};
    double io_pressure_guard_pct{15.0};
    double cpu_pressure_boost_pct{25.0};
    bool emergency_rollback_on_overheat{true};

    // [pid_equilibrium]
    bool enable_pid_equilibrium{true};
    float pid_target_celsius{42.5f};
    float pid_kp{2.5f};
    float pid_ki{0.15f};
    float pid_kd{1.2f};

    // [hardware_longevity]
    bool enable_battery_step_charging{true};
    bool enable_ufs_burst_pacing{true};
    float battery_step_temp_threshold{41.5f};
    uint32_t stepped_charge_current_ma{1500};
    uint32_t top_app_io_weight{800};
    uint32_t background_io_weight{50};

    // [render_pipeline]
    float jank_detection_threshold_ratio{1.25f};
    uint32_t burst_boost_duration_ms{120};
    uint32_t touch_boost_uclamp_min{380};

    static std::vector<std::string> parse_package_list(std::string_view raw) {
        std::vector<std::string> out;
        size_t pos = 0;
        while (pos < raw.size() && out.size() < 64U) {
            const size_t comma = raw.find(',', pos);
            std::string tok = trim(std::string(
                raw.substr(pos, comma == std::string_view::npos ? raw.size() - pos : comma - pos)));
            bool valid = !tok.empty() && tok.size() <= 128U;
            for (const unsigned char ch : tok) {
                if (!std::isalnum(ch) && ch != '.' && ch != '_' && ch != '-') {
                    valid = false;
                    break;
                }
            }
            if (valid && std::find(out.begin(), out.end(), tok) == out.end()) {
                out.push_back(std::move(tok));
            }
            if (comma == std::string_view::npos) break;
            pos = comma + 1U;
        }
        return out;
    }

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::string to_lower_ascii(std::string_view str) {
        std::string out(str);
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return out;
    }

    static bool parse_bool(const std::string& val) {
        std::string lower = to_lower_ascii(val);
        return lower == "true" || lower == "1" || lower == "yes";
    }

    static bool parse_float_safe(std::string_view text, float& out) noexcept {
        double d = 0.0;
        if (!parse_ascii_double(text, d) || !std::isfinite(d)) return false;
        if (std::abs(d) > std::numeric_limits<float>::max()) return false;
        out = static_cast<float>(d);
        return true;
    }

    static bool parse_double_safe(std::string_view text, double& out) noexcept {
        double d = 0.0;
        if (!parse_ascii_double(text, d) || !std::isfinite(d)) return false;
        out = d;
        return true;
    }

    static bool parse_uint32_safe(std::string_view text, uint32_t& out) noexcept {
        double d = 0.0;
        if (!parse_ascii_double(text, d) || !std::isfinite(d) || d < 0.0 || d > 4294967295.0 || std::floor(d) != d) return false;
        out = static_cast<uint32_t>(d);
        return true;
    }

    static std::string canonical_section_for_key(std::string_view key) {
        if (key == "version" || key == "profile" || key == "inspection_mode" ||
            key == "log_level" || key == "zero_polling_mode" ||
            key == "enable_cgroups_v2" || key == "enable_uclamp" ||
            key == "enable_tuner") {
            return "general";
        }
        if (key == "enable_frame_smoothness" || key == "enable_storage_burst" ||
            key == "enable_memory_guard" || key == "enable_thermal_pacing") {
            return "features";
        }
        if (key == "bypass_charging" || key == "bypass_on_gaming" ||
            key == "charge_limit_pct") {
            return "power";
        }
        if (key == "gaming_apps" || key == "smoothness_apps" ||
            key == "battery_apps" || key == "excluded_apps") {
            return "apps";
        }
        if (key == "enable_direct_adaptive" || key == "direct_adaptive_policy") {
            return "direct_adaptive";
        }
        if (key == "tier_override" || key == "unlock_vendor_clamping" ||
            key == "force_120hz_rendering" || key == "enable_cpu_frequency_control" ||
            key == "enable_gpu_frequency_control") {
            return "device_adaptation";
        }
        if (key == "top_app_interactive_min" || key == "top_app_interactive_max" ||
            key == "top_app_burst_launch_min" || key == "top_app_game_min" ||
            key == "top_app_game_max" || key == "foreground_min" ||
            key == "background_max" || key == "sched_util_clamp_min_rt_default" ||
            key == "ui_adaptive_bias" || key == "ui_balanced_uclamp_min") {
            return "uclamp_tuning";
        }
        if (key == "temp_monitor_interval_ms" || key == "thermal_trip_celsius" ||
            key == "thermal_recover_celsius" || key == "emergency_rollback_on_overheat") {
            return "thermal_guardian";
        }
        if (key == "cpu_pressure_boost_pct" || key == "memory_pressure_guard_pct" ||
            key == "io_pressure_guard_pct" || key == "thermal_headroom_celsius" ||
            key == "thermal_emergency_margin_celsius") {
            return "stability_policy";
        }
        if (key == "enable_pid_equilibrium" || key == "pid_target_celsius" ||
            key == "pid_kp" || key == "pid_ki" || key == "pid_kd") {
            return "pid_equilibrium";
        }
        if (key == "enable_battery_step_charging" || key == "enable_ufs_burst_pacing" ||
            key == "battery_step_temp_threshold" || key == "stepped_charge_current_ma" ||
            key == "top_app_io_weight" || key == "background_io_weight") {
            return "hardware_longevity";
        }
        if (key == "jank_detection_threshold_ratio" || key == "burst_boost_duration_ms" ||
            key == "touch_boost_uclamp_min") {
            return "render_pipeline";
        }
        return "";
    }

    static bool section_accepts_key(std::string_view section, std::string_view key) {
        const std::string expected = canonical_section_for_key(key);
        if (expected.empty()) return false;
        if (section == expected) return true;
        // Accept [features] alias for enable_battery_step_charging
        if (key == "enable_battery_step_charging" && section == "features") return true;
        // Accept legacy [stability] and [thermal_guardian] aliases for stability_policy keys
        if (expected == "stability_policy" &&
            (section == "stability" || section == "thermal_guardian")) {
            return true;
        }
        return false;
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
                current_section = to_lower_ascii(trim(line.substr(1, line.size() - 2)));
                continue;
            }

            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) continue;

            std::string key = trim(line.substr(0, eq_pos));
            std::string val = trim(line.substr(eq_pos + 1));

            // Section-aware validation: reject known keys placed in an unrelated section
            if (!section_accepts_key(current_section, key)) {
                continue;
            }

            // [general]
            if (key == "version") cfg.version = val;
            else if (key == "profile") {
                const auto p = to_lower_ascii(val);
                if (p == "balanced" || p == "smoothness" || p == "battery" || p == "gaming") {
                    cfg.profile = p;
                }
            }
            else if (key == "inspection_mode") cfg.inspection_mode = parse_bool(val);
            else if (key == "log_level") cfg.log_level = val;
            else if (key == "zero_polling_mode") cfg.zero_polling_mode = parse_bool(val);
            else if (key == "enable_cgroups_v2") cfg.enable_cgroups_v2 = parse_bool(val);
            else if (key == "enable_uclamp") cfg.enable_uclamp = parse_bool(val);
            else if (key == "enable_tuner") cfg.enable_tuner = parse_bool(val);

            // [features]
            else if (key == "enable_frame_smoothness") cfg.enable_frame_smoothness = parse_bool(val);
            else if (key == "enable_storage_burst") cfg.enable_storage_burst = parse_bool(val);
            else if (key == "enable_memory_guard") cfg.enable_memory_guard = parse_bool(val);
            else if (key == "enable_thermal_pacing") cfg.enable_thermal_pacing = parse_bool(val);

            // [apps]
            else if (key == "gaming_apps" || key == "smoothness_apps" ||
                     key == "battery_apps" || key == "excluded_apps") {
                const auto pkgs = parse_package_list(val);
                std::string joined;
                for (size_t i = 0; i < pkgs.size(); ++i) {
                    if (i != 0U) joined.push_back(',');
                    joined += pkgs[i];
                }
                if (key == "gaming_apps") cfg.gaming_apps = std::move(joined);
                else if (key == "smoothness_apps") cfg.smoothness_apps = std::move(joined);
                else if (key == "battery_apps") cfg.battery_apps = std::move(joined);
                else cfg.excluded_apps = std::move(joined);
            }

            // [device_adaptation]
            else if (key == "tier_override") cfg.tier_override = val;
            else if (key == "unlock_vendor_clamping") cfg.unlock_vendor_clamping = parse_bool(val);
            else if (key == "force_120hz_rendering") cfg.force_120hz_rendering = parse_bool(val);
            else if (key == "enable_cpu_frequency_control") cfg.enable_cpu_frequency_control = parse_bool(val);
            else if (key == "enable_gpu_frequency_control") cfg.enable_gpu_frequency_control = parse_bool(val);

            // [direct_adaptive]
            else if (key == "enable_direct_adaptive") cfg.enable_direct_adaptive = parse_bool(val);
            else if (key == "direct_adaptive_policy") {
                const auto policy = to_lower_ascii(val);
                cfg.direct_adaptive_policy =
                    (policy == "auto" || policy == "disabled") ? policy : "auto";
            }

            // [uclamp_tuning]
            else if (key == "top_app_interactive_min") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.top_app_interactive_min = std::clamp(v, 0u, 1024u);
            }
            else if (key == "top_app_interactive_max") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.top_app_interactive_max = std::clamp(v, 0u, 1024u);
            }
            else if (key == "top_app_burst_launch_min") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.top_app_burst_launch_min = std::clamp(v, 0u, 1024u);
            }
            else if (key == "top_app_game_min") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.top_app_game_min = std::clamp(v, 0u, 1024u);
            }
            else if (key == "top_app_game_max") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.top_app_game_max = std::clamp(v, 0u, 1024u);
            }
            else if (key == "foreground_min") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.foreground_min = std::clamp(v, 0u, 1024u);
            }
            else if (key == "background_max") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.background_max = std::clamp(v, 0u, 1024u);
            }
            else if (key == "sched_util_clamp_min_rt_default") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.sched_util_clamp_min_rt_default = std::clamp(v, 0u, 1024u);
            }
            else if (key == "ui_adaptive_bias") {
                if (val == "battery" || val == "balanced" || val == "responsiveness") {
                    cfg.ui_adaptive_bias = val;
                }
            }
            else if (key == "ui_balanced_uclamp_min") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.ui_balanced_uclamp_min = std::clamp(v, 0u, 1024u);
            }

            // [thermal_guardian]
            else if (key == "temp_monitor_interval_ms") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.temp_monitor_interval_ms = std::clamp(v, 1000u, 60000u);
                else cfg.temp_monitor_interval_ms = 1000u;
            }
            else if (key == "thermal_trip_celsius") {
                float v = 0.0f;
                if (parse_float_safe(val, v) && v >= 30.0f && v <= 80.0f) cfg.thermal_trip_celsius = v;
            }
            else if (key == "thermal_recover_celsius") {
                float v = 0.0f;
                if (parse_float_safe(val, v) && v >= 25.0f && v <= 75.0f) cfg.thermal_recover_celsius = v;
            }
            else if (key == "emergency_rollback_on_overheat") cfg.emergency_rollback_on_overheat = parse_bool(val);

            // [stability_policy]
            else if (key == "cpu_pressure_boost_pct") {
                double v = 0.0;
                if (parse_double_safe(val, v)) cfg.cpu_pressure_boost_pct = std::clamp(v, 0.5, 50.0);
            }
            else if (key == "memory_pressure_guard_pct") {
                double v = 0.0;
                if (parse_double_safe(val, v)) cfg.memory_pressure_guard_pct = std::clamp(v, 0.5, 50.0);
            }
            else if (key == "io_pressure_guard_pct") {
                double v = 0.0;
                if (parse_double_safe(val, v)) cfg.io_pressure_guard_pct = std::clamp(v, 0.5, 50.0);
                else cfg.io_pressure_guard_pct = 5.0;
            }
            else if (key == "thermal_headroom_celsius") {
                float v = 0.0f;
                if (parse_float_safe(val, v)) cfg.thermal_headroom_celsius = std::clamp(v, 0.5f, 10.0f);
            }
            else if (key == "thermal_emergency_margin_celsius") {
                float v = 0.0f;
                if (parse_float_safe(val, v)) cfg.thermal_emergency_margin_celsius = std::clamp(v, 2.0f, 15.0f);
            }

            // [pid_equilibrium]
            else if (key == "enable_pid_equilibrium") cfg.enable_pid_equilibrium = parse_bool(val);
            else if (key == "pid_target_celsius") {
                float v = 0.0f;
                if (parse_float_safe(val, v)) cfg.pid_target_celsius = v;
            }
            else if (key == "pid_kp") {
                float v = 0.0f;
                if (parse_float_safe(val, v)) cfg.pid_kp = v;
            }
            else if (key == "pid_ki") {
                float v = 0.0f;
                if (parse_float_safe(val, v)) cfg.pid_ki = v;
            }
            else if (key == "pid_kd") {
                float v = 0.0f;
                if (parse_float_safe(val, v)) cfg.pid_kd = v;
            }

            // [hardware_longevity]
            else if (key == "enable_battery_step_charging") cfg.enable_battery_step_charging = parse_bool(val);
            else if (key == "enable_ufs_burst_pacing") cfg.enable_ufs_burst_pacing = parse_bool(val);
            else if (key == "battery_step_temp_threshold") {
                float v = 0.0f;
                if (parse_float_safe(val, v)) cfg.battery_step_temp_threshold = v;
            }
            else if (key == "stepped_charge_current_ma") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.stepped_charge_current_ma = v;
            }
            else if (key == "top_app_io_weight") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.top_app_io_weight = v;
            }
            else if (key == "background_io_weight") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.background_io_weight = v;
            }

            // [power]
            else if (key == "bypass_charging") cfg.bypass_charging = parse_bool(val);
            else if (key == "bypass_on_gaming") cfg.bypass_on_gaming = parse_bool(val);
            else if (key == "charge_limit_pct") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) {
                    cfg.charge_limit_pct = (v >= 60u && v <= 95u) ? v : 0u;
                }
            }

            // [render_pipeline]
            else if (key == "jank_detection_threshold_ratio") {
                float v = 0.0f;
                if (parse_float_safe(val, v)) cfg.jank_detection_threshold_ratio = v;
            }
            else if (key == "burst_boost_duration_ms") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.burst_boost_duration_ms = v;
            }
            else if (key == "touch_boost_uclamp_min") {
                uint32_t v = 0;
                if (parse_uint32_safe(val, v)) cfg.touch_boost_uclamp_min = v;
            }
        }

        // Post-validation: ensure max >= min and bounded render pipeline thresholds
        cfg.pid_target_celsius = std::clamp(cfg.pid_target_celsius, 25.0f, cfg.thermal_trip_celsius);
        cfg.pid_kp = std::clamp(cfg.pid_kp, 0.0f, 20.0f);
        cfg.pid_ki = std::clamp(cfg.pid_ki, 0.0f, 20.0f);
        cfg.pid_kd = std::clamp(cfg.pid_kd, 0.0f, 20.0f);
        cfg.stepped_charge_current_ma = std::clamp(cfg.stepped_charge_current_ma, 100u, 5000u);
        cfg.battery_step_temp_threshold = std::clamp(cfg.battery_step_temp_threshold, 30.0f, 45.0f);
        cfg.top_app_io_weight = std::clamp(cfg.top_app_io_weight, 1u, 10000u);
        cfg.background_io_weight = std::clamp(cfg.background_io_weight, 1u, 10000u);
        cfg.jank_detection_threshold_ratio = std::clamp(cfg.jank_detection_threshold_ratio, 1.0f, 5.0f);
        cfg.burst_boost_duration_ms = std::clamp(cfg.burst_boost_duration_ms, 50u, 1000u);
        cfg.touch_boost_uclamp_min = std::clamp(cfg.touch_boost_uclamp_min, 0u, 1024u);
        cfg.top_app_interactive_max = std::max(cfg.top_app_interactive_max, cfg.top_app_interactive_min);
        cfg.top_app_game_max = std::max(cfg.top_app_game_max, cfg.top_app_game_min);
        if (cfg.thermal_recover_celsius >= cfg.thermal_trip_celsius) {
            cfg.thermal_recover_celsius = cfg.thermal_trip_celsius - 4.0f;
        }

        return cfg;
    }

    static bool is_safe_config_value(std::string_view value, std::string_view key = {}) noexcept {
        const bool is_app_list = (key == "gaming_apps" || key == "smoothness_apps" ||
                                  key == "battery_apps" || key == "excluded_apps");
        if (is_app_list) {
            if (value.size() > 2048U) return false;
            for (const unsigned char ch : value) {
                if (!std::isalnum(ch) && ch != '.' && ch != '_' && ch != '-' && ch != ',' && ch != ' ') {
                    return false;
                }
            }
            return true;
        }
        if (value.empty() || value.size() > 256U) return false;
        for (const unsigned char ch : value) {
            if (ch == '\n' || ch == '\r' || ch == '\0' || ch < 0x20U) return false;
        }
        return true;
    }

    // Surgical read-modify-write helper that updates specified keys in their canonical section
    // while preserving comments, formatting, and unknown keys/sections.
    static std::string merge_config_text(
        std::string_view existing_text,
        const std::vector<std::pair<std::string, std::string>>& updates) {
        std::vector<std::pair<std::string, std::string>> safe_updates;
        safe_updates.reserve(updates.size());
        for (const auto& [k, v] : updates) {
            if (!canonical_section_for_key(k).empty() && is_safe_config_value(v, k)) {
                safe_updates.emplace_back(k, v);
            }
        }
        std::vector<bool> applied(safe_updates.size(), false);
        std::istringstream in{std::string(existing_text)};
        std::ostringstream out;
        std::string raw_line;
        std::string current_section;

        auto flush_pending_for_section = [&](std::string_view section) {
            if (section.empty()) return;
            for (size_t i = 0; i < safe_updates.size(); ++i) {
                if (applied[i]) continue;
                const std::string target_sec = canonical_section_for_key(safe_updates[i].first);
                const bool matches = (section == target_sec) ||
                                     (target_sec == "stability_policy" && section == "stability");
                if (matches) {
                    out << safe_updates[i].first << " = " << safe_updates[i].second << "\n";
                    applied[i] = true;
                }
            }
        };

        while (std::getline(in, raw_line)) {
            if (!raw_line.empty() && raw_line.back() == '\r') raw_line.pop_back();
            const std::string trimmed = trim(raw_line);
            if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
                flush_pending_for_section(current_section);
                current_section = to_lower_ascii(trim(trimmed.substr(1, trimmed.size() - 2)));
                out << raw_line << "\n";
                continue;
            }
            if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
                out << raw_line << "\n";
                continue;
            }
            const size_t eq_pos = trimmed.find('=');
            if (eq_pos == std::string::npos) {
                out << raw_line << "\n";
                continue;
            }
            const std::string key = trim(trimmed.substr(0, eq_pos));
            bool replaced = false;
            for (size_t i = 0; i < safe_updates.size(); ++i) {
                if (safe_updates[i].first == key && section_accepts_key(current_section, key)) {
                    if (!applied[i]) {
                        out << key << " = " << safe_updates[i].second << "\n";
                        applied[i] = true;
                    }
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                out << raw_line << "\n";
            }
        }
        flush_pending_for_section(current_section);

        // Append any sections that were not present in existing_text
        for (size_t i = 0; i < safe_updates.size(); ++i) {
            if (applied[i]) continue;
            const std::string sec = canonical_section_for_key(safe_updates[i].first);
            if (sec.empty()) continue;
            out << "\n[" << sec << "]\n";
            for (size_t j = i; j < safe_updates.size(); ++j) {
                if (!applied[j] && canonical_section_for_key(safe_updates[j].first) == sec) {
                    out << safe_updates[j].first << " = " << safe_updates[j].second << "\n";
                    applied[j] = true;
                }
            }
        }
        return out.str();
    }

    static bool update_config_file(
        const std::string& path,
        const std::vector<std::pair<std::string, std::string>>& updates) {
        if (path.empty() || updates.empty()) return false;
        for (const auto& [k, v] : updates) {
            if (canonical_section_for_key(k).empty() || !is_safe_config_value(v, k)) {
                return false;
            }
        }
        struct stat target_st{};
        if (::lstat(path.c_str(), &target_st) == 0) {
            if (!S_ISREG(target_st.st_mode) || S_ISLNK(target_st.st_mode)) return false;
        } else if (errno != ENOENT) {
            return false;
        }

        std::string existing;
        {
            const int in_fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
            if (in_fd >= 0) {
                struct stat st{};
                if (::fstat(in_fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_size >= 0 && st.st_size <= 262144) {
                    existing.reserve(static_cast<size_t>(st.st_size));
                    char buf[4096];
                    for (;;) {
                        const ssize_t n = ::read(in_fd, buf, sizeof(buf));
                        if (n < 0 && errno == EINTR) continue;
                        if (n <= 0) break;
                        if (existing.size() + static_cast<size_t>(n) > 262144U) {
                            (void)::close(in_fd);
                            return false;
                        }
                        existing.append(buf, static_cast<size_t>(n));
                    }
                }
                (void)::close(in_fd);
            }
        }
        const std::string merged = merge_config_text(existing, updates);
        static std::atomic<uint64_t> tmp_seq{0U};
        const uint64_t seq = tmp_seq.fetch_add(1U, std::memory_order_relaxed) + 1U;
        const std::string tmp_path = path + ".tmp." + std::to_string(::getpid()) + "." + std::to_string(seq);
        const int out_fd = ::open(
            tmp_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (out_fd < 0) return false;

        bool ok = true;
        size_t offset = 0U;
        while (offset < merged.size()) {
            const ssize_t n = ::write(out_fd, merged.data() + offset, merged.size() - offset);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) { ok = false; break; }
            offset += static_cast<size_t>(n);
        }
        if (ok) ok = (::fchmod(out_fd, 0600) == 0) && (::fsync(out_fd) == 0);
        if (::close(out_fd) != 0) ok = false;
        if (!ok || ::rename(tmp_path.c_str(), path.c_str()) != 0) {
            (void)::unlink(tmp_path.c_str());
            return false;
        }
        return true;
    }

    static DaemonConfig load_best_available(bool announce = true) {
        const std::vector<std::string> paths = {
            "/data/adb/coreflow/coreflow.conf",
            "/data/adb/modules/cfae_framework/coreflow.conf",
            "/data/adb/modules/cfae_framework/system/etc/coreflow/default.conf",
            "/data/adb/modules/coreflow_engine/coreflow.conf",
            "/system/etc/coreflow/default.conf",
            "config/coreflow.conf"
        };
        for (const auto& path : paths) {
            std::ifstream f(path);
            if (f.good()) {
                if (announce) std::cout << "[Config] Loaded configuration dynamically from: " << path << std::endl;
                return load_from_file(path);
            }
        }
        if (announce) std::cout << "[Config] Using default in-memory configuration." << std::endl;
        return DaemonConfig{};
    }
};

} // namespace coreflow

