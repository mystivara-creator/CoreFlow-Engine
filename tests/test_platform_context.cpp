#include "coreflow/platform_context.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace coreflow;

class FakeProperties final : public AndroidPropertySource {
public:
    std::unordered_map<std::string, std::string> values;
    bool read(std::string_view key, std::string& value) const noexcept override {
        const auto it = values.find(std::string(key));
        if (it == values.end()) return false;
        value = it->second;
        return true;
    }
};

int main() {
    FakeProperties xiaomi;
    xiaomi.values = {
        {"ro.product.manufacturer", "Xiaomi"},
        {"ro.product.brand", "Redmi"},
        {"ro.product.vendor.manufacturer", "Xiaomi"},
        {"ro.mi.os.version.name", "2.0.204.0"},
        {"ro.build.version.release", "15"},
        {"ro.build.fingerprint", "Xiaomi/kunzite_global/kunzite:15/UP1A:user/release-keys"}
    };
    const auto tier1 = PlatformContextDetector(xiaomi).detect(7);
    assert(tier1.generation == 7);
    assert(tier1.identity.vendor == PlatformVendor::Xiaomi);
    assert(tier1.identity.os_family == OsFamily::HyperOS);
    assert(tier1.identity.os_major == 2);
    assert(tier1.tier1_eligible);
    assert(tier1.execution_tier == ExecutionTier::XiaomiHyperOS);
    assert(tier1.policy_family == PolicyFamily::HyperOS4Inspired);

    FakeProperties miui;
    miui.values = {
        {"ro.product.manufacturer", "Xiaomi"},
        {"ro.product.brand", "Redmi"},
        {"ro.miui.ui.version.name", "V14"},
        {"ro.build.version.release", "14"}
    };
    const auto miui_result = PlatformContextDetector(miui).detect();
    assert(miui_result.identity.os_family == OsFamily::MIUI);
    assert(!miui_result.tier1_eligible);
    assert(miui_result.execution_tier == ExecutionTier::UniversalFallback);

    FakeProperties samsung;
    samsung.values = {
        {"ro.product.manufacturer", "samsung"},
        {"ro.product.brand", "samsung"},
        {"ro.build.version.release", "15"}
    };
    const auto samsung_result = PlatformContextDetector(samsung).detect();
    assert(samsung_result.identity.vendor == PlatformVendor::Samsung);
    assert(samsung_result.execution_tier == ExecutionTier::UniversalFallback);
    assert(samsung_result.policy_family == PolicyFamily::CoreFlowBase);

    FakeProperties weak_xiaomi;
    weak_xiaomi.values = {
        {"ro.product.manufacturer", "Xiaomi"},
        {"ro.mi.os.version.name", "3.0"}
    };
    const auto weak_result = PlatformContextDetector(weak_xiaomi).detect();
    assert(!weak_result.tier1_eligible);
    assert(weak_result.execution_tier == ExecutionTier::UniversalFallback);

    BpfCapabilitySnapshot unavailable{};
    assert(select_telemetry_backend(unavailable, true) == TelemetryBackend::SysfsProcfs);
    assert(select_telemetry_backend(unavailable, false) == TelemetryBackend::Unavailable);
    unavailable.syscall = CapabilityState::Available;
    unavailable.btf = CapabilityState::Available;
    unavailable.tracefs = CapabilityState::Available;
    unavailable.ring_buffer = CapabilityState::Available;
    unavailable.sched_switch = CapabilityState::Available;
    unavailable.sched_wakeup = CapabilityState::Available;
    unavailable.selinux_allows_load = true;
    unavailable.loader_verified = true;
    assert(select_telemetry_backend(unavailable, true) == TelemetryBackend::Ebpf);

    std::cout << "Platform tier detection and telemetry backend selection PASS\n";
    return 0;
}
