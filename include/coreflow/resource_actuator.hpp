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
    static bool writable(std::string_view path) noexcept;
};

} // namespace coreflow
