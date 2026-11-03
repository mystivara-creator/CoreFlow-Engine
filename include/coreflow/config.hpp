/**
 * @file config.hpp
 * @brief Hardened fail-safe runtime policy configuration for CoreFlow Modern v2
 * @author mystivara-creator
 * @license MIT
 */
#pragma once

#include "unique_fd.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace coreflow {

/// Locale-independent ASCII decimal parser; immune to LC_NUMERIC comma locales.
[[nodiscard]] inline bool parse_ascii_double(std::string_view text, double& out) noexcept {
    if (text.empty() || text.size() > 64) return false;
    size_t i = 0;
    bool negative = false;
    if (text[i] == '+' || text[i] == '-') {
        negative = (text[i] == '-');
        ++i;
        if (i == text.size()) return false;
    }
    double value = 0.0;
    bool has_digits = false;
    while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
        has_digits = true;
        value = value * 10.0 + static_cast<double>(text[i] - '0');
        if (!std::isfinite(value)) return false;
        ++i;
    }
    if (i < text.size() && text[i] == '.') {
        ++i;
        double scale = 0.1;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
            has_digits = true;
            value += static_cast<double>(text[i] - '0') * scale;
            scale *= 0.1;
            ++i;
        }
    }
    if (!has_digits || i != text.size() || !std::isfinite(value)) return false;
    out = negative ? -value : value;
    return true;
}

struct DaemonConfig {
    // Runtime activation.
    std::string version{"2.0.0"};
    std::string log_level{"info"};
    bool enable_uclamp{true};

    // Scheduler baseline. Values use the kernel SCHED_CAPACITY_SCALE (0..1024)
    // internally and are encoded for the discovered interface at write time.
    uint32_t top_app_interactive_min{320};
    uint32_t top_app_interactive_max{1024};

    // Thermal safety.
    uint32_t temp_monitor_interval_ms{1000};
    float thermal_trip_celsius{44.0f};
    float thermal_recover_celsius{40.0f};
    bool emergency_rollback_on_overheat{true};

    // Lightweight stability gates based on global PSI avg10.
    float cpu_pressure_boost_pct{8.0f};
    float memory_pressure_guard_pct{3.0f};
    float io_pressure_guard_pct{5.0f};
    float thermal_headroom_celsius{3.0f};
    float thermal_emergency_margin_celsius{4.0f};

