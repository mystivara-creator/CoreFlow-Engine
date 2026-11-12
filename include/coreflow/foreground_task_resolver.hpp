/**
 * @file foreground_task_resolver.hpp
 * @brief Resolves a unique foreground app and its render-critical Linux threads.
 * @license Apache-2.0
 */
#pragma once

#include "coreflow/cgroup_v2.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace coreflow {

struct ForegroundTaskSnapshot final {
    bool source_available{false};
    bool ambiguous{false};
    bool rollback_complete{true};
    uint32_t process_count{0};
    uint32_t package_count{0};
    uint32_t discovered_thread_count{0};
    uint32_t migrated_thread_count{0};
    uint32_t rejected_thread_count{0};
    std::string package_name;
};

class ForegroundTaskResolver final {
public:
    explicit ForegroundTaskResolver(
        CgroupV2Controller& controller,
        std::string top_app_cgroup = {},
        std::string proc_root = "/proc") noexcept;
    ~ForegroundTaskResolver();

    ForegroundTaskResolver(const ForegroundTaskResolver&) = delete;
    ForegroundTaskResolver& operator=(const ForegroundTaskResolver&) = delete;

    // Returns false for stale/ambiguous input or incomplete rollback. A forced
    // refresh is provided for deterministic tests and shutdown recovery.
    [[nodiscard]] bool refresh(bool force = false) noexcept;
    [[nodiscard]] bool restore_all() noexcept;
    [[nodiscard]] ForegroundTaskSnapshot snapshot() const;
    [[nodiscard]] size_t tracked_threads() const noexcept;

    [[nodiscard]] static bool is_excluded_process(std::string_view process_name) noexcept;
    [[nodiscard]] static bool is_excluded_thread(std::string_view thread_name) noexcept;
    [[nodiscard]] static bool is_whitelisted_thread(
        pid_t pid, pid_t tid, std::string_view thread_name) noexcept;

private:
    struct TaskIdentity final {
        pid_t tid{0};
        pid_t tgid{0};
        uint64_t start_time_ticks{0};
    };

    CgroupV2Controller& controller_;
    std::string top_app_cgroup_;
    std::string proc_root_;
    mutable std::mutex mutex_;
    ForegroundTaskSnapshot snapshot_{};
    std::unordered_map<pid_t, TaskIdentity> tracked_;
    uint64_t last_refresh_ns_{0};

    [[nodiscard]] bool read_top_app_pids(std::vector<pid_t>& pids) const noexcept;
    [[nodiscard]] bool read_process_identity(
        pid_t pid, std::string& package_name) const noexcept;
    [[nodiscard]] bool collect_render_threads(
        pid_t pid, std::vector<TaskIdentity>& threads) const noexcept;
    [[nodiscard]] bool restore_stale(
        const std::unordered_map<pid_t, TaskIdentity>& selected) noexcept;
};

} // namespace coreflow
