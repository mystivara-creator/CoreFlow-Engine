/**
 * @file uclamp.cpp
 * @brief Uclamp Governor Implementation with RAII Safety & Input Clamping
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/uclamp.hpp"
#include "coreflow/unique_fd.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace coreflow {

namespace fs = std::filesystem;

UclampGovernor::UclampGovernor(std::string_view cgroup_root)
    : cgroup_root_(cgroup_root) {
    std::string test_path = cgroup_root_ + "/top-app/cpu.uclamp.min";
    supported_ = fs::exists(test_path);

    if (supported_) {
        std::cout << "[CoreFlow][Uclamp] EAS Utilization Clamping supported by kernel." << std::endl;
    } else {
        std::cerr << "[CoreFlow][Uclamp] Notice: cpu.uclamp.min not detected at " << test_path << std::endl;
    }
}

bool UclampGovernor::is_supported() const noexcept {
    return supported_;
}

bool UclampGovernor::apply_state(WorkloadState state, DeviceTier tier) {
    if (!supported_) return false;

    UclampValues top_app_clamp;
    UclampValues fg_clamp;
    UclampValues bg_clamp;

    switch (state) {
        case WorkloadState::DeepSleep:
            top_app_clamp = { .uclamp_min = 0, .uclamp_max = 300 };
            fg_clamp      = { .uclamp_min = 0, .uclamp_max = 200 };
            bg_clamp      = { .uclamp_min = 0, .uclamp_max = 100 };
            break;

        case WorkloadState::DailyIdle:
            top_app_clamp = { .uclamp_min = 50, .uclamp_max = 800 };
            fg_clamp      = { .uclamp_min = 20, .uclamp_max = 600 };
            bg_clamp      = { .uclamp_min = 0,  .uclamp_max = 300 };
            break;

        case WorkloadState::Interactive:
            if (tier == DeviceTier::Entry || tier == DeviceTier::MidRange) {
                top_app_clamp = { .uclamp_min = 380, .uclamp_max = 1024 };
            } else {
                top_app_clamp = { .uclamp_min = 250, .uclamp_max = 1024 };
            }
            fg_clamp = { .uclamp_min = 60, .uclamp_max = 800 };
            bg_clamp = { .uclamp_min = 0,  .uclamp_max = 350 };
            break;

        case WorkloadState::BurstLaunch:
            top_app_clamp = { .uclamp_min = 800, .uclamp_max = 1024 };
            fg_clamp      = { .uclamp_min = 200, .uclamp_max = 1024 };
            bg_clamp      = { .uclamp_min = 0,   .uclamp_max = 300 };
            break;

        case WorkloadState::SustainedGame:
            top_app_clamp = { .uclamp_min = 512, .uclamp_max = 980 };
            fg_clamp      = { .uclamp_min = 120, .uclamp_max = 700 };
            bg_clamp      = { .uclamp_min = 0,   .uclamp_max = 200 };
            break;

        case WorkloadState::ThermalThrottled:
            top_app_clamp = { .uclamp_min = 0, .uclamp_max = 600 };
            fg_clamp      = { .uclamp_min = 0, .uclamp_max = 450 };
            bg_clamp      = { .uclamp_min = 0, .uclamp_max = 150 };
            break;
    }

    bool r1 = write_uclamp(CgroupTarget::TopApp, top_app_clamp.uclamp_min, top_app_clamp.uclamp_max);
    bool r2 = write_uclamp(CgroupTarget::Foreground, fg_clamp.uclamp_min, fg_clamp.uclamp_max);
    bool r3 = write_uclamp(CgroupTarget::Background, bg_clamp.uclamp_min, bg_clamp.uclamp_max);

    return r1 && r2 && r3;
}

bool UclampGovernor::write_uclamp(CgroupTarget target, uint32_t min_val, uint32_t max_val) {
    std::string prefix;
    switch (target) {
        case CgroupTarget::TopApp: prefix = cgroup_root_ + "/top-app/"; break;
        case CgroupTarget::Foreground: prefix = cgroup_root_ + "/foreground/"; break;
        case CgroupTarget::Background: prefix = cgroup_root_ + "/background/"; break;
        default: prefix = cgroup_root_ + "/"; break;
    }

    min_val = std::clamp(min_val, 0u, 1024u);
    max_val = std::clamp(max_val, min_val, 1024u);

    auto write_file = [](const std::string& p, uint32_t val) {
        UniqueFd ufd(::open(p.c_str(), O_WRONLY | O_CLOEXEC));
        if (!ufd.valid()) return false;
        std::string s = std::to_string(val);
        ssize_t w = ::write(ufd.get(), s.c_str(), s.size());
        return w == static_cast<ssize_t>(s.size());
    };

    bool ok_min = write_file(prefix + "cpu.uclamp.min", min_val);
    bool ok_max = write_file(prefix + "cpu.uclamp.max", max_val);
    return ok_min && ok_max;
}

bool UclampGovernor::set_rt_clamp_min(uint32_t clamp_val) {
    clamp_val = std::clamp(clamp_val, 0u, 1024u);
    const char* rt_node = "/proc/sys/kernel/sched_util_clamp_min_rt_default";
    UniqueFd ufd(::open(rt_node, O_WRONLY | O_CLOEXEC));
    if (!ufd.valid()) return false;
    std::string s = std::to_string(clamp_val);
    ssize_t w = ::write(ufd.get(), s.c_str(), s.size());
    return w == static_cast<ssize_t>(s.size());
}

uint32_t UclampGovernor::get_top_app_min() const {
    return read_uclamp_node(cgroup_root_ + "/top-app/cpu.uclamp.min");
}

uint32_t UclampGovernor::get_top_app_max() const {
    return read_uclamp_node(cgroup_root_ + "/top-app/cpu.uclamp.max");
}

uint32_t UclampGovernor::read_uclamp_node(const std::string& path) const {
    std::ifstream f(path);
    if (!f.is_open()) return 0;
    uint32_t val = 0;
    f >> val;
    return std::clamp(val, 0u, 1024u);
}

} // namespace coreflow
