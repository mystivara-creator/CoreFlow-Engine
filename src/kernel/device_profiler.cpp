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
    const long detected_cpus = ::sysconf(_SC_NPROCESSORS_CONF);
    topo.total_cpus = detected_cpus > 0 ? static_cast<uint32_t>(detected_cpus) : 0;

    detect_soc_name(topo);
    discover_clusters(topo);

    topo.has_cgroups_v2 = fs::exists("/sys/fs/cgroup/cgroup.controllers");
    topo.has_uclamp = fs::exists("/sys/fs/cgroup/top-app/cpu.uclamp.min");
    topo.has_surfaceflinger_hints = false;

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
        topo.soc_name = "Unknown SoC";
    }
}

void DeviceProfiler::discover_clusters(DeviceTopology& topo) {
    topo.clusters.clear();
    const fs::path root("/sys/devices/system/cpu/cpufreq");
    std::error_code ec;
    if (!fs::is_directory(root, ec)) return;
    for (const auto& entry : fs::directory_iterator(root, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) break;
        const auto name = entry.path().filename().string();
        if (name.rfind("policy", 0) != 0) continue;
        auto read_number = [&](const char* key) -> uint32_t {
            std::ifstream f(entry.path() / key);
            uint32_t value = 0;
            if (f) f >> value;
            return value;
        };
        CpuCluster cluster{};
        cluster.first_cpu = read_number("affected_cpus");
        cluster.last_cpu = cluster.first_cpu;
        cluster.base_freq_khz = read_number("cpuinfo_min_freq");
        cluster.max_freq_khz = read_number("cpuinfo_max_freq");
        cluster.cpufreq_policy_path = entry.path().string();
        cluster.type = ClusterType::Little;
        topo.clusters.push_back(cluster);
    }
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
    return -1.0f; // Unknown: never fabricate a thermal reading
}

std::vector<uint32_t> DeviceProfiler::get_cluster_frequencies() const {
    std::vector<uint32_t> result;
    for (const auto& cluster : cached_topology_.clusters) {
        std::ifstream f(cluster.cpufreq_policy_path + "/scaling_cur_freq");
        uint32_t freq = 0;
        if (f && (f >> freq)) result.push_back(freq);
    }
    return result;
}

} // namespace coreflow
