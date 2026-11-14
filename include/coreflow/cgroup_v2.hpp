/**
 * @file cgroup_v2.hpp
 * @brief cgroups v2 controller with verified, journaled thread placement.
 * @author mystivara-creator
 * @license Apache-2.0
 */
#pragma once
#include "types.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
namespace coreflow {
class CentralMutationCoordinator;
enum class MutationDomain : uint8_t;
enum class ControllerPriority : uint8_t;
class CgroupV2Controller {
public:
    explicit CgroupV2Controller(CentralMutationCoordinator& coordinator,
        std::string_view root_path = {},
        std::string_view proc_root = "/proc");
    ~CgroupV2Controller();
    CgroupV2Controller(const CgroupV2Controller&) = delete;
    CgroupV2Controller& operator=(const CgroupV2Controller&) = delete;
    [[nodiscard]] bool is_supported() const noexcept;
    [[nodiscard]] bool uses_legacy_tasks() const noexcept { return legacy_tasks_; }
    [[nodiscard]] std::string get_cgroup_path(CgroupTarget target) const;
    bool migrate_task(pid_t tid, CgroupTarget target);
    bool migrate_thread(pid_t tid, pid_t expected_tgid,
                        uint64_t expected_start_time_ticks, CgroupTarget target);
    bool restore_thread(pid_t tid);
    bool restore_all_migrated_threads() noexcept;
    [[nodiscard]] bool get_thread_identity(pid_t tid, pid_t& tgid,
                                            uint64_t& start_time_ticks) const noexcept;
    [[nodiscard]] bool is_task_in_target(pid_t tid, CgroupTarget target) const noexcept;
    [[nodiscard]] bool is_task_in_hierarchy(pid_t tid) const noexcept;
    [[nodiscard]] size_t pending_thread_migrations() const noexcept;
    bool set_cpuset(CgroupTarget target, std::string_view cpus);
    bool set_cpu_weight(CgroupTarget target, uint32_t weight);
    bool unlock_vendor_restrictions();
private:
    struct ThreadMigration final {
        pid_t tid{0};
        pid_t tgid{0};
        uint64_t start_time_ticks{0};
        std::string source_relative_path;
        std::string target_relative_path;
    };
    CentralMutationCoordinator& coordinator_;
    std::string root_path_;
    std::string proc_root_;
    std::string controller_name_;
    mutable std::mutex mutex_;
    std::string transaction_id_;
    uint64_t transaction_generation_{0};
    std::vector<std::string> active_mutation_paths_;
    std::vector<ThreadMigration> thread_migrations_;
    bool supported_{false};
    bool legacy_tasks_{false};
    std::array<std::string, 4> target_relative_paths_{};
    std::array<bool, 4> target_available_{};
    bool explicit_root_{false};
    bool write_node(const std::string& path, std::string_view value);
    [[nodiscard]] std::string read_node(const std::string& path) const;
    bool acquire_lease_locked();
    void release_lease_locked() noexcept;
    bool rollback_transaction_locked() noexcept;
    bool restore_thread_locked(pid_t tid) noexcept;
    bool handle_preemption(MutationDomain domain, std::string_view transaction_id) noexcept;
};
} // namespace coreflow
