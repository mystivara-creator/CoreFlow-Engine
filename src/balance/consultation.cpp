#include "coreflow/consultation.hpp"

#include <cstddef>
#include <string>

namespace coreflow {
namespace {

bool isDeniedSurface(const std::string& path, const std::string& name) noexcept {
    // Kernel-critical controls that no policy may ever write. The list names
    // operations and panic/sysrq surfaces, not parameters, so unknown future
    // controls with the same semantics are still denied.
    static constexpr const char* kDeniedSuffixes[] = {
        "drop_caches",
        "compact_memory",
        "stat_refresh",
        "overcommit_memory",
        "overcommit_ratio",
        "panic",
        "panic_on_oom",
        "sysrq",
    };
    for (const char* suffix : kDeniedSuffixes) {
        const std::size_t n = std::char_traits<char>::length(suffix);
        if (name.size() >= n &&
            name.compare(name.size() - n, n, suffix) == 0) {
            return true;
        }
        if (path.size() >= n &&
            path.compare(path.size() - n, n, suffix) == 0) {
            return true;
        }
    }
    if (path.find("/drop_caches") != std::string::npos) return true;
    if (path.find("/compact_memory") != std::string::npos) return true;
    if (path.find("memory.force_empty") != std::string::npos) return true;
    if (path.find("cgroup.kill") != std::string::npos) return true;
    if (path.rfind("/proc/sys/kernel/sysrq", 0) == 0) return true;
    if (path.rfind("/proc/sys/kernel/panic", 0) == 0) return true;
    return false;
}

bool isVirtualBlockDevice(const std::string& name) noexcept {
    return name.rfind("loop", 0) == 0 ||
           name.rfind("dm-", 0) == 0 ||
           name.rfind("zram", 0) == 0 ||
           name.rfind("ram", 0) == 0 ||
           name.rfind("ubiblock", 0) == 0 ||
           name.rfind("md", 0) == 0 ||
           name == "zram0";
}

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
    (void)name;
    switch (domain) {
        case ResourceDomain::Memory:    return ControlNamespace::VmSysctl;
        case ResourceDomain::Io:        return ControlNamespace::BlockQueue;
        case ResourceDomain::Scheduler: return ControlNamespace::Scheduler;
        case ResourceDomain::CpuFreq:   return ControlNamespace::CpuFreq;
        default:                        return ControlNamespace::Other;
    }
}

SurfaceClass classifySurface(ResourceDomain domain, const std::string& name,
                             const std::string& path) noexcept {
    if (path.empty()) return SurfaceClass::OutOfClass;
    if (isDeniedSurface(path, name)) return SurfaceClass::Denied;

    // Class: Linux VM sysctl surface. Any discovered node under /proc/sys/vm/
    // is admissible as a candidate. The actuator's exact allow-list and the
    // EffectModel still decide whether a specific write runs.
    if (domain == ResourceDomain::Memory &&
        path.rfind("/proc/sys/vm/", 0) == 0) {
        return SurfaceClass::Admissible;
    }

    // Class: physical block-queue numeric tunables. Virtual devices are
    // excluded. Scheduler tokens need their own model and stay out of this class.
    if (domain == ResourceDomain::Io) {
        constexpr const char* prefix = "/sys/block/";
        if (path.rfind(prefix, 0) != 0) return SurfaceClass::OutOfClass;
        const auto rest = path.substr(std::char_traits<char>::length(prefix));
        const auto slash = rest.find('/');
        if (slash == std::string::npos) return SurfaceClass::OutOfClass;
        const std::string device = rest.substr(0, slash);
        if (isVirtualBlockDevice(device)) return SurfaceClass::OutOfClass;
        const std::string tail = rest.substr(slash);
        return (tail == "/queue/read_ahead_kb" || tail == "/queue/nr_requests")
                   ? SurfaceClass::Admissible
                   : SurfaceClass::OutOfClass;
    }

    return SurfaceClass::OutOfClass;
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
        case ControlNamespace::CpuFreq:   return "CPUFREQ";
        case ControlNamespace::BuildProp: return "BUILD_PROP";
        case ControlNamespace::SettingsDb: return "SETTINGS_DB";
        case ControlNamespace::Other:     return "OTHER";
    }
    return "UNKNOWN";
}

} // namespace coreflow
