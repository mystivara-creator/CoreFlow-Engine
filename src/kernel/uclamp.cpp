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

bool UclampGovernor::apply_state(WorkloadState state, DeviceTier tier, uint32_t interactive_min, uint32_t interactive_max) {
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
            if (interactive_min > 0) {
                top_app_clamp.uclamp_min = std::min(interactive_min, 1024u);
                top_app_clamp.uclamp_max = std::clamp(interactive_max, top_app_clamp.uclamp_min, 1024u);
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

    // Stop immediately on failure. BaselineStore performs verified rollback.
    return write_uclamp(CgroupTarget::TopApp, top_app_clamp.uclamp_min, top_app_clamp.uclamp_max) &&
           write_uclamp(CgroupTarget::Foreground, fg_clamp.uclamp_min, fg_clamp.uclamp_max) &&
           write_uclamp(CgroupTarget::Background, bg_clamp.uclamp_min, bg_clamp.uclamp_max);
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
    auto to_percentage = [](uint32_t capacity) -> std::string {
        if (capacity >= 1024) return "max";
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
        return baseline.apply_mutation(min_path, to_percentage(min_val)) &&
               baseline.apply_mutation(max_path, to_percentage(max_val));
    }
    return baseline.apply_mutation(max_path, std::to_string(max_val)) &&
           baseline.apply_mutation(min_path, std::to_string(min_val));
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
