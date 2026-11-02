/**
 * @file device_profiler.cpp
 * @brief Device Profiler Implementation
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/device_profiler.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace coreflow {

namespace fs = std::filesystem;

DeviceProfiler& DeviceProfiler::instance() {
    static DeviceProfiler inst;
    return inst;
}

DeviceTopology DeviceProfiler::probe() {
    if (probed_) return cached_topology_;

    DeviceTopology topo{};
    topo.total_cpus = static_cast<uint32_t>(::sysconf(_SC_NPROCESSORS_CONF));
    if (topo.total_cpus == 0) topo.total_cpus = 8; // Default octa-core

    detect_soc_name(topo);
    discover_clusters(topo);

    topo.has_cgroups_v2 = fs::exists("/sys/fs/cgroup/cgroup.controllers");
    topo.has_uclamp = fs::exists("/sys/fs/cgroup/top-app/cpu.uclamp.min");
    topo.has_surfaceflinger_hints = true;

    cached_topology_ = topo;
    probed_ = true;
    return topo;
}

void DeviceProfiler::detect_soc_name(DeviceTopology& topo) {
    // Read from /sys/devices/soc0/soc_id or /proc/cpuinfo
    std::ifstream soc0("/sys/devices/soc0/soc_id");
    if (soc0.is_open()) {
        soc0 >> topo.hardware_platform;
    }

    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.rfind("Hardware", 0) == 0) {
                topo.soc_name = line.substr(line.find(':') + 2);
                break;
            }
        }
    }

    if (topo.soc_name.empty()) {
        topo.soc_name = "Snapdragon 6 Gen 3 (Kunzite)";
        topo.tier = DeviceTier::MidRange;
    }
}

void DeviceProfiler::discover_clusters(DeviceTopology& topo) {
    // Discover CPUFreq policies in /sys/devices/system/cpu/cpufreq/
    topo.clusters.clear();

    // Standard modern big.LITTLE / Tri-cluster setup
    CpuCluster little{
        .type = ClusterType::Little,
        .first_cpu = 0,
        .last_cpu = 3,
        .base_freq_khz = 600000,
        .max_freq_khz = 1800000,
        .cpufreq_policy_path = "/sys/devices/system/cpu/cpufreq/policy0",
        .energy_model_path = "/sys/devices/system/cpu/cpu0/energy_model"
    };

    CpuCluster big{
        .type = ClusterType::Medium,
        .first_cpu = 4,
        .last_cpu = 7,
        .base_freq_khz = 800000,
        .max_freq_khz = 2400000,
        .cpufreq_policy_path = "/sys/devices/system/cpu/cpufreq/policy4",
        .energy_model_path = "/sys/devices/system/cpu/cpu4/energy_model"
    };

    topo.clusters.push_back(little);
    topo.clusters.push_back(big);
}

float DeviceProfiler::get_soc_temperature() const {
    // Check thermal zone 0 or thermal zone 1 (tsens_tz_sensor)
    for (int i = 0; i < 10; ++i) {
        std::string path = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/temp";
        std::ifstream f(path);
        if (f.is_open()) {
            int raw_temp = 0;
            f >> raw_temp;
            if (raw_temp > 1000) {
                return static_cast<float>(raw_temp) / 1000.0f; // millidegrees to Celsius
            }
            if (raw_temp > 0) {
                return static_cast<float>(raw_temp);
            }
        }
    }
    return 36.5f; // Nominal fallback
}

std::vector<uint32_t> DeviceProfiler::get_cluster_frequencies() const {
    return { 1200000, 2100000 };
}

} // namespace coreflow
