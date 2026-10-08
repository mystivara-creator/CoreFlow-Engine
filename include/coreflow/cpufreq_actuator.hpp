#pragma once

#include <string>
#include <unordered_map>

#include "coreflow/actuator.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

/**
 * CPUFreq adapter for the v1.3 actuator boundary.
 *
 * This class deliberately does NOT choose governors. It adapts the already
 * discovered CpuPolicy/scaling_governor interface to the generic actuator
 * contract so MutationController can later delegate the side effect without
 * moving policy into the actuator.
 *
 * Lifecycle:
 *   setProfile -> discover -> observe/apply/restore
 *
 * discover() is a refresh operation and may perform filesystem I/O.
 * observe/apply/restore are individually bounded to one target.
 */
class CpuFreqActuator final : public IActuator {
public:
    CpuFreqActuator() = default;

    void setProfile(const DeviceProfile* profile) noexcept;

    ActuatorId id() const noexcept override {
        return {ActuatorDomain::CpuFreq, 0};
    }

    ActuatorCapability capability() const noexcept override;

    ActuatorStatus discover() noexcept override;

    ActuatorObservation observe(std::string_view target) noexcept override;

    ActuatorResult apply(
        const ActuatorMutation& mutation
    ) noexcept override;

    ActuatorResult restore(std::string_view target) noexcept override;

    // Restore an explicit value (used for crash recovery from the durable
    // mutation journal, when no in-memory baseline exists yet). The value
    // must be advertised by the target policy and is verified by read-back.
    ActuatorResult restoreTo(std::string_view target, std::string_view value) noexcept;

private:
    struct Baseline {
        std::string value;
        bool valid{false};
    };

    const CpuPolicy* findPolicy(std::string_view target) const noexcept;
    bool targetMatchesPolicy(
        const CpuPolicy& policy,
        std::string_view target
    ) const noexcept;

    static bool readText(
        std::string_view path,
        std::string& value
    ) noexcept;

    static bool writeVerified(
        std::string_view path,
        std::string_view value
    ) noexcept;

    bool targetAllowed(
        const CpuPolicy& policy,
        std::string_view target,
        std::string_view requested
    ) const noexcept;

    const DeviceProfile* profile_{nullptr};
    std::unordered_map<std::string, Baseline> baselines_;

    // Returned string_views are valid until the next operation on this
    // actuator instance.
    std::string observation_current_;
    std::string observation_baseline_;
};

} // namespace coreflow
