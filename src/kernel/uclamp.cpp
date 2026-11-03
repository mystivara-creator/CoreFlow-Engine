/**
 * @file uclamp.cpp
 * @brief Uclamp Governor Implementation with RAII Safety & Input Clamping
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/uclamp.hpp"
#include "coreflow/unique_fd.hpp"
#include "coreflow/baseline_store.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace coreflow {

namespace fs = std::filesystem;

UclampGovernor::UclampGovernor(std::string_view cgroup_root)
    : cgroup_root_(cgroup_root) {
    std::string test_path = cgroup_root_ + "/top-app/cpu.uclamp.min";
    supported_ = fs::exists(test_path);

    if (supported_) {
        std::cerr << "[CoreFlow][Uclamp] EAS Utilization Clamping supported by kernel." << std::endl;
    } else {
        std::cerr << "[CoreFlow][Uclamp] Notice: cpu.uclamp.min not detected at " << test_path << std::endl;
    }
}

bool UclampGovernor::is_supported() const noexcept {
    return supported_;
}

bool UclampGovernor::apply_state(WorkloadState state, DeviceTier tier,
                                  uint32_t interactive_min, uint32_t interactive_max) {
    if (!supported_) return false;
    // Deliberately keep Android-owned foreground/background task profiles intact.
    // Only tune the verified top-app group; a single change has fewer side effects.
    UclampValues top;
    switch (state) {
        case WorkloadState::DeepSleep:
            top = { .uclamp_min = 0, .uclamp_max = 300 };
            break;
        case WorkloadState::DailyIdle:
            top = { .uclamp_min = 50, .uclamp_max = 800 };
            break;
        case WorkloadState::Interactive:
            top = { .uclamp_min = (tier == DeviceTier::Entry || tier == DeviceTier::MidRange) ? 380u : 250u,
                    .uclamp_max = 1024 };
            if (interactive_min > 0) {
                top.uclamp_min = std::min(interactive_min, 1024u);
                top.uclamp_max = std::clamp(interactive_max, top.uclamp_min, 1024u);
            }
            break;
        case WorkloadState::BurstLaunch:
            top = { .uclamp_min = 800, .uclamp_max = 1024 };
            break;
        case WorkloadState::SustainedGame:
            top = { .uclamp_min = 512, .uclamp_max = 980 };
            break;
        case WorkloadState::ThermalThrottled:
            top = { .uclamp_min = 0, .uclamp_max = 600 };
            break;
    }
    return write_uclamp(CgroupTarget::TopApp, top.uclamp_min, top.uclamp_max);
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

    auto& baseline = BaselineStore::instance();
    // The cgroup v2 cpu.uclamp files accept *percentage* values, unlike the
    // sched capacity scale (0..1024) used by our internal configurations.
    auto to_percentage = [](uint32_t capacity, bool maximum) -> std::string {
        if (maximum && capacity >= 1024) return "max";
        char out[32];
        std::snprintf(out, sizeof(out), "%.2f", static_cast<double>(capacity) * 100.0 / 1024.0);
        return out;
    };
    // Apply the ceiling before raising the floor, and reverse the order when
    // lowering the ceiling, so intermediate values remain valid.
    const std::string min_path = prefix + "cpu.uclamp.min";
    const std::string max_path = prefix + "cpu.uclamp.max";
    const uint32_t current_min = read_uclamp_node(min_path);
    if (max_val < current_min) {
        return baseline.apply_mutation(min_path, to_percentage(min_val, false)) &&
               baseline.apply_mutation(max_path, to_percentage(max_val, true));
    }
    return baseline.apply_mutation(max_path, to_percentage(max_val, true)) &&
           baseline.apply_mutation(min_path, to_percentage(min_val, false));
}

bool UclampGovernor::set_rt_clamp_min(uint32_t clamp_val) {
    clamp_val = std::clamp(clamp_val, 0u, 1024u);
    return BaselineStore::instance().apply_mutation(
        "/proc/sys/kernel/sched_util_clamp_min_rt_default", std::to_string(clamp_val));
}

uint32_t UclampGovernor::get_top_app_min() const {
    return read_uclamp_node(cgroup_root_ + "/top-app/cpu.uclamp.min");
}

uint32_t UclampGovernor::get_top_app_max() const {
    return read_uclamp_node(cgroup_root_ + "/top-app/cpu.uclamp.max");
}

uint32_t UclampGovernor::read_uclamp_node(const std::string& path) const {
    std::ifstream f(path);
    std::string raw;
    if (!(f >> raw)) return 0;
    if (raw == "max") return 1024;
    char* end = nullptr;
    const double percent = std::strtod(raw.c_str(), &end);
    if (end == raw.c_str() || *end != '\0' || !std::isfinite(percent) ||
        percent < 0.0 || percent > 100.0) return 0;
    return static_cast<uint32_t>(std::lround(percent * 1024.0 / 100.0));
}

} // namespace coreflow
