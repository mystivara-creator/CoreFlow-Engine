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


    // Ambiguous names must not imply unrelated high-capability domains.
    EnvironmentCapabilityMatrix negativeMatrix;
    negativeMatrix.resources.push_back(ResourceCapability{
        ResourceDomain::AndroidRuntime, "gpu_temperature",
        "/sys/devices/gpu_temperature",
        true, true, false, false, false, true});
    negativeMatrix.resources.push_back(ResourceCapability{
        ResourceDomain::Memory, "swap_free", "/proc/meminfo/swap_free",
        true, true, false, false, false, true});
    negativeMatrix.resources.push_back(ResourceCapability{
        ResourceDomain::AndroidRuntime, "block_device_count",
        "/sys/devices/block_device_count",
        true, true, false, false, false, true});
    negativeMatrix.resources.push_back(ResourceCapability{
        ResourceDomain::AndroidRuntime, "thermal_policy",
        "/sys/devices/thermal_policy",
        true, true, false, false, false, true});
    negativeMatrix.resources.push_back(ResourceCapability{
        ResourceDomain::AndroidRuntime, "queue_depth",
        "/sys/devices/queue_depth",
        true, true, false, false, false, true});

    const auto negativeStatuses = HighCapabilityExpansion::assess(negativeMatrix);
    for (const auto& status : negativeStatuses) {
        if (status.id == ExpandedDomain::GpuFrequency ||
            status.id == ExpandedDomain::Zram ||
            status.id == ExpandedDomain::ThermalIntelligence ||
            status.id == ExpandedDomain::FilesystemIo) {
            CHECK(status.matched_resources == 0U);
            CHECK(status.evidence == CapabilityEvidence::Unavailable);
        }
        CHECK(!status.mutation_authorized);
    }

    // Representative valid resource names and paths must still be recognized.
    EnvironmentCapabilityMatrix positiveMatrix;
    positiveMatrix.resources.push_back(ResourceCapability{
        ResourceDomain::Gpu, "gpu_freq",
        "/sys/devices/platform/gpu_freq",
        true, true, false, false, false, true});
    positiveMatrix.resources.push_back(ResourceCapability{
        ResourceDomain::Memory, "zram0_mm_stat", "/sys/block/zram0/mm_stat",
        true, true, false, false, false, true});
    positiveMatrix.resources.push_back(ResourceCapability{
        ResourceDomain::Thermal, "thermal_zone0",
        "/sys/class/thermal/thermal_zone0/temp",
        true, true, false, false, false, true});
    positiveMatrix.resources.push_back(ResourceCapability{
        ResourceDomain::Io, "nr_requests",
        "/sys/block/sda/queue/nr_requests",
        true, true, false, false, false, true});

    const auto positiveStatuses = HighCapabilityExpansion::assess(positiveMatrix);
    for (const auto& status : positiveStatuses) {
        if (status.id == ExpandedDomain::GpuFrequency ||
            status.id == ExpandedDomain::Zram ||
            status.id == ExpandedDomain::ThermalIntelligence ||
            status.id == ExpandedDomain::FilesystemIo) {
            if (status.matched_resources != 1U) {
                std::cerr << "Unexpected match count for domain "
                          << static_cast<unsigned int>(status.id)
                          << ": " << status.matched_resources << "\n";
                ++failures;
            }
            CHECK(status.evidence == CapabilityEvidence::Observable);
        }
        CHECK(!status.mutation_authorized);
    }

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "High capability expansion tests passed\n";
    return EXIT_SUCCESS;
}
