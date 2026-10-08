#include "coreflow/safety_hold.hpp"

#include <cerrno>
#include <sys/stat.h>

namespace coreflow {
namespace {

// Returns true when the marker file is present or its state is unknown.
bool markerActive(const std::string& path) noexcept {
    if (path.empty()) return false;
    struct stat st {};
    if (::stat(path.c_str(), &st) == 0) return true;
    return errno != ENOENT;
}

} // namespace

HoldReason evaluateSafetyHold(const SafetyHoldPaths& paths) noexcept {
    if (markerActive(paths.kill_switch)) return HoldReason::KillSwitch;
    if (markerActive(paths.safe_mode)) return HoldReason::SafeMode;
    return HoldReason::None;
}

const char* holdReasonName(HoldReason reason) noexcept {
    switch (reason) {
        case HoldReason::None: return "NONE";
        case HoldReason::KillSwitch: return "KILL_SWITCH";
        case HoldReason::SafeMode: return "SAFE_MODE";
    }
    return "UNKNOWN";
}

} // namespace coreflow
