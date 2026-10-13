#include "coreflow/consultation.hpp"
#include "coreflow/fatal_surface.hpp"

#include <cstddef>
#include <string>

namespace coreflow {
namespace {

constexpr std::size_t index(ControlNamespace ns) noexcept {
    return static_cast<std::size_t>(ns);
}

constexpr std::size_t index(ConsultReason reason) noexcept {
    return static_cast<std::size_t>(reason);
}

Consultation makeResult(ConsultVerdict verdict, ConsultReason reason,
                        bool need) noexcept {
    Consultation result;
    result.verdict = verdict;
    result.reason = reason;
    result.need_present = need;
    return result;
}

} // namespace

bool UserGrants::allows(ControlNamespace ns) const noexcept {
    switch (ns) {
        case ControlNamespace::VmSysctl:  return memory;
        case ControlNamespace::BlockQueue: return block_queue;
        case ControlNamespace::Scheduler: return scheduler;
        case ControlNamespace::Gpu:       return gpu;
        case ControlNamespace::Cgroup:    return cgroup;
        case ControlNamespace::CpuFreqTunable: return cpufreq;
        case ControlNamespace::KernelTunable:  return kernel;
        default:                          return false;
    }
}

bool runtimeNeedsControl(RuntimeState state) noexcept {
    return state == RuntimeState::Elevated ||
           state == RuntimeState::Pressure ||
           state == RuntimeState::ThermalGuard;
}

bool isStressedState(RuntimeState state) noexcept {
    return state == RuntimeState::ThermalGuard ||
           state == RuntimeState::Pressure;
}

ControlNamespace namespaceOf(ResourceDomain domain, const std::string& name) noexcept {
    switch (domain) {
        case ResourceDomain::Memory:    return ControlNamespace::VmSysctl;
        case ResourceDomain::Io:        return ControlNamespace::BlockQueue;
        case ResourceDomain::Scheduler: return ControlNamespace::Scheduler;
        case ResourceDomain::Gpu:       return ControlNamespace::Gpu;
        case ResourceDomain::UClamp:
        case ResourceDomain::CpuSet:    return ControlNamespace::Cgroup;
        case ResourceDomain::Vendor:    return ControlNamespace::KernelTunable;
        case ResourceDomain::CpuFreq: {
            // The governor node belongs to MutationController; every other
            // cpufreq node is an ordinary resource tunable.
            const bool governor =
                name == "policies" ||
                (name.size() >= 16U &&
                 name.compare(name.size() - 16U, 16U, "scaling_governor") == 0);
            return governor ? ControlNamespace::CpuFreq : ControlNamespace::CpuFreqTunable;
        }
        default:                        return ControlNamespace::Other;
    }
}

SurfaceClass classifySurface(ResourceDomain domain, const std::string& name,
                             const std::string& path) noexcept {
    if (path.empty()) return SurfaceClass::OutOfClass;
    // The only hard denial: universally fatal surfaces (fatal_surface.hpp).
    if (isFatalSurface(path)) return SurfaceClass::Denied;
    // Inventory/telemetry domains are not autonomous tunables.
    if (!isTunableResourceDomain(domain)) return SurfaceClass::OutOfClass;
    // The CPU governor has its own authority (MutationController).
    if (namespaceOf(domain, name) == ControlNamespace::CpuFreq) return SurfaceClass::OutOfClass;
    if (path.size() >= 16U && path.compare(path.size() - 16U, 16U, "scaling_governor") == 0)
        return SurfaceClass::OutOfClass;
    // Capability-driven: every other discovered kernel tunable is a candidate.
    return SurfaceClass::Admissible;
}

Consultation consult(const ControlFact& fact, RuntimeState state,
                     const UserGrants& grants) noexcept {
    const bool need = runtimeNeedsControl(state);

    // Deny: namespaces with no write path in this build. Build properties and
    // settings are observed, never written, until a separate audited actuator
    // exists for them.
    switch (fact.ns) {
        case ControlNamespace::BuildProp:
        case ControlNamespace::SettingsDb:
            return makeResult(ConsultVerdict::Deny, ConsultReason::NoWriteActuator, need);
        case ControlNamespace::CpuFreq:
        case ControlNamespace::Other:
            return makeResult(ConsultVerdict::Deny, ConsultReason::OutOfScope, need);
        case ControlNamespace::VmSysctl:
        case ControlNamespace::BlockQueue:
        case ControlNamespace::Scheduler:
        case ControlNamespace::Gpu:
        case ControlNamespace::Cgroup:
        case ControlNamespace::CpuFreqTunable:
        case ControlNamespace::KernelTunable:
            break;
    }

    // Deny: kernel-critical surfaces are never admissible.
    if (!fact.surface_admissible) {
        return makeResult(ConsultVerdict::Deny, ConsultReason::SurfaceDenied, need);
    }

    // Defer from here on: each condition may clear later without a code change.
    if (!fact.exists || !fact.readable || !fact.writable || !fact.verified) {
        return makeResult(ConsultVerdict::Defer, ConsultReason::NotObservable, need);
    }
    if (!fact.baseline_captured) {
        return makeResult(ConsultVerdict::Defer, ConsultReason::NoRollbackBaseline, need);
    }
    if (!fact.semantic_known) {
        return makeResult(ConsultVerdict::Defer, ConsultReason::NoSemanticPrior, need);
    }
    if (isStressedState(state) && !fact.stabilizing) {
        return makeResult(ConsultVerdict::Defer, ConsultReason::StressedNonStabilizing, need);
    }
    if (!need) {
        return makeResult(ConsultVerdict::Defer, ConsultReason::NoNeed, need);
    }
    if (!fact.benefit_justified) {
        return makeResult(ConsultVerdict::Defer, ConsultReason::NoBenefitEvidence, need);
    }
    // Permission is asked for last: only when the device needs the change and
    // the change is evidently safe and useful.
    if (!grants.allows(fact.ns)) {
        return makeResult(ConsultVerdict::Defer, ConsultReason::AwaitingUserGrant, need);
    }
    return makeResult(ConsultVerdict::Allow, ConsultReason::Allowed, need);
}

void ConsultationLedger::record(ControlNamespace ns, const Consultation& result) noexcept {
    by_reason[index(result.reason)] += 1U;
    last_reason[index(ns)] = result.reason;
    last_need[index(ns)] = result.need_present;
}

const char* consultVerdictName(ConsultVerdict verdict) noexcept {
    switch (verdict) {
        case ConsultVerdict::Deny:  return "DENY";
        case ConsultVerdict::Defer: return "DEFER";
        case ConsultVerdict::Allow: return "ALLOW";
    }
    return "UNKNOWN";
}

const char* consultReasonName(ConsultReason reason) noexcept {
    switch (reason) {
        case ConsultReason::NoWriteActuator:        return "NO_WRITE_ACTUATOR";
        case ConsultReason::OutOfScope:             return "OUT_OF_SCOPE";
        case ConsultReason::SurfaceDenied:          return "SURFACE_DENIED";
        case ConsultReason::NotObservable:          return "NOT_OBSERVABLE";
        case ConsultReason::NoRollbackBaseline:     return "NO_ROLLBACK_BASELINE";
        case ConsultReason::NoSemanticPrior:        return "NO_SEMANTIC_PRIOR";
        case ConsultReason::StressedNonStabilizing: return "STRESSED_NON_STABILIZING";
        case ConsultReason::NoNeed:                 return "NO_NEED";
        case ConsultReason::NoBenefitEvidence:      return "NO_BENEFIT_EVIDENCE";
        case ConsultReason::AwaitingUserGrant:      return "AWAITING_USER_GRANT";
        case ConsultReason::Allowed:                return "ALLOWED";
    }
    return "UNKNOWN";
}

const char* controlNamespaceName(ControlNamespace ns) noexcept {
    switch (ns) {
        case ControlNamespace::VmSysctl:  return "VM_SYSCTL";
        case ControlNamespace::BlockQueue: return "BLOCK_QUEUE";
        case ControlNamespace::Scheduler: return "SCHEDULER";
        case ControlNamespace::Gpu:       return "GPU";
        case ControlNamespace::Cgroup:    return "CGROUP";
        case ControlNamespace::CpuFreqTunable: return "CPUFREQ_TUNABLE";
        case ControlNamespace::KernelTunable:  return "KERNEL_TUNABLE";
        case ControlNamespace::CpuFreq:   return "CPUFREQ";
        case ControlNamespace::BuildProp: return "BUILD_PROP";
        case ControlNamespace::SettingsDb: return "SETTINGS_DB";
        case ControlNamespace::Other:     return "OTHER";
    }
    return "UNKNOWN";
}

} // namespace coreflow