    static std::string trim(std::string_view value) {
        const size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos) return {};
        const size_t last = value.find_last_not_of(" \t\r\n");
        return std::string(value.substr(first, last - first + 1));
    }

    static bool parse_bool(const std::string& text, bool fallback) {
        std::string lower = text;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (lower == "true" || lower == "1" || lower == "yes") return true;
        if (lower == "false" || lower == "0" || lower == "no") return false;
        return fallback;
    }

    static uint32_t parse_uint(const std::string& text, uint32_t fallback) {
        if (text.empty()) return fallback;
        uint32_t result = 0;
        const char* first = text.data();
        const char* last = first + text.size();
        const auto parsed = std::from_chars(first, last, result);
        return parsed.ec == std::errc{} && parsed.ptr == last ? result : fallback;
    }

    static float parse_float(const std::string& text, float fallback) {
        double parsed = 0.0;
        if (!parse_ascii_double(text, parsed)) return fallback;
        const float value = static_cast<float>(parsed);
        return std::isfinite(value) ? value : fallback;
    }

    static bool read_secure_file(const std::string& path, bool enforce_root_owner, std::string& content) {
        content.clear();
        if (path.empty() || path.find('\0') != std::string::npos) return false;
        UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
        if (!fd.valid()) return false;

        struct stat st{};
        if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
        if (st.st_size < 0 || st.st_size > 65536) return false;
        if (enforce_root_owner) {
            if (st.st_uid != 0 || (st.st_mode & (S_IWGRP | S_IWOTH)) != 0) {
                std::cerr << "[CoreFlow][Config] Rejecting insecure permissions/owner on " << path << "\n";
                return false;
            }
        }

        char buffer[4096];
        while (content.size() <= 65536) {
            const ssize_t n = ::read(fd.get(), buffer, sizeof(buffer));
            if (n < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (n == 0) break;
            content.append(buffer, static_cast<size_t>(n));
        }
        return content.size() <= 65536;
    }

    static DaemonConfig parse_content(const std::string& raw) {
        DaemonConfig cfg;
        std::istringstream stream(raw);
        std::string line;
        while (std::getline(stream, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';' ||
                (line.front() == '[' && line.back() == ']')) continue;

            const size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            const std::string key = trim(std::string_view(line).substr(0, eq));
            const std::string value = trim(std::string_view(line).substr(eq + 1));

            if (key == "version" && value.size() <= 32) cfg.version = value;
            else if (key == "log_level" && value.size() <= 16) cfg.log_level = value;
            else if (key == "enable_uclamp") cfg.enable_uclamp = parse_bool(value, cfg.enable_uclamp);
            else if (key == "top_app_interactive_min")
                cfg.top_app_interactive_min = parse_uint(value, cfg.top_app_interactive_min);
            else if (key == "top_app_interactive_max")
                cfg.top_app_interactive_max = parse_uint(value, cfg.top_app_interactive_max);
            else if (key == "temp_monitor_interval_ms")
                cfg.temp_monitor_interval_ms = parse_uint(value, cfg.temp_monitor_interval_ms);
            else if (key == "thermal_trip_celsius")
                cfg.thermal_trip_celsius = parse_float(value, cfg.thermal_trip_celsius);
            else if (key == "thermal_recover_celsius")
                cfg.thermal_recover_celsius = parse_float(value, cfg.thermal_recover_celsius);
            else if (key == "emergency_rollback_on_overheat")
                cfg.emergency_rollback_on_overheat = parse_bool(value, cfg.emergency_rollback_on_overheat);
            else if (key == "cpu_pressure_boost_pct")
                cfg.cpu_pressure_boost_pct = parse_float(value, cfg.cpu_pressure_boost_pct);
            else if (key == "memory_pressure_guard_pct")
                cfg.memory_pressure_guard_pct = parse_float(value, cfg.memory_pressure_guard_pct);
            else if (key == "io_pressure_guard_pct")
                cfg.io_pressure_guard_pct = parse_float(value, cfg.io_pressure_guard_pct);
            else if (key == "thermal_headroom_celsius")
                cfg.thermal_headroom_celsius = parse_float(value, cfg.thermal_headroom_celsius);
            else if (key == "thermal_emergency_margin_celsius")
                cfg.thermal_emergency_margin_celsius = parse_float(value, cfg.thermal_emergency_margin_celsius);
        }

        cfg.temp_monitor_interval_ms = std::clamp(cfg.temp_monitor_interval_ms, 1000u, 60000u);
        cfg.thermal_trip_celsius = std::clamp(cfg.thermal_trip_celsius, 35.0f, 65.0f);
        cfg.thermal_recover_celsius =
            std::clamp(cfg.thermal_recover_celsius, 25.0f, cfg.thermal_trip_celsius - 1.0f);
        cfg.top_app_interactive_min = std::min(cfg.top_app_interactive_min, 1024u);
        cfg.top_app_interactive_max =
            std::clamp(cfg.top_app_interactive_max, cfg.top_app_interactive_min, 1024u);
        cfg.cpu_pressure_boost_pct = std::clamp(cfg.cpu_pressure_boost_pct, 1.0f, 50.0f);
        cfg.memory_pressure_guard_pct = std::clamp(cfg.memory_pressure_guard_pct, 0.5f, 30.0f);
        cfg.io_pressure_guard_pct = std::clamp(cfg.io_pressure_guard_pct, 0.5f, 30.0f);
        cfg.thermal_headroom_celsius = std::clamp(cfg.thermal_headroom_celsius, 1.0f, 10.0f);
        cfg.thermal_emergency_margin_celsius =
            std::clamp(cfg.thermal_emergency_margin_celsius, 2.0f, 10.0f);
        return cfg;
    }

    static DaemonConfig load_from_file(const std::string& path, bool enforce_root_owner = false) {
        std::string content;
        if (!read_secure_file(path, enforce_root_owner, content)) return DaemonConfig{};
        return parse_content(content);
    }

    static DaemonConfig load_best_available() {
        const bool running_as_root = (::geteuid() == 0);
        std::vector<std::string> paths = {
            "/data/adb/coreflow/coreflow.conf",
            "/data/adb/modules/coreflow_engine/coreflow.conf",
            "/system_ext/etc/coreflow/default.conf",
            "/system/etc/coreflow/default.conf",
            "/vendor/etc/coreflow/default.conf",
        };
        // Never load relative working-directory files when running as privileged root service.
        if (!running_as_root) {
            paths.emplace_back("config/coreflow.conf");
        }

        for (const auto& path : paths) {
            std::string content;
            if (read_secure_file(path, running_as_root, content)) {
                std::cout << "[CoreFlow][Config] " << path << "\n";
                return parse_content(content);
            }
        }
        std::cout << "[CoreFlow][Config] defaults\n";
        return {};
    }
};

} // namespace coreflow
