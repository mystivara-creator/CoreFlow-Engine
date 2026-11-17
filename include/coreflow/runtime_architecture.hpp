/**
 * @file runtime_architecture.hpp
 * @brief Zero-fixed-tuning runtime architecture contract for CoreFlow.
 *
 * This header defines the boundary between discovery, capability/state,
 * deterministic policy, mutation, and outcome verification.  It deliberately
 * contains no device-specific node paths, vendor identifiers, cluster indexes,
 * tuning presets, or default operating-point values.
 *
 * C++20.  Filesystem roots are supplied by the platform composition root;
 * individual control-node paths are produced only by discovery.
 */
#pragma once

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace coreflow::runtime {

using Clock = std::chrono::steady_clock;
using Timestamp = Clock::time_point;
using NodeId = std::string;
using MetricId = std::string;
using DomainId = std::string;
using TransactionId = std::string;

using NodeValue = std::variant<std::monostate, std::int64_t, std::uint64_t,
                               double, bool, std::string>;

enum class ValueKind : std::uint8_t {
    Unknown,
    SignedInteger,
    UnsignedInteger,
    FloatingPoint,
    Boolean,
    Enumeration,
    Text
};

enum class AccessState : std::uint8_t {
    Unknown,
    Denied,
    ReadOnly,
    Writable
};

enum class ProbeState : std::uint8_t {
    NotAttempted,
    Available,
    PermissionDenied,
    Missing,
    InvalidType,
    IoError
};

enum class NodeDomain : std::uint8_t {
    Unknown,
    Cpu,
    Gpu,
    Memory,
    Storage,
    Scheduler,
    Thermal,
    Power,
    Kernel,
    Other
};

struct FileIdentity final {
    std::uint64_t device{};
    std::uint64_t inode{};
    [[nodiscard]] constexpr bool valid() const noexcept {
        return device != 0U || inode != 0U;
    }
};

struct NumericRange final {
    long double minimum{};
    long double maximum{};
    std::optional<long double> step{};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(static_cast<double>(minimum)) &&
               std::isfinite(static_cast<double>(maximum)) && maximum > minimum &&
               (!step.has_value() ||
                (std::isfinite(static_cast<double>(*step)) && *step > 0.0L));
    }
};

struct NodeDescriptor final {
    NodeId id;                              // Opaque ID created by discovery.
    std::filesystem::path discovered_path; // Never synthesized by a policy.
    DomainId domain_id;
    NodeDomain domain{NodeDomain::Unknown};
    std::string semantic;                   // Resolved semantic role, not a path.
    ValueKind value_kind{ValueKind::Unknown};
    AccessState access{AccessState::Unknown};
    ProbeState read_probe{ProbeState::NotAttempted};
    ProbeState write_probe{ProbeState::NotAttempted};
    FileIdentity identity{};
    NodeValue observed_value{};
    std::optional<NumericRange> numeric_range{};
    std::vector<NodeValue> advertised_values{};
    Timestamp discovered_at{};
};

struct DiscoveryRoots final {
    // Mount roots are platform inputs (normally supplied by the OS adapter),
    // not individual node paths and not tuning assumptions.
    std::filesystem::path sysfs_root;
    std::filesystem::path procfs_root;
};

struct DiscoveryIssue final {
    std::string source;
    ProbeState reason{ProbeState::IoError};
    std::string detail;
};

struct DeviceManifest final {
    Timestamp generated_at{};
    std::string boot_identity;
    std::vector<NodeDescriptor> nodes;
    std::vector<DiscoveryIssue> issues;

    [[nodiscard]] const NodeDescriptor* find(std::string_view id) const noexcept {
        for (const auto& node : nodes) {
            if (node.id == id) return &node;
        }
        return nullptr;
    }
};

struct DiscoveryResult final {
    DeviceManifest manifest;
    bool enumeration_completed{}; // False means partial discovery, not daemon failure.
};

class IHardwareDiscovery {
public:
    virtual ~IHardwareDiscovery() = default;
    [[nodiscard]] virtual DiscoveryResult discover(const DiscoveryRoots& roots) noexcept = 0;
};

struct OemBaselineEntry final {
    NodeId node_id;
    NodeValue value;
    FileIdentity identity{};
    Timestamp captured_at{};
};

