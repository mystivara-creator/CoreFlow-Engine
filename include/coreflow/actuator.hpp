#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace coreflow {

/**
 * CoreFlow v1.3 actuator contract.
 *
 * This layer deliberately contains no Linux/sysfs implementation.
 * It defines the safety and lifecycle vocabulary shared by future actuator
 * domains such as CPUFreq, uclamp, cpuset, I/O and VM.
 *
 * Design constraints:
 * - bounded state; no dynamic ownership is required by the contract
 * - observation and mutation are separate operations
 * - verification is part of the mutation result
 * - an actuator may remain observational-only
 * - failure is represented as data, never as an exception across the engine
 */
enum class ActuatorDomain : std::uint8_t {
    CpuFreq = 0,
    Uclamp,
    CpuSet,
    Io,
    Vm,
};

enum class ActuatorMode : std::uint8_t {
    Observational = 0,
    Autonomous,
};

enum class ActuatorStatus : std::uint8_t {
    Ok = 0,
    Unsupported,
    Unavailable,
    Invalid,
    ReadFailed,
    ValidationFailed,
    SafetyRejected,
    WriteFailed,
    VerifyFailed,
    RolledBack,
    NoChange,
};

struct ActuatorId {
    ActuatorDomain domain{ActuatorDomain::CpuFreq};
    std::uint16_t instance{0};

    constexpr bool operator==(const ActuatorId& other) const noexcept {
        return domain == other.domain && instance == other.instance;
    }
};

struct ActuatorCapability {
    ActuatorId id{};
    ActuatorMode mode{ActuatorMode::Observational};

    // A mutation is permitted only when all applicable capabilities are true.
    bool readable{false};
    bool writable{false};
    bool verifiable{false};
    bool rollbackable{false};

    // Hard upper bound for one decision cycle. Zero means "no mutation".
    std::uint8_t max_writes_per_cycle{0};

    constexpr bool mutationSafe() const noexcept {
        return mode == ActuatorMode::Autonomous &&
               readable &&
               writable &&
               verifiable &&
               rollbackable &&
               max_writes_per_cycle != 0;
    }
};

struct ActuatorValue {
    std::string_view value{};
    bool valid{false};
};

struct ActuatorObservation {
    ActuatorId id{};
    ActuatorValue current{};
    ActuatorValue baseline{};
    bool baseline_valid{false};
};

struct ActuatorMutation {
    ActuatorId id{};
    std::string_view requested{};
};

struct ActuatorResult {
    ActuatorStatus status{ActuatorStatus::Invalid};
    std::uint8_t writes_attempted{0};
    std::uint8_t writes_verified{0};

    constexpr bool succeeded() const noexcept {
        return status == ActuatorStatus::Ok ||
               status == ActuatorStatus::NoChange;
    }

    constexpr bool changed() const noexcept {
        return status == ActuatorStatus::Ok &&
               writes_verified != 0;
    }
};

/**
 * Minimal actuator boundary.
 *
 * Implementations own their platform-specific discovery and state handling.
 * The engine owns policy: whether a mutation is allowed and when it should
 * happen. This prevents an actuator from silently becoming its own policy
 * engine.
 *
 * Implementations must not perform persistent logging here and should avoid
 * heap allocation on the normal observation path.
 */
class IActuator {
public:
    virtual ~IActuator() = default;

    virtual ActuatorId id() const noexcept = 0;

    virtual ActuatorCapability capability() const noexcept = 0;

    /**
     * Refresh platform state and establish capability information.
     * This operation may touch the filesystem during discovery/refresh and
     * therefore must not be called from the hot observation path.
     */
    virtual ActuatorStatus discover() noexcept = 0;

    /**
     * Read current state and return a bounded, non-owning view.
     * The returned view remains valid only until the next actuator operation.
     */
    virtual ActuatorObservation observe() noexcept = 0;

    /**
     * Apply one already safety-approved mutation and verify it by read-back.
     *
     * The implementation must:
     * 1. reject unsupported/unverified targets,
     * 2. avoid a write when current == requested,
     * 3. enforce its write bound,
     * 4. verify the resulting state,
     * 5. roll back when verification fails and rollback is possible.
     *
     * This method must never be used as a substitute for policy evaluation.
     */
    virtual ActuatorResult apply(const ActuatorMutation& mutation) noexcept = 0;

    /**
     * Restore the actuator's captured baseline.
     *
     * A failed restore is reported through ActuatorStatus and must never
     * terminate the daemon.
     */
    virtual ActuatorResult restore() noexcept = 0;
};

constexpr std::string_view actuatorStatusName(ActuatorStatus status) noexcept {
    switch (status) {
        case ActuatorStatus::Ok:              return "OK";
        case ActuatorStatus::Unsupported:     return "UNSUPPORTED";
        case ActuatorStatus::Unavailable:     return "UNAVAILABLE";
        case ActuatorStatus::Invalid:         return "INVALID";
        case ActuatorStatus::ReadFailed:      return "READ_FAILED";
        case ActuatorStatus::ValidationFailed:return "VALIDATION_FAILED";
        case ActuatorStatus::SafetyRejected:  return "SAFETY_REJECTED";
        case ActuatorStatus::WriteFailed:     return "WRITE_FAILED";
        case ActuatorStatus::VerifyFailed:    return "VERIFY_FAILED";
        case ActuatorStatus::RolledBack:      return "ROLLED_BACK";
        case ActuatorStatus::NoChange:        return "NO_CHANGE";
    }
    return "UNKNOWN";
}

} // namespace coreflow
