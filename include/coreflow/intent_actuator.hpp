#pragma once

#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/baseline_store.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace coreflow {

struct InfrastructurePsiSample final {
    uint64_t monotonic_ns{0};
    double memory_some_avg10{0.0};
    double io_some_avg10{0.0};
    bool memory_valid{false};
    bool io_valid{false};
};

enum class IntentActuatorState : uint8_t {
    Idle, Observing, Retained, RolledBack, Rejected, OwnershipQuarantined, RecoveryRequired
};

struct IntentActuatorSnapshot final {
    IntentActuatorState state{IntentActuatorState::Idle};
    std::string semantic_id;
    std::string path;
    std::string transaction_id;
    std::string reason;
    double before_psi{0.0};
    double after_psi{0.0};
    uint32_t observation_samples{0};
    uint64_t elapsed_ns{0};
};

class IntentActuator final {
public:
    IntentActuator(CentralMutationCoordinator& coordinator,
                   BaselineStore& baseline_store,
                   std::string proc_root = "/proc");
    ~IntentActuator();
    IntentActuator(const IntentActuator&) = delete;
    IntentActuator& operator=(const IntentActuator&) = delete;

    // Start only from a fresh profile and a live PSI bottleneck. Target must
    // satisfy the profile's verified numeric contract. No blind node probing.
    [[nodiscard]] bool begin(DeviceProfileHandle profile,
                             std::string_view semantic_id,
                             std::string_view target_value,
                             const InfrastructurePsiSample& admission);
    // Feed fresh samples. The outcome window is 3-5 seconds and needs >=3 samples.
    [[nodiscard]] IntentActuatorSnapshot observe(const InfrastructurePsiSample& sample);
    [[nodiscard]] bool rollback(std::string_view reason);
    [[nodiscard]] IntentActuatorSnapshot snapshot() const;

private:
    [[nodiscard]] static uint64_t monotonic_now_ns() noexcept;
    [[nodiscard]] bool finish_rollback_locked(std::string_view reason);
    [[nodiscard]] bool read_value_locked(std::string& value) const;
    [[nodiscard]] double selected_psi(const InfrastructurePsiSample& sample) const noexcept;

    CentralMutationCoordinator& coordinator_;
    BaselineStore& baseline_store_;
    std::string proc_root_;
    const std::string controller_id_;
    mutable std::mutex mutex_;
    IntentActuatorSnapshot state_{};
    MutationDomain domain_{MutationDomain::VmMemory};
    std::string baseline_value_;
    std::string target_value_;
    double before_psi_{0.0};
    bool memory_domain_{true};
    uint64_t started_ns_{0};
    std::vector<double> observations_;
};

} // namespace coreflow
