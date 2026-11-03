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
#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <limits>

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
                const auto colon = line.find(':');
                if (colon != std::string::npos) {
                    topo.soc_name = line.substr(colon + 1);
                    while (!topo.soc_name.empty() && topo.soc_name.front() == ' ')
                        topo.soc_name.erase(topo.soc_name.begin());
                }
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
        std::ifstream cpu_list(entry.path() / "related_cpus");
        if (!cpu_list) cpu_list.open(entry.path() / "affected_cpus");
        uint32_t first = std::numeric_limits<uint32_t>::max();
        uint32_t last = 0;
        std::string item;
        while (cpu_list >> item) {
            const auto separator = item.find('-');
            auto parse_id = [](std::string_view text, uint32_t& value) {
                const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
                return result.ec == std::errc{} && result.ptr == text.data() + text.size();
            };
            uint32_t lo = 0, hi = 0;
            if (!parse_id(std::string_view(item).substr(0, separator), lo)) continue;
            if (separator == std::string::npos) hi = lo;
            else if (!parse_id(std::string_view(item).substr(separator + 1), hi) || hi < lo) continue;
            first = std::min(first, lo);
            last = std::max(last, hi);
        }
        if (first == std::numeric_limits<uint32_t>::max()) continue;
        CpuCluster cluster{};
        cluster.first_cpu = first;
        cluster.last_cpu = last;
        cluster.base_freq_khz = read_number("cpuinfo_min_freq");
        cluster.max_freq_khz = read_number("cpuinfo_max_freq");
        cluster.cpufreq_policy_path = entry.path().string();
        cluster.type = ClusterType::Little;
        topo.clusters.push_back(cluster);
    }
    std::sort(topo.clusters.begin(), topo.clusters.end(),
              [](const CpuCluster& a, const CpuCluster& b) { return a.first_cpu < b.first_cpu; });
    // Tier remains a coarse scheduling hint, never a substitute for capabilities.
    const auto max_it = std::max_element(topo.clusters.begin(), topo.clusters.end(),
        [](const CpuCluster& a, const CpuCluster& b) { return a.max_freq_khz < b.max_freq_khz; });
    if (max_it != topo.clusters.end()) {
        const uint32_t peak = max_it->max_freq_khz;
        topo.tier = peak > 3000000 ? DeviceTier::Flagship :
                    peak > 2500000 ? DeviceTier::UpperMid :
                    peak > 2000000 ? DeviceTier::MidRange : DeviceTier::Entry;
    }
}

float DeviceProfiler::get_soc_temperature() const {
    auto valid_temp = [](const std::string& path) -> float {
        std::ifstream stream(path);
        float raw = 0.0f;
        if (!(stream >> raw)) return -1.0f;
        const float value = raw > 1000.0f ? raw / 1000.0f : raw;
        return value >= -10.0f && value <= 130.0f ? value : -1.0f;
    };
    if (!thermal_zone_path_.empty()) {
        const float value = valid_temp(thermal_zone_path_);
        if (value >= 0.0f) return value;
        thermal_zone_path_.clear();
    }
    for (int i = 0; i < 128; ++i) {
        const std::string base = "/sys/class/thermal/thermal_zone" + std::to_string(i);
        std::ifstream label(base + "/type");
        std::string type;
        if (!std::getline(label, type)) continue;
        std::transform(type.begin(), type.end(), type.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (type.find("cpu") == std::string::npos &&
            type.find("soc") == std::string::npos &&
            type.find("tsens") == std::string::npos &&
            type.find("apss") == std::string::npos) continue;
        const std::string candidate = base + "/temp";
        const float value = valid_temp(candidate);
        if (value >= 0.0f) {
            thermal_zone_path_ = candidate;
            return value;
        }
    }
    return -1.0f;
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
