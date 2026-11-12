/**
 * @file device_profile.hpp
 * @brief Immutable, OS-derived hardware and mutation-capability snapshot.
 * @license Apache-2.0
 *
 * A profile is a read-only observation of one device generation. It is not
 * permission to mutate a node: mutation requires the complete capability
 * evidence chain and controller-side ownership/baseline checks.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace coreflow {

enum class NodeCapabilityState : uint8_t {
    Discovered = 0,
    Available = Discovered, // Compatibility alias for existing profile consumers.
    Readable = 1,
    AccessChecked = 2,
    Writable = AccessChecked, // Compatibility alias; access check is not mutation authority.
    SemanticsVerified = 3,
    ConstraintsVerified = 4,
    MutationEligible = 5,
};

enum class NodeRejectionReason : uint8_t {
    None = 0,
    EaccesSelinux,
    NodeMissing,
    FormatMismatch,
    BaselineCaptureFailed,
    BaselineNotCaptured,
    ReadbackMismatch,
    PermissionDenied,
    SemanticsUnverified,
    ConstraintsUnverified,
    AmbiguousTarget,
    IoError,
};

enum class ProfileSource : uint8_t {
    Sysfs,
    Procfs,
    DeviceTree,
    AndroidProperty,
    KernelApi,
    Unknown,
};

enum class HardwareDomain : uint8_t {
    Cpu,
    Gpu,
    Thermal,
    Memory,
    Storage,
    Cgroup,
    AndroidRuntime,
    Graphics,
    Power,
    Unknown,
};

struct CpuPolicyProfile {
    std::string policy_path;
    std::vector<uint32_t> related_cpus;
    std::vector<uint64_t> available_frequencies_khz;
    uint64_t cpuinfo_min_freq_khz{0};
    uint64_t cpuinfo_max_freq_khz{0};
    uint64_t scaling_min_freq_khz{0};
    uint64_t scaling_max_freq_khz{0};
    uint64_t scaling_cur_freq_khz{0};
    std::string governor;
    bool related_cpus_valid{false};
    bool frequency_table_valid{false};
};

struct GpuDomainProfile {
    std::string device_path;
    std::string driver;
    std::string governor;
    std::vector<uint64_t> available_frequencies_hz;
    uint64_t min_freq_hz{0};
    uint64_t max_freq_hz{0};
    uint64_t cur_freq_hz{0};
    bool frequency_table_valid{false};
};

struct ThermalSensorProfile {
    std::string sensor_path;
    std::string sensor_type;
    int64_t temperature_millicelsius{0};
    bool temperature_valid{false};
};

struct NodeCapability {
    std::string path;
    HardwareDomain domain{HardwareDomain::Unknown};
    ProfileSource source{ProfileSource::Unknown};
    NodeCapabilityState state{NodeCapabilityState::Available};

    // Semantic identity and units must come from a known ABI/interface contract,
    // never from a writable bit or a filename guess.
    std::string semantic_id;
    std::string unit;
    std::string current_value;
    std::string accepted_values;
    std::string constraints;
    std::string evidence;
    std::string rejection_reason;
    NodeRejectionReason rejection_code{NodeRejectionReason::None};

    [[nodiscard]] bool mutation_eligible() const noexcept {
        return state == NodeCapabilityState::MutationEligible &&
               !path.empty() && !semantic_id.empty() && !unit.empty() &&
               !constraints.empty();
    }
};

struct DeviceProfileSnapshot final {
    uint64_t generation{0};
    uint64_t captured_monotonic_ns{0};
    std::string fingerprint;
    std::string sys_root{"/sys"};
    std::string proc_root{"/proc"};
    std::string kernel_release;
    std::string android_release;
    uint32_t android_api_level{0};

    // Identity is descriptive only; tuning decisions use capabilities below.
    std::string device_model;
    std::string soc_model;
    std::string hardware_platform;

    std::vector<uint32_t> possible_cpus;
    std::vector<uint32_t> present_cpus;
    std::vector<uint32_t> online_cpus;
    std::vector<CpuPolicyProfile> cpu_policies;
    std::vector<GpuDomainProfile> gpu_domains;
    std::vector<ThermalSensorProfile> thermal_sensors;
    std::vector<NodeCapability> node_capabilities;

    // Generic VM and block-queue controls discovered from procfs/sysfs.
    // Writable state is evidence only; controllers still require semantic,
    // range, PSI-admission, lease and rollback validation.
    uint32_t infrastructure_nodes_scanned{0};
    uint32_t infrastructure_nodes_writable{0};

    bool cpu_topology_valid{false};
    bool memory_telemetry_available{false};
    bool cgroup_v2_available{false};
    bool uclamp_interface_available{false};
};

// Published profiles are shared as shared_ptr<const DeviceProfileSnapshot>.
// The builder owns the only mutable instance and must not retain a mutable
// alias after publication. Controllers retain one generation for a decision
// cycle and request a new snapshot when the profiler publishes a refresh.
using DeviceProfileHandle = std::shared_ptr<const DeviceProfileSnapshot>;

[[nodiscard]] inline bool capability_state_at_least(
    NodeCapabilityState actual, NodeCapabilityState required) noexcept {
    return static_cast<uint8_t>(actual) >= static_cast<uint8_t>(required);
}

} // namespace coreflow
