/**
 * @file cgroup_v2.cpp
 * @brief cgroups v2 Implementation with Direct Sysfs I/O
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/cgroup_v2.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace coreflow {

namespace fs = std::filesystem;

CgroupV2Controller::CgroupV2Controller(std::string_view root_path)
    : root_path_(root_path) {
    // Check if root_path/cgroup.controllers or cgroup.subtree_control exists
    std::string controllers_file = root_path_ + "/cgroup.controllers";
    supported_ = fs::exists(controllers_file);

    if (supported_) {
        std::cout << "[CoreFlow][CgroupV2] Modern cgroups v2 verified at " << root_path_ << std::endl;
    } else {
        std::cerr << "[CoreFlow][CgroupV2] Notice: cgroups v2 not mounted at " << root_path_
                  << ", attempting fallback discovery..." << std::endl;
    }
}

bool CgroupV2Controller::is_supported() const noexcept {
    return supported_;
}

std::string CgroupV2Controller::get_cgroup_path(CgroupTarget target) const {
    switch (target) {
        case CgroupTarget::TopApp:
            return root_path_ + "/top-app";
        case CgroupTarget::Foreground:
            return root_path_ + "/foreground";
        case CgroupTarget::Background:
            return root_path_ + "/background";
        case CgroupTarget::System:
            return root_path_ + "/system";
    }
    return root_path_;
}

bool CgroupV2Controller::migrate_task(pid_t tid, CgroupTarget target) {
    if (!supported_ || tid <= 0) return false;

    // In cgroups v2, threads are migrated via cgroup.threads or cgroup.procs
    std::string target_path = get_cgroup_path(target) + "/cgroup.threads";
    if (!fs::exists(target_path)) {
        target_path = get_cgroup_path(target) + "/cgroup.procs";
    }

    return write_node(target_path, std::to_string(tid));
}

bool CgroupV2Controller::set_cpuset(CgroupTarget target, std::string_view cpus) {
    if (!supported_) return false;
    std::string path = get_cgroup_path(target) + "/cpuset.cpus";
    return write_node(path, cpus);
}

bool CgroupV2Controller::set_cpu_weight(CgroupTarget target, uint32_t weight) {
    if (!supported_) return false;
    weight = std::clamp(weight, 1u, 10000u);
    std::string path = get_cgroup_path(target) + "/cpu.weight";
    return write_node(path, std::to_string(weight));
}

bool CgroupV2Controller::unlock_vendor_restrictions() {
    if (!supported_) return false;

    // Unlock top-app cpuset to all online CPUs (0-7 for octa-core)
    bool ok1 = set_cpuset(CgroupTarget::TopApp, "0-7");
    // Ensure top-app has prioritized weight
    bool ok2 = set_cpu_weight(CgroupTarget::TopApp, 500);
    // Lower background CPU weight so it doesn't starve the render thread
    bool ok3 = set_cpu_weight(CgroupTarget::Background, 50);

    return ok1 && ok2 && ok3;
}

bool CgroupV2Controller::write_node(const std::string& path, std::string_view value) {
    int fd = ::open(path.c_str(), O_WRONLY | O_CLOEXEC);
    if (fd < 0) return false;

    ssize_t written = ::write(fd, value.data(), value.size());
    ::close(fd);
    return written == static_cast<ssize_t>(value.size());
}

std::string CgroupV2Controller::read_node(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::string val;
    file >> val;
    return val;
}

} // namespace coreflow
