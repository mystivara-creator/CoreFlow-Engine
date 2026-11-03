/**
 * @file cgroup_v2.hpp
 * @brief Linux cgroups v2 Unified Hierarchy Controller
 * @author mystivara-creator
 * @license Apache-2.0
 * 
 * Detects standard cgroups v2 when available and journals tunable writes.
 * Task migration/vendor overrides are deliberately disabled in v2 runtime.
 */

#pragma once

#include "types.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace coreflow {

class CgroupV2Controller {
public:
    explicit CgroupV2Controller(std::string_view root_path = "/sys/fs/cgroup");
    ~CgroupV2Controller() = default;

    /// Verify cgroups v2 availability on Android 15+ kernel
    [[nodiscard]] bool is_supported() const noexcept;

    /// Unsupported without membership recovery: returns false
    bool migrate_task(pid_t tid, CgroupTarget target);

    /// Set cpuset allowed CPUs for top-app / foreground (e.g. "0-7" or "4-7")
    bool set_cpuset(CgroupTarget target, std::string_view cpus);

    /// Configure CPU scheduling weight (1 - 10000, default 100)
    bool set_cpu_weight(CgroupTarget target, uint32_t weight);

    /// Unsupported: returns false without mutating vendor policy
    bool unlock_vendor_restrictions();

    /// Retrieve path for target cgroup directory
    [[nodiscard]] std::string get_cgroup_path(CgroupTarget target) const;

private:
    std::string root_path_;
    bool supported_{false};

};

} // namespace coreflow
