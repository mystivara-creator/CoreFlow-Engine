#include "coreflow/high_capability.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace coreflow {
namespace {

struct MatchRule {
    ExpandedDomain id;
    std::array<const char*, 6> tokens;
};

constexpr std::array<MatchRule, 15> kRules{{
    {ExpandedDomain::CpuIdle, {"cpuidle", "cpu.idle", "idle_state", nullptr, nullptr}},
    {ExpandedDomain::Devfreq, {"devfreq", "devfreq.", nullptr, nullptr, nullptr}},
    {ExpandedDomain::GpuFrequency, {"gpu", "kgsl", "mali", "gpuclk", nullptr}},
    {ExpandedDomain::ThermalIntelligence, {"thermal", "cooling_device", "trip_point", nullptr, nullptr}},
    {ExpandedDomain::PressureStall, {"pressure", "psi", "some avg10", "full avg10", nullptr}},
    {ExpandedDomain::Zram, {"zram", "swap", "mm_stat", nullptr, nullptr}},
    {ExpandedDomain::MemoryReclaim, {"vm.", "compact", "reclaim", "pgscan", "pgsteal"}},
    {ExpandedDomain::SchedulerTelemetry, {"sched", "scheduler", "runqueue", "runnable", nullptr}},
    {ExpandedDomain::CpuTopology, {"cpu.present", "cpu.possible", "cpu.online", "related_cpus", "cpu_capacity"}},
    {ExpandedDomain::FilesystemIo, {"block", "queue", "read_ahead_kb", "nr_requests", "io.stat"}},
    {ExpandedDomain::Writeback, {"writeback", "dirty", "wb_", "nr_dirty", "nr_writeback"}},
    {ExpandedDomain::TaskProfiles, {"task_profiles", "task_profiles.json", "cgroup.controllers", "cpuctl", nullptr}},
    {ExpandedDomain::PowerState, {"powercap", "/sys/power", "power_state", "autosuspend", "energy_uj"}},
    {ExpandedDomain::NetworkTelemetry, {"/sys/class/net", "/proc/net", "netdev", "tcp", "network", nullptr}},
    {ExpandedDomain::BatteryEnergy, {"/sys/class/power_supply", "ps.", "battery", "current_now", "voltage_now"}},
}};

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool matches(const ResourceCapability& resource, const MatchRule& rule) {
    const std::string name = lower(resource.name);
    const std::string path = lower(resource.path);
    for (const char* token : rule.tokens) {
        if (token == nullptr) continue;
        const std::string needle(token);
        if (name.find(needle) != std::string::npos ||
            path.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

const std::array<ExpandedDomainDescriptor, 15>&
HighCapabilityExpansion::catalog() noexcept {
    static constexpr std::array<ExpandedDomainDescriptor, 15> entries{{
        {ExpandedDomain::CpuIdle, "CPU Idle", "Idle-state availability and residency", true},
        {ExpandedDomain::Devfreq, "Devfreq", "Dynamic frequency scaling devices", true},
        {ExpandedDomain::GpuFrequency, "GPU Frequency", "GPU clock and utilization surfaces", true},
        {ExpandedDomain::ThermalIntelligence, "Thermal Intelligence", "Thermal zones and cooling state", true},
        {ExpandedDomain::PressureStall, "PSI", "CPU, memory and I/O pressure signals", true},
        {ExpandedDomain::Zram, "zRAM", "Compressed swap device and activity", true},
        {ExpandedDomain::MemoryReclaim, "Memory Reclaim", "VM reclaim and compaction signals", true},
        {ExpandedDomain::SchedulerTelemetry, "Scheduler Telemetry", "Scheduler controls and exported statistics", true},
        {ExpandedDomain::CpuTopology, "CPU Topology", "Present, possible, online and isolated CPUs", true},
        {ExpandedDomain::FilesystemIo, "Filesystem I/O", "Block-device and queue observations", true},
        {ExpandedDomain::Writeback, "Writeback", "Dirty-page and writeback controls", true},
        {ExpandedDomain::TaskProfiles, "Task Profiles", "Android task-profile surface", true},
        {ExpandedDomain::PowerState, "Power State", "Powercap and device power-state surfaces", true},
        {ExpandedDomain::NetworkTelemetry, "Network Telemetry", "Network interface and protocol telemetry", true},
        {ExpandedDomain::BatteryEnergy, "Battery & Energy", "Battery, charging, current and voltage signals", true},
    }};
    return entries;
}

std::vector<ExpandedDomainStatus> HighCapabilityExpansion::assess(
    const EnvironmentCapabilityMatrix& matrix) {
    std::vector<ExpandedDomainStatus> result;
    result.reserve(kRules.size());

    for (const auto& rule : kRules) {
        ExpandedDomainStatus status;
        status.id = rule.id;

        for (const auto& resource : matrix.resources) {
            if (!matches(resource, rule)) {
                continue;
            }
            ++status.matched_resources;
            if (resource.exists && resource.readable && resource.runtime_verified) {
                ++status.readable_resources;
            }
        }

        if (status.matched_resources != 0U) {
            status.evidence = status.readable_resources != 0U
                ? CapabilityEvidence::Observable : CapabilityEvidence::Detected;
        }
        // Do not infer write permission from filesystem access or discovery.
        status.mutation_authorized = false;
        result.push_back(status);
    }
    return result;
}

const char* HighCapabilityExpansion::evidenceName(
    CapabilityEvidence evidence) noexcept {
    switch (evidence) {
        case CapabilityEvidence::Unavailable: return "unavailable";
        case CapabilityEvidence::Detected: return "detected";
        case CapabilityEvidence::Observable: return "observable";
        default: return "unknown";
    }
}

} // namespace coreflow
