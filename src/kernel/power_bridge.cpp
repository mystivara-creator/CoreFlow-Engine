#include "coreflow/power_bridge.hpp"
#include <unistd.h>
#ifdef __ANDROID__
#include <dlfcn.h>
struct AIBinder;
#endif

namespace coreflow {
PowerCapabilities PowerBridge::probe() const {
    PowerCapabilities result;
    result.binder_device = ::access("/dev/binder", R_OK | W_OK) == 0;
    result.hwbinder_device = ::access("/dev/hwbinder", R_OK | W_OK) == 0;
#ifdef __ANDROID__
    // Service-manager interfaces are platform APIs, not public NDK headers.
    // Probe optional exported C symbols without depending on private headers or
    // vendor-generated AIDL stubs. Keep Binder loaded for the process lifetime.
    static void* library = ::dlopen("libbinder_ndk.so", RTLD_NOW | RTLD_LOCAL);
    if (library) {
        using Check = AIBinder* (*)(const char*);
        using Release = void (*)(AIBinder*);
        const auto check = reinterpret_cast<Check>(::dlsym(library, "AServiceManager_checkService"));
        const auto release = reinterpret_cast<Release>(::dlsym(library, "AIBinder_decStrong"));
        if (check && release) {
            if (auto* binder = check("android.hardware.power.IPower/default")) {
                result.aidl_power_service = true;
                release(binder);
            }
        }
    }
#endif
    return result;
}
std::expected<void, std::string_view> PowerBridge::enable_interception() const {
    return std::unexpected("Requires an OEM/AOSP service integration; no transparent interception backend is installed");
}
}
