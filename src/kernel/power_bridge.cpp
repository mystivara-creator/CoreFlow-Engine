#include "coreflow/power_bridge.hpp"
#include <unistd.h>
#ifdef __ANDROID__
#include <android/binder_manager.h>
#include <android/binder_ibinder.h>
#endif

namespace coreflow {
PowerCapabilities PowerBridge::probe() const {
    PowerCapabilities result;
    result.binder_device = ::access("/dev/binder", R_OK | W_OK) == 0;
    result.hwbinder_device = ::access("/dev/hwbinder", R_OK | W_OK) == 0;
#ifdef __ANDROID__
    // checkService is nonblocking; missing/denied services remain unavailable.
    AIBinder* binder = AServiceManager_checkService("android.hardware.power.IPower/default");
    if (binder) {
        result.aidl_power_service = true;
        AIBinder_decStrong(binder);
    }
#endif
    return result;
}
std::expected<void, std::string_view> PowerBridge::enable_interception() const {
    return std::unexpected("Requires an OEM/AOSP service integration; no transparent interception backend is installed");
}
}
