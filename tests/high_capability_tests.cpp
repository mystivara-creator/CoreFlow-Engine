#include "coreflow/high_capability.hpp"

#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::cerr << "FAIL: " #x "\n"; } } while (false)
}

int main() {
    using namespace coreflow;

    CHECK(HighCapabilityExpansion::catalog().size() == 15U);

    EnvironmentCapabilityMatrix matrix;
    matrix.resources.push_back(ResourceCapability{
        ResourceDomain::CpuFreq, "cpu.idle", "/sys/devices/system/cpu/cpuidle",
        true, true, false, false, false, true});
    matrix.resources.push_back(ResourceCapability{
        ResourceDomain::Gpu, "devfreq.gpu", "/sys/class/devfreq/gpu",
        true, true, false, false, false, true});
    matrix.resources.push_back(ResourceCapability{
        ResourceDomain::Power, "ps.battery.capacity", "/sys/class/power_supply/battery/capacity",
        true, true, false, false, false, true});
    matrix.resources.push_back(ResourceCapability{
        ResourceDomain::Memory, "memory.pressure", "/sys/fs/cgroup/memory.pressure",
        true, false, true, true, false, false});
    matrix.resources.push_back(ResourceCapability{
        ResourceDomain::AndroidRuntime, "netdev.rx_bytes", "/sys/class/net/wlan0/statistics/rx_bytes",
        true, true, false, false, false, true});
    matrix.resources.push_back(ResourceCapability{
        ResourceDomain::AndroidRuntime, "cpu.online", "/sys/devices/system/cpu/online",
        true, true, false, false, false, true});

    const auto statuses = HighCapabilityExpansion::assess(matrix);
    CHECK(statuses.size() == 15U);

    bool sawCpuIdle = false;
    bool sawDevfreq = false;
    bool sawBattery = false;
    bool sawPsiDetected = false;
    bool sawNetwork = false;
    bool sawTopology = false;
    for (const auto& status : statuses) {
        CHECK(!status.mutation_authorized);
        if (status.id == ExpandedDomain::CpuIdle) {
            sawCpuIdle = true;
            CHECK(status.evidence == CapabilityEvidence::Observable);
        } else if (status.id == ExpandedDomain::Devfreq) {
            sawDevfreq = true;
            CHECK(status.evidence == CapabilityEvidence::Observable);
        } else if (status.id == ExpandedDomain::BatteryEnergy) {
            sawBattery = true;
            CHECK(status.evidence == CapabilityEvidence::Observable);
        } else if (status.id == ExpandedDomain::PressureStall) {
            sawPsiDetected = true;
            CHECK(status.evidence == CapabilityEvidence::Detected);
        } else if (status.id == ExpandedDomain::NetworkTelemetry) {
            sawNetwork = true;
            CHECK(status.evidence == CapabilityEvidence::Observable);
        } else if (status.id == ExpandedDomain::CpuTopology) {
            sawTopology = true;
            CHECK(status.evidence == CapabilityEvidence::Observable);
        }
    }
    CHECK(sawCpuIdle);
    CHECK(sawDevfreq);
    CHECK(sawBattery);
    CHECK(sawPsiDetected);
    CHECK(sawNetwork);
    CHECK(sawTopology);
    CHECK(HighCapabilityExpansion::evidenceName(CapabilityEvidence::Unavailable) ==
          std::string("unavailable"));

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "High capability expansion tests passed\n";
    return EXIT_SUCCESS;
}
