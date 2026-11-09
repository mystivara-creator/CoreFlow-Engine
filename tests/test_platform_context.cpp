#include "coreflow/platform_context.hpp"
#include "coreflow/workload_intelligence.hpp"

#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <unistd.h>
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
    FakeProperties hyperos3 = xiaomi;
    hyperos3.values["ro.mi.os.version.name"] = "3.0.1";
    const auto tier1_generation3 = PlatformContextDetector(hyperos3).detect();
    assert(tier1_generation3.tier1_eligible);
    FakeProperties hyperos4 = xiaomi;
    hyperos4.values["ro.mi.os.version.name"] = "4.0.0";
    const auto hyperos4_result = PlatformContextDetector(hyperos4).detect();
    assert(hyperos4_result.identity.os_family == OsFamily::HyperOS);
    assert(!hyperos4_result.tier1_eligible);
    assert(hyperos4_result.execution_tier == ExecutionTier::UniversalFallback);
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

    FakeProperties conflicting_identity;
    conflicting_identity.values = {
        {"ro.product.manufacturer", "Samsung"},
        {"ro.product.brand", "Redmi"},
        {"ro.mi.os.version.name", "3.0"}
    };
    const auto conflict_result = PlatformContextDetector(conflicting_identity).detect();
    assert(conflict_result.identity.vendor == PlatformVendor::Unknown);
    assert(!conflict_result.tier1_eligible);
    assert(conflict_result.execution_tier == ExecutionTier::UniversalFallback);

    FakeProperties weak_xiaomi;
    weak_xiaomi.values = {
        {"ro.product.manufacturer", "Xiaomi"},
        {"ro.mi.os.version.name", "3.0"}
    };
    const auto weak_result = PlatformContextDetector(weak_xiaomi).detect();
    assert(!weak_result.tier1_eligible);
    assert(weak_result.execution_tier == ExecutionTier::UniversalFallback);

    char bpf_root_buffer[] = "/tmp/coreflow-bpf-probe-XXXXXX";
    assert(::mkdtemp(bpf_root_buffer) != nullptr);
    const std::filesystem::path bpf_root(bpf_root_buffer);
    const auto write_fixture = [&](const std::filesystem::path& path, const char* value) {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path);
        output << value;
        assert(output.good());
    };
    write_fixture(bpf_root / "kernel/btf/vmlinux", "btf");
    write_fixture(bpf_root / "kernel/tracing/events/sched/sched_switch/id", "1");
    write_fixture(bpf_root / "kernel/tracing/events/sched/sched_wakeup/id", "2");
    write_fixture(bpf_root / "kernel/tracing/events/block/block_rq_issue/id", "3");
    write_fixture(bpf_root / "kernel/tracing/events/block/block_rq_complete/id", "4");
    std::filesystem::create_directories(bpf_root / "fs/bpf");
    const auto probed = BpfCapabilityProbe(bpf_root.string()).probe();
    assert(probed.btf == CapabilityState::Available);
    assert(probed.tracefs == CapabilityState::Available);
    assert(probed.sched_switch == CapabilityState::Available);
    assert(probed.sched_wakeup == CapabilityState::Available);
    assert(probed.block_request == CapabilityState::Available);
    assert(probed.bpf_filesystem == CapabilityState::Unavailable); // directory is not proof of a bpffs mount
    assert(probed.syscall == CapabilityState::Unknown);
    assert(probed.ring_buffer == CapabilityState::Unknown);
    assert(!probed.event_pipeline_ready());
    assert(select_telemetry_backend(probed, true) == TelemetryBackend::SysfsProcfs);
    std::filesystem::remove_all(bpf_root);

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
