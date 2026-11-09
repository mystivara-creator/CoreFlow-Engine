/**
 * @file device_profiler.cpp
 * @brief Read-only, root-injectable OS capability discovery.
 * @license Apache-2.0
 */
#include "coreflow/device_profiler.hpp"
#include "coreflow/thermal_monitor.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;
namespace {
constexpr size_t kMaxAttributeBytes = 64U * 1024U;
constexpr size_t kMaxDirectoryEntries = 256U;
constexpr uint32_t kMaxCpuId = 4095U;
constexpr uint64_t kMaxFrequencyEntries = 512U;

std::string trim(std::string value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}
bool read_attribute(const fs::path& path, std::string& out, size_t limit = kMaxAttributeBytes) {
    out.clear();
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) { (void)::close(fd); return false; }
    std::array<char, 4096> buffer{};
    bool ok = true;
    for (;;) {
        const ssize_t n = ::read(fd, buffer.data(), buffer.size());
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { ok = false; break; }
        if (n == 0) break;
        const size_t count = static_cast<size_t>(n);
        if (out.size() > limit || count > limit - out.size()) { ok = false; break; }
        out.append(buffer.data(), count);
    }
    (void)::close(fd);
    if (!ok) out.clear();
    return ok && !out.empty();
}
bool parse_u64(std::string_view text, uint64_t& out) noexcept {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) text.remove_suffix(1);
    if (text.empty()) return false;
    uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, 10);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
    out = value;
    return true;
}
bool parse_frequency_table(std::string_view text, std::vector<uint64_t>& out) {
    out.clear();
    std::istringstream input{std::string(text)};
    std::string token;
    while (input >> token) {
        uint64_t value = 0;
        if (!parse_u64(token, value) || value == 0 || out.size() >= kMaxFrequencyEntries) {
            out.clear(); return false;
        }
        out.push_back(value);
    }
    if (out.empty()) return false;
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return !out.empty();
}
bool read_u64(const fs::path& path, uint64_t& value) {
    std::string raw;
    return read_attribute(path, raw, 256U) && parse_u64(raw, value);
}
bool cddp_path_exists(const fs::path& path) noexcept {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}
bool writable_by_mode_and_process(const fs::path& path) noexcept {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    // access(W_OK) alone is insufficient under uid 0.
    const bool mode_writable = (st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0;
    return mode_writable && ::access(path.c_str(), W_OK) == 0;
}
std::vector<fs::path> directory_entries(const fs::path& root, std::string_view prefix = {}) {
    std::vector<fs::path> result;
    std::error_code ec;
    fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    for (; !ec && it != end && result.size() < kMaxDirectoryEntries; it.increment(ec)) {
        const std::string name = it->path().filename().string();
        if (!prefix.empty() && name.rfind(prefix, 0) != 0) continue;
        std::error_code type_ec;
        if (fs::is_directory(it->path(), type_ec) && !type_ec) result.push_back(it->path());
    }
    std::sort(result.begin(), result.end());
    return result;
}
uint64_t fnv1a(std::string_view text) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char c : text) { hash ^= static_cast<uint64_t>(c); hash *= 1099511628211ULL; }
    return hash;
}
std::string make_fingerprint(const DeviceProfileSnapshot& profile) {
    std::ostringstream data;
    data << "cddp-v1|" << profile.kernel_release << '|' << profile.device_model << '|'
         << profile.soc_model << '|' << profile.hardware_platform << '|';
    for (const auto cpu : profile.possible_cpus) data << "p" << cpu << ',';
    for (const auto cpu : profile.present_cpus) data << "r" << cpu << ',';
    for (const auto cpu : profile.online_cpus) data << "o" << cpu << ',';
    for (const auto& policy : profile.cpu_policies) {
        data << "cpu:" << policy.policy_path << ':' << policy.governor << ':';
        for (const auto cpu : policy.related_cpus) data << cpu << ',';
        data << policy.cpuinfo_min_freq_khz << ':' << policy.cpuinfo_max_freq_khz << ':';
        for (const auto freq : policy.available_frequencies_khz) data << freq << ',';
    }
    for (const auto& gpu : profile.gpu_domains) {
        data << "gpu:" << gpu.device_path << ':' << gpu.driver << ':';
        for (const auto freq : gpu.available_frequencies_hz) data << freq << ',';
    }
    for (const auto& cap : profile.node_capabilities) {
        data << "cap:" << cap.path << ':' << cap.semantic_id << ':'
             << static_cast<unsigned>(cap.state) << ':' << cap.unit << ':' << cap.constraints << '|';
    }
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << fnv1a(data.str());
    return out.str();
}
NodeCapability observe_capability(
    const fs::path& path, HardwareDomain domain, ProfileSource source,
    std::string semantic, std::string unit, std::string constraints,
    bool semantics_known, bool constraints_valid, bool eligible, std::string evidence) {
    NodeCapability cap{};
    cap.path = path.string();
    cap.domain = domain;
    cap.source = source;
    cap.semantic_id = std::move(semantic);
    cap.unit = std::move(unit);
    cap.constraints = std::move(constraints);
    cap.evidence = std::move(evidence);
    if (!cddp_path_exists(path)) { cap.rejection_reason = "node absent"; return cap; }
    cap.state = NodeCapabilityState::Available;
    std::string raw;
    if (!read_attribute(path, raw, 1024U)) {
        cap.rejection_reason = "node not readable as a bounded regular attribute"; return cap;
    }
    cap.current_value = trim(raw);
    cap.state = NodeCapabilityState::Readable;
    if (!writable_by_mode_and_process(path)) {
        cap.rejection_reason = "write permission not established by mode and process access"; return cap;
    }
    cap.state = NodeCapabilityState::Writable;
    if (!semantics_known || cap.semantic_id.empty() || cap.unit.empty()) {
        cap.rejection_reason = "interface semantics or unit are not verified"; return cap;
    }
    cap.state = NodeCapabilityState::SemanticsVerified;
    if (!constraints_valid || cap.constraints.empty()) {
        cap.rejection_reason = "accepted value constraints or preflight are not verified"; return cap;
    }
    cap.state = NodeCapabilityState::ConstraintsVerified;
    if (!eligible) { cap.rejection_reason = "static checks passed but mutation admission was denied"; return cap; }
    cap.state = NodeCapabilityState::MutationEligible;
    return cap;
}
bool valid_subset(const std::vector<uint32_t>& subset, const std::vector<uint32_t>& superset) {
    return std::all_of(subset.begin(), subset.end(), [&](uint32_t cpu) {
        return std::binary_search(superset.begin(), superset.end(), cpu);
    });
}
std::string join_cpu_ids(const std::vector<uint32_t>& cpus) {
    std::ostringstream out;
    for (size_t i = 0; i < cpus.size(); ++i) { if (i != 0) out << ','; out << cpus[i]; }
    return out.str();
}
} // namespace

