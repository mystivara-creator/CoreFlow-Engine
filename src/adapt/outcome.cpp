#include "coreflow/outcome.hpp"

namespace coreflow {

OutcomeClass classifyOutcome(const ActuatorResult& result) noexcept {
    switch (result.status) {
        case ActuatorStatus::Ok:
            return result.changed() ? OutcomeClass::Neutral : OutcomeClass::Neutral;
        case ActuatorStatus::NoChange:
            return OutcomeClass::Neutral;
        case ActuatorStatus::SafetyRejected:
        case ActuatorStatus::ValidationFailed:
            return OutcomeClass::SafetyBlocked;
        case ActuatorStatus::RolledBack:
            return OutcomeClass::RolledBack;
        case ActuatorStatus::WriteFailed:
        case ActuatorStatus::VerifyFailed:
        case ActuatorStatus::ReadFailed:
            return OutcomeClass::Failed;
        case ActuatorStatus::Unsupported:
        case ActuatorStatus::Unavailable:
        case ActuatorStatus::Invalid:
            return OutcomeClass::Unknown;
    }
    return OutcomeClass::Unknown;
}

const char* outcomeClassName(OutcomeClass result) noexcept {
    switch (result) {
        case OutcomeClass::Unknown: return "UNKNOWN";
        case OutcomeClass::Neutral: return "NEUTRAL";
        case OutcomeClass::Beneficial: return "BENEFICIAL";
        case OutcomeClass::Regressed: return "REGRESSED";
        case OutcomeClass::SafetyBlocked: return "SAFETY_BLOCKED";
        case OutcomeClass::Failed: return "FAILED";
        case OutcomeClass::RolledBack: return "ROLLED_BACK";
    }
    return "UNKNOWN";
}

} // namespace coreflow
