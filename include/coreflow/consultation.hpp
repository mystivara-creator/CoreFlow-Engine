#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "coreflow/environment.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

// Consultation is the question the eyes ask the brain before any control is
// touched: "may this be changed, is it safe and effective, and does the device
// need it right now?" The answer is one of three verdicts:
//
//   Allow  - all checks pass and the user has granted this namespace.
//   Defer  - not now. The control may become eligible when the reason clears
//            (for example, when the device actually needs it). Stay observe-only.
//   Deny   - never, in this build. Observe-only namespaces and kernel-critical
//            surfaces land here.
//
// Eyes supply facts only. This module decides; no caller may turn a Defer into
// a write without going through it.

enum class ControlNamespace : std::uint8_t {
    VmSysctl = 0,   // /proc/sys/vm/*
    BlockQueue,     // /sys/block/<physical>/queue/*
    Scheduler,      // scheduler tokens
    CpuFreq,        // separate authority path; out of scope here
    BuildProp,      // system properties; observe-only in this build
    SettingsDb,     // Android settings providers; observe-only in this build
    Other,
};

inline constexpr std::size_t kControlNamespaceCount =
    static_cast<std::size_t>(ControlNamespace::Other) + 1U;

enum class ConsultVerdict : std::uint8_t {
    Deny = 0,
    Defer,
    Allow,
};

enum class ConsultReason : std::uint8_t {
    // Deny
    NoWriteActuator = 0,     // namespace has no write path in this build
    OutOfScope,              // belongs to another authority (CPU governor, unknown)
    SurfaceDenied,           // actuator surface guard refuses this path
    // Defer
    NotObservable,           // eye lacks readable/writable/verified evidence
    NoRollbackBaseline,      // no captured baseline to restore from
    NoSemanticPrior,         // no reviewed meaning for this control
    StressedNonStabilizing,  // stressed context accepts only load-reducing writes
    NoNeed,                  // runtime state shows no demand right now
    NoBenefitEvidence,       // predicted benefit does not exceed risk
    AwaitingUserGrant,       // everything else passes; user has not permitted it
    // Allow
    Allowed,
};

inline constexpr std::size_t kConsultReasonCount =
    static_cast<std::size_t>(ConsultReason::Allowed) + 1U;

// Surface classification answers "is this path a candidate at all". Discovery
// asks it, so the eye never decides admissibility on its own.
enum class SurfaceClass : std::uint8_t {
    Admissible = 0,
    Denied,      // kernel-critical; never admissible
    OutOfClass,  // not a surface this build understands
};

// Facts the eye reports for one candidate, plus the brain's own evaluations
// (surface class, semantic knowledge, benefit/risk) that the caller supplies.
struct ControlFact {
    ControlNamespace ns{ControlNamespace::Other};
    bool exists{false};
    bool readable{false};
    bool writable{false};
    bool verified{false};            // read-back confirmed the node at runtime
    bool baseline_captured{false};   // factory value recorded for restore
    // Result of the actuator-level surface guard (the same exact allow-list the
    // write path enforces), not the broader discovery candidacy rule.
    bool surface_admissible{false};
    bool semantic_known{false};      // EffectModel has a reviewed direction/bound
    bool stabilizing{false};         // change moves a stressed knob toward relief
    // EffectModel's own judgment that the change is worth making and bounded:
    // benefit meets its minimum and risk stays within its maximum. Consultation
    // reuses that single definition rather than inventing a second threshold.
    bool benefit_justified{false};
};

// Explicit user permission, per namespace. Defaults to none. A device need
// alone never grants permission.
struct UserGrants {
    bool memory{false};       // permits ControlNamespace::VmSysctl
    bool block_queue{false};  // permits ControlNamespace::BlockQueue
    bool scheduler{false};    // permits ControlNamespace::Scheduler

    bool allows(ControlNamespace ns) const noexcept;
};

struct Consultation {
    ConsultVerdict verdict{ConsultVerdict::Deny};
    ConsultReason reason{ConsultReason::NoWriteActuator};
    // Whether the runtime currently shows a need. Lets status explain a Defer
    // of the form "needed, awaiting permission" versus "not needed now".
    bool need_present{false};
};

// The device needs a control when it is under load or stress. Idle, Normal and
// Warming never justify a proactive write.
bool runtimeNeedsControl(RuntimeState state) noexcept;

// Stressed states accept only stabilizing (load-reducing) changes.
bool isStressedState(RuntimeState state) noexcept;

// Brain's answer to the eye: which namespace a resource belongs to.
ControlNamespace namespaceOf(ResourceDomain domain, const std::string& name) noexcept;

// Structural surface classification. Pure function of domain, name and path.
// Behaviour is identical to the discovery-side rules it replaces.
SurfaceClass classifySurface(ResourceDomain domain, const std::string& name,
                             const std::string& path) noexcept;

Consultation consult(const ControlFact& fact, RuntimeState state,
                     const UserGrants& grants) noexcept;

// Bounded, allocation-free record of how consultations ended. Safe to update
// from noexcept paths and cheap to export.
struct ConsultationLedger {
    std::array<std::uint32_t, kConsultReasonCount> by_reason{};
    std::array<ConsultReason, kControlNamespaceCount> last_reason{};
    std::array<bool, kControlNamespaceCount> last_need{};

    void record(ControlNamespace ns, const Consultation& result) noexcept;
};

const char* consultVerdictName(ConsultVerdict verdict) noexcept;
const char* consultReasonName(ConsultReason reason) noexcept;
const char* controlNamespaceName(ControlNamespace ns) noexcept;

} // namespace coreflow