struct OemBaseline final {
    Timestamp captured_at{};
    std::string boot_identity;
    std::vector<OemBaselineEntry> entries;

    [[nodiscard]] const OemBaselineEntry* find(std::string_view id) const noexcept {
        for (const auto& entry : entries) {
            if (entry.node_id == id) return &entry;
        }
        return nullptr;
    }
};

struct RawTelemetrySample final {
    MetricId metric_id;
    NodeValue value;
    Timestamp sampled_at{};
    std::optional<NumericRange> discovered_range{};
    bool valid{};
};

struct TelemetrySnapshot final {
    Timestamp sampled_at{};
    std::vector<RawTelemetrySample> samples;
};

struct NormalizedMetric final {
    MetricId metric_id;
    double value{};       // Valid values are always within [0.0, 1.0].
    double confidence{};  // Valid values are always within [0.0, 1.0].
    Timestamp sampled_at{};
    bool valid{};

    [[nodiscard]] bool well_formed() const noexcept {
        return valid && std::isfinite(value) && value >= 0.0 && value <= 1.0 &&
               std::isfinite(confidence) && confidence >= 0.0 && confidence <= 1.0;
    }
};

struct NormalizedState final {
    Timestamp sampled_at{};
    std::vector<NormalizedMetric> metrics;

    [[nodiscard]] const NormalizedMetric* find(std::string_view id) const noexcept {
        for (const auto& metric : metrics) {
            if (metric.metric_id == id) return &metric;
        }
        return nullptr;
    }
};

struct CapabilitySnapshot final {
    Timestamp generated_at{};
    DeviceManifest manifest;
    OemBaseline baseline;
};

struct CapabilityStateResult final {
    CapabilitySnapshot capabilities;
    NormalizedState state;
};

class ICapabilityStateEngine {
public:
    virtual ~ICapabilityStateEngine() = default;
    // Capture each independently readable node. Missing/denied nodes are omitted.
    [[nodiscard]] virtual OemBaseline capture_oem_baseline(
        const DeviceManifest& manifest) noexcept = 0;
    // A metric is valid only when its type and normalization range are validated.
    // No synthetic range or default sensor value may be generated.
    [[nodiscard]] virtual NormalizedState normalize(
        const DeviceManifest& manifest,
        const TelemetrySnapshot& telemetry) noexcept = 0;
};

enum class OperatingMode : std::uint8_t {
    Unknown,
    Idle,
    Balanced,
    HighLoad,
    ThermalGuard
};

struct MetricEnvelope final {
    MetricId metric_id;
    double lower_quantile{};
    double median{};
    double upper_quantile{};
    std::uint64_t sample_count{};
    bool valid{};

    [[nodiscard]] bool well_formed() const noexcept {
        return valid && std::isfinite(lower_quantile) &&
               std::isfinite(median) && std::isfinite(upper_quantile) &&
               lower_quantile >= 0.0 && upper_quantile <= 1.0 &&
               lower_quantile <= median && median <= upper_quantile &&
               sample_count != 0U;
    }
};

struct AdaptiveReference final {
    Timestamp generated_at{};
    std::vector<MetricEnvelope> envelopes;
};

struct AlgorithmConstants final {
    // Only algorithm-stability and I/O-safety constants belong here.
    // Values are injected by the implementation/configuration layer; no device
    // tuning value or operating-mode preset is stored in this contract.
    double hysteresis_ratio{};
    double ewma_smoothing_factor{};
    std::size_t maximum_mutations_per_batch{};
    std::size_t maximum_node_read_bytes{};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(hysteresis_ratio) && hysteresis_ratio >= 0.0 &&
               hysteresis_ratio < 1.0 &&
               std::isfinite(ewma_smoothing_factor) &&
               ewma_smoothing_factor > 0.0 && ewma_smoothing_factor <= 1.0 &&
               maximum_node_read_bytes != 0U;
    }
};

struct ControlCapability final {
    NodeId node_id;
    ValueKind value_kind{ValueKind::Unknown};
    NumericRange supported_range{};
    NodeValue current_value{};
    NodeValue oem_value{};
    std::vector<NodeValue> advertised_values;
    bool writable{};
};

struct PolicyInput final {
    const DeviceManifest& manifest;
    const OemBaseline& baseline;
    const NormalizedState& state;
    const AdaptiveReference& adaptive_reference;
    std::span<const ControlCapability> controls;
    const AlgorithmConstants& algorithm;
};