DeviceProfiler::DeviceProfiler(std::string sys_root, std::string proc_root)
    : sys_root_(std::move(sys_root)), proc_root_(std::move(proc_root)) {
    if (sys_root_.empty()) sys_root_ = "/sys";
    if (proc_root_.empty()) proc_root_ = "/proc";
}
DeviceProfiler& DeviceProfiler::instance() {
    static DeviceProfiler profiler;
    return profiler;
}
bool DeviceProfiler::parse_cpu_list(std::string_view text, std::vector<uint32_t>& cpus) noexcept {
    cpus.clear();
    if (text.empty() || text.size() > 16384U) return false;

    size_t begin = 0;
    while (begin < text.size()) {
        const size_t comma = text.find(',', begin);
        const size_t end = comma == std::string_view::npos ? text.size() : comma;
        std::string_view item = text.substr(begin, end - begin);
        while (!item.empty() && std::isspace(static_cast<unsigned char>(item.front())) != 0) item.remove_prefix(1);
        while (!item.empty() && std::isspace(static_cast<unsigned char>(item.back())) != 0) item.remove_suffix(1);
        if (item.empty()) return false;

        const size_t dash = item.find('-');
        uint64_t first = 0, last = 0;
        if (dash == std::string_view::npos) {
            if (!parse_u64(item, first)) return false;
            last = first;
        } else {
            if (item.find('-', dash + 1U) != std::string_view::npos ||
                !parse_u64(item.substr(0, dash), first) ||
                !parse_u64(item.substr(dash + 1U), last) || first > last) return false;
        }

        if (last > kMaxCpuId || (last - first) > kMaxCpuId ||
            cpus.size() + (last - first + 1U) > 4096U) return false;
        for (uint64_t cpu = first; cpu <= last; ++cpu) cpus.push_back(static_cast<uint32_t>(cpu));

        if (comma == std::string_view::npos) break;
        begin = comma + 1U;
        if (begin == text.size()) return false;
    }

    std::sort(cpus.begin(), cpus.end());
    if (cpus.empty() || std::adjacent_find(cpus.begin(), cpus.end()) != cpus.end()) {
        cpus.clear();
        return false;
    }
    return true;
}
DeviceProfileHandle DeviceProfiler::profile() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!current_) { ++generation_; current_ = build_profile(generation_); }
    return current_;
}
DeviceProfileHandle DeviceProfiler::refresh() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++generation_;
    current_ = build_profile(generation_);
    return current_;
}
DeviceProfileHandle DeviceProfiler::build_profile(uint64_t generation) const {
    auto snapshot = std::make_shared<DeviceProfileSnapshot>();
    snapshot->generation = generation;
    snapshot->sys_root = sys_root_;
    snapshot->proc_root = proc_root_;
    snapshot->captured_monotonic_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    std::string raw;
    const fs::path sys(sys_root_);
    const fs::path proc(proc_root_);

    // Root injection is hermetic: never substitute the host kernel identity
    // when the injected procfs tree does not expose a release value.
    if (read_attribute(proc / "sys/kernel/osrelease", raw, 256U)) {
        snapshot->kernel_release = trim(raw);
    }

    if (read_attribute(sys / "firmware/devicetree/base/model", raw) ||
        read_attribute(proc / "device-tree/model", raw)) {
        while (!raw.empty() && raw.back() == '\0') raw.pop_back();
        snapshot->device_model = trim(raw);
    }
    if (read_attribute(sys / "devices/soc0/machine", raw, 1024U)) snapshot->soc_model = trim(raw);
    if (read_attribute(sys / "devices/soc0/family", raw, 1024U)) snapshot->hardware_platform = trim(raw);

    const auto read_cpu_set = [&](const char* name, std::vector<uint32_t>& values) {
        std::string content;
        return read_attribute(sys / "devices/system/cpu" / name, content, 16384U) &&
               parse_cpu_list(content, values);
    };
    const bool possible_ok = read_cpu_set("possible", snapshot->possible_cpus);
    const bool present_ok = read_cpu_set("present", snapshot->present_cpus);
    const bool online_ok = read_cpu_set("online", snapshot->online_cpus);
    snapshot->cpu_topology_valid = possible_ok && present_ok && online_ok &&
        valid_subset(snapshot->present_cpus, snapshot->possible_cpus) &&
        valid_subset(snapshot->online_cpus, snapshot->present_cpus);

    const fs::path cpu_root = sys / "devices/system/cpu/cpufreq";
    for (const auto& dir : directory_entries(cpu_root, "policy")) {
        CpuPolicyProfile policy{};
        policy.policy_path = dir.string();
        if (read_attribute(dir / "related_cpus", raw, 16384U) ||
            read_attribute(dir / "affected_cpus", raw, 16384U)) {
            policy.related_cpus_valid = parse_cpu_list(raw, policy.related_cpus) &&
                (!snapshot->cpu_topology_valid || valid_subset(policy.related_cpus, snapshot->present_cpus));
        }
        (void)read_u64(dir / "cpuinfo_min_freq", policy.cpuinfo_min_freq_khz);
        (void)read_u64(dir / "cpuinfo_max_freq", policy.cpuinfo_max_freq_khz);
        (void)read_u64(dir / "scaling_min_freq", policy.scaling_min_freq_khz);
        (void)read_u64(dir / "scaling_max_freq", policy.scaling_max_freq_khz);
        (void)read_u64(dir / "scaling_cur_freq", policy.scaling_cur_freq_khz);
        if (read_attribute(dir / "scaling_governor", raw, 128U)) policy.governor = trim(raw);
        std::vector<uint64_t> table;
        if (read_attribute(dir / "scaling_available_frequencies", raw, 8192U) ||
            read_attribute(dir / "cpuinfo_available_frequencies", raw, 8192U)) {
            policy.frequency_table_valid = parse_frequency_table(raw, table);
            if (policy.frequency_table_valid) policy.available_frequencies_khz = table;
        }
        const bool valid_bounds = policy.cpuinfo_min_freq_khz > 0 &&
            policy.cpuinfo_max_freq_khz >= policy.cpuinfo_min_freq_khz &&
            policy.scaling_min_freq_khz >= policy.cpuinfo_min_freq_khz &&
            policy.scaling_max_freq_khz <= policy.cpuinfo_max_freq_khz &&
            policy.scaling_min_freq_khz <= policy.scaling_max_freq_khz;
        const bool table_contains_active = policy.frequency_table_valid &&
            std::binary_search(policy.available_frequencies_khz.begin(),
                               policy.available_frequencies_khz.end(), policy.scaling_max_freq_khz);
        const fs::path max_path = dir / "scaling_max_freq";
        const bool eligible = snapshot->cpu_topology_valid && policy.related_cpus_valid &&
            valid_bounds && table_contains_active && policy.scaling_max_freq_khz > 0;
        auto cap = observe_capability(max_path, HardwareDomain::Cpu, ProfileSource::Sysfs,
            "cpufreq.scaling_max_freq", "kHz",
            "cpuinfo_min_freq<=target<=cpuinfo_max_freq;target in scaling_available_frequencies",
            true, valid_bounds && table_contains_active, eligible,
            "Linux CPUFreq sysfs ABI; exact policy membership, bounds and advertised table checked");
        if (!eligible && cap.rejection_reason.empty())
            cap.rejection_reason = "CPU topology, policy membership, bounds or frequency table preflight failed";
        snapshot->node_capabilities.push_back(std::move(cap));

        // Governor-specific rate-limit controls are observed but never promoted
        // without a verified driver-specific ABI and accepted range contract.
        if (!policy.governor.empty()) {
            for (const char* tunable : {"up_rate_limit_us", "down_rate_limit_us"}) {
                const fs::path path = dir / policy.governor / tunable;
                NodeCapability governor_cap{};
                governor_cap.path = path.string();
                governor_cap.domain = HardwareDomain::Cpu;
                governor_cap.source = ProfileSource::Sysfs;
                governor_cap.semantic_id = "cpufreq.governor_tunable.unverified";
                governor_cap.unit = "us";
                governor_cap.evidence = "Governor tunable observed; vendor ABI and accepted range not verified";
                if (cddp_path_exists(path)) {
                    governor_cap.state = NodeCapabilityState::Available;
                    if (read_attribute(path, raw, 256U)) {
                        governor_cap.current_value = trim(raw);
                        governor_cap.state = NodeCapabilityState::Readable;
                        if (writable_by_mode_and_process(path)) governor_cap.state = NodeCapabilityState::Writable;
                    }
                }
                governor_cap.rejection_reason = "governor tunable constraints are unverified; observation-only";
                snapshot->node_capabilities.push_back(std::move(governor_cap));
            }
        }
        snapshot->cpu_policies.push_back(std::move(policy));
    }

    const fs::path devfreq_root = sys / "class/devfreq";
    for (const auto& dir : directory_entries(devfreq_root)) {
        const std::string name = dir.filename().string();
        std::string driver;
        if (read_attribute(dir / "name", raw, 256U)) driver = trim(raw);
        if (driver.empty()) driver = name;
        std::string lowered = name + " " + driver;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lowered.find("gpu") == std::string::npos &&
            lowered.find("mali") == std::string::npos &&
            lowered.find("adreno") == std::string::npos &&
            lowered.find("kgsl") == std::string::npos) continue;

        GpuDomainProfile gpu{};
        gpu.device_path = dir.string();
        gpu.driver = driver;
        (void)read_u64(dir / "cur_freq", gpu.cur_freq_hz);
        (void)read_u64(dir / "min_freq", gpu.min_freq_hz);
        (void)read_u64(dir / "max_freq", gpu.max_freq_hz);
        std::vector<uint64_t> table;
        if (read_attribute(dir / "available_frequencies", raw, 8192U)) {
            gpu.frequency_table_valid = parse_frequency_table(raw, table);
            if (gpu.frequency_table_valid) gpu.available_frequencies_hz = table;
        }
        if (read_attribute(dir / "governor", raw, 128U)) gpu.governor = trim(raw);
        const bool recognized_driver = lowered.find("mali") != std::string::npos ||
            lowered.find("adreno") != std::string::npos || lowered.find("gpu") != std::string::npos;
        const bool valid_bounds = gpu.min_freq_hz > 0 && gpu.max_freq_hz >= gpu.min_freq_hz;
        const bool table_contains_active = gpu.frequency_table_valid &&
            std::binary_search(gpu.available_frequencies_hz.begin(),
                               gpu.available_frequencies_hz.end(), gpu.max_freq_hz);
        const bool eligible = recognized_driver && valid_bounds && table_contains_active && gpu.max_freq_hz > 0;
        if (recognized_driver) {
            auto cap = observe_capability(dir / "max_freq", HardwareDomain::Gpu, ProfileSource::Sysfs,
                "devfreq.max_freq", "Hz",
                "min_freq<=target<=max_freq;target in available_frequencies",
                true, valid_bounds && table_contains_active, eligible,
                "Linux devfreq generic ABI; GPU identity, bounds and advertised frequency table checked");
            if (!eligible && cap.rejection_reason.empty())
                cap.rejection_reason = "GPU identity, bounds or frequency table preflight failed";
            snapshot->node_capabilities.push_back(std::move(cap));
        }
        snapshot->gpu_domains.push_back(std::move(gpu));
    }

    // KGSL remains visible but observation-only until its vendor ABI is proven.
    const fs::path kgsl = sys / "class/kgsl/kgsl-3d0";
    if (cddp_path_exists(kgsl)) {
        GpuDomainProfile gpu{};
        gpu.device_path = kgsl.string();
        gpu.driver = "kgsl-observation-only";
        snapshot->gpu_domains.push_back(std::move(gpu));
        for (const char* attr : {"gpuclk", "max_gpuclk", "min_gpuclk", "max_gpu_freq", "min_gpu_freq"}) {
            const fs::path path = kgsl / attr;
            if (!cddp_path_exists(path)) continue;
            NodeCapability cap{};
            cap.path = path.string();
            cap.domain = HardwareDomain::Gpu;
            cap.source = ProfileSource::Sysfs;
            cap.semantic_id = "kgsl.vendor_control.unverified";
            cap.evidence = "KGSL node observed without portable verified mutation contract";
            if (read_attribute(path, raw, 256U)) { cap.current_value = trim(raw); cap.state = NodeCapabilityState::Readable; }
            else cap.state = NodeCapabilityState::Available;
            cap.rejection_reason = "KGSL control is observation-only in this CDDP generation";
            snapshot->node_capabilities.push_back(std::move(cap));
        }
    }

    const fs::path thermal_root = sys / "class/thermal";
    for (const auto& zone : directory_entries(thermal_root, "thermal_zone")) {
        ThermalSensorProfile sensor{};
        sensor.sensor_path = zone.string();
        if (read_attribute(zone / "type", raw, 256U)) sensor.sensor_type = trim(raw);
        uint64_t temp = 0;
        if (read_u64(zone / "temp", temp) && temp <= 200000U) {
            sensor.temperature_millicelsius = static_cast<int64_t>(temp);
            sensor.temperature_valid = true;
        }
        snapshot->thermal_sensors.push_back(std::move(sensor));
    }
    const fs::path hwmon_root = sys / "class/hwmon";
    for (const auto& hwmon : directory_entries(hwmon_root, "hwmon")) {
        std::string chip;
        if (read_attribute(hwmon / "name", chip, 256U)) chip = trim(chip);
        std::error_code ec;
        for (fs::directory_iterator it(hwmon, fs::directory_options::skip_permission_denied, ec), end;
             !ec && it != end; it.increment(ec)) {
            const std::string filename = it->path().filename().string();
            if (filename.rfind("temp", 0) != 0 || filename.find("_input") == std::string::npos) continue;
            uint64_t temp = 0;
            if (!read_u64(it->path(), temp) || temp > 200000U) continue;
            ThermalSensorProfile sensor{};
            sensor.sensor_path = it->path().string();
            sensor.sensor_type = chip.empty() ? filename : chip + "/" + filename;
            sensor.temperature_millicelsius = static_cast<int64_t>(temp);
            sensor.temperature_valid = true;
            snapshot->thermal_sensors.push_back(std::move(sensor));
        }
    }

    if (read_attribute(proc / "meminfo", raw)) {
        snapshot->memory_telemetry_available = raw.find("MemTotal:") != std::string::npos &&
            raw.find("MemAvailable:") != std::string::npos;
    }
    std::string mountinfo;
    const bool mountinfo_read = read_attribute(proc / "self/mountinfo", mountinfo);
    snapshot->cgroup_v2_available = cddp_path_exists(sys / "fs/cgroup/cgroup.controllers") ||
        (mountinfo_read && mountinfo.find(" - cgroup2 ") != std::string::npos);
    snapshot->uclamp_interface_available = cddp_path_exists(sys / "fs/cgroup/cpu.uclamp.min") ||
        cddp_path_exists(sys / "fs/cgroup/top-app/cpu.uclamp.min") ||
        cddp_path_exists(sys / "fs/cgroup/top-app/cpu.uclamp.max");
    snapshot->fingerprint = make_fingerprint(*snapshot);
    return snapshot;
}

