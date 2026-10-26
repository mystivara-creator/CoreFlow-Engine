#pragma once

#include <cstdint>
#include <string>

#include "coreflow/actuator.hpp"
#include "coreflow/control.hpp"

namespace coreflow {

enum class OutcomeClass : std::uint8_t {
    Unknown = 0,
    Neutral,
    Beneficial,
    Regressed,
    SafetyBlocked,
    Failed,
    RolledBack
};

struct DecisionOutcome {
    std::uint64_t cycle{0};
    PolicyAction action{PolicyAction::Observe};
    OutcomeClass result{OutcomeClass::Unknown};
    double expected{0.0};
    double observed{0.0};
    std::string reason;
};

OutcomeClass classifyOutcome(const ActuatorResult& result) noexcept;
const char* outcomeClassName(OutcomeClass result) noexcept;

} // namespace coreflow
