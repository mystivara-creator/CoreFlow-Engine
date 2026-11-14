/**
 * @file types.hpp
 * @brief CFAE System Framework v4.0.0 ISO C++23 Core Type Definitions
 *        and Coordinated Mutation Transaction Models.
 * @author mystivara-creator
 * @license MIT
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <optional>
#include <system_error>
#include <cerrno>
#include <cstdlib>
#include <cmath>
#include <limits>

namespace coreflow {

// ============================================================================
// Robust ASCII Numeric Parsing (Noexcept & Safe)
// ============================================================================
inline bool parse_ascii_double(std::string_view text, double& value) noexcept {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return false;
    const size_t last = text.find_last_not_of(" \t\r\n");
    std::string_view trimmed = text.substr(first, last - first + 1);
    if (trimmed.empty()) return false;

    char* endptr = nullptr;
    std::string s(trimmed);
    errno = 0;
    double d = std::strtod(s.c_str(), &endptr);
    if (endptr == s.c_str() || *endptr != '\0' || errno == ERANGE || !std::isfinite(d)) return false;
    value = d;
    return true;
}

// ============================================================================
// Coordinated Mutation Transaction & Safety Model (v4.0.0)
// ============================================================================
enum class DependencyRelation : uint8_t {
    RequiredBefore = 0, ///< Control A must be applied before Control B
    Requires = 1,       ///< Control B is valid only if prerequisite A is satisfied
    ConflictsWith = 2,  ///< Controls A and B must never be active simultaneously
    CoupledWith = 3,    ///< Verified semantic coupling; may be evaluated as a group
    Unknown = 4         ///< Unverified relationship; never assume safe to group
};

enum class RiskClass : uint8_t {
    Low = 0,       ///< Strong semantics, bounded impact, reversible, high observability
    Medium = 1,    ///< Understood interaction & rollback; applied step-by-step in transaction
    High = 2,      ///< Broad impact or uncertain platform semantics; strictly isolated
    Forbidden = 3  ///< Missing baseline, unsafe rollback, ownership conflict, or policy violation
};

struct RiskDimensions {
    uint8_t semantic_uncertainty{0};
    uint8_t platform_uncertainty{0};
    uint8_t interaction_risk{0};
    uint8_t reversibility_cost{0};
    uint8_t system_impact{0};
    uint8_t observability_gap{0};
    uint8_t ownership_uncertainty{0};
};

enum class TransactionStage : uint8_t {
    Proposed = 0,
    AdmissionCheck = 1,
    DeferredOrRejected = 2,
    Prepared = 3,
    Applying = 4,
    ReadbackVerify = 5,
    RollingBack = 6,
    Observing = 7,
    DeferredReview = 8,
    Committed = 9,
    Retained = 10
};

enum class TransactionOutcome : uint8_t {
    Improved = 0,         ///< Measurable benefit with zero safety regression
    Neutral = 1,          ///< No material benefit observed
    Regressed = 2,        ///< Degradation or elevated risk detected
    Inconclusive = 3,     ///< Insufficient data or non-comparable workload context
    Aborted = 4,          ///< Halted by safety invariant or thermal guard
    OwnershipLost = 5,    ///< External OEM/Android process modified target value
    RecoveryRequired = 6  ///< Unresolved journal state requiring idempotent recovery
};

// ============================================================================
// Device Tier & Architecture Taxonomy
// ============================================================================
enum class DeviceTier : uint8_t {
    Entry,       ///< 4x LITTLE + 4x Mid (e.g. Snapdragon 680, Helio G99)
    MidRange,    ///< 4x Efficiency + 4x Performance (e.g. Snapdragon 6 Gen 3 'Kunzite')
    UpperMid,    ///< High IPC 4+4 or 1+3+4 (e.g. Dimensity 8300, Snapdragon 7+ Gen 3)
    Flagship     ///< Tri/Quad-cluster Prime (e.g. Snapdragon 8 Gen 3/4, Dimensity 9300)
};

enum class WorkloadState : uint8_t {
    Balanced = 0,
    SustainedLoad = 1,
    PressureGuard = 2,
    ThermalGuard = 3,

    // Aliases for compatibility with different submodules
    Interactive = Balanced,
    DailyIdle = Balanced,
    DeepSleep = Balanced,
    BurstLaunch = SustainedLoad,
    SustainedGame = SustainedLoad,
    ThermalThrottled = ThermalGuard
};

enum class ClusterType : uint8_t {
    Little,      ///< High efficiency, background tasks
    Medium,      ///< Balanced performance, UI rendering
    BigPrime     ///< Max IPC, burst launches, heavy frames
};

// ============================================================================
// Hardware Topology Descriptors
// ============================================================================
struct CpuCluster {
    ClusterType type;
    uint32_t first_cpu;
    uint32_t last_cpu;
    uint32_t base_freq_khz;
    uint32_t max_freq_khz;
    std::string cpufreq_policy_path;
    std::string energy_model_path;
};

struct DeviceTopology {
    std::string soc_name;
    std::string hardware_platform;
    DeviceTier tier;
    uint32_t total_cpus;
    std::string online_cpus_mask{"0-7"};
    std::string kernel_version;
    std::string primary_thermal_zone_type;
    std::string battery_control_node;
    std::string gpu_type;
    std::vector<CpuCluster> clusters;
    bool has_cgroups_v2{false};
    bool has_uclamp{false};
    bool has_surfaceflinger_hints{false};
};

// ============================================================================
// Uclamp & Scheduling Constants (Linux kernel 5.10+ / 5.15+)
// Reference: Documentation/scheduler/sched-util-clamping.rst
// Scale: 0 to 1024 (SCHED_CAPACITY_SCALE)
// ============================================================================
struct UclampValues {
    uint32_t uclamp_min{0};    ///< Guaranteed minimum capacity (0 - 1024)
    uint32_t uclamp_max{1024}; ///< Hard frequency ceiling (0 - 1024)
    bool boost_render_thread{false};
};

// ============================================================================
// Cgroup v2 Migration Targets
// ============================================================================
enum class CgroupTarget : uint8_t {
    TopApp,      ///< Semantic target resolved from runtime cgroup topology.
    Foreground,  ///< Semantic target resolved from runtime cgroup topology.
    Background,  ///< Semantic target resolved from runtime cgroup topology.
    System       ///< Semantic target resolved from runtime cgroup topology.
};

// ============================================================================
// Error Codes for Zero-Exception Native Execution
// ============================================================================
enum class CoreFlowError {
    Success = 0,
    NodeNotFound,
    PermissionDenied,
    EpollCreateFailed,
    EpollCtlFailed,
    InvalidTopology,
    BaselineSnapshotFailed,
    RollbackFailed,
    ThermalEmergency
};

std::error_code make_error_code(CoreFlowError e);

} // namespace coreflow
