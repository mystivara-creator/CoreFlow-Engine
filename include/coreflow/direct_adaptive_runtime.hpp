/**
 * @file direct_adaptive_runtime.hpp
 * @brief Production composition of discovery, telemetry, sweet-spot policy,
 *        durable mutation, verification and the daemon epoll reactor.
 */
#pragma once

#include "coreflow/baseline_store.hpp"
#include "coreflow/reactor.hpp"
#include "coreflow/runtime_architecture.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace coreflow::runtime {

struct DirectAdaptiveSnapshot final {
    bool started{};
    bool discovery_complete{};
    bool screen_off{};
    bool thermal_guard{};
    double cpu_utilization{};
    double thermal_pressure{};
    std::uint64_t ram_bytes{};
    std::uint64_t zram_bytes{};
    std::uint32_t refresh_hz{};
    std::string gpu_model{"unavailable"};
    std::string storage_type{"unavailable"};
    OperatingMode mode{OperatingMode::Unknown};
    std::size_t discovered_nodes{};
    std::size_t baseline_nodes{};
    std::size_t verified_mutations{};
    std::size_t skipped_nodes{};
    std::size_t ownership_losses{};
    bool journal_fault{};
    std::string status_json{"{}"};
};

class TopologyScanner final : public IHardwareDiscovery {
public:
    [[nodiscard]] DiscoveryResult discover(const DiscoveryRoots& roots) noexcept override;
};

class Telemetry final : public ICapabilityStateEngine {
public:
    explicit Telemetry(BaselineStore& baseline) noexcept : baseline_(baseline) {}
    [[nodiscard]] OemBaseline capture_oem_baseline(
        const DeviceManifest& manifest) noexcept override;
    [[nodiscard]] NormalizedState normalize(
        const DeviceManifest& manifest,
        const TelemetrySnapshot& telemetry) noexcept override;
    [[nodiscard]] TelemetrySnapshot sample(
        const DeviceManifest& manifest, bool& screen_off,
        std::uint64_t& ram_bytes, std::uint64_t& zram_bytes,
        std::uint32_t& refresh_hz, double& cpu_utilization) noexcept;

private:
    struct CpuCounters final {
        std::uint64_t total{};
        std::uint64_t idle{};
    };

    BaselineStore& baseline_;
    std::uint64_t previous_cpu_total_{};
    std::uint64_t previous_cpu_idle_{};
    std::unordered_map<unsigned, CpuCounters> previous_cpu_counters_;
};

class SweetSpotEngine final {
public:
    [[nodiscard]] PolicyIntent evaluate(
        const DeviceManifest& manifest, const OemBaseline& baseline,
        const NormalizedState& state, std::uint64_t ram_bytes,
        std::uint64_t zram_bytes, std::uint32_t refresh_hz,
        std::string_view storage_type, OperatingMode previous_mode,
        Timestamp evaluation_time) const noexcept;
};

class KernelActuator final : public IWriteAheadJournal,
                             public INodeMutationPort,
                             public IOutcomeVerifier,
                             public IActuatorHealthSink {
public:
    explicit KernelActuator(BaselineStore& baseline) noexcept : baseline_(baseline) {}

    [[nodiscard]] JournalStatus append_and_sync(
        std::span<const MutationRequest> requests) noexcept override;
    [[nodiscard]] NodeIoResult write(
        const NodeDescriptor& node, const NodeValue& expected_current_value,
        const NodeValue& target_value,
        const FileIdentity& expected_identity) noexcept override;
    [[nodiscard]] NodeIoResult read(const NodeDescriptor& node) noexcept override;
    [[nodiscard]] VerificationResult verify(
        const NodeDescriptor& node, const MutationRequest& request,
        const NodeIoResult& readback,
        const OemBaselineEntry* oem_baseline) noexcept override;
    void record(const NodeId& node_id, ActuatorHealth health) noexcept override;

    [[nodiscard]] MutationBatchResult execute(
        const DeviceManifest& manifest, const OemBaseline& baseline,
        const PolicyIntent& intent, const AlgorithmConstants& algorithm) noexcept;
    [[nodiscard]] std::string health_name(const NodeId& id) const;
    [[nodiscard]] bool mutation_blocked() const noexcept { return engine_.mutation_blocked(); }

private:
    BaselineStore& baseline_;
    UnifiedMutationEngine engine_;
    std::unordered_map<NodeId, ActuatorHealth> health_;
    std::unordered_map<NodeId, std::string> paths_;
    bool journal_fault_latched_{};
};

class DirectAdaptiveRuntime final {
public:
    explicit DirectAdaptiveRuntime(BaselineStore& baseline) noexcept;
    ~DirectAdaptiveRuntime();

    DirectAdaptiveRuntime(const DirectAdaptiveRuntime&) = delete;
    DirectAdaptiveRuntime& operator=(const DirectAdaptiveRuntime&) = delete;

    [[nodiscard]] bool start() noexcept;
    void refresh() noexcept;
    void set_mutation_enabled(bool enabled) noexcept;
    void tick(double thermal_pressure = 0.0, bool thermal_valid = false,
              bool thermal_emergency = false,
              bool thermal_guard_active = false) noexcept;
    [[nodiscard]] const DirectAdaptiveSnapshot& snapshot() const noexcept;
    [[nodiscard]] std::string status_json() const;
    [[nodiscard]] bool started() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace coreflow::runtime
