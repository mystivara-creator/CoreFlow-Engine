#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "coreflow/actuator.hpp"

namespace coreflow {

// Generic bounded sysfs/procfs resource actuator. It is intentionally narrow:
// callers provide an already-discovered path and a validated requested value.
// All kernel writes are centralized here and every write is read back.
class ResourceActuator final {
public:
    ResourceActuator() = default;

    ActuatorResult apply(const ActuatorMutation& mutation,
                         const MutationPermit& permit) noexcept;
    ActuatorResult restore(std::string_view path,
                           std::string_view baseline) noexcept;

    static bool read(std::string_view path, std::string& value) noexcept;
    // Like read(), but returns the comparable form (see comparableValue).
    static bool readComparable(std::string_view path, std::string& value) noexcept;
    static bool writable(std::string_view path) noexcept;

    // Selector-style sysfs nodes (e.g. queue/scheduler) read back the whole
    // list with the active entry in brackets: "[mq-deadline] kyber none".
    // The kernel accepts only the bare token on write, and the read-back shows
    // the bracketed list. This returns the bracketed token when present, or the
    // trimmed raw value otherwise, so write, verify, and baseline all compare
    // the same kind of value.
    static std::string comparableValue(std::string_view raw);
};

} // namespace coreflow
