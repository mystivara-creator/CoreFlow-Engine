/**
 * @file device_profiler.cpp
 * @brief Dynamic SoC Topology and Capability Discovery (Linux Kernel & AOSP Standards)
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/device_profiler.hpp"
#include "coreflow/thermal_monitor.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <sys/utsname.h>

namespace coreflow {

namespace fs = std::filesystem;
namespace {
bool exists(const fs::path& path) { std::error_code ec; return fs::exists(path, ec) && !ec; }
std::vector<fs::directory_entry> entries(const fs::path& path) {
    std::error_code ec;
    std::vector<fs::directory_entry> result;
    for (fs::directory_iterator it(path, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end && result.size() < 192; it.increment(ec)) result.push_back(*it);
    return result;
}
}

DeviceProfiler& DeviceProfiler::instance() {
    static DeviceProfiler inst;
    return inst;
}

DeviceTopology DeviceProfiler::probe() {
    if (probed_) return cached_topology_;

    DeviceTopology topo{};
    const long count = ::sysconf(_SC_NPROCESSORS_CONF);
    topo.total_cpus = count > 0 ? static_cast<uint32_t>(count) : 0;

    // 1. Read Linux Kernel Release via uname
    struct utsname uts{};
    if (::uname(&uts) == 0) {
        topo.kernel_version = uts.release;
    }

    // 2. Read Online CPUs Mask (e.g. "0-7" or "0-5")
    std::ifstream online_f("/sys/devices/system/cpu/online");
    if (online_f.is_open()) {
        online_f >> topo.online_cpus_mask;
    } else {
        topo.online_cpus_mask = "0-" + std::to_string(topo.total_cpus > 0 ? topo.total_cpus - 1 : 0);
    }

    // 3. Dynamic SoC and Platform Discovery
    detect_soc_name(topo);

    // 4. Dynamic CPUFreq Cluster Discovery
    discover_clusters(topo);

    // 5. Cgroups & Sched-Uclamp Capability Probe
    topo.has_cgroups_v2 = exists("/sys/fs/cgroup/cgroup.controllers") ||
                          exists("/sys/fs/cgroup/cgroup.subtree_control");
    topo.has_uclamp = exists("/sys/fs/cgroup/top-app/cpu.uclamp.min") ||
                      exists("/dev/cpuctl/top-app/cpu.uclamp.min");
    topo.has_surfaceflinger_hints = false; // Display node presence is not frame telemetry.

    // 6. Dynamic Battery & GPU Hardware Node Probing
    if (exists("/sys/class/power_supply/battery/current_max")) {
        topo.battery_control_node = "/sys/class/power_supply/battery/current_max";
    } else if (exists("/sys/class/power_supply/battery/constant_charge_current_max")) {
        topo.battery_control_node = "/sys/class/power_supply/battery/constant_charge_current_max";
    }

    if (exists("/sys/class/kgsl/kgsl-3d0")) {
        topo.gpu_type = "Qualcomm Adreno (KGSL)";
    } else if (exists("/sys/class/devfreq")) {
        for (const auto& entry : entries("/sys/class/devfreq")) {
            std::string name = entry.path().filename().string();
            if (name.find("mali") != std::string::npos || name.find("gpu") != std::string::npos) {
                topo.gpu_type = "ARM Mali / Immortalis (" + name + ")";
                break;
            }
        }
        if (topo.gpu_type.empty()) topo.gpu_type = "Unavailable (no identified GPU devfreq domain)";
    } else {
        topo.gpu_type = "Unavailable";
    }

    cached_topology_ = topo;
    probed_ = true;
    return topo;
}

void DeviceProfiler::detect_soc_name(DeviceTopology& topo) {
    // Priority 1: Linux Kernel Devicetree model (universal AOSP standard)
    const std::vector<std::string> dt_paths = {
        "/sys/firmware/devicetree/base/model",
        "/proc/device-tree/model"
    };

    for (const auto& path : dt_paths) {
        std::ifstream dt(path, std::ios::binary);
        if (dt.is_open()) {
            std::string model;
            std::getline(dt, model, '\0');
            if (!model.empty()) {
                topo.soc_name = model;
                break;
            }
        }
    }

    // Priority 2: Linux SoC bus (/sys/devices/soc0/)
    if (topo.soc_name.empty()) {
        std::ifstream machine_f("/sys/devices/soc0/machine");
        if (machine_f.is_open()) {
            std::getline(machine_f, topo.soc_name);
        }
        std::ifstream family_f("/sys/devices/soc0/family");
        if (family_f.is_open()) {
            family_f >> topo.hardware_platform;
        }
    }

    // Priority 3: /proc/cpuinfo Hardware field
    if (topo.soc_name.empty()) {
        std::ifstream cpuinfo("/proc/cpuinfo");
        if (cpuinfo.is_open()) {
            std::string line;
            while (std::getline(cpuinfo, line)) {
                if (line.rfind("Hardware", 0) == 0 || line.rfind("model name", 0) == 0) {
                    auto pos = line.find(':');
                    if (pos != std::string::npos && pos + 2 < line.size()) {
                        topo.soc_name = line.substr(pos + 2);
                        break;
                    }
                }
            }
        }
    }

    // Genuine fallback without hardcoded vendor favoritism
    if (topo.soc_name.empty()) {
        topo.soc_name = "Generic AOSP ARM64 Device (" + topo.kernel_version + ")";
    }

    // Determine device tier dynamically from core count and topology
    if (topo.total_cpus >= 8) {
        topo.tier = DeviceTier::MidRange;
    } else {
        topo.tier = DeviceTier::Entry;
    }
}

void DeviceProfiler::discover_clusters(DeviceTopology& topo) {
    topo.clusters.clear();

    const std::string cpufreq_dir = "/sys/devices/system/cpu/cpufreq";
    if (exists(cpufreq_dir)) {
        for (const auto& entry : entries(cpufreq_dir)) {
            std::string dir_name = entry.path().filename().string();
            if (dir_name.rfind("policy", 0) == 0) {
                CpuCluster cluster{};
                cluster.cpufreq_policy_path = entry.path().string();

                // Read related/affected cpus
                std::ifstream cpus_file(cluster.cpufreq_policy_path + "/affected_cpus");
                if (!cpus_file.is_open()) {
                    cpus_file.open(cluster.cpufreq_policy_path + "/related_cpus");
                }
                if (cpus_file.is_open()) {
                    std::string cpus_str;
                    std::getline(cpus_file, cpus_str);
                    std::stringstream ss(cpus_str);
                    ss >> cluster.first_cpu;
                    uint32_t last = cluster.first_cpu;
                    uint32_t val;
                    while (ss >> val) {
                        last = val;
                    }
                    cluster.last_cpu = last;
                }

                // Read min and max frequencies
                std::ifstream min_f(cluster.cpufreq_policy_path + "/cpuinfo_min_freq");
                if (min_f.is_open()) min_f >> cluster.base_freq_khz;

                std::ifstream max_f(cluster.cpufreq_policy_path + "/cpuinfo_max_freq");
                if (max_f.is_open()) max_f >> cluster.max_freq_khz;

                topo.clusters.push_back(cluster);
            }
        }
    }

    // If cpufreq nodes were probed, sort clusters by max_freq to assign Little/Mid/Big dynamically
    if (!topo.clusters.empty()) {
        std::sort(topo.clusters.begin(), topo.clusters.end(), [](const CpuCluster& a, const CpuCluster& b) {
            return a.max_freq_khz < b.max_freq_khz;
        });

        for (size_t i = 0; i < topo.clusters.size(); ++i) {
            if (i == 0) {
                topo.clusters[i].type = ClusterType::Little;
            } else if (i == topo.clusters.size() - 1) {
                topo.clusters[i].type = ClusterType::BigPrime;
            } else {
                topo.clusters[i].type = ClusterType::Medium;
            }
        }
        return;
    }

    // Unavailable topology stays unavailable; never invent clusters or frequencies.
}

float DeviceProfiler::get_soc_temperature() const {
    return ThermalMonitor::instance().hottest_celsius();
}

std::vector<uint32_t> DeviceProfiler::get_cluster_frequencies() const {
    std::vector<uint32_t> freqs;
    for (const auto& cluster : cached_topology_.clusters) {
        std::ifstream cur_f(cluster.cpufreq_policy_path + "/scaling_cur_freq");
        uint32_t cur = 0;
        if (cur_f.is_open()) {
            cur_f >> cur;
        }
        freqs.push_back(cur > 0 ? cur : cluster.base_freq_khz);
    }
    return freqs;
}

} // namespace coreflow

