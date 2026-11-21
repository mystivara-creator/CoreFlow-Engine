/**
 * @file context_engine.hpp
 * @brief CoreFlow v5 Pillar 2: 5-Mode Context Classifier, Foreground App & Critical Thread Resolver.
 * @license Apache-2.0
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace coreflow::v5 {

enum class PerformanceProfile : uint8_t {
    Balanced = 0,
    Smoothness = 1,
    Battery = 2,
    Gaming = 3,
};

enum class ContextMode : uint8_t {
    Efficiency = 0,
    Interactive = 1,
    LaunchBurst = 2,
    Gaming = 3,
    ScreenOff = 4,
    Excluded = 5,
};

struct ForegroundAppInfo {
    int32_t pid{0};
    std::string package_name;
    std::vector<int32_t> critical_tids;
    bool has_game_thread{false};
    bool has_render_thread{false};
    bool is_camera_app{false};
    bool has_media_thread{false};
};

struct ContextSignalInput {
    uint64_t now_ms{0};
    bool screen_off{false};
    bool thermal_guard{false};
    double cpu_util_pct{0.0};
    std::optional<double> gpu_busy_pct{};
    int32_t foreground_pid{0};
    std::string foreground_package;
    bool has_game_thread{false};
    bool is_camera_app{false};
    bool has_media_thread{false};
};

struct ContextDecision {
    ContextMode mode{ContextMode::Efficiency};
    PerformanceProfile effective_profile{PerformanceProfile::Balanced};
    bool newly_launched_app{false};
    bool app_excluded{false};
    bool app_forced_gaming{false};
    bool camera_boost_active{false};
    bool media_pipeline_active{false};
    int32_t foreground_pid{0};
    std::string foreground_package;
    uint64_t launch_burst_remaining_ms{0};
    uint64_t touch_hold_remaining_ms{0};
};

class ContextEngine {
public:
    explicit ContextEngine(std::string procfs_root = "/proc") noexcept;

    void set_profile(PerformanceProfile profile) noexcept;
    void set_app_rules(const std::vector<std::string>& gaming_apps,
                       const std::vector<std::string>& excluded_apps,
                       const std::vector<std::string>& smoothness_apps = {},
                       const std::vector<std::string>& battery_apps = {});

    // Called from epoll input path (< 5ms latency) on touch activity.
    void notify_touch(uint64_t now_ms) noexcept;

    // Evaluates current system signals into one clean context mode.
    [[nodiscard]] ContextDecision evaluate(const ContextSignalInput& input) noexcept;

    // Lightweight /proc readers (parameterized by procfs_root_ for unit tests).
    [[nodiscard]] ForegroundAppInfo resolve_foreground_app(
        const std::string& top_app_procs_path) const noexcept;
    [[nodiscard]] std::vector<int32_t> resolve_surfaceflinger_tids() noexcept;
    [[nodiscard]] std::vector<int32_t> resolve_camera_hal_tids() noexcept;

    [[nodiscard]] PerformanceProfile profile() const noexcept { return profile_; }
    [[nodiscard]] static PerformanceProfile parse_profile(std::string_view text) noexcept;
    [[nodiscard]] static const char* profile_name(PerformanceProfile profile) noexcept;
    [[nodiscard]] static const char* mode_name(ContextMode mode) noexcept;
    [[nodiscard]] static bool is_critical_render_thread_name(std::string_view comm) noexcept;
    [[nodiscard]] static bool is_game_engine_thread_name(std::string_view comm) noexcept;
    [[nodiscard]] static bool is_media_pipeline_thread_name(std::string_view comm) noexcept;
    [[nodiscard]] static bool is_camera_package(std::string_view pkg) noexcept;
    [[nodiscard]] static bool is_background_system_service(std::string_view pkg) noexcept;

private:
    std::string procfs_root_;
    PerformanceProfile profile_{PerformanceProfile::Balanced};
    std::unordered_set<std::string> gaming_apps_;
    std::unordered_set<std::string> excluded_apps_;
    std::unordered_set<std::string> smoothness_apps_;
    std::unordered_set<std::string> battery_apps_;

    uint64_t last_touch_ms_{0};
    uint64_t launch_burst_until_ms_{0};
    int32_t last_fg_pid_{0};
    std::string last_fg_package_;
    bool gaming_latched_{false};
    uint32_t gpu_high_streak_{0};
    uint32_t gpu_low_streak_{0};

    int32_t cached_sf_pid_{0};
    uint64_t cached_sf_checked_ms_{0};
    std::vector<int32_t> cached_sf_tids_;

    uint64_t cached_cam_checked_ms_{0};
    std::vector<int32_t> cached_cam_tids_;

    mutable int32_t cached_fg_pid_{0};
    mutable uint64_t cached_fg_tids_ms_{0};
    mutable std::vector<int32_t> cached_fg_tids_;
    mutable bool cached_fg_has_game_thread_{false};
    mutable bool cached_fg_has_media_thread_{false};
};

} // namespace coreflow::v5