struct PolicyAction final {
    NodeId node_id;
    NodeValue expected_current_value;
    NodeValue target_value; // Must be derived from discovered range/capability.
    std::string rationale;
};

struct PolicyIntent final {
    OperatingMode mode{OperatingMode::Unknown};
    Timestamp evaluated_at{};
    std::vector<PolicyAction> actions;
    std::vector<std::string> unavailable_evidence;
};

// Pure/deterministic policy boundary: no I/O, clocks, random source, global state,
// device paths, vendor checks, or preset target tables may be consulted here.
[[nodiscard]] PolicyIntent calculate_policy_intent(
    const PolicyInput& input, Timestamp evaluation_time) noexcept;

enum class NodeIoStatus : std::uint8_t {
    Ok,
    Missing,
    PermissionDenied,
    InvalidValue,
    IdentityChanged,
    Contended,
    IoError
};

struct NodeIoResult final {
    NodeIoStatus status{NodeIoStatus::IoError};
    NodeValue value{};
    FileIdentity identity{};
};

struct MutationRequest final {
    TransactionId transaction_id;
    NodeId node_id;
    FileIdentity expected_identity{};
    NodeValue expected_current_value;
    NodeValue target_value;
    std::string rationale;
};

enum class JournalStatus : std::uint8_t {
    Committed,
    JournalFault
};

class IWriteAheadJournal {
public:
    virtual ~IWriteAheadJournal() = default;
    // Must durably append and sync the complete eligible batch before any node write.
    [[nodiscard]] virtual JournalStatus append_and_sync(
        std::span<const MutationRequest> requests) noexcept = 0;
};

class INodeMutationPort {
public:
    virtual ~INodeMutationPort() = default;
    [[nodiscard]] virtual NodeIoResult write(
        const NodeDescriptor& node, const NodeValue& value,
        const FileIdentity& expected_identity) noexcept = 0;
    [[nodiscard]] virtual NodeIoResult read(
        const NodeDescriptor& node) noexcept = 0;
};

enum class ActuatorHealth : std::uint8_t {
    Healthy,
    SkippedUnavailable,
    PermissionDenied,
    WriteFailed,
    ReadbackFailed,
    ValueMismatch,
    OemContention,
    IdentityLost,
    Quarantined
};

struct VerificationResult final {
    ActuatorHealth health{ActuatorHealth::ReadbackFailed};
    bool accepted{};
};

class IOutcomeVerifier {
public:
    virtual ~IOutcomeVerifier() = default;
    // Implementations compare readback, expected state, inode identity, and OEM
    // ownership. OEM contention must quarantine the affected actuator, not fight it.
    [[nodiscard]] virtual VerificationResult verify(
        const NodeDescriptor& node,
        const MutationRequest& request,
        const NodeIoResult& readback,
        const OemBaselineEntry* oem_baseline) noexcept = 0;
};

class IActuatorHealthSink {
public:
    virtual ~IActuatorHealthSink() = default;
    virtual void record(const NodeId& node_id, ActuatorHealth health) noexcept = 0;
};

struct MutationNodeResult final {
    NodeId node_id;
    ActuatorHealth health{ActuatorHealth::SkippedUnavailable};
    bool attempted{};
};

struct MutationBatchResult final {
    JournalStatus journal_status{JournalStatus::Committed};
    bool global_mutation_stop{}; // True only for JournalFault.
    std::vector<MutationNodeResult> nodes;
};

// Reference orchestration that enforces write-ahead ordering and per-node
// isolation. One failed actuator never prevents independent eligible nodes
// from being attempted. JournalFault prevents every write in the batch.
class UnifiedMutationEngine final {
public:
    [[nodiscard]] bool mutation_blocked() const noexcept { return journal_fault_latched_; }

