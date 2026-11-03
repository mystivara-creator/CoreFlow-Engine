/**
 * @file cgroup_v2.hpp
 * @brief cgroups v2 Unified Controller Interface
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include "types.hpp"
#include <string>
#include <string_view>

namespace coreflow {

class CgroupV2Controller {
public:
    explicit CgroupV2Controller(std::string_view root_path = "/sys/fs/cgroup");
    ~CgroupV2Controller() = default;

    [[nodiscard]] bool is_supported() const noexcept;
    [[nodiscard]] std::string get_cgroup_path(CgroupTarget target) const;

    bool migrate_task(pid_t tid, CgroupTarget target);
    bool set_cpuset(CgroupTarget target, std::string_view cpus);
    bool set_cpu_weight(CgroupTarget target, uint32_t weight);
    bool unlock_vendor_restrictions();

private:
    std::string root_path_;
    bool supported_{false};

    bool write_node(const std::string& path, std::string_view value);
    [[nodiscard]] std::string read_node(const std::string& path) const;
};

} // namespace coreflow
