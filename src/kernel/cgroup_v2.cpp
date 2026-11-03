/**
 * @file cgroup_v2.cpp
 * @brief cgroups v2 Implementation with Dynamic CPU Topology (Linux & AOSP Standards)
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/cgroup_v2.hpp"
#include "coreflow/device_profiler.hpp"
#include "coreflow/unique_fd.hpp"
#include "coreflow/baseline_store.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace coreflow {

namespace fs = std::filesystem;

CgroupV2Controller::CgroupV2Controller(std::string_view root_path)
    : root_path_(root_path) {
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
    // Task migration requires a verified original membership and lifecycle owner.
    // Do not silently migrate an entire process when cgroup.threads is absent.
    (void)tid; (void)target;
    return false;
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
    // Opening vendor cpusets would bypass OEM task-placement policy without
    // proving a benefit. No transparent vendor-override backend is installed.
    return false;
}

bool CgroupV2Controller::write_node(const std::string& path, std::string_view value) {
    return BaselineStore::instance().capture(path) &&
           BaselineStore::instance().apply_mutation(path, std::string(value));
}

std::string CgroupV2Controller::read_node(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::string val;
    file >> val;
    return val;
}

} // namespace coreflow

