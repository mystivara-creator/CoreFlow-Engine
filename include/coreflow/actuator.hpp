#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace coreflow {

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
    bool readable{false};
    bool writable{false};
    bool verifiable{false};
    bool rollbackable{false};
    std::uint8_t max_writes_per_cycle{0};

    constexpr bool mutationSafe() const noexcept {
        return mode == ActuatorMode::Autonomous &&
               readable && writable && verifiable &&
               rollbackable && max_writes_per_cycle != 0;
    }
};

struct ActuatorObservation {
    ActuatorId id{};
    std::string_view target{};
    std::string_view current{};
    std::string_view baseline{};
    bool valid{false};
    bool baseline_valid{false};
};

struct ActuatorMutation {
    ActuatorId id{};
    std::string_view target{};
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

class IActuator {
public:
    virtual ~IActuator() = default;

    virtual ActuatorId id() const noexcept = 0;
    virtual ActuatorCapability capability() const noexcept = 0;

    // Discovery/refresh may perform filesystem I/O. Never call from the
    // normal sampling hot path.
    virtual ActuatorStatus discover() noexcept = 0;

    // Observation must not mutate kernel state.
    virtual ActuatorObservation observe(std::string_view target) noexcept = 0;

    // The caller owns policy selection. This method only performs a bounded,
    // already-approved mutation and read-back verification.
    virtual ActuatorResult apply(
        const ActuatorMutation& mutation
    ) noexcept = 0;

    virtual ActuatorResult restore(
        std::string_view target
    ) noexcept = 0;
};

constexpr std::string_view actuatorStatusName(
    ActuatorStatus status
) noexcept {
    switch (status) {
        case ActuatorStatus::Ok:               return "OK";
        case ActuatorStatus::Unsupported:      return "UNSUPPORTED";
        case ActuatorStatus::Unavailable:      return "UNAVAILABLE";
        case ActuatorStatus::Invalid:          return "INVALID";
        case ActuatorStatus::ReadFailed:       return "READ_FAILED";
        case ActuatorStatus::ValidationFailed: return "VALIDATION_FAILED";
        case ActuatorStatus::SafetyRejected:   return "SAFETY_REJECTED";
        case ActuatorStatus::WriteFailed:      return "WRITE_FAILED";
        case ActuatorStatus::VerifyFailed:     return "VERIFY_FAILED";
        case ActuatorStatus::RolledBack:      return "ROLLED_BACK";
        case ActuatorStatus::NoChange:        return "NO_CHANGE";
    }
    return "UNKNOWN";
}

} // namespace coreflow
