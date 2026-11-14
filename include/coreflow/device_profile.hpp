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
#include <string_view>
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

// A capability inventory is not a mutation allow-list.  This classification
// records the kernel ABI contract independently from current access bits.
// Only RuntimeTunable nodes with a verified range and captured baseline may
// be admitted by CandidateSelector.
enum class KernelNodeClassification : uint8_t {
    RuntimeTunable = 0,
    ObservationOnly = 1,
    TestOnly = 2,
    Protected = 3,
};

[[nodiscard]] inline constexpr std::string_view kernel_node_classification_name(
    KernelNodeClassification classification) noexcept {
    switch (classification) {
        case KernelNodeClassification::RuntimeTunable: return "RUNTIME_TUNABLE";
        case KernelNodeClassification::ObservationOnly: return "OBSERVATION_ONLY";
        case KernelNodeClassification::TestOnly: return "TEST_ONLY";
        case KernelNodeClassification::Protected: return "PROTECTED";
    }
    return "OBSERVATION_ONLY";
}

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
    std::string cluster_role;
    uint32_t capacity_rank{0};
    std::vector<std::string> available_governors;
    std::vector<std::string> governor_tunables;
    bool related_cpus_valid{false};
    bool frequency_table_valid{false};
};


struct BlockDeviceProfile {
    std::string device_name;
    std::string device_path;
    std::string model;
    std::string vendor;
    std::string transport_hint;
    std::string active_scheduler;
    std::vector<std::string> available_schedulers;
    std::vector<std::string> queue_tunables;
    std::vector<std::string> iosched_tunables;
    uint64_t capacity_bytes{0};
    uint32_t hardware_queues{0};
    bool rotational{false};
    bool rotational_valid{false};
};

struct ZramDeviceProfile {
    std::string device_name;
    std::string device_path;
    std::string active_algorithm;
    std::vector<std::string> available_algorithms;
    uint64_t disksize_bytes{0};
    bool swap_active{false};
};

struct MemoryHardwareProfile {
    uint64_t total_ram_bytes{0};
    std::vector<ZramDeviceProfile> zram_devices;
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
    KernelNodeClassification classification{KernelNodeClassification::ObservationOnly};

    // Semantic identity and units must come from a known ABI/interface contract,
    // never from a writable bit or a filename guess.
    std::string semantic_id;
    std::string unit;
    std::string current_value;
    std::string accepted_values;
    std::string constraints;
    std::string evidence;
    std::string rejection_reason;
    // Parent-policy relationship is descriptive; changing the parent requires
    // re-discovering children before any dependent mutation is admitted.
    std::string parent_policy_path;
    std::string parent_policy_value;
    std::string dependency_group;
    bool rediscover_after_parent_change{false};
    NodeRejectionReason rejection_code{NodeRejectionReason::None};

    [[nodiscard]] bool mutation_eligible() const noexcept {
        return classification == KernelNodeClassification::RuntimeTunable &&
               state == NodeCapabilityState::MutationEligible &&
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
    std::vector<BlockDeviceProfile> block_devices;
    MemoryHardwareProfile memory_hardware;
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