DeviceTopology DeviceProfiler::probe() {
    const auto current = profile();
    DeviceTopology topo{};
    topo.tier = DeviceTier::Entry; // Legacy presentation default only; no CPU-count tier inference.
    topo.total_cpus = static_cast<uint32_t>(current->present_cpus.size());
    topo.online_cpus_mask = join_cpu_ids(current->online_cpus);
    topo.kernel_version = current->kernel_release;
    topo.soc_name = !current->soc_model.empty() ? current->soc_model :
        (!current->device_model.empty() ? current->device_model : "Unknown SoC");
    topo.hardware_platform = current->hardware_platform;
    topo.has_cgroups_v2 = current->cgroup_v2_available;
    topo.has_uclamp = current->uclamp_interface_available;
    topo.has_surfaceflinger_hints = false;
    topo.gpu_type = current->gpu_domains.empty() ? "Unavailable" : current->gpu_domains.front().driver;
    for (const auto& policy : current->cpu_policies) {
        CpuCluster cluster{};
        cluster.cpufreq_policy_path = policy.policy_path;
        cluster.first_cpu = policy.related_cpus.empty() ? 0U : policy.related_cpus.front();
        cluster.last_cpu = policy.related_cpus.empty() ? 0U : policy.related_cpus.back();
        cluster.base_freq_khz = static_cast<uint32_t>(std::min<uint64_t>(policy.cpuinfo_min_freq_khz, UINT32_MAX));
        cluster.max_freq_khz = static_cast<uint32_t>(std::min<uint64_t>(policy.cpuinfo_max_freq_khz, UINT32_MAX));
        cluster.type = ClusterType::Medium;
        topo.clusters.push_back(std::move(cluster));
    }
    std::sort(topo.clusters.begin(), topo.clusters.end(), [](const auto& a, const auto& b) {
        return a.max_freq_khz < b.max_freq_khz;
    });
    if (!topo.clusters.empty()) {
        topo.clusters.front().type = ClusterType::Little;
        if (topo.clusters.size() > 1U) topo.clusters.back().type = ClusterType::BigPrime;
    }
    { std::lock_guard<std::mutex> lock(mutex_); cached_topology_ = topo; }
    return topo;
}
float DeviceProfiler::get_soc_temperature() const { return ThermalMonitor::instance().hottest_celsius(); }
std::vector<uint32_t> DeviceProfiler::get_cluster_frequencies() const {
    const auto current = const_cast<DeviceProfiler*>(this)->profile();
    std::vector<uint32_t> frequencies;
    frequencies.reserve(current->cpu_policies.size());
    for (const auto& policy : current->cpu_policies) {
        const uint64_t value = policy.scaling_cur_freq_khz > 0 ?
            policy.scaling_cur_freq_khz : policy.scaling_max_freq_khz;
        frequencies.push_back(static_cast<uint32_t>(std::min<uint64_t>(value, UINT32_MAX)));
    }
    return frequencies;
}
} // namespace coreflow
