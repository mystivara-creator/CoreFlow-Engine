#pragma once

#include <cstdint>
#include <string>

namespace coreflow {

/**
 * Operator-controlled safety holds. Any hold forces observe-only behaviour and
 * restores the factory governor if this daemon previously changed it.
 *
 *   KillSwitch : operator placed a DISABLE file.
 *   SafeMode   : the boot/crash guards placed a SAFE_MODE file, or the
 *                operator placed one. Cleared only by removing the file.
 *
 * Fail-closed: a file whose state cannot be determined (any stat error other
 * than "does not exist") is treated as a hold.
 */
enum class HoldReason : std::uint8_t {
    None = 0,
    KillSwitch,
    SafeMode
};

struct SafetyHoldPaths {
    std::string kill_switch;
    std::string safe_mode;
};

HoldReason evaluateSafetyHold(const SafetyHoldPaths& paths) noexcept;
const char* holdReasonName(HoldReason reason) noexcept;

} // namespace coreflow