    [[nodiscard]] MutationBatchResult execute(
        const DeviceManifest& manifest, const OemBaseline& baseline,
        std::span<const MutationRequest> requests, IWriteAheadJournal& journal,
        INodeMutationPort& node_port, IOutcomeVerifier& verifier,
        IActuatorHealthSink& health_sink, const AlgorithmConstants& algorithm) noexcept {
        MutationBatchResult report;
        if (journal_fault_latched_) {
            report.journal_status = JournalStatus::JournalFault;
            report.global_mutation_stop = true;
            for (const auto& request : requests) {
                report.nodes.push_back({request.node_id, ActuatorHealth::Quarantined, false});
                health_sink.record(request.node_id, ActuatorHealth::Quarantined);
            }
            return report;
        }

        std::vector<const MutationRequest*> eligible;
        eligible.reserve(requests.size());
        for (const auto& request : requests) {
            const NodeDescriptor* node = manifest.find(request.node_id);
            bool rejected = node == nullptr || node->access != AccessState::Writable ||
                node->write_probe != ProbeState::Available || node->value_kind == ValueKind::Unknown;
            ActuatorHealth rejection = node != nullptr &&
                node->write_probe == ProbeState::PermissionDenied
                    ? ActuatorHealth::PermissionDenied : ActuatorHealth::SkippedUnavailable;
            if (node != nullptr && request.expected_identity.valid() && node->identity.valid() &&
                (request.expected_identity.device != node->identity.device ||
                 request.expected_identity.inode != node->identity.inode)) {
                rejected = true;
                rejection = ActuatorHealth::IdentityLost;
            }
            if (rejected) {
                report.nodes.push_back({request.node_id, rejection, false});
                health_sink.record(request.node_id, rejection);
                continue;
            }
            if (algorithm.maximum_mutations_per_batch != 0U &&
                eligible.size() >= algorithm.maximum_mutations_per_batch) {
                report.nodes.push_back({request.node_id, ActuatorHealth::SkippedUnavailable, false});
                health_sink.record(request.node_id, ActuatorHealth::SkippedUnavailable);
                continue;
            }
            eligible.push_back(&request);
        }
        if (eligible.empty()) return report;

        std::vector<MutationRequest> write_ahead_records;
        write_ahead_records.reserve(eligible.size());
        for (const MutationRequest* request : eligible) write_ahead_records.push_back(*request);
        report.journal_status = journal.append_and_sync(write_ahead_records);
        if (report.journal_status == JournalStatus::JournalFault) {
            journal_fault_latched_ = true;
            report.global_mutation_stop = true;
            for (const MutationRequest* request : eligible) {
                report.nodes.push_back({request->node_id, ActuatorHealth::Quarantined, false});
                health_sink.record(request->node_id, ActuatorHealth::Quarantined);
            }
            return report;
        }

        for (const MutationRequest* request : eligible) {
            const NodeDescriptor* node = manifest.find(request->node_id);
            if (node == nullptr) {
                report.nodes.push_back({request->node_id, ActuatorHealth::SkippedUnavailable, false});
                health_sink.record(request->node_id, ActuatorHealth::SkippedUnavailable);
                continue;
            }
            const NodeIoResult write_result = node_port.write(
                *node, request->target_value, request->expected_identity);
            if (write_result.status != NodeIoStatus::Ok) {
                ActuatorHealth health = ActuatorHealth::WriteFailed;
                switch (write_result.status) {
                    case NodeIoStatus::Missing: health = ActuatorHealth::SkippedUnavailable; break;
                    case NodeIoStatus::PermissionDenied: health = ActuatorHealth::PermissionDenied; break;
                    case NodeIoStatus::IdentityChanged: health = ActuatorHealth::IdentityLost; break;
                    case NodeIoStatus::Contended: health = ActuatorHealth::OemContention; break;
                    default: break;
                }
                report.nodes.push_back({request->node_id, health, true});
                health_sink.record(request->node_id, health);
                continue;
            }

            const NodeIoResult readback = node_port.read(*node);
            const OemBaselineEntry* baseline_entry = baseline.find(request->node_id);
            const VerificationResult verification =
                verifier.verify(*node, *request, readback, baseline_entry);
            const ActuatorHealth health = verification.accepted
                ? ActuatorHealth::Healthy
                : (verification.health == ActuatorHealth::Healthy
                    ? ActuatorHealth::ValueMismatch : verification.health);
            report.nodes.push_back({request->node_id, health, true});
            health_sink.record(request->node_id, health);
        }
        return report;
    }

private:
    // Latched for this daemon lifetime. Reconstruct only after durable recovery.
    bool journal_fault_latched_{};
};
} // namespace coreflow::runtime
