#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "coreflow/actuator.hpp"

namespace coreflow {

// Generic sysfs/procfs/cgroup resource actuator. Callers provide a discovered
// path and candidate value. The final write gate checks the fatal-surface policy,
// the opened descriptor, and the kernel pseudo-filesystem class; every write is
// read back. Host tests emulate these controls with ordinary temporary files.
class ResourceActuator final {
public:
    ResourceActuator() = default;

    ActuatorCapability capability(ActuatorId id, std::string_view target) const noexcept;
    ActuatorResult apply(const ActuatorMutation& mutation,
                         const MutationPermit& permit) noexcept;
    ActuatorResult restore(std::string_view path,
                           std::string_view baseline) noexcept;

    static bool read(std::string_view path, std::string& value) noexcept;
    // Like read(), but returns the comparable form (see comparableValue).
    static bool readComparable(std::string_view path, std::string& value) noexcept;
    static bool writable(std::string_view path) noexcept;
    // Path-level gate shared by proposal and execution. It rejects the
    // universally fatal classes and the CPU governor node (owned by the
    // MutationController); writable()/the final descriptor gate additionally
    // require an actual supported kernel tunable filesystem.
    static bool mutationTargetAllowed(std::string_view path) noexcept;

    // Selector-style sysfs nodes (e.g. queue/scheduler) read back the whole
    // list with the active entry in brackets: "[mq-deadline] kyber none".
    // The kernel accepts only the bare token on write, and the read-back shows
    // the bracketed list. This returns the bracketed token when present, or the
    // trimmed raw value otherwise, so write, verify, and baseline all compare
    // the same kind of value.
    static std::string comparableValue(std::string_view raw);
};

} // namespace coreflow
