/**
 * @file device_profiler.cpp
 * @brief Dynamic SoC Topology and Capability Discovery (Linux Kernel & AOSP Standards)
 * @author mystivara-creator
 * @license Apache-2.0
 */

#include "coreflow/device_profiler.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <sys/utsname.h>

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
    if (topo.total_cpus == 0) topo.total_cpus = 8;

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
        topo.online_cpus_mask = "0-" + std::to_string(topo.total_cpus > 0 ? topo.total_cpus - 1 : 7);
    }

    // 3. Dynamic SoC and Platform Discovery
    detect_soc_name(topo);

    // 4. Dynamic CPUFreq Cluster Discovery
    discover_clusters(topo);

    // 5. Cgroups & Sched-Uclamp Capability Probe
    topo.has_cgroups_v2 = fs::exists("/sys/fs/cgroup/cgroup.controllers") ||
                          fs::exists("/sys/fs/cgroup/cgroup.subtree_control");
    topo.has_uclamp = fs::exists("/sys/fs/cgroup/top-app/cpu.uclamp.min") ||
                      fs::exists("/dev/cpuctl/top-app/cpu.uclamp.min");
    topo.has_surfaceflinger_hints = fs::exists("/sys/class/drm") ||
                                    fs::exists("/sys/devices/virtual/graphics/fb0");

    // 6. Dynamic Battery & GPU Hardware Node Probing
    if (fs::exists("/sys/class/power_supply/battery/current_max")) {
        topo.battery_control_node = "/sys/class/power_supply/battery/current_max";
    } else if (fs::exists("/sys/class/power_supply/battery/constant_charge_current_max")) {
        topo.battery_control_node = "/sys/class/power_supply/battery/constant_charge_current_max";
    }

    if (fs::exists("/sys/class/kgsl/kgsl-3d0")) {
        topo.gpu_type = "Qualcomm Adreno (KGSL)";
    } else if (fs::exists("/sys/class/devfreq")) {
        for (const auto& entry : fs::directory_iterator("/sys/class/devfreq")) {
            std::string name = entry.path().filename().string();
            if (name.find("mali") != std::string::npos || name.find("gpu") != std::string::npos) {
                topo.gpu_type = "ARM Mali / Immortalis (" + name + ")";
                break;
            }
        }
        if (topo.gpu_type.empty()) topo.gpu_type = "Linux Generic Devfreq";
    } else {
        topo.gpu_type = "AOSP Standard SurfaceFlinger";
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
    if (fs::exists(cpufreq_dir)) {
        for (const auto& entry : fs::directory_iterator(cpufreq_dir)) {
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

    // Dynamic fallback when /sys/devices/system/cpu/cpufreq is not mounted (e.g. host CI)
    uint32_t half = topo.total_cpus / 2;
    topo.clusters.push_back(CpuCluster{
        .type = ClusterType::Little,
        .first_cpu = 0,
        .last_cpu = half > 0 ? half - 1 : 0,
        .base_freq_khz = 600000,
        .max_freq_khz = 1800000,
        .cpufreq_policy_path = "/sys/devices/system/cpu/cpufreq/policy0",
        .energy_model_path = ""
    });

    if (topo.total_cpus > 1) {
        topo.clusters.push_back(CpuCluster{
            .type = ClusterType::BigPrime,
            .first_cpu = half,
            .last_cpu = topo.total_cpus - 1,
            .base_freq_khz = 800000,
            .max_freq_khz = 2400000,
            .cpufreq_policy_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(half),
            .energy_model_path = ""
        });
    }
}

float DeviceProfiler::get_soc_temperature() const {
    // Linux sysfs thermal framework: scan thermal zones dynamically
    int best_zone = -1;
    std::string best_zone_type;

    for (int i = 0; i < 30; ++i) {
        std::string type_path = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/type";
        std::ifstream type_f(type_path);
        if (type_f.is_open()) {
            std::string ztype;
            std::getline(type_f, ztype);
            std::string lower = ztype;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            // Match genuine SoC/CPU thermal sensor identifiers
            if (lower.find("soc") != std::string::npos ||
                lower.find("cpu") != std::string::npos ||
                lower.find("tsens") != std::string::npos ||
                lower.find("mtkts") != std::string::npos ||
                lower.find("thermal") != std::string::npos) {
                best_zone = i;
                best_zone_type = ztype;
                break;
            }
            if (best_zone < 0) {
                best_zone = i;
                best_zone_type = ztype;
            }
        }
    }

    if (best_zone >= 0) {
        std::string temp_path = "/sys/class/thermal/thermal_zone" + std::to_string(best_zone) + "/temp";
        std::ifstream temp_f(temp_path);
        if (temp_f.is_open()) {
            int raw_temp = 0;
            temp_f >> raw_temp;
            if (raw_temp > 1000) {
                return static_cast<float>(raw_temp) / 1000.0f; // millidegrees to Celsius
            }
            if (raw_temp > 0) {
                return static_cast<float>(raw_temp);
            }
        }
    }

    return 36.5f; // Nominal ambient thermal reading
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
