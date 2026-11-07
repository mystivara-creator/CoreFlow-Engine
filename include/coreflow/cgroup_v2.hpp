/**
 * @file cgroup_v2.hpp
 * @brief cgroups v2 Unified Controller Interface
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include "types.hpp"
#include <string>
#include <cstdint>
#include <mutex>
#include <vector>
#include <string_view>

namespace coreflow {
enum class MutationDomain : uint8_t;
enum class ControllerPriority : uint8_t;

class CgroupV2Controller {
public:
    explicit CgroupV2Controller(std::string_view root_path = "/sys/fs/cgroup");
    ~CgroupV2Controller();
    CgroupV2Controller(const CgroupV2Controller&) = delete;
    CgroupV2Controller& operator=(const CgroupV2Controller&) = delete;

    [[nodiscard]] bool is_supported() const noexcept;
    [[nodiscard]] std::string get_cgroup_path(CgroupTarget target) const;

    bool migrate_task(pid_t tid, CgroupTarget target);
    bool set_cpuset(CgroupTarget target, std::string_view cpus);
    bool set_cpu_weight(CgroupTarget target, uint32_t weight);
    bool unlock_vendor_restrictions();

private:
    std::string root_path_;
    std::string controller_name_;
    mutable std::mutex mutex_;
    std::string transaction_id_;
    uint64_t transaction_generation_{0};
    std::vector<std::string> active_mutation_paths_;
    bool supported_{false};

    bool write_node(const std::string& path, std::string_view value);
    [[nodiscard]] std::string read_node(const std::string& path) const;
    bool acquire_lease_locked();
    void release_lease_locked() noexcept;
    bool rollback_transaction_locked() noexcept;
    bool handle_preemption(MutationDomain domain, std::string_view transaction_id) noexcept;
};

} // namespace coreflow
